<h1 align="center">libthinwire</h1>

<div align="center">
  <a href="https://github.com/metebykl/libthinwire/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-MIT-blue" alt="MIT" title="MIT License" />
  </a>
</div>

<br>

libthinwire is a lightweight, header-only C library for building HTTP servers.

## Example

```c
#define THINWIRE_IMPL
#include "thinwire.h"

#define PORT 8080

void handle_request(tw_conn *conn, tw_request *req, tw_response *res) {
  const char *message = "Hello World!";
  tw_response_set_status(res, 200);
  tw_response_set_header(res, "Content-Type", "text/plain");
  tw_response_set_body(res, message, strlen(message));

  tw_response_send(conn, res);
}

int main() {
  tw_server server;
  if (!tw_server_init(&server, PORT)) {
    exit(EXIT_FAILURE);
  };

  tw_log(TW_INFO, "Server listening on port %d", PORT);
  tw_server_run(&server, handle_request);

  tw_server_stop(&server);
  return 0;
}
```

## License

libthinwire is licensed under the MIT License, see [LICENSE](LICENSE) for details.
