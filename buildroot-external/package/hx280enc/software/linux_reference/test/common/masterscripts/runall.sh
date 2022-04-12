#!/bin/bash

# Check that the dir is correct
if [ ! -e runall.sh ]
then
    echo "This script must be run in masterscript dir!"
    exit -1
fi

cd ..

if [ ! -e test_h264.sh ]
then
    echo "../test_h264.sh not found!"
    exit -1
fi

if [ ! -e test_jpeg.sh ]
then
    echo "../test_jpeg.sh not found!"
    exit -1
fi

if [ ! -e test_vs.sh ]
then
    echo "../test_vs.sh not found!"
    exit -1
fi

/sbin/rmmod memalloc
./memalloc_load.sh alloc_method=2
/sbin/rmmod hx280enc
./driver_load.sh


#Execute smoketests

rm -rf smoketest_ok
rm -rf smoketest_not_ok
rm -rf run_done
rm -rf random_start
rm -rf smoketest_done

echo "Executing smoketests..."

./test_h264.sh 1000 
./test_jpeg.sh 2000
./test_vs.sh 1751

touch smoketest_done

echo "Smoketests done."
echo "Now start checkall.sh."
echo -n "Waiting for checkall to signal smoketest OK"

while [ 1 ]
do

    echo -n "."
    sleep 10

    if [ -e smoketest_ok ]
    then

        echo ""
        echo "Executing test cases..."

        # First jpeg cases since they are only taking some minutes
        # H.264 cases takes hours
        ./test_jpeg.sh all
        ./test_h264.sh all
        ./test_vs.sh all

        echo "Next will be test cases in random order, you can abort with Ctrl-C."
        sleep 20

        echo "Waiting for checker to signal random start..."

        while [ ! -e random_start ]
        do
            sleep 10
        done

        echo "Executing test cases in random order..."

        ./test_random_cases.sh

        echo "Executing random parameter test cases... or abort with Ctrl-C"
        sleep 10

        ./test_random_parameter.sh

        echo "Test cases finished!"

        touch run_done

        exit -1

    elif [ -e smoketest_not_ok ]
    then
        echo "Smoketest FAILED!"
        exit -1
    fi

done
