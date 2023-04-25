#!/bin/bash

cd ../../build/bin/bm


for i in 0 1 2 3 4 5 6 7
do
   valgrind --tool=massif --massif-out-file=../../../benchmarks/outputs/ram/fuse_ram_out_$i.txt ./fuse_ram_bm $i
   valgrind --tool=massif --massif-out-file=../../../benchmarks/outputs/ram/hycc_ram_out_$i.txt ./hycc_ram_bm $i
done

cd ../../../benchmarks/outputs/ram

for j in 0 1 2 3 4 5 6 7
do
    ms_print fuse_ram_out_$j.txt | tee fuse_vis_$j.txt
    ms_print hycc_ram_out_$j.txt | tee hycc_vis_$j.txt
done
