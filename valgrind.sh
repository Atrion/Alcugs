#!/bin/sh
valgrind --track-origins=yes --leak-check=full --show-reachable=yes --leak-resolution=med --num-callers=20 --suppressions=$(dirname $0)/suppressions $*
