typedef int(*unit_test_main_t)();

// Call this from the main function in your unit-test in order to start running
// the given function with fuzzy libc behavior. The given function would usually
// be the one that runs all your actual unit-tests.
int libcfuzzed_loop(int argc, char *argv[], unit_test_main_t);

// Call this function in cases where assertions fail "expectedly" for one reason
// or the other. The fuzzer will see small code coverage and it will likely
// assume that it's not worth digging deeper. It will start the next interation
// with a new libc behavior.
void libcfuzzed_loop_repeat();
