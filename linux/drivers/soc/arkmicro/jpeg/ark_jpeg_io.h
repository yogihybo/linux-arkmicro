/*****************************************************************************
 * To be included by both applications and driver
 *****************************************************************************/

#ifndef _ARK_JPEG_IO_H_
#define _ARK_JPEG_IO_H_

/*************************************************************************
 * Ioctl command definition
 *************************************************************************/
#define ARKJPEG_IOCTL_BASE		0xe0

#define ARKJPEG_DECODE  			_IOW(ARKJPEG_IOCTL_BASE, 0, unsigned long)
#define ARKJPEG_SET_DECODE_OPT      _IOW(ARKJPEG_IOCTL_BASE, 1, unsigned long)
#define ARKJPEG_GET_INPUTBUF        _IOW(ARKJPEG_IOCTL_BASE, 2, unsigned long)
#define ARKJPEG_GET_DECSTATUS  		_IOW(ARKJPEG_IOCTL_BASE, 3, unsigned long)
#define ARKJPEG_SET_JPG_SIZE		_IOW(ARKJPEG_IOCTL_BASE, 4, unsigned long)
#define ARKJPEG_BREAK_DECODE		_IOW(ARKJPEG_IOCTL_BASE, 5, unsigned long)
#define ARKJPEG_GET_DECODEINFO		_IOW(ARKJPEG_IOCTL_BASE, 6, unsigned long)
#define ARKJPEG_GET_APIEVENT		_IOW(ARKJPEG_IOCTL_BASE, 7, unsigned long)
#define ARKJPEG_SET_APIDONEEVENT	_IOW(ARKJPEG_IOCTL_BASE, 8, unsigned long)
#define ARKJPEG_SET_BUFFER  		_IOW(ARKJPEG_IOCTL_BASE, 9, unsigned long)
#define ARKJPEG_GET_BUFFER  		_IOW(ARKJPEG_IOCTL_BASE,10, unsigned long)
#define ARKJPEG_REPEAT_SCALER  		_IOW(ARKJPEG_IOCTL_BASE,11, unsigned long)
#define ARKJPEG_GET_PART_PIC  		_IOW(ARKJPEG_IOCTL_BASE,12, unsigned long)

typedef enum {
	NO_SCALER,		//no scaler
	UNIFORM_SCALER,		//scaler keep the aspect ratio 
	NOMAL_SCALER		//scaler don't keep the aspect ratio 
} JPEG_SCALER_MODE;

typedef enum {
	ZOOM_IN_ONLY,
	ZOOM_OUT_ONLY,
	ZOOM_IN_OUT
} JPEG_ZOOM_MODE;

typedef enum {
	FILE_IN_DEVICE,
	FILE_IN_MEMORY
} JPEG_FILE_TYPE;

typedef enum {
	DEC_FREE,
	DEC_BUSYING
} JPEG_DEC_STATUS;

typedef enum {
	DEC_SUCCESS,
	DEC_ERROR,
	DEC_BREAKED
} JPEG_DEC_RESULT;

typedef enum {
	CLOCKWISE_0,
	CLOCKWISE_90,
	CLOCKWISE_180,
	CLOCKWISE_270,
} ROTATE_ANGLE;

typedef enum {
	FREAD,
	FSEEK,
	DEC_OVER
} API_EVENT;

typedef struct {
	JPEG_SCALER_MODE ScalerMode;
	JPEG_ZOOM_MODE ZoomMode;
	ROTATE_ANGLE RotateAngle;
	unsigned int dwDestWidth;
	unsigned int dwDestHeight;
	unsigned int RepeatdwSrcWidth;
	unsigned int RepeatdwSrcHeight;
	unsigned int format;
	unsigned int SrcImagePhyAddr;
	unsigned int DestImagePhyAddr;
} JPEG_DECODE_OPT, *PJPEG_DECODE_OPT;

typedef struct {
	JPEG_DEC_RESULT DecResult;
	unsigned int dwDecSize;
	unsigned int dwSrcWidth;
	unsigned int dwSrcHeight;
	unsigned int RepeatdwSrcWidth;
	unsigned int RepeatdwSrcHeight;
	unsigned int dwOutWidth;
	unsigned int dwOutHeight;
} JPEG_DECODE_INFO;

typedef struct {
	API_EVENT EventType;
	unsigned int dwReadLen;
	long lOffset;
	int nOrigin;
	JPEG_DEC_RESULT DecResult;
} JPEG_API_INFO;

typedef struct {
	unsigned int dwReadedLen;
	int nSeekRet;
} JPEG_API_RETINFO;

typedef struct {
	unsigned int base_phys;
	int size;
} JPEG_INPUT_BUFINFO;

struct part_pic {
	unsigned src_phys_base;
	unsigned src_width;
	unsigned src_height;

	unsigned part_phys_base;
	unsigned x_width;
	unsigned y_height;
	unsigned part_width;
	unsigned part_height;
};

struct jpeg_buffer {
	unsigned int file_size;
	unsigned int file_base_phys;
	void *file_user_base_virt;
	unsigned int decode_size;
	unsigned int decode_base_phys;
	void *decode_user_base_virt;
};

#endif
