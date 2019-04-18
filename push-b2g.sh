#!/bin/bash

set -e

adb root && adb remount

adb shell stop b2g

mkdir tmp_b2g
cd tmp_b2g
tar xf ../$GECKO_OBJDIR/dist/b2g-68.0a1.en-US.linux-androideabi-arm.tar.gz
cd b2g
adb push * /data/kaios/b2g/
cd ../..
rm -rf tmp_b2g

#adb shell start b2g
