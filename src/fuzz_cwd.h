#ifndef LIBCFUZZED_SRC_FUZZ_CWD_H
#define LIBCFUZZED_SRC_FUZZ_CWD_H

#include <stddef.h>
#include <stdint.h>

// Static initialization for the cwd fuzzer. It's called from
// libcfuzzed_static_init() if cwd fuzzing is enabled.
void libcfuzzed_fuzz_cwd_init();

// Initialize the fuzzed cwd from the given libFuzzer data pattern. This is
// called once before each invocation of the client test suite.
size_t libcfuzzed_fuzz_cwd_reset(const uint8_t *data, size_t size);

#endif // LIBCFUZZED_SRC_FUZZ_CWD_H
