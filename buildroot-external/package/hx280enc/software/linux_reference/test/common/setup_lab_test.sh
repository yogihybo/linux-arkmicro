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
#--   Abstract     : Script for copying test scripts to test directory        --
#--                                                                           --
#-------------------------------------------------------------------------------

#=====--------------------------------------------------------------------=====#

if [ ! -e masterscripts ]
then
    echo "This script must be run in software/linux_reference/test/common"
    exit
fi

TESTDIR=../../../../testdir

if [ ! -e $TESTDIR ]
then
    mkdir $TESTDIR
    chmod a+w $TESTDIR
fi

cp -r masterscripts $TESTDIR

echo "$TESTDIR/masterscripts created."

