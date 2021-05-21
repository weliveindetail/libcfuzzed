#define _GNU_SOURCE

#include "libcfu.h"

#include <dlfcn.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <linux/limits.h>
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

typedef struct {
  char before[PATH_MAX];
  char after[PATH_MAX];
  size_t len_before;
  size_t len_after;
} fuzzdep_prefix_substitution_t;

static fuzzdep_prefix_substitution_t fuzzdep_cwd_fuzz;
static fuzzdep_prefix_substitution_t fuzzdep_cwd_recover;

static void reset_cwd_substitution(const uint8_t *corpus, const uint8_t *end);
static void invert_prefix_substitution(const fuzzdep_prefix_substitution_t *subst,
                                       fuzzdep_prefix_substitution_t *subst_out);

jmp_buf failed_assert_exit;

// Provide the landing pad for installing the return point in the top-level
// stack frame.
jmp_buf *fuzzdep_landing_pad() {
  return &failed_assert_exit;
}

// Provide a special assertion handler for the test suite. From here we jump
// back to the landing pad installed in the top-level stack frame.
__attribute__((noreturn))
void handle_assertion_failure() {
  longjmp(failed_assert_exit, EXIT_FAILURE);
}

__attribute__((constructor))
static void fuzzdep_static_init() {
  fprintf(stderr,
          "About to intercept stdlib functions to install fuzzer logic. "
          "You can attach a debugger now. Press any key to continue.\n");
  getchar();

  o_realpath = dlsym(RTLD_NEXT, "realpath");
  o_xstat = dlsym(RTLD_NEXT, "__xstat");
  o_xstat64 = dlsym(RTLD_NEXT, "__xstat64");
  o_fopen = dlsym(RTLD_NEXT, "fopen");
  o_opendir = dlsym(RTLD_NEXT, "opendir");

  if ((*o_realpath)(".", fuzzdep_cwd_fuzz.before) == 0) {
    fprintf(stderr, "Failed precondition: cannot obtain current working dir\n");
    exit(1);
  }

  fuzzdep_cwd_fuzz.len_before =
      strlen(fuzzdep_cwd_fuzz.before);
}

void fuzzdep_reset_corpus(const uint8_t *data, size_t size) {
  reset_cwd_substitution(data, data + size);
}

static void reset_cwd_substitution(const uint8_t *corpus, const uint8_t *end) {
  char *fuzzed_path = fuzzdep_cwd_fuzz.after;
  char *pos = fuzzed_path;
  *(pos++) = '/';

  char next;
  do {
    bool terminal = (corpus >= end || pos - fuzzed_path >= PATH_MAX - 1);
    next = terminal ? '\0' : *(corpus++);
    if (next != '.')
      *(pos++) = next;
  } while (next != '\0');

  fuzzdep_cwd_fuzz.len_after = pos - fuzzed_path - 1;
  invert_prefix_substitution(&fuzzdep_cwd_fuzz, &fuzzdep_cwd_recover);
}

static void invert_prefix_substitution(const fuzzdep_prefix_substitution_t *subst,
                                       fuzzdep_prefix_substitution_t *subst_out) {
  if (subst && subst_out) {
    memcpy(subst_out->after, subst->before, subst->len_before);
    memcpy(subst_out->before, subst->after, subst->len_after);
    subst_out->len_after = subst->len_before;
    subst_out->len_before = subst->len_after;
  }
}

static void substitute_cwd(const fuzzdep_prefix_substitution_t *subst,
                           const char *path, char *path_out) {
  if (strncmp(path, subst->before, subst->len_before) == 0) {
    memcpy(path_out, subst->after, subst->len_after);
    const char *remaining_path = path + subst->len_before;
    memcpy(path_out + subst->len_after, remaining_path, strlen(remaining_path) + 1);
  } else {
    memcpy(path_out, path, strlen(path) + 1);
  }
}

// Override POSIX realpath(3) from stdlib.h in order to fuzz the current
// working directory.
char *realpath(const char *path, char *resolved_path) {
  // Incoming paths may refer to the fuzzed cwd.
  char actual_path[PATH_MAX];
  substitute_cwd(&fuzzdep_cwd_recover, path, actual_path);

  // Canonicalize the actual path that represents a valid disk location.
  char actual_resolved_path[PATH_MAX];
  if ((*o_realpath)(actual_path, actual_resolved_path) == 0)
    return 0;

  // Apply CWD fuzzing on the return value, so any client path calculations
  // will use the fuzzed path.
  substitute_cwd(&fuzzdep_cwd_fuzz, actual_resolved_path, resolved_path);
  return resolved_path;
}

int stat(const char *path, struct stat *buffer) {
  char actual_path[PATH_MAX];
  substitute_cwd(&fuzzdep_cwd_recover, path, actual_path);
  return (*o_xstat)(_STAT_VER, actual_path, buffer);
}

int stat64(const char *path, struct stat64 *buffer) {
  char actual_path[PATH_MAX];
  substitute_cwd(&fuzzdep_cwd_recover, path, actual_path);
  return (*o_xstat64)(_STAT_VER, actual_path, buffer);
}

FILE *fopen(const char *path, const char *mode) {
  char actual_path[PATH_MAX];
  substitute_cwd(&fuzzdep_cwd_recover, path, actual_path);
  return (*o_fopen)(actual_path, mode);
}

DIR *opendir(const char *path) {
  char actual_path[PATH_MAX];
  substitute_cwd(&fuzzdep_cwd_recover, path, actual_path);
  return (*o_opendir)(actual_path);
}
