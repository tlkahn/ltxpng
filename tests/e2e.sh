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

echo ""
echo "E2E: $pass passed, $fail failed"
[ $fail -eq 0 ]
exit $fail
