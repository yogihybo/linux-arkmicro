#!/bin/bash

# Check that the dir is correct
if [ ! -e checkall.sh ]
then
    echo "This script must be run in masterscript dir!"
    exit -1
fi

# This script must be run in masterscript dir
cd ..

if [ ! -e checkcase_h264.sh ]
then
    echo "../checkcase_h264.sh not found!"
    exit -1
fi

if [ ! -e checkcase_jpeg.sh ]
then
    echo "../checkcase_jpeg.sh not found!"
    exit -1
fi

if [ ! -e checkcase_vs.sh ]
then
    echo "../checkcase_vs.sh not found!"
    exit -1
fi

rm -rf smoketestcheck.log
rm -rf smoketest_ok
rm -rf smoketest_not_ok
rm -rf random_start

while [ ! -e smoketest_done ]
do
    echo -n "."
    sleep 10
done

echo ""
echo "Checking smoketests"

./checkcase_h264.sh 1000
case_h264_res=$?
./checkcase_jpeg.sh 2000
case_jpeg_res=$?
./checkcase_vs.sh   1751
case_vs_res=$?

echo "$case_h264_res $case_jpeg_res $case_vs_res"

if  ([ "$case_h264_res" == "0" ]) && ([ "$case_jpeg_res" == "0" ]) && ([ "$case_vs_res" == "0" ])

then
    echo "Smoketests OK" 
    touch smoketest_ok

    echo "Checking test results..."

    sleep 1

    ./checkall_jpeg.sh   -csv | tee checkall.log
    sleep 1

    ./checkall_h264.sh   -csv | tee -a checkall.log
    sleep 1

    ./checkall_vs.sh     -csv | tee -a checkall.log
    sleep 1

    echo "Checking done..."

    # Create signal to start cases in random order
    touch random_start

    echo "Checking cases in random order..."

    ./checkall_random_cases.sh -csv | tee -a checkall.log
    sleep 1

    echo "Checking random parameter cases..."

    ./checkall_random_parameter.sh | tee -a checkall.log
    sleep 1

    echo "Test results checked!"
    echo "Results log in checkall.log"
    echo "Now check the reports in projects/8290/integration/test_reports."
    echo "Also update the tag status in PWA."
    exit -1

else

    echo "Smoketests failed! Check test environment!"
    touch smoketest_not_ok
    exit -1

fi

rm -rf smoketest_ok
rm -rf smoketest_not_ok
rm -rf random_start


