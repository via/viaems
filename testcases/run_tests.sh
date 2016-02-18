#!/bin/sh
set -x

for test in $@; do
  cp $test event_in
  ../src/tfi
  cat event_out | python timing_test.py event_out > $(basename $test eventin)timing
  mv event_out $(basename $test eventin)eventout
  rm event_in
done
