#!/bin/bash

#  $1 is the set benchmark binary.

SKIPLIST_SIZE=1280000
SKIPLIST_RANGE=2560000

BINARY_TREE_SIZE=1280000
BINARY_TREE_RANGE=2560000

HASH_TABLE_SIZE=3200000
HASH_TABLE_RANGE=6400000

# Lock-Free Skip List
./param_set_numa_benchmark.sh $1 fhsl_lf leaky $SKIPLIST_SIZE $SKIPLIST_RANGE 20
./param_set_numa_benchmark.sh $1 fhsl_lf retire $SKIPLIST_SIZE $SKIPLIST_RANGE 20
./param_set_numa_benchmark.sh $1 c_fhsl_lf leaky $SKIPLIST_SIZE $SKIPLIST_RANGE 20

# Lock-Free Binary Tree
./param_set_numa_benchmark.sh $1 bt_lf leaky $BINARY_TREE_SIZE $BINARY_TREE_RANGE 20
./param_set_numa_benchmark.sh $1 bt_lf retire $BINARY_TREE_SIZE $BINARY_TREE_RANGE 20
./param_set_numa_benchmark.sh $1 c_bt_lf leaky $BINARY_TREE_SIZE $BINARY_TREE_RANGE 20

# Maged Michael Hash Table
./param_set_numa_benchmark.sh $1 mm_ht leaky $HASH_TABLE_SIZE $HASH_TABLE_RANGE 20
./param_set_numa_benchmark.sh $1 mm_ht retire $HASH_TABLE_SIZE $HASH_TABLE_RANGE 20
./param_set_numa_benchmark.sh $1 c_mm_ht leaky $HASH_TABLE_SIZE $HASH_TABLE_RANGE 20