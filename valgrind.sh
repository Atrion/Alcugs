#!/bin/sh
valgrind --leak-check=full --show-reachable=yes --leak-resolution=med --num-callers=20 --suppressions=suppressions $*
