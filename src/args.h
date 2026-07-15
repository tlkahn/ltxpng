/* ltxpng: CLI argument parsing */

#ifndef ARGS_H
#define ARGS_H

typedef struct {
  const char *fragment;   /* NULL means read stdin */
  const char *output;     /* default "out.png" */
  int dpi;                /* default 300 */
  int transparent;        /* default 0 */
  const char *margin;     /* default "2pt" */
  int mode_override;      /* -1 auto, 0 inline, 1 display, 2 raw */
  int keep;               /* keep temp dir */
  int verbose;            /* show child output */
  char **preamble;        /* array of preamble strings */
  int n_preamble;         /* count */
} opts;

/* Parse argv. On success, fills opts and returns 0.
   On error, prints message to stderr and returns 1. */
int parse_args(int argc, char *argv[], opts *o);

#endif /* ARGS_H */
