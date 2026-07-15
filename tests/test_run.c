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

/* --- Edge cases --- */

TEST(run_cmd_with_exit_code_42) {
  const char *argv[] = {"sh", "-c", "exit 42", NULL};
  int rc = run_cmd(argv, NULL);
  ASSERT_EQ(rc, 42);
}

TEST(run_cmd_with_large_output) {
  const char *capfile = "/tmp/ltxpng_test_bigout.txt";
  const char *argv[] = {"sh", "-c", "seq 1 10000", NULL};
  int rc = run_cmd(argv, capfile);
  ASSERT_EQ(rc, 0);

  FILE *f = fopen(capfile, "r");
  ASSERT_TRUE(f != NULL);
  size_t len;
  char *content = read_all(f, &len);
  fclose(f);
  ASSERT_TRUE(len > 1000);
  ASSERT_STRCONTAINS(content, "10000");
  free(content);
  remove(capfile);
}

TEST(run_cmd_with_args_containing_spaces) {
  const char *capfile = "/tmp/ltxpng_test_spaces.txt";
  const char *argv[] = {"sh", "-c", "echo 'hello world'", NULL};
  int rc = run_cmd(argv, capfile);
  ASSERT_EQ(rc, 0);

  FILE *f = fopen(capfile, "r");
  ASSERT_TRUE(f != NULL);
  size_t len;
  char *content = read_all(f, &len);
  fclose(f);
  ASSERT_STRCONTAINS(content, "hello world");
  free(content);
  remove(capfile);
}

TEST(run_cmd_without_capture_file) {
  const char *argv[] = {"true", NULL};
  int rc = run_cmd(argv, NULL);
  ASSERT_EQ(rc, 0);
}

TEST(tmpdir_create_unique_paths) {
  char *p1 = tmpdir_create();
  char *p2 = tmpdir_create();
  ASSERT_TRUE(p1 != NULL);
  ASSERT_TRUE(p2 != NULL);
  ASSERT_TRUE(strcmp(p1, p2) != 0);
  tmpdir_remove(p1);
  tmpdir_remove(p2);
  free(p1);
  free(p2);
}

TEST(tmpdir_remove_empty_directory) {
  char *path = tmpdir_create();
  ASSERT_TRUE(path != NULL);
  tmpdir_remove(path);
  struct stat st;
  ASSERT_EQ(stat(path, &st), -1);
  free(path);
}

TEST(tmpdir_remove_null_does_not_crash) {
  tmpdir_remove(NULL);
  ASSERT_TRUE(1);
}

TEST(find_on_path_finds_ls) {
  char *path = find_on_path("ls");
  ASSERT_TRUE(path != NULL);
  ASSERT_STRCONTAINS(path, "/ls");
  free(path);
}

TEST(find_on_path_empty_name) {
  char *path = find_on_path("");
  /* Empty name shouldn't find anything meaningful */
  /* (behavior is implementation-dependent but shouldn't crash) */
  if (path) free(path);
  ASSERT_TRUE(1);
}

TEST(extract_log_errors_nonexistent_file) {
  sb b;
  sb_init(&b);
  extract_log_errors("/tmp/ltxpng_nonexistent_xyz.log", &b);
  ASSERT_STRCONTAINS(b.buf, "could not open log");
  sb_free(&b);
}

TEST(extract_log_errors_empty_file) {
  const char *logpath = "/tmp/ltxpng_test_empty.log";
  FILE *f = fopen(logpath, "w");
  ASSERT_TRUE(f != NULL);
  fclose(f);

  sb b;
  sb_init(&b);
  extract_log_errors(logpath, &b);
  /* Empty file, no ! lines, tail of 0 lines = empty output */
  ASSERT_EQ(b.len, 0);
  sb_free(&b);
  remove(logpath);
}

TEST(extract_log_errors_many_exclamation_lines) {
  const char *logpath = "/tmp/ltxpng_test_many_err.log";
  FILE *f = fopen(logpath, "w");
  ASSERT_TRUE(f != NULL);
  for (int i = 0; i < 50; i++)
    fprintf(f, "! Error %d\n", i);
  fclose(f);

  sb b;
  sb_init(&b);
  extract_log_errors(logpath, &b);
  ASSERT_STRCONTAINS(b.buf, "! Error 0");
  ASSERT_STRCONTAINS(b.buf, "! Error 49");
  sb_free(&b);
  remove(logpath);
}

TEST(extract_log_errors_tail_boundary_exactly_30) {
  const char *logpath = "/tmp/ltxpng_test_30.log";
  FILE *f = fopen(logpath, "w");
  ASSERT_TRUE(f != NULL);
  for (int i = 0; i < 30; i++)
    fprintf(f, "line %d\n", i);
  fclose(f);

  sb b;
  sb_init(&b);
  extract_log_errors(logpath, &b);
  ASSERT_STRCONTAINS(b.buf, "line 0");
  ASSERT_STRCONTAINS(b.buf, "line 29");
  sb_free(&b);
  remove(logpath);
}

TEST(extract_log_errors_tail_more_than_30) {
  const char *logpath = "/tmp/ltxpng_test_50.log";
  FILE *f = fopen(logpath, "w");
  ASSERT_TRUE(f != NULL);
  for (int i = 0; i < 50; i++)
    fprintf(f, "line %d\n", i);
  fclose(f);

  sb b;
  sb_init(&b);
  extract_log_errors(logpath, &b);
  /* Should NOT contain line 0-19 (only last 30 lines) */
  ASSERT_TRUE(strstr(b.buf, "line 0\n") == NULL);
  ASSERT_STRCONTAINS(b.buf, "line 20");
  ASSERT_STRCONTAINS(b.buf, "line 49");
  sb_free(&b);
  remove(logpath);
}

TEST(tmpdir_remove_deeply_nested) {
  char *path = tmpdir_create();
  ASSERT_TRUE(path != NULL);

  char sub1[4096], sub2[4096];
  snprintf(sub1, sizeof(sub1), "%s/a", path);
  snprintf(sub2, sizeof(sub2), "%s/a/b", path);
  ASSERT_EQ(mkdir(sub1, 0755), 0);
  ASSERT_EQ(mkdir(sub2, 0755), 0);

  char fpath[4096];
  snprintf(fpath, sizeof(fpath), "%s/a/b/deep.txt", path);
  FILE *f = fopen(fpath, "w");
  ASSERT_TRUE(f != NULL);
  fprintf(f, "deep");
  fclose(f);

  tmpdir_remove(path);
  struct stat st;
  ASSERT_EQ(stat(path, &st), -1);
  free(path);
}

TEST(run_cmd_stderr_only_captured) {
  const char *capfile = "/tmp/ltxpng_test_stderr_only.txt";
  const char *argv[] = {"sh", "-c", "echo errormsg >&2", NULL};
  int rc = run_cmd(argv, capfile);
  ASSERT_EQ(rc, 0);

  FILE *f = fopen(capfile, "r");
  ASSERT_TRUE(f != NULL);
  size_t len;
  char *content = read_all(f, &len);
  fclose(f);
  ASSERT_STRCONTAINS(content, "errormsg");
  free(content);
  remove(capfile);
}

int main(void) {
  return test_run_all();
}
