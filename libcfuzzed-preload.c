#define _GNU_SOURCE

#include "libcfuzzed-preload.h"
#include "src/fuzz_cwd.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  bool disable_cwd;
} libcfuzzed_config_t;

libcfuzzed_config_t libcfuzzed_config;

__attribute__((constructor))
static void libcfuzzed_static_init() {
  if (getenv("LIBCFUZZED_WAIT_ATTACH")) {
    fprintf(stderr,
            "About to intercept stdlib functions to install fuzzer logic. "
            "You can attach a debugger now. Press any key to continue.\n");
    getchar();
  }

  libcfuzzed_config.disable_cwd = (getenv("LIBCFUZZED_NO_CWD") != NULL);

  if (!libcfuzzed_config.disable_cwd) {
    libcfuzzed_fuzz_cwd_init();
  }
}

void libcfuzzed_reset(const uint8_t *data, size_t size) {
  if (!libcfuzzed_config.disable_cwd) {
    size_t consumed = libcfuzzed_fuzz_cwd_reset(data, size);
    data += consumed;
    size -= consumed;
  }
}
