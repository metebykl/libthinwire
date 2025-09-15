#ifndef TEST_H
#define TEST_H

#include <stdio.h>

extern int __tests_run;
extern int __tests_failed;

#define TEST_BEGIN() int _test_failed = 0
#define TEST_END() return _test_failed

#define ASSERT(expr)                                                   \
  do {                                                                 \
    if (!(expr)) {                                                     \
      fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
      _test_failed = 1;                                                \
    }                                                                  \
  } while (0)

void test_run(const char *name, int (*fn)(void));
#define RUN_TEST(fn) test_run(#fn, fn)

int test_summary(void);

#endif