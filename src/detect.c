/* ltxpng: fragment classification and auto-package detection */

#include "detect.h"
#include <string.h>
#include <ctype.h>

static const pkg_entry pkg_table[] = {
  {"\\begin{algorithmic}", "algpseudocode"},
  {"\\State",              "algpseudocode"},
  {"\\begin{algorithm}",   "algorithm"},
  {"\\begin{tikzpicture}", "tikz"},
};
static const int pkg_table_count = sizeof(pkg_table) / sizeof(pkg_table[0]);

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

void detect_packages(const char *fragment, int *count, char pkgs[][64], int max) {
  *count = 0;
  for (int i = 0; i < pkg_table_count; i++) {
    if (strstr(fragment, pkg_table[i].keyword) != NULL) {
      /* Check for duplicates */
      int dup = 0;
      for (int j = 0; j < *count; j++) {
        if (strcmp(pkgs[j], pkg_table[i].package) == 0) {
          dup = 1;
          break;
        }
      }
      if (!dup && *count < max) {
        strncpy(pkgs[*count], pkg_table[i].package, 63);
        pkgs[*count][63] = '\0';
        (*count)++;
      }
    }
  }
}
