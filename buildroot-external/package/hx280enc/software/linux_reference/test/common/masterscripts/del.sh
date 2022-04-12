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
#--   Abstract     : Script for cleaning the test directory. Doesn't delete   --
#                    test scripts                                             --
#--                                                                           --
#-------------------------------------------------------------------------------

#=====--------------------------------------------------------------------=====#

rm -rf ../case_*
rm -rf ../vs_case*
rm -rf ../*.trc
rm -rf ../*.log
rm -rf ../*.yuv
rm -rf ../run_*
rm -rf ../memalloc.ko
rm -rf ../hx280enc.ko
rm -rf ../mpeg4_testenc 
rm -rf ../h264_testenc 
rm -rf ../jpeg_testenc 
rm -rf ../videostabtest
rm -rf ../random_*
