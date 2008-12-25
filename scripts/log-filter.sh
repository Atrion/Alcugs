#!/bin/bash

# called with the parameter "errors", this script shows the full error log
# called with "warnings", it also shows warnings (and errors) from other logfiles
# called with "problems", it also shows the output of both "warnings" and "errors"
# called with "infos", it shows everything marked as information
# The 2nd parameter can be "rec" or "recursive" to tell the script to search recursively in subdirectories (useful for game server logs)

catfiles(){
	if [ -f "$1.1.log" ]; then
		ls -r $1.*.log | while read file; do
			cat $file
		done
	fi
       cat $1.log
}

logfilter(){
	echo "General alcugs log:"
	catfiles "alcugs" | grep -Ehi "$1"
	if [ -f tracking.log ]; then
		echo
		echo "Tracking log:"
		catfiles "tracking" | grep -Ehi "$1"
	fi
	if [ -f auth.log ]; then
		echo
		echo "Auth log:"
		catfiles "auth" | grep -Ehi "$1"
	fi
	if [ -f vault.log ]; then
		echo
		echo "Vault log:"
		catfiles "vault" | grep -Ehi "$1"
	fi
	if [ -f agestate.log ]; then
		echo
		echo "Age state log:"
		catfiles "agestate" | grep -Ehi "$1"
	fi
}

oldPwd="`pwd`"

if [[ "$2" == "rec" || "$2" == "recursive" ]]; then
	if [ ! -f alcugs.log ]; then # no log found so far, go on searching
		if `ls -d */ &> /dev/null`; then
			ls -d */ | while read dir; do
				cd "$oldPwd/$dir"
				$0 $1 $2
			done
		fi
		cd $oldPwd
		exit
	fi
	echo "============================================================================"
	echo "`pwd`:"
fi

if [ ! -f alcugs.log ]; then
	if [ -f log/alcugs.log ]; then
		cd log
	else
		echo "No alcugs log found in current directory"
		exit
	fi
fi

case $1 in
	problem|problems)
		$0 errors
		$0 warnings
	;;
	error|errors)
		catfiles "error"
		echo
		if [ -f fork_err.log ]; then
			echo "Fork error log"
			cat fork_err.log
			echo
		fi
	;;
	warning|warnings)
		logfilter "warn|[^a-z]err|unx|unexpect|fatal"
	;;
	info|infos)
		logfilter "[^e]\sinf"
	;;
	*)
		echo "Usage: $0 {errors|warnings|problems|infos}"
	;;
esac

cd $oldPwd
