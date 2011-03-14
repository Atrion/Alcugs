#!/bin/bash
set -e
# save source and build dir
if [[ -f "CMakeCache.txt" ]]; then
	builddir="$(pwd)"
else # assume we are run by the testsuite
	builddir="$(pwd)/.."
fi
srcdir="$(dirname $0)/../../"
# get source file
FILE="$srcdir/Doxyfile"
PORT=5938
if [ "$1" ]; then
	FILE=$1
fi
FILE=$(readlink "$FILE" -e) # get absolute filename
echo "Using test file $FILE"
# get us a clean directory to work in
tmpdir="/tmp/alctest"
rm -rf "$tmpdir"
mkdir "$tmpdir"
cd "$tmpdir"
# start server
"$builddir/servers/alcmsgtest" -lh localhost -lp "$PORT" -nl -v 0 &
sleep 0.1 # give it some time to get up
# send file
"$builddir/servers/alcmsgtest" "localhost:$PORT" -f "$FILE" -v 3 ; # wait till it finished
# kill server
killall -s INT alcmsgtest
sleep 0.1 # give it time to go down
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
