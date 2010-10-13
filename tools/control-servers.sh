#!/bin/bash
set -e

# configuration
source ${0%/*}/control-config.sh # the file has to be in the same directory as the script
export LD_LIBRARY_PATH="$bindir"

# Borrowed from LSB init-functions
# define the output string colors and text
ESC=`echo -en "\033"`
RESULT_OK="${ESC}[\061;32m${ESC}[70G[\040\040\040OK\040\040\040]${ESC}[m"
RESULT_WARN="${ESC}[\061;33m${ESC}[70G[\040\040WARN\040\040]${ESC}[m"
RESULT_FAIL="${ESC}[\061;31m${ESC}[70G[\040FAILED\040]${ESC}[m"

getPid(){
	mkdir $basedir -p
	if [[ -f $basedir/$1.pid  ]]; then
		PID=`cat $basedir/$1.pid`
		# check if the provess is still running with the same name
		if [[ ( -n "$PID" ) && ( -n `ps -p $PID | grep alcugs_$1` ) ]]; then
			echo $PID
		else
			rm $basedir/$1.pid
		fi
	else
		# check if we can find the PID without the file
		PID=`pidof $bindir/alcugs_$prog`
		if [[ $PID ]]; then
			echo $PID > $basedir/$1.pid
			echo $PID
		fi
	fi
}

if [[ "`whoami`" == "root" ]]; then
	echo "Don't run the alcugs servers as root!"
	exit
fi

case $1 in
	start)
		ulimit -c unlimited # make sure we get coredumps
		for prog in $start_servers; do
			echo -n "Starting $prog... "
			if [[ -z `getPid $prog` ]]; then
				mkdir $basedir/$prog -p
				cd $basedir/$prog
				$bindir/alcugs_$prog -D -c $config
				RETVAL=$?
				
				if [[ ( -n `getPid $prog`) && ( $RETVAL ) ]]; then
					echo -e $RESULT_OK
				else
					echo -e $RESULT_FAIL
				fi
				sleep $waittime
			else
				echo -n " already running"
				echo -e $RESULT_WARN
			fi
		done
	;;
	
	stop)
		for prog in $stop_servers; do
			unset PID
			PID=`getPid $prog`
			echo -n "Stopping $prog..."
			if [[ -n $PID ]]; then
				kill -INT $PID
				sleep 1
				i=0
				while [[ `getPid $prog` ]]; do
					echo -n "."
					if [[ $i -lt 2 ]]; then
						kill -INT $PID
					else
						kill -KILL $PID
					fi
					i=$[$i+1]
					sleep $waittime
				done
				echo -e $RESULT_OK
			else
				echo -n " not running"
				echo -e $RESULT_WARN
			fi
		done
	;;
	
	status)
		for prog in $start_servers; do
			echo -n "Checking $prog..."
			if [[ `getPid $prog` ]]; then
				echo -n " running"
				echo -e $RESULT_OK
			else
				echo -n " NOT running"
				echo -e $RESULT_WARN
			fi
		done
	;;
	
	restart)
		$0 stop
		$0 start
	;;
	
	*)
		echo "Usage: $0 {start|stop|restart|status}"
	;;
	
esac
