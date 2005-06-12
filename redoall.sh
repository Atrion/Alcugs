#!/bin/bash

make clean
make distclean
./reconf.sh && ./configure && make

