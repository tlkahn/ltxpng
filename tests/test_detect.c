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
  detect_result res;
  detect_packages("\\begin{algorithmic}", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
}

TEST(algpseudocode_detected_from_State) {
  detect_result res;
  detect_packages("\\State something", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
}

TEST(algorithm_detected) {
  detect_result res;
  detect_packages("\\begin{algorithm}", &res);
  ASSERT_EQ(res.n_pkgs, 2);
  ASSERT_STREQ(res.pkgs[0], "algorithm");
  ASSERT_STREQ(res.pkgs[1], "float");
  ASSERT_EQ(res.n_extra, 2);
  ASSERT_STREQ(res.extra_preamble[0], "\\floatplacement{algorithm}{H}");
  /* second extra line redefines algorithm as non-float for standalone */
  ASSERT_TRUE(strstr(res.extra_preamble[1], "\\newenvironment{algorithm}") != NULL);
}

TEST(tikz_detected) {
  detect_result res;
  detect_packages("\\begin{tikzpicture}", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "tikz");
}

TEST(plain_math_yields_no_packages) {
  detect_result res;
  detect_packages("E = mc^2", &res);
  ASSERT_EQ(res.n_pkgs, 0);
}

TEST(duplicate_packages_not_repeated) {
  detect_result res;
  detect_packages("\\begin{algorithmic} \\State \\begin{algorithmic}", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
}

TEST(multiple_packages_detected) {
  detect_result res;
  detect_packages("\\begin{algorithmic} \\begin{tikzpicture} \\State", &res);
  ASSERT_EQ(res.n_pkgs, 2);
  /* Order depends on table order: algpseudocode first (two triggers), tikz second */
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
  ASSERT_STREQ(res.pkgs[1], "tikz");
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
  detect_result res;
  detect_packages("\\StateSpace is wide", &res);
  /* strstr matches \\State as prefix of \\StateSpace - this IS current behavior */
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
}

TEST(all_four_package_triggers_at_once) {
  detect_result res;
  detect_packages(
    "\\begin{algorithm}\n\\begin{algorithmic}\n"
    "\\State x\n\\begin{tikzpicture}", &res);
  ASSERT_EQ(res.n_pkgs, 4);
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
  ASSERT_STREQ(res.pkgs[1], "algorithm");
  ASSERT_STREQ(res.pkgs[2], "float");
  ASSERT_STREQ(res.pkgs[3], "tikz");
  ASSERT_EQ(res.n_extra, 2);
  ASSERT_STREQ(res.extra_preamble[0], "\\floatplacement{algorithm}{H}");
  ASSERT_TRUE(strstr(res.extra_preamble[1], "\\newenvironment{algorithm}") != NULL);
}

TEST(algorithm_float_shim_extra_preamble) {
  detect_result res;
  detect_packages("\\begin{algorithm}", &res);
  ASSERT_EQ(res.n_extra, 2);
  ASSERT_STREQ(res.extra_preamble[0], "\\floatplacement{algorithm}{H}");
  ASSERT_TRUE(strstr(res.extra_preamble[1], "\\newenvironment{algorithm}") != NULL);
  /* packages include algorithm + float */
  int has_algorithm = 0, has_float = 0;
  for (int i = 0; i < res.n_pkgs; i++) {
    if (strcmp(res.pkgs[i], "algorithm") == 0) has_algorithm = 1;
    if (strcmp(res.pkgs[i], "float") == 0) has_float = 1;
  }
  ASSERT_EQ(has_algorithm, 1);
  ASSERT_EQ(has_float, 1);
}

TEST(packages_with_max_one_slot) {
  /* max param is gone; repurpose to assert internal bounds */
  detect_result res;
  detect_packages(
    "\\begin{algorithmic} \\begin{tikzpicture} \\begin{algorithm} \\State",
    &res);
  ASSERT_TRUE(res.n_pkgs <= DETECT_MAX_PKGS);
  ASSERT_TRUE(res.n_pkgs > 0);
}

TEST(packages_empty_fragment) {
  detect_result res;
  detect_packages("", &res);
  ASSERT_EQ(res.n_pkgs, 0);
}

/* --- Uppercase algorithmic package selection --- */

TEST(uppercase_STATE_with_begin_yields_algorithmic) {
  detect_result res;
  detect_packages(
    "\\begin{algorithmic}\\STATE x\\end{algorithmic}", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
  /* never load algpseudocode when uppercase wins */
  for (int i = 0; i < res.n_pkgs; i++)
    ASSERT_TRUE(strcmp(res.pkgs[i], "algpseudocode") != 0);
}

TEST(uppercase_REQUIRE_with_begin_yields_algorithmic) {
  detect_result res;
  detect_packages(
    "\\begin{algorithmic}\\REQUIRE x\\ENSURE y\\end{algorithmic}", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
}

TEST(bare_RETURN_yields_algorithmic) {
  detect_result res;
  detect_packages("\\RETURN x", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
}

TEST(uppercase_FOR_yields_algorithmic) {
  detect_result res;
  detect_packages("\\FOR{i} x \\ENDFOR", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
}

TEST(uppercase_ENDIF_yields_algorithmic) {
  detect_result res;
  detect_packages("\\IF{c} \\ENDIF", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
}

TEST(uppercase_WHILE_yields_algorithmic) {
  detect_result res;
  detect_packages("\\WHILE{c} \\ENDWHILE", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
}

TEST(uppercase_ENSURE_yields_algorithmic) {
  detect_result res;
  detect_packages("\\ENSURE y", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
}

TEST(mixed_STATE_and_State_yields_algorithmic_with_flag) {
  detect_result res;
  detect_packages("\\STATE x \\State y", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
  ASSERT_EQ(res.mixed_algo_styles, 1);
}

TEST(uppercase_only_mixed_flag_is_zero) {
  detect_result res;
  detect_packages("\\STATE x \\RETURN y", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algorithmic");
  ASSERT_EQ(res.mixed_algo_styles, 0);
}

TEST(camelcase_only_mixed_flag_is_zero) {
  detect_result res;
  detect_packages("\\State x", &res);
  ASSERT_EQ(res.n_pkgs, 1);
  ASSERT_STREQ(res.pkgs[0], "algpseudocode");
  ASSERT_EQ(res.mixed_algo_styles, 0);
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
