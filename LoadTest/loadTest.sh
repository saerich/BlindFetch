#!/bin/bash
N=${1:-1}
mkdir -p logs
for ((i=1; i<=N; i++)); do
    ./build/BlindFetchLoadTest > "logs/client_$i.log" 2>&1 &
done
wait
echo "All $N clients finished."