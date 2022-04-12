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
--  Description :  Encode picture
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "ewl.h"
#include "H264CodeFrame.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

static const u32 h264Intra16Favor[52] = {
    24, 24, 24, 26, 27, 30, 32, 35, 39, 43, 48, 53, 58, 64, 71, 78,
    85, 93, 102, 111, 121, 131, 142, 154, 167, 180, 195, 211, 229,
    248, 271, 296, 326, 361, 404, 457, 523, 607, 714, 852, 1034,
    1272, 1588, 2008, 2568, 3318, 4323, 5672, 7486, 9928, 13216,
    17648
};

/* H.264 motion estimation parameters */
static const u32 h264InterFavor[52] = {
    40, 40, 41, 42, 43, 44, 45, 48, 51, 53, 55, 60, 62, 67, 69, 72,
    78, 84, 90, 96, 110, 120, 135, 152, 170, 189, 210, 235, 265,
    297, 335, 376, 420, 470, 522, 572, 620, 670, 724, 770, 820,
    867, 915, 970, 1020, 1076, 1132, 1180, 1230, 1275, 1320, 1370
};

/* Favor value for web cam use case, 1.3x larger */
static const u32 h264InterFavorWebCam[52] = {
      52,   52,   53,   54,   55,   57,   58,   62,   66,   68,
      71,   78,   80,   87,   89,   93,  101,  109,  117,  124,
     143,  156,  175,  197,  221,  245,  273,  305,  344,  386,
     435,  488,  546,  611,  678,  743,  806,  871,  941, 1001,
    1066, 1127, 1189, 1261, 1326, 1398, 1471, 1534, 1599, 1657,
    1716, 1781
};

/* Penalty factor in 1/256 units for skip mode, 2550/(qp-1)-50 */
static const u32 h264SkipSadPenalty[52] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 233, 205, 182, 163, 146,
    132, 120, 109, 100,  92,  84,  78,  71,  66,  61,  56,  52,  48,  44,  41,
     38,  35,  32,  30,  27,  25,  23,  21,  19,  17,  15,  14,  12,  11,   9,
      8,   7,   5,   4,   3,   2,   1
};

/* sqrt(2^((qp-12)/3))*8 */
static const u32 h264DiffMvPenalty[52] =
    { 2, 2, 3, 3, 3, 4, 4, 4, 5, 6,
    6, 7, 8, 9, 10, 11, 13, 14, 16, 18,
    20, 23, 26, 29, 32, 36, 40, 45, 51, 57,
    64, 72, 81, 91, 102, 114, 128, 144, 161, 181,
    203, 228, 256, 287, 323, 362, 406, 456, 512, 575,
    645, 724
};

/* 31*sqrt(2^((qp-12)/3))/4 */
static const u32 h264DiffMvPenalty4p[52] =
    { 2, 2, 2, 3, 3, 3, 4, 4, 5, 5,
    6, 7, 8, 9, 10, 11, 12, 14, 16, 17,
    20, 22, 25, 28, 31, 35, 39, 44, 49, 55,
    62, 70, 78, 88, 98, 110, 124, 139, 156, 175,
    197, 221, 248, 278, 312, 351, 394, 442, 496, 557,
    625, 701
};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void H264SetNewFrame(h264Instance_s * inst);

/*------------------------------------------------------------------------------

    H264CodeFrame

------------------------------------------------------------------------------*/
h264EncodeFrame_e H264CodeFrame(h264Instance_s * inst)
{
    asicData_s *asic = &inst->asic;
    h264EncodeFrame_e ret;
    i32 status = ASIC_STATUS_ERROR;
    H264EncSliceReady slice;

    /* Reset callback struct */
    slice.slicesReadyPrev = 0;
    slice.slicesReady = 0;
    slice.sliceSizes = (u32 *) inst->asic.sizeTbl.nal.virtualAddress;
    slice.sliceSizes += inst->naluOffset;
    slice.pOutBuf = inst->pOutBuf;
    slice.pAppData = inst->pAppData;

    H264SetNewFrame(inst);

    EncAsicFrameStart(inst->asic.ewl, &inst->asic.regs);

    do {
        /* Encode one frame */
        i32 ewl_ret;

        /* Wait for IRQ for every slice or for complete frame */
        if ((inst->slice.sliceSize > 0) && inst->sliceReadyCbFunc)
            ewl_ret = EWLWaitHwRdy(asic->ewl, &slice.slicesReady);
        else
            ewl_ret = EWLWaitHwRdy(asic->ewl, NULL);

        if(ewl_ret != EWL_OK)
        {
            status = ASIC_STATUS_ERROR;

            if(ewl_ret == EWL_ERROR)
            {
                /* IRQ error => Stop and release HW */
                ret = H264ENCODE_SYSTEM_ERROR;
            }
            else    /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
            {
                /* IRQ Timeout => Stop and release HW */
                ret = H264ENCODE_TIMEOUT;
            }

            EncAsicStop(asic->ewl);
            /* Release HW so that it can be used by other codecs */
            EWLReleaseHw(asic->ewl);

        }
        else
        {
            /* Check ASIC status bits and possibly release HW */
            status = EncAsicCheckStatus_V2(asic);

            switch (status)
            {
            case ASIC_STATUS_ERROR:
                ret = H264ENCODE_HW_ERROR;
                break;
            case ASIC_STATUS_SLICE_READY:
                ret = H264ENCODE_OK;

                /* Issue callback to application telling how many slices
                 * are available. */
                if (inst->sliceReadyCbFunc &&
                    (slice.slicesReadyPrev < slice.slicesReady))
                    inst->sliceReadyCbFunc(&slice);
                slice.slicesReadyPrev = slice.slicesReady;
                break;
            case ASIC_STATUS_BUFF_FULL:
                ret = H264ENCODE_OK;
                inst->stream.overflow = ENCHW_YES;
                break;
            case ASIC_STATUS_HW_RESET:
                ret = H264ENCODE_HW_RESET;
                break;
            case ASIC_STATUS_FRAME_READY:
                {
                    /* Stream header remainder ie. last not full 64-bit address
                     * is counted in HW data. */
                    const u32 hw_offset = inst->stream.byteCnt & (0x07U);

                    inst->stream.byteCnt +=
                        asic->regs.outputStrmSize - hw_offset;
                    inst->stream.stream +=
                        asic->regs.outputStrmSize - hw_offset;

                    ret = H264ENCODE_OK;
                    break;
                }
            default:
                /* should never get here */
                ASSERT(0);
                ret = H264ENCODE_HW_ERROR;
            }
        }
    } while (status == ASIC_STATUS_SLICE_READY);

    /* Reset the favor values for next frame */
    inst->asic.regs.intra16Favor = 0;
    inst->asic.regs.interFavor = 0;
    inst->asic.regs.skipPenalty = 1;
    inst->asic.regs.diffMvPenalty = 0;
    inst->asic.regs.diffMvPenalty4p = 0;

    return ret;
}

/*------------------------------------------------------------------------------

    Set encoding parameters at the beginning of a new frame.

------------------------------------------------------------------------------*/
void H264SetNewFrame(h264Instance_s * inst)
{
    asicData_s *asic = &inst->asic;
    regValues_s *regs = &inst->asic.regs;

    regs->outputStrmSize -= inst->stream.byteCnt;
    regs->outputStrmSize /= 8;  /* 64-bit addresses */
    regs->outputStrmSize &= (~0x07);    /* 8 multiple size */

    /* 64-bit aligned stream base address */
    regs->outputStrmBase += (inst->stream.byteCnt & (~0x07));

    /* bit offset in the last 64-bit word */
    regs->firstFreeBit = (inst->stream.byteCnt & 0x07) * 8;

    /* header remainder is byte aligned, max 7 bytes = 56 bits */
    if(regs->firstFreeBit != 0)
    {
        /* 64-bit aligned stream pointer */
        u8 *pTmp = (u8 *) ((size_t) (inst->stream.stream) & (u32) (~0x07));
        u32 val;

        /* Clear remaining bits */
        for (val = 6; val >= regs->firstFreeBit/8; val--)
            pTmp[val] = 0;

        val = pTmp[0] << 24;
        val |= pTmp[1] << 16;
        val |= pTmp[2] << 8;
        val |= pTmp[3];

        regs->strmStartMSB = val;  /* 32 bits to MSB */

        if(regs->firstFreeBit > 32)
        {
            val = pTmp[4] << 24;
            val |= pTmp[5] << 16;
            val |= pTmp[6] << 8;

            regs->strmStartLSB = val;
        }
        else
            regs->strmStartLSB = 0;
    }
    else
    {
        regs->strmStartMSB = regs->strmStartLSB = 0;
    }


    if (inst->numViews > 1)
        regs->frameNum = inst->slice.frameNum/2;
    else
        regs->frameNum = inst->slice.frameNum;

    regs->idrPicId = inst->slice.idrPicId;

    /* Store the final register values in the register structure */
    regs->sliceSizeMbRows = inst->slice.sliceSize / inst->mbPerRow;
    regs->chromaQpIndexOffset = inst->picParameterSet.chromaQpIndexOffset;

    /* Enable slice ready interrupts if defined by config and slices in use */
    regs->sliceReadyInterrupt =
            ENC8290_SLICE_READY_INTERRUPT & (inst->slice.sliceSize > 0);

    regs->picInitQp = (u32) (inst->picParameterSet.picInitQpMinus26 + 26);

    regs->qp = inst->rateControl.qpHdr;
    regs->qpMin = inst->rateControl.qpMin;
    regs->qpMax = inst->rateControl.qpMax;

    if(inst->rateControl.mbRc)
    {
        regs->cpTarget = (u32 *) inst->rateControl.qpCtrl.wordCntTarget;
        regs->targetError = inst->rateControl.qpCtrl.wordError;
        regs->deltaQp = inst->rateControl.qpCtrl.qpChange;

        regs->cpDistanceMbs = inst->rateControl.qpCtrl.checkPointDistance;

        regs->cpTargetResults = (u32 *) inst->rateControl.qpCtrl.wordCntPrev;
    }
    else
    {
        regs->cpTarget = NULL;
    }

    regs->filterDisable = inst->slice.disableDeblocking;
    if(inst->slice.disableDeblocking != 1)
    {
        regs->sliceAlphaOffset = inst->slice.filterOffsetA / 2;
        regs->sliceBetaOffset = inst->slice.filterOffsetB / 2;
    }
    else
    {
        regs->sliceAlphaOffset = 0;
        regs->sliceBetaOffset = 0;
    }
    regs->transform8x8Mode = inst->picParameterSet.transform8x8Mode;

    /* CABAC mode 2 uses ppsId=0 (CAVLC) for intra frames and
     * ppsId=1 (CABAC) for inter frames */
    if (inst->picParameterSet.enableCabac == 2)
    {
        regs->ppsId = (inst->slice.sliceType == ISLICE) ? 0 : 1;
        regs->enableCabac = regs->ppsId;
    }
    else
    {
        regs->ppsId = 0;
        regs->enableCabac = inst->picParameterSet.enableCabac;
    }

    if(inst->picParameterSet.enableCabac)
        regs->cabacInitIdc = inst->slice.cabacInitIdc;

    regs->constrainedIntraPrediction =
        (inst->picParameterSet.constIntraPred == ENCHW_YES) ? 1 : 0;

    /* Select frame type based on viewMode and frame number */
    if (inst->slice.sliceType == ISLICE)
        regs->frameCodingType = ASIC_INTRA;
    else if ((inst->numViews > 1) && (inst->numRefBuffsLum == 1) &&
             (inst->slice.frameNum % 2) && (inst->slice.frameNum > 1))
        regs->frameCodingType = ASIC_MVC_REF_MOD;
    else if ((inst->numViews > 1) && (inst->slice.frameNum % 2))
        regs->frameCodingType = ASIC_MVC;
    else
        regs->frameCodingType = ASIC_INTER;

    /* Disable frame reconstruction for view=1 frames that are not used
     * as reference. */
    regs->recWriteDisable = (inst->numViews > 1) &&
                    (inst->numRefBuffsLum == 1) && (inst->slice.frameNum % 2);

    if (inst->slice.quarterPixelMv == 0)
        inst->asic.regs.disableQuarterPixelMv = 1;
    else if (inst->slice.quarterPixelMv == 1)
    {
        /* Adaptive setting. When resolution larger than 720p = 3600 macroblocks
         * there is not enough time to do 1/4 pixel ME */
        if(inst->mbPerFrame > 3600)
            inst->asic.regs.disableQuarterPixelMv = 1;
        else
            inst->asic.regs.disableQuarterPixelMv = 0;
    }
    else
        inst->asic.regs.disableQuarterPixelMv = 0;

    /* If favor has not been set earlier by testId use default */
    if (regs->intra16Favor == 0)
        regs->intra16Favor = h264Intra16Favor[regs->qp];
    if (regs->interFavor == 0)
        regs->interFavor = h264InterFavor[regs->qp];
    if (regs->skipPenalty == 1) {
        i32 scaler = MAX(1, 200/(inst->mbPerRow+inst->mbPerCol));
        regs->skipPenalty = MIN(255, h264SkipSadPenalty[regs->qp]*scaler);
    }
    if (regs->diffMvPenalty == 0)
        regs->diffMvPenalty = h264DiffMvPenalty[regs->qp];
    if (regs->diffMvPenalty4p == 0)
        regs->diffMvPenalty4p = h264DiffMvPenalty4p[regs->qp];

    /* HW base address for NAL unit sizes is affected by start offset
     * and SW created NALUs. */
    regs->sizeTblBase.nal = asic->sizeTbl.nal.busAddress +
                            inst->naluOffset*4 + inst->numNalus*4;
    regs->sizeTblPresent = 1;

    /* HW Base must be 64-bit aligned */
    ASSERT(regs->sizeTblBase.nal%8 == 0);

    /* MAD threshold range [0, 63*256] register 6-bits range [0,63] */
    regs->madThreshold = inst->mad.threshold / 256;
    regs->madQpDelta = inst->rateControl.mbQpAdjustment;

    /* MVC */
    regs->mvcAnchorPicFlag = inst->mvc.anchorPicFlag;
    regs->mvcPriorityId = inst->mvc.priorityId;
    regs->mvcViewId = inst->mvc.viewId;
    regs->mvcTemporalId = inst->mvc.temporalId;
    regs->mvcInterViewFlag = inst->mvc.interViewFlag;

#if defined(TRACE_RECON) || defined(ASIC_WAVE_TRACE_TRIGGER)
    {
        u32 index;

        if(asic->regs.internalImageLumBaseW ==
           asic->internalImageLuma[0].busAddress)
            index = 0;
        else
            index = 1;

        EWLmemset(asic->internalImageLuma[index].virtualAddress, 0,
                  asic->internalImageLuma[index].size);
    }
#endif

}
