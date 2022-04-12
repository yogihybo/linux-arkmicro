#ifndef __MFCAPI_H__
#define __MFCAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dwl.h"

#define RAW_STRM_TYPE_H264				104
#define RAW_STRM_TYPE_MP2				105
#define RAW_STRM_TYPE_MP4				106
#define RAW_STRM_TYPE_REAL				107
#define RAW_STRM_TYPE_VC1				108
#define RAW_STRM_TYPE_WMV3				109
#define RAW_STRM_TYPE_JPEG				110
#define RAW_STRM_TYPE_SOR				111
#define RAW_STRM_TYPE_VP8				112
#define RAW_STRM_TYPE_VP6				113
#define RAW_STRM_TYPE_RV1020			114
#define RAW_STRM_TYPE_MSMP4				115
#define RAW_STRM_TYPE_MP4_CUSTOM		116
#define RAW_STRM_TYPE_H264_NOREORDER	(RAW_STRM_TYPE_H264 | (1 << 8))

#define MAX_OUTFRAME_NUM	8

#define MAX_OUTFRAME_WIDTH	1920
#define MAX_OUTFRAME_HEIGHT	1088

typedef struct
{
	u32 yBusAddress;
	u32 cbcrBusAddress;
	u32 crBusAddress;
	const void *pyVirAddress;
	const void *pcbcrVirAddress;
	const void *pcrVirAddress;
	i32 keyPicture;
}FrameBuffer;

typedef struct
{
	i32 num;
	i32 codedWidth;
	i32 codedHeight;
	i32 frameWidth;
	i32 frameHeight;
	FrameBuffer buffer[MAX_OUTFRAME_NUM];
}OutFrameBuffer;

typedef enum{
	MFCDEC_RET_OK = 1,
	MFCDEC_RET_DECFAIL = 0,
	MFCDEC_RET_MEMFAIL = -1,
	MFCDEC_RET_LOCKFAIL = -2,
}MFCDecodeRet;

typedef struct
{
	void *decInst;
	void *ppInst;
	int streamType;
	int delayInit;
	int isCustomMp4;
	int outputFormat;
	int ppInit;
	int ppMemalloc;
	int ppOutWidth;
	int ppOutHeight;
	int ppOutFormat;
	int ppOutBufferIndex;
	DWLLinearMem_t ppOutBuffer;
}MFCHandle;

MFCHandle *mfc_init(int streamType);

int mfc_decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer);

void mfc_uninit(MFCHandle *handle);

int mfc_pp_init(MFCHandle *handle, int outWidth, int outHeight, int outFormat);

#ifdef __cplusplus
}
#endif

#endif
