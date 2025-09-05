#define THINWIRE_IMPL
#include "../thinwire.h"

void handle_request(tw_conn *conn, tw_request *req, tw_response *res) {
  tw_log(TW_INFO, "Method: %s, Path: %s", req->method, req->path);

  const char *host = tw_request_get_header(req, "Host");
  if (host) tw_log(TW_INFO, "Host: %s", host);

  if (strcmp(req->path, "/") != 0) {
    res->status_code = 404;
    tw_response_send(conn, res);
    return;
  }

  res->status_code = 200;
  res->body = "Hello World!";
  res->body_len = strlen(res->body);
  tw_response_set_header(res, "Content-Type", "text/plain");

  tw_response_send(conn, res);
}

int main() {
  tw_server server;
  if (!tw_server_start(&server)) {
    exit(EXIT_FAILURE);
  };

  tw_log(TW_INFO, "Server listening on port 8080");
  tw_server_run(&server, handle_request);

  tw_server_stop(&server);
  return 0;
}