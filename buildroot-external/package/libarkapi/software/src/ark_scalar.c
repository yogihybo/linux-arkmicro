#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/syscall.h>
#include <semaphore.h>
#include "../include/ark_api.h"


/* scale module */
struct ark_scale_param {
	unsigned int iyaddr;
	unsigned int iuaddr;
	unsigned int ivaddr;
	scalar_in_format iformat;
	unsigned int iwidth;
	unsigned int iheight;
	unsigned int ix;
	unsigned int iy;
	unsigned int iwinwidth;
	unsigned int iwinheight;
	unsigned int left_cut;
	unsigned int right_cut;
	unsigned int up_cut;
	unsigned int bottom_cut;
	unsigned int owidth;
	unsigned int oheight;
	unsigned int oyaddr;
	unsigned int ouaddr;
	unsigned int ovaddr;
	scalar_out_format oformat;	//Only ark1668e.
	scale_rotate rotate;		//Only ark1668e.
};


#define SCALE_IOCTL_BASE			0x99
#define SCALE_IOCTL_START				_IOW(SCALE_IOCTL_BASE, 0, struct ark_scale_param)
#define SCALE_IOCTL_START_NO_WAIT		_IOW(SCALE_IOCTL_BASE, 1, struct ark_scale_param)
#define SCALE_IOCTL_WAIT_IDLE			_IO(SCALE_IOCTL_BASE, 2)
#define SCALE_IOCTL_GET_BUSY_SYATUS		_IOR(SCALE_IOCTL_BASE, 3, int)


static int fd_scale = -1;
static int sem_init_state = 0;
static sem_t ark_scalar_sem;


int arkapi_scalar_lock(void)
{
	int ret = 0;

	if(!sem_init_state) {
		if (sem_init(&ark_scalar_sem, 1, 1) < 0) {
			printf("%s sem_init fail\n", __func__);
			return ret;
		}
		sem_init_state = 1;
	}

	ret = sem_wait(&ark_scalar_sem);
	if (ret < 0 ) {
		printf("%s sem_wait fail, error=%d.\n", __func__, ret);
	}

	return ret;
}

int arkapi_scalar_unlock(void)
{
	int ret = 0;

	if(sem_init_state) {
		ret = sem_post(&ark_scalar_sem);
			if (ret < 0) {
			printf("%s sem_post fail, error=%d.\n", __func__, ret);
		}
	}
	return ret;
}

int arkapi_scalar(
	unsigned int iyaddr,
	unsigned int iuaddr,
	unsigned int ivaddr,
	scalar_in_format iformat,
	unsigned int iwidth,
	unsigned int iheight,
	unsigned int iwindow_x,
	unsigned int iwindow_y,
	unsigned int iwinwidth,
	unsigned int iwinheight,
	unsigned int left_cut,
	unsigned int right_cut,
	unsigned int up_cut,
	unsigned int bottom_cut,
	unsigned int owidth,
	unsigned int oheight,
	unsigned int oyaddr,
	unsigned int ouaddr,
	unsigned int ovaddr,
	scalar_out_format oformat,
	scale_rotate rotate)
{
	struct ark_scale_param scale_param = {0};
	int ret;

	if(fd_scale == -1) {
		fd_scale = open("/dev/axi_scale", O_RDWR);
		if (fd_scale < 0) {
			printf("%s: open fd_scale fail.\n", __func__);
			arkapi_scalar_unlock();
			return -1;
		}
	}

	scale_param.iyaddr = (unsigned int)iyaddr;
	scale_param.iuaddr = iuaddr;
	scale_param.ivaddr = ivaddr;
	scale_param.ix = iwindow_x;
	scale_param.iy = iwindow_y;
	scale_param.iwinwidth = iwinwidth;
	scale_param.iwinheight = iwinheight;
	scale_param.iwidth = iwidth;
	scale_param.iheight = iheight;
	scale_param.iformat = iformat;
	scale_param.left_cut = left_cut;
	scale_param.right_cut = right_cut;
	scale_param.up_cut = up_cut;
	scale_param.bottom_cut = bottom_cut;
	scale_param.owidth = owidth;
	scale_param.oheight = oheight;
	scale_param.oyaddr = oyaddr;
	scale_param.ouaddr = ouaddr;
	scale_param.ovaddr = ovaddr;
	scale_param.oformat = oformat;
	scale_param.rotate = rotate;

	ret = ioctl(fd_scale, SCALE_IOCTL_START, &scale_param);
	if (ret != 0)
		printf("%s, SCALE_IOCTL_START fail.\n", __func__);
	//close(fd_scale);

	return ret;
}

int arkapi_scalar_nowait(
	unsigned int iyaddr,
	unsigned int iuaddr,
	unsigned int ivaddr,
	scalar_in_format iformat,
	unsigned int iwidth,
	unsigned int iheight,
	unsigned int iwindow_x,
	unsigned int iwindow_y,
	unsigned int iwinwidth,
	unsigned int iwinheight,
	unsigned int left_cut,
	unsigned int right_cut,
	unsigned int up_cut,
	unsigned int bottom_cut,
	unsigned int owidth,
	unsigned int oheight,
	unsigned int oyaddr,
	unsigned int ouaddr,
	unsigned int ovaddr,
	scalar_out_format oformat,
	scale_rotate rotate)
{
	struct ark_scale_param scale_param = {0};
	int ret;

	if(fd_scale == -1) {
		fd_scale = open("/dev/axi_scale", O_RDWR);
		if (fd_scale < 0) {
			printf("%s: open fd_scale fail.\n", __func__);
			arkapi_scalar_unlock();
			return -1;
		}
	}

	scale_param.iyaddr = (unsigned int)iyaddr;
	scale_param.iuaddr = iuaddr;
	scale_param.ivaddr = ivaddr;
	scale_param.ix = iwindow_x;
	scale_param.iy = iwindow_y;
	scale_param.iwinwidth = iwinwidth;
	scale_param.iwinheight = iwinheight;
	scale_param.iwidth = iwidth;
	scale_param.iheight = iheight;
	scale_param.iformat = iformat;
	scale_param.left_cut = left_cut;
	scale_param.right_cut = right_cut;
	scale_param.up_cut = up_cut;
	scale_param.bottom_cut = bottom_cut;
	scale_param.owidth = owidth;
	scale_param.oheight = oheight;
	scale_param.oyaddr = oyaddr;
	scale_param.ouaddr = ouaddr;
	scale_param.ovaddr = ovaddr;
	scale_param.oformat = oformat;
	scale_param.rotate = rotate;

	ret = ioctl(fd_scale, SCALE_IOCTL_START_NO_WAIT, &scale_param);
	if (ret != 0)
		printf("%s, SCALE_IOCTL_START_NO_WAIT fail.\n", __func__);
	//close(fd_scale);

	return ret;
}

int arkapi_scalar_wait_idle(void)
{
	int ret;

	if(fd_scale == -1) {
		fd_scale = open("/dev/axi_scale", O_RDWR);
		if (fd_scale < 0) {
			printf("%s: open fd_scale fail.\n", __func__);
			arkapi_scalar_unlock();
			return -1;
		}
	}

	ret = ioctl(fd_scale, SCALE_IOCTL_WAIT_IDLE, NULL);
	if (ret != 0)
		printf("%s, SCALE_IOCTL_WAIT_IDLE fail.\n", __func__);
	//close(fd_scale);

	return ret;
}

int arkapi_n141_scalar(
	unsigned int iyaddr,
	unsigned int iuaddr,
	unsigned int ivaddr,
	unsigned int window_x,
	unsigned int window_y,
	unsigned int window_width,
	unsigned int window_height,
	unsigned int iwidth,
	unsigned int iheight,
	unsigned int owidth,
	unsigned int oheight,
	unsigned int ostride,
	unsigned int format,
	unsigned int yout,
	unsigned int uout,
	unsigned int vout)
{
	return arkapi_scalar(iyaddr,iuaddr,ivaddr,format, iwidth, iheight,
					window_x,window_y, window_width, window_height,
					0, 0, 0, 0, owidth, oheight, yout, uout, vout, 0, 0);
}

int arkapi_n141_scalar_lock(void)
{
	return arkapi_scalar_lock();
}

int arkapi_n141_scalar_unlock(void)
{
	return arkapi_scalar_unlock();
}
