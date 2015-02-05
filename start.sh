#!/bin/bash

if [ $# -eq 1 ]
then
	if [ "$1" -gt "0" ] && [ "$1" -lt "16" ]
	then
		killall -9 eibd
	
		eibd -i -t31 -e 1.0.$1 tpuarts:/dev/knxCLR  --listen-local=/tmp/knxCLR  &
		if [ $? -eq 1 ]
		then
			echo "opening of CLR line failed"
			exit
		else
			echo "CLR ok"
		fi
		eibd -i -t31 -e 1.1.$1 tpuarts:/dev/knxSEC1 --listen-local=/tmp/knxSEC1 &
		eibd -i -t31 -e 1.1.$1 tpuarts:/dev/knxSEC2 --listen-local=/tmp/knxSEC2 &

		./src/master --clrSocket local:/tmp/knxCLR --sec1Socket local:/tmp/knxSEC1 --sec2Socket local:/tmp/knxSEC2
	else
		echo "usage: "
	fi
else
	echo "usage: "
fi
