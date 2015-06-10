#!/bin/bash

OPT="--tpuarts-ack-all-group --tpuarts-ack-all-individual"

if [ -a /dev/knxCLR ] && [ -a /dev/knxSEC1 ] && [ -a /dev/knxSEC2 ]
then
	echo "device files exist - OK"
else
	echo "device file(s) missing / abort"
	exit 1
fi

if [ $# -eq 1 ]
then
	if [ "$1" -gt "0" ] && [ "$1" -lt "16" ]
	then
		killall -9 eibd
		killall -9 eibd

		rm /tmp/knxCLR
		rm /tmp/knxSEC1
		rm /tmp/knxSEC2

		cd src

		echo "clr"	
		eibd -t2 -e 1.0.$1 tpuarts:/dev/knxCLR  --listen-local=/tmp/knxCLR $OPT  &
		sleep 1
		echo "sec1"
		eibd -t2 -e 1.1.$1 tpuarts:/dev/knxSEC1 --listen-local=/tmp/knxSEC1 $OPT &
		sleep 1
		echo "sec2"
		eibd -t2 -e 1.2.$1 tpuarts:/dev/knxSEC2 --listen-local=/tmp/knxSEC2 $OPT &

		echo "starting master daemon"
		./master --clrSocket local:/tmp/knxCLR --sec1Socket local:/tmp/knxSEC1 --sec2Socket local:/tmp/knxSEC2 --addr $1
	else
		echo "usage: "
	fi
else
	clear
	echo "usage: ./start.sh <device addr> "
	echo
	echo
fi
