#!/bin/bash
#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--   Abstract     : Script functions for checking the results the SW/HW      --
#--                  verification test cases                                  --
#--                                                                           --
#-------------------------------------------------------------------------------


#-------------------------------------------------------------------------------
#-                                                                            --
#- Calculates the execution time for the test case                            --
#-                                                                            --
#-                                                                            --
#- Parameters                                                                 --
#- 1 : Test case number			                                      --
#-                                                                            --
#- Exports                                                                    --
#- execution_time                                                             --
#-                                                                            --
#-------------------------------------------------------------------------------

 .  ./masterscripts/commonconfig.sh

importFiles()
{
    # import commonconfig
    if [ ! -e masterscripts/commonconfig.sh ]
    then
        echo "masterscripts/commonconfig.sh not found!"
        exit -1
    else
        .  ./masterscripts/commonconfig.sh
    fi

    if [ "$test_case_list_dir" == "" ]
    then
        echo "Check test_case_list_dir missing from <commonconfig.sh>."
        exit -1
    fi

}

saveFileToDir()
{
    # param1 = file to copy
    # param2 = destination directory
    # param3 = destination filename

    if [ ! -e $1 ]
    then
        echo -n "$1 doesn't exist! "
        return
    fi

    if [ ! -e $2 ]
    then
        mkdir -p $2
        if [ ! -e $2 ]
        then
            echo -n "Failed to create $2! "
            return
        fi
    fi

    cp $1 $2/$3
}

getExecutionTime()
{
    case_number=$1
    
    execution_time="N/A"
    if [ -e "${TESTDIR}/case_${case_number}.time" ]
    then
        start_time=`grep START ${TESTDIR}/case_${case_number}.time | awk -F : '{print $2}'`
        end_time=`grep END ${TESTDIR}/case_${case_number}.time | awk -F : '{print $2}'`
        let 'execution_time=end_time-start_time'
    fi
    
    export execution_time
}

removeBaseAddress()
{
    # Remove base addresses and other registers that will not match between
    # system model and test run from SW/HW register trace file
    # param1 = register trace file

    # mb= not printed by system model
    grep -e "mb=" -v $1 > foo.tmp; mv -f foo.tmp $1
    # ASIC ID not present on system model
    grep -e "000/" -v $1 > foo.tmp; mv -f foo.tmp $1
    # Swap bits don't match
    grep -e "008/" -v $1 > foo.tmp; mv -f foo.tmp $1
    # Base addresses
    grep -e "014/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "018/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "01c/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "020/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "024/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "028/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "02c/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "030/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "034/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "09c/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "0cc/" -v $1 > foo.tmp; mv -f foo.tmp $1
    grep -e "0d0/" -v $1 > foo.tmp; mv -f foo.tmp $1
    # Stream buffer size
    grep -e "060/" -v $1 > foo.tmp; mv -f foo.tmp $1
    # Config register
    grep -e "0fc/" -v $1 > foo.tmp; mv -f foo.tmp $1
    # Jpeg quant table regs 0x100-0x17c not possible to read from ASIC
    grep -e " 000001" -v $1 > foo.tmp; mv -f foo.tmp $1


}
