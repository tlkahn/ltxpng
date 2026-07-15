/* ltxpng: unit tests for run module */

#include "test.h"
#include "run.h"
#include "util.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

TEST(run_cmd_true_returns_zero) {
  const char *argv[] = {"true", NULL};
  int rc = run_cmd(argv, NULL);
  ASSERT_EQ(rc, 0);
}

TEST(run_cmd_false_returns_nonzero) {
  const char *argv[] = {"false", NULL};
  int rc = run_cmd(argv, NULL);
  ASSERT_TRUE(rc != 0);
}

TEST(run_cmd_nonexistent_returns_127) {
  const char *argv[] = {"nonexistent_cmd_xyzzy", NULL};
  int rc = run_cmd(argv, NULL);
  ASSERT_EQ(rc, 127);
}

TEST(run_cmd_captures_output) {
  const char *capfile = "/tmp/ltxpng_test_capture.txt";
  const char *argv[] = {"sh", "-c", "echo out; echo err >&2", NULL};
  int rc = run_cmd(argv, capfile);
  ASSERT_EQ(rc, 0);

  /* Read and check capture file */
  FILE *f = fopen(capfile, "r");
  ASSERT_TRUE(f != NULL);
  size_t len;
  char *content = read_all(f, &len);
  fclose(f);
  ASSERT_TRUE(strstr(content, "out") != NULL);
  ASSERT_TRUE(strstr(content, "err") != NULL);
  free(content);
  remove(capfile);
}

TEST(tmpdir_create_makes_writable_directory) {
  char *path = tmpdir_create();
  ASSERT_TRUE(path != NULL);

  /* Check it's a directory and writable */
  struct stat st;
  ASSERT_EQ(stat(path, &st), 0);
  ASSERT_TRUE(S_ISDIR(st.st_mode));
  ASSERT_EQ(access(path, W_OK), 0);

  /* Clean up */
  tmpdir_remove(path);
  ASSERT_EQ(stat(path, &st), -1); /* should not exist */
  free(path);
}

TEST(tmpdir_remove_removes_contents) {
  char *path = tmpdir_create();
  ASSERT_TRUE(path != NULL);

  /* Create a file and a subdirectory inside */
  char fpath[1024];
  snprintf(fpath, sizeof(fpath), "%s/test.txt", path);
  FILE *f = fopen(fpath, "w");
  ASSERT_TRUE(f != NULL);
  fprintf(f, "hello");
  fclose(f);

  char dpath[1024];
  snprintf(dpath, sizeof(dpath), "%s/subdir", path);
  ASSERT_EQ(mkdir(dpath, 0755), 0);

  char dfpath[1024];
  snprintf(dfpath, sizeof(dfpath), "%s/nested.txt", dpath);
  f = fopen(dfpath, "w");
  ASSERT_TRUE(f != NULL);
  fclose(f);

  /* Remove */
  tmpdir_remove(path);
  struct stat st;
  ASSERT_EQ(stat(path, &st), -1); /* all gone */
  free(path);
}

TEST(find_on_path_finds_sh) {
  char *path = find_on_path("sh");
  ASSERT_TRUE(path != NULL);
  ASSERT_TRUE(strstr(path, "/sh") != NULL || strstr(path, "/sh") != NULL);
  /* Should be an executable file */
  struct stat st;
  ASSERT_EQ(stat(path, &st), 0);
  ASSERT_TRUE(st.st_mode & S_IXUSR);
  free(path);
}

TEST(find_on_path_rejects_nonsense) {
  char *path = find_on_path("nonexistent_cmd_xyzzy_123");
  ASSERT_TRUE(path == NULL);
}

TEST(extract_log_errors_finds_exclamation_lines) {
  /* Write a fixture .log with ! error lines */
  const char *logpath = "/tmp/ltxpng_test.log";
  FILE *f = fopen(logpath, "w");
  ASSERT_TRUE(f != NULL);
  fprintf(f, "This is a log file\n");
  fprintf(f, "! Undefined control sequence.\n");
  fprintf(f, "l.5 \\undefined\n");
  fprintf(f, "Some more context\n");
  fprintf(f, "! Another error.\n");
  fclose(f);

  sb b;
  sb_init(&b);
  extract_log_errors(logpath, &b);

  ASSERT_TRUE(strstr(b.buf, "! Undefined control sequence.") != NULL);
  ASSERT_TRUE(strstr(b.buf, "! Another error.") != NULL);
  sb_free(&b);
  remove(logpath);
}

TEST(extract_log_errors_tail_without_exclamations) {
  /* Write a .log without ! lines */
  const char *logpath = "/tmp/ltxpng_test2.log";
  FILE *f = fopen(logpath, "w");
  ASSERT_TRUE(f != NULL);
  fprintf(f, "line1\nline2\nline3\n");
  fclose(f);

  sb b;
  sb_init(&b);
  extract_log_errors(logpath, &b);

  /* Should contain the log content */
  ASSERT_TRUE(strstr(b.buf, "line1") != NULL);
  ASSERT_TRUE(strstr(b.buf, "line3") != NULL);
  sb_free(&b);
  remove(logpath);
}

int main(void) {
  return test_run_all();
}
