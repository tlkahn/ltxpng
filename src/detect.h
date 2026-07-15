/* ltxpng: fragment classification and auto-package detection */

#ifndef DETECT_H
#define DETECT_H

typedef enum {
  FRAG_DISPLAY,   /* inline math auto-wrapped in \[ ... \] */
  FRAG_RAW,       /* verbatim into body (starts with \begin{...} or \[) */
  FRAG_FULLDOC    /* contains \documentclass; compile as-is */
} frag_kind;

typedef struct {
  const char *keyword;
  const char *package;
} pkg_entry;

frag_kind classify_fragment(const char *fragment);
void detect_packages(const char *fragment, int *count, char pkgs[][64], int max);

#endif /* DETECT_H */
