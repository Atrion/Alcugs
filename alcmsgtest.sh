#!/bin/bash
FILE="src/alcnet/sql.cpp"
PORT=31732
# start server
./alcmsgtest -lm -lh localhost -lp $PORT -nl &
PID=`pidof alcmsgtest`
sleep 1
# send file
./alcmsgtest localhost:$PORT -f $FILE -z > /dev/null
sleep 1
# kill server
killall -s INT alcmsgtest
sleep 1
TEST=`ps -cp $PID | grep alcmsgtest`
if [ -n "$TEST" ]; then
	echo "alcmsgtest did not exit within 1s"
	exit
fi
# the error log files must be empty
if [ -s log/error.log ]; then
	echo "error.log is not empty"
	exit
fi
if [ -s log/uneterr.log ]; then
	echo "uneterr.log is not empty"
	exit
fi
# the recieved file msut exist
if [ ! -e rcvmsg.raw ] ; then
	echo "recieved file doesn't exist"
	exit
fi
# compare files
TEST=`diff rcvmsg.raw $FILE`
if [ -n "$TEST" ] ; then
	echo "sent file and recieved file differ"
	exit
fi
echo "Ok, all went fine"
