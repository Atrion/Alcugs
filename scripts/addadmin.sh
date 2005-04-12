#!/bin/bash
#

## you may set your defaults here
#
#
#

#ask questions?
ask=1;

#defaults
#default main host where the database is
dhost="matrix"
#satellite node without access to a local database
dhost2="amlporta"
#default db user
duser="uru"
#default auth db
ddatabase="uru_auth_test"
#default admin
dadmin="almlys"
######################

if [ $ask -eq 1 ]; then
	echo "Data Base Host?:"
	read host
	echo "DB User?:"
	read user
	echo "Database Name?:"
	read database
fi

if [ -z "$1" ]; then
	echo "Admin account name?:"
	read admin
	if [ -z "$admin" ]; then
		admin=$dadmin
	fi
else
	admin=$1
fi


if [ -z "$host" ]; then
	host=`hostname`
	if [ "$host"=="$dhost2" ]; then
		host=$dhost
	fi
fi

if [ -z "$user" ]; then
	user=$duser
fi

if [ -z "$database" ]; then
	database=$ddatabase
fi

mysql -h $host -u $user -p <<EOF
USE $database;
INSERT INTO accounts (name,a_level) values('$admin',0);
EOF

if [ "$?" -ne 0 ]; then

echo "There was a problem inserting the user, please be sure that you have already started the auth server and created at least one player"


fi
