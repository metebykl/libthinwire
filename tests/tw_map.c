#include <assert.h>

#include "test.h"

#define THINWIRE_IMPL
#include "../thinwire.h"

static int test_tw_map_get_and_set(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  ASSERT(tw_map_set(&map, "foo", "bar"));
  ASSERT(!strcmp(tw_map_get(&map, "foo"), "bar"));
  ASSERT(tw_map_get(&map, "baz") == NULL);

  tw_map_free(&map);
  TEST_END();
}

static int test_tw_map_remove(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  ASSERT(tw_map_set(&map, "foo", "bar"));
  ASSERT(tw_map_remove(&map, "foo"));
  ASSERT(!tw_map_remove(&map, "baz"));

  tw_map_free(&map);
  TEST_END();
}

static int test_tw_map_remove_at(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  /* add multiple entries */
  ASSERT(tw_map_set(&map, "first", "1"));
  ASSERT(tw_map_set(&map, "middle", "2"));
  ASSERT(tw_map_set(&map, "last", "3"));
  ASSERT(map.size == 3);

  /* remove from middle */
  ASSERT(tw_map_remove_at(&map, 1));
  ASSERT(map.size == 2);
  ASSERT(tw_map_get(&map, "middle") == NULL);
  ASSERT(!strcmp(tw_map_get(&map, "first"), "1"));
  ASSERT(!strcmp(tw_map_get(&map, "last"), "3"));

  /* remove from beginning */
  ASSERT(tw_map_remove_at(&map, 0));
  ASSERT(map.size == 1);
  ASSERT(tw_map_get(&map, "first") == NULL);
  ASSERT(!strcmp(tw_map_get(&map, "last"), "3"));

  /* remove last item */
  ASSERT(tw_map_remove_at(&map, 0));
  ASSERT(map.size == 0);
  ASSERT(tw_map_get(&map, "last") == NULL);

  tw_map_free(&map);
  TEST_END();
}

static int test_tw_map_empty(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  ASSERT(tw_map_set(&map, "hello", "world"));
  ASSERT(tw_map_empty(&map));
  ASSERT(tw_map_get(&map, "hello") == NULL);

  tw_map_free(&map);
  TEST_END();
}

static int test_tw_map_update_existing(void) {
  TEST_BEGIN();

  tw_map map;
  ASSERT(tw_map_init(&map));

  /* set entry */
  ASSERT(tw_map_set(&map, "key", "value1"));
  ASSERT(!strcmp(tw_map_get(&map, "key"), "value1"));

  /* update existing entry value */
  ASSERT(tw_map_set(&map, "key", "value2"));
  ASSERT(!strcmp(tw_map_get(&map, "key"), "value2"));

  tw_map_free(&map);
  TEST_END();
}

int main(void) {
  RUN_TEST(test_tw_map_get_and_set);
  RUN_TEST(test_tw_map_remove);
  RUN_TEST(test_tw_map_remove_at);
  RUN_TEST(test_tw_map_empty);
  RUN_TEST(test_tw_map_update_existing);

  return test_summary();
}