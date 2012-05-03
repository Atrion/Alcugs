#!/bin/bash
set -e

# You can have multiple alcugs instances on the same machine, but they must be run by different users or this script will fail!

# configuration
source ${0%/*}/control-config.sh # the file has to be in the same directory as the script

if [[ "$NOCOLOR" ]]; then
	RESULT_OK="  [\040\040\040OK\040\040\040]"
	RESULT_WARN="  [\040\040WARN\040\040]"
	RESULT_FAIL="  [\040FAILED\040]"
else
	# Borrowed from LSB init-functions
	# define the output string colors and text
	ESC=$(echo -en "\033")
	RESULT_OK="${ESC}[\061;32m${ESC}[70G[\040\040\040OK\040\040\040]${ESC}[m"
	RESULT_WARN="${ESC}[\061;33m${ESC}[70G[\040\040WARN\040\040]${ESC}[m"
	RESULT_FAIL="${ESC}[\061;31m${ESC}[70G[\040FAILED\040]${ESC}[m"
fi

getPid(){
	# check if we can find the process, running as the current user
	pgrep -u "$(whoami)" "^alcugs_$1\$" || true # pgrep returns non-0 if the process was not found
}

if [[ "$(whoami)" == "root" ]]; then
	echo "Don't run the alcugs servers as root!"
	exit
fi

case $1 in
	start)
		ulimit -c unlimited # make sure we get coredumps
		for prog in $start_servers; do
			echo -n "Starting $prog... "
			if [[ -z $(getPid $prog) ]]; then
				mkdir "$basedir/$prog" -p
				cd "$basedir/$prog"
				"$bindir/alcugs_$prog" -D -c "$config"
				RETVAL=$?
				
				if [[ ( -n "$(getPid "$prog")" ) && ( $RETVAL ) ]]; then
					echo -e $RESULT_OK
				else
					echo -e $RESULT_FAIL
				fi
				sleep "$waittime" # don't be too quick
			else
				echo -n " already running"
				echo -e $RESULT_WARN
			fi
		done
	;;
	
	stop)
		for prog in $stop_servers; do
			PID=$(getPid $prog)
			echo -n "Stopping $prog..."
			if [[ "$PID" ]]; then
				kill -INT "$PID"
				sleep "$waittime"
				i=0
				while [[ "$(getPid "$prog")" ]]; do
					echo -n "."
					if [[ "$i" -lt 2 ]]; then
						kill -INT "$PID"
					else
						kill -KILL "$PID"
					fi
					i=$[$i+1]
					sleep "$waittime"
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
			if [[ $(getPid $prog) ]]; then
				echo -n " running"
				echo -e $RESULT_OK
			else
				echo -n " NOT running"
				echo -e $RESULT_WARN
			fi
		done
	;;
	
	restart)
		"$0" stop
		"$0" start
	;;
	
	check)
		for prog in $start_servers; do
			if [[ ! "$(getPid "$prog")" ]]; then
				echo "$prog not running!"
				fail=yes
			fi
		done
		# check if we found a failure
		if [[ "$fail" ]]; then
			echo "Found a problem, restarting servers"
			"$0" restart
		fi
	;;
	
	*)
		echo "Usage: $0 {start|stop|restart|status|check}"
	;;
	
esac
