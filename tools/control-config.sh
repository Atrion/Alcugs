# Configuration file for control-servers.sh
root=/home/alcugs/ # change this to the path you work in

# Some more config options which are less likely to change
basedir=$root/var/
bindir=$root/bin/
config=$root/etc/uru.conf
waittime=0.5 # time to wait after a server was started
start_servers="tracking vault auth lobby"
stop_servers="lobby auth tracking vault"
