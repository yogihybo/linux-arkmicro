#!/bin/sh

#Imports
.  ./f_check.sh

# Use the same time for all reports and faildirs
export REPORTTIME=`date +%d%m%y_%k%M | sed -e 's/ //'`

importFiles

# Enable this when running 'test_h264.sh size', then the check script will
# check all the cases which have been tested so far
checkExistingDirsOnly=0

Usage="\n$0 [-csv]\n\t -csv generate csv file report\n"

resultcsv=result.csv

CSV_REPORT=0

csvfile=$csv_path/integrationreport_h264_${hwtag}_${swtag}_${reporter}_$reporttime
list=0

# parse argument list
if [ $# -eq 1 ]; then
    first_case=1000
    last_case=1999
else
    first_case=$1;
    last_case=$2;
fi
while [ $# -ge 1 ]; do
        case $1 in
        -csv*)  CSV_REPORT=1 ;;
        -*)     echo -e $Usage; exit 1 ;;
        list)   list=1;;
        *)      ;;
        esac

        shift
done

if [ $CSV_REPORT -eq 1 ]
then
    echo "Creating report: $csvfile.csv"
    echo "TestCase;TestPhase;Category;TestedPictures;Language;StatusDate;ExecutionTime;Status;HWTag;EncSystemTag;SWTag;TestPerson;Comments;Performance;KernelVersion;RootfsVersion;CompilerVersion;TestDeviceIP;TestDeviceVersion" >> $csvfile.csv
fi

sum_ok=0
sum_notrun=0
sum_failed=0

check_case() {

    let "set_nro=${case_nr}-${case_nr}/5*5"

    . $test_case_list_dir/test_data_parameter_h264.sh "$case_nr"

    if [ ${valid[${set_nro}]} -eq 0 ]
    then
        if [ ${validForCheck[${set_nro}]} -eq 0 ]
        then
            if [ $checkExistingDirsOnly -eq 1 ] && [ ! -e case_${case_nr} ]
            then
                # The case dir doesn't exist so move on
                continue
            else
                # Compare case against system model reference
                # If case dir doesn't exist it will be waited for
                sh checkcase_h264.sh $case_nr
                res=$?
                extra_comment=${comment}" `cat case_${case_nr}/check.log`"
            fi
        else
            # The case is not valid for checking, report as not run
            res=1
            extra_comment=${comment}" Not valid for checking."
        fi

        getExecutionTime "$case_nr"

        if [ $CSV_REPORT -eq 1 ]
        then
            echo -n "case_$case_nr;Integration;H264Case;;$language;$timeform1 $timeform2;$execution_time;" >> $csvfile.csv
            echo -n "case_$case_nr;" >> $resultcsv
            case $res in
            0)
                echo "OK;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "OK;" >> $resultcsv
                let "sum_ok++"
                ;;
            1)
                echo "NOT RUN;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "NOT RUN;" >> $resultcsv
                let "sum_notrun++"
                ;;
            *)
                echo "FAILED;$hwtag;$systag;$swtag;$reporter;$extra_comment;;$kernelversion;$rootfsversion;$compilerversion;$testdeviceip;$testdeviceversion;" >> $csvfile.csv
                echo "FAILED;" >> $resultcsv
                let "sum_failed++"
                ;;
            esac
        else
            cat case_${case_nr}/check.log
            echo ""
        fi
    fi
}

if [ $list -eq 1 ]
then
    # Check list of cases
    list=`cat list`
    echo "Checking cases: $list"
    for case_nr in $list
    do
        check_case
    done
else
    # Run all cases in range
    for ((case_nr=$first_case; case_nr<=$last_case; case_nr++))
    do
        check_case
    done
fi

echo ""
if [ $CSV_REPORT -eq 1 ]
then
    echo "Totals: $sum_ok OK   $sum_notrun NOT_RUN   $sum_failed FAILED"
    grep FAILED $csvfile.csv | awk -F "_" '{ print $2 }' | awk -F ";" '{ print $1 }' > list
    echo "Failed cases written to 'list'."
    echo "Copying report $csvfile.csv to projects/8290/integration/test_reports/"
    cp $csvfile.csv /afs/hantro.com/projects/8290/integration/test_reports/
fi
