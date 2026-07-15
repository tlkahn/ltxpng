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

int main(void) {
  return test_run_all();
}
