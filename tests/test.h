/* ltxpng: minimal test harness (C11) */

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Test result counters; extern so the macros share state across TUs. */
extern int test_passed;
extern int test_failed;
extern int test_assert_failures;

#define TEST(name)                                 \
  static void test_##name(int *pass, int *fail);    \
  __attribute__((constructor))                      \
  static void reg_##name(void) {                    \
    test_register(#name, test_##name);              \
  }                                                 \
  static void test_##name(int *pass, int *fail)

#define ASSERT_TRUE(cond) do {                                  \
  if (!(cond)) {                                                \
    fprintf(stderr, "  FAIL at %s:%d: ASSERT_TRUE(%s)\n",       \
            __FILE__, __LINE__, #cond);                         \
    (*fail)++;                                                  \
    return;                                                     \
  }                                                             \
} while (0)

#define ASSERT_EQ(a, b) do {                                    \
  if ((a) != (b)) {                                             \
    fprintf(stderr, "  FAIL at %s:%d: ASSERT_EQ(%s, %s)\n",     \
            __FILE__, __LINE__, #a, #b);                        \
    fprintf(stderr, "    got %lld, expected %lld\n",            \
            (long long)(a), (long long)(b));                    \
    (*fail)++;                                                  \
    return;                                                     \
  }                                                             \
} while (0)

#define ASSERT_STREQ(a, b) do {                                 \
  const char *_a = (a);                                         \
  const char *_b = (b);                                         \
  if (!_a || !_b || strcmp(_a, _b) != 0) {                      \
    fprintf(stderr, "  FAIL at %s:%d: ASSERT_STREQ(%s, %s)\n",  \
            __FILE__, __LINE__, #a, #b);                        \
    fprintf(stderr, "    got \"%s\", expected \"%s\"\n",        \
            _a ? _a : "(null)", _b ? _b : "(null)");            \
    (*fail)++;                                                  \
    return;                                                     \
  }                                                             \
} while (0)

/* Test runner internals */
typedef void (*test_fn)(int *, int *);

typedef struct test_entry {
  const char *name;
  test_fn fn;
  struct test_entry *next;
} test_entry;

void test_register(const char *name, test_fn fn);
int test_run_all(void);

#endif /* TEST_H */
