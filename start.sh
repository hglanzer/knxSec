#!/bin/bash

if [ $# -eq 1 ]
then
	if [ "$1" -gt "0" ] && [ "$1" -lt "16" ]
	then
		killall -9 eibd

		cd src
		make debug
	
		#eibd -t31 -e 1.0.$1 tpuarts:/dev/knxCLR  --listen-local=/tmp/knxCLR  &
		#eibd -t31 -e 1.1.$1 tpuarts:/dev/knxSEC1 --listen-local=/tmp/knxSEC1 &
		#eibd -t31 -e 1.1.$1 tpuarts:/dev/knxSEC2 --listen-local=/tmp/knxSEC2 &
		
		eibd -e 1.0.$1 tpuarts:/dev/knxCLR  --listen-local=/tmp/knxCLR  &
		eibd -e 1.1.$1 tpuarts:/dev/knxSEC1 --listen-local=/tmp/knxSEC1 &
		eibd -e 1.2.$1 tpuarts:/dev/knxSEC2 --listen-local=/tmp/knxSEC2 &

		./master --clrSocket local:/tmp/knxCLR --sec1Socket local:/tmp/knxSEC1 --sec2Socket local:/tmp/knxSEC2
	else
		echo "usage: "
	fi
else
	echo "usage: "
fi
