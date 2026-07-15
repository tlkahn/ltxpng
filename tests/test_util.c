/* ltxpng: unit tests for util module */

#include "test.h"
#include "util.h"
#include <string.h>

TEST(sb_init_creates_empty_buffer) {
  sb b;
  sb_init(&b);
  ASSERT_TRUE(b.buf != NULL);
  ASSERT_EQ(b.len, 0);
  ASSERT_EQ(b.buf[0], '\0');
  ASSERT_TRUE(b.cap >= 1);
  sb_free(&b);
}

TEST(sb_append_adds_content) {
  sb b;
  sb_init(&b);
  sb_append(&b, "hello");
  ASSERT_EQ(b.len, 5);
  ASSERT_STREQ(b.buf, "hello");
  sb_append(&b, " world");
  ASSERT_EQ(b.len, 11);
  ASSERT_STREQ(b.buf, "hello world");
  sb_free(&b);
}

TEST(sb_append_grows_past_initial_capacity) {
  sb b;
  sb_init(&b);
  /* Default capacity is 64, append more than that */
  char big[100];
  memset(big, 'A', 99);
  big[99] = '\0';
  sb_append(&b, big);
  ASSERT_EQ(b.len, 99);
  ASSERT_TRUE(b.cap > 64);
  /* Append again to verify growth still works */
  sb_append(&b, "BBB");
  ASSERT_EQ(b.len, 102);
  ASSERT_STREQ(b.buf + 99, "BBB");
  sb_free(&b);
}

TEST(sb_appendf_formats_text) {
  sb b;
  sb_init(&b);
  sb_appendf(&b, "%d + %d = %d", 2, 3, 5);
  ASSERT_STREQ(b.buf, "2 + 3 = 5");
  sb_appendf(&b, " and %s", "done");
  ASSERT_STREQ(b.buf, "2 + 3 = 5 and done");
  sb_free(&b);
}

TEST(sb_detach_returns_and_clears) {
  sb b;
  sb_init(&b);
  sb_append(&b, "detach me");
  char *s = sb_detach(&b);
  ASSERT_STREQ(s, "detach me");
  ASSERT_TRUE(b.buf == NULL);
  ASSERT_EQ(b.len, 0);
  ASSERT_EQ(b.cap, 0);
  free(s);
}

TEST(read_all_returns_full_content) {
  /* Write test data to a temp file and read it back */
  FILE *f = fopen("/tmp/ltxpng_test_readall.txt", "w+");
  ASSERT_TRUE(f != NULL);
  fprintf(f, "line1\nline2\nline3\n");
  rewind(f);
  size_t len;
  char *content = read_all(f, &len);
  ASSERT_STREQ(content, "line1\nline2\nline3\n");
  ASSERT_EQ(len, 18);
  free(content);
  fclose(f);
  remove("/tmp/ltxpng_test_readall.txt");
}

TEST(read_all_large_content) {
  /* Content larger than the internal 4096-byte buffer */
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);
  char big[5000];
  memset(big, 'B', 4999);
  big[4999] = '\0';
  fprintf(f, "%s", big);
  rewind(f);
  size_t len;
  char *content = read_all(f, &len);
  ASSERT_EQ(len, 4999);
  ASSERT_EQ(content[0], 'B');
  ASSERT_EQ(content[4998], 'B');
  ASSERT_EQ(content[4999], '\0');
  free(content);
  fclose(f);
}

TEST(chomp_strips_trailing_whitespace) {
  char s1[] = "hello  \t\n";
  chomp(s1);
  ASSERT_STREQ(s1, "hello");

  char s2[] = "no trail";
  chomp(s2);
  ASSERT_STREQ(s2, "no trail");

  char s3[] = "\n\n";
  chomp(s3);
  ASSERT_STREQ(s3, "");
}

int main(void) {
  return test_run_all();
}
