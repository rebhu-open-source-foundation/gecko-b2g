# Gecko on Gonk

This is an experiment to see what it takes to build current Gecko on Gonk (M base).

Currently the setup is:
- get an Android M clone of B2G
  `git clone --branch emulator-m https://github.com/gabrielesvelto/B2G`
  `git clone --branch emulator-m https://github.com/gabrielesvelto/b2g-manifest`
  `cd B2G`
  `env REPO_INIT_FLAGS="--depth=1" REPO_SYNC_FLAGS="-j16 --force-sync" GITREPO=../b2g-manifest BRANCH=emulator-m ./config.sh emulator-m`

- modify mozconfig-b2g to point to an android NDK r17b & SDK
- modify build-b2g.sh to point to an android NDK r17b & SDK
- cd to directory `$NDK_DIR/sysroot/usr/lib` and run:
```bash
for i in $NDK_DIR/platforms/android-23/arch-arm/usr/lib/*; do ln -snf $i .; done
```
- edit `$NDK_DIR/sysroot/usr/include/bits/signal_types.h`
- insert these lines in the line `69`:
```c
#undef sa_handler
#undef sa_sigaction
```
- run `./build-b2g.sh`

Note that Gecko now builds with clang > 3.8 instead of gcc. It's unclear if it's mandatory for now but it's likely that gcc builds will break sooner or later.
