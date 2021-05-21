#include "libcfu.h"

#include <stdio.h>
#include <stdlib.h>

extern int unit_test_main();

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Initialize LD_PRELOAD fuzzer.
  //(void)data;
  //uint8_t crash[] = { 'a' }; //0x3f, 0x8a
  //fuzzdep_reset_corpus(crash, 0);
  fuzzdep_reset_corpus(data, size);

  // On assertion failures we drop the call stack and jump back here with an
  // error status that we return the to fuzzer driver.
  int exit_code;
  if ((exit_code = setjmp(*fuzzdep_landing_pad())) != EXIT_SUCCESS) {
    fprintf(stderr, "Exiting after assertion failure from fuzzer input: ");
    for (size_t i = 0; i < size; i++)
      fprintf(stderr, "%02X ", data[i]);
    fprintf(stderr, "\n");
    return exit_code;
  }

  // Run the test suite as usual.
  return unit_test_main();
}
