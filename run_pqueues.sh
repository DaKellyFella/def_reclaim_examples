#!/bin/bash

#  $1 is the pqueue benchmark binary.

# Shavit Lotan Priority Queue
./param_pqueue_benchmark.sh $2 sl_pq leaky 12000000 24000000
./param_pqueue_benchmark.sh $2 sl_pq retire 12000000 24000000
./param_pqueue_benchmark.sh $2 c_sl_pq leaky 12000000 24000000

# SprayList Queue
./param_pqueue_benchmark.sh $2 spray leaky 12000000 24000000
./param_pqueue_benchmark.sh $2 sl_pq retire 12000000 24000000
./param_pqueue_benchmark.sh $2 c_spray leaky 12000000 24000000
