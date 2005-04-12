#!/bin/sh

HURUDIR='/home/huru'

USER=$1

mode=U
[ "$USER" = "All" ] && mode=A

. $HURUDIR/bin/hurulib.sh

[ -z "$USER" ] && echo "Missing User name" && exit 1

[ "$mode" = "U" ] && echo "$USER" >$TMPF.2
[ "$mode" = "A" ] && showusers >$TMPF.2

AGES=`getallagenames`

cat $TMPF.2 | while read user; do
  echo "Checking $user "
	for agename in $AGES
	do
    ageid=`getagefromplayer "$user" $agename C`
		if [ -n "$ageid" ]; then
			guid=`getfieldfromidx $ageid guid`	
			ispriv=`expr substr $guid 2 4`
		  if [ "$ispriv" = "0000" ]; then
			  echo -n "   "
			else
			  echo -n "  P"
			fi
	    echo "  $agename is $ageid "
	  fi
	done
done

rm -f $TMPF.2

