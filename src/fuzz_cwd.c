#define _GNU_SOURCE

#include "fuzz_cwd.h"
#include "prefix_subst.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

struct stat;
struct stat64;

static char *(*o_realpath)(const char *, char *);
static char *(*o_realpath)(const char *, char *);
static int (*o_xstat)(int, const char *, struct stat *);
static int (*o_xstat64)(int, const char *, struct stat64 *);
static FILE *(*o_fopen)(const char*, const char*);
static DIR *(*o_opendir)(const char *);

void libcfud_fuzz_cwd_init() {
  o_realpath = dlsym(RTLD_NEXT, "realpath");
  o_xstat = dlsym(RTLD_NEXT, "__xstat");
  o_xstat64 = dlsym(RTLD_NEXT, "__xstat64");
  o_fopen = dlsym(RTLD_NEXT, "fopen");
  o_opendir = dlsym(RTLD_NEXT, "opendir");

  char actual_cwd[PATH_MAX];
  if ((*o_realpath)(".", actual_cwd) == 0) {
    fprintf(stderr, "Failed precondition: cannot obtain current working dir\n");
    exit(1);
  }

  set_actual_cwd(actual_cwd);
}

size_t libcfud_fuzz_cwd_reset(const uint8_t *data, size_t size) {
  char cwd_subst[PATH_MAX];
  char *write_pos = cwd_subst;
  const char *write_end = cwd_subst + PATH_MAX - 1;
  const uint8_t *read_pos = data;
  const uint8_t *read_end = data + size - 1;

  *(write_pos++) = '/';
  char ch;
  do {
    bool terminal = (read_pos > read_end || write_pos >= write_end);
    ch = terminal ? '\0' : *(read_pos++);
    if (ch != '.')
      *(write_pos++) = ch;
  } while (ch != '\0');

  set_fuzzed_cwd(cwd_subst);
  return read_pos - data;
}

// Fuzz the current working directory by intercepting POSIX realpath(3).
//
// 
// The
// resolved path is guaranteed to exist on disk, so we have to intercept other
// i/o functions as well and recover the actual paths there. While accessing the
// actual files on disk while forcing the client logic to deal with fuzzed
// .
char *realpath(const char *path, char *resolved_path) {
  // Incoming paths may use the fuzzed cwd. Try to recover first.
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);

  // The resolved path represents a valid disk location.
  char actual_resolved_path[PATH_MAX];
  if ((*o_realpath)(actual_path, actual_resolved_path) == 0)
    return 0;

  // Substitute the cwd in the resolved path.
  fuzz_cwd(actual_resolved_path, resolved_path);
  return resolved_path;
}

int stat(const char *path, struct stat *buffer) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*o_xstat)(_STAT_VER, actual_path, buffer);
}

int stat64(const char *path, struct stat64 *buffer) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*o_xstat64)(_STAT_VER, actual_path, buffer);
}

FILE *fopen(const char *path, const char *mode) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*o_fopen)(actual_path, mode);
}

DIR *opendir(const char *path) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*o_opendir)(actual_path);
}
