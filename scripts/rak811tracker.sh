#!/bin/bash

#set -x
set -e

FIFO=/tmp/`uuidgen`.fifo

if [ $# -ne 2 ]; then
	echo "usage; $0 <broker> <deveui>"
	exit 1
fi

mkfifo $FIFO
(
	mosquitto_sub -h $1 -t tlwbe/uplink/+/$2/# > $FIFO
)&
SUBPID=$!
trap 'kill -TERM $SUBPID' TERM

hextodec () {
	echo "ibase=16;obase=A;$1" | bc
}

divide () {
	echo "$1/$2" | bc -l
}

while read line; do
	hex=`echo $line | jq -r .payload | base64 -d | xxd -p -u`
	type=${hex:0:4}
	case "$type" in
	0188)
		echo "got GPS data"
		lat=$(divide $(hextodec ${hex:4:6}) 10000)
		lon=$(divide $(hextodec ${hex:10:6}) 10000)
		alt=$(divide $(hextodec ${hex:16:6}) 100)
		echo "lat/lon/alt $lat,$lon,$alt"
		;;
	0267)
		echo "got temp data"
		temp=$(divide $(hextodec ${hex:4:4}) 10)
		vbat=$(divide $(hextodec ${hex:12:4}) 100)
		echo "temp/vbat $temp,$vbat"
		;;
	0371)
		echo "got acceleration data"
		;;
	esac
done < $FIFO


kill -TERM $SUBPID
