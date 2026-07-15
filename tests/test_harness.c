/* ltxpng: test runner implementation */

#include "test.h"

int test_passed = 0;
int test_failed = 0;
int test_assert_failures = 0;

static test_entry *test_list = NULL;

void test_register(const char *name, test_fn fn) {
  test_entry *e = (test_entry *)malloc(sizeof(test_entry));
  if (!e) {
    fprintf(stderr, "test_register: OOM\n");
    exit(1);
  }
  e->name = name;
  e->fn = fn;
  e->next = test_list;
  test_list = e;
}

int test_run_all(void) {
  int total = 0;
  int passed = 0;
  int failed = 0;

  for (test_entry *e = test_list; e; e = e->next) {
    total++;
    int p = 0, f = 0;
    fprintf(stdout, "TEST %s ... ", e->name);
    fflush(stdout);
    e->fn(&p, &f);
    if (f > 0) {
      fprintf(stdout, "FAIL\n");
      failed++;
    } else {
      fprintf(stdout, "ok\n");
      passed++;
    }
  }

  fprintf(stdout, "\n%d test(s): %d passed, %d failed\n", total, passed, failed);
  return failed == 0 ? 0 : 1;
}
