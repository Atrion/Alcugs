#!/bin/bash
set -e
# save source and build dir
builddir="$(pwd)/.."
srcdir="$1"
# get us a clean directory to work in
tmpdir="/tmp/alctest"
rm -rf "$tmpdir"
mkdir "$tmpdir"
cd "$tmpdir"
# do it!
FILE="$srcdir/Doxyfile"
PORT=5938
if [ "$2" ]; then
	FILE=$2
fi
# start server
"$builddir/servers/alcmsgtest" -lm -lh localhost -lp "$PORT" -nl &
sleep 0.5
# send file
"$builddir/servers/alcmsgtest" "localhost:$PORT" -f "$FILE" -v 1 ; # wait till it finished
sleep 0.5
# kill server
killall -s INT alcmsgtest
sleep 1.5
TEST="$(pidof alcmsgtest || exit 0)" # avoid stopping the script if pidof fails
if [ -n "$TEST" ]; then
	echo "alcmsgtest did not exit within 1.5s"
	exit 1
fi
# the error log files must be empty
if [ -s log/error.log ]; then
	echo "WARN: error.log is not empty"
fi
if [ -s log/uneterr.log ]; then
	echo "WARN: uneterr.log is not empty"
fi
# the recieved file must exist
if [ ! -e rcvmsg.raw ] ; then
	echo "recieved file doesn't exist"
	exit 1
fi
# compare files
"$builddir/alcnet/bincomp" rcvmsg.raw "$FILE" > /dev/null || (
	echo "sent file and recieved file differ";
	exit 1
)
rm -rf "$tmpdir"
echo "Ok, all went fine"
