#define _GNU_SOURCE

#include "prefix_subst.h"

#include <assert.h>
#include <string.h>
#include <linux/limits.h>

typedef struct {
  char before[PATH_MAX];
  char after[PATH_MAX];
  size_t len_before;
  size_t len_after;
} prefix_substitution_t;

static prefix_substitution_t cwd_fuzz;
static prefix_substitution_t cwd_recover;

static void substitute_cwd(const prefix_substitution_t *subst,
                           const char *path, char *path_out) {
  if (strncmp(path, subst->before, subst->len_before) == 0) {
    memcpy(path_out, subst->after, subst->len_after);
    const char *remaining_path = path + subst->len_before;
    memcpy(path_out + subst->len_after, remaining_path, strlen(remaining_path) + 1);
  } else {
    memcpy(path_out, path, strlen(path) + 1);
  }
}

void fuzz_cwd(const char *path, char *path_out) {
  if (cwd_fuzz.len_after == 0)
    return; // Not initialized
  substitute_cwd(&cwd_fuzz, path, path_out);
}

void unfuzz_cwd(const char *path, char *path_out) {
  if (cwd_recover.len_before == 0)
    return; // Not initialized
  substitute_cwd(&cwd_recover, path, path_out);
}

static void invert_prefix_substitution(const prefix_substitution_t *subst,
                                       prefix_substitution_t *subst_out) {
  assert(strlen(subst->after) > 0 && "Won't be able to recover empty prefix");
  memcpy(subst_out->after, subst->before, subst->len_before);
  memcpy(subst_out->before, subst->after, subst->len_after);
  subst_out->len_after = subst->len_before;
  subst_out->len_before = subst->len_after;
}

void set_fuzzed_cwd(const char *cwd) {
  cwd_fuzz.len_after = strlen(cwd);
  memcpy(cwd_fuzz.after, cwd, cwd_fuzz.len_after + 1);
  invert_prefix_substitution(&cwd_fuzz, &cwd_recover);
}

void set_actual_cwd(const char *cwd) {
  // Invalidate subsitution state; set_fuzzed_cwd() will reinstantiate it.
  *cwd_fuzz.after = '\0';
  *cwd_recover.after = '\0';
  *cwd_recover.before = '\0';

  cwd_fuzz.len_before = strlen(cwd);
  memcpy(cwd_fuzz.before, cwd, cwd_fuzz.len_before + 1);
}
