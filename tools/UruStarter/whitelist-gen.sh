#!/bin/bash
# This script creates a file in the syntax UruStarter expects for whitelist-checksums.txt
for file in $*; do
	md5=`md5sum "$file"`
	md5=${md5%% *}
	size=`du -b "$file"`
	size=${size%%[^0-9]*}
	echo "$md5,$size  $file"
done
