/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : ASIC low level controller
--
------------------------------------------------------------------------------*/
#ifndef __ENC_ASIC_CONTROLLER_H__
#define __ENC_ASIC_CONTROLLER_H__

#include "basetype.h"
#include "enccfg.h"
#include "ewl.h"

/* HW status register bits */
#define ASIC_STATUS_ALL                 0x1FD

#define ASIC_STATUS_SLICE_READY         0x100
#define ASIC_STATUS_TEST_IRQ2           0x080
#define ASIC_STATUS_TEST_IRQ1           0x040
#define ASIC_STATUS_BUFF_FULL           0x020
#define ASIC_STATUS_HW_RESET            0x010
#define ASIC_STATUS_ERROR               0x008
#define ASIC_STATUS_FRAME_READY         0x004

#define ASIC_IRQ_LINE                   0x001

#define ASIC_STATUS_ENABLE              0x001

#define ASIC_H264_BYTE_STREAM           0x00
#define ASIC_H264_NAL_UNIT              0x01

#define ASIC_INPUT_YUV420PLANAR         0x00
#define ASIC_INPUT_YUV420SEMIPLANAR     0x01
#define ASIC_INPUT_YUYV422INTERLEAVED   0x02
#define ASIC_INPUT_UYVY422INTERLEAVED   0x03
#define ASIC_INPUT_RGB565               0x04
#define ASIC_INPUT_RGB555               0x05
#define ASIC_INPUT_RGB444               0x06
#define ASIC_INPUT_RGB888               0x07
#define ASIC_INPUT_RGB101010            0x08
#define ASIC_INPUT_YUYV422TILED         0x09

typedef enum
{
    IDLE = 0,   /* Initial state, both HW and SW disabled */
    HWON_SWOFF, /* HW processing, SW waiting for HW */
    HWON_SWON,  /* Both HW and SW processing */
    HWOFF_SWON, /* HW is paused or disabled, SW is processing */
    DONE
} bufferState_e;

typedef enum
{
    ASIC_MPEG4 = 0,
    ASIC_H263 = 1,
    ASIC_JPEG = 2,
    ASIC_H264 = 3
} asicCodingType_e;

typedef enum
{
    ASIC_P_16x16 = 0,
    ASIC_P_16x8 = 1,
    ASIC_P_8x16 = 2,
    ASIC_P_8x8 = 3,
    ASIC_I_4x4 = 4,
    ASIC_I_16x16 = 5
} asicMbType_e;

typedef enum
{
    ASIC_INTER = 0,
    ASIC_INTRA = 1,
    ASIC_MVC = 2,
    ASIC_MVC_REF_MOD = 3
} asicFrameCodingType_e;

typedef struct
{
    u32 irqDisable;
    u32 irqInterval;
    u32 mbsInCol;
    u32 mbsInRow;
    u32 qp;
    u32 qpMin;
    u32 qpMax;
    u32 constrainedIntraPrediction;
    u32 roundingCtrl;
    u32 frameCodingType;
    u32 codingType;
    u32 pixelsOnRow;
    u32 xFill;
    u32 yFill;
    u32 ppsId;
    u32 idrPicId;
    u32 frameNum;
    u32 picInitQp;
    i32 sliceAlphaOffset;
    i32 sliceBetaOffset;
    u32 filterDisable;
    u32 transform8x8Mode;
    u32 enableCabac;
    u32 cabacInitIdc;
    i32 chromaQpIndexOffset;
    u32 sliceSizeMbRows;
    u32 inputImageFormat;
    u32 inputImageRotation;
    u32 outputStrmBase;
    u32 outputStrmSize;
    u32 firstFreeBit;
    u32 strmStartMSB;
    u32 strmStartLSB;
    u32 rlcBase;
    u32 rlcLimitSpace;
    union
    {
        u32 nal;
        u32 vp;
        u32 gob;
    } sizeTblBase;
    u32 sliceReadyInterrupt;
    u32 recWriteDisable;
    u32 reconImageId;
    u32 internalImageLumBaseW;
    u32 internalImageChrBaseW;
    u32 internalImageLumBaseR;
    u32 internalImageChrBaseR;
    u32 inputLumBase;
    u32 inputCbBase;
    u32 inputCrBase;
    u32 cpDistanceMbs;
    u32 *cpTargetResults;
    const u32 *cpTarget;
    const i32 *targetError;
    const i32 *deltaQp;
    u32 rlcCount;
    u32 qpSum;
    u32 h264StrmMode;   /* 0 - byte stream, 1 - NAL units */
    u32 sizeTblPresent;
    u32 gobHeaderMask;
    u32 gobFrameId;
    u8 quantTable[8 * 8 * 2];
    u32 jpegMode;
    u32 jpegSliceEnable;
    u32 jpegRestartInterval;
    u32 jpegRestartMarker;
    u32 regMirror[64];
    u32 inputLumaBaseOffset;
    u32 inputLumaBaseOffsetVert;
    u32 inputChromaBaseOffset;
    u32 h264Inter4x4Disabled;
    u32 disableQuarterPixelMv;
    u32 vsNextLumaBase;
    u32 vsMode;
    u32 vpSize;
    u32 vpMbBits;
    u32 hec;
    u32 moduloTimeBase;
    u32 intraDcVlcThr;
    u32 vopFcode;
    u32 timeInc;
    u32 timeIncBits;
    u32 asicCfgReg;
    u32 asicHwId;
    u32 intra16Favor;
    u32 interFavor;
    u32 skipPenalty;
    u32 diffMvPenalty;
    u32 diffMvPenalty4p;
    i32 madQpDelta;
    u32 madThreshold;
    u32 madCount;
    u32 mvcAnchorPicFlag;
    u32 mvcPriorityId;
    u32 mvcViewId;
    u32 mvcTemporalId;
    u32 mvcInterViewFlag;
    u32 cirStart;
    u32 cirInterval;
    u32 intraSliceMap1;
    u32 intraSliceMap2;
    u32 intraSliceMap3;
    u32 intraAreaTop;
    u32 intraAreaLeft;
    u32 intraAreaBottom;
    u32 intraAreaRight;
    u32 roi1Top;
    u32 roi1Left;
    u32 roi1Bottom;
    u32 roi1Right;
    u32 roi2Top;
    u32 roi2Left;
    u32 roi2Bottom;
    u32 roi2Right;
    i32 roi1DeltaQp;
    i32 roi2DeltaQp;
    u32 mvOutputBase;
    u32 cabacCtxBase;
    u32 colorConversionCoeffA;
    u32 colorConversionCoeffB;
    u32 colorConversionCoeffC;
    u32 colorConversionCoeffE;
    u32 colorConversionCoeffF;
    u32 rMaskMsb;
    u32 gMaskMsb;
    u32 bMaskMsb;
#ifdef ASIC_WAVE_TRACE_TRIGGER
    u32 vop_count;
#endif
} regValues_s;

typedef struct
{
    const void *ewl;
    regValues_s regs;
    EWLLinearMem_t internalImageLuma[3];
    EWLLinearMem_t internalImageChroma[3];
    EWLLinearMem_t cabacCtx;
    EWLLinearMem_t mvOutput;
    u32 sizeTblSize;
    union
    {
        EWLLinearMem_t nal;
        EWLLinearMem_t vp;
        EWLLinearMem_t gob;
    } sizeTbl;
} asicData_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s * asic);

i32 EncAsicMemAlloc_V2(asicData_s * asic, u32 width, u32 height,
                       u32 encodingType, u32 numRefBuffsLum, u32 numRefBuffsChr);
void EncAsicMemFree_V2(asicData_s * asic);

/* Functions for controlling ASIC */
void EncAsicSetQuantTable(asicData_s * asic,
                          const u8 * lumTable, const u8 * chTable);

void EncAsicGetRegisters(const void *ewl, regValues_s * val);
u32 EncAsicGetStatus(const void *ewl);

u32 EncAsicGetId(const void *ewl);

void EncAsicFrameStart(const void *ewl, regValues_s * val);

void EncAsicStop(const void *ewl);

void EncAsicRecycleInternalImage(asicData_s *asic, u32 numViews, u32 viewId,
        u32 anchor, u32 numRefBuffsLum, u32 numRefBuffsChr);

i32 EncAsicCheckStatus_V2(asicData_s * asic);

#ifdef MPEG4_HW_RLC_MODE_ENABLED

i32 EncAsicMemAlloc(asicData_s * asic, u32 width, u32 height, u32 rlcBufSize);
void EncAsicMemFree(asicData_s * asic);

/* Functions for parsing data from ASIC output tables */
asicMbType_e EncAsicMbType(const u32 * control);
i32 EncAsicQp(const u32 * control);
void EncAsicMv(const u32 * control, i8 mv[4], i32 xy);
void EncAsicDc(i32 * mbDc, const u32 * control);
i32 EncAsicRlcCount(const u32 * mbRlc[6], i32 mbRlcCount[6],
                    const u32 * rlcData, const u32 * control);

void EncAsicFrameContinue(const void *ewl, regValues_s * val);

i32 EncAsicCheckStatus(asicData_s * asic);

#endif /* MPEG4_HW_RLC_MODE_ENABLED */

#endif
