#!/bin/bash
# /*******************************************************************************
# *    Alcugs H'uru server                                                       *
# *                                                                              *
# *    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
# *    See the file AUTHORS for more info about the team                         *
# *                                                                              *
# *    This program is free software; you can redistribute it and/or modify      *
# *    it under the terms of the GNU General Public License as published by      *
# *    the Free Software Foundation; either version 2 of the License, or         *
# *    (at your option) any later version.                                       *
# *                                                                              *
# *    This program is distributed in the hope that it will be useful,           *
# *    but WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
# *    GNU General Public License for more details.                              *
# *                                                                              *
# *    You should have received a copy of the GNU General Public License         *
# *    along with this program; if not, write to the Free Software               *
# *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
# *                                                                              *
# *    Please see the file COPYING for the full license.                         *
# *    Please see the file DISCLAIMER for more details, before doing nothing.    *
# *                                                                              *
# *                                                                              *
# *******************************************************************************/
#
# Installation script
# This script, will create the required tree for the correct server operation
#
# TODO: permissions, and check if we are the ROOT user, parse the old files
# if them exist
#
# $Id$
#

#script version
ver="1.4"

#ask for metaserver?
ask_meta=1

if [ -z "$1" ]; then
	echo "Usage: ./install.sh [devel | normal] [location-path]"
	exit
fi

## available options
# auto - Fork on demand?
# max_players - (Don't touch, leave it big, or you will have problems due to a bug)

if [ "$1" == "devel" ]; then
	#set here optimal options for a development install
	auto=0
	max_players=200
	ask_meta=0
elif [ "$1" == "normal" ]; then
	#set here optimal options for a normal install
	auto=1
	max_players=300
else
	echo "Usage: ./install.sh [devel | normal] [location-path]"
	exit
fi

if [ -n "$2" ]; then
	path=$2
	if [ "$1" == "devel" ]; then
		echo "The devel installation can't be installed outside the source tree, use the normal one instead"
		exit
	fi
else
	path="./"
fi

valid=0; #validated by the user

while [ $valid -ne 1 ];
do

	doet=0

	#1st Path
	while [ $doet -ne 1 ];
	do
		echo "##################################################"
		echo "Alcugs H'uru servers automatic installation script"
		echo " Script version: $ver "
		echo ""
		if [[ "`whoami`" == "root" ]]; then
			echo "Note: You are running this script with root privilegues"
			echo "This script is still not finished/verified to be run as root"
			echo "Please run the script as the same user that is going to run the servers"
			exit
		else
			runuser=`whoami`
			echo "The script has configured the default user to run the servers as $runuser."
			echo "You can change it at any moment editing the startup script"
		fi
		echo "Installing servers at: $path"
		echo "Are you sure? [Y/n]:"
		read res

		if [[ "$res" == "Y"  || "$res" == "y" ]]; then
			echo "Servers will be installed at: $path"
			doet=1
		elif [[ "$res" == "N"  || "$res" == "n" ]]; then
			echo "Please type ./install $1 /where/you/want/to/install/the/servers"
			doet=1
			exit
		else
			echo "Please, Enter a valid choice..."
			doet=0
		fi
	done

	#2nd Cluster
	echo ""
	echo "__________________________________________________________________"
	echo "** Cluster Installation **"
	echo "Note: At current time, each node MUST have his own Public address"

	doet=0

	while [ $doet -ne 1 ];
	do
		echo "Are you doing a cluster installation? [Y/n]:"
		read res

		if [[ "$res" == "Y"  || "$res" == "y" ]]; then
			echo "Doing cluster installation..."
			cluster=1
			doet=1
		elif [[ "$res" == "N"  || "$res" == "n" ]]; then
			echo "Doing single installation..."
			cluster=0
			doet=1
		else
			echo "Please, Enter a valid choice..."
			doet=0
		fi
	done

	#if cluster then ask servers to install
	if [ $cluster -eq 1 ]; then

		echo ""
		echo "__________________________________________________________________"
		echo "** Setting up the Auth Server **"
		echo "Note: You are able to run several auth servers at different machines"
		echo "      the auth server needs access to auth.plasma.corelands.com:80 for"
		echo "      end user validation."
		echo "Do you want to run the auth server at this machine? [Y/n]"
		read res
		if [[ "$res" == "N"  || "$res" == "n" ]]; then
			auth=0
		else
			auth=1
		fi

		echo ""
		echo "__________________________________________________________________"
		echo "** Setting up the Vault Server **"
		echo "Note: You MUST run only ONE vault server"
		echo "Do you want to run the vault server at this machine? [Y/n]"
		read res
		if [[ "$res" == "N"  || "$res" == "n" ]]; then
			vault=0
		else
			vault=1
		fi

		echo ""
		echo "__________________________________________________________________"
		echo "** Setting up the Tracking Server **"
		echo "Note: You MUST run only ONE tracking server"
		echo "Do you want to run the tracking server at this machine? [Y/n]"
		read res
		if [[ "$res" == "N"  || "$res" == "n" ]]; then
			track=0
		else
			track=1
		fi

		echo ""
		echo "__________________________________________________________________"
		echo "** Setting up the Lobby/Game(s) Server(s) **"
		echo "Note: You are able to run several game servers at different machines"
		echo "Do you want to run game(s) server(s) at this machine? [Y/n]"
		read res
		if [[ "$res" == "N"  || "$res" == "n" ]]; then
			lobby=0
		else
			lobby=1
		fi

	else
		auth=1;
		vault=1;
		track=1;
		lobby=1;
	fi

  single_conf=0
	conf_path="uru.conf"
	if [ $cluster -eq 0 ]; then
	  echo "Do you want to generate multiple uru.conf ? (y/n default n)"
		read ans
		case "$ans" in
		  y|Y)	echo "Ok, now try to maintain multiple configuration files :)"
					  single_conf=0
			;;
			*)	echo "Ok, only one uru.conf will be generated in $path/etc folder"
					  single_conf=1
						conf_path="etc/uru.conf"
						mkdir -p $path/etc
			;;
		esac
	fi

	#now ask the global options
	echo ""
	echo "__________________________________________________________________"
	echo "** GLOBAL OPTIONS **"
	echo "Please set the verbose level used in the log files:"
	echo " * 3-> print all messages to the stdout (default)"
	echo " * 2-> print only informative and error messages"
	echo " * 1-> print only error messages"
	echo " * 0-> total silence, perfect for daemon mode"
	echo "Select a level 0,1,2,3:"
	read verbose_level

	echo "Server Bandwidth, type your uplink in kilobits per second (kbps):"
	read bandwidth

	echo "Please enter server public address: (type the private one if the server is not"
	echo " public)"
	read public_ip

	echo "Please enter the private address: (enter the public if there isn't any private"
	echo " address)"
	read private_ip

	#set the bind address. Note, if there are multiple interfaces or ip's,
	# clients placed inside the LAN will use the default address
	# BUG: server binds to 172.26.0.10 and 172.26.0.11, default interface
	# is 172.26.0.10, client conects to 172.26.0.11, server will answer
	# from 172.26.0.10 and then the client will reject the message because
	# it's comming from another address.
	# TODO: Allow several "bind = address" entryes.
	if [ -z "$private_ip" ]; then
		private_ip="0.0.0.0"
	fi

	echo "Please enter the private network mask: (For example 255.255.255.0 if you have a"		echo " class C network, enter 255.255.255.255 if there isn't any private network and"
	echo " 0.0.0.0 if there isn't public access to the server)"
	read private_mask

	echo "Please enter the website address that will be displayed when a client logs in:"
	read website

	echo "DataSet: (The type of clients that you want, default 5)"
	echo "  0 -> custom"
	echo "  1 -> prime '10-02-2003 Branch - Built 10/9/2003 at 6:53 A'"
	echo "  2 -> prime12 '10-24-2003 Branch - Built 11/12/2003 at 1:12 PM'"
	echo "  3 -> live/UU 'Build37 Branch - Built 7/30/2004 at 2:38 PM'"
	echo "  4 -> todni 'Exp1 Branch - Built 3/9/2004 at 11:39 AM'"
	echo "  5 -> tpots 'Main Branch - Built 5/26/2004 at 5:19 PM'"
	echo "  6 -> tpots2 'Alcugs H'uru official client patch distro for tpots'"
	echo "  7 and above -> '3rd party client patch distros' (each group working on a client distro MUST use an unique DataSet id)"
	read dataset
	[ -z "$dataset" ] && dataset=5

  echo
	echo "Please enter location of your sdl folder (relative to $path, default sdl)"
	read sdlfolder
	[ -z "$sdlfolder" ] && sdlfolder="sdl"
	if [ ! -e "$path/sdl" ]; then
		mkdir $path/sdl
	fi
	echo "Please copy here all the *.sdl files from the client dataset id: $dataset">$path/sdl/README

  echo
	echo "Please enter location of your age folder (relative to $path, default age)"
	read agefolder
	[ -z "$agefolder" ] && agefolder="age"
	if [ ! -e "$path/age" ]; then
		mkdir $path/age
	fi
	echo "Please copy here all the *.age files from the client dataset id: $dataset">$path/age/README
	echo " or delete the folder, and symlink them ln -s ../lobby/age">>$path/age/README

	#summary

	echo ""
	echo "##############################################################"
	echo "Summary:"
	echo "Path: $path"
	echo "Type: $1"
	echo "Auth: $auth, Tracking: $track, Vault: $vault, Games: $lobby"
	echo "Verbose Level: $verbose_level"
	echo "Bandwidth: $bandwidth"
	echo "Public address: $public_ip"
	echo "Private address: $private_ip"
	echo "Private MASK: $private_mask"
	echo "Website: $website"
	echo "dataset: $dataset"
	echo "sdl folder: $path/$sdlfolder"
	echo "age folder: $path/$agefolder"
	echo "##############################################################"


	doet=0;

	while [ $doet -ne 1 ];
	do

		echo "Are you sure that these settings are correct? [Y/n]:"
		read res

		if [[ "$res" == "Y"  || "$res" == "y" ]]; then
			valid=1
			doet=1
		elif [[ "$res" == "N"  || "$res" == "n" ]]; then
			valid=0
			doet=1
		else
			echo "Please, Enter a valid choice..."
			doet=0
		fi

	done


done

#metaserver

meta=0
valid=0

if [ $ask_meta -eq 1 ]; then

	while [ $valid -ne 1 ];
	do

		doet=0

		while [ $doet -ne 1 ];
		do
			echo ""
			echo "** Metaserver **"
			echo "The metaserver stores a list of all public H'uru servers."
			echo "Players will be able to easily locate it and check if it is running."
			echo "To register your shard as a public shard in the metaserver, you need to have"
			echo " the required ports (5000-6000) open to the public."
			echo "Also, you are going to recieve a packet from almlys.dyns.net every 15-30"
			echo " minutes to check if your servers are up and running. Dead servers will be"
			echo " automatically removed in about 48-72 hours. If you did a mistake and you"
			echo " wish to immediatly delete your server from the public list, you will need to"
			echo " manually delete it via the supplyed password, or contacting the maintainer."
			echo " Public servers list is available at: http://alcugs.almlys.dyns.net/servers.php"
			echo ""
			echo "Do you want to register your shard to the public list? [Y/n]:"
			read res

			if [[ "$res" == "Y"  || "$res" == "y" ]]; then
				echo "Your shard will be listed as a public server"
				doet=1
				meta=1
			elif [[ "$res" == "N"  || "$res" == "n" ]]; then
				echo "Your shard will remain private"
				doet=1
				meta=0
				valid=1
			else
				echo "Please, Enter a valid choice..."
				doet=0
			fi
		done

		if [ $meta -eq 1 ]; then

			echo ""
			echo "The next information is required to register your shard in the public list"
			echo ""
			echo "Shard Name: (your shard name)"
			read shard_name
			echo "Shard Website: (url to the website of your shard)"
			read shard_website
			echo "Shard Description: (small description about your shard)"
			read shard_description
			echo "Administrative contact: (url or E-mail to contact with the admin, note that is going to be placed in a public website, so the mailto:me AT myserver DOT org notation is recomended to avoid spam, or use a url to a forum)"
			read shard_contact
			echo "Deletion Password: (If you want to delete the shard from the public list, you will be asked for this password. IMPORTANT!, the password is stored in plain text in the database and may be visible to the metaserver administrators)"
			read shard_password

			#summary

			echo ""
			echo "##############################################################"
			echo "Summary:"
			echo "Shard Name: $shard_name"
			echo "Shard Website: $shard_website"
			echo "Shard Description: $shard_description"
			echo "Administrative contact: $shard_contact"
			echo "Deletion Password: $shard_password"
			echo "##############################################################"

			doet=0;

			while [ $doet -ne 1 ];
			do

				echo "Are you sure that these settings are correct? [Y/n]:"
				read res

				if [[ "$res" == "Y"  || "$res" == "y" ]]; then
					valid=1
					doet=1
				elif [[ "$res" == "N"  || "$res" == "n" ]]; then
					valid=0
					doet=1
				else
					echo "Please, Enter a valid choice..."
					doet=0
				fi

			done
		fi
	done
fi

mkdir -p $path

current=`pwd`
cd $path

destination=`pwd`
mkdir -p bin var etc

if [ $single_conf -eq 1 ]; then
  conf_path=$destination/$conf_path
fi

rm -f var/servers.list
touch var/servers.list

#now ask the per server settings
if [ $auth -eq 1 ]; then

	echo "auth" >> $destination/var/servers.list

	if [ ! -e "auth" ]; then
		mkdir auth
	fi
	cd auth
	if [ "$1" == "devel" ]; then
		if [ ! -e "uru_auth" ]; then
			ln -s ../uru_auth
		fi
	else
		cp -f "$current/uru_auth" $destination/bin
	fi


	valid=0

	while [ $valid -ne 1 ];
	do

		echo ""
		echo "__________________________________________________________________"
		echo "** Setting up the Auth Server **"
		echo "The auth server needs access to a MySql database, you need to create a user with permissions to create databases, and to be able to create/delete/modify tables in the created databases"
		echo "Please set the MySQL host: (leave blank to use a socket)"
		read auth_db_host
		echo "Please set the MySQL Auth user:"
		read auth_db_username
		echo "Please set the MySQL Auth password:"
		read auth_db_password
		echo "Please set the MySQL Auth DatabaseName:"
		read auth_db_name

		echo
		echo "Do you want to restrict server access to already known accounts (from accounts table) ? (y/n) default n"
		read ans
		case "$ans" in
		  y|Y)	echo "Ok, server will be in restricted mode"
					  allow_unknown_accounts=0
			;;
			*)	echo "Ok, leaving your server opened, which is the default behavior"
					  allow_unknown_accounts=1
			;;
		esac

		echo
		echo "Do you want to stop automatically filling accounts table ? (y/n, default n)"
		read ans
		case "$ans" in
		  y|Y)	echo "Ok, we're not going to fill in accounts table"
					  auto_register_account=0
			;;
			*)	echo "Ok, we're filling accounts table, which is the default behavior"
					  auto_register_account=1
			;;
		esac

		echo ""
		echo "########## Auth Database #############################"
		echo "Host: $auth_db_host"
		echo "Username: $auth_db_username"
		echo "Password: $auth_db_password"
		echo "Database: $auth_db_name"
		echo "######################################################"
		echo "Restricted mode: $allow_unknown_accounts"
		echo "Auto-fill accounts: $auto_register_account"

		doet=0;

		while [ $doet -ne 1 ];
		do

			echo "Are these setting OK? [Y/n]:"
			read res

			if [[ "$res" == "Y"  || "$res" == "y" ]]; then
				valid=1
				doet=1
			elif [[ "$res" == "N"  || "$res" == "n" ]]; then
				valid=0
				doet=1
			else
				echo "Please, Enter a valid choice..."
				doet=0
			fi

		done

	done

  rm -f uru.conf
	echo "# This is an autogenerated file for the Auth server">>$conf_path

	if [ -n "$verbose_level" ]; then
		echo "verbose_level = $verbose_level">>$conf_path
	fi
# 	if [ -n "$bandwidth" ]; then
# 		echo "bandwidth = $bandwidth">>$conf_path
# 	fi
	echo "auth_server_port = 2010">>$conf_path

	if [ -n "$private_ip" ]; then
		echo "auth_server = $private_ip">>$conf_path
	fi

	if [ -n "$auth_db_host" ]; then
		echo "db_server = $auth_db_host">>$conf_path
	fi
	if [ -n "$auth_db_name" ]; then
		echo "db_name = $auth_db_name">>$conf_path
	fi
	if [ -n "$auth_db_username" ]; then
		echo "db_username = $auth_db_username">>$conf_path
	fi
	if [ -n "$auth_db_password" ]; then
		echo "db_passwd = $auth_db_password">>$conf_path
	fi

	echo "max_players = $max_players">>$conf_path

  echo "sdl = $destination/$sdlfolder" >>$conf_path
  echo "age = $destination/$agefolder" >>$conf_path

  echo "allow_unknown_accounts = $allow_unknown_accounts" >>$conf_path
	echo "auto_register_account = $auto_register_account" >>$conf_path

	if [ "$private_ip" == "0.0.0.0" ]; then
		auth_ip="127.0.0.1"
	else
		auth_ip=$private_ip
	fi
	cd ..

else
	echo "Please enter the address of the node that runs the auth server:"
	read auth_ip
fi


#now ask the per server settings
if [ $track -eq 1 ]; then

	echo "tracking" >> $destination/var/servers.list

	if [ ! -e "tracking" ]; then
		mkdir tracking
	fi
	cd tracking
	if [ "$1" == "devel" ]; then
		if [ ! -e "uru_tracking" ]; then
			ln -s ../uru_tracking
		fi
	else
		cp -f "$current/uru_tracking" $destination/bin
	fi

	valid=0

	echo ""
	echo "__________________________________________________________________"
	echo "** Setting up the Tracking Server **"

  rm -f uru.conf
	echo "# This is an autogenerated file for the Tracking server">>$conf_path

# 	if [ -n "$bandwidth" ]; then
# 		echo "bandwidth = $bandwidth">>$conf_path
# 	fi
	echo "tracking_server_port = 2011">>$conf_path


	if [ -n "$private_ip" ]; then
		echo "tracking_server = $private_ip">>$conf_path
	fi

#
#	echo "private_mask = $private_mask">>$conf_path

  if [ $single_conf -eq 0 ]; then
		if [ -n "$verbose_level" ]; then
			echo "verbose_level = $verbose_level">>$conf_path
		fi

	  echo "max_players = $max_players">>$conf_path

    echo "sdl = $destination/$sdlfolder" >>$conf_path
    echo "age = $destination/$agefolder" >>$conf_path
	fi

	### meta settings

	if [ $meta -eq 1 ]; then

		if [ -n "$shard_name" ]; then
			echo "shard_name = \"$shard_name\"">>$conf_path
		fi

		if [ -n "$shard_website" ]; then
			echo "shard_website = \"$shard_website\"">>$conf_path
		fi

		if [ -n "$shard_description" ]; then
			echo "shard_description = \"$shard_description\"">>$conf_path
		fi

		if [ -n "$shard_contact" ]; then
			echo "shard_contact = \"$shard_contact\"">>$conf_path
		fi

		if [ -n "$shard_password" ]; then
			echo "shard_password = \"$shard_password\"">>$conf_path
		fi

		echo "enable_metaserver = 1">>$conf_path

	else

		echo "enable_metaserver = 0">>$conf_path

	fi

	if [ "$1" == "devel" ]; then
		#disable the multiple instance mode
		echo "instance_mode = 0">>$conf_path
	fi

	if [ -n "$dataset" ]; then
		echo "dataset = $dataset" >>$conf_path
	fi

	if [ "$private_ip" == "0.0.0.0" ]; then
		track_ip="127.0.0.1"
	else
		track_ip=$private_ip
	fi

	cd ..

else
	echo "Please enter the address of the node that runs the tracking server:"
	read track_ip
fi

#now ask the per server settings
if [ $vault -eq 1 ]; then

	echo "vault" >> $destination/var/servers.list

	if [ ! -e "vault" ]; then
		mkdir vault
	fi
	cd vault
	if [ "$1" == "devel" ]; then
		if [ ! -e "uru_vault" ]; then
			ln -s ../uru_vault
		fi
	else
		cp -f "$current/uru_vault" $destination/bin
	fi

	valid=0

	while [ $valid -ne 1 ];
	do

		echo ""
		echo "__________________________________________________________________"
		echo "** Setting up the Vault Server **"
		if [ $single_conf -eq 0 ]; then
		  echo "The vault server needs access to a MySql database, you need to create a user with permissions to create databases, and to be able to create/delete/modify tables in the created databases"
		  echo "Please set the MySQL host: (leave blank to use a socket)"
		  read vault_db_host
		  echo "Please set the MySQL Vault user:"
		  read vault_db_username
		  echo "Please set the MySQL Vault password:"
		  read vault_db_password
		  echo "Please set the MySQL Vault DatabaseName:"
		  read vault_db_name
		fi

		echo ""
		echo "Vault defaults:"
		echo "You can set the name of the default Neighborhood, and optionnal comment for it"
		echo "Do you want to change ?"
		read ans
		case "$ans" in
		  y|Y)	echo "Please set the neighborhood name:"
			read neighborhood_name
			echo "Please set the neighborhood comment:"
			read neighborhood_comment
			;;
			*)	echo "Ok, leaving neighborhood defaults"
			;;
		esac

    if [ $single_conf -eq 0 ]; then
		  echo "########## Vault Database ############################"
		  echo "Host: $vault_db_host"
		  echo "Username: $vault_db_username"
		  echo "Password: $vault_db_password"
		  echo "Database: $vault_db_name"
		fi
		echo "########## Initial vault data ########################"
		echo "Neighborhood Name: $neighborhood_name"
		echo "Neighborhood Description: $neighborhood_comment"
		echo "######################################################"

		doet=0;

		while [ $doet -ne 1 ];
		do

			echo "Are these setting OK? [Y/n]:"
			read res

			if [[ "$res" == "Y"  || "$res" == "y" ]]; then
				valid=1
				doet=1
			elif [[ "$res" == "N"  || "$res" == "n" ]]; then
				valid=0
				doet=1
			else
				echo "Please, Enter a valid choice..."
				doet=0
			fi

		done

	done

  rm -f uru.conf
	echo "# This is an autogenerated file for the Vault server">>$conf_path

# 	if [ -n "$bandwidth" ]; then
# 		echo "bandwidth = $bandwidth">>$conf_path
# 	fi
	echo "vault_server_port = 2012">>$conf_path

	if [ -n "$private_ip" ]; then
		echo "vault_server = $private_ip">>$conf_path
	fi

  if [ $single_conf -eq 0 ]; then
		if [ -n "$verbose_level" ]; then
			echo "verbose_level = $verbose_level">>$conf_path
		fi
	  if [ -n "$vault_db_host" ]; then
	  	echo "db_server = $vault_db_host">>$conf_path
	  fi
	  if [ -n "$vault_db_name" ]; then
  		echo "db_name = $vault_db_name">>$conf_path
  	fi
  	if [ -n "$vault_db_username" ]; then
  		echo "db_username = $vault_db_username">>$conf_path
  	fi
  	if [ -n "$vault_db_password" ]; then
  		echo "db_passwd = $vault_db_password">>$conf_path
  	fi

	  echo "max_players = $max_players">>$conf_path

    echo "sdl = $destination/$sdlfolder" >>$conf_path
    echo "age = $destination/$agefolder" >>$conf_path
	fi

	[ -n "$neighborhood_name" ] && echo "neighborhood_name = \"$neighborhood_name\"">>$conf_path
	[ -n "$neighborhood_comment" ] && echo "neighborhood_comment = \"$neighborhood_comment\"">>$conf_path

	if [ "$1" == "devel" ]; then
		#disable the multiple instance mode
		echo "instance_mode = 0">>$conf_path
	fi

	if [ "$private_ip" == "0.0.0.0" ]; then
		vault_ip="127.0.0.1"
	else
		vault_ip=$private_ip
	fi

	cd ..

else
	echo "Please enter the address of the node that runs the vault server:"
	read vault_ip
fi

#now ask the per server settings
if [ $lobby -eq 1 ]; then

	echo "lobby" >> $destination/var/servers.list

	if [ ! -e "lobby" ]; then
		mkdir lobby
	fi
	cd lobby
	if [ "$1" == "devel" ]; then
		if [ ! -e "uru_lobby" ]; then
			ln -s ../uru_lobby
		fi
		if [ ! -e "uru_game" ]; then
			ln -s ../uru_game
		fi
		#only for devel
		ln -s ../servers.list
		ln -s ../run.sh
	else
		cp -f "$current/uru_lobby" $destination/bin
		cp -f "$current/uru_game" $destination/bin
	fi

	valid=0

	echo ""
	echo "__________________________________________________________________"
	echo "** Setting up the Lobby/Game Server(s) **"

  rm -f uru.conf
	echo "# This is an autogenerated file for the Lobby/Game server(s)">>$conf_path

 	if [ -n "$bandwidth" ]; then
 		echo "bandwidth = $bandwidth">>$conf_path
 	fi

	if [ -n "$private_ip" ]; then
		echo "public_bind = $private_ip">>$conf_path
	fi
	#echo "private_address = $private_ip">>$conf_path

	if [ -n "$private_mask" ]; then
		echo "private_mask = $private_mask">>$conf_path
	fi
	if [ -n "$public_ip" ]; then
		echo "public_address = $public_ip">>$conf_path
	fi
	if [ -n "$website" ]; then
		echo "website = $website">>$conf_path
	fi

  if [ $single_conf -eq 0 ]; then

		if [ -n "$verbose_level" ]; then
			echo "verbose_level = $verbose_level">>$conf_path
		fi

	  echo "max_players = $max_players">>$conf_path

	  echo "auth_server = $auth_ip">>$conf_path
	  echo "auth_server_port = 2010">>$conf_path

	  echo "tracking_server = $track_ip">>$conf_path
	  echo "tracking_server_port = 2011">>$conf_path

	  echo "vault_server = $vault_ip">>$conf_path
	  echo "vault_server_port = 2012">>$conf_path

    echo "sdl = $destination/$sdlfolder" >>$conf_path
    echo "age = $destination/$agefolder" >>$conf_path

		if [ $meta -eq 1 ]; then
			echo "enable_metaserver = 1">>$conf_path
		else
			echo "enable_metaserver = 0">>$conf_path
		fi
	fi


	if [ "$1" == "devel" ]; then
		#disable the automatic Lobby/Game forking
		echo "load_on_demand = 0">>$conf_path
	fi

	cd ..

fi

if [ "$1" == "devel" ]; then

#only for development
	mkdir game1
	cd game1
	ln -s ../uru_game
	ln -s ../servers.list
	ln -s ../run.sh
	cp ../lobby/uru.conf .
	cd ..

	cp -rf game1 game2
	cp -rf game2 game3

fi


# #create the start.sh
# 
# echo "#!/bin/sh" > start.sh
# chmod 755 start.sh
# echo "#This is an autogenerated file" >> start.sh
# echo "ulimit -c unlimited">>start.sh
# 
# if [ $auth -eq 1 ]; then
# 
# 	echo "echo \"Starting Auth Server...\"">> start.sh
# 	echo "cd auth">>start.sh
# 	echo "./uru_auth -D">>start.sh
# 	echo "cd .." >> start.sh
# 
# fi
# 
# 
# if [ $track -eq 1 ]; then
# 
# 	echo "echo \"Starting Tracking Server...\"">> start.sh
# 	echo "cd tracking" >> start.sh
# 	echo "./uru_tracking -D" >> start.sh
# 	echo "cd .." >> start.sh
# 
# fi
# 
# if [ $vault -eq 1 ]; then
# 
# 	echo "echo \"Starting Vault Server...\"">> start.sh
# 	echo "cd vault" >> start.sh
# 	echo "./uru_vault -D" >> start.sh
# 	echo "cd .." >> start.sh
# 
# fi
# 
# if [ $lobby -eq 1 ]; then
# 
# 	echo "echo \"Starting Lobby & Game(s) Server(s)...\"">> start.sh
# 	echo "cd lobby" >> start.sh
# 	echo "./uru_lobby -D" >> start.sh
# 	echo "cd .." >> start.sh
# 
# fi

echo ""
echo "** Setting up init script **"
echo ""

#cp "$current/script/huru_servers.sh" .
lines=`wc -l $current/script/huru-servers.sh | awk '{print $1}'`
nh=`grep -n ____HEAD____ $current/script/huru-servers.sh | cut -d ":" -f1`
nt=`grep -n ____BODY____ $current/script/huru-servers.sh | cut -d ":" -f1`
let lines=$lines-$nt
let lines++

head -n $nh $current/script/huru-servers.sh > huru_servers.sh
echo "### EDIT THESE ##########">>huru_servers.sh
echo "# Full Path to H'uru Root:">>huru_servers.sh
echo "HURU_ROOT=$destination">>huru_servers.sh
echo "# User to run servers as. (root == bad)">>huru_servers.sh
echo "RUNAS=$runuser">>huru_servers.sh
tail -n $lines $current/script/huru-servers.sh >>huru_servers.sh

chmod 755 huru_servers.sh

echo ""
echo "If all went OK, then the server(s) may be correctly installed at $path ($destination) and you can start/stop them typing ./huru_servers.sh (thx 2 Sjaak)"

# TODO: Finish the script

