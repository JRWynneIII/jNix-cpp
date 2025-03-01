# jNix x86_64

This project is simply a playground to learn more about kernel/x86_64 development and various OS internals. Don't expect this to be anything useful/pretty/exciting/novel/etc.

Requirements: x86_64 GCC cross compilation toolchain, Qemu 9.1.2, git, Xorriso, wget

The makefile will automatically download and build the appropriate deps:
* limine
* flanterm


To build:
```
make
```

To run:
```
make run
```

To run and block waiting on a debugger to attach to qemu:
```
make debug
```
