#define _GNU_SOURCE

#include "fuzz_cwd.h"
#include "libcfuzzed.h"

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
  char before[PATH_MAX];
  char after[PATH_MAX];
  size_t len_before;
  size_t len_after;
} prefix_substitution_t;

static enum { DISJUNCT, OVERLAPPED, OVERLAPPING } cwd_overlap;
static char *cwd_parent_dirs[512];
static prefix_substitution_t cwd_fuzz;
static prefix_substitution_t cwd_recover;

struct stat;
struct stat64;

static char *(*real_getcwd)(char *, size_t);
static char *(*real_getwd)(char *);
static char *(*real_get_current_dir_name)();
static char *(*real_realpath)(const char *, char *);
static int (*real_xstat)(int, const char *, struct stat *);
static int (*real_xstat64)(int, const char *, struct stat64 *);
static FILE *(*real_fopen)(const char*, const char*);
static DIR *(*real_opendir)(const char *);

static void invert_prefix_substitution(const prefix_substitution_t *subst,
                                       prefix_substitution_t *subst_out) {
  assert(strlen(subst->after) > 0 && "Won't be able to recover empty prefix");
  memcpy(subst_out->after, subst->before, subst->len_before);
  memcpy(subst_out->before, subst->after, subst->len_after);
  subst_out->len_after = subst->len_before;
  subst_out->len_before = subst->len_after;
}

static bool is_subpath(const char *path) {
  // TODO: Check the directory part of `path` against the known real parents,
  // so that we can access files in there correctly.
  size_t path_len = strlen(path);
  for (size_t i = 0; cwd_parent_dirs[i]; ++i)
    if (strncmp(cwd_parent_dirs[i], path, path_len) == 0)
      return true;

  return false;
}

void set_fuzzed_cwd(const char *cwd) {
  cwd_fuzz.len_after = strlen(cwd);
  memcpy(cwd_fuzz.after, cwd, cwd_fuzz.len_after + 1);
  invert_prefix_substitution(&cwd_fuzz, &cwd_recover);

  if (is_subpath(cwd)) {
    // Fuzzed CWD is subpath of the actual CWD. Most common case is root folder.
    cwd_overlap = OVERLAPPING;
  } else if (strncmp(cwd_fuzz.before, cwd_fuzz.after, cwd_fuzz.len_after) == 0) {
    // Actual CWD is subpath of the fuzzed CWD. This is really rare and we
    // intentionally avoid it, because it doesn't add any value.
    cwd_overlap = OVERLAPPED;
    libcfuzzed_loop_repeat();
  } else {
    // No overlap between fuzzed and actual CWD.
    cwd_overlap = DISJUNCT;
  }
}

void set_actual_cwd(const char *cwd, const char *(*get_parent)(const char *)) {
  // Invalidate subsitution state; set_fuzzed_cwd() will reinstantiate it.
  *cwd_fuzz.after = '\0';
  *cwd_recover.after = '\0';
  *cwd_recover.before = '\0';

  cwd_fuzz.len_before = strlen(cwd);
  memcpy(cwd_fuzz.before, cwd, cwd_fuzz.len_before + 1);

  size_t i = 0;
  const char *p = get_parent(cwd);
  for (;;) {
    size_t p_len = strlen(p);
    cwd_parent_dirs[i] = (char *)malloc(p_len + 1);
    memcpy(cwd_parent_dirs[i], p, p_len + 1);
    if (strcmp(p, "/") == 0)
      break;
    p = get_parent(cwd_parent_dirs[i]);
    ++i;
  }

  cwd_parent_dirs[++i] = NULL;
}

static const char *get_parent_dir(const char *path) {
  static char buffer[PATH_MAX];
  static const char *dotdot = "/..";
  static size_t dotdot_len = 3;

  char tmp[PATH_MAX];
  size_t path_len = strlen(path);
  memcpy(tmp, path, path_len);
  memcpy(tmp + path_len, dotdot, dotdot_len + 1);
  if ((*real_realpath)(tmp, buffer) == NULL) 
    return NULL;

  return buffer;
}

void libcfuzzed_fuzz_cwd_init() {
  real_getcwd = dlsym(RTLD_NEXT, "getcwd");
  real_getwd = dlsym(RTLD_NEXT, "getwd");
  real_get_current_dir_name = dlsym(RTLD_NEXT, "get_current_dir_name");
  real_realpath = dlsym(RTLD_NEXT, "realpath");
  real_xstat = dlsym(RTLD_NEXT, "__xstat");
  real_xstat64 = dlsym(RTLD_NEXT, "__xstat64");
  real_fopen = dlsym(RTLD_NEXT, "fopen");
  real_opendir = dlsym(RTLD_NEXT, "opendir");

  char actual_cwd[PATH_MAX];
  if ((*real_realpath)(".", actual_cwd) == 0) {
    fprintf(stderr, "Failed precondition: cannot obtain current working dir\n");
    exit(1);
  }

  set_actual_cwd(actual_cwd, &get_parent_dir);
}

size_t libcfuzzed_fuzz_cwd_reset(const uint8_t *data, size_t size) {
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
    if (ch == '.') {
      const char extra_long_seg[] =
          "this_is_an_intentionally_long_name_segment/"
          "sreal_we_reach_edge_cases_with_long_names/as_well";
      memcpy(write_pos, extra_long_seg, sizeof(extra_long_seg));
      write_pos += sizeof(extra_long_seg);
    } else {
      *(write_pos++) = ch;
    }
  } while (ch != '\0');

  set_fuzzed_cwd(cwd_subst);
  return read_pos - data;
}

static bool substitute_cwd(const prefix_substitution_t *subst,
                           const char *path, char *path_out) {
  if (strncmp(path, subst->before, subst->len_before) == 0) {
    memcpy(path_out, subst->after, subst->len_after);
    const char *remaining_path = path + subst->len_before;
    size_t remaining_len = strlen(remaining_path);
    if (remaining_len == 0) {
      path_out[subst->len_after] = '\0';
    } else {
      size_t offset_seperator = 0;
      if (path_out[subst->len_after - 1] != '/') {
        path_out[subst->len_after] = '/';
        offset_seperator = 1;
      }

      memcpy(path_out + subst->len_after + offset_seperator, remaining_path,
             remaining_len + 1);
    }
    return true;
  }
  return false;
}

static void fuzz_cwd(const char *path, char *path_out) {
  if (cwd_fuzz.len_after > 0)
    if (substitute_cwd(&cwd_fuzz, path, path_out))
      return;

  memcpy(path_out, path, strlen(path) + 1);
}

static bool is_absolute(const char *path) {
  return path[0] == '/';
}

static void unfuzz_cwd(const char *path, char *path_out) {
  if (cwd_recover.len_before > 0)
    if (!is_absolute(path) || cwd_overlap != OVERLAPPING || !is_subpath(path))
      if (substitute_cwd(&cwd_recover, path, path_out))
        return;

  memcpy(path_out, path, strlen(path) + 1);
}

char *getcwd(char *buf, size_t size) {
  char actual_path[PATH_MAX];
  if ((*real_getcwd)(actual_path, PATH_MAX) == NULL)
    return NULL;

  // Substitute the cwd in the resolved path.
  char fuzzed_path[PATH_MAX];
  fuzz_cwd(actual_path, fuzzed_path);

  // Mimic getcwd(3) behavior.
  size_t fuzzed_path_len = strlen(fuzzed_path);
  if (fuzzed_path_len >= size) {
    errno = ERANGE;
    return NULL;
  }

  memcpy(buf, fuzzed_path, fuzzed_path_len + 1);
  return buf;
}

// getwd(3) is deprecated
char *getwd(char *buf) {
  char actual_path[PATH_MAX];
  if ((*real_getwd)(actual_path) == NULL)
    return NULL;

  // Substitute the cwd in the resolved path.
  char fuzzed_path[PATH_MAX];
  fuzz_cwd(actual_path, fuzzed_path);

  memcpy(buf, fuzzed_path, strlen(fuzzed_path) + 1);
  return buf;
}

char *get_current_dir_name() {
  char *actual_path = (*real_get_current_dir_name)();

  // Substitute the cwd in the resolved path.
  char fuzzed_path[PATH_MAX];
  fuzz_cwd(actual_path, fuzzed_path);
  free(actual_path);

  size_t fuzzed_path_len = strlen(fuzzed_path);
  char *result = (char *)malloc(fuzzed_path_len + 1);
  memcpy(result, fuzzed_path, fuzzed_path_len + 1);
  return result;
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
  if ((*real_realpath)(actual_path, actual_resolved_path) == 0)
    return 0;

  // Substitute the cwd in the resolved path.
  fuzz_cwd(actual_resolved_path, resolved_path);
  return resolved_path;
}

int stat(const char *path, struct stat *buffer) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*real_xstat)(_STAT_VER, actual_path, buffer);
}

int stat64(const char *path, struct stat64 *buffer) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*real_xstat64)(_STAT_VER, actual_path, buffer);
}

FILE *fopen(const char *path, const char *mode) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*real_fopen)(actual_path, mode);
}

DIR *opendir(const char *path) {
  char actual_path[PATH_MAX];
  unfuzz_cwd(path, actual_path);
  return (*real_opendir)(actual_path);
}
