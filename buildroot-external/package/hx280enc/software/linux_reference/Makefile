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
#--  Abstract : Makefile for encoder library
#--
#-------------------------------------------------------------------------------

# Comment/uncomment the following lines to define which control codes to 
# include in the library build
INCLUDE_H264 = y
INCLUDE_JPEG = y
INCLUDE_VIDSTAB = y

# Include internal testing with testId-parameter
INCLUDE_TESTING = y
 
ifeq (pci,$(findstring pci, $(MAKECMDGOALS)))
# this is just for DEMO
include Baseaddress
endif

# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

# set this to 'y' for Electric Fence checking
USE_EFENCE = n

 # don't use IRQs
#POLLING = y


ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -I/afs/hantro.com/projects/adder/users/atna/efence_2_4_13
endif

# Add your debugging flags (or not)
ifeq ($(DEBUG),y)
  DEBFLAGS = -O1 -g -DDEBUG -D_DEBUG_PRINT -D_ASSERT_USED
  DEBFLAGS += -DH8290_HAVE_ENCDEBUG_H
  DEBFLAGS += -DH8290_HAVE_ENCTRACE_H
  DEBFLAGS += -DH8290_HAVE_VIDSTABDEBUG_H
   
  #DEBFLAGS += -DASIC_WAVE_TRACE_TRIGGER  # enable logic analyzer trigger
  #DEBFLAGS += -DTRACE_EWL       # for wrapper layer trace
  #DEBFLAGS += -DTRACE_RC        # Rate Control trace  
  #DEBFLAGS += -DTRACE_REGS      # reg.trc register dump
  #DEBFLAGS += -DTRACE_RECON     # mb.trc  
  #DEBFLAGS += -DTRACE_STREAM    # stream.trc
  
  #DEBFLAGS += -DTRACE_VIDEOSTAB_INTERNAL # prints stabilization info
    
  #DEBFLAGS += -DTRACE_PIC_TIMING # trace SEI picture timing values
      
  #DEBFLAGS += -DH264ENC_TRACE   # api.trc
  #DEBFLAGS += -DJPEGENC_TRACE   # api.trc
  #DEBFLAGS += -DVIDEOSTB_TRACE   # api.trc   
else
  DEBFLAGS = -O2 -DNDEBUG
endif

# Architecture flags for gcc
#ARCH= 
#CROSS_COMPILE=

# C -compiler name, can be replaced by another compiler(replace gcc)
CC = $(CROSS_COMPILE)gcc

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring system,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring testdata,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring eval,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
endif

# System model library
MODELLIB = ../../system/models/enc8290_asic_model.a

# System model library that writes test data traces
TESTDATALIB = ../../system/models/enc8290_asic_model_trace.a


# show the path to compiler, where to find header files
INCLUDE = -I../source/h264 -I../source/jpeg \
          -I../inc -I../source/common -Iewl -Idebug_trace \
          -I../source/camstab -Ikernel_module -Imemalloc

# compiler switches
CFLAGS  = $(ARCH) -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
          $(DEBFLAGS) $(INCLUDE) #-DH8290_MEMCPY_CABAC_INIT

# list of used sourcefiles
SRC_ENC_COMMON := encasiccontroller_v2.c \
                  encasiccontroller.c \
                  encpreprocess.c 

SRC_H264 := H264CodeFrame.c\
            H264Init.c\
            H264NalUnit.c\
            H264PictureParameterSet.c\
            H264PutBits.c\
            H264RateControl.c\
            H264SequenceParameterSet.c\
            H264Slice.c\
            H264EncApi.c\
            H264Cabac.c\
            H264Mad.c\
            H264Sei.c 
            
SRC_JPEG := EncJpeg.c\
            EncJpegInit.c\
            EncJpegCodeFrame.c\
            EncJpegPutBits.c\
            JpegEncApi.c

SRC_VIDSTAB := vidstabcommon.c vidstabalg.c           
SRC_VIDSTAB_API := vidstabapi.c vidstabinternal.c

SRC_TRACE = enctrace.c enctracestream.c

# Source files for test case specific test IDs, compiler flag INTERNAL_TEST
# If these are not included some tests will fail
SRC_TESTING = H264TestId.c h264encapi_ext.c

SRC_EWL_PC := ewl_x280_file.c

SRC_EWL_ARM = ewl_x280_common.c ewl_linux_lock.c

ifeq ($(POLLING),y)
    SRC_EWL_ARM += ewl_x280_polling.c
	CFLAGS += -DENC8290_IRQ_DISABLE=1
else
    SRC_EWL_ARM += ewl_x280_irq.c
endif


# common parts only for encoder and not for video stab
INCLUDE_ENC_COMMON=n
    
# Combine the list of all the source files included in the library build
ifeq ($(INCLUDE_H264),y)
    SRCS += $(SRC_H264)
    INCLUDE_ENC_COMMON=y
endif
ifeq ($(INCLUDE_JPEG),y)
    SRCS += $(SRC_JPEG) 
    INCLUDE_ENC_COMMON=y
endif
ifeq ($(INCLUDE_VIDSTAB),y)
    SRCS += $(SRC_VIDSTAB) $(SRC_VIDSTAB_API)
    CFLAGS += -DVIDEOSTAB_ENABLED
endif
ifeq ($(INCLUDE_TESTING),y)
    SRCS += $(SRC_TESTING)
    CONFFLAGS += -DINTERNAL_TEST
endif

# add common encoder files
ifeq ($(INCLUDE_ENC_COMMON),y)
    SRCS += $(SRC_ENC_COMMON)
endif

# if tracing flags are defined we need to compile the tracing functions
ifeq ($(DEBUG),y)
    SRCS += $(SRC_TRACE)
endif

# choose EWL source, system model uses its own EWL
ifneq (,$(findstring pclinux, $(MAKECMDGOALS)))
    SRCS +=
else
ifneq (,$(findstring system, $(MAKECMDGOALS)))
    SRCS +=
else
ifneq (,$(findstring testdata, $(MAKECMDGOALS)))
    SRCS +=
else
ifneq (,$(findstring eval, $(MAKECMDGOALS)))
    SRCS +=
else
    SRCS += $(SRC_EWL_ARM)
endif
endif
endif
endif

#source search path
vpath %.c
vpath %.c ../source/common ../source/h264 ../source/jpeg \
          ../source/camstab ./ewl ./debug_trace

# name of the outputfile (library)
ENCLIB = lib8290enc.a

# MACRO for cleaning object -files
RM  = rm -f

# MACRO for creating library that includes all the object files
AR  = $(CROSS_COMPILE)ar rcsv

# object files will be generated from .c sourcefiles
OBJS    = $(SRCS:.c=.o)


#Here are rules for building codes and generating object library.
all:    tags
	@echo ---------------------------------------
	@echo "Usage: make [ system | testdata | versatile | integrator ]"
	@echo "system     - PC system model (== pclinux)"
	@echo "testdata   - PC system model for test data creation"
	@echo "eval       - PC system model for evaluation with frame limit"
	@echo "versatile  - ARM versatile with FPGA HW"
	@echo "integrator - ARM integrator with FPGA HW"
	@echo "NOTE! Make sure to do 'make clean'"
	@echo "between compiling to different targets!"
	@echo ---------------------------------------

.PHONY: pclinux system testdata integrator versatile clean tags depend

evaluation: eval
eval: CFLAGS += -DEVALUATION_LIMIT=1000
eval: system

pclinux: system

system: $(ENCLIB)

testdata: CFLAGS += -DTEST_DATA -DTRACE_STREAM
testdata: $(ENCLIB)

integrator: ENVSET  = -DSDRAM_LM_BASE=0x80000000 \
                      -DENC_MODULE_PATH=\"/dev/hx280\" \
                      -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"
integrator: ENVSET  += -DEWL_NO_HW_TIMEOUT                       
integrator: CROSS_COMPILE = arm-linux-
integrator: ARCH = -mcpu=arm9tdmi -mtune=arm9tdmi
integrator: $(ENCLIB)

versatile: ENVSET  = -DSDRAM_LM_BASE=0x80000000 \
                     -DENC_MODULE_PATH=\"/tmp/dev/hx280\" \
                     -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
#versatile: ENVSET  += -DEWL_NO_HW_TIMEOUT
#versatile: CROSS_COMPILE = arm-none-linux-gnueabihf-
#versatile: ARCH = -mcpu=cortex-a5 -mfloat-abi=softfp -mfpu=neon
versatile: ARCH = -mtune=cortex-a5
versatile: $(ENCLIB)


versatile_tb: versatile
	$(MAKE) -w -C test/h264 versatile
#	$(MAKE) -w -C test/mpeg4 versatile
	$(MAKE) -w -C test/camstab versatile
	$(MAKE) -w -C test/jpeg versatile


pcdemo: CFLAGS  = -O2 -g -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
                  -DDEBUG -D_ASSERT_USED -DH8290_HAVE_ENCDEBUG_H -DH8290_HAVE_VIDSTABDEBUG_H $(INCLUDE) 
pcdemo: $(SRC_H264:.c=.o) $(SRC_ENC_COMMON:.c=.o)  $(SRC_VIDSTAB:.c=.o)
	$(AR) $(ENCLIB) $+


.PHONY: pci
pci: 
pci: CROSS_COMPILE=
pci: ARCH=
pci: ENVSET  = -DPC_PCI_FPGA_DEMO \
               -DSDRAM_LM_BASE=$(CHW_BASE_ADDRESS) \
               -DENC_MODULE_PATH=\"/tmp/dev/hx280\" \
               -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
pci: ENVSET  += -DEWL_NO_HW_TIMEOUT 
pci: ENVSET  += -DENC_DEMO 
pci: $(ENCLIB)


system_cov: CC = covc --retain -t!debug_trace/enctrace.c,!debug_trace/enctracestream.c g++
system_cov: $(ENCLIB)


$(ENCLIB): depend static_lib

static_lib: $(OBJS)
	$(AR) $(ENCLIB) $+

%.o: %.c
	$(CC) -c $(CFLAGS) $(ENVSET) $(CONFFLAGS) $(EFENCE) $< -o $@

clean:
	$(RM) $(ENCLIB)
	$(RM) .depend
	$(RM) *.o        

tags: 
	ctags ../inc/*h ../source/common/*[ch] ../source/camstab/*[ch]\
            ../source/h264/*[ch] ../source/jpeg/*[ch]

lint: LINT_DEF=-dVIDEOSTAB_ENABLED
lint: $(SRC_H264) $(SRC_JPEG) $(SRC_VIDSTAB) $(SRC_VIDSTAB_API) # take out any not relevant sources
        # -e537 removes warnings about multiple include
        # -e641 removes warnings about converting enum to int
	-lint-nt -w2 -e537 -e641 $(LINT_DEF) $(INCLUDE) $^ > pc-lint-report.txt
        
depend: CFLAGS = $(INCLUDE)
depend: $(SRCS)
	$(CC) $(CFLAGS) $(ENVSET) $(EFENCE) -M  $^ > .depend

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
ifeq (.depend, $(wildcard .depend))
include .depend
endif
endif
