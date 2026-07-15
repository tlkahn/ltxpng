/* ltxpng: LaTeX fragment to PNG CLI tool */

#include "args.h"
#include "detect.h"
#include "template.h"
#include "run.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <libgen.h>

#define MAX_AUTO_PKGS 16
#define MAX_PREAMBLE_ITEMS 64

static int check_tool(const char *name) {
  char *path = find_on_path(name);
  if (!path) {
    fprintf(stderr, "error: required tool not found on PATH: %s\n", name);
    return 0;
  }
  free(path);
  return 1;
}

/* If arg is a path to an existing file, read its contents into buf.
   Otherwise append arg verbatim. Returns 0 on success. */
static int resolve_preamble_arg(const char *arg, sb *buf) {
  struct stat st;
  if (stat(arg, &st) == 0 && S_ISREG(st.st_mode)) {
    FILE *f = fopen(arg, "r");
    if (!f) {
      fprintf(stderr, "error: cannot open preamble file: %s\n", arg);
      return 1;
    }
    size_t len;
    char *content = read_all(f, &len);
    fclose(f);
    if (content) {
      sb_append(buf, content);
      free(content);
    }
    return 0;
  }
  /* Not a file, use verbatim */
  sb_append(buf, arg);
  return 0;
}

/* Move src to dst, with fallback for cross-device moves */
static int move_file(const char *src, const char *dst) {
  if (rename(src, dst) == 0) return 0;
  /* Cross-device: copy then unlink */
  FILE *fin = fopen(src, "rb");
  if (!fin) return -1;
  FILE *fout = fopen(dst, "wb");
  if (!fout) { fclose(fin); return -1; }
  char buf[8192];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), fin)) > 0) {
    if (fwrite(buf, 1, n, fout) != n) {
      fclose(fin); fclose(fout); return -1;
    }
  }
  fclose(fin);
  fclose(fout);
  unlink(src);
  return 0;
}

int main(int argc, char *argv[]) {
  opts o;
  if (parse_args(argc, argv, &o) != 0) {
    return 1;
  }

  /* Help and version handled by parse_args via exit(0) */

  /* Check required tools */
  if (!check_tool("xelatex")) return 127;
  if (!check_tool("gs")) return 127;

  /* Read fragment */
  char *fragment = NULL;
  int free_fragment = 0;

  if (o.fragment) {
    fragment = strdup(o.fragment);
    if (!fragment) { fprintf(stderr, "error: out of memory\n"); return 1; }
    free_fragment = 1;
  } else {
    /* Read stdin */
    size_t len;
    fragment = read_all(stdin, &len);
    if (!fragment || len == 0) {
      fprintf(stderr, "error: empty stdin\n");
      free(fragment);
      return 1;
    }
    free_fragment = 1;
    chomp(fragment);
    if (fragment[0] == '\0') {
      fprintf(stderr, "error: empty stdin\n");
      free(fragment);
      return 1;
    }
  }

  /* Classify fragment */
  frag_kind kind = classify_fragment(fragment);

  /* Detect auto packages */
  char auto_pkgs_buf[MAX_AUTO_PKGS][64];
  const char *auto_pkgs[MAX_AUTO_PKGS];
  int n_auto = 0;
  if (kind != FRAG_FULLDOC) {
    detect_packages(fragment, &n_auto, auto_pkgs_buf, MAX_AUTO_PKGS);
    for (int i = 0; i < n_auto; i++) auto_pkgs[i] = auto_pkgs_buf[i];
  }

  /* Resolve preamble args (files or verbatim) */
  char *resolved_preambles[MAX_PREAMBLE_ITEMS];
  int n_resolved = 0;
  for (int i = 0; i < o.n_preamble && n_resolved < MAX_PREAMBLE_ITEMS; i++) {
    sb b;
    sb_init(&b);
    if (resolve_preamble_arg(o.preamble[i], &b) != 0) {
      sb_free(&b);
      if (free_fragment) free(fragment);
      return 1;
    }
    /* If the resolved content is from a file, it might have trailing whitespace */
    chomp(b.buf);
    resolved_preambles[n_resolved] = sb_detach(&b);
    n_resolved++;
  }

  /* Build template */
  sb doc;
  sb_init(&doc);
  build_template(&doc, fragment, kind, o.margin, o.mode_override,
                 (const char **)auto_pkgs, n_auto,
                 (const char **)resolved_preambles, n_resolved);

  /* Create temp dir */
  char *tmpdir = tmpdir_create();
  if (!tmpdir) {
    fprintf(stderr, "error: could not create temp directory\n");
    if (free_fragment) free(fragment);
    for (int i = 0; i < n_resolved; i++) free(resolved_preambles[i]);
    free(o.preamble);
    sb_free(&doc);
    return 1;
  }

  int exit_code = 0;
  char tex_path[4096];
  snprintf(tex_path, sizeof(tex_path), "%s/job.tex", tmpdir);

  /* Write job.tex */
  FILE *tex = fopen(tex_path, "w");
  if (!tex) {
    fprintf(stderr, "error: could not write %s\n", tex_path);
    exit_code = 1;
    goto cleanup;
  }
  fwrite(doc.buf, 1, doc.len, tex);
  fclose(tex);

  /* Run xelatex */
  if (o.verbose) fprintf(stderr, "[xelatex] compiling...\n");
  char log_path[4096];
  snprintf(log_path, sizeof(log_path), "%s/job.log", tmpdir);
  char pdf_path[4096];
  snprintf(pdf_path, sizeof(pdf_path), "%s/job.pdf", tmpdir);

  const char *xelatex_argv[] = {
    "xelatex",
    "-interaction=nonstopmode",
    "-halt-on-error",
    "-output-directory", tmpdir,
    tex_path,
    NULL
  };

  int xelatex_rc = run_cmd(xelatex_argv, o.verbose ? NULL : log_path);
  if (xelatex_rc != 0) {
    /* Print error region from log */
    sb err;
    sb_init(&err);
    extract_log_errors(log_path, &err);
    fprintf(stderr, "%s", err.buf);
    sb_free(&err);
    exit_code = 2;
    goto cleanup;
  }

  /* Full document mode: run pdfcrop */
  char crop_pdf[4096] = "";
  if (kind == FRAG_FULLDOC) {
    /* Compute margin in bp (1pt = 1/72.27 inch, 1bp = 1/72 inch, approx equal) */
    char margin_bp[64];
    /* Parse the margin dimension - strip the unit, use numeric value */
    const char *m = o.margin;
    char *end;
    double margin_val = strtod(m, &end);
    if (end && *end) {
      snprintf(margin_bp, sizeof(margin_bp), "%.0f", margin_val);
    } else {
      snprintf(margin_bp, sizeof(margin_bp), "%.0f", margin_val);
    }

    snprintf(crop_pdf, sizeof(crop_pdf), "%s/cropped.pdf", tmpdir);
    const char *pdfcrop_argv[] = {
      "pdfcrop",
      "--margins", margin_bp,
      pdf_path,
      crop_pdf,
      NULL
    };
    if (o.verbose) fprintf(stderr, "[pdfcrop] trimming...\n");
    int crop_rc = run_cmd(pdfcrop_argv, o.verbose ? NULL : log_path);
    if (crop_rc != 0) {
      sb err;
      sb_init(&err);
      extract_log_errors(log_path, &err);
      fprintf(stderr, "%s", err.buf);
      sb_free(&err);
      exit_code = 3;
      goto cleanup;
    }
  }

  /* Run Ghostscript */
  if (o.verbose) fprintf(stderr, "[gs] rasterizing...\n");
  char png_path[4096];
  snprintf(png_path, sizeof(png_path), "%s/out.png", tmpdir);
  /* Build device arg as single token: -sDEVICE=png16m */
  char dev_arg[64];
  snprintf(dev_arg, sizeof(dev_arg), "-sDEVICE=%s",
           o.transparent ? "pngalpha" : "png16m");
  char dpi_arg[32];
  snprintf(dpi_arg, sizeof(dpi_arg), "-r%d", o.dpi);

  const char *gs_argv[16];
  int gs_idx = 0;
  gs_argv[gs_idx++] = "gs";
  gs_argv[gs_idx++] = "-dSAFER";
  gs_argv[gs_idx++] = "-dBATCH";
  gs_argv[gs_idx++] = "-dNOPAUSE";
  gs_argv[gs_idx++] = dpi_arg;
  gs_argv[gs_idx++] = dev_arg;
  if (!o.transparent) {
    gs_argv[gs_idx++] = "-dTextAlphaBits=4";
    gs_argv[gs_idx++] = "-dGraphicsAlphaBits=4";
  }
  gs_argv[gs_idx++] = "-o";
  gs_argv[gs_idx++] = png_path;
  gs_argv[gs_idx++] = kind == FRAG_FULLDOC ? crop_pdf : pdf_path;
  gs_argv[gs_idx++] = NULL;

  int gs_rc = run_cmd(gs_argv, o.verbose ? NULL : log_path);

  if (gs_rc != 0) {
    sb err;
    sb_init(&err);
    extract_log_errors(log_path, &err);
    fprintf(stderr, "%s", err.buf);
    sb_free(&err);
    exit_code = 3;
    goto cleanup;
  }

  /* Move output to destination */
  if (move_file(png_path, o.output) != 0) {
    fprintf(stderr, "error: could not write %s\n", o.output);
    exit_code = 1;
    goto cleanup;
  }

cleanup:
  if (o.keep && exit_code == 0) {
    printf("%s\n", tmpdir);
  } else if (!o.keep) {
    tmpdir_remove(tmpdir);
  }

  if (free_fragment) free(fragment);
  for (int i = 0; i < n_resolved; i++) free(resolved_preambles[i]);
  free(o.preamble);
  sb_free(&doc);
  free(tmpdir);

  return exit_code;
}
