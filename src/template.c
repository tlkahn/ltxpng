/* ltxpng: LaTeX document template generation */

#include "template.h"
#include "util.h"
#include <string.h>

void build_template(sb *buf,
                    const char *fragment,
                    frag_kind kind,
                    const char *margin,
                    int mode_override,
                    const char *auto_pkgs[], int n_auto_pkgs,
                    const char *user_preamble[], int n_user_preamble)
{
  /* Determine effective wrapping mode:
     mode_override: -1 auto, 0 inline, 1 display, 2 raw
     frag_kind: 0=display, 1=raw (FULLDOC handled above) */
  int effective_mode;
  if (mode_override >= 0) {
    effective_mode = mode_override;
  } else {
    switch (kind) {
      case FRAG_DISPLAY: effective_mode = 1; break;
      case FRAG_RAW:     effective_mode = 2; break;
      default:           effective_mode = 2; break;
    }
  }

  if (kind == FRAG_FULLDOC) {
    sb_append(buf, fragment);
    return;
  }

  /* Preamble */
  sb_appendf(buf, "\\documentclass[border=%s,varwidth]{standalone}\n", margin);
  sb_append(buf, "\\usepackage{amsmath,amssymb}\n");

  for (int i = 0; i < n_auto_pkgs; i++)
    sb_appendf(buf, "\\usepackage{%s}\n", auto_pkgs[i]);

  for (int i = 0; i < n_user_preamble; i++)
    sb_appendf(buf, "%s\n", user_preamble[i]);

  sb_append(buf, "\\begin{document}\n");

  /* Body wrapping */
  switch (effective_mode) {
    case 0: /* inline */
      sb_appendf(buf, "$%s$\n", fragment);
      break;
    case 1: /* display */
      sb_appendf(buf, "\\[%s\\]\n", fragment);
      break;
    case 2: /* raw */
    default:
      sb_appendf(buf, "%s\n", fragment);
      break;
  }

  sb_append(buf, "\\end{document}\n");
}
