/* ltxpng: unit tests for args module */

#include "test.h"
#include "args.h"
#include <string.h>
#include <stdlib.h>

/*
 * Wrapper: reset optind before each call so getopt_long re-processes.
 * We define it here since argc/argv based testing needs external optind reset.
 */
#include <getopt.h>
#include <unistd.h>

static int parse_test(int argc, char *argv[], opts *o) {
  optind = 1;   /* reset getopt_long */
  opterr = 0;   /* suppress getopt error messages */
  return parse_args(argc, argv, o);
}

TEST(no_args_returns_defaults) {
  opts o;
  char *args[] = {"ltxpng", NULL};
  int ret = parse_test(1, args, &o);
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(o.fragment == NULL);   /* stdin */
  ASSERT_STREQ(o.output, "out.png");
  ASSERT_EQ(o.dpi, 300);
  ASSERT_EQ(o.transparent, 0);
  ASSERT_STREQ(o.margin, "2pt");
  ASSERT_EQ(o.mode_override, -1);
  ASSERT_EQ(o.keep, 0);
  ASSERT_EQ(o.verbose, 0);
  free(o.preamble);
}

TEST(o_flag_sets_output) {
  opts o;
  char *args[] = {"ltxpng", "-o", "result.png", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 0);
  ASSERT_STREQ(o.output, "result.png");
  free(o.preamble);
}

TEST(d_flag_sets_dpi) {
  opts o;
  char *args[] = {"ltxpng", "-d", "600", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 0);
  ASSERT_EQ(o.dpi, 600);
  free(o.preamble);
}

TEST(t_flag_sets_transparent) {
  opts o;
  char *args[] = {"ltxpng", "-t", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_EQ(o.transparent, 1);
  free(o.preamble);
}

TEST(m_flag_sets_margin) {
  opts o;
  char *args[] = {"ltxpng", "-m", "10pt", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 0);
  ASSERT_STREQ(o.margin, "10pt");
  free(o.preamble);
}

TEST(positional_arg_sets_fragment) {
  opts o;
  char *args[] = {"ltxpng", "E = mc^2", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_STREQ(o.fragment, "E = mc^2");
  free(o.preamble);
}

TEST(dash_positional_means_stdin) {
  opts o;
  char *args[] = {"ltxpng", "-", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_TRUE(o.fragment == NULL);  /* stdin */
  free(o.preamble);
}

TEST(repeated_preamble_accumulates_in_order) {
  opts o;
  char *args[] = {"ltxpng", "-p", "\\usepackage{foo}", "-p", "\\usepackage{bar}", NULL};
  ASSERT_EQ(parse_test(5, args, &o), 0);
  ASSERT_EQ(o.n_preamble, 2);
  ASSERT_STREQ(o.preamble[0], "\\usepackage{foo}");
  ASSERT_STREQ(o.preamble[1], "\\usepackage{bar}");
  free(o.preamble[0]);
  free(o.preamble[1]);
  free(o.preamble);
}

TEST(inline_flag_sets_mode_override) {
  opts o;
  char *args[] = {"ltxpng", "--inline", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_EQ(o.mode_override, 0);
  free(o.preamble);
}

TEST(display_flag_sets_mode_override) {
  opts o;
  char *args[] = {"ltxpng", "--display", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_EQ(o.mode_override, 1);
  free(o.preamble);
}

TEST(raw_flag_sets_mode_override) {
  opts o;
  char *args[] = {"ltxpng", "--raw", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_EQ(o.mode_override, 2);
  free(o.preamble);
}

TEST(invalid_dpi_returns_error) {
  opts o;
  char *args[] = {"ltxpng", "-d", "abc", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 1);
  free(o.preamble);
}

TEST(zero_dpi_returns_error) {
  opts o;
  char *args[] = {"ltxpng", "-d", "0", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 1);
  free(o.preamble);
}

TEST(negative_dpi_returns_error) {
  opts o;
  char *args[] = {"ltxpng", "-d", "-100", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 1);
  free(o.preamble);
}

TEST(keep_flag) {
  opts o;
  char *args[] = {"ltxpng", "-k", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_EQ(o.keep, 1);
  free(o.preamble);
}

TEST(verbose_flag) {
  opts o;
  char *args[] = {"ltxpng", "-v", NULL};
  ASSERT_EQ(parse_test(2, args, &o), 0);
  ASSERT_EQ(o.verbose, 1);
  free(o.preamble);
}

TEST(long_output_flag) {
  opts o;
  char *args[] = {"ltxpng", "--output", "out.png", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 0);
  ASSERT_STREQ(o.output, "out.png");
  free(o.preamble);
}

TEST(long_dpi_flag) {
  opts o;
  char *args[] = {"ltxpng", "--dpi", "150", NULL};
  ASSERT_EQ(parse_test(3, args, &o), 0);
  ASSERT_EQ(o.dpi, 150);
  free(o.preamble);
}

int main(void) {
  return test_run_all();
}
