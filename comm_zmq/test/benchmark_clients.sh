#!/usr/bin/env bash

# if seen "TEST PASSED" appears for each process, then the test is passed

if [ -z $1 ]
then
    echo "usage: $0 <num-clients> <sip> <sport>"
    exit
fi
clicnt=0

while [ $clicnt -lt $1 ]
do
    ((clicnt += 1))
    ../bin/benclient --id $clicnt --sip $2 --sport $3 &
done