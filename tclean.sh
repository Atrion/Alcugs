#!/bin/bash
#cleans the tree

make distclean

rm log -rf
rm dumps -rf
rm *.raw -f
rm game1/log -rf
rm game1/dumps -rf
rm game1/*.raw -f
rm game2/log -rf
rm game2/dumps -rf
rm game2/*.raw -f
rm game3/log -rf
rm game3/dumps -rf
rm game3/*.raw -f
rm auth/log -rf
rm auth/dumps -rf
rm auth/*.raw -f
rm vault/log -rf
rm vault/dumps -rf
rm vault/*.raw -f
rm lobby/log -rf
rm lobby/dumps -rf
rm lobby/*.raw -f
rm tracking/log -rf
rm tracking/dumps -rf
rm tracking/*.raw -f
rm meta/log -rf
rm meta/dumps -rf
rm meta/*.raw -f
rm *.core
rm core
rm configure
rm config.guess
rm Makefile.in
rm config.h.in

./tclean.py

