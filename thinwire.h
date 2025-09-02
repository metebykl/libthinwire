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
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef _WIN32
typedef int socklen_t;
#define close(fd) closesocket(fd)
#endif

typedef enum { TW_INFO, TW_WARNING, TW_ERROR } tw_log_level;

TWDEF void tw_log(tw_log_level level, const char *fmt, ...);

#define TW_DEFAULT_PORT 8080

typedef struct {
  int fd;
  struct sockaddr_in addr;
} tw_server;

typedef struct {
  int fd;
  struct sockaddr_in addr;
} tw_conn;

TWDEF bool tw_server_start(tw_server *server);
TWDEF bool tw_server_stop(tw_server *server);
TWDEF bool tw_server_accept(tw_server *server, tw_conn *conn);

TWDEF ssize_t tw_conn_read(tw_conn *conn, char *buf, size_t len);
TWDEF ssize_t tw_conn_write(tw_conn *conn, const char *buf, size_t len);
TWDEF void tw_conn_close(tw_conn *conn);

typedef struct {
  char method[64];
  size_t method_len;

  char version[16];
  size_t version_len;

  char path[1024];
  size_t path_len;
} tw_request;

TWDEF bool tw_request_parse(const char *buf, tw_request *req);

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

TWDEF bool tw_server_start(tw_server *server) {
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

  server->addr.sin_family = AF_INET;
  server->addr.sin_addr.s_addr = INADDR_ANY;
  server->addr.sin_port = htons(TW_DEFAULT_PORT);

  if (bind(server->fd, (struct sockaddr *)&server->addr, sizeof(server->addr)) <
      0) {
    tw_log(TW_ERROR, "Socket bind failed");
    return false;
  }

  if (listen(server->fd, 10) < 0) {
    tw_log(TW_ERROR, "Socket listen failed");
    return false;
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

TWDEF bool tw_server_accept(tw_server *server, tw_conn *conn) {
  socklen_t addr_len = sizeof(conn->addr);

  if ((conn->fd =
           accept(server->fd, (struct sockaddr *)&conn->addr, &addr_len)) < 0) {
    tw_log(TW_ERROR, "Accept failed");
    return false;
  }

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
  const char *method_end = strchr(buf, ' ');
  if (!method_end) {
    return false;
  }
  req->method_len = (size_t)(method_end - buf);
  if (req->method_len >= sizeof(req->method)) {
    return false;
  }
  memcpy(req->method, buf, req->method_len);
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

  return true;
}

#endif  // THINWIRE_IMPL