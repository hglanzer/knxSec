#!/bin/bash


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
		eibd -t2 -e 1.0.$1 tpuarts:/dev/knxCLR  --listen-local=/tmp/knxCLR  &
		sleep 1
		echo "sec1"
		eibd -t2 -e 1.1.$1 tpuarts:/dev/knxSEC1 --listen-local=/tmp/knxSEC1 &
		sleep 1
		echo "sec2"
		eibd -t2 -e 1.2.$1 tpuarts:/dev/knxSEC2 --listen-local=/tmp/knxSEC2 &

		echo "starting master daemon"
		sleep 1

		./master --clrSocket local:/tmp/knxCLR --sec1Socket local:/tmp/knxSEC1 --sec2Socket local:/tmp/knxSEC2
	else
		echo "usage: "
	fi
else
	echo "usage: "
fi
