#!/bin/bash
export MALLOC_CHECK_=3
ulimit -c unlimited
#eval `command ./dmalloc/dmalloc -l memory.log -i 1 high -p check-fence`
eval `command dmalloc -l memory.log -i 1 high -p check-fence -p error-free-null`

if [ -n "$1" ]; then
  $1
fi

