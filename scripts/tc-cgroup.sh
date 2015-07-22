#!/bin/bash

# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

if [ ! -d "/sys/fs/cgroup/net_prio" ]; then
	echo "Your kernel needs to have the cls_prio cgroup loaded!" 1>&2
	exit 1
fi

if [ ! -d "/sys/fs/cgroup/net_prio/netem" ]; then
	cgcreate -g net_prio:netem

	for i in $(ls /sys/class/net/);
	do
		echo "$i 15" > /sys/fs/cgroup/net_prio/netem/net_prio.ifpriomap
	done
fi

cgexec   -g net_prio:netem $@
