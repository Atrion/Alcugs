#!/bin/bash

./tclean.sh
./reconf.sh && ./myconf.sh $@ && make && make check && make install

