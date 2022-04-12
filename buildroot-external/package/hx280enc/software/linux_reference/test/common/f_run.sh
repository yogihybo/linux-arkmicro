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
#--   Abstract     : Script functions for running the SW/HW verification      --
#                    test cases                                               --
#--                                                                           --
#-------------------------------------------------------------------------------            

.  ./masterscripts/commonconfig.sh


startTimeCounter()
{
    case_nro=$1
    rm -f "${TESTDIR}/case_${case_nro}.time"
    start_time=$(date +%s)
    echo "START:${start_time}" >> ${TESTDIR}/"case_${case_nro}.time"
}

endTimeCounter()
{
    case_nro=$1
    end_time=$(date +%s)
    echo "END:${end_time}" >> ${TESTDIR}/"case_${case_nro}.time"
}
