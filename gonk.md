# Gecko on Gonk

This is an experiment to see what it takes to build current Gecko on Gonk (M base).

## Preparing an emulator build
Currently the setup is:
- get an Android M clone of B2G and build it:
  `git clone --branch emulator-m https://github.com/gabrielesvelto/B2G`
  `git clone --branch emulator-m https://github.com/gabrielesvelto/b2g-manifest`
  `cd B2G`
  `env REPO_INIT_FLAGS="--depth=1" REPO_SYNC_FLAGS="-j16 --force-sync" GITREPO=../b2g-manifest BRANCH=emulator-m ./config.sh emulator-m`
  `./build.sh`

## Preparing a device build
- Use a b2g checkout already built for your device.

## Get toolchain dependencies
Run `./mach bootstrap` first and choose option 4 (GeckoView/Firefox for Android)

## Build system "fixes"
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

## Building
- run `./build-b2g.sh`
- package with `./build-b2g.sh package`

## Running on the emulator
- unpack the dist tarball in the B2G folder `tar -x -v -f obj-arm-unknown-linux-androideabi/dist/b2g-*.tar.gz -C ../B2G/out/target/product/generic/system/`
- go into the B2G folder
- rebuild the emulator with `./build.sh ENABLE_DEFAULT_BOOTANIMATION=true`
- run the emulator with `./run-emulator.sh`

## Running on device
- push the build by running `./push-b2g.sh`
- start it with `adb shell b2g.sh` or `adb shell start b2g`

## Serving test content.

A basic system app loading a page in remote iframe is in b2g/system. The shell is for now hardcoded to load http://localhost:8081/system/index.html as a system UI.

If you can't run a http server on device, remote the port 8081 to your host:
- `adb root`
- `adb reverse tcp:8081 tcp:8081`
- `cd $path-to-gecko-dev/b2g`
- `python -m SimpleHTTPServer 8081`

## Download Build Artifacts
- Click the pipeline status will redirect to the latest commit of the default branch.
- In the lastest commit page, click the related build to be downloaded.
- In the jobs page, the job artifacts are available at the right menu.

Note: Will keep the artifacts for two weeks only.
