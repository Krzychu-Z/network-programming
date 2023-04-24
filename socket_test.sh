#!/bin/bash

# Start the server
docker exec -d -it network-programming-pc1-1 ./srvv6

# Get the PID
pid=$( docker exec -it network-programming-pc1-1 ps aux | grep srvv6 | awk -F ' ' '{ print $2 }' | head -n 1 )

# Open first connection
docker exec -it network-programming-pc2-1 ./cliv6 fc00:1:1:1::1

# Restart the server
docker exec -it network-programming-pc1-1 kill -9 $pid

#This should not be allowed (Docker issue?)
docker exec -d -it network-programming-pc1-1 ./srvv6

docker exec -it network-programming-pc2-1 ./cliv6 fc00:1:1:1::1
