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
#--   Abstract     : Script for compiling software and test benches for       --
#                    versatile                                                --
#--                                                                           --
#-------------------------------------------------------------------------------

#=====--------------------------------------------------------------------=====#

# This script must be run in masterscripts dir
# Check that the dir is correct
if [ ! -e make_versatile.sh ]
then
    echo "This script must be run in masterscripts dir!"
    exit -1
fi

# Commonconfig assumes that it is run in testdir
# TODO Fix all this directory mess
cd ..
. masterscripts/commonconfig.sh
cd masterscripts


TARGET=$1

BUILD_LOG=$PWD/"build.log"
rm -rf $BUILD_LOG
if [ -z $TARGET ]
then
    TARGET=versatile
fi

ROOTDIR=$PWD

#-------------------------------------------------------------------------------
#-                                                                            --
#- Compiles the encoder test bench.                                           --
#-                                                                            --
#- Parameters                                                                 --
#- 1 : The directory name where test bench make file is located               --
#- 2 : The resulting test bench binary                                        --
#-                                                                            --
#-------------------------------------------------------------------------------
compileEncoderTestBench()
{
    tb=$1
    bin=$2
    echo -n "Compiling ${tb} test bench... "
    if [ -d "../../software/linux_reference/test/${tb}" ]
    then
        cd ../../software/linux_reference/test/${tb}
        make clean >> $BUILD_LOG 2>&1 
        make libclean >> $BUILD_LOG 2>&1 
        make $TARGET >> $BUILD_LOG 2>&1 
        if [ ! -f "$bin" ]
        then
            echo "FAILED"
        else
            echo "OK"
            cp $bin $TESTDIR
        fi
        cd $ROOTDIR
    else
        echo "FAILED"
    fi    
}

#-------------------------------------------------------------------------------
#-                                                                            --
#- Compiles other modules                                                     --
#-                                                                            --
#- Parameters                                                                 --
#- 1 : The module dir to compile                                              --
#- 2 : The resulting module binary                                            --
#-                                                                            --
#-------------------------------------------------------------------------------

compileModule()
{
    compile_dir=$1
    bin=$2
    echo -n "Compiling "$compile_dir" module... "
    if [ -d "$compile_dir" ]
    then
        cd ${compile_dir}
        make clean >> $BUILD_LOG 2>&1 
        make >> $BUILD_LOG 2>&1
        if [ ! -f "$bin" ]
        then
            echo "FAILED"
        else
            echo "OK"
        fi
        cd $ROOTDIR
    else
        echo "FAILED"
    fi    
}

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

compileEncoderSystemModel ()
{
    bin=$1
    sysmodel_dir="../$systag/system/"
    echo -n "Compiling system model... "
    if [ -d "$sysmodel_dir" ]
    then
        cd ${sysmodel_dir}
        make clean >> $BUILD_LOG 2>&1 
        make testdata >> $BUILD_LOG 2>&1 
        if [ ! -f "$bin" ]
        then
            echo "FAILED"
        else
            echo "OK"
        fi
        cd $ROOTDIR
    else
        echo "FAILED"
    fi    
}

# compile all the binaries

echo "Compile target: ${TARGET}"
#echo "source /afs/hantro.com/i386_linux24/usr/arm/arm-2007q1-21-arm-none-linux-gnueabi-i686-pc-linux-gnu/settings.csh"
. "${COMPILER_SETTINGS}"
compileModule "../../software/linux_reference/memalloc/" "memalloc.ko"
compileModule "../../software/linux_reference/kernel_module/" "hx280enc.ko"
compileModule "../../software/linux_reference/test/integration/" "integration_testbench"
compileEncoderTestBench "h264" "h264_testenc"
compileEncoderTestBench "jpeg" "jpeg_testenc"
compileEncoderTestBench "camstab" "videostabtest"
compileEncoderSystemModel "h264_testenc"

TARGET=versatile_multifile
echo "Compile target: ${TARGET}"
compileEncoderTestBench "h264" "h264_testenc_multifile"

