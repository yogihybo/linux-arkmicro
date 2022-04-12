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
--  Description :  Encoder SW/HW interface register definitions
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. External compiler flags
    3. Module defines

------------------------------------------------------------------------------*/
#ifndef ENC_SWHWREGISTERS_H
#define ENC_SWHWREGISTERS_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define ASIC_SWREG_AMOUNT               96

#define ASIC_INPUT_YUV420PLANAR         0x00
#define ASIC_INPUT_YUV420SEMIPLANAR     0x01
#define ASIC_INPUT_YUYV422INTERLEAVED   0x02
#define ASIC_INPUT_UYVY422INTERLEAVED   0x03
#define ASIC_INPUT_RGB565               0x04
#define ASIC_INPUT_RGB555               0x05
#define ASIC_INPUT_RGB444               0x06
#define ASIC_INPUT_RGB888               0x07
#define ASIC_INPUT_RGB101010            0x08

/* HW Register field names */
typedef enum {
    HEncProductID,
    HEncProductMajor,
    HEncProductMinor,
    HEncProductBuild,

    HEncIRQSliceReady,
    HEncIRQTest1,
    HEncIRQTest2,
    HEncIRQBuffer,
    HEncIRQReset,
    HEncIRQBusError,
    HEncIRQFrameReady,
    HEncIRQDisable,
    HEncIRQ,

    HEncAXIWriteID,
    HEncAXIReadID,
    HEncOutputSwap16,
    HEncInputSwap16,
    HEncBurstLength,
    HEncBurstDisable,
    HEncBurstIncr,
    HEncDataDiscard,
    HEncClockGating,
    HEncOutputSwap32,
    HEncInputSwap32,
    HEncOutputSwap8,
    HEncInputSwap8,

    HEncTestCounter,
    HEncTestLength,
    HEncTestMem,
    HEncTestReg,
    HEncTestIrq,

    HEncBaseStream,
    HEncBaseControl,
    HEncBaseRefLum,
    HEncBaseRefChr,
    HEncBaseRecLum,
    HEncBaseRecChr,
    HEncBaseInLum,
    HEncBaseInCb,
    HEncBaseInCr,

    HEncIntTimeout,
    HEncMvWrite,
    HEncNalMode,
    HEncIntSliceReady,
    HEncWidth,
    HEncHeight,
    HEncRecWriteDisable,
    HEncPictureType,
    HEncEncodingMode,
    HEncEnable,

    HEncChrOffset,
    HEncLumOffset,
    HEncRowLength,
    HEncXFill,
    HEncYFill,
    HEncInputFormat,
    HEncInputRot,

    HEncPicInitQp,
    HEncSliceAlpha,
    HEncSliceBeta,
    HEncChromaQp,
    HEncDeblocking,
    HEncIdrPicId,
    HEncConstrIP,

    HEncPPSID,
    HEncIPPrevModeFavor,
    HEncIPIntra16Favor,

    HEncSliceSize,
    HEncDisableQPMV,
    HEncTransform8x8,
    HEncCabacInitIdc,
    HEncCabacEnable,
    HEncInter4Restrict,
    HEncStreamMode,
    HEncFrameNum,

    HEncDMVPenalty,
    HEncDMVPenalty4p,

    HEncJpegMode,
    HEncJpegSlice,
    HEncJpegRSTInt,
    HEncJpegRST,

    HEncSkipPenalty,
    HEncNumSlicesReady,
    HEncInterFavor,

    HEncStrmHdrRem1,
    HEncStrmHdrRem2,

    HEncStrmBufLimit,

    HEncMadQpDelta,
    HEncMadThreshold,
    HEncQpSum,

    HEncQp,
    HEncMaxQp,
    HEncMinQp,
    HEncCPDist,

    HEncCP1WordTarget,
    HEncCP2WordTarget,
    HEncCP3WordTarget,
    HEncCP4WordTarget,
    HEncCP5WordTarget,
    HEncCP6WordTarget,
    HEncCP7WordTarget,
    HEncCP8WordTarget,
    HEncCP9WordTarget,
    HEncCP10WordTarget,

    HEncCPWordError1,
    HEncCPWordError2,
    HEncCPWordError3,
    HEncCPWordError4,
    HEncCPWordError5,
    HEncCPWordError6,

    HEncCPDeltaQp1,
    HEncCPDeltaQp2,
    HEncCPDeltaQp3,
    HEncCPDeltaQp4,
    HEncCPDeltaQp5,
    HEncCPDeltaQp6,
    HEncCPDeltaQp7,

    HEncStartOffset,
    HEncRlcSum,
    HEncMadCount,
    HEncMbCount,

    HEncBaseNextLum,

    HEncStabMode,
    HEncStabMinimum,
    HEncStabMotionSum,
    HEncStabGmvX,
    HEncStabMatrix1,
    HEncStabGmvY,
    HEncStabMatrix2,
    HEncStabMatrix3,
    HEncStabMatrix4,
    HEncStabMatrix5,
    HEncStabMatrix6,
    HEncStabMatrix7,
    HEncStabMatrix8,
    HEncStabMatrix9,

    HEncBaseCabacCtx,
    HEncBaseMvWrite,

    HEncRGBCoeffA,
    HEncRGBCoeffB,
    HEncRGBCoeffC,
    HEncRGBCoeffE,
    HEncRGBCoeffF,

    HEncRMaskMSB,
    HEncGMaskMSB,
    HEncBMaskMSB,

    HEncIntraAreaLeft,
    HEncIntraAreaRight,
    HEncIntraAreaTop,
    HEncIntraAreaBottom,

    HEncCirStart,
    HEncCirInterval,

    HEncIntraSliceMap1,
    HEncIntraSliceMap2,
    HEncIntraSliceMap3,

    HEncRoi1Left,
    HEncRoi1Right,
    HEncRoi1Top,
    HEncRoi1Bottom,

    HEncRoi2Left,
    HEncRoi2Right,
    HEncRoi2Top,
    HEncRoi2Bottom,

    HEncRoi1DeltaQp,
    HEncRoi2DeltaQp,

    HEncMvcPriorityId,
    HEncMvcViewId,
    HEncMvcTemporalId,
    HEncMvcAnchorPicFlag,
    HEncMvcInterViewFlag,

    HEncHWTiledSupport,
    HEncHWSearchArea,
    HEncHWRgbSupport,
    HEncHWH264Support,
    HEncHWMpeg4Support,
    HEncHWJpegSupport,
    HEncHWStabSupport,
    HEncHWBus,
    HEncHWSynthesisLan,
    HEncHWBusWidth,
    HEncHWMaxVideoWidth,

    HEncJpegQuantLuma1,
    HEncJpegQuantLuma2,
    HEncJpegQuantLuma3,
    HEncJpegQuantLuma4,
    HEncJpegQuantLuma5,
    HEncJpegQuantLuma6,
    HEncJpegQuantLuma7,
    HEncJpegQuantLuma8,
    HEncJpegQuantLuma9,
    HEncJpegQuantLuma10,
    HEncJpegQuantLuma11,
    HEncJpegQuantLuma12,
    HEncJpegQuantLuma13,
    HEncJpegQuantLuma14,
    HEncJpegQuantLuma15,
    HEncJpegQuantLuma16,

    HEncJpegQuantChroma1,
    HEncJpegQuantChroma2,
    HEncJpegQuantChroma3,
    HEncJpegQuantChroma4,
    HEncJpegQuantChroma5,
    HEncJpegQuantChroma6,
    HEncJpegQuantChroma7,
    HEncJpegQuantChroma8,
    HEncJpegQuantChroma9,
    HEncJpegQuantChroma10,
    HEncJpegQuantChroma11,
    HEncJpegQuantChroma12,
    HEncJpegQuantChroma13,
    HEncJpegQuantChroma14,
    HEncJpegQuantChroma15,
    HEncJpegQuantChroma16,

    HEncRegisterAmount

} regName;

/* HW Register field descriptions */
typedef struct {
    i32 name;               /* Register name and index  */
    i32 base;               /* Register base address  */
    u32 mask;               /* Bitmask for this field */
    i32 lsb;                /* LSB for this field [31..0] */
    i32 trace;              /* Enable/disable writing in swreg_params.trc */
    char *description;      /* Field description */
} regField_s;

/* NOTE: Don't use ',' in descriptions, because it is used as separator in csv
 * parsing. */
static const regField_s asicRegisterDesc[] = {
    {HEncProductID        , 0x000, 0xffff0000, 16, 0, "Product ID"},
    {HEncProductMajor     , 0x000, 0x0000f000, 12, 0, "Major number"},
    {HEncProductMinor     , 0x000, 0x00000ff0,  4, 0, "Minor number"},
    {HEncProductBuild     , 0x000, 0x0000000f,  0, 0, "Build number defined in synthesis."},
    {HEncIRQSliceReady    , 0x004, 0x00000100,  8, 0, "IRQ slice ready status bit."},
    {HEncIRQTest1         , 0x004, 0x00000080,  7, 0, "IRQ test 2 status bit. Memory coherency test."},
    {HEncIRQTest2         , 0x004, 0x00000040,  6, 0, "IRQ test 1 status bit. Interrupt test."},
    {HEncIRQBuffer        , 0x004, 0x00000020,  5, 0, "IRQ buffer full status bit. HW waiting for new buffer."},
    {HEncIRQReset         , 0x004, 0x00000010,  4, 0, "IRQ SW reset status bit."},
    {HEncIRQBusError      , 0x004, 0x00000008,  3, 0, "IRQ bus error or timeout status bit."},
    {HEncIRQFrameReady    , 0x004, 0x00000004,  2, 0, "IRQ frame ready status bit. Encoder has finished a frame."},
    {HEncIRQDisable       , 0x004, 0x00000002,  1, 0, "IRQ disable. No interrupts from HW. SW must use polling."},
    {HEncIRQ              , 0x004, 0x00000001,  0, 0, "HINTenc Interrupt from HW. SW resets at IRQ handler."},
    {HEncAXIWriteID       , 0x008, 0xff000000, 24, 0, "AXI Write ID"},
    {HEncAXIReadID        , 0x008, 0x00ff0000, 16, 0, "AXI Read ID"},
    {HEncOutputSwap16     , 0x008, 0x00008000, 15, 0, "Enable output swap 16-bits"},
    {HEncInputSwap16      , 0x008, 0x00004000, 14, 0, "Enable input swap 16-bits"},
    {HEncBurstLength      , 0x008, 0x00003f00,  8, 0, "Burst length. 0=incremental. 4=max BURST4.8=max BURST8. 16=max BURST16"},
    {HEncBurstDisable     , 0x008, 0x00000080,  7, 0, "Disable burst mode for AXI"},
    {HEncBurstIncr        , 0x008, 0x00000040,  6, 0, "Burst incremental. 1=INCR burst allowed. 0=use SINGLE burst"},
    {HEncDataDiscard      , 0x008, 0x00000020,  5, 0, "Enable burst data discard. 2 or 3 long reads are using BURST4"},
    {HEncClockGating      , 0x008, 0x00000010,  4, 0, "Enable clock gating"},
    {HEncOutputSwap32     , 0x008, 0x00000008,  3, 0, "Enable output swap 32-bits"},
    {HEncInputSwap32      , 0x008, 0x00000004,  2, 0, "Enable input swap 32-bits"},
    {HEncOutputSwap8      , 0x008, 0x00000002,  1, 0, "Enable output swap 8-bits"},
    {HEncInputSwap8       , 0x008, 0x00000001,  0, 0, "Enable input swap 8-bits"},
    {HEncTestCounter      , 0x00c, 0xf0000000, 28, 0, "Test counter"},
    {HEncTestLength       , 0x00c, 0x001ffff8,  3, 0, "Test data length for memory test"},
    {HEncTestMem          , 0x00c, 0x00000004,  2, 0, "Enable memory coherency test. Reads BaseStream. Writes BaseControl"},
    {HEncTestReg          , 0x00c, 0x00000002,  1, 0, "Enable register coherency test. Increments test counter"},
    {HEncTestIrq          , 0x00c, 0x00000001,  0, 0, "Enable IRQ test. HW gives interrupt"},
    {HEncBaseStream       , 0x014, 0xffffffff,  0, 0, "Base address for output stream data"},
    {HEncBaseControl      , 0x018, 0xffffffff,  0, 0, "Base address for output control data"},
    {HEncBaseRefLum       , 0x01c, 0xffffffff,  0, 0, "Base address for reference luma"},
    {HEncBaseRefChr       , 0x020, 0xffffffff,  0, 0, "Base address for reference chroma"},
    {HEncBaseRecLum       , 0x024, 0xffffffff,  0, 0, "Base address for reconstructed luma"},
    {HEncBaseRecChr       , 0x028, 0xffffffff,  0, 0, "Base address for reconstructed chroma"},
    {HEncBaseInLum        , 0x02c, 0xffffffff,  0, 0, "Base address for input picture luma"},
    {HEncBaseInCb         , 0x030, 0xffffffff,  0, 0, "Base address for input picture cb"},
    {HEncBaseInCr         , 0x034, 0xffffffff,  0, 0, "Base address for input picture cr"},
    {HEncIntTimeout       , 0x038, 0x80000000, 31, 0, "Enable interrupt for timeout"},
    {HEncMvWrite          , 0x038, 0x40000000, 30, 1, "Enable writing MV and SAD of each MB to BaseMvWrite"},
    {HEncNalMode          , 0x038, 0x20000000, 29, 1, "Enable writing size of each NAL unit to BaseControl, nalSizeWriteOut"},
    {HEncIntSliceReady    , 0x038, 0x10000000, 28, 0, "Enable interrupt for slice ready"},
    {HEncWidth            , 0x038, 0x0ff80000, 19, 1, "Encoded width. lumWidth (macroblocks) H264:[6..255] JPEG:[6..511]"},
    {HEncHeight           , 0x038, 0x0007fc00, 10, 1, "Encoded height. lumHeight (macroblocks) H264:[6..255] JPEG:[2..511]"},
    {HEncRecWriteDisable  , 0x038, 0x00000040,  6, 1, "Disable writing of reconstructed image. recWriteDisable"},
    {HEncPictureType      , 0x038, 0x00000018,  3, 1, "Encoded picture type. frameType. 0=INTER. 1=INTRA(IDR). 2=MVC-INTER. 3=MVC-INTER(ref mod)."},
    {HEncEncodingMode     , 0x038, 0x00000006,  1, 1, "Encoding mode. streamType. 2=JPEG. 3=H264"},
    {HEncEnable           , 0x038, 0x00000001,  0, 0, "Encoder enable"},
    {HEncChrOffset        , 0x03c, 0xe0000000, 29, 0, "Input chrominance offset (bytes) [0..7]"},
    {HEncLumOffset        , 0x03c, 0x1c000000, 26, 0, "Input luminance offset (bytes) [0..7]"},
    {HEncRowLength        , 0x03c, 0x03fff000, 12, 1, "Input luminance row length. lumWidthSrc (bytes) [96..8192]"},
    {HEncXFill            , 0x03c, 0x00000c00, 10, 0, "Overfill pixels on right edge of image div4 [0..3]"},
    {HEncYFill            , 0x03c, 0x000003c0,  6, 1, "Overfill pixels on bottom edge of image. YFill. [0..15]"},
    {HEncInputFormat      , 0x03c, 0x0000003c,  2, 1, "Input image format. inputFormat. YUV420P/YUV420SP/YUYV422/UYVY422/RGB565/RGB555/RGB444/RGB888/RGB101010"},
    {HEncInputRot         , 0x03c, 0x00000003,  0, 1, "Input image rotation. 0=disabled. 1=90degrees right. 2=90 degrees left"},
    {HEncPicInitQp        , 0x040, 0xfc000000, 26, 0, "H.264 Pic init qp in PPS [0..51]"},
    {HEncSliceAlpha       , 0x040, 0x03c00000, 22, 0, "H.264 Slice filter alpha c0 offset div2 [-6..6]"},
    {HEncSliceBeta        , 0x040, 0x003c0000, 18, 0, "H.264 Slice filter beta offset div2 [-6..6]"},
    {HEncChromaQp         , 0x040, 0x0003e000, 13, 0, "H.264 Chroma qp index offset [-12..12]"},
    {HEncDeblocking       , 0x040, 0x00000060,  5, 0, "H.264 Deblocking filter mode. 0=enabled. 1=disabled. 2=disabled on slice borders"},
    {HEncIdrPicId         , 0x040, 0x0000001e,  1, 0, "H.264 IDR picture ID"},
    {HEncConstrIP         , 0x040, 0x00000001,  0, 1, "H.264 Constrained intra prediction enable. constIntraPred"},
    {HEncPPSID            , 0x044, 0xff000000, 24, 0, "H.264 pic_parameter_set_id"},
    {HEncIPPrevModeFavor  , 0x044, 0x00ff0000, 16, 0, "H.264 Intra prediction previous 4x4 mode favor"},
    {HEncIPIntra16Favor   , 0x044, 0x0000ffff,  0, 0, "H.264 Intra prediction intra 16x16 mode favor"},
    {HEncSliceSize        , 0x048, 0x3f800000, 23, 1, "H.264 Slice size. mbRowPerSlice (mb rows) [0..127] 0=one slice per picture"},
    {HEncDisableQPMV      , 0x048, 0x00400000, 22, 1, "H.264 Disable quarter pixel MVs. disableQuarterPixelMv"},
    {HEncTransform8x8     , 0x048, 0x00200000, 21, 1, "H.264 Transform 8x8 enable. High Profile H.264. transform8x8Mode"},
    {HEncCabacInitIdc     , 0x048, 0x00180000, 19, 0, "H.264 CABAC initial IDC. [0..2]"},
    {HEncCabacEnable      , 0x048, 0x00040000, 18, 1, "H.264 CABAC enable. entropyCodingMode. 0=CAVLC (Baseline Profile H.264). 1=CABAC (Main Profile H.264)"},
    {HEncInter4Restrict   , 0x048, 0x00020000, 17, 1, "H.264 Inter 4x4 mode restriction. restricted4x4Mode"},
    {HEncStreamMode       , 0x048, 0x00010000, 16, 1, "H.264 Stream mode. byteStream. 0=NAL unit stream. 1=Byte stream"},
    {HEncFrameNum         , 0x048, 0x0000ffff,  0, 0, "H.264 Frame num"},
    {HEncDMVPenalty       , 0x04c, 0x000003ff,  0, 1, "H.264 Differential MV penalty for 1p/qp ME. DMVPenalty1p"},
    {HEncDMVPenalty4p     , 0x04c, 0x000ffc00, 10, 1, "H.264 Differential MV penalty for 4p ME. DMVPenalty4p"},
    {HEncJpegMode         , 0x050, 0x02000000, 25, 0, "JPEG mode. 0=4:2:0 (4lum+2chr blocks/MCU). 1=4:2:2 (2lum+2chr blocks/MCU)"},
    {HEncJpegSlice        , 0x050, 0x01000000, 24, 0, "JPEG slice enable. 0=picture ends with EOI. 1=slice ends with RST"},
    {HEncJpegRSTInt       , 0x050, 0x00ff0000, 16, 0, "JPEG restart marker interval when slices are disabled (mb rows) [0..255]"},
    {HEncJpegRST          , 0x050, 0x0000ffff,  0, 0, "JPEG restart marker for first RST. incremented by HW for next RST"},
    {HEncSkipPenalty      , 0x054, 0xff000000, 24, 0, "H.264 SKIP macroblock mode penalty"},
    {HEncNumSlicesReady   , 0x054, 0x00ff0000, 16, 0, "H.264 amount of completed slices."},
    {HEncInterFavor       , 0x054, 0x0000ffff,  0, 0, "H.264 Inter MB mode favor in intra/inter selection"},
    {HEncStrmHdrRem1      , 0x058, 0xffffffff,  0, 0, "Stream header remainder bits MSB (MSB aligned)"},
    {HEncStrmHdrRem2      , 0x05c, 0xffffffff,  0, 0, "Stream header remainder bits LSB (MSB aligned)"},
    {HEncStrmBufLimit     , 0x060, 0xffffffff,  0, 1, "Stream buffer limit (64bit addresses) / output stream size (bits). HWStreamDataCount. If limit is reached buffer full IRQ is given."},
    {HEncMadQpDelta       , 0x064, 0xf0000000, 28, 1, "MAD based QP adjustment. madQpChange [-8..7]"},
    {HEncMadThreshold     , 0x064, 0x0fc00000, 22, 0, "MAD threshold div256"},
    {HEncQpSum            , 0x064, 0x001fffff,  0, 0, "QP Sum div2 output"},
    {HEncQp               , 0x06c, 0xfc000000, 26, 1, "Initial QP. qpLum [0..51]"},
    {HEncMaxQp            , 0x06c, 0x03f00000, 20, 1, "Maximum QP. qpMax [0..51]"},
    {HEncMinQp            , 0x06c, 0x000fc000, 14, 1, "Minimum QP. qpMin [0..51]"},
    {HEncCPDist           , 0x06c, 0x00001fff,  0, 0, "Checkpoint distance (mb) 0=disabled [0..8191]"},
    {HEncCP1WordTarget    , 0x070, 0xffff0000, 16, 0, "Checkpoint 1 word target/usage div32 [0..65535]"},
    {HEncCP2WordTarget    , 0x070, 0x0000ffff,  0, 0, "Checkpoint 2 word target/usage div32 [0..65535]"},
    {HEncCP3WordTarget    , 0x074, 0xffff0000, 16, 0, "Checkpoint 3 word target/usage div32 [0..65535]"},
    {HEncCP4WordTarget    , 0x074, 0x0000ffff,  0, 0, "Checkpoint 4 word target/usage div32 [0..65535]"},
    {HEncCP5WordTarget    , 0x078, 0xffff0000, 16, 0, "Checkpoint 5 word target/usage div32 [0..65535]"},
    {HEncCP6WordTarget    , 0x078, 0x0000ffff,  0, 0, "Checkpoint 6 word target/usage div32 [0..65535]"},
    {HEncCP7WordTarget    , 0x07c, 0xffff0000, 16, 0, "Checkpoint 7 word target/usage div32 [0..65535]"},
    {HEncCP8WordTarget    , 0x07c, 0x0000ffff,  0, 0, "Checkpoint 8 word target/usage div32 [0..65535]"},
    {HEncCP9WordTarget    , 0x080, 0xffff0000, 16, 0, "Checkpoint 9 word target/usage div32 [0..65535]"},
    {HEncCP10WordTarget   , 0x080, 0x0000ffff,  0, 0, "Checkpoint 10 word target/usage div32 [0..65535]"},
    {HEncCPWordError1     , 0x084, 0xffff0000, 16, 0, "Checkpoint word error 1 div4 [-32768..32767]"},
    {HEncCPWordError2     , 0x084, 0x0000ffff,  0, 0, "Checkpoint word error 2 div4 [-32768..32767]"},
    {HEncCPWordError3     , 0x088, 0xffff0000, 16, 0, "Checkpoint word error 3 div4 [-32768..32767]"},
    {HEncCPWordError4     , 0x088, 0x0000ffff,  0, 0, "Checkpoint word error 4 div4 [-32768..32767]"},
    {HEncCPWordError5     , 0x08c, 0xffff0000, 16, 0, "Checkpoint word error 5 div4 [-32768..32767]"},
    {HEncCPWordError6     , 0x08c, 0x0000ffff,  0, 0, "Checkpoint word error 6 div4 [-32768..32767]"},
    {HEncCPDeltaQp1       , 0x090, 0x0f000000, 24, 0, "Checkpoint delta QP 1 [-8..7]"},
    {HEncCPDeltaQp2       , 0x090, 0x00f00000, 20, 0, "Checkpoint delta QP 2 [-8..7]"},
    {HEncCPDeltaQp3       , 0x090, 0x000f0000, 16, 0, "Checkpoint delta QP 3 [-8..7]"},
    {HEncCPDeltaQp4       , 0x090, 0x0000f000, 12, 0, "Checkpoint delta QP 4 [-8..7]"},
    {HEncCPDeltaQp5       , 0x090, 0x00000f00,  8, 0, "Checkpoint delta QP 5 [-8..7]"},
    {HEncCPDeltaQp6       , 0x090, 0x000000f0,  4, 0, "Checkpoint delta QP 6 [-8..7]"},
    {HEncCPDeltaQp7       , 0x090, 0x0000000f,  0, 0, "Checkpoint delta QP 7 [-8..7]"},
    {HEncStartOffset      , 0x094, 0x1f800000, 23, 0, "Stream start offset = amount of StrmHdrRem (bits) [0..63]"},
    {HEncRlcSum           , 0x094, 0x007fffff,  0, 0, "RLC codeword count div4 output. max 255*255*384/4"},
    {HEncMadCount         , 0x098, 0xffff0000, 16, 0, "Macroblock count with MAD value under threshold output"},
    {HEncMbCount          , 0x098, 0x0000ffff,  0, 0, "MB count output. max 255*255"},
    {HEncBaseNextLum      , 0x09c, 0xffffffff,  0, 0, "Base address for next pic luminance"},
    {HEncStabMode         , 0x0a0, 0xc0000000, 30, 1, "Stabilization mode. 0=disabled. 1=stab only. 2=stab+encode"},
    {HEncStabMinimum      , 0x0a0, 0x00ffffff,  0, 0, "Stabilization minimum value output. max 253*253*255"},
    {HEncStabMotionSum    , 0x0a4, 0xffffffff,  0, 0, "Stabilization motion sum div8 output. max 253*253*255*1089/8"},
    {HEncStabGmvX         , 0x0a8, 0xfc000000, 26, 0, "Stabilization GMV horizontal output [-16..16]"},
    {HEncStabMatrix1      , 0x0a8, 0x00ffffff,  0, 0, "Stabilization matrix 1 (up-left position) output"},
    {HEncStabGmvY         , 0x0ac, 0xfc000000, 26, 0, "Stabilization GMV vertical output [-16..16]"},
    {HEncStabMatrix2      , 0x0ac, 0x00ffffff,  0, 0, "Stabilization matrix 2 (up position) output"},
    {HEncStabMatrix3      , 0x0b0, 0x00ffffff,  0, 0, "Stabilization matrix 3 (up-right position) output"},
    {HEncStabMatrix4      , 0x0b4, 0x00ffffff,  0, 0, "Stabilization matrix 4 (left position) output"},
    {HEncStabMatrix5      , 0x0b8, 0x00ffffff,  0, 0, "Stabilization matrix 5 (GMV position) output"},
    {HEncStabMatrix6      , 0x0bc, 0x00ffffff,  0, 0, "Stabilization matrix 6 (right position) output"},
    {HEncStabMatrix7      , 0x0c0, 0x00ffffff,  0, 0, "Stabilization matrix 7 (down-left position) output"},
    {HEncStabMatrix8      , 0x0c4, 0x00ffffff,  0, 0, "Stabilization matrix 8 (down position) output"},
    {HEncStabMatrix9      , 0x0c8, 0x00ffffff,  0, 0, "Stabilization matrix 9 (down-right position) output"},
    {HEncBaseCabacCtx     , 0x0cc, 0xffffffff,  0, 0, "Base address for cabac context tables"},
    {HEncBaseMvWrite      , 0x0d0, 0xffffffff,  0, 0, "Base address for MV output writing"},
    {HEncRGBCoeffA        , 0x0d4, 0x0000ffff,  0, 0, "RGB to YUV conversion coefficient A"},
    {HEncRGBCoeffB        , 0x0d4, 0xffff0000, 16, 0, "RGB to YUV conversion coefficient B"},
    {HEncRGBCoeffC        , 0x0d8, 0x0000ffff,  0, 0, "RGB to YUV conversion coefficient C"},
    {HEncRGBCoeffE        , 0x0d8, 0xffff0000, 16, 0, "RGB to YUV conversion coefficient E"},
    {HEncRGBCoeffF        , 0x0dc, 0x0000ffff,  0, 0, "RGB to YUV conversion coefficient F"},
    {HEncRMaskMSB         , 0x0dc, 0x001f0000, 16, 0, "RGB R-component mask MSB bit position [0..31]"},
    {HEncGMaskMSB         , 0x0dc, 0x03e00000, 21, 0, "RGB G-component mask MSB bit position [0..31]"},
    {HEncBMaskMSB         , 0x0dc, 0x7c000000, 26, 0, "RGB B-component mask MSB bit position [0..31]"},
    {HEncIntraAreaLeft    , 0x0e0, 0xff000000, 24, 0, "Intra area left mb column (inside area) [0..255]"},
    {HEncIntraAreaRight   , 0x0e0, 0x00ff0000, 16, 0, "Intra area right mb column (outside area) [0..255]"},
    {HEncIntraAreaTop     , 0x0e0, 0x0000ff00,  8, 0, "Intra area top mb row (inside area) [0..255]"},
    {HEncIntraAreaBottom  , 0x0e0, 0x000000ff,  0, 0, "Intra area bottom mb row (outside area) [0..255]"},
    {HEncCirStart         , 0x0e4, 0xffff0000, 16, 0, "CIR first intra mb. 0=disabled [0..65535]"},
    {HEncCirInterval      , 0x0e4, 0x0000ffff,  0, 0, "CIR intra mb interval. 0=disabled [0..65535]"},
    {HEncIntraSliceMap1   , 0x0e8, 0xffffffff,  0, 0, "Intra slice bitmap for slices 0..31. LSB=slice0. MSB=slice31. 1=intra."},
    {HEncIntraSliceMap2   , 0x0ec, 0xffffffff,  0, 0, "Intra slice bitmap for slices 32..63. LSB=slice32. MSB=slice63. 1=intra."},
    {HEncIntraSliceMap3   , 0x068, 0xffffffff,  0, 0, "Intra slice bitmap for slices 64..95. LSB=slice64. MSB=slice95. 1=intra."},
    {HEncRoi1Left         , 0x0f0, 0xff000000, 24, 0, "1st ROI area left mb column (inside area)"},
    {HEncRoi1Right        , 0x0f0, 0x00ff0000, 16, 0, "1st ROI area right mb column (inside area)"},
    {HEncRoi1Top          , 0x0f0, 0x0000ff00,  8, 0, "1st ROI area top mb row (inside area)"},
    {HEncRoi1Bottom       , 0x0f0, 0x000000ff,  0, 0, "1st ROI area bottom mb row (inside area)"},
    {HEncRoi2Left         , 0x0f4, 0xff000000, 24, 0, "2nd ROI area left mb column (inside area)"},
    {HEncRoi2Right        , 0x0f4, 0x00ff0000, 16, 0, "2nd ROI area right mb column (inside area)"},
    {HEncRoi2Top          , 0x0f4, 0x0000ff00,  8, 0, "2nd ROI area top mb row (inside area)"},
    {HEncRoi2Bottom       , 0x0f4, 0x000000ff,  0, 0, "2nd ROI area bottom mb row (inside area)"},
    {HEncRoi1DeltaQp      , 0x0f8, 0x000000f0,  4, 0, "1st ROI area delta QP. qp = Qp - Roi1DeltaQp [0..15]"},
    {HEncRoi2DeltaQp      , 0x0f8, 0x0000000f,  0, 0, "2nd ROI area delta QP. qp = Qp - Roi2DeltaQp [0..15]"},
    {HEncMvcPriorityId    , 0x0f8, 0x00070000, 16, 0, "MVC priority_id [0..7]"},
    {HEncMvcViewId        , 0x0f8, 0x0000e000, 13, 0, "MVC view_id [0..7]"},
    {HEncMvcTemporalId    , 0x0f8, 0x00001c00, 10, 0, "MVC temporal_id [0..7]"},
    {HEncMvcAnchorPicFlag , 0x0f8, 0x00000200,  9, 0, "MVC anchor_pic_flag. Specifies that the picture is part of an anchor access unit."},
    {HEncMvcInterViewFlag , 0x0f8, 0x00000100,  8, 0, "MVC inter_view_flag. Specifies that the picture is used for inter-view prediction."},
    {HEncHWTiledSupport   , 0x0fc, 0x40000000, 30, 0, "Tiled 4x4 input mode supported by HW. 0=not supported. 1=supported"},
    {HEncHWSearchArea     , 0x0fc, 0x20000000, 29, 0, "HW search area height. 0=5 MB rows. 1=3 MB rows"},
    {HEncHWRgbSupport     , 0x0fc, 0x10000000, 28, 0, "RGB to YUV conversion supported by HW. 0=not supported. 1=supported"},
    {HEncHWH264Support    , 0x0fc, 0x08000000, 27, 0, "H.264 encoding supported by HW. 0=not supported. 1=supported"},
    {HEncHWMpeg4Support   , 0x0fc, 0x04000000, 26, 0, "MPEG-4 encoding supported by HW. 0=not supported. 1=supported"},
    {HEncHWJpegSupport    , 0x0fc, 0x02000000, 25, 0, "JPEG encoding supported by HW. 0=not supported. 1=supported"},
    {HEncHWStabSupport    , 0x0fc, 0x01000000, 24, 0, "Stabilization supported by HW. 0=not supported. 1=supported"},
    {HEncHWBus            , 0x0fc, 0x00f00000, 20, 0, "Bus connection of HW. 1=AHB. 2=OCP. 3=AXI. 4=PCI. 5=AXIAHB. 6=AXIAPB."},
    {HEncHWSynthesisLan   , 0x0fc, 0x000f0000, 16, 0, "Synthesis language. 1=vhdl. 2=verilog"},
    {HEncHWBusWidth       , 0x0fc, 0x0000f000, 12, 0, "Bus width of HW. 0=32b. 1=64b. 2=128b"},
    {HEncHWMaxVideoWidth  , 0x0fc, 0x00000fff,  0, 0, "Maximum video width supported by HW (pixels)"},
    {HEncJpegQuantLuma1   , 0x100, 0xffffffff,  0, 0, "JPEG luma quantization 1"},
    {HEncJpegQuantLuma2   , 0x104, 0xffffffff,  0, 0, "JPEG luma quantization 2"},
    {HEncJpegQuantLuma3   , 0x108, 0xffffffff,  0, 0, "JPEG luma quantization 3"},
    {HEncJpegQuantLuma4   , 0x10c, 0xffffffff,  0, 0, "JPEG luma quantization 4"},
    {HEncJpegQuantLuma5   , 0x110, 0xffffffff,  0, 0, "JPEG luma quantization 5"},
    {HEncJpegQuantLuma6   , 0x114, 0xffffffff,  0, 0, "JPEG luma quantization 6"},
    {HEncJpegQuantLuma7   , 0x118, 0xffffffff,  0, 0, "JPEG luma quantization 7"},
    {HEncJpegQuantLuma8   , 0x11c, 0xffffffff,  0, 0, "JPEG luma quantization 8"},
    {HEncJpegQuantLuma9   , 0x120, 0xffffffff,  0, 0, "JPEG luma quantization 9"},
    {HEncJpegQuantLuma10  , 0x124, 0xffffffff,  0, 0, "JPEG luma quantization 10"},
    {HEncJpegQuantLuma11  , 0x128, 0xffffffff,  0, 0, "JPEG luma quantization 11"},
    {HEncJpegQuantLuma12  , 0x12c, 0xffffffff,  0, 0, "JPEG luma quantization 12"},
    {HEncJpegQuantLuma13  , 0x130, 0xffffffff,  0, 0, "JPEG luma quantization 13"},
    {HEncJpegQuantLuma14  , 0x134, 0xffffffff,  0, 0, "JPEG luma quantization 14"},
    {HEncJpegQuantLuma15  , 0x138, 0xffffffff,  0, 0, "JPEG luma quantization 15"},
    {HEncJpegQuantLuma16  , 0x13c, 0xffffffff,  0, 0, "JPEG luma quantization 16"},
    {HEncJpegQuantChroma1 , 0x140, 0xffffffff,  0, 0, "JPEG chroma quantization 1"},
    {HEncJpegQuantChroma2 , 0x144, 0xffffffff,  0, 0, "JPEG chroma quantization 2"},
    {HEncJpegQuantChroma3 , 0x148, 0xffffffff,  0, 0, "JPEG chroma quantization 3"},
    {HEncJpegQuantChroma4 , 0x14c, 0xffffffff,  0, 0, "JPEG chroma quantization 4"},
    {HEncJpegQuantChroma5 , 0x150, 0xffffffff,  0, 0, "JPEG chroma quantization 5"},
    {HEncJpegQuantChroma6 , 0x154, 0xffffffff,  0, 0, "JPEG chroma quantization 6"},
    {HEncJpegQuantChroma7 , 0x158, 0xffffffff,  0, 0, "JPEG chroma quantization 7"},
    {HEncJpegQuantChroma8 , 0x15c, 0xffffffff,  0, 0, "JPEG chroma quantization 8"},
    {HEncJpegQuantChroma9 , 0x160, 0xffffffff,  0, 0, "JPEG chroma quantization 9"},
    {HEncJpegQuantChroma10, 0x164, 0xffffffff,  0, 0, "JPEG chroma quantization 10"},
    {HEncJpegQuantChroma11, 0x168, 0xffffffff,  0, 0, "JPEG chroma quantization 11"},
    {HEncJpegQuantChroma12, 0x16c, 0xffffffff,  0, 0, "JPEG chroma quantization 12"},
    {HEncJpegQuantChroma13, 0x170, 0xffffffff,  0, 0, "JPEG chroma quantization 13"},
    {HEncJpegQuantChroma14, 0x174, 0xffffffff,  0, 0, "JPEG chroma quantization 14"},
    {HEncJpegQuantChroma15, 0x178, 0xffffffff,  0, 0, "JPEG chroma quantization 15"},
    {HEncJpegQuantChroma16, 0x17C, 0xffffffff,  0, 0, "JPEG chroma quantization 16"}
};

#endif
