#!/bin/bash

GECKO_OBJDIR=${GECKO_OBJDIR:-objdir-gecko}

# Copy file/dir to device via ADB
#
# Usage: adb_push <local> <remote>
# The behavior would act like "adb push <local>/* <remote>"
adb_push()
{
  if [ -d $1 ]; then
    LOCAL_FILES="$1/*"
    for file in $LOCAL_FILES
    do
      adb_push $file "$2/$(basename $file)"
    done
  else
    printf "push: %-30s " $(basename $file)
    adb push $1 $2
  fi
}


adb root
adb wait-for-device
adb remount

adb shell stop b2g

mkdir -p tmp
tar xf $GECKO_OBJDIR/dist/b2g-*-arm.tar.gz -C tmp
adb_push tmp/b2g /system/b2g
adb shell chmod +x /system/b2g/b2g
rm -rf tmp

adb shell start b2g
