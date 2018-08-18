#!/bin/bash

#  $1 is the pqueue benchmark binary.


PQUEUE_SIZE=1280000
PQUEUE_RANGE=2560000

# Shavit Lotan Priority Queue
./param_pqueue_benchmark.sh $2 sl_pq leaky $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $2 sl_pq retire $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $2 c_sl_pq leaky $PQUEUE_SIZE $PQUEUE_RANGE

# SprayList Queue
./param_pqueue_benchmark.sh $2 spray leaky $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $2 spray retire $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $2 c_spray leaky $PQUEUE_SIZE $PQUEUE_RANGE
