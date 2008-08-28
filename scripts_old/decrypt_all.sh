#!/bin/bash
# $Id$
# This script decrypts all files in a folder, you should also have urucrypt in your bin path

if [ "$#" -ne 2 ]; then
	echo "Usage $0 source_dir destination_dir"
	exit
fi

res=`ls -l $1 | awk '{print $9}'`

if [ -z "$res" ]; then
	echo "Error"
	exit
fi

mkdir $2

for foo in $res
do
	urucrypt d $1/$foo $2/$foo
done

