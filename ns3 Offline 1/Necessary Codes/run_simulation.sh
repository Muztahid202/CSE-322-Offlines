#!/bin/bash

# Define the possible values for each parameter
NODE_NUMS=(20 40 70 100)
PACKETS_PER_SECOND=(100 200 300 400)
SPEEDS=(5 10 15 20)

# Define constant values to use when varying a parameter
CONSTANT_NODE_NUM=20
CONSTANT_PACKETS=100
CONSTANT_SPEED=5

# Output file for results (ensure it matches the CSV file used in your program)
OUTPUT_FILE="result.csv"

## Ensure the header is written only once
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "NumOfNodes,PacketsPerSec,NodeSpeed,Throughput,EndToEndDelay,PacketDeliveryRatio,PacketDropRatio" > $OUTPUT_FILE
fi

# Vary the number of nodes while keeping other parameters constant
for NODES in "${NODE_NUMS[@]}"; do
    echo "Running simulation with NodeNum=$NODES, PacketsPerSec=$CONSTANT_PACKETS, Speed=$CONSTANT_SPEED"
    ./ns3 run "scratch/raodv_usage --numberOfNodes=$NODES --packetsPerSecond=$CONSTANT_PACKETS --nodeSpeed=$CONSTANT_SPEED"
done

# Vary the number of packets per second while keeping other parameters constant
for PACKETS in "${PACKETS_PER_SECOND[@]}"; do
    echo "Running simulation with NodeNum=$CONSTANT_NODE_NUM, PacketsPerSec=$PACKETS, Speed=$CONSTANT_SPEED"
    ./ns3 run "scratch/raodv_usage --numberOfNodes=$CONSTANT_NODE_NUM --packetsPerSecond=$PACKETS --nodeSpeed=$CONSTANT_SPEED"
done

# Vary the speed of nodes while keeping other parameters constant
for SPEED in "${SPEEDS[@]}"; do
    echo "Running simulation with NodeNum=$CONSTANT_NODE_NUM, PacketsPerSec=$CONSTANT_PACKETS, Speed=$SPEED"
    ./ns3 run "scratch/raodv_usage --numberOfNodes=$CONSTANT_NODE_NUM --packetsPerSecond=$CONSTANT_PACKETS --nodeSpeed=$SPEED"
done

echo "All simulations completed. Results stored in $OUTPUT_FILE."
