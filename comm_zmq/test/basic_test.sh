#!/usr/bin/env bash

# if seen "TEST PASSED" appears for each process, then the test is passed

if [ -z $1 ]
then
    echo "usage: $0 <num-clients> [recompile]"
    exit
fi

if [ ! -z $2 ]
then
    (cd .. ; make clean ; make server ; make client)
fi

clicnt=0

../bin/server --ncli $1 &

while [ $clicnt -lt $1 ]
do
    ((clicnt += 1))
    ../bin/client --id $clicnt &
done