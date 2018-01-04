#!/bin/sh

set -e
set -x

if [ $# -ne 5 ]; then
	echo "usage; $0 <broker> <appeui> <deveui> <port> <payload>"
	exit 1
fi

TOKEN=`uuidgen`

mosquitto_pub -h $1 -t "tlwbe/downlink/queue/$2/$3/$4/$TOKEN" -m $5