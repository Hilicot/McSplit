#!/bin/bash
counter=0
desc=${1:-Default description}
for pair in ascii_edgelists/* ; do
    if [ ! -d "$pair" ]; then
        continue
    fi
    counter=$((counter+1))
    if [ $counter -eq 3 -o $counter -eq 5 ]; then
        continue
    fi
    g1="$pair/g1.txt"
    g2="$pair/g2.txt"
    timeout=300
    #timeout=$(cat "$pair/timeout.txt")
    echo "Processing $pair with timeout $timeout"
    current_time=$(date "+%Y.%m.%d-%H.%M.%S")
    outfile="$pair/result-dal-$current_time.txt"
    ./mcsplit-dal -A -t $timeout min_max $g1 $g2 2>&1 | tee $outfile
    echo $desc >> $outfile
    echo "timeout: $timeout" >> $outfile
done 
python3 /home/porro/telecho/telecho.py marco done
