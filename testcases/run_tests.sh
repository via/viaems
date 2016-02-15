#!/bin/sh
set -x

for test in $@; do
  cp $test event_in
  ../src/tfi
  cat event_out | python timing_test.py event_out > $(basename $test eventin)timing
  rm event_in event_out
done
