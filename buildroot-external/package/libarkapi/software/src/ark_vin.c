#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <linux/ioctl.h> 

#include "ark_vin.h"
#include "ark_api.h"

#define VIN_FILE_PATH	"/dev/video0"

extern int arkapi_set_vin_start(int start);

int arkapi_open_vin(void)
{
	int fd = open(VIN_FILE_PATH, O_RDWR);
	if (fd == -1) {
		printf("open vin device %s failure.\n", VIN_FILE_PATH);
		return -1;
	}	
	return fd;
}

int arkapi_close_vin(int vin_fd)
{
	if (vin_fd != -1) {
		close(vin_fd);	
	}
	return 0;
}

int arkapi_vin_config(int vin_fd,int progressive, int itu601en)
{
	struct vin_para vin_parm;
	int ret = 0;
	vin_parm.progressive = progressive;
	vin_parm.itu601en = itu601en;
	ret = ioctl(vin_fd, VIN_CONFIG, &vin_parm);
	if(ret < 0)
		printf("arkapi_vin_config error\n");
	return ret;
}

int arkapi_vin_start(int vin_fd)
{
	int ret = 0;
	arkapi_set_vin_start(1);
	ret = ioctl(vin_fd, VIN_START);
	if(ret < 0)
		printf("arkapi_vin_start error\n");
	return ret;
}

int arkapi_vin_stop(int vin_fd)
{
	int ret = 0;
	ret = ioctl(vin_fd, VIN_STOP);
	if(ret < 0)
		printf("arkapi_vin_stop error\n");
	arkapi_set_vin_start(0);
	return ret;	
}

int arkapi_vin_detect_signal(int vin_fd)
{
	struct v4l2_input tV4l2inp;
	int ret;
	ret = ioctl(vin_fd, VIDIOC_G_INPUT, &tV4l2inp.status); 
	if (ret < 0){ 
		printf("arkapi_vin_detect_signal error\n");
		ret = -1;
	}
	return tV4l2inp.status;
}

int arkapi_vin_switch_channel(int vin_fd, enum dvr_source source)
{
	int ret = 0;
	int channel = 0;
	channel = source;
	ret = ioctl(vin_fd, VIN_SWITCH_CHANNEL,&channel);
	if(ret < 0)
		printf("arkapi_vin_switch_channel error\n");
	return ret;	
}

