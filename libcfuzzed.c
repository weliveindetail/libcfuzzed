#include "libcfuzzed-preload.h"

#include <stdio.h>
#include <stdlib.h>

extern int unit_test_main();

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Initialize LD_PRELOAD fuzzer.
  libcfuzzed_reset(data, size);

  // On assertion failures we drop the call stack and jump back here with an
  // error status that we return the to fuzzer driver.
  int exit_code;
  if ((exit_code = setjmp(*libcfuzzed_landing_pad())) != EXIT_SUCCESS) {
    fprintf(stderr, "Exiting after assertion failure from fuzzer input: ");
    for (size_t i = 0; i < size; i++)
      fprintf(stderr, "%02X ", data[i]);
    fprintf(stderr, "\n");
    return exit_code;
  }

  // Run the test suite as usual.
  return unit_test_main();
}
