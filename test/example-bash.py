#!/bin/bash

echo "Test"


#SYNC_OPT=""
#FRAMES=128
#NUM_PERIODS="2"
#/usr/bin/jackd -R -P 80 $SYNC_OPT -C /etc/jack-internal-session.conf -d alsa -d hw:MODDUO -r 48000 -p $FRAMES -n $NUM_PERIODS -X raw

/usr/bin/jackd --name test -R -P 80 --driver dummy -r 48000 -p128 -C2 -P2

