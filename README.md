# libcfuzzed

Experimental tool to simulate fuzzy libc behavior for your test suite.
So far it can only apply modifications on the current working directory.
Build require a recent clang release.

## Integrate with your own unit tests in C and C++

That's quite straightforward actually:

```
#include <libcfuzzed.h>

int unit_test_main() {
  // Run all your tests here.
}

int main(int argc, char *argv[]) {
  // Run the unit test in a loop with fuzzy libc behavior.
  if (libcfuzzed_is_loaded())
    return libcfuzzed_loop(argc, argv, &unit_test_main);

  // Run the unit test as usual.
  return unit_test_main();
}
```

There's a few things that you need to talk about with your linker:
```
> clang-12 -g -Wl,--dynamic-list=/home/libcfud/test/unit_test.exported -fsanitize=fuzzer-no-link /path/to/libcfuzzed/build/libcfuzzed.a /usr/lib/llvm-12/lib/clang/12.0.1/lib/linux/libclang_rt.fuzzer_no_main-x86_64.a unit_test.o -o unit_test
```

The [library's own unit test](https://github.com/weliveindetail/libcfuzzed/tree/main/test) gives some insight on how to arrange things. Once this works, you can easily run your unit tests with libcfuzzed enabled:
```
> LD_PRELOAD=/path/to/libcfuzzed/build/libcfuzzed-preload.so ./unit_test
```

Or in order to run it as usual do:
```
> ./unit_test
```

## Build and run

The additional privileges are required to enable debugging in the docker container:

```
% git clone https://github.com/weliveindetail/libcfuzzed
% docker run --privileged --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --security-opt apparmor=unconfined -v $(pwd)/libcfuzzed:/home --name libcfuzzed-dev -it weliveindetail/libcfuzzed-dev
> mkdir /home/build
> cd /home/build
> cmake -GNinja ..
> ninja unit_test
> (cd test && ./unit_test)
INFO: Running with entropic power schedule (0xFF, 100).
INFO: Seed: 1466974996
INFO: Loaded 1 modules   (51 inline 8-bit counters): 51 [0x2f1f08, 0x2f1f3b),
INFO: Loaded 1 PC tables (51 PCs): 51 [0x216dd8,0x217108),
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: A corpus is not provided, starting from an empty corpus
#2      INITED cov: 17 ft: 18 corp: 1/1b exec/s: 0 rss: 25Mb
        NEW_FUNC[1/1]: 0x25be00 in did_trigger_all_assertion_failures /home/libcfud/build/../test/unit_test.c:115
#45     NEW    cov: 21 ft: 22 corp: 2/2b lim: 4 exec/s: 0 rss: 29Mb L: 1/1 MS: 3 CopyPart-CrossOver-ChangeByte-
#49     NEW    cov: 22 ft: 23 corp: 3/4b lim: 4 exec/s: 0 rss: 29Mb L: 2/2 MS: 4 ChangeByte-ChangeBit-ChangeBit-CrossOver-
#66     REDUCE cov: 22 ft: 23 corp: 3/3b lim: 4 exec/s: 0 rss: 30Mb L: 1/1 MS: 2 ChangeByte-EraseBytes-
#77     REDUCE cov: 23 ft: 24 corp: 4/4b lim: 4 exec/s: 0 rss: 30Mb L: 1/1 MS: 1 ChangeByte-
#89     NEW    cov: 25 ft: 26 corp: 5/5b lim: 4 exec/s: 0 rss: 31Mb L: 1/1 MS: 2 ShuffleBytes-ChangeByte-
#171    NEW    cov: 26 ft: 27 corp: 6/7b lim: 4 exec/s: 0 rss: 34Mb L: 2/2 MS: 2 CopyPart-CMP- DE: "\x00\x00"-
#192    REDUCE cov: 26 ft: 27 corp: 6/6b lim: 4 exec/s: 192 rss: 35Mb L: 1/1 MS: 1 EraseBytes-
Found all assertion failures in 268 attempts.

ID  Attempt
0   89
1   268
2   45
```
