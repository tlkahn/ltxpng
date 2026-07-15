/* ltxpng: unit tests for detect module */

#include "test.h"
#include "detect.h"
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

int main(void) {
  return test_run_all();
}
