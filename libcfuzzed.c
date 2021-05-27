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

// Default implementation so we can link and run without libcfuzzed-preload.so
void libcfuzzed_reset(const uint8_t *data, size_t size) {
  abort(); // We never arrive here.
}

// Pointer to the implementation in the preload lib or NULL.
static void (*real_libcfuzzed_reset)(const uint8_t *, size_t) = NULL;

// Function that runs inside the libFuzzer main loop.
int libfuzzer_callback(const uint8_t *data, size_t size) {
  // Initialize LD_PRELOAD fuzzer.
  if (real_libcfuzzed_reset)
    real_libcfuzzed_reset(data, size);

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

// If the shared library was not preloaded, then run the test suite as usual.
// This is a feature, because it allows to permanently hardcode libcfuzzed in
// the unit-test driver and only use it in case the preload lib is actually
// provided.
bool libcfuzzed_is_loaded() {
  if (real_libcfuzzed_reset == NULL)
    real_libcfuzzed_reset = dlsym(RTLD_NEXT, "libcfuzzed_reset");

  return (real_libcfuzzed_reset != NULL);
}

// Print a warning for the user to realize that the preload lib is missing.
static void warn_preload_lib_missing(int argc, char *argv[]) {
  fprintf(stderr, "Warning: libcfuzzed_loop() running without "
                  "libcfuzzed-preload.so! This means fuzzing is disabled. Your "
                  "test suite will run in an endless loop without any change. "
                  "Please consider to restart your test suite like this:\n\n"
                  "  > LD_PRELOAD=libcfuzzed-preload.so ");
  for (int i = 0; i < argc; ++i)
    fprintf(stderr, "%s ", argv[i]);
  fprintf(stderr, "\n\nExit now? [Y/n]\n");

  char answer;
  if (scanf("%c", &answer) != 1 || answer != 'n')
    exit(1);
}

// The main function for libcfuzzed. It starts the main loop in libFuzzer.
int libcfuzzed_loop(int argc, char *argv[], unit_test_main_t main) {
  if (!libcfuzzed_is_loaded())
    warn_preload_lib_missing(argc, argv);

  unit_test_main = main;
  return LLVMFuzzerRunDriver(&argc, &argv, &libfuzzer_callback);
}

// Endpoint for "expected" assertion failures in the test suite. It drops the
// call stack and directly returns to the libfuzzer_callback() frame.
void libcfuzzed_loop_repeat() {
  longjmp(loop_repeat_landing_pad, EXIT_FAILURE);
}
