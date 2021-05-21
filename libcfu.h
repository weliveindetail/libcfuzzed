#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>

// The preload lib provides the actual landing pad for assertion failures. It's
// used to install the top-level stack frame as the return point for assertion
// failures before any run of the test suite.
jmp_buf *fuzzdep_landing_pad();

// The preload lib provides this function and expects it to be called for each
// new corpus.
void fuzzdep_reset_corpus(const uint8_t *data, size_t size);
