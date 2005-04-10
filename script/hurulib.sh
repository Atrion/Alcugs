#!/bin/sh

# IMPORTANT: please make sure your database names keep their prefix (vault*,pls*,auth*).

HURUDIR="/home/huru"		# UntilUru installation directory
BCKDIR="/home/backups"		# Backups directory

# Don't edit from this point on
TMPF="/tmp/`basename $0`.$$"

[ ! -d $HURUDIR/tmp ] && mkdir -p $HURUDIR/tmp
config="$HURUDIR/tmp/uru.conf.expanded"

# Node Types
NODE_PLAYER=2
NODE_AGE=3
NODE_GAME=4
NODE_ADMIN=5
NODE_VAULT=6
NODE_FOLDER=22
NODE_PLAYERINFO=23
NODE_SYSTEM=24
NODE_IMAGE=25
NODE_TEXTNOTE=26
NODE_SDL=27
NODE_LINK=28			# ->NODE_AGEINFO
NODE_CHRONICLE=29
NODE_PLAYERINFOLIST=30		# same as NODE_FOLDER
NODE_MARKER=32
NODE_AGEINFO=33			#
NODE_MARKERLIST=35			# same as NODE_FOLDER but holds FLDR_BUDDYLIST

# Folder Types
FLDR_LINK0=0			# FLDR_*->self ????? 	(Unlocked LINK)
FLDR_LINK=1			# FLDR_*->self		(Locked LINK)
FLDR_BUDDYLIST=2		# from nodetype=NODE_MARKERLIST
FLDR_PEOPLEIKNOWABOUT=4		# NODE_AGE->self
FLDR_IGNORELIST=3		# NODE_PLAYERINFO->self
FLDR_CHRONICLE=6		# NODE_PLAYERINFO->self->NODE_CHRONICLE
FLDR_AVATAROUTFIT=7		# NODE_PLAYERINFO->self->NODE_SDL
FLDR_AGETYPEJOURNAL=8		# FLDR_AGEJOURNAL->self-> ?
FLDR_SUBAGES=9			# NODE_AGE->self
FLDR_DEVICEINBOX=10		# NODE_TEXTNOTE->self->NODE_IMAGE(m)
FLDR_ALLPLAYERS=12		# root->self->NODE_PLAYERINFO(m)
FLDR_AGEJOURNAL=14		# NODE_PLAYERINFO->self->FLDR_AGETYPEJOURNAL(m)
FLDR_AGEDEVICES=15		# NODE_AGE->self->NODE_TEXTNOTE->FLDR_DEVICEINBOX->NODE_IMAGE(m)
FLDR_CANVISIT=18		# NODE_AGEINFO->self->NODE_PLAYERINFO(m)
FLDR_AGEOWNERS=19		# NODE_AGEINFO->self->NODE_PLAYERINFO(m)
FLDR_AGESIOWN=23		# NODE_PLAYERINFO->self->NODE_LINK(m)
FLDR_AGESICANVISIT=24		# NODE_PLAYERINFO->self->NODE_LINK(m)
FLDR_AVATARCLOSET=25		# NODE_PLAYERINFO->self->NODE_SDL(m)
FLDR_PLAYERINVITE=28		# NODE_PLAYER->self-> ?
FLDR_GLOBALINBOX=30		# NODE_SYSTEM->self->NODE_TEXTNOTE
FLDR_CHILDAGES=31

get_config()
{
  FILE=$1
  section="[global]"

  [ ! -f $FILE ] && return
  rm -f $config
  cat $FILE |
  while read line; do
    type=`echo $line | grep "^\[.*\].*"`
    if [ -n "$type" ]; then
      section=$type
    else
      echo "$section $line" >>$config
    fi
  done
}

read_config()
{
  FILE=$1
  OPTION=$2
  SECTION=$3

  [ ! -f $FILE ] && echo ""
  line=`grep "^\[$SECTION\] ${OPTION}[  ]*=" $FILE`
  [ -z "$line" ] && echo ""
  res=`echo $line | awk -F"=" '{ print $2 }'`
  echo $res
}

get_default()
{
  local res=""

  OPTION=$1
  SECTION=$2

  #name=\$"$1"
  #def=`eval "expr \"$name\" "`
  #echo "default $OPTION=$def"

  res=`read_config $config $OPTION $SECTION`
  if [ -n "$res" ]; then
    eval "$OPTION=$res"
  else # now try global if couldn't find in specific section
    if [ "$SECTION" != "global" ]; then
      res=`read_config $config $OPTION global`
      [ -n "$res" ] && eval "$OPTION=$res"
    fi
  fi
}

updtimestamp()
{
  NOW=`date +%s`
	NANO=`date +%N`
	MICRO=`expr $NANO / 1000`

  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
UPDATE vault SET timestamp=$NOW, microseconds=$MICRO WHERE idx=$1;
EOF
`
}

getplayer()
{
  USER=`echo "$1" | sed "s/'/\\\\\'/g"` # backslash quotes for mysql
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT idx FROM vault WHERE type=$NODE_PLAYER AND LOWER(avie) = LOWER('$USER') LIMIT 1;
EOF
`
  echo $res
}

getplayerinfo()
{
  USER=`echo $1 | sed "s/'/\\\\\'/g"` # backslash quotes for mysql
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT idx FROM vault WHERE type=$NODE_PLAYERINFO AND LOWER(avie) = LOWER('$USER') LIMIT 1;
EOF
`
echo $res
}

getagesiown()
{
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT idx FROM vault WHERE owner=$1 AND type=$NODE_FOLDER AND torans=$FLDR_AGESIOWN LIMIT 1;
EOF
`
echo $res
}

getagefromplayer()
{
  playid=`getplayer "$1"`
  age=`echo $2 | sed "s/'/\\\\\'/g"` # backslash quotes for mysql
  parent=$3
  [ -z "$playid" -o -z "$age" -o -z "$parent" ] && return
  case $parent in
    P) parent="r.id2" ;;
    C) parent="r.id3" ;;
    *) echo "Bad parent parameter"; return ;;
  esac
  agesiown=`getagesiown $playid`
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT id3 FROM ref_vault WHERE id2=$agesiown;
EOF
`
  inarg=`echo $res | sed 's/ /,/g'`
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT $parent FROM ref_vault AS r LEFT JOIN vault AS v ON v.idx=r.id3 WHERE r.id2 IN ($inarg) AND LOWER(entry_name)=LOWER('$age');
EOF
`
  echo $res
}

getallagenames()
{
	res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT DISTINCT(entry_name) FROM vault WHERE type=$NODE_AGEINFO ORDER BY entry_name ASC;
EOF
`
	echo $res
}

getneighbname()
{
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT owner_name FROM vault WHERE idx=$1 LIMIT 1;
EOF
`
  echo $res
}

getplayerfolder()
{
  playid=`getplayer $1`
  [ -z "$playid" ] && return
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT idx FROM vault WHERE iwner=$playid AND type=$NODE_FOLDER AND torans=$2 LIMIT 1;
EOF
`
  echo $res
}

getplayerchronicle()
{
  playid=`getplayer $1`
  [ -z "$playid" ] && return
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT idx FROM vault WHERE owner=$playid AND type=$NODE_CHRONICLE AND LOWER(entry_name)=LOWER('$2') LIMIT 1;
EOF
`
  echo $res
}

getfieldfromidx()
{
  res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT $2 FROM vault WHERE idx=$1 LIMIT 1;
EOF
`
  echo $res
}

setfieldfromidx()
{
  IDX=$1
  FIELD=$2
	VALUE=$3
	cond=""
	if [ "$FIELD" = "data" ]; then
	  len=`expr length "$VALUE"`
		len=`expr $len - 2`	# get rid of quotes
		cond=", data_size=$len"
	fi
res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
UPDATE vault SET $FIELD=$VALUE $cond WHERE idx=$IDX;
EOF
`
	updtimestamp $IDX
}

showusers()
{
mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT avie FROM vault WHERE type=$NODE_PLAYERINFO AND owner !=0
ORDER BY avie ASC;
EOF
}

get_config $HURUDIR/etc/uru.conf

get_default db_username vault
get_default db_passwd vault
get_default db_server vault
get_default db_name vault

HURUDBUSER=$db_username
HURUDBPASS=$db_passwd
HURUDBHOST=$db_server
HURUDBVAULT=$db_name
HURUDBS=`grep db_name $HURUDIR/etc/uru.conf | awk -F= '{ print $2 }'`

