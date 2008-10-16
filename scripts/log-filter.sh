#!/bin/sh

# called with the parameter "problems", this script shows the full error log as well as all warnings, errors etc. printed in other log files
# called with "infos", it shows everything marked as information

logfilter(){
	echo "General alcugs log:"
	grep -Ehi "$1" alcugs.4.log alcugs.3.log alcugs.2.log alcugs.1.log alcugs.log
	if [ -f tracking.log ]; then
		echo
		echo "Tracking log:"
		grep "$1" tracking.4.log tracking.3.log tracking.2.log tracking.1.log tracking.log
	fi
	if [ -f auth.log ]; then
		echo
		echo "Auth log:"
		grep "$1" auth.4.log auth.3.log auth.2.log auth.1.log auth.log
	fi
	if [ -f vault.log ]; then
		echo
		echo "Vault log:"
		grep "$1" vault.4.log vault.3.log vault.2.log vault.1.log vault.log
	fi
}

oldPwd="`pwd`"
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
		echo "General errors:"
		cat error.4.log error.3.log error.2.log error.1.log error.log
		echo
		if [ -f fork_err.log ]; then
			echo "Fork error log"
			cat fork_err.log
			echo
		fi
		logfilter "warn|[^a-z]err|unx|unexpect|fatal"
	;;
	info|infos)
		logfilter "[^e]\sinf"
	;;
	*)
		echo "Usage: $0 {problems|infos}"
	;;
esac

cd $oldPwd
