typedef int(*unit_test_main_t)();

// Call this from the main function in your unit-test in order to start running
// the given function with fuzzy libc behavior.
int libcfuzzed_loop(int argc, char *argv[], unit_test_main_t);

// Call this function in cases where assertions fail "expectedly" for one reason
// or the other. The fuzzer will see small code coverage and assume it's not
// worth to dig deeper. The fuzzer loop will continue running with a new libc
// behavior.
void libcfuzzed_loop_repeat();
