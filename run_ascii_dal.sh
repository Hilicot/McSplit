#!/bin/bash
counter=0
for pair in ascii_edgelists/* ; do
    counter=$((counter+1))
    if [ $counter -eq 3 -o $counter -eq 5 ]; then
        continue
    fi
    echo "Processing $pair"
    g1="$pair/g1.txt"
    g2="$pair/g2.txt"
    #timeout=3000
    timeout=$(cat "$pair/timeout.txt")
    ./mcsplit-dal min_max classic -A -t $timeout $g1 $g2 2>&1 | tee "$pair/result-dal.txt"
done 
python3 /home/porro/telecho/telecho.py marco done
