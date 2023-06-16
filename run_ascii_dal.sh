#!/bin/bash

results_folder=/home/porro/McSplit/exported_gnn_pairs_synthetic
desc=${1:-Default description}

output_folder="$results_folder/results_dal_$(date "+%Y.%m.%d-%H.%M.%S")"



counter=0
for pair in ../LargeDatasets/processed/syn_pairs/* ; do
    if [ ! -d "$pair" ]; then
        continue
    fi
    counter=$((counter+1))
    #if [ $counter -eq 3 -o $counter -eq 5 ]; then
    #    continue
    #fi
    g1="$pair/g1.txt"
    g2="$pair/g2.txt"
    solution="$pair/solution.txt"
    #timeout=$(cat "$pair/timeout.txt")
    echo "Processing $pair"
    pair_name=$(basename $pair)
    outfile="$output_folder/$pair_name.txt"
    ./build/mcsplit-dal -A -S exported_gnn_pairs_synthetic -y $solution -s pagerank -t 60 min_max $g1 $g2 #2>&1 | tee $outfile
    #echo "timeout: $timeout" >> $outfile
done 
#python3 /home/porro/telecho/telecho.py marco done
