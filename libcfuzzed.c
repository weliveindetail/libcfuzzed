#define _GNU_SOURCE

#include "libcfuzzed.h"
#include "libcfuzzed-preload.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// Landing pad for "expected" assertion failures. The driver uses it to escape
// from the current run of the test suite and request to re-run with a different
// data pattern.
static jmp_buf loop_repeat_landing_pad;

// Client function that runs the actual unit tests.
static unit_test_main_t unit_test_main;

// Function that runs inside the libFuzzer main loop.
int libfuzzer_callback(const uint8_t *data, size_t size) {
  // Initialize LD_PRELOAD fuzzer.
  libcfuzzed_reset(data, size);

  // Run the test suite as usual.
  if (setjmp(loop_repeat_landing_pad) == EXIT_SUCCESS)
    return unit_test_main();

  // "Expected" assertion failures arrived in libcfuzzed_loop_repeat() and
  // longjmp'ed back here, where control gets returned to libFuzzer.
  if (getenv("LIBCFUZZED_DUMP_ESCAPES")) {
    fprintf(stderr, "Exiting after assertion failure from fuzzer input: ");
    for (size_t i = 0; i < size; i++)
      fprintf(stderr, "%02X ", data[i]);
    fprintf(stderr, "\n");
  }
  return EXIT_FAILURE;
}

// External libFuzzer main loop that generates new data patterns for
// the libfuzzer_callback() handler.
extern int LLVMFuzzerRunDriver(int *, char ***, typeof(libfuzzer_callback));

// The main function for libcfuzzed. It starts the main loop in libFuzzer.
int libcfuzzed_loop(int argc, char *argv[], unit_test_main_t main) {
  unit_test_main = main;
  return LLVMFuzzerRunDriver(&argc, &argv, &libfuzzer_callback);
}

// Endpoint for "expected" assertion failures in the test suite. It drops the
// call stack and directly returns to the libfuzzer_callback() frame.
void libcfuzzed_loop_repeat() {
  longjmp(loop_repeat_landing_pad, EXIT_FAILURE);
}
