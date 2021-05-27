#define _GNU_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <linux/limits.h>
#include <sys/types.h>

#include <libcfuzzed.h>

#define ASSERT(expr)                                                \
  do {                                                              \
    if (!(expr)) {                                                  \
      if (did_trigger_all_assertion_failures(__COUNTER__))          \
        exit(EXIT_SUCCESS);                                         \
      libcfuzzed_loop_repeat();                                     \
    }                                                               \
  } while (0)

bool did_trigger_all_assertion_failures(int assert_id);

/* --- Put all test code here ----------------------------------------------- */

static char cons2_buf[PATH_MAX];
char *cons2(const char *p1, const char *p2) {
  size_t l1 = strlen(p1);
  memcpy(cons2_buf, p1, l1);
  if (p1[l1 - 1] != '/')
    cons2_buf[l1++] = '/';
  memcpy(cons2_buf + l1, p2, strlen(p2) + 1);
  return cons2_buf;
}

char fchars_buf[80];
unsigned fchars(const char *file_name) {
  FILE *file = fopen(file_name, "r");
  return file ? fread(fchars_buf, 1, sizeof(fchars_buf), file) : 0;
}

unsigned count_subdirs(const char *dir) {
  unsigned count = 0;
  DIR *d = opendir(dir);
  if (d) {
    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
      if (entry->d_type == DT_DIR)
        if (strcmp(entry->d_name, ".") != 0)
          if (strcmp(entry->d_name, "..") != 0)
            ++count;
    }
    closedir(d);
  }
  return count;
}

void check_cwd() {
  // Get the fake working directory.
  char cwd[PATH_MAX];
  realpath(".", cwd);

  // Make sure relative paths access the expected files on disk.
  unsigned chars = fchars("libcfuzzed_build_test");
  assert(chars == fchars("./libcfuzzed_build_test"));
  assert(chars == fchars(cons2(cwd, "libcfuzzed_build_test")));
  assert(chars == fchars("../libcfuzzed_build_root"));
  assert(chars == fchars(cons2(cwd, "../libcfuzzed_build_root")));
  assert(chars == fchars("nested/libcfuzzed_build_nested"));
  assert(chars == fchars("./nested/libcfuzzed_build_nested"));
  assert(chars == fchars(cons2(cwd, "nested/libcfuzzed_build_nested")));

  // We can access the direct parent directory.
  char parent[PATH_MAX];
  realpath(cons2(cwd, ".."), parent);
  assert(count_subdirs(parent) > 1);

  // Note that any fake CWD (including root) may behave like a symlink, i.e. if
  // `realpath .` is the root then `realpath ..` would usually be root as well.
  // However, we know the unit test is running in a nested folder and thus the
  // parent directory is not root in our case.
  if (strcmp(cwd, "/") == 0) {
    assert(strcmp(parent, "/") != 0);
  }

  // Make sure alternative library functions provide a consistent view.
  char getcwd_result[PATH_MAX];
  assert(getcwd(getcwd_result, PATH_MAX) != NULL);
  assert(strcmp(cwd, getcwd_result) == 0);

  char getwd_result[PATH_MAX];
  assert(getwd(getwd_result) != NULL);
  assert(strcmp(cwd, getwd_result) == 0);

  char *malloced_result = get_current_dir_name();
  assert(strcmp(cwd, malloced_result) == 0);
  free(malloced_result);

  // Test the limitations that the fuzzer should find at some point. Under the
  // hood this is using "expected" failures.
  ASSERT(strchr(cwd, '#') == NULL);
  ASSERT(strchr(cwd, '?') == NULL);
  ASSERT(strlen(cwd) < 80);
}

/* --- Assertion tracking infrastructure ------------------------------------ */

static const size_t num_assertions_total = __COUNTER__;
static unsigned assertions_failed[num_assertions_total];
static unsigned attempts = 0;

bool did_trigger_all_assertion_failures(int assert_id) {
  // Track first attempt where assertion failed.
  if (assertions_failed[assert_id] == 0)
    assertions_failed[assert_id] = attempts;

  // Continue running until all assertions failed at least once.
  for (int i = 0; i < num_assertions_total; ++i)
    if (assertions_failed[i] == 0)
      return false;

  // Print a summary.
  fprintf(stderr, "Found all assertion failures in %d attempts.\n\n", attempts);
  fprintf(stderr, "ID  Attempt\n");
  fprintf(stderr, "0   %-6d\n", assertions_failed[0]);
  for (int i = 1; i < num_assertions_total; ++i)
    fprintf(stderr, "%-3d %d\n", i, assertions_failed[i]);
  fprintf(stderr, "\n");

  return true;
}

/* --- Test driver and options ---------------------------------------------- */

int unit_test_main() {
  ++attempts;
  check_cwd();
  return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
  // Run the unit test in a loop with fuzzy libc behavior.
  if (libcfuzzed_is_loaded())
    return libcfuzzed_loop(argc, argv, &unit_test_main);

  // Run the unit test as usual.
  return unit_test_main();
}
