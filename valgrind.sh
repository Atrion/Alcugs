#!/bin/sh
# I'm not sure about these suppressions, but I can't find a way to fix them and hiding them makes searching for new problems easier
valgrind --leak-check=full --show-reachable=yes --leak-resolution=med --suppressions=suppressions $*
