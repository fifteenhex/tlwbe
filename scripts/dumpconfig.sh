#!/bin/bash

#set -x
set -e
set -u

if [ $# -ne 1 ]; then
	echo "usage; $0 <broker>";
	exit 1
fi

BROKER=$1
LISTTOKEN=`uuidgen`
DEVTOKEN=`uuidgen`
FIFO=/tmp/$LISTTOKEN.fifo

# start reading the result topic in the background
mkfifo $FIFO
(
	mosquitto_sub -h $BROKER -t "tlwbe/control/result" > $FIFO
)&
SUBPID=$!

# start consuming lines from the fifo
trap 'kill -TERM $SUBPID $PROCESSORPID' TERM
(
	while read line; do
		RESULTTOKEN=`echo $line | jq -r ".token"`
		if [ "$RESULTTOKEN" == "$LISTTOKEN" ]; then
			echo $line | jq -r ".result[]" |\
			while read deveui; do
				mosquitto_pub -h $BROKER -t "tlwbe/control/dev/get" -m "{\"token\": \"$DEVTOKEN\", \"eui\": \"$deveui\"}"
			done
		elif [ "$RESULTTOKEN" == "$DEVTOKEN" ]; then
			echo $line | jq ".dev"
		fi
	done < $FIFO
)&
PROCESSORPID=$!


mosquitto_pub -h $BROKER -t "tlwbe/control/dev/list" -m "{\"token\": \"$LISTTOKEN\"}"

sleep 10

kill -TERM $SUBPID $PROCESSORPID
rm $FIFO
