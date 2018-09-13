# Gecko on Gonk, the return of the revenge.

This is an experiment to see what it takes to build current Gecko on Gonk (M base).

Currently the setup is:
- modify mozconfig-b2g to point to an android NDK (I used 15c since 17b crashes compiling SpiderMonkey on my machine).
- `export MOZCONFIG=mozconfig-b2g`
- `export GONK_PATH=<path_to_you_gonk_repo>`
- run `./mach build`

The build fails because of mismatches between includes from the NDK and local ones from Gonk, like:
```
0:19.75 In file included from /home/fabrice/dev/gecko-dev/widget/gonk/nativewindow/FakeSurfaceComposer.cpp:27:
 0:19.75 In file included from /home/fabrice/dev/kaios/qrd256/system/core/include/private/android_filesystem_config.h:32:
 0:19.75 /home/fabrice/dev/kaios/qrd256/system/core/include/private/android_filesystem_capability.h:37:16: error: redefinition of '__user_cap_header_struct'
 0:19.76 typedef struct __user_cap_header_struct {
 0:19.76                ^
 0:19.76 /home/fabrice/.mozbuild/android-ndk-r15c/platforms/android-16/arch-arm/usr/include/linux/capability.h:20:16: note: previous definition is here
 0:19.76 typedef struct __user_cap_header_struct {
```

I tried to setup the build more manually with the toolchain flags as environment variables in the `build.sh` script. This may be the right path but that doesn't work either yet.

Note that Gecko now builds with clang > 3.8 instead of gcc. It's unclear if it's mandatory for now but it's likely that gcc builds will break sooner or later.