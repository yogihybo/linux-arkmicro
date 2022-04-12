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
#include "encpreprocess.h"
#include "encasiccontroller.h"
#include "enccommon.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    EncAsicMemAlloc_V2

    Allocate HW/SW shared memory

    Input:
        asic        asicData structure
        width       width of encoded image, multiple of four
        height      height of encoded image
        type        ASIC_MPEG4 / ASIC_H263 / ASIC_JPEG
        numRefBuffs amount of reference luma frame buffers to be allocated

    Output:
        asic        base addresses point to allocated memory areas

    Return:
        ENCHW_OK        Success.
        ENCHW_NOK       Error: memory allocation failed, no memories allocated
                        and EWL instance released

------------------------------------------------------------------------------*/
i32 EncAsicMemAlloc_V2(asicData_s * asic, u32 width, u32 height,
                       u32 encodingType, u32 numRefBuffsLum, u32 numRefBuffsChr)
{
    u32 mbTotal, i;
    regValues_s *regs;
    EWLLinearMem_t *buff = NULL;

    ASSERT(asic != NULL);
    ASSERT(width != 0);
    ASSERT(height != 0);
    ASSERT((height % 2) == 0);
    ASSERT((width % 4) == 0);

    regs = &asic->regs;

    regs->codingType = encodingType;

    width = (width + 15) / 16;
    height = (height + 15) / 16;

    mbTotal = width * height;

    /* Allocate H.264 internal memories */
    if(regs->codingType != ASIC_JPEG)
    {
        /* The sizes of the memories */
        u32 internalImageLumaSize = mbTotal * (16 * 16);
        u32 internalImageChromaSize = mbTotal * (2 * 8 * 8);

        ASSERT((numRefBuffsLum >= 1) && (numRefBuffsLum <= 2));
        ASSERT((numRefBuffsChr >= 2) && (numRefBuffsChr <= 3));

        for (i = 0; i < numRefBuffsLum; i++)
        {
            if(EWLMallocRefFrm(asic->ewl, internalImageLumaSize,
                               &asic->internalImageLuma[i]) != EWL_OK)
            {
                EncAsicMemFree_V2(asic);
                return ENCHW_NOK;
            }
        }

        for (i = 0; i < numRefBuffsChr; i++)
        {
            if(EWLMallocRefFrm(asic->ewl, internalImageChromaSize,
                               &asic->internalImageChroma[i]) != EWL_OK)
            {
                EncAsicMemFree_V2(asic);
                return ENCHW_NOK;
            }
        }

        /* Set base addresses to the registers */
        regs->internalImageLumBaseW = asic->internalImageLuma[0].busAddress;
        regs->internalImageChrBaseW = asic->internalImageChroma[0].busAddress;
        if (numRefBuffsLum > 1)
            regs->internalImageLumBaseR = asic->internalImageLuma[1].busAddress;
        else
            regs->internalImageLumBaseR = asic->internalImageLuma[0].busAddress;
        regs->internalImageChrBaseR = asic->internalImageChroma[1].busAddress;

        /* NAL size table, table size must be 64-bit multiple,
         * space for SEI, MVC prefix, filler and zero at the end of table */
        if(regs->codingType == ASIC_H264)
        {
            /* Atleast 1 macroblock row in every slice */
            buff = &asic->sizeTbl.nal;
            asic->sizeTblSize = (sizeof(u32) * (height+4) + 7) & (~7);
        }

        if(EWLMallocLinear(asic->ewl, asic->sizeTblSize, buff) != EWL_OK)
        {
            EncAsicMemFree_V2(asic);
            return ENCHW_NOK;
        }

        /* CABAC context tables: all qps, intra+inter, 464 bytes/table  */
        if(EWLMallocLinear(asic->ewl, 52*2*464, &asic->cabacCtx) != EWL_OK)
        {
            EncAsicMemFree_V2(asic);
            return ENCHW_NOK;
        }
        regs->cabacCtxBase = asic->cabacCtx.busAddress;

        /* MV output table: 4 bytes per MB  */
        if(EWLMallocLinear(asic->ewl, mbTotal*4, &asic->mvOutput) != EWL_OK)
        {
            EncAsicMemFree_V2(asic);
            return ENCHW_NOK;
        }
        regs->mvOutputBase = asic->mvOutput.busAddress;

        /* Clear mv output memory*/
        for (i = 0; i < mbTotal; i++)
            asic->mvOutput.virtualAddress[i] = 0;

    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    EncAsicMemFree_V2

    Free HW/SW shared memory

------------------------------------------------------------------------------*/
void EncAsicMemFree_V2(asicData_s * asic)
{
    ASSERT(asic != NULL);

    if(asic->internalImageLuma[0].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageLuma[0]);

    if(asic->internalImageLuma[1].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageLuma[1]);

    if(asic->internalImageChroma[0].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageChroma[0]);

    if(asic->internalImageChroma[1].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageChroma[1]);

    if(asic->internalImageChroma[2].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageChroma[2]);

    if(asic->sizeTbl.nal.virtualAddress != NULL)
        EWLFreeLinear(asic->ewl, &asic->sizeTbl.nal);

    if(asic->cabacCtx.virtualAddress != NULL)
        EWLFreeLinear(asic->ewl, &asic->cabacCtx);

    if(asic->mvOutput.virtualAddress != NULL)
        EWLFreeLinear(asic->ewl, &asic->mvOutput);

    asic->internalImageLuma[0].virtualAddress = NULL;
    asic->internalImageLuma[1].virtualAddress = NULL;
    asic->internalImageChroma[0].virtualAddress = NULL;
    asic->internalImageChroma[1].virtualAddress = NULL;
    asic->internalImageChroma[2].virtualAddress = NULL;
    asic->sizeTbl.nal.virtualAddress = NULL;
    asic->cabacCtx.virtualAddress = NULL;
    asic->mvOutput.virtualAddress = NULL;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
i32 EncAsicCheckStatus_V2(asicData_s * asic)
{
    i32 ret;
    u32 status;

    status = EncAsicGetStatus(asic->ewl);

    if(status & ASIC_STATUS_ERROR)
    {
        ret = ASIC_STATUS_ERROR;

        EWLReleaseHw(asic->ewl);
    }
    else if(status & ASIC_STATUS_HW_RESET)
    {
        ret = ASIC_STATUS_HW_RESET;

        EWLReleaseHw(asic->ewl);
    }
    else if(status & ASIC_STATUS_FRAME_READY)
    {
        regValues_s *regs = &asic->regs;

        EncAsicGetRegisters(asic->ewl, regs);

        ret = ASIC_STATUS_FRAME_READY;

        EWLReleaseHw(asic->ewl);
    }
    else if(status & ASIC_STATUS_BUFF_FULL)
    {
        ret = ASIC_STATUS_BUFF_FULL;
        /* we do not support recovery from buffer full situation so */
        /* ASIC has to be stopped                                   */
        EncAsicStop(asic->ewl);
        EWLReleaseHw(asic->ewl);
    }
    else /* Don't check SLICE_READY status bit, it is reseted by IRQ handler */
    {
        /* IRQ received but none of the status bits is high, so it must be
         * slice ready. */
        ret = ASIC_STATUS_SLICE_READY;
    }

    return ret;
}
