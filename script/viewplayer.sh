#!/bin/sh

HURUDIR='/home/huru'

getchild()
{
  parent=$1

  mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2 |
SELECT id3 FROM ref_vault WHERE id2 = $parent;
EOF
  while read chidx; do
    res=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT type,torans FROM vault WHERE idx = $chidx;
EOF
`
    eval `echo $res | sed 's/\(.*\) \(.*\)/ntype="\1" int321="\2"/'` 
    case $LEVEL in
      1) echo -n "  " ;;
      2) echo -n "    " ;;
      3) echo -n "      " ;;
      4) echo -n "        " ;;
      5) echo -n "          " ;;
      6) echo -n "            " ;;
      7) echo -n "              " ;;
      8) echo -n "                " ;;
      9) echo -n "                  " ;;
      10) echo -n "                    " ;;
    esac
    case $ntype in
      $NODE_FOLDER|$NODE_LINK|$NODE_PLAYERINFOLIST)
        case $int321 in
          $FLDR_LINK0) echo -n "[LINK0]" ;;
          $FLDR_LINK) echo -n "[LINK]" ;;
          $FLDR_BUDDYLIST) echo -n "(BUDDYLIST)" ;;
          $FLDR_PEOPLEIKNOWABOUT) echo -n "(PEOPLEIKNOWABOUT)" ;;
          $FLDR_IGNORELIST) echo -n "(IGNORELIST)" ;;
          $FLDR_CHRONICLE) echo -n "(CHRONICLE)" ;;
          $FLDR_AVATAROUTFIT) echo -n "(AVATAROUTFIT)" ;;
          $FLDR_AGETYPEJOURNAL) echo -n "(AGETYPEJOURNAL)" ;;
          $FLDR_SUBAGES) echo -n "(SUBAGES)" ;;
          $FLDR_DEVICEINBOX) echo -n "(DEVICEINBOX)" ;;
          $FLDR_ALLPLAYERS) echo -n "(ALLPLAYERS)" ;;
          $FLDR_AGEJOURNAL) echo -n "(AGEJOURNAL)" ;;
          $FLDR_AGEDEVICES) echo -n "(AGEDEVICES)" ;;
          $FLDR_CANVISIT) echo -n "(CANVISIT)" ;;
          $FLDR_AGEOWNERS) echo -n "(AGEOWNERS)" ;;
          $FLDR_AGESIOWN) echo -n "(AGESIOWN)" ;;
          $FLDR_AGESICANVISIT) echo -n "(AGESICANVISIT)" ;;
          $FLDR_AVATARCLOSET) echo -n "(AVATARCLOSET)" ;;
          $FLDR_PLAYERINVITE) echo -n "(PLAYERINVITE)" ;;
          $FLDR_GLOBALINBOX) echo -n "(GLOBALINBOX)" ;;
          $FLDR_CHILDAGES) echo -n "(CHILDAGES)" ;;
          *) echo -n "f?$int321" ;;
        esac
        ;;
      $NODE_PLAYER) echo -n "*PLAYER*" ;;
      $NODE_AGE) echo -n "*AGE*" ;;
      $NODE_GAME) echo -n "*GAME*" ;;
      $NODE_ADMIN) echo -n "*ADMIN*" ;;
      $NODE_VAULT) echo -n "*VAULT*" ;;
      $NODE_PLAYERINFO) echo -n "*PLAYINFO*" ;;
      $NODE_SYSTEM) echo -n "*SYSTEM*" ;;
      $NODE_IMAGE) echo -n "*IMAGE*" ;;
      $NODE_TEXTNOTE) echo -n "*TEXTNOTE*" ;;
      $NODE_SDL) echo -n "*SDL*" ;;
      $NODE_CHRONICLE) echo -n "*CHRONICLE*" ;;
      $NODE_PLAYERINFOLIST) echo -n "*PLAYERINFOLIST*" ;;
      $NODE_MARKER) echo -n "*MARKER*" ;;
      $NODE_AGEINFO) echo -n "*AGEINFO*" ;;
      $NODE_MARKERLIST) echo -n "*MARKERLIST*" ;;
      *) echo -n "n?$ntype" ;;
    esac
    echo " $chidx"
    LEVEL=$((LEVEL+1)) 
    getchild $chidx
  done
  LEVEL=$((LEVEL-1)) 
}

USER=$1
LEVEL=1

[ -z "$USER" ] && echo "Usage: $0 username" && exit

. $HURUDIR/bin/hurulib.sh

playidx=`mysql -B -h$HURUDBHOST -u$HURUDBUSER -p$HURUDBPASS $HURUDBVAULT <<EOF | tail -n +2
SELECT owner FROM vault WHERE type=$NODE_PLAYERINFO AND LOWER(avie) = LOWER('$USER') LIMIT 1;
EOF
`
getchild $playidx
echo
