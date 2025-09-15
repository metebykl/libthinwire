#include <assert.h>

#include "test.h"

#define THINWIRE_IMPL
#include "../thinwire.h"

static int test_map_get_and_set(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  ASSERT(tw_map_set(&map, "foo", "bar"));
  ASSERT(!strcmp(tw_map_get(&map, "foo"), "bar"));
  ASSERT(tw_map_get(&map, "baz") == NULL);

  TEST_END();
}

static int test_map_remove(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  ASSERT(tw_map_set(&map, "foo", "bar"));
  ASSERT(tw_map_remove(&map, "foo"));
  ASSERT(!tw_map_remove(&map, "baz"));

  TEST_END();
}

static int test_map_empty(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  ASSERT(tw_map_set(&map, "hello", "world"));
  ASSERT(tw_map_empty(&map));
  ASSERT(tw_map_get(&map, "hello") == NULL);

  TEST_END();
}

int main(void) {
  RUN_TEST(test_map_get_and_set);
  RUN_TEST(test_map_remove);
  RUN_TEST(test_map_empty);

  return test_summary();
}