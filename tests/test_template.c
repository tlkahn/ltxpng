/* ltxpng: unit tests for template module */

#include "test.h"
#include "template.h"
#include "util.h"
#include <string.h>

TEST(display_fragment_properly_wrapped) {
  sb b;
  sb_init(&b);

  const char *fragment = "E = mc^2";
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, fragment, FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 0);

  const char *doc = b.buf;
  ASSERT_TRUE(strstr(doc, "\\documentclass[border=2pt,varwidth]{standalone}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{amsmath,amssymb}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\[E = mc^2\\]") != NULL);
  ASSERT_TRUE(strstr(doc, "\\end{document}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{tikz}") == NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{algpseudocode}") == NULL);

  sb_free(&b);
}

TEST(raw_fragment_inserted_verbatim) {
  sb b;
  sb_init(&b);

  const char *fragment = "\\begin{tikzpicture}\\draw (0,0) circle (1);\\end{tikzpicture}";
  const char *pkgs[] = {"tikz"};
  const char *user[] = {};
  build_template(&b, fragment, FRAG_RAW, "2pt", -1, pkgs, 1, user, 0);

  const char *doc = b.buf;
  ASSERT_TRUE(strstr(doc, "\\begin{document}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\begin{tikzpicture}\\draw (0,0) circle (1);\\end{tikzpicture}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{tikz}") != NULL);
  sb_free(&b);
}

TEST(margin_appears_in_documentclass) {
  sb b;
  sb_init(&b);

  const char *fragment = "x";
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, fragment, FRAG_DISPLAY, "10pt", -1, pkgs, 0, user, 0);

  ASSERT_TRUE(strstr(b.buf, "border=10pt") != NULL);
  sb_free(&b);
}

TEST(auto_packages_appear_as_usepackage_lines) {
  sb b;
  sb_init(&b);

  const char *fragment = "test";
  const char *pkgs[] = {"algpseudocode", "tikz"};
  const char *user[] = {};
  build_template(&b, fragment, FRAG_DISPLAY, "2pt", -1, pkgs, 2, user, 0);

  const char *doc = b.buf;
  ASSERT_TRUE(strstr(doc, "\\usepackage{amsmath,amssymb}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{algpseudocode}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{tikz}") != NULL);

  /* tikz should come after algpseudocode */
  const char *ap = strstr(doc, "\\usepackage{algpseudocode}");
  const char *tz = strstr(doc, "\\usepackage{tikz}");
  ASSERT_TRUE(ap != NULL && tz != NULL && ap < tz);

  sb_free(&b);
}

TEST(user_preamble_appears_after_auto_packages) {
  sb b;
  sb_init(&b);

  const char *fragment = "test";
  const char *pkgs[] = {"tikz"};
  const char *user[] = {"\\usepackage{hyperref}", "\\newcommand{\\foo}{bar}"};
  build_template(&b, fragment, FRAG_DISPLAY, "2pt", -1, pkgs, 1, user, 2);

  const char *doc = b.buf;
  ASSERT_TRUE(strstr(doc, "\\usepackage{tikz}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\usepackage{hyperref}") != NULL);
  ASSERT_TRUE(strstr(doc, "\\newcommand{\\foo}{bar}") != NULL);

  /* User entries after auto packages */
  const char *tz = strstr(doc, "\\usepackage{tikz}");
  const char *hr = strstr(doc, "\\usepackage{hyperref}");
  ASSERT_TRUE(tz != NULL && hr != NULL && tz < hr);

  sb_free(&b);
}

TEST(fulldoc_passed_through_unchanged) {
  sb b;
  sb_init(&b);

  const char *fragment = "\\documentclass{article}\n\\begin{document}\nHello\n\\end{document}";
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, fragment, FRAG_FULLDOC, "2pt", -1, pkgs, 0, user, 0);

  ASSERT_STREQ(b.buf, fragment);
  sb_free(&b);
}

TEST(mode_override_inline_wraps_in_dollar_signs) {
  sb b;
  sb_init(&b);

  const char *fragment = "E = mc^2";
  const char *pkgs[] = {};
  const char *user[] = {};
  /* mode_override=0 means inline */
  build_template(&b, fragment, FRAG_DISPLAY, "2pt", 0, pkgs, 0, user, 0);

  ASSERT_TRUE(strstr(b.buf, "$E = mc^2$") != NULL);
  sb_free(&b);
}

TEST(mode_override_raw_wraps_verbatim) {
  sb b;
  sb_init(&b);

  const char *fragment = "\\begin{equation}x\\end{equation}";
  const char *pkgs[] = {};
  const char *user[] = {};
  /* mode_override=2 means raw */
  build_template(&b, fragment, FRAG_RAW, "2pt", 2, pkgs, 0, user, 0);

  ASSERT_TRUE(strstr(b.buf, "\\begin{equation}x\\end{equation}") != NULL);
  sb_free(&b);
}

/* --- Edge cases --- */

TEST(empty_fragment_display_mode) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, "", FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, "\\[\\]");
  ASSERT_STRCONTAINS(b.buf, "\\begin{document}");
  ASSERT_STRCONTAINS(b.buf, "\\end{document}");
  sb_free(&b);
}

TEST(empty_fragment_inline_mode) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, "", FRAG_DISPLAY, "2pt", 0, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, "$$");
  sb_free(&b);
}

TEST(multiline_fragment_display) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  const char *frag = "a &= b + c \\\\\n  &= d + e";
  build_template(&b, frag, FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, frag);
  sb_free(&b);
}

TEST(fragment_with_percent_signs) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, "50\\% tax", FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, "50\\% tax");
  sb_free(&b);
}

TEST(large_margin_value) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, "x", FRAG_DISPLAY, "100mm", -1, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, "border=100mm");
  sb_free(&b);
}

TEST(zero_auto_pkgs_with_user_preamble) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {"\\usepackage{hyperref}"};
  build_template(&b, "x", FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 1);
  ASSERT_STRCONTAINS(b.buf, "\\usepackage{hyperref}");
  ASSERT_TRUE(strstr(b.buf, "\\usepackage{tikz}") == NULL);
  sb_free(&b);
}

TEST(document_structure_ordering) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {"tikz"};
  const char *user[] = {"\\newcommand{\\foo}{bar}"};
  build_template(&b, "test", FRAG_DISPLAY, "5pt", -1, pkgs, 1, user, 1);

  const char *docclass = strstr(b.buf, "\\documentclass");
  const char *amsmath = strstr(b.buf, "\\usepackage{amsmath");
  const char *tikzpkg = strstr(b.buf, "\\usepackage{tikz}");
  const char *usercmd = strstr(b.buf, "\\newcommand");
  const char *begdoc = strstr(b.buf, "\\begin{document}");
  const char *body = strstr(b.buf, "\\[test\\]");
  const char *enddoc = strstr(b.buf, "\\end{document}");

  ASSERT_TRUE(docclass != NULL);
  ASSERT_TRUE(amsmath != NULL && amsmath > docclass);
  ASSERT_TRUE(tikzpkg != NULL && tikzpkg > amsmath);
  ASSERT_TRUE(usercmd != NULL && usercmd > tikzpkg);
  ASSERT_TRUE(begdoc != NULL && begdoc > usercmd);
  ASSERT_TRUE(body != NULL && body > begdoc);
  ASSERT_TRUE(enddoc != NULL && enddoc > body);
  sb_free(&b);
}

TEST(mode_override_display_on_raw_kind) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, "\\begin{equation}x\\end{equation}",
                 FRAG_RAW, "2pt", 1, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, "\\[\\begin{equation}x\\end{equation}\\]");
  sb_free(&b);
}

TEST(mode_override_inline_on_raw_kind) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  build_template(&b, "x^2", FRAG_RAW, "2pt", 0, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, "$x^2$");
  sb_free(&b);
}

TEST(fulldoc_ignores_all_template_options) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {"tikz", "algpseudocode"};
  const char *user[] = {"\\usepackage{custom}"};
  const char *frag = "\\documentclass{beamer}\n\\begin{document}\nHi\n\\end{document}";
  build_template(&b, frag, FRAG_FULLDOC, "99pt", 0, pkgs, 2, user, 1);
  ASSERT_STREQ(b.buf, frag);
  sb_free(&b);
}

TEST(multiple_user_preamble_entries_order) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {"\\usepackage{a}", "\\usepackage{b}", "\\usepackage{c}"};
  build_template(&b, "x", FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 3);

  const char *a = strstr(b.buf, "\\usepackage{a}");
  const char *bb = strstr(b.buf, "\\usepackage{b}");
  const char *c = strstr(b.buf, "\\usepackage{c}");
  ASSERT_TRUE(a != NULL && bb != NULL && c != NULL);
  ASSERT_TRUE(a < bb && bb < c);
  sb_free(&b);
}

TEST(fragment_with_braces_and_backslashes) {
  sb b;
  sb_init(&b);
  const char *pkgs[] = {};
  const char *user[] = {};
  const char *frag = "\\left\\{ \\frac{1}{2} \\right\\}";
  build_template(&b, frag, FRAG_DISPLAY, "2pt", -1, pkgs, 0, user, 0);
  ASSERT_STRCONTAINS(b.buf, frag);
  sb_free(&b);
}

int main(void) {
  return test_run_all();
}
