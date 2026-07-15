/* ltxpng: fragment classification and auto-package detection */

#ifndef DETECT_H
#define DETECT_H

#define DETECT_MAX_PKGS 16
#define DETECT_MAX_EXTRA 8

typedef enum {
  FRAG_DISPLAY,   /* inline math auto-wrapped in \[ ... \] */
  FRAG_RAW,       /* verbatim into body (starts with \begin{...} or \[) */
  FRAG_FULLDOC    /* contains \documentclass; compile as-is */
} frag_kind;

typedef struct {
  const char *keyword;
  const char *package;
} pkg_entry;

typedef struct {
  char pkgs[DETECT_MAX_PKGS][64];
  int  n_pkgs;
  char extra_preamble[DETECT_MAX_EXTRA][512]; /* verbatim preamble lines */
  int  n_extra;
  int  mixed_algo_styles; /* both \STATE-style and \State-style present */
} detect_result;

frag_kind classify_fragment(const char *fragment);
void detect_packages(const char *fragment, detect_result *res);

#endif /* DETECT_H */
