#!/bin/bash
counter=0
desc=${1:-Default description}
for pair in ascii_edgelists/* ; do
    counter=$((counter+1))
    if [ $counter -eq 3 -o $counter -eq 5 ]; then
        continue
    fi
    g1="$pair/g1.txt"
    g2="$pair/g2.txt"
    timeout=300    
    #timeout=$(cat "$pair/timeout.txt")
    echo "Processing $pair with timeout $timoeout"
    current_time=$(date "+%Y.%m.%d-%H.%M.%S")
    ./build/mcsplit-dal -A -t $timeout min_max $g1 $g2 2>&1 | tee "$pair/result-dal-$current_time.txt"
    echo $desc >> $pair/result-dal-$current_time.txt
done 
python3 /home/porro/telecho/telecho.py salvo done
