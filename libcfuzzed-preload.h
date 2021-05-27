#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

// Landing pad for assertion failures that SHOULD NOT exit fuzzing. The
// libcfuzzed driver uses it to leave the current run of the test suite after
// handle_assertion_failure().
jmp_buf *libcfud_landing_pad();

// The preload lib provides this function and expects it to be called for each
// new corpus.
void libcfuzzed_reset(const uint8_t *data, size_t size);
