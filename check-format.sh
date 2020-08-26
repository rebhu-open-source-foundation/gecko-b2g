#!/bin/bash

if [ "$1" = "--fix" ]; then
 arg="--fix"
else
  arg=
fi

CI_COMMIT_SHA=${CI_COMMIT_SHA:-"HEAD"}

js_files=`git show $CI_COMMIT_SHA --name-only --pretty="" | grep '.*\.\(js\|jsm\)$'`
if [ -n "$js_files" ]; then

./mach lint -f unix -l eslint $arg $js_files
done=$?

if [ "$done" = "1" ]; then
  echo "============================================================"
  echo "Please fix linting errors:"
  echo "Run './mach lint --fix $js_files' locally to get more details."
  echo "============================================================"
  exit 1
fi
fi

other_files=`git show $CI_COMMIT_SHA --name-only --pretty="" | grep '.*\.\(c\|cpp\|cc\|css\|rs\|idl\|webidl\)$'`
if [ -n "$other_files" ]; then

./mach lint -f unix $arg $other_files
done=$?

if [ "$done" = "1" ]; then
  echo "============================================================"
  echo "Please fix linting errors:"
  echo "Run './mach lint --fix $other_files' locally to get more details."
  echo "============================================================"
  exit 1
fi
fi
