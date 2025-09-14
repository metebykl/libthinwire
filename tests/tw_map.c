#include <assert.h>

#define THINWIRE_IMPL
#include "../thinwire.h"

int main(void) {
  tw_map map;
  assert(tw_map_init(&map));

  /* tests for tw_map_set & tw_map_get */
  assert(tw_map_set(&map, "foo", "bar"));
  assert(!strcmp(tw_map_get(&map, "foo"), "bar"));
  assert(tw_map_get(&map, "baz") == NULL);

  /* tests for tw_map_remove */
  assert(tw_map_remove(&map, "foo"));
  assert(!tw_map_remove(&map, "baz"));

  /* tests for tw_map_empty */
  assert(tw_map_set(&map, "hello", "world"));
  assert(tw_map_empty(&map));
  assert(tw_map_get(&map, "hello") == NULL);

  return 0;
}