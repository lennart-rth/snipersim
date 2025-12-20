#!/bin/bash

CONFIG=$1
BENCH_DIR=$2
RESULT_DIR=$3
N_PROCS=${4:-$(nproc)}

for SUB_BENCH in $BENCH_DIR/*; do
  #if its not a directory, skip
  [ -d "${SUB_BENCH}" ] || continue

  SUB_BENCH_NAME=$(basename $SUB_BENCH)
  SUB_BENCH_RESULT_DIR=$RESULT_DIR/$SUB_BENCH_NAME
  mkdir -p $SUB_BENCH_RESULT_DIR
  echo "Running benchmark: $SUB_BENCH_NAME"

  for PP_ADDR in $SUB_BENCH/*.address; do
    while [ $(jobs -r -p | wc -l) -ge $N_PROCS ]; do
      wait -n
    done
    PP_NAME=$(basename ${PP_ADDR%.*})

    PP_RESULT_DIR=$SUB_BENCH_RESULT_DIR/$PP_NAME
    mkdir -p $PP_RESULT_DIR

    ./run-sniper -c $CONFIG -d $PP_RESULT_DIR \
      --pinplay-addr-trans --pinballs=$SUB_BENCH/$PP_NAME \
      > $PP_RESULT_DIR.out 2> $PP_RESULT_DIR.err &
  done
done

wait
