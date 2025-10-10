#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "dwl.h"
#include "h264decapi.h"
#include "rvdecapi.h"
#include "rv_container.h"
#include "on2rvdecapi.h"
#include "jpegdecapi.h"
#include "jpegdeccontainer.h"
#include "mpeg2decapi.h"
#include "mp4decapi.h"
#include "vc1decapi.h"
#include "vp8decapi.h"
#include "vp6decapi.h"
#include "memalloc.h"
#include "ppapi.h"
#include "mfcapi.h"

#define PP_OUTBUF_NUM	4

#define MEMALLOC_MODULE_PATH	"/dev/memalloc"

u32 g_picDecodeNumber = 0;

u32 g_JpegYBusAddress = 0;
u32 g_JpegCbCrBusAddress = 0;

DWLLinearMem_t g_JpegYMemInfo[2] = {0};
int g_JpegMemallocFd = -1;
u32 g_JpegMemWidth = 0;
u32 g_JpegMemHeight = 0;
u32 g_JpegMemIndex = 0;

static codecSegmentInfo g_SliceInfo[128];

static int JpegAllocBuffer(u32 size);

static void JpegReleaseBuffer(void);

static int get_pp_dec_type(int streamType);

static int pp_alloc_buffer(int fdmemalloc, u32 size, DWLLinearMem_t * info);

static void pp_free_buffer(int fdmemalloc, DWLLinearMem_t * info);

static int pp_set_config(MFCHandle *handle, int in_width, int in_height, int in_format, int out_addr);

static int pp_reset_outimg_addr(MFCHandle *handle, u32 addr);

#define PP_OUTBUFFER_PHYADDR(index)	(handle->ppOutBuffer.busAddress + \
	handle->ppOutWidth * handle->ppOutHeight * 4 * (index))
#define PP_OUTBUFFER_VIRADDR(index)	((u32)handle->ppOutBuffer.virtualAddress + \
	handle->ppOutWidth * handle->ppOutHeight * 4 * (index))

static int ark_interlaced = 0;

int MFC_Get_Interlaced(void)
{
	return ark_interlaced;
}

int MFCH264Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	H264DecRet ret;
	H264DecRet infoRet;
	H264DecRet picRet;
	H264DecInput decIn;
	H264DecOutput decOut;
	static H264DecInfo decInfo;
	H264DecPicture decPic;

	decIn.dataLen = inBuffer->size;
	decIn.pStream = (u8*)inBuffer->virtualAddress;
	decIn.streamBusAddress = inBuffer->busAddress;

	do
	{
		/* Picture ID is the picture number in decoding order */
		decIn.picId = g_picDecodeNumber;
		/* decode the stream data */
		ret = H264DecDecode(handle->decInst, &decIn, &decOut);
		switch(ret)
		{
		case H264DEC_HDRS_RDY:
			/* read stream info */
			infoRet = H264DecGetInfo(handle->decInst, &decInfo);
			outBuffer->codedWidth = decInfo.cropParams.cropOutWidth;
			outBuffer->codedHeight = decInfo.cropParams.cropOutHeight;
			if (handle->ppInst) {
				pp_set_config(handle, decInfo.picWidth, decInfo.picHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
			break;

		case H264DEC_ADVANCED_TOOLS:
			/* Arbitrary Slice Order/multiple slice groups noticed */
			/* in the stream. The decoder has to reallocate resources */
			break;

		case H264DEC_PIC_DECODED: /* a picture was decoded */
			g_picDecodeNumber++;

			if (H264DecNextPicture(handle->decInst, &decPic, 0) == H264DEC_PIC_RDY) {
				ark_interlaced = decPic.interlaced;

				if(outBuffer->num >= MAX_OUTFRAME_NUM)
					return 0;

				outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.isIdrPicture;
				if (!handle->ppInst) {
					outBuffer->buffer[outBuffer->num].yBusAddress =
						decPic.outputPictureBusAddress;
					outBuffer->buffer[outBuffer->num].pyVirAddress =
						decPic.pOutputPicture;
					outBuffer->frameWidth = decInfo.picWidth;
					outBuffer->frameHeight = decInfo.picHeight;
					outBuffer->num++;
				} else if (PPGetResult(handle->ppInst) == PP_OK) {
					outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
					outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
					outBuffer->frameWidth = handle->ppOutWidth;
					outBuffer->frameHeight = handle->ppOutHeight;
					outBuffer->num++;

					handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
					pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
				}
			}

			break;

		case H264DEC_PENDING_FLUSH:
			g_picDecodeNumber++;

			do {
    			picRet = H264DecNextPicture(handle->decInst, &decPic, 0);
    			if (picRet != H264DEC_PIC_RDY) {
    				break;
    			}
				if(outBuffer->num >= MAX_OUTFRAME_NUM)
					return 0;
				outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.isIdrPicture;
				if (!handle->ppInst) {
					outBuffer->buffer[outBuffer->num].yBusAddress =
						decPic.outputPictureBusAddress;
					outBuffer->buffer[outBuffer->num].pyVirAddress =
						decPic.pOutputPicture;
					outBuffer->frameWidth = decInfo.picWidth;
					outBuffer->frameHeight = decInfo.picHeight;
					outBuffer->num++;
				} else if (PPGetResult(handle->ppInst) == PP_OK) {
					outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
					outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
					outBuffer->frameWidth = handle->ppOutWidth;
					outBuffer->frameHeight = handle->ppOutHeight;
					outBuffer->num++;

					handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
					pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
				}
  			} while (picRet == H264DEC_PIC_RDY);

			break;

		case H264DEC_STRM_PROCESSED:
			/* Input stream was processed but no picture is ready */
			break;

		case H264DEC_MEMFAIL:
			return -1;

		default:
			/* all other cases are errors where decoding cannot continue */
			return -1;
		}

		if(decOut.dataLeft == 0)
		{
			break;
		}
		else
		{
			/* data left undecoded */
			decIn.dataLen = decOut.dataLeft;
			decIn.pStream = decOut.pStrmCurrPos;
			decIn.streamBusAddress = decOut.strmCurrBusAddress;
		}
	}while ((ret != H264DEC_STRM_PROCESSED) && (decOut.dataLeft > 0));

	return 0;
}

int MFCH264DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	H264DecPicture decPic;

	if (H264DecNextPicture(handle->decInst, &decPic, 1) == H264DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.isIdrPicture;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputPictureBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputPicture;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

typedef struct tagSliceInfo
{
	u32 bValid;
	u32 nSliceOffset;
}SlicInfo;

typedef struct tagFrameHead
{
	u32 nDataLen;
	u32 nTimeStamp;
	u16 nSeqNum;
	u16 flag;
	u32 nLastPacket;
	u32 nNumSlices;
//	SlicInfo SlicData[0];
}FrameHead;

typedef struct tagSequanceHeader
{
	u8 format[8];
	u32 four_cc_coded;
	u16 FrameWidth;
	u16 FrameHeight;
	u16 BitCount;
	u16 PadWidth;
	u16 PadHeight;
	u32 FrameRate;
	u32 OpaqueData;
	u32 StreamVersion;
}SequanceHeader;

u32 ReadDWord(u8 ** ppSeqBitStream)
{
	u8 *pSeqBitStream;
	u32 ret;

	pSeqBitStream = *ppSeqBitStream;

	ret = pSeqBitStream[0] << 24 | pSeqBitStream[1] << 16 | pSeqBitStream[2] << 8 | pSeqBitStream[3];

	*ppSeqBitStream += 4;
	return ret;
}

u32 ReadDWord_LE(u8 ** ppSeqBitStream)
{
	u8 *pSeqBitStream;
	u32 ret;

	pSeqBitStream = *ppSeqBitStream;

	ret = pSeqBitStream[0] | pSeqBitStream[1] << 8 | pSeqBitStream[2] << 16 | pSeqBitStream[3] << 24;

	*ppSeqBitStream += 4;
	return ret;
}

u16 ReadWord(u8 ** ppSeqBitStream)
{
	u8 *pSeqBitStream;
	u16 ret;

	pSeqBitStream = *ppSeqBitStream;

	ret = pSeqBitStream[0] << 8 | pSeqBitStream[1];

	*ppSeqBitStream += 2;
	return ret;
}

u8 ReadByte(u8 ** ppSeqBitStream)
{
	u8 *pSeqBitStream;
	u8 ret;

	pSeqBitStream = *ppSeqBitStream;

	ret = pSeqBitStream[0];

	*ppSeqBitStream += 1;

	return ret;
}

void ParseRVSequanceHeader(u8 *pSeqBitStream, SequanceHeader *pSequanceHeader)
{
	int i;

	for(i=0;i<8;i++)
	{
		pSequanceHeader->format[i] = ReadByte(&pSeqBitStream);
	}
	pSequanceHeader->four_cc_coded = ReadDWord(&pSeqBitStream);
	pSequanceHeader->FrameWidth = ReadWord(&pSeqBitStream);
	pSequanceHeader->FrameHeight = ReadWord(&pSeqBitStream);
	pSequanceHeader->BitCount = ReadWord(&pSeqBitStream);
	pSequanceHeader->PadWidth = ReadWord(&pSeqBitStream);
	pSequanceHeader->PadHeight = ReadWord(&pSeqBitStream);
	pSequanceHeader->FrameRate = ReadDWord(&pSeqBitStream);
	pSequanceHeader->OpaqueData = ReadDWord(&pSeqBitStream);
	pSequanceHeader->StreamVersion = ReadDWord(&pSeqBitStream);
}

void ParseRVFrameHeader(u8 *pBitStream, FrameHead *pFrameHead)
{
	pFrameHead->nDataLen = 0;
	pFrameHead->nTimeStamp = 0;
	pFrameHead->nSeqNum =  0;
	pFrameHead->flag =  0;
	pFrameHead->nLastPacket =  0;
	pFrameHead->nNumSlices =  ReadByte(&pBitStream) + 1;
}

int MFCRvDecode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	On2RvDecRet infoRet;
	RvDecInst       decoder;
	On2DecoderOutParams  decOut ;
	On2DecoderInParams   decIn  ;
	On2DecoderInit       decinit;
	On2RvMsgSetDecoderRprSizes msg_id;

	SequanceHeader stSequanceHeader;
	SequanceHeader *pSequanceHeader;
	FrameHead *pFrameHead;
	FrameHead stFrameHead;
	int i;
	static int bRV8;
	u8 *pBitStream;
	char *pdata = (u8*)inBuffer->virtualAddress;
	DWLLinearMem_t frameBuffer;
	i32 ret;
	RvDecPicture decPic;

	if(handle->delayInit)
	{
		g_picDecodeNumber = 0;
		ParseRVSequanceHeader((u8*)inBuffer->virtualAddress, &stSequanceHeader);
		pSequanceHeader = &stSequanceHeader;
		decinit.pels  = (pSequanceHeader->FrameWidth + 15) & ~15;
		decinit.lines = (pSequanceHeader->FrameHeight + 15) & ~15;
		decinit.nPadHeight = pSequanceHeader->PadHeight;
		decinit.nPadWidth  = pSequanceHeader->PadWidth;
		decinit.ulStreamVersion = pSequanceHeader->StreamVersion;

		if(pSequanceHeader->four_cc_coded == 0x52563430)
		{
			//this is rv9
			pdata += 34;
			bRV8 = 0;
		}
		else
		{
			// this is rv8
			bRV8 = 1;
			pdata += 34;
			msg_id.message_id = ON2RV_MSG_ID_Set_RVDecoder_RPR_Sizes;
			msg_id.num_sizes = ((pSequanceHeader->OpaqueData & 0x00070000) >> 16) + 1;
			msg_id.sizes = (u32*)DWLmalloc(msg_id.num_sizes * 2 * sizeof(u32));
			msg_id.sizes[0] = pSequanceHeader->FrameWidth;
			msg_id.sizes[1] = pSequanceHeader->FrameHeight;

			for(i=1;i<msg_id.num_sizes;i++)
			{
				msg_id.sizes[i*2] = (*pdata) << 2;
				pdata++;

				msg_id.sizes[i*2+1] = (*pdata) << 2;
				pdata++;
			}
			pFrameHead = (FrameHead*)pdata;
		}

		infoRet = On2RvDecInit(&decinit,&decoder);
		if(infoRet !=ON2RVDEC_OK)
		{
			printf("On2RvDecInit failure %d.\n", infoRet);
			return -1;
		}
		handle->decInst = (void*)decoder;
		handle->delayInit = 0;

		if(bRV8)
		{
			infoRet = On2RvDecCustomMessage(&msg_id, handle->decInst);
			if(infoRet !=ON2RVDEC_OK)
			{
				printf("On2RvDecCustomMessage failure.\n");
				return -1;
			}
		}

		if (handle->ppInit) {
			PPInst pp = NULL;
			PPResult ppRet;

			ppRet = PPInit(&pp);
			if(ppRet != PP_OK){
				printf("PPInit fail.\n");
				handle->ppInst = NULL;
			} else {
				handle->ppInst = (void*)pp;
				if (PPDecCombinedModeEnable(pp, handle->decInst, get_pp_dec_type(handle->streamType)) != PP_OK) {
					printf("PPDecCombinedModeEnable fail.\n");
					PPRelease(pp);
					handle->ppInst = NULL;
				}
			}

			if (handle->ppInst) {
				pp_set_config(handle, pSequanceHeader->FrameWidth, pSequanceHeader->FrameHeight, 
					PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
		}	
	}

	ParseRVFrameHeader(pdata, &stFrameHead);
	pFrameHead = &stFrameHead;
	decIn.numDataSegments = pFrameHead->nNumSlices;
	decIn.timestamp = pFrameHead->nTimeStamp;
	decIn.flags = pFrameHead->flag;

	pBitStream = pdata+1;

	for(i=0;i<pFrameHead->nNumSlices;i++)
	{
		g_SliceInfo[i].bIsValid = ReadDWord_LE(&pBitStream); //pFrameHead->SlicData[i].bValid;
		g_SliceInfo[i].ulSegmentOffset = ReadDWord_LE(&pBitStream); //pFrameHead->SlicData[i].nSliceOffset;
	}

	pFrameHead->nDataLen = inBuffer->size - (pBitStream - (u8*)inBuffer->virtualAddress);
	ret = DWLMallocLinear(((DecContainer*)handle->decInst)->dwl, pFrameHead->nDataLen, &frameBuffer);
	if(ret == DWL_ERROR)
	{
		printf("RV malloc failure.\n");
		return -1;
	}
	DWLmemcpy(frameBuffer.virtualAddress, pBitStream, pFrameHead->nDataLen);
	decIn.pDataSegments = g_SliceInfo;
	decIn.dataLength = pFrameHead->nDataLen;
	decIn.streamBusAddr = frameBuffer.busAddress; //stream_bus_address;
	// Start to decode RV Frame
	infoRet = On2RvDecDecode((u8*)frameBuffer.virtualAddress,
					0 , /* unused */
					&decIn,
					&decOut,
					handle->decInst);
	DWLFreeLinear(((DecContainer*)handle->decInst)->dwl, &frameBuffer);

	if(infoRet == ON2RVDEC_OK)
	{
		while(RvDecNextPicture(handle->decInst, &decPic, 0) == RVDEC_PIC_RDY )
		{
			if(outBuffer->num >= MAX_OUTFRAME_NUM)
				break;
			outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.keyPicture;
			if (!handle->ppInst) {
				outBuffer->buffer[outBuffer->num].yBusAddress =
					decPic.outputPictureBusAddress;
				outBuffer->buffer[outBuffer->num].pyVirAddress =
					decPic.pOutputPicture;
				outBuffer->frameWidth = decPic.frameWidth;
				outBuffer->frameHeight = decPic.frameHeight;
				outBuffer->num++;
			} else if (PPGetResult(handle->ppInst) == PP_OK) {
				outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
				outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
				outBuffer->frameWidth = handle->ppOutWidth;
				outBuffer->frameHeight = handle->ppOutHeight;
				outBuffer->num++;

				handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
				pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
			outBuffer->codedWidth = decPic.codedWidth;
			outBuffer->codedHeight = decPic.codedHeight;
		}

		return 0;
	}
	else if (infoRet == ON2RVDEC_OUTOFMEMORY)
	{
		return -1;
	}

	return -1;
}

int MFCRvDecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	RvDecPicture decPic;

	if (RvDecNextPicture(handle->decInst, &decPic, 1) == RVDEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
				decPic.keyPicture;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputPictureBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputPicture;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

int MFCMpeg2Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	Mpeg2DecRet       ret;
	Mpeg2DecRet       infoRet;
	Mpeg2DecInput     decIn;
	Mpeg2DecOutput    decOut;
	Mpeg2DecInfo      decInfo;
	Mpeg2DecPicture   decPic;

	decIn.dataLen = inBuffer->size;
	decIn.pStream = (u8*)inBuffer->virtualAddress; //stream_virtual_address;
	decIn.streamBusAddress = inBuffer->busAddress; //stream_bus_address;

	do
	{
	    /* Picture ID is the picture number in decoding order */
	    decIn.picId = g_picDecodeNumber;

	    /* decode the stream data */
	    ret = Mpeg2DecDecode(handle->decInst,  &decIn,  &decOut);

		switch(ret)
	    {
	    case MPEG2DEC_HDRS_RDY:
	        /* read stream info */
	        infoRet = Mpeg2DecGetInfo(handle->decInst, &decInfo);
			if (handle->ppInst) {
				pp_set_config(handle, decInfo.codedWidth, decInfo.codedHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
	        break;

	    /* a picture was decoded */
	    case MPEG2DEC_PIC_DECODED:

	        /* Increment decoding number after every decoded picture */
	        g_picDecodeNumber++;

			while(Mpeg2DecNextPicture(handle->decInst, &decPic, 0) == MPEG2DEC_PIC_RDY )
			{
				if(decPic.interlaced && decPic.fieldPicture && decPic.topField && !handle->ppInst)
					continue;
				if(outBuffer->num >= MAX_OUTFRAME_NUM)
					return 0;
				outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.keyPicture;
				if (!handle->ppInst) {
					outBuffer->buffer[outBuffer->num].yBusAddress =
						decPic.outputPictureBusAddress;
					outBuffer->buffer[outBuffer->num].pyVirAddress =
						decPic.pOutputPicture;
					outBuffer->frameWidth = decPic.frameWidth;
					outBuffer->frameHeight = decPic.frameHeight;
					outBuffer->num++;
				} else if (PPGetResult(handle->ppInst) == PP_OK) {
					outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
					outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
					outBuffer->frameWidth = handle->ppOutWidth;
					outBuffer->frameHeight = handle->ppOutHeight;
					outBuffer->num++;

					handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
					pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
				}
				outBuffer->codedWidth = decPic.codedWidth;
				outBuffer->codedHeight = decPic.codedHeight;
			}
	        break;

	    case MPEG2DEC_STRM_PROCESSED:
	    case MPEG2DEC_STRM_ERROR:
	        /* Input stream was processed but no picture is ready */
	        break;

	    case MPEG2DEC_MEMFAIL:
	    	return -1;

	    default:
	       /* all other cases are errors where decoding cannot continue */
	       return -1;
	    }

	    if(decOut.dataLeft == 0)
	    {
			break;
	    }
		else
	    {
	        /* data left undecoded */
	        decIn.dataLen =  decOut.dataLeft;
	        decIn.pStream = decOut.pStrmCurrPos;
	        decIn.streamBusAddress = decOut.strmCurrBusAddress;
	    }

	    /* keep decoding until all data from input stream buffer consumed */
	} while(decIn.dataLen > 0);

	return 0;
}

int MFCMpeg2DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	Mpeg2DecPicture   decPic;

	if (Mpeg2DecNextPicture(handle->decInst, &decPic, 1) == MPEG2DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.keyPicture;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputPictureBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputPicture;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

int MFCMp4Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	MP4DecRet       ret;
	MP4DecRet       infoRet;
	MP4DecInput     decIn;
	MP4DecOutput    decOut;
	MP4DecInfo      decInfo;
	MP4DecPicture   decPic;

	decIn.dataLen = inBuffer->size;
	decIn.pStream = (u8*)inBuffer->virtualAddress;//stream_virtual_address;
	decIn.streamBusAddress =inBuffer->busAddress; //stream_bus_address;

	do
	{
		/* Picture ID is the picture number in decoding order */
		decIn.picId = g_picDecodeNumber;

		/* decode the stream data */
		ret = MP4DecDecode(handle->decInst,  &decIn,  &decOut);

		switch(ret)
		{
		case MP4DEC_HDRS_RDY:
			/* read stream info */
			infoRet = MP4DecGetInfo(handle->decInst, &decInfo);
			printf("%d, %d.%d\n", decInfo.codedWidth, decInfo.codedHeight, decInfo.interlacedSequence);
			if (handle->isCustomMp4)
				MP4DecSetInfo(handle->decInst, decInfo.codedWidth, decInfo.codedHeight);
			if (handle->ppInst) {
				pp_set_config(handle, decInfo.codedWidth, decInfo.codedHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
			break;

		case MP4DEC_DP_HDRS_RDY:
			/* Data partitioning used in the stream.
			* The decoder has to reallocate resources */
			break;

		/* a picture was decoded */
		case MP4DEC_PIC_DECODED:

			/* Increment decoding number after every decoded picture */
			g_picDecodeNumber++;

			while(MP4DecNextPicture(handle->decInst, &decPic, 0) == MP4DEC_PIC_RDY )
			{
				if(decPic.interlaced && decPic.fieldPicture && decPic.topField && !handle->ppInst)
					continue;
				if(outBuffer->num >= MAX_OUTFRAME_NUM)
					return 0;
				outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.keyPicture;
				if (!handle->ppInst) {
					outBuffer->buffer[outBuffer->num].yBusAddress =
						decPic.outputPictureBusAddress;
					outBuffer->buffer[outBuffer->num].pyVirAddress =
						decPic.pOutputPicture;
					outBuffer->frameWidth = decPic.frameWidth;
					outBuffer->frameHeight = decPic.frameHeight;
					outBuffer->num++;
				} else if (PPGetResult(handle->ppInst) == PP_OK) {
					outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
					outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
					outBuffer->frameWidth = handle->ppOutWidth;
					outBuffer->frameHeight = handle->ppOutHeight;
					outBuffer->num++;

					handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
					pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
				}
				outBuffer->codedWidth = decPic.codedWidth;
				outBuffer->codedHeight = decPic.codedHeight;
			}
			break;

		case MP4DEC_STRM_PROCESSED:
			break;

		case MP4DEC_MEMFAIL:
			return -1;

		case MP4DEC_STRM_ERROR:
			/* Input stream was processed but no picture is ready */
		default:
			/* all other cases are errors where decoding cannot continue */
			return -1;
		}

		if(decOut.dataLeft == 0)
		{
			break;
		}
		else
		{
			/* data left undecoded */
			decIn.dataLen =  decOut.dataLeft;
			decIn.pStream = decOut.pStrmCurrPos;
			decIn.streamBusAddress = decOut.strmCurrBusAddress;
		}

		/* keep decoding until all data from input stream buffer consumed */
	} while(decIn.dataLen > 0);


	return 0;
}

int MFCMp4DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	MP4DecPicture   decPic;

	if (MP4DecNextPicture(handle->decInst, &decPic, 1) == MP4DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.keyPicture;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputPictureBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputPicture;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

int MFCVp8Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	VP8DecRet       ret;
	VP8DecInput     decInput;
	VP8DecOutput    decOutput;
	VP8DecInfo      info;
	VP8DecPicture   decPic;


    DWLmemset(&decInput, 0, sizeof(decInput));

 	/* Start decode loop */
	decInput.dataLen = inBuffer->size;
	decInput.pStream = (u8*)inBuffer->virtualAddress;//stream_virtual_address;
	decInput.streamBusAddress = inBuffer->busAddress; //stream_bus_address;

    ret = VP8DecDecode(handle->decInst, &decInput, &decOutput);

	if (ret == VP8DEC_HDRS_RDY)
	{
		VP8DecGetInfo(handle->decInst, &info);
		if (handle->ppInst) {
			pp_set_config(handle, info.codedWidth, info.codedHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
				PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}

		ret = VP8DecDecode(handle->decInst, &decInput, &decOutput);
	}

	if (ret == VP8DEC_MEMFAIL)
	{
		return -1;
	}

	while(VP8DecNextPicture(handle->decInst, &decPic, 0) == VP8DEC_PIC_RDY )
	{
		if(outBuffer->num >= MAX_OUTFRAME_NUM)
			break;
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.isIntraFrame;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputFrameBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputFrame;
			outBuffer->frameWidth = decPic.frameWidth;
			outBuffer->frameHeight = decPic.frameHeight;
			outBuffer->num++;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			outBuffer->frameWidth = handle->ppOutWidth;
			outBuffer->frameHeight = handle->ppOutHeight;
			outBuffer->num++;

			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		outBuffer->codedWidth = decPic.codedWidth;
		outBuffer->codedHeight = decPic.codedHeight;
	}

	return 0;
}

int MFCVp8DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	VP8DecPicture   decPic;

	if (VP8DecNextPicture(handle->decInst, &decPic, 1) == VP8DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.isIntraFrame;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputFrameBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputFrame;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

int MFCVp6Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	VP6DecInput input;
	VP6DecOutput output;
	VP6DecRet ret;
	VP6DecPicture decPic;
	unsigned int currentVideoFrame = 0;

 	/* Start decode loop */
	input.dataLen = inBuffer->size;
	input.pStream = (u8*)inBuffer->virtualAddress;
	input.streamBusAddress = inBuffer->busAddress;

	do
	{
	    ret = VP6DecDecode(handle->decInst, &input, &output);

	    if(ret == VP6DEC_HDRS_RDY)
	    {
	        VP6DecInfo decInfo;

	        ret = VP6DecGetInfo(handle->decInst, &decInfo);
	        if (ret != VP6DEC_OK)
	        {
	        	printf("VP6DecGetInfo failure.\n");
				return -1;
	        }
			outBuffer->codedWidth = decInfo.width;
			outBuffer->codedHeight = decInfo.height;
	        ret = VP6DEC_HDRS_RDY;  /* restore */

			if (handle->ppInst) {
				pp_set_config(handle, decInfo.width, decInfo.height, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
	    }

	}
	while(ret == VP6DEC_HDRS_RDY);

	if(ret == VP6DEC_PIC_DECODED)
	{
		while(VP6DecNextPicture(handle->decInst, &decPic, 0) == VP6DEC_PIC_RDY )
		{
			if(outBuffer->num >= MAX_OUTFRAME_NUM)
				break;
			outBuffer->buffer[outBuffer->num].keyPicture =
				decPic.isIntraFrame;
			if (!handle->ppInst) {
				outBuffer->buffer[outBuffer->num].yBusAddress =
					decPic.outputFrameBusAddress;
				outBuffer->buffer[outBuffer->num].pyVirAddress =
					decPic.pOutputFrame;
				outBuffer->frameWidth = decPic.picWidth;
				outBuffer->frameHeight = decPic.picHeight;
				outBuffer->num++;
			} else if (PPGetResult(handle->ppInst) == PP_OK) {
				outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
				outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
				outBuffer->frameWidth = handle->ppOutWidth;
				outBuffer->frameHeight = handle->ppOutHeight;
				outBuffer->num++;

				handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
				pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
		}
		return 0;
	}
	else if (ret == VP6DEC_MEMFAIL)
	{
		return -1;
	}

	return -1;
}

int MFCVp6DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	VP6DecPicture decPic;

	if (VP6DecNextPicture(handle->decInst, &decPic, 1) == VP6DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.isIntraFrame;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputFrameBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputFrame;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}


int MFCVC1Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	VC1DecRet       ret;
	VC1DecRet       infoRet;
	VC1DecInput     decIn;
	VC1DecOutput    decOut;
	VC1DecInfo      decInfo;
	VC1DecPicture   decPic;

	decIn.streamSize = inBuffer->size; //-nMetaDataLen;
	decIn.pStream = (u8*)inBuffer->virtualAddress;//stream_virtual_address;
	decIn.streamBusAddress = inBuffer->busAddress; //stream_bus_address;

	do
	{
		/* Picture ID is the picture number in decoding order */
		decIn.picId = g_picDecodeNumber;

		/* decode the stream data */
		ret = VC1DecDecode(handle->decInst,  &decIn,  &decOut);

		switch(ret)
		{
		case VC1DEC_RESOLUTION_CHANGED:
			VC1DecGetInfo(handle->decInst, &decInfo);
			if (handle->ppInst) {
				pp_set_config(handle, decInfo.codedWidth, decInfo.codedHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
			break;

		case VC1DEC_HDRS_RDY:
			/* read stream info */
			infoRet = VC1DecGetInfo(handle->decInst, &decInfo);
			if (handle->ppInst) {
				pp_set_config(handle, decInfo.codedWidth, decInfo.codedHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
			break;

		/* a picture was decoded */
		case VC1DEC_PIC_DECODED:
			VC1DecGetInfo(handle->decInst, &decInfo);

		/* Increment decoding number after every decoded picture */
			g_picDecodeNumber++;

			while(VC1DecNextPicture(handle->decInst, &decPic, 0) == VC1DEC_PIC_RDY )
			{
				if(decPic.interlaced && decPic.fieldPicture && decPic.topField && !handle->ppInst)
					continue;
				if(outBuffer->num >= MAX_OUTFRAME_NUM)
					return 0;
				outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.keyPicture;
				if (!handle->ppInst) {
					outBuffer->buffer[outBuffer->num].yBusAddress =
						decPic.outputPictureBusAddress;
					outBuffer->buffer[outBuffer->num].pyVirAddress =
						decPic.pOutputPicture;
					outBuffer->frameWidth = decPic.frameWidth;
					outBuffer->frameHeight = decPic.frameHeight;
					outBuffer->num++;
				} else if (PPGetResult(handle->ppInst) == PP_OK) {
					outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
					outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
					outBuffer->frameWidth = handle->ppOutWidth;
					outBuffer->frameHeight = handle->ppOutHeight;
					handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
					pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
					outBuffer->num++;
				}
				outBuffer->codedWidth = decPic.codedWidth;
				outBuffer->codedHeight = decPic.codedHeight;
			}
			break;

		case VC1DEC_STRM_PROCESSED:
		case VC1DEC_STRM_ERROR:
			/* Input stream was processed but no picture is ready */
			break;

		case VC1DEC_MEMFAIL:
			return -1;

		default:
			/* all other cases are errors where decoding cannot continue */
			return -1;
		}

		if(decOut.dataLeft == 0)
		{
			break;
		}
		else
		{
			/* data left undecoded */
			decIn.streamSize =  decOut.dataLeft;
			decIn.pStream = decOut.pStreamCurrPos;
			decIn.streamBusAddress +=  (decOut.pStreamCurrPos - decIn.pStream);
		}

		/* keep decoding until all data from input stream buffer consumed */
	}
	while(decIn.streamSize > 0);

	return 0;
}

int MFCVC1DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	VC1DecPicture   decPic;

	if (VC1DecNextPicture(handle->decInst, &decPic, 1) == VC1DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.keyPicture;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputPictureBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputPicture;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

/* Define RCV format metadata max size. Standard specifies 44 bytes, add one
 * to support some non-compliant streams */
#define RCV_METADATA_MAX_SIZE   (44+1)

/* Global variables for stream handling */
u32 rcvV2;
u32 rcvMetadataSize = 0;

#define SHOW1(p) (p[0]); p+=1;
#define SHOW2(p) (p[0]) | (p[1]<<8); p+=2;
#define SHOW3(p) (p[0]) | (p[1]<<8) | (p[2]<<16); p+=3;
#define SHOW4(p) (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); p+=4;

#define    BIT0(tmp)  ((tmp & 1)   >>0);
#define    BIT1(tmp)  ((tmp & 2)   >>1);
#define    BIT2(tmp)  ((tmp & 4)   >>2);
#define    BIT3(tmp)  ((tmp & 8)   >>3);
#define    BIT4(tmp)  ((tmp & 16)  >>4);
#define    BIT5(tmp)  ((tmp & 32)  >>5);
#define    BIT6(tmp)  ((tmp & 64)  >>6);
#define    BIT7(tmp)  ((tmp & 128) >>7);

/*------------------------------------------------------------------------------

    Function name:  DecodeFrameLayerData

     Purpose:
     Decodes initialization frame layer from rcv format.

      Returns:
      Frame size in bytes.

------------------------------------------------------------------------------*/
u32 DecodeFrameLayerData(u8 *stream)
{
    u32 tmp = 0;
    u32 timeStamp = 0;
    u32 frameSize = 0;
    u8 *p = stream;

    frameSize = SHOW3(p);
    tmp = SHOW1(p);
    tmp = BIT7(tmp);
    if( rcvV2 )
    {
        timeStamp = SHOW4(p);
    }

    return frameSize;
}

/*------------------------------------------------------------------------------

    Function name:  DecodeRCV

     Purpose:
     Decodes initialization metadata from rcv format.

------------------------------------------------------------------------------*/
i32 DecodeRCV(u8 *stream, u32 strmLen, VC1DecMetaData *metaData)
{
    u32 tmp1 = 0;
    u32 tmp2 = 0;
    u32 tmp3 = 0;
    u32 profile = 0;
    u8 *p;
    p = stream;

    if (strmLen < 9*4+8)
        return -1;

    tmp1 = SHOW3(p);
    tmp1 = SHOW1(p);
    if( tmp1 & 0x40 )   rcvV2 = 1;
    else                rcvV2 = 0;

    rcvMetadataSize = SHOW4(p);

    /* Decode image dimensions */
    p += rcvMetadataSize;
    tmp1 = SHOW4(p);
    metaData->maxCodedHeight = tmp1;
    tmp1 = SHOW4(p);
    metaData->maxCodedWidth = tmp1;

    return 0;
}


int MFCWMV3Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	VC1DecInst      decoder;
	VC1DecRet       ret;
	VC1DecRet       infoRet;
	VC1DecInput     decIn;
	VC1DecOutput    decOut;
	VC1DecInfo      decInfo;
	VC1DecMetaData  metaData;
	VC1DecPicture   decPic;
	u32 tmp;

	if(handle->delayInit)
	{
		g_picDecodeNumber = 0;
		/* reset metadata structure */
		DWLmemset(&metaData, 0, sizeof(metaData));

        /* decode row coded video header (coded metadata). DecodeRCV function reads
        * image dimensions from struct A, struct C information is parsed by
        * VC1DecUnpackMetaData function */
        tmp = DecodeRCV((u8*)inBuffer->virtualAddress, RCV_METADATA_MAX_SIZE, &metaData);
        if (tmp != 0)
        {
        	printf("DECODING RCV FAILED.\n");
            return -1;
        }

        tmp = VC1DecUnpackMetaData((u8*)inBuffer->virtualAddress+8, 4, &metaData);
        if (tmp != VC1DEC_OK)
        {
        	printf("UNPACKING META DATA FAILED.\n");
            return -1;
        }

	 	/* size of Sequence layer data structure */
        decIn.pStream = (u8*)inBuffer->virtualAddress + ( 4 + 4 * rcvV2 ) *4 + rcvMetadataSize;
        decIn.streamSize = DecodeFrameLayerData((u8*)decIn.pStream);
        decIn.pStream += 4 + 4 * rcvV2; /* size of Frame layer data structure */
        decIn.streamBusAddress = inBuffer->busAddress +
                (decIn.pStream - (u8*)inBuffer->virtualAddress);


		/* Decoder initialization, output reordering enabled  */
		/* check for any initialization errors must be done */
		infoRet = VC1DecInit(&decoder, &metaData, 0, 16);
		if(infoRet != VC1DEC_OK)
		{
			printf("VC1DecInit failure.\n");
			return -1;
		}
		handle->decInst = (void*)decoder;
		handle->delayInit = 0;
		if (handle->ppInit) {
			PPInst pp = NULL;
			PPResult ppRet;

			ppRet = PPInit(&pp);
			if(ppRet != PP_OK){
				printf("PPInit fail.\n");
				handle->ppInst = NULL;
			} else {
				handle->ppInst = (void*)pp;
				if (PPDecCombinedModeEnable(pp, handle->decInst, get_pp_dec_type(handle->streamType)) != PP_OK) {
					printf("PPDecCombinedModeEnable fail.\n");
					PPRelease(pp);
					handle->ppInst = NULL;
				}
			}

			if (handle->ppInst) {
				pp_set_config(handle, metaData.maxCodedWidth, metaData.maxCodedHeight, PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR,
					PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
			}
		}
	}
	else
	{
		decIn.streamSize = inBuffer->size;
		decIn.pStream = (u8*)inBuffer->virtualAddress; //stream_virtual_address;
		decIn.streamBusAddress = inBuffer->busAddress; //stream_bus_address;
	}

dec:
	/* Picture ID is the picture number in decoding order */
	decIn.picId = g_picDecodeNumber;

	/* decode the stream data */
	ret = VC1DecDecode(handle->decInst,  &decIn,  &decOut);

	switch(ret)
	{
	case VC1DEC_RESOLUTION_CHANGED:
		VC1DecGetInfo(handle->decInst, &decInfo);
		break;

	case VC1DEC_HDRS_RDY:

		tmp = (decOut.pStreamCurrPos - decIn.pStream);
		decIn.pStream = decOut.pStreamCurrPos;
		decIn.streamBusAddress += tmp;
		decIn.streamSize = decOut.dataLeft;
		/* read stream info */
		infoRet = VC1DecGetInfo(handle->decInst, &decInfo);
		goto dec;

	//  case VC1DEC_DP_HDRS_RDY:
		/* Data partitioning used in the stream.
		* The decoder has to reallocate resources */
	//    break;

	/* a picture was decoded */
	case VC1DEC_PIC_DECODED:
		VC1DecGetInfo(handle->decInst, &decInfo);

		/* Increment decoding number after every decoded picture */
		g_picDecodeNumber++;

		while(VC1DecNextPicture(handle->decInst, &decPic, 0) == VC1DEC_PIC_RDY )
		{
			//interlacedͼ��ֻ��������ʱ�п��ܽ����õ���ͬ������ͼ��
			//��ʱ��Ҫ����һ��
			if(decPic.interlaced && decPic.fieldPicture && decPic.topField && !handle->ppInst)
				continue;
			if(outBuffer->num >= MAX_OUTFRAME_NUM)
				break;
			outBuffer->buffer[outBuffer->num].keyPicture =
				decPic.keyPicture;
			if (!handle->ppInst) {
				outBuffer->buffer[outBuffer->num].yBusAddress =
					decPic.outputPictureBusAddress;
				outBuffer->buffer[outBuffer->num].pyVirAddress =
					decPic.pOutputPicture;
				outBuffer->frameWidth = decPic.frameWidth;
				outBuffer->frameHeight = decPic.frameHeight;
				outBuffer->num++;
			} else if (PPGetResult(handle->ppInst) == PP_OK) {
				outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
				outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
				outBuffer->frameWidth = handle->ppOutWidth;
				outBuffer->frameHeight = handle->ppOutHeight;
				handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
				pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
				outBuffer->num++;
			}
			outBuffer->codedWidth = decPic.codedWidth;
			outBuffer->codedHeight = decPic.codedHeight;
		}
		break;

	case VC1DEC_STRM_PROCESSED:
	case VC1DEC_STRM_ERROR:
		/* Input stream was processed but no picture is ready */
		break;

	case VC1DEC_MEMFAIL:
		return -1;

	default:
		/* all other cases are errors where decoding cannot continue */
		return -1;
	}

	return 0;
}

int MFCWMV3DecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	VC1DecPicture   decPic;

	if (VC1DecNextPicture(handle->decInst, &decPic, 1) == VC1DEC_PIC_RDY)
	{
		outBuffer->buffer[outBuffer->num].keyPicture =
			decPic.keyPicture;
		if (!handle->ppInst) {
			outBuffer->buffer[outBuffer->num].yBusAddress =
				decPic.outputPictureBusAddress;
			outBuffer->buffer[outBuffer->num].pyVirAddress =
				decPic.pOutputPicture;
		} else if (PPGetResult(handle->ppInst) == PP_OK) {
			outBuffer->buffer[outBuffer->num].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
			outBuffer->buffer[outBuffer->num].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
			handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
			pp_reset_outimg_addr(handle, PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex));
		}
		return 1;
	}

	return 0;
}

int MFCJpegDecode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	JpegDecInst  decoder;
	JpegDecRet  infoRet;
	JpegDecInput DecIn;
	JpegDecImageInfo DecImgInf;
	JpegDecOutput DecOut;
	PPResult ppRet;
	int ret = -1;
	int i;
	int format;

	infoRet = JpegDecInit(&decoder);
	if(infoRet !=JPEGDEC_OK) {
		printf("JpegDecInit failure %d.\n", infoRet);
		return -1;
	}
	handle->decInst = (void*)decoder;

	if (handle->ppInit) {
		PPInst pp = NULL;

		ppRet = PPInit(&pp);
		if(ppRet != PP_OK){
			printf("PPInit fail.\n");
			handle->ppInst = NULL;
		} else {
			handle->ppInst = (void*)pp;
			if (PPDecCombinedModeEnable(pp, handle->decInst, PP_PIPELINED_DEC_TYPE_JPEG) != PP_OK) {
				printf("PPDecCombinedModeEnable fail.\n");
				PPRelease(pp);
				handle->ppInst = NULL;
			}
		}
	}

	DecIn.decImageType = JPEGDEC_IMAGE;
	DecIn.sliceMbSet = 0;

	DecIn.bufferSize = 0;
	DecIn.streamLength = inBuffer->size;
	DecIn.streamBuffer.busAddress= inBuffer->busAddress;
	DecIn.streamBuffer.pVirtualAddress = inBuffer->virtualAddress;

	g_JpegYBusAddress = 0;

	/* Get image information of the JFIF */
    infoRet = JpegDecGetImageInfo(handle->decInst,
                                   &DecIn,
                                   &DecImgInf);
	if(infoRet !=JPEGDEC_OK)
	{
		printf("JpegDecGetImageInfo failure.\n");
		goto end;
	}

	if (DecImgInf.outputWidth > MAX_JPEG_OUTFRAME_WIDTH || DecImgInf.outputHeight > MAX_JPEG_OUTFRAME_HEIGHT)
	{
		printf("%dx%d image is too large to decode", DecImgInf.outputWidth, DecImgInf.outputHeight);
		goto end;
	}

	if (handle->ppInst) {
		if (DecImgInf.outputFormat == JPEGDEC_YCbCr400)
			format = PP_PIX_FMT_YCBCR_4_0_0;
		else if (DecImgInf.outputFormat == JPEGDEC_YCbCr440)
			format = PP_PIX_FMT_YCBCR_4_4_0;
		else if (DecImgInf.outputFormat == JPEGDEC_YCbCr411_SEMIPLANAR)
			format = PP_PIX_FMT_YCBCR_4_1_1_SEMIPLANAR;
		else if (DecImgInf.outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR)
			format = PP_PIX_FMT_YCBCR_4_4_4_SEMIPLANAR;
		else if (DecImgInf.outputFormat == JPEGDEC_YCbCr420_SEMIPLANAR)
			format = PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;
		else
			format = PP_PIX_FMT_YCBCR_4_2_2_SEMIPLANAR;
		if (pp_set_config(handle, DecImgInf.displayWidth, DecImgInf.displayHeight, format,
			PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex)) < 0) {
			goto end;
		}
		DecIn.pictureBufferY.busAddress = 0;
		DecIn.pictureBufferY.pVirtualAddress = NULL;
		DecIn.pictureBufferCbCr.busAddress = 0;
		DecIn.pictureBufferCbCr.pVirtualAddress = NULL;
		DecIn.pictureBufferCr.busAddress = 0;
		DecIn.pictureBufferCr.pVirtualAddress = 0;
	} else {
		if (g_JpegMemWidth != DecImgInf.outputWidth || g_JpegMemHeight != DecImgInf.outputHeight) {
			JpegReleaseBuffer();
			if (JpegAllocBuffer(DecImgInf.outputWidth * DecImgInf.outputHeight * 3) < 0) {
				printf("JpegAllocBuffer failure.\n");
				goto end;
			}
			g_JpegMemWidth = DecImgInf.outputWidth;
			g_JpegMemHeight = DecImgInf.outputHeight;
		}
		DecIn.pictureBufferY.busAddress = g_JpegYMemInfo[g_JpegMemIndex].busAddress;
		DecIn.pictureBufferY.pVirtualAddress = g_JpegYMemInfo[g_JpegMemIndex].virtualAddress;
		DecIn.pictureBufferCbCr.busAddress = g_JpegYMemInfo[g_JpegMemIndex].busAddress + DecImgInf.outputWidth * DecImgInf.outputHeight;
		DecIn.pictureBufferCbCr.pVirtualAddress = (u32*)((u32)g_JpegYMemInfo[g_JpegMemIndex].virtualAddress +
				DecImgInf.outputWidth * DecImgInf.outputHeight);
		DecIn.pictureBufferCr.busAddress = 0;
		DecIn.pictureBufferCr.pVirtualAddress = 0;
	}

	g_JpegMemIndex = !g_JpegMemIndex;

	/* Decode JFIF */
    infoRet = JpegDecDecode(handle->decInst, &DecIn, &DecOut);
	if(infoRet != JPEGDEC_FRAME_READY)
	{
		printf("JpegDecDecode failure, ret=%d\n", infoRet);
		if (infoRet == JPEGDEC_MEMFAIL)
			ret = -1;
		goto end;
	}

	ret = 0;

	outBuffer->codedWidth = DecImgInf.displayWidth;
	outBuffer->codedHeight = DecImgInf.displayHeight;
	outBuffer->buffer[0].keyPicture = 1;
	if (!handle->ppInst) {
		g_JpegYBusAddress = DecOut.outputPictureY.busAddress;
		g_JpegCbCrBusAddress = DecOut.outputPictureCbCr.busAddress;
		if (g_JpegYBusAddress) {
			outBuffer->num = 1;
			outBuffer->buffer[0].yBusAddress = g_JpegYBusAddress;
			outBuffer->buffer[0].cbcrBusAddress = g_JpegCbCrBusAddress;
			outBuffer->buffer[0].pyVirAddress = DecOut.outputPictureY.pVirtualAddress;
			outBuffer->buffer[0].pcbcrVirAddress = DecOut.outputPictureCbCr.pVirtualAddress;
			outBuffer->frameWidth = DecImgInf.outputWidth;
			outBuffer->frameHeight = DecImgInf.outputHeight;
		}
	} else if (PPGetResult(handle->ppInst) == PP_OK) {
		outBuffer->num = 1;
		outBuffer->buffer[0].yBusAddress = PP_OUTBUFFER_PHYADDR(handle->ppOutBufferIndex);
		outBuffer->buffer[0].pyVirAddress = (void*)PP_OUTBUFFER_VIRADDR(handle->ppOutBufferIndex);
		outBuffer->frameWidth = handle->ppOutWidth;
		outBuffer->frameHeight = handle->ppOutHeight;

		handle->ppOutBufferIndex = (handle->ppOutBufferIndex + 1) % PP_OUTBUF_NUM;
	}

end:
	if (handle->ppInst) {
		PPRelease(handle->ppInst);
		handle->ppInst = NULL;
	}

	JpegDecRelease((JpegDecInst)handle->decInst);

	return ret;
}

int MFCJpegDecodeEof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	return 0;
}

static int JpegAllocBuffer(u32 size)
{
	u32 pgsize = getpagesize();
	MemallocParams params;
	int fdmem;
	void *pvirt;

	fdmem = open("/dev/mem", O_RDWR | O_SYNC);
	if (fdmem < 0) {
		printf("JpegAllocBuffer failed to open memdev\n");
		return -1;
	}

	if (g_JpegMemallocFd < 0) {
		g_JpegMemallocFd = open(MEMALLOC_MODULE_PATH, O_RDWR | O_SYNC);
		if (g_JpegMemallocFd < 0) {
			printf("JpegAllocBuffer failed to open: %s\n", MEMALLOC_MODULE_PATH);
			close(fdmem);
			return -1;
		}
	}

	size = (size + (pgsize - 1)) & (~(pgsize - 1));
	params.size = size * 2;

    /* get memory linear memory buffers */
    ioctl(g_JpegMemallocFd, MEMALLOC_IOCXGETBUFFER, &params);
    if(params.busAddress == 0) {
        printf("JpegAllocBuffer no linear buffer available\n");
		close(fdmem);
		return -1;
    }

    /* Map the bus address to virtual address */
    pvirt = mmap(0, params.size, PROT_READ | PROT_WRITE, MAP_SHARED, fdmem, params.busAddress);
    if(pvirt == MAP_FAILED) {
		ioctl(g_JpegMemallocFd, MEMALLOC_IOCSFREEBUFFER, &params.busAddress);
		close(fdmem);
        return -1;
	}

	g_JpegYMemInfo[0].size = size;
	g_JpegYMemInfo[0].virtualAddress = pvirt;
	g_JpegYMemInfo[0].busAddress = params.busAddress;
	g_JpegYMemInfo[1].size = size;
	g_JpegYMemInfo[1].virtualAddress = (void*)((u32)pvirt + size);
	g_JpegYMemInfo[1].busAddress = params.busAddress + size;

	close(fdmem);

	return 0;
}

static void JpegReleaseBuffer(void)
{
	if (!g_JpegYMemInfo[0].busAddress)
		return;

    if(g_JpegMemallocFd < 0)
        return;

	ioctl(g_JpegMemallocFd, MEMALLOC_IOCSFREEBUFFER, &g_JpegYMemInfo[0].busAddress);
	if (g_JpegYMemInfo[0].virtualAddress && g_JpegYMemInfo[0].virtualAddress != MAP_FAILED)
		munmap(g_JpegYMemInfo[0].virtualAddress, g_JpegYMemInfo[0].size * 2);
	memset(&g_JpegYMemInfo[0], 0, sizeof(g_JpegYMemInfo[0]));
	memset(&g_JpegYMemInfo[1], 0, sizeof(g_JpegYMemInfo[1]));
	close(g_JpegMemallocFd);
	g_JpegMemallocFd = -1;
}

MFCHandle* mfc_init(int streamType)
{
	MFCHandle *handle = NULL;
	void *decInst = NULL;
	int delayInit = 0;

	ark_interlaced = 0;

	switch(streamType) {
    case RAW_STRM_TYPE_WMV3:
		delayInit = 1;
		break;

	case RAW_STRM_TYPE_H264: {
		H264DecInst decoder;
		H264DecRet infoRet;

		g_picDecodeNumber = 0;
		infoRet = H264DecInit(&decoder, 0, 0, 1);
		if(infoRet != H264DEC_OK) {
			printf("H264DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

	case RAW_STRM_TYPE_H264_NOREORDER: {
		H264DecInst decoder;
		H264DecRet infoRet;

		g_picDecodeNumber = 0;
		infoRet = H264DecInit(&decoder, 1, 0, 1);
		if(infoRet != H264DEC_OK) {
			printf("H264DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

    case RAW_STRM_TYPE_MP2: {
		Mpeg2DecInst      decoder;
		Mpeg2DecRet       infoRet;

		g_picDecodeNumber = 0;
		infoRet = Mpeg2DecInit(&decoder, 0, 16);
		if(infoRet != MPEG2DEC_OK) {
			printf("Mpeg2DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

	case RAW_STRM_TYPE_MP4_CUSTOM:
    case RAW_STRM_TYPE_MP4: {
		MP4DecInst      decoder;
		MP4DecRet       infoRet;

		g_picDecodeNumber = 0;
		infoRet = MP4DecInit(&decoder, 0, 0, 16);
		if(infoRet != MP4DEC_OK) {
			printf("MP4DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

    case RAW_STRM_TYPE_SOR: {
		MP4DecInst      decoder;
		MP4DecRet       infoRet;

		g_picDecodeNumber = 0;
		infoRet = MP4DecInit(&decoder, MP4DEC_SORENSON, 0, 16);
		if(infoRet != MP4DEC_OK) {
			printf("MP4DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

	case RAW_STRM_TYPE_REAL:
		delayInit = 1;
		break;

	case RAW_STRM_TYPE_VC1: {
		VC1DecInst      decoder;
		VC1DecRet       infoRet;
		VC1DecMetaData  metaData;

		g_picDecodeNumber = 0;
		//Advance Profile
		metaData.profile = 8;

		/* Decoder initialization, output reordering enabled  */
		/* check for any initialization errors must be done */
		infoRet = VC1DecInit(&decoder, &metaData, 0, 16);
		if(infoRet != VC1DEC_OK) {
			printf("VC1DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

    case RAW_STRM_TYPE_JPEG:
		g_JpegMemWidth = 0;
		g_JpegMemHeight = 0;
		break;

    case RAW_STRM_TYPE_VP8: {
		VP8DecInst      decoder;
		VP8DecRet       infoRet;
		VP8DecFormat    decFormat;

		g_picDecodeNumber = 0;
		decFormat = VP8DEC_VP8;
		/* Decoder initialization, */
		/* check for any initialization errors must be done */
		infoRet = VP8DecInit(&decoder, decFormat,
		                 0, 16, DEC_REF_FRM_RASTER_SCAN);
		if(infoRet != VP8DEC_OK) {
			printf("VP8DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

 	case RAW_STRM_TYPE_VP6: {
		VP6DecInst decoder;
		VP6DecRet ret;

		g_picDecodeNumber = 0;
		ret = VP6DecInit(&decoder, 0, 16);
		if (ret != VP6DEC_OK)
		{
		    printf("VP6DecInit failure.\n");
		    return NULL;
		}
		decInst = (void*)decoder;
    	break;
	}

	default:
		printf("MFCDecode unsupported format.\n");
		return NULL;
	}

	handle = (MFCHandle*)malloc(sizeof(MFCHandle));
	if (!handle)
		return NULL;
	memset(handle, 0, sizeof(MFCHandle));
	handle->decInst = decInst;
	handle->streamType = streamType;
	handle->delayInit = delayInit;
	if (streamType == RAW_STRM_TYPE_MP4_CUSTOM)
		handle->isCustomMp4 = 1;
	else
		handle->isCustomMp4 = 0;

	return handle;
}

int mfc_decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	if (handle == NULL) {
		printf("Invalid handle.\n");
		return -1;
	}

	outBuffer->num = 0;
	switch(handle->streamType) {
    case RAW_STRM_TYPE_WMV3:
		return MFCWMV3Decode(handle, inBuffer, outBuffer);

	case RAW_STRM_TYPE_H264:
	case RAW_STRM_TYPE_H264_NOREORDER:
		return MFCH264Decode(handle, inBuffer, outBuffer);

    case RAW_STRM_TYPE_MP2:
		return MFCMpeg2Decode(handle, inBuffer, outBuffer);

    case RAW_STRM_TYPE_MP4:
	case RAW_STRM_TYPE_SOR:
	case RAW_STRM_TYPE_MP4_CUSTOM:
		return MFCMp4Decode(handle, inBuffer, outBuffer);

	case RAW_STRM_TYPE_REAL:
		return MFCRvDecode(handle, inBuffer, outBuffer);

	case RAW_STRM_TYPE_VC1:
		return MFCVC1Decode(handle, inBuffer, outBuffer);

    case RAW_STRM_TYPE_JPEG:
		return MFCJpegDecode(handle, inBuffer, outBuffer);

    case RAW_STRM_TYPE_VP8:
		return MFCVp8Decode(handle, inBuffer, outBuffer);

 	case RAW_STRM_TYPE_VP6:
 		return MFCVp6Decode(handle, inBuffer, outBuffer);

	default:
		printf("MFCDecode unsupported format.\n");
		return -1;
	}
}

int mfc_decode_eof(MFCHandle *handle, OutFrameBuffer *outBuffer)
{
	if (handle == NULL) {
		printf("Invalid handle.\n");
		return -1;
	}

	outBuffer->num = 0;
	switch(handle->streamType) {
    case RAW_STRM_TYPE_WMV3:
		return MFCWMV3DecodeEof(handle, outBuffer);

	case RAW_STRM_TYPE_H264:
	case RAW_STRM_TYPE_H264_NOREORDER:
		return MFCH264DecodeEof(handle, outBuffer);

    case RAW_STRM_TYPE_MP2:
		return MFCMpeg2DecodeEof(handle, outBuffer);

    case RAW_STRM_TYPE_MP4:
	case RAW_STRM_TYPE_SOR:
	case RAW_STRM_TYPE_MP4_CUSTOM:
		return MFCMp4DecodeEof(handle, outBuffer);

	case RAW_STRM_TYPE_REAL:
		return MFCRvDecodeEof(handle, outBuffer);

	case RAW_STRM_TYPE_VC1:
		return MFCVC1DecodeEof(handle, outBuffer);

    case RAW_STRM_TYPE_JPEG:
		return MFCJpegDecodeEof(handle, outBuffer);

    case RAW_STRM_TYPE_VP8:
		return MFCVp8DecodeEof(handle, outBuffer);

	case RAW_STRM_TYPE_VP6:
		return MFCVp6DecodeEof(handle, outBuffer);

	default:
		printf("MFCDecode unsupported format.\n");
		return -1;
	}
}

void mfc_uninit(MFCHandle *handle)
{
	if (handle == NULL) {
		printf("Invalid handle.\n");
		return ;
	}

	if (handle->ppInst) {
		PPRelease(handle->ppInst);
	}

	if (handle->ppMemalloc > 0) {
		pp_free_buffer(handle->ppMemalloc, &handle->ppOutBuffer);
		close(handle->ppMemalloc);
	}

	switch(handle->streamType)
	{
		case RAW_STRM_TYPE_H264:
		case RAW_STRM_TYPE_H264_NOREORDER:
			if(handle->decInst)
				H264DecRelease((H264DecInst)handle->decInst);
			break;

        case RAW_STRM_TYPE_MP2:
			if(handle->decInst)
				Mpeg2DecRelease((Mpeg2DecInst)handle->decInst);
			break;

        case RAW_STRM_TYPE_MP4:
        case RAW_STRM_TYPE_MP4_CUSTOM:
		case RAW_STRM_TYPE_SOR:
			if(handle->decInst)
				MP4DecRelease((MP4DecInst)handle->decInst);
			break;

		case RAW_STRM_TYPE_REAL:
			if(handle->decInst)
				On2RvDecFree((RvDecInst)handle->decInst);
			break;

		case RAW_STRM_TYPE_VC1:
		case RAW_STRM_TYPE_WMV3:
			if(handle->decInst)
				VC1DecRelease((VC1DecInst)handle->decInst);
			break;

        case RAW_STRM_TYPE_JPEG:
			JpegReleaseBuffer();
			break;

        case RAW_STRM_TYPE_VP8:
			if(handle->decInst)
				VP8DecRelease((VP8DecInst)handle->decInst);
			break;

        case RAW_STRM_TYPE_VP6:
			if(handle->decInst)
				VP6DecRelease((VP6DecInst)handle->decInst);
			break;

		default:
			printf("MFCRelease unsupported format.\n");
			break;
	}

	free(handle);
}

static int get_pp_dec_type(int streamType)
{
	switch(streamType)
	{
		case RAW_STRM_TYPE_H264:
		case RAW_STRM_TYPE_H264_NOREORDER:
			return PP_PIPELINED_DEC_TYPE_H264;

        case RAW_STRM_TYPE_MP2:
			return PP_PIPELINED_DEC_TYPE_MPEG2;

        case RAW_STRM_TYPE_MP4:
        case RAW_STRM_TYPE_MP4_CUSTOM:
		case RAW_STRM_TYPE_SOR:
			return PP_PIPELINED_DEC_TYPE_MPEG4;

		case RAW_STRM_TYPE_REAL:
			return PP_PIPELINED_DEC_TYPE_RV;

		case RAW_STRM_TYPE_VC1:
		case RAW_STRM_TYPE_WMV3:
			return PP_PIPELINED_DEC_TYPE_VC1;

        case RAW_STRM_TYPE_JPEG:
			return PP_PIPELINED_DEC_TYPE_JPEG;

        case RAW_STRM_TYPE_VP8:
			return PP_PIPELINED_DEC_TYPE_VP8;

        case RAW_STRM_TYPE_VP6:
			return PP_PIPELINED_DEC_TYPE_VP6;

		default:
			printf("get_pp_dec_type unsupported format %d.\n", streamType);
			return -1;
	}
}

static int pp_alloc_buffer(int fdmemalloc, u32 size, DWLLinearMem_t * info)
{
    u32 pgsize = getpagesize();
    MemallocParams params;
	int fd_mem;

	fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd_mem == -1) {
		printf("Failed to open: %s\n", "/dev/mem");
		return -1;
	}

    size = (size + (pgsize - 1)) & (~(pgsize - 1));

    info->size = size;
    info->virtualAddress = MAP_FAILED;
    info->busAddress = 0;

    params.size = size;

    /* get memory linear memory buffers */
    ioctl(fdmemalloc, MEMALLOC_IOCXGETBUFFER, &params);
    if(params.busAddress == 0)
    {
        printf("ERROR! No linear buffer available\n");
		close(fd_mem);
        return -1;
    }

    info->busAddress = params.busAddress;

    /* Map the bus address to virtual address */
    info->virtualAddress = (u32 *) mmap(0, info->size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, fd_mem,
                                        params.busAddress);

    if(info->virtualAddress == MAP_FAILED) {
		close(fd_mem);
        return -1;
	}

	close(fd_mem);

    return 0;
}

static void pp_free_buffer(int fdmemalloc, DWLLinearMem_t * info)
{
    if(info->busAddress != 0)
        ioctl(fdmemalloc, MEMALLOC_IOCSFREEBUFFER,
              &info->busAddress);

    if(info->virtualAddress != MAP_FAILED)
        munmap(info->virtualAddress, info->size);
}

static int pp_set_config(MFCHandle *handle, int in_width, int in_height, int in_format, int out_addr)
{
	PPResult ppRet;
	PPConfig pPpConf;

	/* First get the default PP settings */
	ppRet = PPGetConfig(handle->ppInst, &pPpConf);
	if(ppRet != PP_OK){
		/* Handle errors here */
		printf("PPGetConfig fail.\n");
		return -1;
	}

	/* setup PP */
	/* Set input width */
	pPpConf.ppInImg.width = (in_width + 15) & ~15;
	/* Set input height */
	pPpConf.ppInImg.height = (in_height + 15) & ~15;
	/* set input Crop */
	if (in_width & 15 || in_height & 15) {

		pPpConf.ppInCrop.enable = 1;
		pPpConf.ppInCrop.originX = 0;
		pPpConf.ppInCrop.originY = 0;
		pPpConf.ppInCrop.width = in_width & ~7;
		pPpConf.ppInCrop.height = in_height & ~7;
	}
	/* Set video range to 0 */
	pPpConf.ppInImg.videoRange = 0;
	/* Set the input format to be YCbCr 4:2:0 Semi-planar */
	pPpConf.ppInImg.pixFormat = in_format;
	/* Set output width */
	pPpConf.ppOutImg.width = handle->ppOutWidth;
	/* Set input height */
	pPpConf.ppOutImg.height = handle->ppOutHeight;
	/* Set output picture base address */
	pPpConf.ppOutImg.bufferBusAddr = out_addr;
	if (handle->ppOutFormat) {
		/* Set output picture to RGB 32-bit format */
		pPpConf.ppOutImg.pixFormat = PP_PIX_FMT_RGB32_CUSTOM;
		pPpConf.ppOutRgb.rgbBitmask.maskAlpha = 0xff;
		pPpConf.ppOutRgb.rgbBitmask.maskR = 0xff00;
		pPpConf.ppOutRgb.rgbBitmask.maskG = 0xff0000;
		pPpConf.ppOutRgb.rgbBitmask.maskB = 0xff000000;
	} else {
		/* Set output picture to RGB 16-bit format */
		pPpConf.ppOutImg.pixFormat = PP_PIX_FMT_RGB16_CUSTOM;
		pPpConf.ppOutRgb.rgbBitmask.maskR = 0x001f;
		pPpConf.ppOutRgb.rgbBitmask.maskG = 0x07e0;
		pPpConf.ppOutRgb.rgbBitmask.maskB = 0xf800;
	}

	/* Use the BT.709 equation for the color conversion */
	//pPpConf.ppOutRgb.rgbTransform = PP_YCBCR2RGB_TRANSFORM_BT_709;

	//Enable deinterlace
	if (handle->streamType == RAW_STRM_TYPE_VC1 || handle->streamType == RAW_STRM_TYPE_MP2 ||
		handle->streamType == RAW_STRM_TYPE_MP4)
		pPpConf.ppOutDeinterlace.enable = 1;

	/* Now use the PP API to write in the new setup */
	ppRet = PPSetConfig(handle->ppInst, &pPpConf);
	if(ppRet != PP_OK){
		/* Handle errors here */
		printf("PPSetConfig fail.ret=%d.\n", ppRet);
		return -1;
	}

	return 0;
}

static int pp_reset_outimg_addr(MFCHandle *handle, u32 addr)
{
	PPResult ppRet;
	PPConfig pPpConf;

	/* First get the default PP settings */
	ppRet = PPGetConfig(handle->ppInst, &pPpConf);
	if(ppRet != PP_OK){
		/* Handle errors here */
		printf("PPGetConfig fail.\n");
		return -1;
	}
	/* Set output picture base address */
	pPpConf.ppOutImg.bufferBusAddr = addr;
	ppRet = PPSetConfig(handle->ppInst, &pPpConf);
	if(ppRet != PP_OK){
		/* Handle errors here */
		printf("PPSetConfig fail.ret=%d.\n", ppRet);
		return -1;
	}

	return 0;
}

int mfc_pp_init(MFCHandle *handle, int outWidth, int outHeight, int outFormat)
{
	PPInst pp = NULL;
	PPResult ppRet;
	int i;

	if (handle == NULL)
		return -1;

	if (get_pp_dec_type(handle->streamType) != PP_PIPELINED_DEC_TYPE_JPEG && !handle->delayInit) {
		ppRet = PPInit(&pp);
		if(ppRet != PP_OK){
			printf("PPInit fail.\n");
			return -1;
		}

		if (PPDecCombinedModeEnable(pp, handle->decInst, get_pp_dec_type(handle->streamType)) != PP_OK) {
			printf("PPDecCombinedModeEnable fail.\n");
			PPRelease(pp);
			return -1;
		}
	} else {
		handle->ppInit = 1;
	}

	handle->ppMemalloc = open(MEMALLOC_MODULE_PATH, O_RDWR | O_SYNC);
	if (handle->ppMemalloc < 0) {
		printf("Opem memalloc fail.\n");
		PPRelease(pp);
		return -1;
	}
	if (pp_alloc_buffer(handle->ppMemalloc, outWidth * outHeight * 4 * PP_OUTBUF_NUM,
			&handle->ppOutBuffer)) {
		printf("No enough memory for pp.\n");
		close(handle->ppMemalloc);
		PPRelease(pp);
		return -1;
	}
	//printf("DWLMallocLinear 0x%x, 0x%x.\n", handle->ppOutBuffer.busAddress, handle->ppOutBuffer.virtualAddress);

	/* if (get_pp_dec_type(handle->streamType) != PP_PIPELINED_DEC_TYPE_H264) {
		PPOutputBuffers ppOutBuf;
		ppOutBuf.nbrOfBuffers = PP_OUTBUF_NUM;
		for (i = 0; i < PP_OUTBUF_NUM; i++) {
			ppOutBuf.ppOutputBuffers[i].bufferBusAddr = handle->ppOutBuffer.busAddress +
				outWidth * outHeight * 4 * i;
			ppOutBuf.ppOutputBuffers[i].bufferChromaBusAddr =
				ppOutBuf.ppOutputBuffers[i].bufferBusAddr + outWidth * outHeight;
		}
		PPDecSetMultipleOutput(pp, &ppOutBuf);
	} */

	handle->ppInst = (void *)pp;
	handle->ppOutWidth = outWidth;
	handle->ppOutHeight = outHeight;
	handle->ppOutFormat = outFormat;

	return 0;
}

int MFCJpegGetInfo(MFCHandle *handle, DWLLinearMem_t *inBuffer, MFCStreamInfo *info)
{
	JpegDecInst  decoder;
	JpegDecRet  infoRet;
	JpegDecInput DecIn;
	JpegDecImageInfo DecImgInf;
	int ret = 0;

	if (!handle || !inBuffer || !info) {
		printf("Invalid parameter!\n");
		return -1;
	}

	infoRet = JpegDecInit(&decoder);
	if(infoRet !=JPEGDEC_OK) {
		printf("JpegDecInit failure %d.\n", infoRet);
		return -1;
	}
	handle->decInst = (void*)decoder;

	DecIn.decImageType = JPEGDEC_IMAGE;
	DecIn.sliceMbSet = 0;
	DecIn.bufferSize = 0;
	DecIn.streamLength = inBuffer->size;
	DecIn.streamBuffer.busAddress= inBuffer->busAddress;
	DecIn.streamBuffer.pVirtualAddress = inBuffer->virtualAddress;

	/* Get image information of the JFIF */
    infoRet = JpegDecGetImageInfo(handle->decInst,
                                   &DecIn,
                                   &DecImgInf);
	if(infoRet !=JPEGDEC_OK)
	{
		printf("JpegDecGetImageInfo failure.\n");
		info->codedWidth = 0;
		info->codedHeight = 0;
		info->frameWidth = 0;
		info->frameHeight = 0;
		info->format = MFCFORMAT_NONE;
		ret = -1;
		goto end;
	}

	info->codedWidth = DecImgInf.displayWidth;
	info->codedHeight = DecImgInf.displayHeight;
	info->frameWidth = DecImgInf.outputWidth;
	info->frameHeight = DecImgInf.outputHeight;
	if (DecImgInf.outputFormat == JPEGDEC_YCbCr400)
		info->format = MFCFORMAT_YCBCR400;
	else if (DecImgInf.outputFormat == JPEGDEC_YCbCr440)
		info->format = MFCFORMAT_YCBCR440;
	else if (DecImgInf.outputFormat == JPEGDEC_YCbCr411_SEMIPLANAR)
		info->format = MFCFORMAT_YCBCR411_SEMIPLANAR;
	else if (DecImgInf.outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR)
		info->format = MFCFORMAT_YCBCR444_SEMIPLANAR;
	else if (DecImgInf.outputFormat == JPEGDEC_YCbCr420_SEMIPLANAR)
		info->format = MFCFORMAT_YCBCR420_SEMIPLANAR;
	else
		info->format = MFCFORMAT_YCBCR422_SEMIPLANAR;

end:
	JpegDecRelease((JpegDecInst)handle->decInst);

	return ret;
}

int mfc_get_stream_info(MFCHandle *handle, DWLLinearMem_t *inBuffer, MFCStreamInfo *info)
{
	if (handle == NULL) {
		printf("Invalid handle.\n");
		return -1;
	}

	switch(handle->streamType) {
	case RAW_STRM_TYPE_JPEG:
		return MFCJpegGetInfo(handle, inBuffer, info);
	case RAW_STRM_TYPE_WMV3:
	case RAW_STRM_TYPE_H264:
	case RAW_STRM_TYPE_H264_NOREORDER:
	case RAW_STRM_TYPE_MP2:
	case RAW_STRM_TYPE_MP4:
	case RAW_STRM_TYPE_SOR:
	case RAW_STRM_TYPE_MP4_CUSTOM:
	case RAW_STRM_TYPE_REAL:
	case RAW_STRM_TYPE_VC1:
	case RAW_STRM_TYPE_VP8:
	case RAW_STRM_TYPE_VP6:
	default:
		printf("Not supported yet.\n");
		return -1;
	}
}
