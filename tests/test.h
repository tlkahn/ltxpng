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

#define ASSERT_STRCONTAINS(haystack, needle) do {               \
  const char *_h = (haystack);                                  \
  const char *_n = (needle);                                    \
  if (!_h || !_n || strstr(_h, _n) == NULL) {                   \
    fprintf(stderr,                                             \
      "  FAIL at %s:%d: ASSERT_STRCONTAINS(%s, %s)\n",          \
            __FILE__, __LINE__, #haystack, #needle);            \
    fprintf(stderr, "    \"%s\" not found in \"%s\"\n",         \
            _n ? _n : "(null)",                                 \
            _h ? (strlen(_h) > 200 ? "(long string)" : _h)     \
               : "(null)");                                     \
    (*fail)++;                                                  \
    return;                                                     \
  }                                                             \
} while (0)

#define ASSERT_NULL(ptr) do {                                   \
  if ((ptr) != NULL) {                                          \
    fprintf(stderr, "  FAIL at %s:%d: ASSERT_NULL(%s)\n",        \
            __FILE__, __LINE__, #ptr);                          \
    (*fail)++;                                                  \
    return;                                                     \
  }                                                             \
} while (0)

#define ASSERT_NEQ(a, b) do {                                   \
  if ((a) == (b)) {                                             \
    fprintf(stderr, "  FAIL at %s:%d: ASSERT_NEQ(%s, %s)\n",    \
            __FILE__, __LINE__, #a, #b);                        \
    fprintf(stderr, "    both are %lld\n", (long long)(a));     \
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
