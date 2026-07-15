/* ltxpng: fragment classification and auto-package detection */

#include "detect.h"
#include <string.h>
#include <ctype.h>

/* Residual non-algorithmic package triggers */
static const pkg_entry pkg_table[] = {
  {"\\begin{tikzpicture}", "tikz"},
};
static const int pkg_table_count = sizeof(pkg_table) / sizeof(pkg_table[0]);

/* Old-style algorithmic package uppercase commands (case-sensitive) */
static const char *uc_cmds[] = {
  "\\STATE", "\\REQUIRE", "\\ENSURE", "\\IF", "\\ELSE", "\\ELSIF", "\\ENDIF",
  "\\FOR", "\\FORALL", "\\ENDFOR", "\\WHILE", "\\ENDWHILE", "\\REPEAT",
  "\\UNTIL", "\\LOOP", "\\ENDLOOP", "\\RETURN", "\\PRINT", "\\COMMENT",
};
static const int uc_cmds_count = sizeof(uc_cmds) / sizeof(uc_cmds[0]);

static void add_pkg(detect_result *res, const char *pkg) {
  for (int j = 0; j < res->n_pkgs; j++) {
    if (strcmp(res->pkgs[j], pkg) == 0)
      return;
  }
  if (res->n_pkgs < DETECT_MAX_PKGS) {
    strncpy(res->pkgs[res->n_pkgs], pkg, 63);
    res->pkgs[res->n_pkgs][63] = '\0';
    res->n_pkgs++;
  }
}

static void add_extra(detect_result *res, const char *line) {
  if (res->n_extra < DETECT_MAX_EXTRA) {
    strncpy(res->extra_preamble[res->n_extra], line, 511);
    res->extra_preamble[res->n_extra][511] = '\0';
    res->n_extra++;
  }
}

static int has_uc_cmd(const char *fragment) {
  for (int i = 0; i < uc_cmds_count; i++) {
    if (strstr(fragment, uc_cmds[i]) != NULL)
      return 1;
  }
  return 0;
}

frag_kind classify_fragment(const char *fragment) {
  /* Skip leading whitespace */
  while (*fragment && (unsigned char)*fragment <= ' ') fragment++;

  if (strstr(fragment, "\\documentclass") != NULL)
    return FRAG_FULLDOC;

  if (strncmp(fragment, "\\begin{", 7) == 0)
    return FRAG_RAW;

  if (strncmp(fragment, "\\[", 2) == 0)
    return FRAG_RAW;

  return FRAG_DISPLAY;
}

void detect_packages(const char *fragment, detect_result *res) {
  memset(res, 0, sizeof(*res));

  int uc  = has_uc_cmd(fragment);
  int cc  = strstr(fragment, "\\State") != NULL;
  int env = strstr(fragment, "\\begin{algorithmic}") != NULL;

  if (uc) {
    add_pkg(res, "algorithmic");
    if (cc)
      res->mixed_algo_styles = 1;
  } else if (cc || env) {
    add_pkg(res, "algpseudocode");
  }

  if (strstr(fragment, "\\begin{algorithm}") != NULL) {
    add_pkg(res, "algorithm");
    add_pkg(res, "float");
    /* standalone+varwidth is not outer par mode; floats fail there.
       floatplacement{H} alone is insufficient for float.sty \\newfloat
       environments, so also redefine algorithm as an in-place minipage. */
    add_extra(res, "\\floatplacement{algorithm}{H}");
    add_extra(res,
      "\\makeatletter"
      "\\let\\algorithm\\relax\\let\\endalgorithm\\relax"
      "\\newenvironment{algorithm}[1][]{"
      "\\par\\noindent\\begin{minipage}{\\linewidth}"
      "\\hrule height.8pt depth0pt \\kern2pt"
      "\\def\\caption##1{\\refstepcounter{algorithm}"
      "\\textbf{Algorithm~\\thealgorithm}\\quad ##1"
      "\\par\\kern2pt\\hrule\\kern2pt}"
      "}{"
      "\\par\\kern2pt\\hrule\\relax\\end{minipage}\\par"
      "}"
      "\\makeatother");
  }

  for (int i = 0; i < pkg_table_count; i++) {
    if (strstr(fragment, pkg_table[i].keyword) != NULL)
      add_pkg(res, pkg_table[i].package);
  }
}
