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
#--   Abstract     : Common configuration script for the execution of         --
#--                  the SW/HW  verification test cases. To be used on set-up -- 
#--                  time.                                                    --
#--                                                                           --
#-------------------------------------------------------------------------------

#=====--------------------------------------------------------------------=====#

#=====---- Common configuration parameters for various sub configurations
PRODUCT="8290"

#SET THIS
#Component versions
export swtag="enc8290_0_71"
export hwtag="enc8290_0_71"
export systag="enc8290_0_71"
export jpegsystag="enc8290_0_71"

#Test environment configuration and information

#SET_THIS
#Test Device IP
export testdeviceip=99
TEST_DEVICE_IP=$testdeviceip

#Comments about test settings
export comment=""

# Configuration for AHB versatile board with interrupts
if [ "$testdeviceip" == "45" ]
then
    export kernelversion=linux-2.6.28-arm1/v0_1/linux-2.6.28-arm1
    export rootfsversion=v2_3_0_2
    export compilerversion=arm-2007q1-21-arm-none-linux-gnueabi-i686-pc-linux-gnu

    # Board Configuration: EB, AXIVERSATILE, VERSATILE
    BOARD="VERSATILE"

    # Kernel directory for kernel modules compilation
    KDIR="/afs/hantro.com/projects/Testing/Board_Version_Control/Realview_PB/PB926EJS/SW/Linux/linux-2.6.28-arm1/v0_1/linux-2.6.28-arm1"

    # DWL implementation to use: POLLING, IRQ
    DWL_IMPLEMENTATION="IRQ"

fi

# Configuration for AXI versatile board without interrupts
if [ "$testdeviceip" == "81" ] || [ "$testdeviceip" == "82" ]
then
    export kernelversion=linux-2.6.28-arm1/v0_0-v6/linux-2.6.28-arm1
    export rootfsversion=v2_5_0
    export compilerversion=arm-2007q1-21-arm-none-linux-gnueabi-i686-pc-linux-gnu

    # Board Configuration: EB, AXIVERSATILE, VERSATILE
    BOARD="AXIVERSATILE"

    # Kernel directory for kernel modules compilation
    KDIR="/afs/hantro.com/projects/Testing/Board_Version_Control/SW_Common/ARM_realview_v6/2.6.28-arm1/v0_0-v6/linux-2.6.28-arm1"

    # DWL implementation to use: POLLING, IRQ
    DWL_IMPLEMENTATION="POLLING"
fi

# Configuration for Socle board with interrupts
if [ "$testdeviceip" == "99" ]
then
    export kernelversion=2.6.29/v0_5/android_linux-2.6.29
    export rootfsversion=openlinux_2_0
    export compilerversion=arm-2007q1-21-arm-none-linux-gnueabi-i686-pc-linux-gnu

    # Board Configuration: EB, AXIVERSATILE, VERSATILE, SOCLE
    BOARD="SOCLE"

    # Kernel directory for kernel modules compilation
    KDIR=" /afs/hantro.com/projects/Testing/Board_Version_Control/SW_Common/SOCLE_MDK-3D/openlinux/2.6.29/v0_5/android_linux-2.6.29"

    # DWL implementation to use: POLLING, IRQ
    DWL_IMPLEMENTATION="POLLING"

fi

# TODO add configurations if testing on some other board

#SET THIS
#User data removal option for sony testing, options y or n
REMOVE_HANTRO_USER_DATA="n"

#SET THIS
#To enable internal test cases with extended api. Usually disabled with customer releases
INTERNAL_TEST="y"

#SET_THIS
#Maximum input width resolution
MAX_WIDTH="4080"

#SET_THIS
#Maximum input height resolution
MAX_HEIGHT="4080"

#SET_THIS
#Tracefiles used in case comparison
TRACE_SW_REGISTER="y"
TRACE_EWL="y"

# Optional information
export testdeviceversion=""

# Report information
reporter=$USER
timeform1=`date +%d.%m.20%y`
timeform2=`date +%k:%M:%S`

if [ "$REPORTTIME" == "" ]
then
    reporttime=`date +%d%m%y_%k%M | sed -e 's/ //'`
else
    reporttime=$REPORTTIME
fi

# System model directories used by the scripts
export SYSTEM_MODEL_HOME="$PWD/$systag/system/"
export TEST_DATA_HOME="$PWD/$systag"
export TEST_DATA_FILES="$SYSTEM_MODEL_HOME/test_data_files.txt"

# random_run signals that the cases are run in random order
# TODO fix this directory mess
if [ -e random_run ]
then
    TESTDIR=$PWD/random_cases
else
    TESTDIR=$PWD
fi

# How many cases to run with randomized parameters for H.264/JPEG, max 1000
export RANDOM_PARAMETER_CASE_AMOUNT=500

root_prefix=""
if [ -d "/misc/export" ]
then
    root_prefix="/misc"
fi

csv_path="$PWD"

#Compiler Settings
COMPILER_SETTINGS="/afs/hantro.com/i386_linux24/usr/arm/${compilerversion}/settings.sh"

#System model home directory
system_model_home=$SYSTEM_MODEL_HOME

#Test case list directory
test_case_list_dir="$SYSTEM_MODEL_HOME/test_data"

#Test data home
test_data_home=$TEST_DATA_HOME

#User data home
USER_DATA_HOME=$SYSTEM_MODEL_HOME/test_data

#Endian width
ENDIAN_WIDTH_64_BIT="1"
ENDIAN_WIDTH_32_BIT="0"

# Testcase input data directories
YUV_SEQUENCE_HOME=/afs/hantro.com/sequence/yuv

# Memory base address configuration and interrupt line

if [ "$DWL_IMPLEMENTATION" == "POLLING" ]
then
    IRQ_LINE="-1"
fi

if [ "$BOARD" == "EB" ]
then
    HW_BASE_ADDRESS="0x84000000"
    MEM_BASE_ADDRESS="0x08000000"
    MEM_BASE_ADDRESS_END="0x0DFFFFFF"
    
    if [ "$DWL_IMPLEMENTATION" == "IRQ" ]
    then 
        IRQ_LINE="72"
    fi

elif [ "$BOARD" == "AXIVERSATILE" ]
then
    HW_BASE_ADDRESS="0xC4000000"
    # Linear memory space 256MB
    MEM_BASE_ADDRESS="0x80000000"
    MEM_BASE_ADDRESS_END="0x8FFFFFFF"

    # IRQ's not supported
elif [ "$BOARD" == "VERSATILE" ]
then
    HW_BASE_ADDRESS="0xC0000000"
    # Linear memory space 96MB
    MEM_BASE_ADDRESS="0x02000000"
    MEM_BASE_ADDRESS_END="0x07FFFFFF"

    if [ "$DWL_IMPLEMENTATION" == "IRQ" ]
    then
        IRQ_LINE="30"
    fi
elif [ "$BOARD" == "SOCLE" ]
then
    HW_BASE_ADDRESS="0x20000000"
    # Linear memory space 192MB
    MEM_BASE_ADDRESS="0x44000000"
    MEM_BASE_ADDRESS_END="0x4FFFFFFF"

    if [ "$DWL_IMPLEMENTATION" == "IRQ" ]
    then
        IRQ_LINE="30"
    fi
fi





