#!/bin/bash

#  $1 is the set benchmark binary, $2 is the pqueue benchmark binary.

# Lock-Free Skip List
./param_set_benchmark.sh $1 fhsl_lf leaky 12800000 25600000 10
./param_set_benchmark.sh $1 fhsl_lf retire 12800000 25600000 10
./param_set_benchmark.sh $1 c_fhsl_lf leaky 12800000 25600000 10

# Lock-Free Binary Tree
./param_set_benchmark.sh $1 bt_lf leaky 12000000 24000000 10
./param_set_benchmark.sh $1 bt_lf retire 12000000 24000000 10
./param_set_benchmark.sh $1 c_bt_lf leaky 12000000 24000000 10

# Maged Michael Hash Table
./param_set_benchmark.sh $1 mm_ht leaky 32000000 64000000 10
./param_set_benchmark.sh $1 mm_ht retire 32000000 64000000 10
./param_set_benchmark.sh $1 c_mm_ht leaky 32000000 64000000 10

# Shavit Lotan Priority Queue
./param_pqueue_benchmark.sh $2 sl_pq leaky 12000000 24000000
#./param_pqueue_benchmark.sh $2 sl_pq retire 12000000 24000000
./param_pqueue_benchmark.sh $2 c_sl_pq leaky 12000000 24000000

# SprayList Queue
./param_pqueue_benchmark.sh $2 spray leaky 12000000 24000000
#./param_pqueue_benchmark.sh $2 sl_pq retire 12000000 24000000
./param_pqueue_benchmark.sh $2 c_spray leaky 12000000 24000000
