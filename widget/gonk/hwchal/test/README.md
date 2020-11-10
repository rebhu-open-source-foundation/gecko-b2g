# Tests for hwchal

## Build

```bash
source build/envsetup.sh 

lunch msm8937_32go

mmm gecko/widget/gonk/hwchal/test/
```

## Run

```bash
#!/bin/sh

set -e

PROJECT=msm8937_32go

adb push --sync out/target/product/$PROJECT/system/lib/libhwchaltest.so /system/lib
adb push --sync out/target/product/$PROJECT/testcases/testHwc2/arm/testHwc2 /system/bin/test
adb shell chmod +x /system/bin/test

adb shell /system/bin/test --ShowValidateResult

```

## Example output

```text
[==========] Running 10 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 10 tests from Hwc2Test
[ RUN      ] Hwc2Test.GET_CAPABILITIES
[       OK ] Hwc2Test.GET_CAPABILITIES (138 ms)
[ RUN      ] Hwc2Test.CONNECT_TO_PRIMARYDISPLAY
layer size: 240 x 320
[       OK ] Hwc2Test.CONNECT_TO_PRIMARYDISPLAY (310 ms)
[ RUN      ] Hwc2Test.CREATE_DESTROY_LAYER
[       OK ] Hwc2Test.CREATE_DESTROY_LAYER (11 ms)
[ RUN      ] Hwc2Test.SET_VSYNC_ENABLED_callback
[       OK ] Hwc2Test.SET_VSYNC_ENABLED_callback (319 ms)
[ RUN      ] Hwc2Test.VALIDATE_DISPLAY_default_5
[       OK ] Hwc2Test.VALIDATE_DISPLAY_default_5 (490 ms)
[ RUN      ] Hwc2Test.GET_DISPLAY_REQUESTS_basic
[       OK ] Hwc2Test.GET_DISPLAY_REQUESTS_basic (1320 ms)
[ RUN      ] Hwc2Test.PRESENT_DISPLAY_layer_test_1
0: 1.0-Device
1: 0.5-Device->Client
[       OK ] Hwc2Test.PRESENT_DISPLAY_layer_test_1 (589 ms)
[ RUN      ] Hwc2Test.PRESENT_DISPLAY_layer_test_2
2: 1.0-Device->Client 1.0-Device->Client
2: 0.5-Device->Client 1.0-Device->Client
2: 1.0-Device->Client 0.5-Device->Client
2: 0.5-Device->Client 0.5-Device->Client
[       OK ] Hwc2Test.PRESENT_DISPLAY_layer_test_2 (1060 ms)
[ RUN      ] Hwc2Test.PRESENT_DISPLAY_layer_test_3
3: 1.0-Device->Client 1.0-Device->Client 1.0-Device->Client
3: 0.5-Device->Client 1.0-Device->Client 1.0-Device->Client
3: 1.0-Device->Client 0.5-Device->Client 1.0-Device->Client
3: 0.5-Device->Client 0.5-Device->Client 1.0-Device->Client
3: 1.0-Device->Client 1.0-Device->Client 0.5-Device->Client
3: 0.5-Device->Client 1.0-Device->Client 0.5-Device->Client
3: 1.0-Device->Client 0.5-Device->Client 0.5-Device->Client
3: 0.5-Device->Client 0.5-Device->Client 0.5-Device->Client
[       OK ] Hwc2Test.PRESENT_DISPLAY_layer_test_3 (2180 ms)
[ RUN      ] Hwc2Test.PRESENT_DISPLAY_layer_test_4
Warning: Unsupported ChipID value for QGPU features (0x3000620), using a3x as default.
4: 1.0-Device->Client 1.0-Device->Client 1.0-Device->Client 1.0-Device->Client
4: 0.5-Device->Client 1.0-Device->Client 1.0-Device->Client 1.0-Device->Client
4: 1.0-Device->Client 0.5-Device->Client 1.0-Device->Client 1.0-Device->Client
4: 0.5-Device->Client 0.5-Device->Client 1.0-Device->Client 1.0-Device->Client
4: 1.0-Device->Client 1.0-Device->Client 0.5-Device->Client 1.0-Device->Client
4: 0.5-Device->Client 1.0-Device->Client 0.5-Device->Client 1.0-Device->Client
4: 1.0-Device->Client 0.5-Device->Client 0.5-Device->Client 1.0-Device->Client
4: 0.5-Device->Client 0.5-Device->Client 0.5-Device->Client 1.0-Device->Client
4: 1.0-Device->Client 1.0-Device->Client 1.0-Device->Client 0.5-Device->Client
4: 0.5-Device->Client 1.0-Device->Client 1.0-Device->Client 0.5-Device->Client
4: 1.0-Device->Client 0.5-Device->Client 1.0-Device->Client 0.5-Device->Client
4: 0.5-Device->Client 0.5-Device->Client 1.0-Device->Client 0.5-Device->Client
4: 1.0-Device->Client 1.0-Device->Client 0.5-Device->Client 0.5-Device->Client
4: 0.5-Device->Client 1.0-Device->Client 0.5-Device->Client 0.5-Device->Client
4: 1.0-Device->Client 0.5-Device->Client 0.5-Device->Client 0.5-Device->Client
4: 0.5-Device->Client 0.5-Device->Client 0.5-Device->Client 0.5-Device->Client
[       OK ] Hwc2Test.PRESENT_DISPLAY_layer_test_4 (4850 ms)
[----------] 10 tests from Hwc2Test (11269 ms total)

[----------] Global test environment tear-down
[==========] 10 tests from 1 test suite ran. (11277 ms total)
[  PASSED  ] 10 tests.
```