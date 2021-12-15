#!/bin/bash
make test
echo "running with $1"
if [ $1 == "fork" ]
then
    
    ./test random
else
    if [ $1 == "sep" ]
    then 
        for x in 1 2
        do
            ./test
        done
        else
            echo "usage : ./tester.sh -opt where opt is fork or sep"
    fi
    
fi