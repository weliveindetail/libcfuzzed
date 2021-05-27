#define _GNU_SOURCE

#include "libcfuzzed-preload.h"
#include "src/fuzz_cwd.h"

#include <stdio.h>
#include <stdlib.h>

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
