#!/bin/bash
export MALLOC_CHECK_=3
ulimit -c unlimited

if [ -n "$1" ]; then
  $1
fi

