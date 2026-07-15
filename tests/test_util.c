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

/* --- Edge cases --- */

TEST(sb_append_empty_string_is_noop) {
  sb b;
  sb_init(&b);
  sb_append(&b, "hello");
  sb_append(&b, "");
  sb_append(&b, "");
  ASSERT_EQ(b.len, 5);
  ASSERT_STREQ(b.buf, "hello");
  sb_free(&b);
}

TEST(sb_append_single_char_repeated) {
  sb b;
  sb_init(&b);
  for (int i = 0; i < 200; i++)
    sb_append(&b, "x");
  ASSERT_EQ(b.len, 200);
  ASSERT_EQ(b.buf[0], 'x');
  ASSERT_EQ(b.buf[199], 'x');
  ASSERT_EQ(b.buf[200], '\0');
  sb_free(&b);
}

TEST(sb_appendf_long_format_forces_growth) {
  sb b;
  sb_init(&b);
  char big[300];
  memset(big, 'Z', 299);
  big[299] = '\0';
  sb_appendf(&b, "prefix:%s:suffix", big);
  ASSERT_TRUE(b.len > 300);
  ASSERT_STRCONTAINS(b.buf, "prefix:");
  ASSERT_STRCONTAINS(b.buf, ":suffix");
  sb_free(&b);
}

TEST(sb_appendf_empty_format) {
  sb b;
  sb_init(&b);
  sb_appendf(&b, "%s", "");
  ASSERT_EQ(b.len, 0);
  ASSERT_STREQ(b.buf, "");
  sb_free(&b);
}

TEST(sb_free_then_reinit) {
  sb b;
  sb_init(&b);
  sb_append(&b, "first");
  sb_free(&b);
  ASSERT_NULL(b.buf);
  sb_init(&b);
  sb_append(&b, "second");
  ASSERT_STREQ(b.buf, "second");
  sb_free(&b);
}

TEST(sb_detach_then_reinit) {
  sb b;
  sb_init(&b);
  sb_append(&b, "data");
  char *s = sb_detach(&b);
  ASSERT_STREQ(s, "data");
  sb_init(&b);
  sb_append(&b, "new");
  ASSERT_STREQ(b.buf, "new");
  free(s);
  sb_free(&b);
}

TEST(read_all_empty_file) {
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);
  size_t len;
  char *content = read_all(f, &len);
  ASSERT_EQ(len, 0);
  ASSERT_STREQ(content, "");
  free(content);
  fclose(f);
}

TEST(read_all_exactly_one_chunk_boundary) {
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);
  char big[4096];
  memset(big, 'C', 4096);
  fwrite(big, 1, 4096, f);
  rewind(f);
  size_t len;
  char *content = read_all(f, &len);
  ASSERT_EQ(len, 4096);
  ASSERT_EQ(content[0], 'C');
  ASSERT_EQ(content[4095], 'C');
  ASSERT_EQ(content[4096], '\0');
  free(content);
  fclose(f);
}

TEST(read_all_with_null_len_pointer) {
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);
  fprintf(f, "test");
  rewind(f);
  char *content = read_all(f, NULL);
  ASSERT_STREQ(content, "test");
  free(content);
  fclose(f);
}

TEST(chomp_single_char_string) {
  char s1[] = "a";
  chomp(s1);
  ASSERT_STREQ(s1, "a");

  char s2[] = " ";
  chomp(s2);
  ASSERT_STREQ(s2, "");
}

TEST(chomp_only_spaces) {
  char s[] = "   ";
  chomp(s);
  ASSERT_STREQ(s, "");
}

TEST(chomp_null_returns_null) {
  char *r = chomp(NULL);
  ASSERT_NULL(r);
}

TEST(chomp_mixed_whitespace_at_end) {
  char s[] = "data\r\n\t ";
  chomp(s);
  ASSERT_STREQ(s, "data");
}

TEST(sb_append_newlines_and_special_chars) {
  sb b;
  sb_init(&b);
  sb_append(&b, "line1\nline2\ttab\\backslash\0hidden");
  ASSERT_STREQ(b.buf, "line1\nline2\ttab\\backslash");
  sb_free(&b);
}

TEST(read_all_binary_content_with_internal_nuls) {
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);
  char data[] = {0x89, 0x50, 0x4E, 0x47, 0x00, 0x41, 0x42};
  fwrite(data, 1, 7, f);
  rewind(f);
  size_t len;
  char *content = read_all(f, &len);
  ASSERT_EQ(len, 7);
  ASSERT_EQ((unsigned char)content[0], 0x89);
  ASSERT_EQ((unsigned char)content[4], 0x00);
  ASSERT_EQ((unsigned char)content[5], 0x41);
  free(content);
  fclose(f);
}

int main(void) {
  return test_run_all();
}
