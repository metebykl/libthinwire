#ifndef THINWIRE_H
#define THINWIRE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TWDEF
#define TWDEF
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _WIN32
typedef int socklen_t;
#define close(fd) closesocket(fd)

#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
#endif

typedef enum { TW_INFO, TW_WARNING, TW_ERROR } tw_log_level;

TWDEF void tw_log(tw_log_level level, const char *fmt, ...);

#ifndef TW_DEFAULT_PORT
#define TW_DEFAULT_PORT 8080
#endif

#ifndef TW_MAX_CLIENTS
#define TW_MAX_CLIENTS 100
#endif

typedef struct {
  int fd;
  struct sockaddr_in addr;
} tw_conn;

typedef struct {
  int fd;
  struct sockaddr_in addr;

  struct pollfd fds[TW_MAX_CLIENTS + 1];
  tw_conn conns[TW_MAX_CLIENTS + 1];
  int nfds;
} tw_server;

#ifndef TW_MAX_HEADERS
#define TW_MAX_HEADERS 64
#endif

#ifndef TW_MAX_HEADER_NAME
#define TW_MAX_HEADER_NAME 256
#endif

#ifndef TW_MAX_HEADER_VALUE
#define TW_MAX_HEADER_VALUE 8192
#endif

typedef struct {
  char name[TW_MAX_HEADER_NAME];
  char value[TW_MAX_HEADER_VALUE];
} tw_header;

#ifndef TW_MAX_REQUEST_SIZE
#define TW_MAX_REQUEST_SIZE 8192
#endif

typedef struct {
  char method[64];
  size_t method_len;

  char version[16];
  size_t version_len;

  char path[1024];
  size_t path_len;

  tw_header headers[TW_MAX_HEADERS];
  size_t header_count;

  bool keep_alive;
} tw_request;

typedef struct {
  int status_code;

  const char *body;
  size_t body_len;

  tw_header headers[TW_MAX_HEADERS];
  size_t header_count;
} tw_response;

typedef void (*tw_request_handler_fn)(tw_conn *conn, tw_request *req,
                                      tw_response *res);

TWDEF bool tw_server_init(tw_server *server, int port);
TWDEF bool tw_server_run(tw_server *server, tw_request_handler_fn handler);
TWDEF bool tw_server_stop(tw_server *server);
TWDEF bool tw__set_nonblocking(int fd);

TWDEF ssize_t tw_conn_read(tw_conn *conn, char *buf, size_t len);
TWDEF ssize_t tw_conn_write(tw_conn *conn, const char *buf, size_t len);
TWDEF void tw_conn_close(tw_conn *conn);

TWDEF bool tw_request_parse(const char *buf, tw_request *req);
TWDEF const char *tw_request_get_header(tw_request *req, const char *name);

TWDEF void tw_response_set_header(tw_response *res, const char *name,
                                  const char *value);
TWDEF bool tw_response_send(tw_conn *conn, tw_response *res);

#ifdef __cplusplus
}
#endif

#endif  // THINWIRE_H

#ifdef THINWIRE_IMPL

TWDEF void tw_log(tw_log_level level, const char *fmt, ...) {
  FILE *stream = stdout;

  switch (level) {
    case TW_INFO:
      fprintf(stream, "[INFO] ");
      break;
    case TW_WARNING:
      fprintf(stream, "[WARNING] ");
      break;
    case TW_ERROR:
      stream = stderr;
      fprintf(stream, "[ERROR] ");
      break;
    default:
      break;
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(stream, fmt, args);
  va_end(args);
  fprintf(stream, "\n");
}

TWDEF bool tw_server_init(tw_server *server, int port) {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    tw_log(TW_ERROR, "WSAStartup failed");
    return false;
  }
#endif

  if ((server->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    tw_log(TW_ERROR, "Socket failed");
    return false;
  }

  if (port <= 0) {
    port = TW_DEFAULT_PORT;
  }

  server->addr.sin_family = AF_INET;
  server->addr.sin_addr.s_addr = INADDR_ANY;
  server->addr.sin_port = htons(port);

  if (bind(server->fd, (struct sockaddr *)&server->addr, sizeof(server->addr)) <
      0) {
    tw_log(TW_ERROR, "Socket bind failed");
    return false;
  }

  if (listen(server->fd, 10) < 0) {
    tw_log(TW_ERROR, "Socket listen failed");
    return false;
  }

  if (!tw__set_nonblocking(server->fd)) {
    return false;
  }

  server->nfds = 1;
  memset(server->fds, 0, sizeof(server->fds));
  server->fds[0].fd = server->fd;
  server->fds[0].events = POLLIN;

  return true;
}

TWDEF bool tw_server_run(tw_server *server, tw_request_handler_fn handler) {
  while (1) {
    int ret = poll(server->fds, server->nfds, -1);
    if (ret < 0) {
      tw_log(TW_ERROR, "Poll failed");
      return false;
    }

    if (server->fds[0].revents & (POLLIN | POLLERR | POLLHUP)) {
      while (server->nfds < TW_MAX_CLIENTS + 1) {
        tw_conn conn;
        socklen_t addr_len = sizeof(conn.addr);
#ifdef _WIN32
        SOCKET conn_fd =
            accept(server->fd, (struct sockaddr *)&conn.addr, &addr_len);
        if (conn_fd == INVALID_SOCKET) {
          int werr = WSAGetLastError();
          if (werr == WSAEWOULDBLOCK) {
            /* no more pending connections */
            break;
          } else {
            tw_log(TW_ERROR, "accept failed: %d", werr);
            break;
          }
        }
        conn.fd = (int)conn_fd;
#else
        int conn_fd =
            accept(server->fd, (struct sockaddr *)&conn.addr, &addr_len);
        if (conn_fd < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* no more pending connections */
            break;
          } else {
            tw_log(TW_ERROR, "accept failed: %s", strerror(errno));
            break;
          }
        }
        conn.fd = conn_fd;
#endif

        tw__set_nonblocking(conn_fd);

        server->conns[server->nfds] = conn;
        server->fds[server->nfds].fd = conn_fd;
        server->fds[server->nfds].events = POLLIN;
        server->fds[server->nfds].revents = 0;
        server->nfds++;
      }

      server->fds[0].revents = 0;
    }

    for (int i = 1; i < server->nfds; i++) {
      tw_conn *conn = &server->conns[i];
      short revents = server->fds[i].revents;

      if (!(revents & POLLIN)) {
        if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
          tw_conn_close(conn);
#ifdef _WIN32
          server->fds[i].fd = (SOCKET)-1;
#else
          server->fds[i].fd = -1;
#endif
          conn->fd = -1;
        }
        server->fds[i].revents = 0;
        continue;
      }

      bool keep_alive = true;
      while (keep_alive) {
        char buf[TW_MAX_REQUEST_SIZE];
        ssize_t bytes_read = tw_conn_read(conn, buf, sizeof(buf) - 1);
        if (bytes_read <= 0) {
          keep_alive = false;
          break;
        }

        buf[bytes_read] = '\0';

        tw_request req;
        if (!tw_request_parse(buf, &req)) {
          tw_response res = {0};
          res.status_code = 400;

          const char *body = "Bad Request";
          res.body = body;
          res.body_len = strlen(body);

          tw_response_send(conn, &res);
          keep_alive = false;
          break;
        }

        tw_response res = {0};
        res.status_code = 200;

        if (req.keep_alive) {
          tw_response_set_header(&res, "Connection", "keep-alive");
        } else {
          tw_response_set_header(&res, "Connection", "close");
          keep_alive = false;
        }

        handler(conn, &req, &res);

        if (!keep_alive) {
          tw_conn_close(conn);
#ifdef _WIN32
          server->fds[i].fd = (SOCKET)-1;
#else
          server->fds[i].fd = -1;
#endif
          conn->fd = -1;
          break;
        }
      }

      server->fds[i].revents = 0;
    }

    int current = 1;
    for (int i = 1; i < server->nfds; i++) {
#ifdef _WIN32
      if (server->fds[i].fd != (SOCKET)-1) {
#else
      if (server->fds[i].fd != -1) {
#endif
        if (current != i) {
          server->fds[current] = server->fds[i];
          server->conns[current] = server->conns[i];
        }
        current++;
      } else {
        server->fds[i].events = 0;
        server->fds[i].revents = 0;
        server->conns[i].fd = -1;
      }
    }

    server->nfds = current;
  }

  return true;
}

TWDEF bool tw_server_stop(tw_server *server) {
  if (close(server->fd) < 0) {
    return false;
  }

#ifdef _WIN32
  WSACleanup();
#endif

  return true;
}

TWDEF bool tw__set_nonblocking(int fd) {
#ifdef _WIN32
  DWORD mode = 1;
  if (ioctlsocket(fd, FIONBIO, &mode) != 0) {
    tw_log(TW_ERROR, "ioctlsocket failed");
    return false;
  }
#else
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    tw_log(TW_ERROR, "fcntl(F_GETFL) failed");
    return false;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    tw_log(TW_ERROR, "fcntl(F_SETFL) failed");
    return false;
  }
#endif
  return true;
}

TWDEF ssize_t tw_conn_read(tw_conn *conn, char *buf, size_t len) {
  ssize_t bytes_read = recv(conn->fd, buf, len, 0);
  return bytes_read;
};

TWDEF ssize_t tw_conn_write(tw_conn *conn, const char *buf, size_t len) {
  ssize_t bytes_sent = send(conn->fd, buf, len, 0);
  return bytes_sent;
};

TWDEF void tw_conn_close(tw_conn *conn) { close(conn->fd); };

TWDEF bool tw_request_parse(const char *buf, tw_request *req) {
  const char *pos = buf;

  const char *method_end = strchr(buf, ' ');
  if (!method_end) {
    return false;
  }
  req->method_len = (size_t)(method_end - pos);
  if (req->method_len >= sizeof(req->method)) {
    return false;
  }
  memcpy(req->method, pos, req->method_len);
  req->method[req->method_len] = '\0';

  const char *path_start = method_end + 1;
  const char *path_end = strchr(path_start, ' ');
  if (!path_end) {
    return false;
  }
  req->path_len = (size_t)(path_end - path_start);
  if (req->path_len >= sizeof(req->path)) {
    return false;
  }
  memcpy(req->path, path_start, req->path_len);
  req->path[req->path_len] = '\0';

  const char *version_start = path_end + 1;
  const char *version_end = strstr(version_start, "\r\n");
  if (!version_end) {
    version_end = strchr(version_start, '\n');
    if (!version_end) {
      return false;
    }
  }
  req->version_len = (size_t)(version_end - version_start);
  if (req->version_len >= sizeof(req->version)) {
    return false;
  }
  memcpy(req->version, version_start, req->version_len);
  req->version[req->version_len] = '\0';

  pos = version_end + 2;

  req->header_count = 0;
  while (*pos && !(pos[0] == '\r' && pos[1] == '\n')) {
    if (req->header_count >= TW_MAX_HEADERS) return false;

    const char *colon = strchr(pos, ':');
    if (!colon) return false;

    size_t name_len = (size_t)(colon - pos);
    if (name_len >= TW_MAX_HEADER_NAME) return false;
    memcpy(req->headers[req->header_count].name, pos, name_len);
    req->headers[req->header_count].name[name_len] = '\0';

    const char *value_start = colon + 1;
    while (*value_start == ' ') value_start++;

    const char *line_end = strstr(value_start, "\r\n");
    if (!line_end) return false;

    size_t value_len = (size_t)(line_end - value_start);
    if (value_len >= TW_MAX_HEADER_VALUE) return false;
    memcpy(req->headers[req->header_count].value, value_start, value_len);
    req->headers[req->header_count].value[value_len] = '\0';

    req->header_count++;
    pos = line_end + 2;
  }

  req->keep_alive = false;

  const char *conn_hdr = tw_request_get_header(req, "Connection");
  if (conn_hdr) {
    if (strcasecmp(conn_hdr, "keep-alive") == 0) {
      req->keep_alive = true;
    }
  } else if (strcmp(req->version, "HTTP/1.1") == 0) {
    // HTTP/1.1 defaults to keep-alive
    req->keep_alive = true;
  }

  return true;
}

TWDEF const char *tw_request_get_header(tw_request *req, const char *name) {
  for (size_t i = 0; i < req->header_count; i++) {
    if (strcasecmp(req->headers[i].name, name) == 0) {
      return req->headers[i].value;
    }
  }
  return NULL;
}

TWDEF void tw_response_set_header(tw_response *res, const char *name,
                                  const char *value) {
  if (res->header_count >= TW_MAX_HEADERS) {
    tw_log(TW_WARNING, "Cannot set header, max headers reached.");
    return;
  }

  size_t name_len = strlen(name);
  if (name_len >= TW_MAX_HEADER_NAME) {
    tw_log(TW_WARNING, "Header name too long.");
    return;
  }

  size_t value_len = strlen(value);
  if (value_len >= TW_MAX_HEADER_VALUE) {
    tw_log(TW_WARNING, "Header value too long.");
    return;
  }

  for (size_t i = 0; i < res->header_count; i++) {
    if (strcasecmp(res->headers[i].name, name) == 0) {
      strcpy(res->headers[i].value, value);
      return;
    }
  }

  strcpy(res->headers[res->header_count].name, name);
  strcpy(res->headers[res->header_count].value, value);
  res->header_count++;
}

static const char *tw_get_status_message(int status_code) {
  switch (status_code) {
    case 100:
      return "Continue";
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 204:
      return "No Content";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 500:
      return "Internal Server Error";
    case 502:
      return "Bad Gateway";
    case 503:
      return "Service Unavailable";
    default:
      return "Unknown Status";
  }
}

TWDEF bool tw_response_send(tw_conn *conn, tw_response *res) {
  char header_buf[TW_MAX_HEADER_VALUE * 2];
  int offset = 0;

  const char *status_message = tw_get_status_message(res->status_code);
  offset += snprintf(header_buf + offset, sizeof(header_buf) - offset,
                     "HTTP/1.1 %d %s\r\n", res->status_code, status_message);

  offset += snprintf(header_buf + offset, sizeof(header_buf) - offset,
                     "Content-Length: %zu\r\n", res->body_len);

  for (size_t i = 0; i < res->header_count; i++) {
    offset +=
        snprintf(header_buf + offset, sizeof(header_buf) - offset, "%s: %s\r\n",
                 res->headers[i].name, res->headers[i].value);
  }

  offset += snprintf(header_buf + offset, sizeof(header_buf) - offset, "\r\n");

  if (tw_conn_write(conn, header_buf, offset) < 0) {
    tw_log(TW_ERROR, "Failed to send response headers");
    return false;
  }

  if (res->body && res->body_len > 0) {
    if (tw_conn_write(conn, res->body, res->body_len) < 0) {
      tw_log(TW_ERROR, "Failed to send response body");
      return false;
    }
  }

  return true;
}

#endif  // THINWIRE_IMPL