#!/bin/bash
#
# Wipes the vault

echo "DBHost?:"
read host
echo "User?:"
read user
echo "Database?:"
read database

## you may set your defaults here
if [ -z "$host" ]; then
	host=`hostname`
	if [ $host=="amlporta" ]; then
		host="matrix.aml.loc"
	fi
fi

if [ -z "$user" ]; then
	user="uru";
fi

if [ -z "$database" ]; then
	database="uru_vault_test"
fi

mysql -h $host -u $user -p <<EOF
drop database $database
EOF

