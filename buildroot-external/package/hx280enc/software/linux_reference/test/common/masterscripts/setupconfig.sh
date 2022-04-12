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
#--   Abstract     : Script for setting up configuration(s).                  --
#--                                                                           --
#-------------------------------------------------------------------------------

#=====--------------------------------------------------------------------=====#

# This script must be run in masterscripts dir
# Check that the dir is correct
if [ ! -e setupconfig.sh ]
then
    echo "This script must be run in masterscripts dir!"
    exit -1
fi

cd ..
. masterscripts/commonconfig.sh
cd masterscripts

# Check that the config is valid

if [ ! -d $KDIR ]
then
    echo "Invalid kernel dir: $KDIR"
    exit
fi

if [ ! -e $COMPILER_SETTINGS ]
then
    echo "Invalid compiler settings: $COMPILER_SETTINGS"
    exit
fi

# set up compile time flags
echo "Setting up compiler flags in config headers and Makefiles..."

#Hantro user data removal
echo "REMOVE_HANTRO_USER_DATA=$REMOVE_HANTRO_USER_DATA"
if [ "$REMOVE_HANTRO_USER_DATA" == "y" ]
then
    sed "s,CONFFLAGS += -DMPEG4_HW_VLC_MODE_ENABLED -DMPEG4_HW_RLC_MODE_ENABLED #-DREMOVE_HANTRO_USER_DATA.*,CONFFLAGS += -DMPEG4_HW_VLC_MODE_ENABLED -DMPEG4_HW_RLC_MODE_ENABLED -DREMOVE_HANTRO_USER_DATA," ../../software/linux_reference/Makefile > tmp
    mv tmp ../../software/linux_reference/Makefile

elif [ "$REMOVE_HANTRO_USER_DATA" == "n" ]
then    
    sed "s,CONFFLAGS += -DMPEG4_HW_VLC_MODE_ENABLED -DMPEG4_HW_RLC_MODE_ENABLED -DREMOVE_HANTRO_USER_DATA.*,CONFFLAGS += -DMPEG4_HW_VLC_MODE_ENABLED -DMPEG4_HW_RLC_MODE_ENABLED #-DREMOVE_HANTRO_USER_DATA," ../../software/linux_reference/Makefile > tmp
    mv tmp ../../software/linux_reference/Makefile
fi

# HW base address
echo "HW_BASE_ADDRESS=$HW_BASE_ADDRESS"
sed "s/#define[ \t]*INTEGRATOR_LOGIC_MODULE0_BASE[ \t].*/#define INTEGRATOR_LOGIC_MODULE0_BASE $HW_BASE_ADDRESS/" ../../software/linux_reference/kernel_module/hx280enc.c > tmp
mv tmp ../../software/linux_reference/kernel_module/hx280enc.c

# Memory base address
echo "MEM_BASE_ADDRESS=$MEM_BASE_ADDRESS"
sed "s/HLINA_START[ \t]*=[ \t]*.*/HLINA_START = $MEM_BASE_ADDRESS/" ../../software/linux_reference/memalloc/Makefile > tmp
mv tmp ../../software/linux_reference/memalloc/Makefile

echo "MEM_BASE_ADDRESS_END=$MEM_BASE_ADDRESS_END"
sed "s/HLINA_END[ \t]*=[ \t]*.*/HLINA_END = $MEM_BASE_ADDRESS_END/" ../../software/linux_reference/memalloc/Makefile > tmp
mv tmp ../../software/linux_reference/memalloc/Makefile


# Kernel directory for kernel modules compilation
echo "KDIR=$KDIR"
sed "s,KDIR[ \t]*:=.*,KDIR := $KDIR," ../../software/linux_reference/memalloc/Makefile > tmp
mv tmp ../../software/linux_reference/memalloc/Makefile

sed "s,KDIR[ \t]*:=.*,KDIR := $KDIR," ../../software/linux_reference/kernel_module/Makefile > tmp
mv tmp ../../software/linux_reference/kernel_module/Makefile

#Interrupt or polling mode
echo "DWL_IMPLEMENTATION=$DWL_IMPLEMENTATION"
if [ "$DWL_IMPLEMENTATION" == "IRQ" ]
then
    sed "s/POLLING[ \t]*=.*/POLLING = n/" ../../software/linux_reference/Makefile > tmp
    mv tmp ../../software/linux_reference/Makefile

elif [ "$DWL_IMPLEMENTATION" == "POLLING" ]
then
    sed "s/POLLING[ \t]*=.*/POLLING = y/" ../../software/linux_reference/Makefile > tmp
    mv tmp ../../software/linux_reference/Makefile
fi

# IRQ line
echo "IRQ_LINE=$IRQ_LINE"
sed "s/#define[ \t]*VP_PB_INT_LT[ \t].*/#define VP_PB_INT_LT                    $IRQ_LINE/" ../../software/linux_reference/kernel_module/hx280enc.c > tmp
mv tmp ../../software/linux_reference/kernel_module/hx280enc.c

#Endian width
echo "BOARD=$BOARD"
if ( [ "$BOARD" == "EB" ]  || [ "$BOARD" == "AXIVERSATILE" ] )
then

    sed "s/#define[ \t]*ENC8290_INPUT_SWAP_32_YUV[ \t].*/#define ENC8290_INPUT_SWAP_32_YUV                         $ENDIAN_WIDTH_64_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h

    sed "s/#define[ \t]*ENC8290_INPUT_SWAP_32_RGB16[ \t].*/#define ENC8290_INPUT_SWAP_32_RGB16                       $ENDIAN_WIDTH_64_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h
    
    sed "s/#define[ \t]*ENC8290_INPUT_SWAP_32_RGB32[ \t].*/#define ENC8290_INPUT_SWAP_32_RGB32                        $ENDIAN_WIDTH_64_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h

    sed "s/#define[ \t]*ENC8290_OUTPUT_SWAP_32[ \t].*/#define ENC8290_OUTPUT_SWAP_32                        $ENDIAN_WIDTH_64_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h
    
    sed "s/#define[ \t]*VS8290_INPUT_SWAP_32_YUV[ \t].*/#define VS8290_INPUT_SWAP_32_YUV                        $ENDIAN_WIDTH_64_BIT/" ../../software/source/camstab/vidstabcfg.h > tmp
    mv tmp ../../software/source/camstab/vidstabcfg.h

    sed "s/#define[ \t]*VS8290_INPUT_SWAP_32_RGB16[ \t].*/#define VS8290_INPUT_SWAP_32_RGB16                        $ENDIAN_WIDTH_64_BIT/" ../../software/source/camstab/vidstabcfg.h > tmp
    mv tmp ../../software/source/camstab/vidstabcfg.h
    
    sed "s/#define[ \t]*VS8290_INPUT_SWAP_32_RGB32[ \t].*/#define VS8290_INPUT_SWAP_32_RGB32                        $ENDIAN_WIDTH_64_BIT/" ../../software/source/camstab/vidstabcfg.h > tmp
    mv tmp ../../software/source/camstab/vidstabcfg.h

elif [ "$BOARD" == "VERSATILE" ]
then

    sed "s/#define[ \t]*ENC8290_INPUT_SWAP_32_YUV[ \t].*/#define ENC8290_INPUT_SWAP_32_YUV                         $ENDIAN_WIDTH_32_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h

    sed "s/#define[ \t]*ENC8290_INPUT_SWAP_32_RGB16[ \t].*/#define ENC8290_INPUT_SWAP_32_RGB16                       $ENDIAN_WIDTH_32_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h
    
    sed "s/#define[ \t]*ENC8290_INPUT_SWAP_32_RGB32[ \t].*/#define ENC8290_INPUT_SWAP_32_RGB32                        $ENDIAN_WIDTH_32_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h

    sed "s/#define[ \t]*ENC8290_OUTPUT_SWAP_32[ \t].*/#define ENC8290_OUTPUT_SWAP_32                        $ENDIAN_WIDTH_32_BIT/" ../../software/source/common/enccfg.h > tmp
    mv tmp ../../software/source/common/enccfg.h
    
    sed "s/#define[ \t]*VS8290_INPUT_SWAP_32_YUV[ \t].*/#define VS8290_INPUT_SWAP_32_YUV                        $ENDIAN_WIDTH_32_BIT/" ../../software/source/camstab/vidstabcfg.h > tmp
    mv tmp ../../software/source/camstab/vidstabcfg.h

    sed "s/#define[ \t]*VS8290_INPUT_SWAP_32_RGB16[ \t].*/#define VS8290_INPUT_SWAP_32_RGB16                        $ENDIAN_WIDTH_32_BIT/" ../../software/source/camstab/vidstabcfg.h > tmp
    mv tmp ../../software/source/camstab/vidstabcfg.h
    
    sed "s/#define[ \t]*VS8290_INPUT_SWAP_32_RGB32[ \t].*/#define VS8290_INPUT_SWAP_32_RGB32                        $ENDIAN_WIDTH_32_BIT/" ../../software/source/camstab/vidstabcfg.h > tmp
    mv tmp ../../software/source/camstab/vidstabcfg.h
    
fi

#Setup Internal test for H264 & Mpeg4
echo "INTERNAL_TEST=$INTERNAL_TEST"

if ( [ "$INTERNAL_TEST" == "y" ] )
then
    sed "s/INTERNAL_TEST[ \t]*=.*/INTERNAL_TEST = y/" ../../software/linux_reference/test/h264/Makefile  > tmp
    mv tmp ../../software/linux_reference/test/h264/Makefile

elif ( [ "$INTERNAL_TEST" == "n" ] )
then
    sed "s/INTERNAL_TEST[ \t]*=.*/INTERNAL_TEST = n/" ../../software/linux_reference/test/h264/Makefile  > tmp
    mv tmp ../../software/linux_reference/test/h264/Makefile

fi

MASTERSCRIPTS=$PWD

# Checkout system model with correct tag and compile if not done earlier
if [ ! -d "../$systag" ]
then
        cd ..

        echo "Checking out system model with tag $systag"

        git clone -n /afs/hantro.com/projects/8290/git/8290_encoder/ $systag

        cd $systag

        if [ `git tag | grep -c $systag` != "1" ]
        then
                echo "System model tag $systag is not correct!"
                exit
        fi

        echo "git checkout -b branch_$systag $systag"
        git checkout -b branch_$systag $systag
        cd system

        echo "video_stab_result.log" > test_data_files.txt
        #echo "swreg.trc" >> test_data_files.txt

        cd $MASTERSCRIPTS
fi

echo "TRACE_SW_REGISTER=$TRACE_SW_REGISTER"
if ( [ "$TRACE_SW_REGISTER" == "y" ] )
then
    sed "s,#DEBFLAGS += -DTRACE_REGS.,DEBFLAGS += -DTRACE_REGS," ../../software/linux_reference/Makefile  > tmp
    mv tmp ../../software/linux_reference/Makefile
    sed "s,#DEBFLAGS += -DTRACE_REGS.,DEBFLAGS += -DTRACE_REGS," ../${systag}/software/linux_reference/Makefile  > tmp
    mv tmp ../${systag}/software/linux_reference/Makefile

elif ( [ "$TRACE_SW_REGISTER" == "n" ] )
then
    sed "s,.DEBFLAGS += -DTRACE_REGS,#DEBFLAGS += -DTRACE_REGS," ../../software/linux_reference/Makefile  > tmp
    mv tmp ../../software/linux_reference/Makefile
    sed "s,.DEBFLAGS += -DTRACE_REGS,#DEBFLAGS += -DTRACE_REGS," ../${systag}/software/linux_reference/Makefile  > tmp
    mv tmp ../${systag}/software/linux_reference/Makefile

fi

echo "TRACE_EWL=$TRACE_EWL"
if ( [ "$TRACE_EWL" == "y" ] )
then
    sed "s,#DEBFLAGS += -DTRACE_EWL.,DEBFLAGS += -DTRACE_EWL," ../../software/linux_reference/Makefile  > tmp
    mv tmp ../../software/linux_reference/Makefile

elif ( [ "$TRACE_EWL" == "n" ] )
then
    sed "s,.DEBFLAGS += -DTRACE_EWL,#DEBFLAGS += -DTRACE_EWL," ../../software/linux_reference/Makefile  > tmp
    mv tmp ../../software/linux_reference/Makefile

fi


