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
} tw_server_t;

typedef struct {
  int fd;
  struct sockaddr_in addr;
} tw_conn_t;

TWDEF bool tw_server_start(tw_server_t *server);
TWDEF bool tw_server_stop(tw_server_t *server);
TWDEF bool tw_server_accept(tw_server_t *server, tw_conn_t *conn);

TWDEF ssize_t tw_conn_read(tw_conn_t *conn, char *buf, size_t len);
TWDEF ssize_t tw_conn_write(tw_conn_t *conn, const char *buf, size_t len);
TWDEF void tw_conn_close(tw_conn_t *conn);

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

TWDEF bool tw_server_start(tw_server_t *server) {
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

TWDEF bool tw_server_stop(tw_server_t *server) {
  if (close(server->fd) < 0) {
    return false;
  }

#ifdef _WIN32
  WSACleanup();
#endif

  return true;
}

TWDEF bool tw_server_accept(tw_server_t *server, tw_conn_t *conn) {
  socklen_t addr_len = sizeof(conn->addr);

  if ((conn->fd =
           accept(server->fd, (struct sockaddr *)&conn->addr, &addr_len)) < 0) {
    tw_log(TW_ERROR, "Accept failed");
    return false;
  }

  return true;
}

TWDEF ssize_t tw_conn_read(tw_conn_t *conn, char *buf, size_t len) {
  ssize_t bytes_read = recv(conn->fd, buf, len, 0);
  return bytes_read;
};

TWDEF ssize_t tw_conn_write(tw_conn_t *conn, const char *buf, size_t len) {
  ssize_t bytes_sent = send(conn->fd, buf, len, 0);
  return bytes_sent;
};

TWDEF void tw_conn_close(tw_conn_t *conn) { close(conn->fd); };

#endif  // THINWIRE_IMPL