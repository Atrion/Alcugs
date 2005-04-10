#!/bin/sh

HURUH="/home/huru"

cd $HURUH
$HURUH/etc/cleanlog.sh
rm -rf vault/uploads/
rm -f vault/zlib*.raw
rm -f lobby/zlib*.raw
rm -f tracking/status.html
rm -rf lobby/dumps
mysql -uhuru -phuru huru <<EOF
drop table vault;
EOF
mysql -uhuru -phuru huru <<EOF
drop table ref_vault;
EOF
mysql -uhuru -phuru huru <<EOF
drop table accounts;
EOF
mysql -uhuru -phuru huru <<EOF
CREATE TABLE accounts (
  uid int(10) unsigned NOT NULL auto_increment,
  name varchar(50) NOT NULL default '',
  a_level tinyint(1) unsigned NOT NULL default '15',
  PRIMARY KEY (uid),
  UNIQUE KEY name (name)
) TYPE=MyISAM;
insert into accounts (name,a_level) values('Khoufou',0);
EOF

