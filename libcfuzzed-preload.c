#define _GNU_SOURCE

#include "libcfuzzed-preload.h"
#include "src/fuzz_cwd.h"

#include <stdio.h>
#include <stdlib.h>

jmp_buf failed_assert_exit;

// Provide the landing pad for installing the return point in the top-level
// stack frame.
jmp_buf *libcfuzzed_landing_pad() {
  return &failed_assert_exit;
}

// Provide a special assertion handler for the test suite. From here we jump
// back to the landing pad installed in the top-level stack frame.
__attribute__((noreturn))
void handle_assertion_failure() {
  longjmp(failed_assert_exit, EXIT_FAILURE);
}

__attribute__((constructor))
static void libcfuzzed_static_init() {
  fprintf(stderr,
          "About to intercept stdlib functions to install fuzzer logic. "
          "You can attach a debugger now. Press any key to continue.\n");
  getchar();

  libcfuzzed_fuzz_cwd_init();
}

void libcfuzzed_reset(const uint8_t *data, size_t size) {
  size_t consumed = libcfuzzed_fuzz_cwd_reset(data, size);
  data += consumed;
  size -= consumed;
}
