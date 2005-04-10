#!/bin/bash
# $Id$
# This script 'starts' an specific game server

if [ -z "$1" ]; then
	echo "Usage ./run.sh Age [guid]"
	exit
fi

game=$1

if [[ "$1" == "avatar" || "$1" == "AC" || "$1" == "closset" || "$1" == "custom" ]]; then
	game="AvatarCustomization"
fi

if [[ "$1" == "relto" ]]; then
	game="Personal"
fi

if [[ "$1" == "hood" ]]; then
	game="Neighborhood"
fi

if [[ "$1" == "kirel" ]]; then
	game="Neighborhood02"
fi

if [[ "$1" == "BCO" ]]; then
	game="BaronCityOffice"
fi

if [[ "$1" == "phil" ]]; then #phil's relto
	game="Personal02"
fi

if [[ "$1" == "shaft" ]]; then #The Great Shaft
	game="Descent"
fi

if [[ "$1" == "GZ" ]]; then
	game="GreatZero"
fi

if [[ "$1" == "Restoration" || "$1" == "tree" || "$1" == "watcher" ]]; then
	game="RestorationGuild"
fi



res=`grep "$game " servers.list | awk '{print $1}'`

if [ -z "$res" ]; then
	echo "$game Not found!"
	exit
fi

port=`grep "$game " servers.list | awk '{print $2}'`
guid=`grep "$game " servers.list | awk '{print $3}'`

if [ -n "$2" ]; then
	guid = $2
fi

echo "Running $game with $guid at $port..."


./uru_game -p $port -guid $guid -name $game -log log/game/$game/$guid/
