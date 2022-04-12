#!/bin/sh    

# Check script for comparing lab output to system model reference test data
# Checks output stream, encoding status, sw reg trace etc and writes check log
# Return 0 for success, -1 for fail and 1 for not failed (out of memory, unsupported size etc)

#Imports
. ./f_check.sh

importFiles

supportedPicWidth=$MAX_WIDTH
supportedPicHeight=$MAX_HEIGHT
roi=0
sei=0
    
let "case_nro=$1"
DECODER=./hx170dec_pclinux

# Dir where to save failed cases
FAILDIR=/afs/hantro.com/tmp/8290/${hwtag}_${reporttime}

# Reference test data dir
case_dir=$test_data_home/case_$case_nro

# Encoded output data dir
testcasedir=${TESTDIR}/case_${case_nro}

# If random_run in progress, the TESTDIR will be in random_cases
if [ -e random_run ]
then
    echo -en "\rRandom H.264 Case $case_nro   "
else
    echo -en "\rH.264 Case $case_nro   "
fi

if [ ! -e $test_case_list_dir/test_data_parameter_h264.sh ]
then
    echo "$test_case_list_dir/test_data_parameter_h264.sh doesn't exist!"
    exit -1
fi

. $test_case_list_dir/test_data_parameter_h264.sh "$case_nro"

set_nro=$((case_nro-case_nro/5*5))          
picWidth=$((width[set_nro]))
picHeight=$((height[set_nro]))
casetestId=$((testId[set_nro]))

# Create check.log that contains the messages of all checks.
rm -f ${testcasedir}/check.log

if [ $picWidth -gt $supportedPicWidth ] || [ $picHeight -gt $supportedPicHeight ]
then
    echo -n "Unsupported picture width or height!" >> ${testcasedir}/check.log
    exit 1
fi

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
    sleep 1
    )
fi

# Find the failing picture
findFailingPicture()
{

    if ! [ -e $1 ] || ! [ -e $2  ]
    then
        echo -n "Missing decoded YUV file(s) to cmp! "
        failing_picture=0
        export failing_picture
        return 1
    fi

    # Find the failing picture
    # Calculate the failing picture using the picture width and height
    error_pixel_number=`cmp $1 $2 | awk '{print $5}' | awk 'BEGIN {FS=","} {print $1}'`
    error_pixel=0

    if [ "$error_pixel_number" != "" ] 
    then
	# The multiplier should be 1,5 (multiply the values by 10)
	pixel_in_pic=$((picWidth*picHeight*3/2))
	failing_picture=$((error_pixel_number/pixel_in_pic))
        error_pixel=$((error_pixel_number-pixel_in_pic*failing_picture))
	export failing_picture
	export error_pixel
    return 1
    else
    return 0
    fi
}

decode()
{
    # param1 = stream file
    # param2 = output yuv

    if [ ! -e ${DECODER} ]
    then
        return
    fi

    ${DECODER} -P -O$2 $1
}

###############

# Check that reference stream exists
if [ -e ${case_dir}/stream.h264 ]
then
(
    #echo "$TESTDIR"

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

    # Compare output to reference
    if (cmp -s ${testcasedir}/case_$case_nro.h264 ${case_dir}/stream.h264)
    then
        # Stream matches reference
        echo -n "Stream OK. " >> ${testcasedir}/check.log
    else
        # Stream doesn't match reference
        fail=-1
        echo -n "Stream FAILED! " >> ${testcasedir}/check.log

        saveFileToDir ${testcasedir}/case_${case_nro}.log $FAILDIR case_${case_nro}.log
        saveFileToDir ${testcasedir}/case_${case_nro}.h264 $FAILDIR case_${case_nro}.h264
        saveFileToDir ${case_dir}/encoder.log $FAILDIR case_${case_nro}.log.ref
        saveFileToDir ${case_dir}/stream.h264 $FAILDIR case_${case_nro}.h264.ref

        # Decode output stream
        decode ${testcasedir}/case_$case_nro.h264 ${testcasedir}/case_$case_nro.yuv &> ${testcasedir}/dec_h264.log
        if [ ! -s ${testcasedir}/case_${case_nro}.yuv ]
        then
            echo -n "case_${case_nro}.yuv doesn't exist! " >> ${testcasedir}/check.log
        else
            # Decode reference stream
            decode ${case_dir}/stream.h264 ${testcasedir}/ref_case_${case_nro}.yuv &> ${testcasedir}/dec_h264.log
            if [ ! -s ${testcasedir}/ref_case_${case_nro}.yuv ]
            then
                echo -n "ref_case_${case_nro}.yuv doesn't exist! " >> ${testcasedir}/check.log
            fi
        fi

        # Compare output to reference
        findFailingPicture ${testcasedir}/case_$case_nro.yuv ${testcasedir}/ref_case_$case_nro.yuv >> ${testcasedir}/check.log

        if [ $? == 0 ]
        then
            echo -n "YUV data OK. " >> ${testcasedir}/check.log
            rm -f ${testcasedir}/case_$case_nro.yuv ${testcasedir}/ref_case_$case_nro.yuv
        else
            saveFileToDir ${testcasedir}/case_${case_nro}.yuv $FAILDIR case_${case_nro}.yuv
            saveFileToDir ${testcasedir}/ref_case_${case_nro}.yuv $FAILDIR case_${case_nro}.yuv.ref
            echo -n "YUV differs in picture $failing_picture pixel $error_pixel. " >> ${testcasedir}/check.log
        fi
    fi

    # Check encoder log for NAL size errors printed by testbench
    grep "Error: NAL" ${testcasedir}/case_${case_nro}.log > /dev/null
    res=$?
    if [ $res -eq 1 ]
    then
        echo -n "NAL sizes OK. " >> ${testcasedir}/check.log
    else
        fail=-1
        echo -n "NAL sizes FAILED! " >> ${testcasedir}/check.log
        saveFileToDir ${testcasedir}/case_${case_nro}.log $FAILDIR case_${case_nro}.log
        saveFileToDir ${case_dir}/encoder.log $FAILDIR case_${case_nro}.log.ref
    fi

    # Compare MV output
    if (cmp -s ${testcasedir}/mv.txt ${case_dir}/mv.txt)
    then
        echo -n "MV output OK. " >> ${testcasedir}/check.log
    else
        fail=-1
        echo -n "MV output FAILED! " >> ${testcasedir}/check.log
        saveFileToDir ${testcasedir}/mv.txt $FAILDIR case_${case_nro}_mv.txt
        saveFileToDir ${case_dir}/mv.txt $FAILDIR case_${case_nro}_mv.txt.ref
    fi

    # Compare register trace
    if ( [ "$TRACE_SW_REGISTER" == "y" ] )
    then
        # Parse register traces to get rid of differing regs
        removeBaseAddress "$testcasedir/sw_reg.trc"
        removeBaseAddress "$case_dir/sw_reg.trc"

        if (cmp -s ${testcasedir}/sw_reg.trc ${case_dir}/sw_reg.trc)
        then
            echo -n "Reg trace OK." >> ${testcasedir}/check.log
        else
            fail=-1
            echo -n "Reg trace FAILED!" >> ${testcasedir}/check.log
            saveFileToDir ${testcasedir}/case_${case_nro}.log $FAILDIR case_${case_nro}.log
            saveFileToDir ${testcasedir}/sw_reg.trc $FAILDIR case_${case_nro}_sw_reg.trc
            saveFileToDir ${case_dir}/encoder.log $FAILDIR case_${case_nro}.log.ref
            saveFileToDir ${case_dir}/sw_reg.trc $FAILDIR case_${case_nro}_sw_reg.trc.ref
        fi
    fi

    exit $fail
)
else
    echo -n "Reference stream.h264 missing!" >> ${testcasedir}/check.log
    exit -1
fi
 
