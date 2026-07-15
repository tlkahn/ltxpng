/* ltxpng: subprocess execution, temp dir management, PATH lookup */

#ifndef RUN_H
#define RUN_H

#include <sys/types.h>
#include "util.h"

/* Execute a command, capturing stdout+stderr to a file.
   Returns the exit status (waitpid exit code, or 127 if exec failed).
   If capfile is non-NULL, child output is written there. */
int run_cmd(const char *argv[], const char *capfile);

/* Create a temp directory. Returns allocated string path, or NULL on failure. */
char *tmpdir_create(void);

/* Remove a temp directory and all contents. */
void tmpdir_remove(const char *path);

/* Find an executable on PATH. Returns allocated full path, or NULL if not found. */
char *find_on_path(const char *name);

/* Extract error context from a .log file (lines starting with ! plus context).
   Writes result into buf (already initialised sb). */
void extract_log_errors(const char *logpath, sb *buf);

#endif /* RUN_H */
