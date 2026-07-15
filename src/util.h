/* ltxpng: utility APIs (string buffer, read-all) */

#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>

/* Growable string buffer */
typedef struct {
  char *buf;
  size_t len;   /* length without NUL */
  size_t cap;   /* allocated capacity (includes NUL) */
} sb;

void sb_init(sb *b);
void sb_append(sb *b, const char *s);
void sb_appendf(sb *b, const char *fmt, ...);
char *sb_detach(sb *b);
void sb_free(sb *b);

/* Read all bytes from a FILE* into a NUL-terminated string. */
char *read_all(FILE *f, size_t *out_len);

/* Strip trailing whitespace from a string in-place, return s. */
char *chomp(char *s);

#endif /* UTIL_H */
