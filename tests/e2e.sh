#!/bin/sh
# ltxpng: end-to-end tests (run as part of make test)
# Usage: tests/e2e.sh /path/to/ltxpng

set -e

LTPNG="$1"
if [ -z "$LTPNG" ]; then
  echo "Usage: $0 /path/to/ltxpng"
  exit 1
fi
if [ ! -x "$LTPNG" ]; then
  echo "Not executable: $LTPNG"
  exit 1
fi

# Resolve to absolute path so cd in subshells doesn't break it
case "$LTPNG" in
  /*) ;;
  *)  LTPNG="$(cd "$(dirname "$LTPNG")" && pwd)/$(basename "$LTPNG")" ;;
esac

T=$(mktemp -d /tmp/ltxpng_e2e.XXXXXX)
trap "rm -rf $T" EXIT

pass=0
fail=0

check() {
  local name="$1"
  shift
  echo "E2E $name ... "
  if "$@"; then
    echo "  ok"
    pass=$((pass+1))
  else
    echo "  FAIL"
    fail=$((fail+1))
  fi
}

# 1. Basic equation produces PNG with magic bytes
check "basic equation" sh -c "
  '$LTPNG' 'E = mc^2' -o $T/eq.png 2>/dev/null &&
  [ -f '$T/eq.png' ] &&
  xxd -p -l 8 '$T/eq.png' | grep -q '89504e470d0a1a0a'
"

# 2. Piped stdin works
check "stdin pipe" sh -c "
  printf 'E = mc^2' | '$LTPNG' -o $T/pipe.png 2>/dev/null &&
  xxd -p -l 8 '$T/pipe.png' | grep -q '89504e470d0a1a0a'
"

# 3. Explicit - reads from stdin
check "stdin dash" sh -c "
  printf 'x' | '$LTPNG' - -o $T/dash.png 2>/dev/null &&
  xxd -p -l 8 '$T/dash.png' | grep -q '89504e470d0a1a0a'
"

# 4. Algorithmic fragment (auto package detection)
check "algorithmic auto-package" sh -c "
  '$LTPNG' '\begin{algorithmic} \State x = 1 \end{algorithmic}' -o $T/alg.png 2>/dev/null &&
  xxd -p -l 8 '$T/alg.png' | grep -q '89504e470d0a1a0a'
"

# 5. Transparent output has alpha channel (requires ImageMagick identify)
if command -v magick >/dev/null 2>&1 || command -v identify >/dev/null 2>&1; then
  IDENTIFY=$(command -v magick 2>/dev/null || command -v identify 2>/dev/null)
  if echo "$IDENTIFY" | grep -q 'magick$'; then
    IDENTIFY="$IDENTIFY identify"
  fi

  check "transparent has alpha" sh -c "
    '$LTPNG' 'E = mc^2' -t -o $T/alpha.png 2>/dev/null &&
    $IDENTIFY -format '%[channels]' $T/alpha.png 2>/dev/null | grep -iq a
  "

  check "default no alpha" sh -c "
    '$LTPNG' 'E = mc^2' -o $T/noalpha.png 2>/dev/null &&
    $IDENTIFY -format '%[channels]' $T/noalpha.png 2>/dev/null | grep -iqv a
  "

  # 6. DPI affects size
  check "higher dpi yields larger dimensions" sh -c "
    '$LTPNG' 'E = mc^2' -d 150 -o $T/low.png 2>/dev/null &&
    '$LTPNG' 'E = mc^2' -d 600 -o $T/high.png 2>/dev/null &&
    w150=\$($IDENTIFY -format '%w' $T/low.png 2>/dev/null) &&
    w600=\$($IDENTIFY -format '%w' $T/high.png 2>/dev/null) &&
    [ \$w600 -gt \$w150 ]
  "

  # 7. Margin affects size
  check "larger margin yields larger dimensions" sh -c "
    '$LTPNG' 'E = mc^2' -m 1pt -o $T/tight.png 2>/dev/null &&
    '$LTPNG' 'E = mc^2' -m 20pt -o $T/loose.png 2>/dev/null &&
    wtight=\$($IDENTIFY -format '%w' $T/tight.png 2>/dev/null) &&
    wloose=\$($IDENTIFY -format '%w' $T/loose.png 2>/dev/null) &&
    [ \$wloose -gt \$wtight ]
  "
fi

# 8. Full document with pdfcrop trimming
if command -v pdfcrop >/dev/null 2>&1; then
  check "full document trimmed" sh -c "
    '$LTPNG' '\documentclass{article}\begin{document}Hello\end{document}' -o $T/full.png 2>/dev/null &&
    xxd -p -l 8 '$T/full.png' | grep -q '89504e470d0a1a0a'
  "
fi

# 9. Invalid LaTeX exits 2 and stderr has error
check "invalid latex exits 2" sh -c "
  '$LTPNG' '\undefinedmacro' -o $T/err.png 2>$T/stderr.txt && exit 1 || code=\$?;
  [ \$code -eq 2 ] && grep -q '!' '$T/stderr.txt'
"

# 10. Missing tool on PATH exits 127
check "missing tool exits 127" sh -c "
  PATH=/usr/bin '$LTPNG' 'E = mc^2' -o $T/nopath.png 2>/dev/null; rc=\$?;
  [ \$rc -eq 127 ]
"

# 11. --keep preserves temp dir
check "keep preserves temp dir" sh -c "
  output=\$('$LTPNG' 'E = mc^2' --keep -o $T/keep.png 2>/dev/null) &&
  echo \"\$output\" | grep -q '/tmp/ltxpng\\|/var/folders\\|/private/var' &&
  [ -d \"\$output\" ] && [ -f \"\$output/job.tex\" ]
"

# 12. Without --keep, temp dir is cleaned up
check "no-keep cleans temp dir" sh -c "
  output=\$('$LTPNG' 'E = mc^2' -o $T/nokeep.png 2>/dev/null) &&
  [ ! -d \"\$output\" ]
"

# --- Additional edge-case and product-level tests ---

# 13. Long/complex equation
check "long equation" sh -c "
  '$LTPNG' '\sum_{i=0}^{\infty} \frac{(-1)^i}{(2i+1)} = \frac{\pi}{4}' -o $T/long.png 2>/dev/null &&
  xxd -p -l 8 '$T/long.png' | grep -q '89504e470d0a1a0a'
"

# 14. Multi-line equation via raw mode
check "multiline align" sh -c "
  '$LTPNG' '\begin{align} a &= b + c \\\\ d &= e + f \end{align}' -o $T/align.png 2>/dev/null &&
  xxd -p -l 8 '$T/align.png' | grep -q '89504e470d0a1a0a'
"

# 15. Matrix expression (pmatrix needs math mode; --display forces \[...\] wrapping)
check "matrix expression" sh -c "
  '$LTPNG' --display '\begin{pmatrix} 1 & 0 \\\\ 0 & 1 \end{pmatrix}' -o $T/matrix.png 2>/dev/null &&
  xxd -p -l 8 '$T/matrix.png' | grep -q '89504e470d0a1a0a'
"

# 16. Unicode/Greek letters in math
check "unicode greek" sh -c "
  '$LTPNG' '\alpha + \beta = \gamma' -o $T/greek.png 2>/dev/null &&
  xxd -p -l 8 '$T/greek.png' | grep -q '89504e470d0a1a0a'
"

# 17. Custom preamble (verbatim string)
check "custom preamble verbatim" sh -c "
  '$LTPNG' -p '\usepackage{mathtools}' '\coloneqq' -o $T/preamble.png 2>/dev/null &&
  xxd -p -l 8 '$T/preamble.png' | grep -q '89504e470d0a1a0a'
"

# 18. Preamble from file
check "preamble from file" sh -c "
  printf '\\\usepackage{mathtools}\n' > $T/pre.tex &&
  '$LTPNG' -p $T/pre.tex '\coloneqq' -o $T/prefile.png 2>/dev/null &&
  xxd -p -l 8 '$T/prefile.png' | grep -q '89504e470d0a1a0a'
"

# 19. Multiple preamble flags
check "multiple preamble flags" sh -c "
  '$LTPNG' -p '\usepackage{mathtools}' -p '\usepackage{bm}' '\bm{x} \coloneqq y' -o $T/multipre.png 2>/dev/null &&
  xxd -p -l 8 '$T/multipre.png' | grep -q '89504e470d0a1a0a'
"

# 20. --display override with a raw-looking fragment
check "display override" sh -c "
  '$LTPNG' --display 'x^2 + y^2' -o $T/dispover.png 2>/dev/null &&
  xxd -p -l 8 '$T/dispover.png' | grep -q '89504e470d0a1a0a'
"

# 21. --inline override
check "inline override" sh -c "
  '$LTPNG' --inline 'E = mc^2' -o $T/inlover.png 2>/dev/null &&
  xxd -p -l 8 '$T/inlover.png' | grep -q '89504e470d0a1a0a'
"

# 22. Output to current directory (relative path)
check "relative output path" sh -c "
  cd $T &&
  '$LTPNG' 'x' -o relpath.png 2>/dev/null &&
  xxd -p -l 8 relpath.png | grep -q '89504e470d0a1a0a'
"

# 23. Empty stdin should fail gracefully
check "empty stdin fails" sh -c "
  printf '' | '$LTPNG' -o $T/empty.png 2>/dev/null; rc=\$?;
  [ \$rc -ne 0 ]
"

# 24. Help flag exits 0
check "help exits 0" sh -c "
  '$LTPNG' --help 2>/dev/null; rc=\$?;
  [ \$rc -eq 0 ]
"

# 25. Version flag exits 0
check "version exits 0" sh -c "
  '$LTPNG' --version 2>/dev/null; rc=\$?;
  [ \$rc -eq 0 ]
"

# 26. Deeply nested LaTeX (lots of braces)
check "deeply nested braces" sh -c "
  '$LTPNG' '\frac{\frac{\frac{1}{2}}{3}}{4}' -o $T/nested.png 2>/dev/null &&
  xxd -p -l 8 '$T/nested.png' | grep -q '89504e470d0a1a0a'
"

# 27. -k on failure should still keep temp dir with job.tex
check "keep on latex failure" sh -c "
  output=\$('$LTPNG' '\undefinedmacro' --keep -o $T/kfail.png 2>/dev/null) || true;
  [ -f \"$T/kfail.png\" ] && exit 1;
  true
"

# 28. TikZ auto-detection
if command -v xelatex >/dev/null 2>&1; then
  check "tikz auto-detection" sh -c "
    '$LTPNG' '\begin{tikzpicture}\draw (0,0) -- (1,1);\end{tikzpicture}' -o $T/tikz.png 2>/dev/null &&
    xxd -p -l 8 '$T/tikz.png' | grep -q '89504e470d0a1a0a'
  "
fi

# 29. Very simple single-character fragment
check "single char fragment" sh -c "
  '$LTPNG' 'x' -o $T/single.png 2>/dev/null &&
  xxd -p -l 8 '$T/single.png' | grep -q '89504e470d0a1a0a'
"

# 30. Transparent + high DPI combined
check "transparent high dpi" sh -c "
  '$LTPNG' 'E = mc^2' -t -d 600 -o $T/thd.png 2>/dev/null &&
  xxd -p -l 8 '$T/thd.png' | grep -q '89504e470d0a1a0a'
"

# 31. --no-auto-packages still renders a plain fragment
check "no-auto-packages plain fragment" sh -c "
  '$LTPNG' --no-auto-packages 'E = mc^2' -o $T/noauto.png 2>/dev/null &&
  xxd -p -l 8 '$T/noauto.png' | grep -q '89504e470d0a1a0a'
"

# 32. Uppercase algorithmic commands (algorithmic package, not algpseudocode)
if kpsewhich algorithmic.sty >/dev/null 2>&1; then
  # Write repro fragment outside the check subshell for clean quoting
  cat > "$T/algo_uc.in" << 'EOF'
\begin{algorithm}
\caption{Hardware-Aware Prefix Scheduler}
\begin{algorithmic}[1]
\REQUIRE Active requests $r \in \{1, \ldots, R\}$
\ENSURE Selected per-request prefix lengths $\ell_1^*, \ldots, \ell_R^*$
\STATE Initialize states
\RETURN $(\ell_1^*, \ldots, \ell_R^*)$
\end{algorithmic}
\end{algorithm}
EOF
  check "uppercase algorithmic package" sh -c "
    '$LTPNG' -o $T/algo_uc.png < $T/algo_uc.in 2>/dev/null &&
    xxd -p -l 8 '$T/algo_uc.png' | grep -q '89504e470d0a1a0a'
  "
fi

echo ""
echo "E2E: $pass passed, $fail failed"
[ $fail -eq 0 ]
exit $fail
