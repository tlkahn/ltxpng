# ltxpng -- LaTeX fragment to PNG CLI

Convert a LaTeX fragment (equation, algorithmic code, tikz diagram, etc.) into a
tightly-cropped PNG image using `xelatex` and Ghostscript.

## Requirements

- TeX Live (or equivalent) with `xelatex`, `pdfcrop`, and the `standalone`,
  `varwidth` packages
- Ghostscript (`gs`)
- C11 compiler (Clang, GCC)

## Install

```
make
sudo make install   # copies to /usr/local/bin
```

## Usage

```
ltxpng [options] [FRAGMENT]
```

**Arguments:**
- `FRAGMENT` -- LaTeX source as a single argument. If absent or `-`, read stdin.

**Options:**
- `-o, --output FILE` -- Output PNG path (default `out.png`)
- `-d, --dpi N` -- Render resolution (default 300)
- `-t, --transparent` -- Transparent background (default opaque white)
- `-p, --preamble ARG` -- Extra preamble (repeatable; file path or verbatim)
- `-m, --margin DIM` -- Whitespace border, a TeX dimension (default `2pt`)
- `--inline` / `--display` / `--raw` -- Force wrapping mode
- `--no-auto-packages` -- Skip automatic package detection (`-p` still honored)
- `-k, --keep` -- Keep temp build dir and print its path
- `-v, --verbose` -- Stream xelatex/gs output
- `-h, --help`, `-V, --version`

### Package auto-detection

When the input is not a full document, ltxpng scans the fragment and loads
packages automatically (unless `--no-auto-packages` is set):

| Trigger | Package(s) |
|---------|------------|
| `\begin{tikzpicture}` | `tikz` |
| `\begin{algorithm}` | `algorithm`, `float`, plus a float-to-inline shim |
| Uppercase algorithmic commands (`\STATE`, `\REQUIRE`, `\FOR`, ...) | `algorithmic` |
| CamelCase algorithmic commands (`\State`) or `\begin{algorithmic}` alone | `algpseudocode` |

`algorithmic` and `algpseudocode` are mutually exclusive (loading both conflicts).
If both uppercase and CamelCase styles appear in one fragment, uppercase wins and
a warning is printed to stderr.

The `\begin{algorithm}` float cannot be placed inside `standalone`/`varwidth`, so
detection also injects `\floatplacement{algorithm}{H}` and redefines the
environment as an in-place minipage so the fragment still renders.

### Examples

```sh
# Basic equation
ltxpng 'E = mc^2' -o equation.png

# Algorithmic code (auto-detects algpseudocode package)
ltxpng '\begin{algorithmic} \State x = 1 \end{algorithmic}' -o algo.png

# Old-style uppercase algorithmic (auto-detects algorithmic package)
ltxpng '\begin{algorithmic} \STATE x = 1 \end{algorithmic}' -o algo.png

# TikZ diagram (auto-detects tikz package)
ltxpng '\begin{tikzpicture} \draw (0,0) circle (1); \end{tikzpicture}' -o circle.png

# Full LaTeX document (compiled as-is, then cropped)
ltxpng '\documentclass{article}\begin{document}Hello\end{document}' -o doc.png

# Transparent background at 600 DPI
ltxpng 'E = mc^2' -t -d 600 -o hires.png

# Read from stdin
printf '\begin{equation} x^2 \end{equation}' | ltxpng -o eq.png

# Extra preamble packages
ltxpng 'x^2 + y^2 = z^2' -p '\usepackage{mathtools}' -o eq.png
```

## Exit codes

| Code | Meaning                        |
|------|--------------------------------|
| 0    | Success                        |
| 1    | Usage or I/O error             |
| 2    | LaTeX compilation failure      |
| 3    | Ghostscript / pdfcrop failure  |
| 127  | Required tool not found        |

## Development

```
make test      # run all unit tests + end-to-end tests
make debug     # build with AddressSanitizer
make clean     # remove build artifacts
```

Tests use a minimal handrolled harness (no test framework dependency).
All tests must pass before changes are accepted.

## License

MIT
