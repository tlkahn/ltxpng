/* ltxpng: LaTeX document template generation */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "detect.h"
#include "util.h"

/* Build the full .tex document into buf (a pre-initialised sb).
   fragment: the user's LaTeX source.
   kind: classification from classify_fragment.
   margin: TeX dimension string (e.g. "2pt").
   mode_override: -1 for auto, 0 for inline, 1 for display, 2 for raw.
   auto_pkgs: auto-detected package names.
   n_auto_pkgs: count of auto_pkgs.
   user_preamble: user-provided preamble strings.
   n_user_preamble: count of user_preamble. */
void build_template(sb *buf,
                    const char *fragment,
                    frag_kind kind,
                    const char *margin,
                    int mode_override,
                    const char *auto_pkgs[], int n_auto_pkgs,
                    const char *user_preamble[], int n_user_preamble);

#endif /* TEMPLATE_H */
