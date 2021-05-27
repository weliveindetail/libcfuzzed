#ifndef LIBCFUZZED_H
#define LIBCFUZZED_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*unit_test_main_t)();

// Determine dynamically whether the libcfuzzed-preload.so is available or not.
//
// Test suites would typically run the libcfuzzed_loop() if the preload lib was
// passed from the command line and otherwise just run as usual. This allows to
// permanently hardcode libcfuzzed in the unit-test driver and only use it in
// case the preload lib is actually provided. It doesn't need separate binaries
// or compile-time switches.
bool libcfuzzed_is_loaded();

// Call this from the main function in your unit-test in order to start running
// the given function with fuzzy libc behavior. The given function would usually
// be the one that runs all your actual unit-tests.
int libcfuzzed_loop(int argc, char *argv[], unit_test_main_t);

// Call this function in cases where assertions fail "expectedly" for one reason
// or the other. The fuzzer will see small code coverage and it will likely
// assume that it's not worth digging deeper. It will start the next interation
// with a new libc behavior.
void libcfuzzed_loop_repeat();

#ifdef __cplusplus
}
#endif

#endif // LIBCFUZZED_H
