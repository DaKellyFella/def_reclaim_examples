#!/bin/bash

#  $1 is benchmark binary, $2 is structure, $3 is policy, $4 is size, $5 is range.

for t in {1..4}
do
  numactl -i 0 ./$1 -d 20 --csv -b $2 -p $3 -t $t -i $4 -r $5
done

for t in {9..36..9}
do
  numactl -i 0 ./$1 -d 20 --csv -b $2 -p $3 -t $t -i $4 -r $5
done

for t in {45..72..9}
do
  numactl -i 0,2 ./$1 -d 20 --csv -b $2 -p $3 -t $t -i $4 -r $5
done

for t in {81..108..9}
do
  numactl -i 0,2,1 ./$1 -d 20 --csv -b $2 -p $3 -t $t -i $4 -r $5
done

for t in {117..144..9}
do
  numactl -i 0,1,2,3 ./$1 -d 20 --csv -b $2 -p $3 -t $t -i $4 -r $5
done