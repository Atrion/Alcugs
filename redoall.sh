#!/bin/bash

dest="/home/alcugs"

#make clean
#make distclean
./tclean.sh
./reconf.sh && ./configure --prefix=$dest $@ && make && make check && make install

