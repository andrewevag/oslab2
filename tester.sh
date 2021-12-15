#!/bin/bash

if [$2 = "fork"]
then
    make test
    ./test random
else
    if [$2 = "sep"]
    then 
        for x in 1 2
        do
            ./test
        done
        else
            echo "usage : ./tester.sh -opt where opt is fork or sep"
    fi
    
fi