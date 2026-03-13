#!/bin/bash

# Usage: ./plots.sh -f <flag> -np <num_processors>
# Flags: -s (Scatter), -g (AllGather), -r (AllReduce), -a (AlltoAll_Arbitrary), -h (AlltoAll_Hypercube)

FLAG=""
NP=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -f) FLAG="$2"; shift 2 ;;
        -np) NP="$2"; shift 2 ;;
        *) echo "Unknown argument: $1"; exit 1 ;;
    esac
done

if [[ -z "$FLAG" || -z "$NP" ]]; then
    echo "Usage: $0 -f <flag> -np <num_processors>"
    echo "  Flags: -s, -g, -r, -a, -h"
    exit 1
fi

SIZES=(10000 100000 1000000 10000000 100000000)

for SIZE in "${SIZES[@]}"; do
    echo "Running: srun -n $NP ./primitives $FLAG $SIZE 7"
    srun -n "$NP" ./primitives "$FLAG" "$SIZE" 7
    echo "----------------------------------"
done
