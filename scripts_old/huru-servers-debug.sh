#!/usr/bin/env bash
#
# chkconfig: 345 95 95
# description: controls huru servers
#
# Original script by Striker for Plasmaservers
# Altered to cope with the H'uru servers by Sjaak-de-Daak
# Some minor changes has been done to work with the installer.

# The next line contains information that will be used by
# the installer.
# ____HEAD____

### EDIT THESE ##########
# Full Path to H'uru Root:
HURU_ROOT=/home/huru/uru_server3__build_004_runtime
# User to run servers as. (root == bad)
RUNAS=huru

# ____BODY____
# The above line contains information that will be used by
# the installer.
#
# Any text, commands, comentaries between _HEAD_ and _BODY_ will
# be ignored and replaced with custom HURU_ROOT and RUNAS options
#
# Time to sleep after starting a server
SLEEP=3

# Org. credits:
# Thanks to Striker for writing this script. -chip
# Thanks to Sjaak-de-Draak for altering this script. -almlys :p

PATH=${HURU_ROOT}/bin:$PATH:/sbin

BASE_DIR=${HURU_ROOT}/var
ETC=${HURU_ROOT}/etc
SERVER_LIST=${HURU_ROOT}/var/servers.list
RETVAL=0

start(){
    if [[ "$RUNAS" == "root" ]]; then
        echo -e "ERROR: Cannot run as root$RESULT_FAIL"
        RETVAL=1
        return
    fi
    for prog in `cat ${SERVER_LIST}`; do
        echo -n $"Starting $prog: "
        pushd $BASE_DIR  > /dev/null 2>&1
        ulimit -c unlimited

        if [[ -a ${BASE_DIR}/$prog.pid  ]]; then
            PID=`cat  ${BASE_DIR}/$prog.pid`
            if [[ -n `ps -cp $PID | grep uru_${prog}` ]]; then
                echo -e "Already running!$RESULT_WARN"
                RETVAL=1
                continue
            else
                echo -n "Stale PID file."
            fi
        fi

        if [[ "`whoami`" == "root" ]]; then
# Use this line for almost all linux distro's, otherwise use: su $RUNAS -p -c 
            su $RUNAS -f -c \
            "cd ${HURU_ROOT}/$prog ; uru_${prog} -D -c ${ETC}/uru.conf >>${HURU_ROOT}/var/log/${prog}/stderr.log 2>&1; echo \$? > /tmp/$$.retval" >/dev/null
        else
            ( cd ${HURU_ROOT}/$prog ; uru_${prog} -D -c ${ETC}/uru.conf >>${HURU_ROOT}/var/log/${prog}/stderr.log 2>&1 )
            echo $? > /tmp/$$.retval
	    cd $BASE_DIR
        fi

        [ `cat /tmp/$$.retval` == 0 ] && echo -en $RESULT_OK || echo -en $RESULT_FAIL
        rm -f /tmp/$$.retval

	# hmm, looks like we'll have to create our own PID file(s)
        PID=`pidof uru_${prog} | awk '{ print $NF }'`
        echo $PID > ${BASE_DIR}/$prog.pid

	# sleep for a while
	sleep $SLEEP

        popd  > /dev/null 2>&1
        echo
    done
}

stop(){
		TAC=`which tac 2>&1`
		[ $? -eq 1 ] && TAC="cat"
    for prog in `$TAC ${SERVER_LIST}`; do
        echo -n "Stopping $prog: "
        if [[ -a ${BASE_DIR}/$prog.pid  ]]; then
            PID=`cat  ${BASE_DIR}/$prog.pid`
        else
            echo -n "PID File missing "
            PID=`pidof uru_${prog} | awk '{ print $NF }'`
            if [ -z "$PID" ]; then
                echo -e "and proc not found.$RESULT_WARN"
                RETVAL=1
                continue
            fi
        fi
        kill -TERM $PID
        sleep 1

        iter=0
        while [[ -n `ps -cp $PID | grep uru_${prog}` && "$iter" -lt "4" ]]; do
            echo -n '.'
            sleep 1
            kill -KILL $PID 2>/dev/null
            iter=$[$iter+1]
        done
        if [[ -n `ps -cp $PID | grep uru_${prog}` ]]; then
            echo -en $RESULT_FAIL
        else
            echo -en $RESULT_OK
            rm -f ${BASE_DIR}/$prog.pid
        fi
        echo
    done
    return
}

restart(){
    stop
    start
}

reload(){
    for prog in `cat ${SERVER_LIST}`; do
        echo -n "Reloading conf for $prog: "
        if [[ -a ${BASE_DIR}/$prog.pid  ]]; then
            PID=`cat  ${BASE_DIR}/$prog.pid`
						kill -HUP $PID
            echo -e $RESULT_OK
        else
            echo -n "PID File missing, can't reload "
            echo -e $RESULT_FAIL
				fi
		done
}

status(){
    # crazy voodoo magic!
    for prog in `cat ${SERVER_LIST}`; do
        echo -n "$prog: "
        if [[ -a ${BASE_DIR}/$prog.pid  ]]; then
            PID=`cat  ${BASE_DIR}/$prog.pid`
            if [[ -z `ps -cp $PID | grep uru_${prog}` ]]; then
                echo -e "Stale PID file!$RESULT_FAIL"
                RETVAL=1
            else
            	if [[ ! -z `ps -cA | grep uru_${prog} | grep '<defunct>'` ]]; then
            	DEFUNCT=`ps -cA | grep uru_${prog} | grep '<defunct>' |cut -f 1 -d " "`
			echo -e "<defunct> server! PID: $DEFUNCT $RESULT_WARN"
		else
                	echo -e "Running, PID: $PID$RESULT_OK"
		fi
            fi
        else
            echo -en "PID file missing"
            PID=`pidof uru_${prog} | awk '{ print $NF }'`
            if [ -n "$PID" ]; then
                echo -en "; PID Found: $PID$RESULT_WARN"
            else
                echo -en "; Not running.$RESULT_FAIL"
            fi
            RETVAL=1
            echo
        fi
    done
    return
}


fix(){
    # crazy voodoo magic!
    for prog in `cat ${SERVER_LIST}`; do
        echo -n "$prog: "
        if [[ -a ${BASE_DIR}/$prog.pid  ]]; then
            PID=`cat  ${BASE_DIR}/$prog.pid`
            if [[ -z `ps -cp $PID | grep uru_${prog}` ]]; then
                echo -e "Stale PID file!$RESULT_FAIL"
                RETVAL=1
            else
            	if [[ ! -z `ps -cA | grep uru_${prog} | grep '<defunct>'` ]]; then
            	DEFUNCT=`ps -cA | grep uru_${prog} | grep '<defunct>' |cut -f 1 -d " "`
			echo -e "<defunct> server! PID: $DEFUNCT $RESULT_WARN"
			# trying to instantly fix the situation
			kill -9 $PID
			echo -e "I KILLED the Parent PID: $PID"
			return 1
		else
                	echo -e "Running, PID: $PID$RESULT_OK"
		fi
            fi
        else
            echo -en "PID file missing"
            PID=`pidof uru_${prog} | awk '{ print $NF }'`
            if [ -n "$PID" ]; then
                echo -en "; PID Found: $PID$RESULT_WARN"
            else
                echo -en "; Not running.$RESULT_FAIL"
            fi
            RETVAL=1
            echo
        fi
    done
    return
}

# Borrowed from LSB init-functions
# define the output string colors and text
        ESC=`echo -en "\033"`
  RESULT_OK="${ESC}[\061;32m${ESC}[70G[\040\040\040OK\040\040\040]${ESC}[m"
RESULT_FAIL="${ESC}[\061;31m${ESC}[70G[\040FAILED\040]${ESC}[m"
RESULT_WARN="${ESC}[\061;33m${ESC}[70G[\040\040WARN\040\040]${ESC}[m"

# See how we were called.
case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    reload)
        reload
        ;;
    status)
        status
        ;;
    fix)
        fix
	if [ "$?" == "1" ] ; then
	  echo "ok, autostarting "
	  echo 
	  start
	else
	  echo 
	  echo "Good, no need to fix anything "
	  echo 
	fi
        ;;
    *)
        echo "Usage: $0 {status|start|stop|restart|fix}"
        RETVAL=1
esac

exit $RETVAL
