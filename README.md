# libcfuzzed

## Run docker container

```
docker run --privileged --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --security-opt apparmor=unconfined -v $(pwd):/home --name debian_fuzzdep -t weliveindetail/libcfuzzed-debian
```
