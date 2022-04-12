#!/bin/sh -f    

#Imports
. ./f_check.sh

importFiles

let "case_nro=$1"

# Dir where to save failed cases
FAILDIR=/afs/hantro.com/tmp/8290/${hwtag}_${reporttime}

# Reference test data dir
case_dir=$test_data_home/case_$case_nro

# Encoded output data dir
testcasedir=${TESTDIR}/case_${case_nro}

# If random_run in progress, the TESTDIR will be in random_cases
if [ -e random_run ]
then
    echo -en "\rRandom JPEG Case $case_nro   "
else
    echo -en "\rJPEG Case $case_nro   "
fi

if [ ! -e $test_case_list_dir/test_data_parameter_jpeg.sh ]
then
    echo "$test_case_list_dir/test_data_parameter_jpeg.sh doesn't exist!"
    exit -1
fi

. $test_case_list_dir/test_data_parameter_jpeg.sh "$case_nro"

casetestId=$((testId[set_nro]))

# Create check.log that contains the messages of all checks.
rm -f ${testcasedir}/check.log

if ( [ "$INTERNAL_TEST" == "n" ] )
then
    if [ $casetestId != "0" ] 
    then
        echo -n "Internal test, does not work with customer releases!" >> ${testcasedir}/check.log
        exit 1
    fi
fi

# If reference test data doesn't exist, generate it
if [ ! -e $case_dir ]
then
    (
    cd $systag/system
    ./test_data/test_data.sh $case_nro >> /dev/null 2>&1
    )
fi


###############

(
    #echo "$TESTDIR/case_$case_nro"

    # Compare stream to reference
    if [ -e ${case_dir}/stream.jpg ]
    then
    (

        # Wait until the case has been run
        while [ ! -e ${testcasedir}/run_status ]
        do
            echo -n "."
            sleep 10
        done

        fail=0

        # Check run status
        run_status="`cat ${testcasedir}/run_status`"
        if [ $run_status -eq 0 ]
        then
            echo -n "Run OK. " >> ${testcasedir}/check.log
        elif [ $run_status -eq 1 ]
        then
            echo -n "Out of memory. " >> ${testcasedir}/check.log
            exit 1
        else
            echo -n "Run FAILED! " >> ${testcasedir}/check.log
        fi

        if (cmp -s ${testcasedir}/case_$case_nro.jpg ${case_dir}/stream.jpg)
        then
            # Stream matches reference
            echo -n "Stream OK. " >> ${testcasedir}/check.log
        else
            # Stream doesn't match reference
            fail=-1
            echo -n "Stream FAILED! " >> ${testcasedir}/check.log

            saveFileToDir ${testcasedir}/case_${case_nro}.log $FAILDIR case_${case_nro}.log
            saveFileToDir ${testcasedir}/case_${case_nro}.jpg $FAILDIR case_${case_nro}.jpg
            saveFileToDir ${case_dir}/encoder.log $FAILDIR case_${case_nro}.log.ref
            saveFileToDir ${case_dir}/stream.jpg $FAILDIR case_${case_nro}.jpg.ref
        fi

        # Compare register trace
        if ( [ "$TRACE_SW_REGISTER" == "y" ] )
        then
            # Parse register traces to get rid of differing regs
            removeBaseAddress "$testcasedir/sw_reg.trc"
            removeBaseAddress "$case_dir/sw_reg.trc"

            if (cmp -s ${testcasedir}/sw_reg.trc  ${case_dir}/sw_reg.trc)
            then
                echo -n "Reg trace OK. " >> ${testcasedir}/check.log
            else
                fail=-1
                echo -n "Reg trace FAILED! " >> ${testcasedir}/check.log
                saveFileToDir ${testcasedir}/case_${case_nro}.log $FAILDIR case_${case_nro}.log
                saveFileToDir ${testcasedir}/sw_reg.trc $FAILDIR case_${case_nro}_sw_reg.trc
                saveFileToDir ${case_dir}/encoder.log $FAILDIR case_${case_nro}.log.ref
                saveFileToDir ${case_dir}/sw_reg.trc $FAILDIR case_${case_nro}_sw_reg.trc.ref
                exit -1
            fi
        fi

        exit $fail
    )
    else
        echo "Reference stream.jpg missing! " >> ${testcasedir}/check.log
        exit -1
    fi
)

 
