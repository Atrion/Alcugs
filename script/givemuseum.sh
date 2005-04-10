#!/bin/sh

HURUDIR='/home/huru'

USER=$1
DEL=$2

mode=U
[ "$USER" = "All" ] && mode=A

. $HURUDIR/bin/hurulib.sh

[ -z "$USER" ] && echo "Missing User name" && exit 1

[ "$mode" = "U" ] && echo "$USER" >$TMPF.2
[ "$mode" = "A" ] && showusers >$TMPF.2

cat $TMPF.2 | while read user; do
  echo -n "Museum for $user "
  linkptr=`getagefromplayer "$user" city P`
  res=`getfieldfromidx $linkptr data`
  # Sanity check
  isok=`echo $res | grep Ferry`
  [ -z "$isok" ] && echo "ERROR: was not a valid link file" && exit
  isok=`echo $res | grep Museum`
  if [ "$DEL" = "S" ]; then
    if [ -z "$isok" ]; then
      echo "ERROR: museum wasn't active"
    else
      new=`echo $res | sed 's/Museum\:MuseumIntStart\:\;//'`
			setfieldfromidx $linkptr data "'$new'"
      echo "suppressed"
    fi
  else
    if [ -n "$isok" ]; then
      echo "ERROR: museum already active"
    else
			setfieldfromidx $linkptr data "'Museum:MuseumIntStart:;$res'"
      echo "activated"
    fi
  fi
done


