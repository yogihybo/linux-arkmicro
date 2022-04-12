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

/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/

#include "enccommon.h"
#include "encasiccontroller.h"
#include "encpreprocess.h"
#include "ewl.h"
#include "encswhwregisters.h"

/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

#ifdef ASIC_WAVE_TRACE_TRIGGER
extern i32 trigger_point;    /* picture which will be traced */
#endif

/* Mask fields */
#define mask_2b         (u32)0x00000003
#define mask_3b         (u32)0x00000007
#define mask_4b         (u32)0x0000000F
#define mask_5b         (u32)0x0000001F
#define mask_6b         (u32)0x0000003F
#define mask_11b        (u32)0x000007FF
#define mask_14b        (u32)0x00003FFF
#define mask_16b        (u32)0x0000FFFF

#define HSWREG(n)       ((n)*4)

/* MPEG-4 motion estimation parameters */
static const i32 mpeg4InterFavor[32] = { 0,
    0, 120, 140, 160, 200, 240, 280, 340, 400, 460, 520, 600, 680,
    760, 840, 920, 1000, 1080, 1160, 1240, 1320, 1400, 1480, 1560,
    1640, 1720, 1800, 1880, 1960, 2040, 2120
};

static const u32 mpeg4DiffMvPenalty[32] = { 0,
    4, 5, 6, 7, 8, 9, 10, 11, 14, 17, 20, 23, 27, 31, 35, 38, 41,
    44, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59
};

static const u32 h264PrevModeFavor[52] = {
    7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 24, 25, 27, 29, 30, 32, 34, 36, 38, 41, 43, 46,
    49, 51, 55, 58, 61, 65, 69, 73, 78, 82, 87, 93, 98, 104, 110,
    117, 124, 132, 140
};

/* JPEG QUANT table order */
static const u32 qpReorderTable[64] =
    { 0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
      2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
      4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
      6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Initialize empty structure with default values.
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s * asic)
{
    ASSERT(asic != NULL);

    /* Initialize default values from defined configuration */
    asic->regs.irqDisable = ENC8290_IRQ_DISABLE;

    asic->regs.asicCfgReg =
        ((ENC8290_AXI_WRITE_ID & (255)) << 24) |
        ((ENC8290_AXI_READ_ID & (255)) << 16) |
        ((ENC8290_OUTPUT_SWAP_16 & (1)) << 15) |
        ((ENC8290_BURST_LENGTH & (63)) << 8) |
        ((ENC8290_BURST_SCMD_DISABLE & (1)) << 7) |
        ((ENC8290_BURST_INCR_TYPE_ENABLED & (1)) << 6) |
        ((ENC8290_BURST_DATA_DISCARD_ENABLED & (1)) << 5) |
        ((ENC8290_ASIC_CLOCK_GATING_ENABLED & (1)) << 4) |
        ((ENC8290_OUTPUT_SWAP_32 & (1)) << 3) |
        ((ENC8290_OUTPUT_SWAP_8 & (1)) << 1);

#ifdef INTERNAL_TEST
    /* With internal testing use random values for burst mode. */
    asic->regs.asicCfgReg |= (EWLReadReg(asic->ewl, 0x60) & 0xE0);
#endif

    /* Initialize default values */
    asic->regs.roundingCtrl = 0;
    asic->regs.cpDistanceMbs = 0;
    asic->regs.reconImageId = 0;

    /* User must set these */
    asic->regs.inputLumBase = 0;
    asic->regs.inputCbBase = 0;
    asic->regs.inputCrBase = 0;

    asic->internalImageLuma[0].virtualAddress = NULL;
    asic->internalImageChroma[0].virtualAddress = NULL;
    asic->internalImageLuma[1].virtualAddress = NULL;
    asic->internalImageChroma[1].virtualAddress = NULL;
    asic->internalImageLuma[2].virtualAddress = NULL;
    asic->internalImageChroma[2].virtualAddress = NULL;
    asic->cabacCtx.virtualAddress = NULL;
    asic->mvOutput.virtualAddress = NULL;

#ifdef ASIC_WAVE_TRACE_TRIGGER
    asic->regs.vop_count = 0;
#endif

    /* get ASIC ID value */
    asic->regs.asicHwId = EncAsicGetId(asic->ewl);

/* we do NOT reset hardware at this point because */
/* of the multi-instance support                  */

    return ENCHW_OK;
}


/*------------------------------------------------------------------------------

    EncAsicSetQuantTable

    Set new jpeg quantization table to be used by ASIC

------------------------------------------------------------------------------*/
void EncAsicSetQuantTable(asicData_s * asic,
                          const u8 * lumTable, const u8 * chTable)
{
    i32 i;

    ASSERT(lumTable);
    ASSERT(chTable);

    for(i = 0; i < 64; i++)
    {
        asic->regs.quantTable[i] = lumTable[qpReorderTable[i]];
    }
    for(i = 0; i < 64; i++)
    {
        asic->regs.quantTable[64 + i] = chTable[qpReorderTable[i]];
    }
}

/*------------------------------------------------------------------------------

    EncAsicSetRegisterValue

    Set a value into a defined register field

------------------------------------------------------------------------------*/
void EncAsicSetRegisterValue(u32 *regMirror, regName name, u32 value)
{
    const regField_s *field;
    u32 regVal;

    field = &asicRegisterDesc[name];

    /* Check that value fits in field */
    ASSERT(field->name == name);
    ASSERT((field->mask >> field->lsb) >= value);
    ASSERT(field->base < ASIC_SWREG_AMOUNT*4);

    /* Clear previous value of field in register */
    regVal = regMirror[field->base/4] & ~(field->mask);

    /* Put new value of field in register */
    regMirror[field->base/4] = regVal | ((value << field->lsb) & field->mask);
}

/*------------------------------------------------------------------------------

    EncAsicGetRegisterValue

    Get an unsigned value from the ASIC registers

------------------------------------------------------------------------------*/
u32 EncAsicGetRegisterValue(const void *ewl, u32 *regMirror, regName name)
{
    const regField_s *field;
    u32 value;

    field = &asicRegisterDesc[name];

    ASSERT(field->base < ASIC_SWREG_AMOUNT*4);

    value = regMirror[field->base/4] = EWLReadReg(ewl, field->base);
    value = (value & field->mask) >> field->lsb;

    return value;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetId(const void *ewl)
{
    return EWLReadReg(ewl, 0x0);
}

/*------------------------------------------------------------------------------
    When the frame is successfully encoded the internal image is recycled
    so that during the next frame the current internal image is read and
    the new reconstructed image is written. Note that this is done for
    the NEXT frame.
------------------------------------------------------------------------------*/
#define NEXTID(currentId, maxId) ((currentId == maxId) ? 0 : currentId+1)
#define PREVID(currentId, maxId) ((currentId == 0) ? maxId : currentId-1)

void EncAsicRecycleInternalImage(asicData_s *asic, u32 numViews, u32 viewId,
        u32 anchor, u32 numRefBuffsLum, u32 numRefBuffsChr)
{
    u32 tmp, id;

    if (numViews == 1)
    {
        /* H.264 base view stream, just swap buffers */
        tmp = asic->regs.internalImageLumBaseW;
        asic->regs.internalImageLumBaseW = asic->regs.internalImageLumBaseR;
        asic->regs.internalImageLumBaseR = tmp;

        tmp = asic->regs.internalImageChrBaseW;
        asic->regs.internalImageChrBaseW = asic->regs.internalImageChrBaseR;
        asic->regs.internalImageChrBaseR = tmp;
    }
    else
    {
        /* MVC stereo stream, the buffer amount tells if the second view
         * is inter-view or inter coded. This affects how the buffers
         * should be swapped. */

        if ((viewId == 0) && (anchor || (numRefBuffsLum == 1)))
        {
            /* Next frame is viewId=1 with inter-view prediction */
            asic->regs.internalImageLumBaseR = asic->internalImageLuma[0].busAddress;
            asic->regs.internalImageLumBaseW = asic->internalImageLuma[1].busAddress;
        }
        else if (viewId == 0)
        {
            /* Next frame is viewId=1 with inter prediction */
            asic->regs.internalImageLumBaseR = asic->internalImageLuma[1].busAddress;
            asic->regs.internalImageLumBaseW = asic->internalImageLuma[1].busAddress;
        }
        else
        {
            /* Next frame is viewId=0 with inter prediction */
            asic->regs.internalImageLumBaseR = asic->internalImageLuma[0].busAddress;
            asic->regs.internalImageLumBaseW = asic->internalImageLuma[0].busAddress;
        }

        if (numRefBuffsChr == 2)
        {
            if (viewId == 0)
            {
                /* For inter-view prediction chroma is swapped after base view only */
                tmp = asic->regs.internalImageChrBaseW;
                asic->regs.internalImageChrBaseW = asic->regs.internalImageChrBaseR;
                asic->regs.internalImageChrBaseR = tmp;
            }
        }
        else
        {
            /* For viewId==1 the anchor frame uses inter-view prediction,
             * all other frames use inter-prediction. */
            if ((viewId == 0) && anchor)
                id = asic->regs.reconImageId;
            else
                id = PREVID(asic->regs.reconImageId, 2);

            asic->regs.internalImageChrBaseR = asic->internalImageChroma[id].busAddress;
            asic->regs.reconImageId = id = NEXTID(asic->regs.reconImageId, 2);
            asic->regs.internalImageChrBaseW = asic->internalImageChroma[id].busAddress;
        }
    }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void CheckRegisterValues(regValues_s * val)
{
    u32 i;

    ASSERT(val->irqDisable <= 1);
    ASSERT(val->rlcLimitSpace / 2 < (1 << 20));
    ASSERT(val->mbsInCol <= 511);
    ASSERT(val->mbsInRow <= 511);
    ASSERT(val->filterDisable <= 2);
    ASSERT(val->recWriteDisable <= 1);
    ASSERT(val->madThreshold <= 63);
    ASSERT(val->madQpDelta >= -8 && val->madQpDelta <= 7);
    ASSERT(val->qp <= 51);
    ASSERT(val->constrainedIntraPrediction <= 1);
    ASSERT(val->roundingCtrl <= 1);
    ASSERT(val->frameCodingType <= 3);
    ASSERT(val->codingType <= 3);
    ASSERT(val->outputStrmSize <= 0x1FFFFFF);
    ASSERT(val->pixelsOnRow >= 16 && val->pixelsOnRow <= 8192); /* max input for cropping */
    ASSERT(val->xFill <= 3);
    ASSERT(val->yFill <= 14 && ((val->yFill & 0x01) == 0));
    ASSERT(val->inputLumaBaseOffset <= 7);
    ASSERT(val->inputChromaBaseOffset <= 7);
    ASSERT(val->sliceAlphaOffset >= -6 && val->sliceAlphaOffset <= 6);
    ASSERT(val->sliceBetaOffset >= -6 && val->sliceBetaOffset <= 6);
    ASSERT(val->chromaQpIndexOffset >= -12 && val->chromaQpIndexOffset <= 12);
    ASSERT(val->sliceSizeMbRows <= 127);
    ASSERT(val->inputImageFormat <= ASIC_INPUT_YUYV422TILED);
    ASSERT(val->inputImageRotation <= 2);
    ASSERT(val->cpDistanceMbs <= 8191);
    ASSERT(val->roi1DeltaQp >= 0 && val->roi1DeltaQp <= 15);
    ASSERT(val->roi2DeltaQp >= 0 && val->roi2DeltaQp <= 15);
    ASSERT(val->cirStart <= 65535);
    ASSERT(val->cirInterval <= 65535);
    ASSERT(val->intraAreaTop <= 255);
    ASSERT(val->intraAreaLeft <= 255);
    ASSERT(val->intraAreaBottom <= 255);
    ASSERT(val->intraAreaRight <= 255);
    ASSERT(val->roi1Top <= 255);
    ASSERT(val->roi1Left <= 255);
    ASSERT(val->roi1Bottom <= 255);
    ASSERT(val->roi1Right <= 255);
    ASSERT(val->roi2Top <= 255);
    ASSERT(val->roi2Left <= 255);
    ASSERT(val->roi2Bottom <= 255);
    ASSERT(val->roi2Right <= 255);

    if(val->codingType != ASIC_JPEG && val->cpTarget != NULL)
    {
        ASSERT(val->cpTargetResults != NULL);

        for(i = 0; i < 10; i++)
        {
            ASSERT(*val->cpTarget < (1 << 16));
        }

        ASSERT(val->targetError != NULL);

        for(i = 0; i < 7; i++)
        {
            ASSERT((*val->targetError) >= -32768 &&
                   (*val->targetError) < 32768);
        }

        ASSERT(val->deltaQp != NULL);

        for(i = 0; i < 7; i++)
        {
            ASSERT((*val->deltaQp) >= -8 && (*val->deltaQp) < 8);
        }
    }

    (void) val;
}

/*------------------------------------------------------------------------------
    Function name   : EncAsicFrameStart
    Description     : 
    Return type     : void 
    Argument        : const void *ewl
    Argument        : regValues_s * val
------------------------------------------------------------------------------*/
void EncAsicFrameStart(const void *ewl, regValues_s * val)
{
    u32 interFavor = 0, diffMvPenalty = 0, prevModeFavor = 0;

    /* Set the interrupt interval in macroblock rows (JPEG) or
     * in macroblocks (video) */
    if(val->codingType != ASIC_JPEG)
    {
        switch (val->codingType)
        {
        case ASIC_MPEG4:
        case ASIC_H263:
            interFavor = mpeg4InterFavor[val->qp];
            diffMvPenalty = mpeg4DiffMvPenalty[val->qp];
            break;
        case ASIC_H264:
            prevModeFavor = h264PrevModeFavor[val->qp];
            diffMvPenalty = val->diffMvPenalty;
            break;
        default:
            break;
        }
    }

    /*{
    const regField_s *field;
    u32 mask;
    i32 e, i, j;

    for (e = 0; e <= (i32)HEncJpegQuantChroma16; e++)
    {
        for (i = 0; i < (i32)(sizeof(asicRegisterDesc)/sizeof(regField_s)); i++)
        {
            if (asicRegisterDesc[i].name == e)
            {
                field = &asicRegisterDesc[i];

                mask = 1;
                for (j = 0; j < field->msb-field->lsb; j++)
                    mask = (mask << 1) + 1;
                mask <<= field->lsb;
                printf("[] = {0x%03x, 0x%08x, %2d, %d, \"%s\"},\n",
                        field->base, mask, field->lsb, field->trace, field->description);
            }
        }
    }
    }*/

    CheckRegisterValues(val);

    EWLmemset(val->regMirror, 0, sizeof(val->regMirror));

    /* encoder interrupt */
    EncAsicSetRegisterValue(val->regMirror, HEncIRQDisable, val->irqDisable);

    /* system configuration */
    if (val->inputImageFormat < ASIC_INPUT_RGB565)      /* YUV input */
        val->regMirror[2] = val->asicCfgReg |
            ((ENC8290_INPUT_SWAP_16_YUV & (1)) << 14) |
            ((ENC8290_INPUT_SWAP_32_YUV & (1)) << 2) |
            (ENC8290_INPUT_SWAP_8_YUV & (1));
    else if (val->inputImageFormat < ASIC_INPUT_RGB888) /* 16-bit RGB input */
        val->regMirror[2] = val->asicCfgReg |
            ((ENC8290_INPUT_SWAP_16_RGB16 & (1)) << 14) |
            ((ENC8290_INPUT_SWAP_32_RGB16 & (1)) << 2) |
            (ENC8290_INPUT_SWAP_8_RGB16 & (1));
    else    /* 32-bit RGB input */
        val->regMirror[2] = val->asicCfgReg |
            ((ENC8290_INPUT_SWAP_16_RGB32 & (1)) << 14) |
            ((ENC8290_INPUT_SWAP_32_RGB32 & (1)) << 2) |
            (ENC8290_INPUT_SWAP_8_RGB32 & (1));

    /* output stream buffer */
    EncAsicSetRegisterValue(val->regMirror, HEncBaseStream, val->outputStrmBase);

    if(val->codingType == ASIC_H264)
    {
        if(val->sizeTblBase.nal)
            val->sizeTblPresent = 1;
        EncAsicSetRegisterValue(val->regMirror, HEncBaseControl, val->sizeTblBase.nal);
        EncAsicSetRegisterValue(val->regMirror, HEncNalMode, val->sizeTblPresent);

        EncAsicSetRegisterValue(val->regMirror, HEncMvWrite, val->mvOutputBase != 0);
    }

    /* Video encoding reference picture buffers */
    if(val->codingType != ASIC_JPEG)
    {
        EncAsicSetRegisterValue(val->regMirror, HEncBaseRefLum, val->internalImageLumBaseR);
        EncAsicSetRegisterValue(val->regMirror, HEncBaseRefChr, val->internalImageChrBaseR);
        EncAsicSetRegisterValue(val->regMirror, HEncBaseRecLum, val->internalImageLumBaseW);
        EncAsicSetRegisterValue(val->regMirror, HEncBaseRecChr, val->internalImageChrBaseW);
    }

    /* Input picture buffers */
    EncAsicSetRegisterValue(val->regMirror, HEncBaseInLum, val->inputLumBase);
    EncAsicSetRegisterValue(val->regMirror, HEncBaseInCb, val->inputCbBase);
    EncAsicSetRegisterValue(val->regMirror, HEncBaseInCr, val->inputCrBase);

    /* Common control register */
    EncAsicSetRegisterValue(val->regMirror, HEncIntTimeout, ENC8290_TIMEOUT_INTERRUPT&1);
    EncAsicSetRegisterValue(val->regMirror, HEncIntSliceReady, val->sliceReadyInterrupt);
    EncAsicSetRegisterValue(val->regMirror, HEncRecWriteDisable, val->recWriteDisable);
    EncAsicSetRegisterValue(val->regMirror, HEncWidth, val->mbsInRow);
    EncAsicSetRegisterValue(val->regMirror, HEncHeight, val->mbsInCol);
    EncAsicSetRegisterValue(val->regMirror, HEncPictureType, val->frameCodingType);
    EncAsicSetRegisterValue(val->regMirror, HEncEncodingMode, val->codingType);

    /* PreP control */
    EncAsicSetRegisterValue(val->regMirror, HEncChrOffset, val->inputChromaBaseOffset);
    EncAsicSetRegisterValue(val->regMirror, HEncLumOffset, val->inputLumaBaseOffset);
    EncAsicSetRegisterValue(val->regMirror, HEncRowLength, val->pixelsOnRow);
    EncAsicSetRegisterValue(val->regMirror, HEncXFill, val->xFill);
    EncAsicSetRegisterValue(val->regMirror, HEncYFill, val->yFill);
    EncAsicSetRegisterValue(val->regMirror, HEncInputFormat, val->inputImageFormat);
    EncAsicSetRegisterValue(val->regMirror, HEncInputRot, val->inputImageRotation);

    /* H.264 control */
    EncAsicSetRegisterValue(val->regMirror, HEncPicInitQp, val->picInitQp);
    EncAsicSetRegisterValue(val->regMirror, HEncSliceAlpha, val->sliceAlphaOffset & mask_4b);
    EncAsicSetRegisterValue(val->regMirror, HEncSliceBeta, val->sliceBetaOffset & mask_4b);
    EncAsicSetRegisterValue(val->regMirror, HEncChromaQp, val->chromaQpIndexOffset & mask_5b);
    EncAsicSetRegisterValue(val->regMirror, HEncDeblocking, val->filterDisable);
    EncAsicSetRegisterValue(val->regMirror, HEncIdrPicId, val->idrPicId);
    EncAsicSetRegisterValue(val->regMirror, HEncConstrIP, val->constrainedIntraPrediction);

    EncAsicSetRegisterValue(val->regMirror, HEncPPSID, val->ppsId);
    EncAsicSetRegisterValue(val->regMirror, HEncIPPrevModeFavor, prevModeFavor);
    EncAsicSetRegisterValue(val->regMirror, HEncIPIntra16Favor, val->intra16Favor);

    EncAsicSetRegisterValue(val->regMirror, HEncSliceSize, val->sliceSizeMbRows);
    EncAsicSetRegisterValue(val->regMirror, HEncDisableQPMV, val->disableQuarterPixelMv);
    EncAsicSetRegisterValue(val->regMirror, HEncTransform8x8, val->transform8x8Mode);
    EncAsicSetRegisterValue(val->regMirror, HEncCabacInitIdc, val->cabacInitIdc);
    EncAsicSetRegisterValue(val->regMirror, HEncCabacEnable, val->enableCabac);
    EncAsicSetRegisterValue(val->regMirror, HEncInter4Restrict, val->h264Inter4x4Disabled);
    EncAsicSetRegisterValue(val->regMirror, HEncStreamMode, val->h264StrmMode);
    EncAsicSetRegisterValue(val->regMirror, HEncFrameNum, val->frameNum);

    /* JPEG control */
    EncAsicSetRegisterValue(val->regMirror, HEncJpegMode, val->jpegMode);
    EncAsicSetRegisterValue(val->regMirror, HEncJpegSlice, val->jpegSliceEnable);
    EncAsicSetRegisterValue(val->regMirror, HEncJpegRSTInt, val->jpegRestartInterval);
    EncAsicSetRegisterValue(val->regMirror, HEncJpegRST, val->jpegRestartMarker);

    /* Motion Estimation control */
    EncAsicSetRegisterValue(val->regMirror, HEncSkipPenalty, val->skipPenalty);
    EncAsicSetRegisterValue(val->regMirror, HEncInterFavor, val->interFavor);

    /* stream buffer limits */
    EncAsicSetRegisterValue(val->regMirror, HEncStrmHdrRem1, val->strmStartMSB);
    EncAsicSetRegisterValue(val->regMirror, HEncStrmHdrRem2, val->strmStartLSB);
    EncAsicSetRegisterValue(val->regMirror, HEncStrmBufLimit, val->outputStrmSize);

    EncAsicSetRegisterValue(val->regMirror, HEncMadQpDelta, val->madQpDelta & mask_4b);
    EncAsicSetRegisterValue(val->regMirror, HEncMadThreshold, val->madThreshold);

    /* video encoding rate control */
    if(val->codingType != ASIC_JPEG)
    {
        EncAsicSetRegisterValue(val->regMirror, HEncQp, val->qp);
        EncAsicSetRegisterValue(val->regMirror, HEncMaxQp, val->qpMax);
        EncAsicSetRegisterValue(val->regMirror, HEncMinQp, val->qpMin);
        EncAsicSetRegisterValue(val->regMirror, HEncCPDist, val->cpDistanceMbs);

        if(val->cpTarget != NULL)
        {
            EncAsicSetRegisterValue(val->regMirror, HEncCP1WordTarget, val->cpTarget[0]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP2WordTarget, val->cpTarget[1]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP3WordTarget, val->cpTarget[2]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP4WordTarget, val->cpTarget[3]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP5WordTarget, val->cpTarget[4]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP6WordTarget, val->cpTarget[5]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP7WordTarget, val->cpTarget[6]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP8WordTarget, val->cpTarget[7]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP9WordTarget, val->cpTarget[8]);
            EncAsicSetRegisterValue(val->regMirror, HEncCP10WordTarget, val->cpTarget[9]);

            EncAsicSetRegisterValue(val->regMirror, HEncCPWordError1,
                                    val->targetError[0] & mask_16b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPWordError2,
                                    val->targetError[1] & mask_16b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPWordError3,
                                    val->targetError[2] & mask_16b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPWordError4,
                                    val->targetError[3] & mask_16b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPWordError5,
                                    val->targetError[4] & mask_16b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPWordError6,
                                    val->targetError[5] & mask_16b);

            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp1, val->deltaQp[0] & mask_4b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp2, val->deltaQp[1] & mask_4b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp3, val->deltaQp[2] & mask_4b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp4, val->deltaQp[3] & mask_4b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp5, val->deltaQp[4] & mask_4b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp6, val->deltaQp[5] & mask_4b);
            EncAsicSetRegisterValue(val->regMirror, HEncCPDeltaQp7, val->deltaQp[6] & mask_4b);
        }
    }

    /* Stream start offset */
    EncAsicSetRegisterValue(val->regMirror, HEncStartOffset, val->firstFreeBit);

    /* Stabilization */
    if(val->codingType != ASIC_JPEG)
    {
        EncAsicSetRegisterValue(val->regMirror, HEncBaseNextLum, val->vsNextLumaBase);
        EncAsicSetRegisterValue(val->regMirror, HEncStabMode, val->vsMode);
    }

    EncAsicSetRegisterValue(val->regMirror, HEncDMVPenalty, diffMvPenalty);
    EncAsicSetRegisterValue(val->regMirror, HEncDMVPenalty4p, val->diffMvPenalty4p);

    EncAsicSetRegisterValue(val->regMirror, HEncBaseCabacCtx, val->cabacCtxBase);
    EncAsicSetRegisterValue(val->regMirror, HEncBaseMvWrite, val->mvOutputBase);

    EncAsicSetRegisterValue(val->regMirror, HEncRGBCoeffA,
                            val->colorConversionCoeffA & mask_16b);
    EncAsicSetRegisterValue(val->regMirror, HEncRGBCoeffB,
                            val->colorConversionCoeffB & mask_16b);
    EncAsicSetRegisterValue(val->regMirror, HEncRGBCoeffC,
                            val->colorConversionCoeffC & mask_16b);
    EncAsicSetRegisterValue(val->regMirror, HEncRGBCoeffE,
                            val->colorConversionCoeffE & mask_16b);
    EncAsicSetRegisterValue(val->regMirror, HEncRGBCoeffF,
                            val->colorConversionCoeffF & mask_16b);

    EncAsicSetRegisterValue(val->regMirror, HEncRMaskMSB, val->rMaskMsb & mask_5b);
    EncAsicSetRegisterValue(val->regMirror, HEncGMaskMSB, val->gMaskMsb & mask_5b);
    EncAsicSetRegisterValue(val->regMirror, HEncBMaskMSB, val->bMaskMsb & mask_5b);

    /* Other video coding controls */
    if(val->codingType != ASIC_JPEG)
    {
        EncAsicSetRegisterValue(val->regMirror, HEncCirStart, val->cirStart);
        EncAsicSetRegisterValue(val->regMirror, HEncCirInterval, val->cirInterval);

        EncAsicSetRegisterValue(val->regMirror, HEncIntraSliceMap1, val->intraSliceMap1);
        EncAsicSetRegisterValue(val->regMirror, HEncIntraSliceMap2, val->intraSliceMap2);
        EncAsicSetRegisterValue(val->regMirror, HEncIntraSliceMap3, val->intraSliceMap3);

        EncAsicSetRegisterValue(val->regMirror, HEncIntraAreaLeft, val->intraAreaLeft);
        EncAsicSetRegisterValue(val->regMirror, HEncIntraAreaRight, val->intraAreaRight);
        EncAsicSetRegisterValue(val->regMirror, HEncIntraAreaTop, val->intraAreaTop);
        EncAsicSetRegisterValue(val->regMirror, HEncIntraAreaBottom, val->intraAreaBottom);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi1Left, val->roi1Left);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi1Right, val->roi1Right);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi1Top, val->roi1Top);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi1Bottom, val->roi1Bottom);

        EncAsicSetRegisterValue(val->regMirror, HEncRoi2Left, val->roi2Left);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi2Right, val->roi2Right);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi2Top, val->roi2Top);
        EncAsicSetRegisterValue(val->regMirror, HEncRoi2Bottom, val->roi2Bottom);

        /* Limit ROI delta QPs so that ROI area QP is always >= 0.
         * ASIC doesn't check this. */
        EncAsicSetRegisterValue(val->regMirror, HEncRoi1DeltaQp, MIN(val->qp, val->roi1DeltaQp));
        EncAsicSetRegisterValue(val->regMirror, HEncRoi2DeltaQp, MIN(val->qp, val->roi2DeltaQp));

        /* MVC */
        EncAsicSetRegisterValue(val->regMirror, HEncMvcAnchorPicFlag, val->mvcAnchorPicFlag);
        EncAsicSetRegisterValue(val->regMirror, HEncMvcPriorityId, val->mvcPriorityId);
        EncAsicSetRegisterValue(val->regMirror, HEncMvcViewId, val->mvcViewId);
        EncAsicSetRegisterValue(val->regMirror, HEncMvcTemporalId, val->mvcTemporalId);
        EncAsicSetRegisterValue(val->regMirror, HEncMvcInterViewFlag, val->mvcInterViewFlag);
    }

#ifdef ASIC_WAVE_TRACE_TRIGGER
    if(val->vop_count++ == trigger_point)
    {
        /* logic analyzer triggered by writing to the ID reg */
        EWLWriteReg(ewl, 0x00, ~0);
    }
#endif

    /* Write regMirror to registers */
    {
        i32 i;

        for(i = 1; i <= 62; i++)
        {
            EWLWriteReg(ewl, HSWREG(i), val->regMirror[i]);
        }
    }

    /* Write JPEG quantization tables to regs if needed (JPEG) */
    if(val->codingType == ASIC_JPEG)
    {
        i32 i = 0;

        for(i = 0; i < 128; i += 4)
        {
            /* swreg[64] to swreg[95] */
            EWLWriteReg(ewl, HSWREG(64 + (i/4)), (val->quantTable[i] << 24) |
                        (val->quantTable[i + 1] << 16) |
                        (val->quantTable[i + 2] << 8) |
                        (val->quantTable[i + 3]));
        }
    }

#ifdef TRACE_REGS
    EncTraceRegs(ewl, 0, 0);
#endif

    /* Register with enable bit is written last */
    val->regMirror[14] |= ASIC_STATUS_ENABLE;

    EWLEnableHW(ewl, HSWREG(14), val->regMirror[14]);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicFrameContinue(const void *ewl, regValues_s * val)
{
    /* clear status bits, clear IRQ => HW restart */
    u32 status = val->regMirror[1];

    status &= (~ASIC_STATUS_ALL);
    status &= ~ASIC_IRQ_LINE;

    val->regMirror[1] = status;

    /*CheckRegisterValues(val); */

    /* Write only registers which may be updated mid frame */
    EWLWriteReg(ewl, HSWREG(24), (val->rlcLimitSpace / 2));

    val->regMirror[5] = val->rlcBase;
    EWLWriteReg(ewl, HSWREG(5), val->regMirror[5]);

#ifdef TRACE_REGS
    EncTraceRegs(ewl, 0, EncAsicGetRegisterValue(ewl, val->regMirror, HEncMbCount));
#endif

    /* Register with status bits is written last */
    EWLEnableHW(ewl, HSWREG(1), val->regMirror[1]);

}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicGetRegisters(const void *ewl, regValues_s * val)
{

    /* HW output stream size, bits to bytes */
    val->outputStrmSize =
            EncAsicGetRegisterValue(ewl, val->regMirror, HEncStrmBufLimit) / 8;

    if(val->codingType != ASIC_JPEG && val->cpTarget != NULL)
    {
        /* video coding with MB rate control ON */
        u32 i;
        u32 cpt_prev = 0;
        u32 overflow = 0;

        for(i = 0; i < 10; i++)
        {
            u32 cpt;

            /* Checkpoint result div32 */
            cpt = EncAsicGetRegisterValue(ewl, val->regMirror,
                                    (regName)(HEncCP1WordTarget+i)) * 32;

            /* detect any overflow, overflow div32 is 65536 => overflow at 21 bits */
            if(cpt < cpt_prev)
                overflow += (1 << 21);
            cpt_prev = cpt;

            val->cpTargetResults[i] = cpt + overflow;
        }
    }

    /* QP sum div2 */
    val->qpSum = EncAsicGetRegisterValue(ewl, val->regMirror, HEncQpSum) * 2;

    /* MAD MB count*/
    val->madCount = EncAsicGetRegisterValue(ewl, val->regMirror, HEncMadCount);

    /* Non-zero coefficient count*/
    val->rlcCount = EncAsicGetRegisterValue(ewl, val->regMirror, HEncRlcSum) * 4;

    /* get stabilization results if needed */
    if(val->vsMode != 0)
    {
        i32 i;

        for(i = 40; i <= 50; i++)
        {
            val->regMirror[i] = EWLReadReg(ewl, HSWREG(i));
        }
    }

#ifdef TRACE_REGS
    EncTraceRegs(ewl, 1, EncAsicGetRegisterValue(ewl, val->regMirror, HEncMbCount));
#endif

}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicStop(const void *ewl)
{
    EWLDisableHW(ewl, HSWREG(14), 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetStatus(const void *ewl)
{
    return EWLReadReg(ewl, HSWREG(1));
}


