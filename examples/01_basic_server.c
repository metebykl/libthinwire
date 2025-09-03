#include <stdio.h>

#define THINWIRE_IMPL
#include "../thinwire.h"

int main() {
  tw_server server;
  if (!tw_server_start(&server)) {
    exit(EXIT_FAILURE);
  };

  tw_log(TW_INFO, "Server listening on port 8080");

  while (1) {
    tw_conn conn;
    if (!tw_server_accept(&server, &conn)) {
      continue;
    }

    tw_log(TW_INFO, "Client connected");

    char buf[4096];
    ssize_t bytes_read = tw_conn_read(&conn, buf, sizeof(buf) - 1);
    if (bytes_read <= 0) {
      tw_log(TW_WARNING, "Failed to read buffer");
      tw_conn_close(&conn);
      continue;
    }

    buf[bytes_read] = '\0';

    tw_request req;
    if (!tw_request_parse(buf, &req)) {
      tw_log(TW_WARNING, "Failed to parse request");
      tw_conn_close(&conn);
      continue;
    }

    tw_log(TW_INFO, "Method: %s, Path: %s", req.method, req.path);
    const char *host = tw_request_get_header(&req, "Host");
    if (host) tw_log(TW_INFO, "Host: %s", host);

    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!";

    tw_conn_write(&conn, response, strlen(response));
    tw_conn_close(&conn);
  }

  tw_server_stop(&server);
  return 0;
}