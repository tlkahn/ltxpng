/* ltxpng: subprocess execution, temp dir management, PATH lookup */

#include "run.h"
#include "util.h"
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

int run_cmd(const char *argv[], const char *capfile) {
  pid_t pid = fork();
  if (pid == 0) {
    /* Child */
    if (capfile) {
      int fd = open(capfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
    }
    execvp(argv[0], (char **)argv);
    /* If we get here, exec failed */
    _exit(127);
  } else if (pid < 0) {
    return 127;
  }

  /* Parent */
  int status;
  waitpid(pid, &status, 0);
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  return 127;
}

char *tmpdir_create(void) {
  const char *tmp = getenv("TMPDIR");
  if (!tmp) tmp = "/tmp";
  size_t len = strlen(tmp) + 20;
  char *path = (char *)malloc(len);
  if (!path) return NULL;
  snprintf(path, len, "%s/ltxpng.XXXXXX", tmp);
  if (mkdtemp(path) == NULL) {
    free(path);
    return NULL;
  }
  return path;
}

static void remove_recursive(const char *path) {
  DIR *d = opendir(path);
  if (!d) return;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
      continue;
    char full[4096];
    snprintf(full, sizeof(full), "%s/%s", path, e->d_name);
    struct stat st;
    if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
      remove_recursive(full);
    else
      unlink(full);
  }
  closedir(d);
  rmdir(path);
}

void tmpdir_remove(const char *path) {
  if (path) remove_recursive(path);
}

char *find_on_path(const char *name) {
  const char *path_env = getenv("PATH");
  if (!path_env) return NULL;

  /* Duplicate so we can strtok */
  char *path_copy = strdup(path_env);
  if (!path_copy) return NULL;

  char *dir;
  char *saveptr;
  char *result = NULL;

  dir = strtok_r(path_copy, ":", &saveptr);
  while (dir) {
    size_t dirlen = strlen(dir);
    size_t namelen = strlen(name);
    char *full = (char *)malloc(dirlen + 1 + namelen + 1);
    if (!full) break;
    snprintf(full, dirlen + namelen + 2, "%s/%s", dir, name);
    struct stat st;
    if (stat(full, &st) == 0 && (st.st_mode & S_IXUSR)) {
      result = full;
      break;
    }
    free(full);
    dir = strtok_r(NULL, ":", &saveptr);
  }

  free(path_copy);
  return result;
}

void extract_log_errors(const char *logpath, sb *buf) {
  FILE *f = fopen(logpath, "r");
  if (!f) {
    sb_append(buf, "(could not open log)");
    return;
  }

  /* Read all lines */
  char **lines = NULL;
  int nlines = 0;
  char line[4096];

  while (fgets(line, sizeof(line), f)) {
    lines = (char **)realloc(lines, (nlines + 1) * sizeof(char *));
    lines[nlines] = strdup(line);
    nlines++;
  }
  fclose(f);

  /* Find lines starting with '!' */
  int found = 0;
  for (int i = 0; i < nlines; i++) {
    if (lines[i][0] == '!') {
      found = 1;
      /* Print this line and next few */
      for (int j = i; j < nlines && j < i + 10; j++) {
        sb_append(buf, lines[j]);
      }
      sb_append(buf, "\n");
    }
  }

  if (!found) {
    /* Print tail (last 30 lines) */
    int start = nlines - 30;
    if (start < 0) start = 0;
    for (int i = start; i < nlines; i++) {
      sb_append(buf, lines[i]);
    }
  }

  for (int i = 0; i < nlines; i++) free(lines[i]);
  free(lines);
}
