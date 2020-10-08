#!/bin/sh

adb shell /system/bin/gfxdbg -c gralloc -l

INDICES=`adb shell /system/bin/gfxdbg -c gralloc -l | grep index | awk 'BEGIN{FS="="}{print $2}'`
for i in $INDICES; do
  CMD="adb shell /system/bin/gfxdbg -c gralloc -d $i | grep output"
  echo $CMD
  OUTPUT=`$CMD | awk 'BEGIN{FS=":"}{print $3}'`
  CMD="adb pull $OUTPUT"
  echo $CMD
  `$CMD`

  FILENAME=$(basename -- "$OUTPUT")
  MAIN_FN="${FILENAME%.*}"

  SIZE=`echo $MAIN_FN | awk 'BEGIN{FS="-"}{print $3}'`
  CMD="ffmpeg -loglevel quiet -f rawvideo -pixel_format rgba -video_size $SIZE -i $FILENAME $MAIN_FN.png"
  echo $CMD
  eval $CMD
done
