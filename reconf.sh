#!/bin/bash

rm -f config.cache
rm -f acconfig.h
aclocal -I m4
autoconf -W all
autoheader -W all
automake -a --copy

