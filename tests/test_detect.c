/* ltxpng: unit tests for detect module */

#include "test.h"
#include "detect.h"
#include "util.h"
#include <string.h>

TEST(plain_math_classifies_as_display) {
  frag_kind k = classify_fragment("E = mc^2");
  ASSERT_EQ(k, FRAG_DISPLAY);
}

TEST(begin_env_classifies_as_raw) {
  frag_kind k = classify_fragment("\\begin{equation} x^2 \\end{equation}");
  ASSERT_EQ(k, FRAG_RAW);
}

TEST(begin_env_with_ws_classifies_as_raw) {
  frag_kind k = classify_fragment("  \t\n\\begin{equation}");
  ASSERT_EQ(k, FRAG_RAW);
}

TEST(display_math_bracket_classifies_as_raw) {
  frag_kind k = classify_fragment("\\[ x^2 \\]");
  ASSERT_EQ(k, FRAG_RAW);
}

TEST(documentclass_anywhere_classifies_as_fulldoc) {
  frag_kind k = classify_fragment("\\documentclass{article}");
  ASSERT_EQ(k, FRAG_FULLDOC);
}

TEST(documentclass_in_middle_classifies_as_fulldoc) {
  frag_kind k = classify_fragment("% comment\n\\documentclass[a4paper]{article}");
  ASSERT_EQ(k, FRAG_FULLDOC);
}

TEST(empty_fragment_classifies_as_display) {
  frag_kind k = classify_fragment("");
  ASSERT_EQ(k, FRAG_DISPLAY);
}

/* --- Package detection --- */

TEST(algpseudocode_detected_from_begin_algorithmic) {
  char pkgs[5][64];
  int n;
  detect_packages("\\begin{algorithmic}", &n, pkgs, 5);
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "algpseudocode");
}

TEST(algpseudocode_detected_from_State) {
  char pkgs[5][64];
  int n;
  detect_packages("\\State something", &n, pkgs, 5);
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "algpseudocode");
}

TEST(algorithm_detected) {
  char pkgs[5][64];
  int n;
  detect_packages("\\begin{algorithm}", &n, pkgs, 5);
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "algorithm");
}

TEST(tikz_detected) {
  char pkgs[5][64];
  int n;
  detect_packages("\\begin{tikzpicture}", &n, pkgs, 5);
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "tikz");
}

TEST(plain_math_yields_no_packages) {
  char pkgs[5][64];
  int n;
  detect_packages("E = mc^2", &n, pkgs, 5);
  ASSERT_EQ(n, 0);
}

TEST(duplicate_packages_not_repeated) {
  char pkgs[5][64];
  int n;
  detect_packages("\\begin{algorithmic} \\State \\begin{algorithmic}", &n, pkgs, 5);
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "algpseudocode");
}

TEST(multiple_packages_detected) {
  char pkgs[5][64];
  int n;
  detect_packages("\\begin{algorithmic} \\begin{tikzpicture} \\State", &n, pkgs, 5);
  ASSERT_EQ(n, 2);
  /* Order depends on table order: algpseudocode first (two triggers), tikz second */
  ASSERT_STREQ(pkgs[0], "algpseudocode");
  ASSERT_STREQ(pkgs[1], "tikz");
}

/* --- Edge cases: classification --- */

TEST(whitespace_only_classifies_as_display) {
  ASSERT_EQ(classify_fragment("   \t\n  "), FRAG_DISPLAY);
}

TEST(backslash_bracket_with_leading_ws_classifies_as_raw) {
  ASSERT_EQ(classify_fragment("  \n  \\[ x \\]"), FRAG_RAW);
}

TEST(documentclass_beats_begin) {
  ASSERT_EQ(classify_fragment(
    "\\begin{document}\n\\documentclass{article}"), FRAG_FULLDOC);
}

TEST(begin_align_classifies_as_raw) {
  ASSERT_EQ(classify_fragment("\\begin{align}x &= 1\\end{align}"), FRAG_RAW);
}

TEST(begin_equation_star_classifies_as_raw) {
  ASSERT_EQ(classify_fragment("\\begin{equation*}x\\end{equation*}"), FRAG_RAW);
}

TEST(plain_fraction_classifies_as_display) {
  ASSERT_EQ(classify_fragment("\\frac{a}{b}"), FRAG_DISPLAY);
}

TEST(plain_sum_classifies_as_display) {
  ASSERT_EQ(classify_fragment("\\sum_{i=0}^{n} x_i"), FRAG_DISPLAY);
}

TEST(fragment_with_newlines_and_begin) {
  ASSERT_EQ(classify_fragment(
    "\n\n\n\\begin{itemize}\n\\item one\n\\end{itemize}"), FRAG_RAW);
}

TEST(long_math_fragment_classifies_as_display) {
  sb b;
  sb_init(&b);
  for (int i = 0; i < 100; i++)
    sb_appendf(&b, "x_%d + ", i);
  sb_append(&b, "0");
  ASSERT_EQ(classify_fragment(b.buf), FRAG_DISPLAY);
  sb_free(&b);
}

TEST(documentclass_in_comment_still_detected) {
  ASSERT_EQ(classify_fragment(
    "% \\documentclass{article}\nE = mc^2"), FRAG_FULLDOC);
}

/* --- Edge cases: package detection --- */

TEST(State_as_substring_falsely_detected) {
  char pkgs[5][64];
  int n;
  detect_packages("\\StateSpace is wide", &n, pkgs, 5);
  /* strstr matches \\State as prefix of \\StateSpace - this IS current behavior */
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "algpseudocode");
}

TEST(all_four_package_triggers_at_once) {
  char pkgs[5][64];
  int n;
  detect_packages(
    "\\begin{algorithm}\n\\begin{algorithmic}\n"
    "\\State x\n\\begin{tikzpicture}", &n, pkgs, 5);
  ASSERT_EQ(n, 3);
  ASSERT_STREQ(pkgs[0], "algpseudocode");
  ASSERT_STREQ(pkgs[1], "algorithm");
  ASSERT_STREQ(pkgs[2], "tikz");
}

TEST(packages_with_max_one_slot) {
  char pkgs[1][64];
  int n;
  detect_packages("\\begin{algorithmic} \\begin{tikzpicture}", &n, pkgs, 1);
  ASSERT_EQ(n, 1);
  ASSERT_STREQ(pkgs[0], "algpseudocode");
}

TEST(packages_empty_fragment) {
  char pkgs[5][64];
  int n;
  detect_packages("", &n, pkgs, 5);
  ASSERT_EQ(n, 0);
}

TEST(begin_with_no_closing_brace) {
  ASSERT_EQ(classify_fragment("\\begin{"), FRAG_RAW);
}

TEST(just_backslash_bracket_no_content) {
  ASSERT_EQ(classify_fragment("\\[\\]"), FRAG_RAW);
}

TEST(fragment_with_unicode) {
  ASSERT_EQ(classify_fragment("\xce\xb1 + \xce\xb2 = \xce\xb3"), FRAG_DISPLAY);
}

TEST(fragment_with_multiline_documentclass) {
  ASSERT_EQ(classify_fragment(
    "preamble text\nmore text\n\\documentclass[12pt]{report}\n"
    "\\begin{document}\nHello\n\\end{document}"), FRAG_FULLDOC);
}

int main(void) {
  return test_run_all();
}
