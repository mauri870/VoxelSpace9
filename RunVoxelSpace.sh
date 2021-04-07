#!/bin/sh -e

png -tc maps/C1W.png > /tmp/C1W.data
png -tc maps/D1.png > /tmp/D1.data

make clean && make 

./VoxelSpace /tmp/C1W.data /tmp/D1.data
