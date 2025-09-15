#include "test.h"

int __tests_run = 0;
int __tests_failed = 0;

void test_run(const char *name, int (*fn)(void)) {
  printf("[ RUN ] %s\n", name);
  __tests_run++;
  int r = fn();
  if (r) {
    __tests_failed++;
    printf("[ FAIL ] %s\n", name);
  } else {
    printf("[ PASS ] %s\n", name);
  }
}

int test_summary(void) {
  if (__tests_failed == 0) {
    printf("\nAll %d tests passed.\n", __tests_run);
    return 0;
  } else {
    printf("\n%d/%d tests failed.\n", __tests_failed, __tests_run);
    return 1;
  }
}