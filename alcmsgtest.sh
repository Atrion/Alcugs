#!/bin/bash
set -e

FILE="config.status"
PORT=31732
if [ "$1" ]; then
	PORT=$1
fi
# cleanup
rm rcvmsg.raw -rf
# start server
./alcmsgtest -lm -lh localhost -lp $PORT -nl &
sleep 1
# send file
./alcmsgtest localhost:$PORT -f "$FILE" 
sleep 1
# kill server
killall -s INT alcmsgtest
sleep 2
TEST="`pidof alcmsgtest || exit 0`" # avoid stopping the script if pidof fails
if [ -n "$TEST" ]; then
	echo "alcmsgtest did not exit within 2s"
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
TEST=`diff rcvmsg.raw $FILE`
if [ -n "$TEST" ] ; then
	echo "sent file and recieved file differ"
	exit 1
fi
echo "Ok, all went fine"
