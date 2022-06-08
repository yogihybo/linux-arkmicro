#ifndef __ARK_CARLINK_H__
#define __ARK_CARLINK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>

#include "ark_api.h"


#define MAX_STREAM_BUFFER_SIZE	        (1024*1024)
#define RAW_STRM_TYPE_H264		104

#define DISPLAY_DATA_ID			0x4449

struct display_data {
	sem_t sem;
	int init;
	int kernel_used;
	int avin_used;
	int display_mode;
	int active_pid;
	video_handle *active_handle;
	screen_info screen;
};

#ifdef __cplusplus
}
#endif

#endif

