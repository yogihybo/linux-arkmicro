#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "ark_jpeg_io.h"
#include "memalloc.h"
#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "ark1668ft.h"

#define JPEG_DEVICE_PATH "/dev/ark_jpeg"

static struct cma_mem *inbuf = NULL, *outbuf = NULL;

int JPEGSetFileSize(int fdjpeg, int file_size)
{
	if (fdjpeg < 0)
		return -1;

	if (ioctl(fdjpeg, ARKJPEG_SET_JPG_SIZE, &file_size) < 0) {
		printf("ARKJPEG_SET_JPG_SIZE fail.\n");
		return -1;
	}

	return 0;
}

JPEG_DEC_RESULT JPEGDecodeExe(int fdjpeg, unsigned char *filename)
{
	int fd = -1;
	JPEG_DEC_RESULT ret = DEC_SUCCESS;
	int count = 0;

	fd = open(filename, O_RDWR);
	if(fd < 0)
	{
		printf("open jpeg file %s fail.\n", filename);
		return DEC_ERROR;
	}

	unsigned int filesize = get_file_size(filename);

	JPEGSetFileSize(fdjpeg, filesize);

	if (ioctl(fdjpeg, ARKJPEG_DECODE, NULL) < 0) {
		printf("ARKJPEG_DECODE fail.\n");
		return DEC_ERROR;
	}

	while(1)
	{
		JPEG_API_INFO apiInfo;
		JPEG_API_RETINFO apiRetInfo;

		if (ioctl(fdjpeg, ARKJPEG_GET_APIEVENT, &apiInfo) < 0) {
			printf("Wait jpeg api event timeout.\n");
			if(++count == 3)
			{
				ret = DEC_ERROR;
				goto end;
			}
		} else {
			printf("EventType=%d.\n", apiInfo.EventType);
			switch(apiInfo.EventType) {
			case FREAD:
				apiRetInfo.dwReadedLen = read(fd, inbuf->viraddr, apiInfo.dwReadLen);
				if (ioctl(fdjpeg, ARKJPEG_SET_APIDONEEVENT, &apiRetInfo) < 0) {
					printf("ARKJPEG_SET_APIEVENT fail.\n");
					ret = DEC_ERROR;
					goto end;
				}
				break;
			case FSEEK:
				apiRetInfo.nSeekRet = lseek(fd, apiInfo.lOffset, apiInfo.nOrigin);
				if (ioctl(fdjpeg, ARKJPEG_SET_APIDONEEVENT, &apiRetInfo) < 0) {
					printf("ARKJPEG_SET_APIEVENT fail.\n");
					ret = DEC_ERROR;
					goto end;
				}
				break;
			case DEC_OVER:
				ret = apiInfo.DecResult;
				goto end;
			}
		}
	}

end:
	close(fd);
	return ret;
}

int JPEGGetDecodeInfo(int fdjpeg, u32 *srcWidth, u32 *SrcHeight,
			u32 *destWidth, u32 *destHeight, u32 *dataSize)
{
	if (fdjpeg < 0)
		return -1;

	JPEG_DECODE_INFO info;

	if (ioctl(fdjpeg, ARKJPEG_GET_DECODEINFO, &info) < 0) {
		printf("ARKJPEG_GET_DECODEINFO fail.\n");
		return -1;
	}

    *srcWidth = info.dwSrcWidth;
    *SrcHeight = info.dwSrcHeight;
    *destWidth = info.dwOutWidth;
	*destHeight = info.dwOutHeight;
	*dataSize = info.dwDecSize;

    return 0;
}

void *jpg_ps_test_thread(void *arg)
{
    struct ft_runtime *rt = (struct ft_runtime *)arg;
    int fdjpeg = -1;
    unsigned int srcwidth, srcheight, outwidth, outheight, outsize;
    int fddata = -1;
    int datasize = 0;
    unsigned int *databuf = NULL;
    unsigned int *tmp;
    int i;

    inbuf = alloc_cma_mem(0x200000);
    if (!inbuf) {
        printf("alloc cma memory fail.\n");
        goto err;
    }

    outbuf = alloc_cma_mem(0x100000);
    if (!outbuf) {
        printf("alloc cma memory fail.\n");
        goto err;
    }

	fdjpeg = open(JPEG_DEVICE_PATH, O_RDWR | O_SYNC);
	if (fdjpeg == -1) {
		printf("open jpeg device file fail.\n");
		goto err;
	}

	JPEG_DECODE_OPT opt = {0};
	opt.ScalerMode = NOMAL_SCALER;
	opt.ZoomMode = ZOOM_IN_OUT;
	opt.RotateAngle = CLOCKWISE_0;
	opt.dwDestWidth = JPG_OUT_WIDTH;
	opt.dwDestHeight = JPG_OUT_HEIGHT;
	if (ioctl(fdjpeg, ARKJPEG_SET_DECODE_OPT, &opt) < 0) {
		printf("ARKJPEG_SET_DECODE_OPT fail.\n");
		goto err;
	}

	struct jpeg_buffer buffer;
	buffer.file_size = 0x200000;
	buffer.file_base_phys = inbuf->phyaddr;
	buffer.file_user_base_virt = inbuf->viraddr;
	buffer.decode_size = 0x100000;
	buffer.decode_base_phys = outbuf->phyaddr;
	buffer.decode_user_base_virt = outbuf->viraddr;
	if (ioctl(fdjpeg, ARKJPEG_SET_BUFFER, &buffer) < 0) {
		printf("ARKJPEG_SET_BUFFER fail.\n");
		goto err;
	}

	if (JPEGDecodeExe(fdjpeg, JPG_FILE_PATH) == DEC_SUCCESS) {
		JPEGGetDecodeInfo(fdjpeg, &srcwidth, &srcheight, &outwidth, &outheight, &outsize);
		printf("srcwidth=%d, srcheight=%d, outwidth=%d, outheight=%d, outsize=%d.\n",
			srcwidth, srcheight, outwidth, outheight, outsize);
	} else {
		printf("jpeg decode error.\n");
		goto err;
	}

    fddata = open(JPG_YUVDATA_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open data file fail.\n");
        goto err;
    }
    datasize = get_file_size(JPG_YUVDATA_PATH);
    databuf = malloc(datasize);
    if (!databuf) {
        printf("mallco databuf fail.\n");
        goto err;
    }
    if (read(fddata, databuf, datasize) != datasize) {
        printf("read data file err.\n");
        goto err;
    }

    tmp = (unsigned int*)outbuf->viraddr;
    for (i = 0; i < datasize / 4; i++) {
        if (databuf[i] != tmp[i]) {
            printf("compare data fail.0x%x, 0x%x.\n", databuf[i], tmp[i]);
            goto err;
        }
    }

    free(databuf);
    free_cma_mem(inbuf);
    free_cma_mem(outbuf);
    close(fdjpeg);
    close(fddata);
    rt->finish = 1;
    rt->pass = 1;
    return (void*)0;

err:
    if (databuf)
        free(databuf);
    if (inbuf)
        free_cma_mem(inbuf);
    if (outbuf)
        free_cma_mem(outbuf);
    if (fdjpeg > 0)
	    close(fdjpeg);
    if (fddata > 0)
        close(fddata);
    rt->finish = 1;
    return (void*)-1;
}