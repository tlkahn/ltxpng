/* ltxpng: utility implementations */

#include "util.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define SB_INIT_CAP 64

void sb_init(sb *b) {
  b->buf = (char *)malloc(SB_INIT_CAP);
  b->buf[0] = '\0';
  b->len = 0;
  b->cap = SB_INIT_CAP;
}

void sb_append(sb *b, const char *s) {
  size_t slen = strlen(s);
  size_t need = b->len + slen + 1;
  if (need > b->cap) {
    while (b->cap < need) b->cap *= 2;
    b->buf = (char *)realloc(b->buf, b->cap);
  }
  memcpy(b->buf + b->len, s, slen);
  b->len += slen;
  b->buf[b->len] = '\0';
}

void sb_appendf(sb *b, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  size_t need = b->len + (size_t)n + 1;
  if (need > b->cap) {
    while (b->cap < need) b->cap *= 2;
    b->buf = (char *)realloc(b->buf, b->cap);
  }
  va_start(ap, fmt);
  vsnprintf(b->buf + b->len, (size_t)n + 1, fmt, ap);
  va_end(ap);
  b->len += (size_t)n;
}

char *sb_detach(sb *b) {
  char *ret = b->buf;
  b->buf = NULL;
  b->len = 0;
  b->cap = 0;
  return ret;
}

void sb_free(sb *b) {
  free(b->buf);
  b->buf = NULL;
  b->len = 0;
  b->cap = 0;
}

char *read_all(FILE *f, size_t *out_len) {
  sb b;
  sb_init(&b);
  char buf[4096];
  size_t nread;
  while ((nread = fread(buf, 1, sizeof(buf), f)) > 0) {
    size_t old = b.len;
    size_t need = b.len + nread + 1;
    if (need > b.cap) {
      while (b.cap < need) b.cap *= 2;
      b.buf = (char *)realloc(b.buf, b.cap);
    }
    memcpy(b.buf + old, buf, nread);
    b.len += nread;
    b.buf[b.len] = '\0';
  }
  if (out_len) *out_len = b.len;
  return sb_detach(&b);
}

char *chomp(char *s) {
  if (!s) return s;
  size_t n = strlen(s);
  while (n > 0 && (unsigned char)s[n-1] <= ' ') s[--n] = '\0';
  return s;
}
