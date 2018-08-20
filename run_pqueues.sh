#!/bin/bash

#  $1 is the pqueue benchmark binary.


PQUEUE_SIZE=1280000
PQUEUE_RANGE=2560000

# Linden Jonsson Queue
./param_pqueue_benchmark.sh $1 lj_pq leaky $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $1 lj_pq retire $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $1 c_lj_pq leaky $PQUEUE_SIZE $PQUEUE_RANGE

# SprayList Queue
./param_pqueue_benchmark.sh $1 spray leaky $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $1 spray retire $PQUEUE_SIZE $PQUEUE_RANGE
./param_pqueue_benchmark.sh $1 c_spray leaky $PQUEUE_SIZE $PQUEUE_RANGE
