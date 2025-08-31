#include <stdio.h>

#define THINWIRE_IMPL
#include "../thinwire.h"

int main() {
  if (tw_server_start() < 0) {
    fprintf(stderr, "error: failed to initialize server");
    return 1;
  };

  return 0;
}