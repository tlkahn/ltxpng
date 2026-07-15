/* ltxpng: CLI argument parsing via getopt_long */

#include "args.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

#define MAX_PREAMBLE 64

int parse_args(int argc, char *argv[], opts *o) {
  /* Defaults */
  o->fragment = NULL;
  o->output = "out.png";
  o->dpi = 300;
  o->transparent = 0;
  o->margin = "2pt";
  o->mode_override = -1;
  o->keep = 0;
  o->verbose = 0;
  o->no_auto_packages = 0;
  o->preamble = NULL;
  o->n_preamble = 0;

  static struct option long_opts[] = {
    {"output",           required_argument, 0, 'o'},
    {"dpi",              required_argument, 0, 'd'},
    {"transparent",      no_argument,       0, 't'},
    {"preamble",         required_argument, 0, 'p'},
    {"margin",           required_argument, 0, 'm'},
    {"inline",           no_argument,       0, 1},
    {"display",          no_argument,       0, 2},
    {"raw",              no_argument,       0, 3},
    {"no-auto-packages", no_argument,       0, 4},
    {"keep",             no_argument,       0, 'k'},
    {"verbose",          no_argument,       0, 'v'},
    {"help",             no_argument,       0, 'h'},
    {"version",          no_argument,       0, 'V'},
    {0, 0, 0, 0}
  };

  /* Pre-allocate preamble storage */
  char **preambles = (char **)calloc(MAX_PREAMBLE, sizeof(char *));
  int np = 0;

  int c;
  while ((c = getopt_long(argc, argv, "o:d:tp:m:kvhV", long_opts, NULL)) != -1) {
    switch (c) {
      case 'o': o->output = optarg; break;
      case 'd': {
        char *end;
        long val = strtol(optarg, &end, 10);
        if (*end != '\0' || val <= 0) {
          fprintf(stderr, "error: invalid DPI '%s'\n", optarg);
          free(preambles);
          return 1;
        }
        o->dpi = (int)val;
        break;
      }
      case 't': o->transparent = 1; break;
      case 'p':
        if (np >= MAX_PREAMBLE) {
          fprintf(stderr, "error: too many -p options (max %d)\n", MAX_PREAMBLE);
          free(preambles);
          return 1;
        }
        preambles[np] = strdup(optarg);
        if (preambles[np]) np++;
        break;
      case 'm': o->margin = optarg; break;
      case 1:  o->mode_override = 0; break;  /* --inline */
      case 2:  o->mode_override = 1; break;  /* --display */
      case 3:  o->mode_override = 2; break;  /* --raw */
      case 4:  o->no_auto_packages = 1; break;
      case 'k': o->keep = 1; break;
      case 'v': o->verbose = 1; break;
      case 'h':
        fprintf(stderr,
          "usage: ltxpng [options] [FRAGMENT]\n"
          "  --no-auto-packages  skip automatic package detection\n");
        free(preambles);
        exit(0);
      case 'V':
        fprintf(stderr, "ltxpng 0.1.0\n");
        free(preambles);
        exit(0);
      default:
        free(preambles);
        return 1;
    }
  }

  /* Positional argument: fragment */
  if (optind < argc) {
    o->fragment = argv[optind];
    if (strcmp(o->fragment, "-") == 0)
      o->fragment = NULL; /* stdin */
  }

  o->preamble = preambles;
  o->n_preamble = np;
  return 0;
}
