#!/bin/sh -e

make clean && make 

./VoxelSpace <(png -tc maps/C1W.png) <(png -tc maps/D1.png)
