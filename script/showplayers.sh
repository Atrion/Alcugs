#!/bin/sh

MODE=$1

[ -z "$MODE" ] && echo "Usage: $0 [I/O/A] (In, Out, All)" && exit
[ "$MODE" = "I" ] && cond="AND n1.torans = 1"
[ "$MODE" = "O" ] && cond="AND n1.torans = 0"
[ "$MODE" = "A" ] && cond=""

HURUDIR='/home/huru'
. $HURUDIR/bin/hurulib.sh

mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT n1.idx,n1.avie,n2.uid,n1.entry_name FROM vault AS n1
LEFT JOIN vault AS n2 ON n1.owner=n2.idx
WHERE n1.type=$NODE_PLAYERINFO $cond AND n1.owner !=0
ORDER BY n2.uid;
EOF
