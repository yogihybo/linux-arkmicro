#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <semaphore.h>
#include <linux/fb.h>
#include <string.h>
#include <math.h>

#include "ark_common.h"
#include "ark_display.h"


static char *fb_path[MAX_LAYERS] = {
	"/dev/fb0",
	"/dev/fb1",
	"/dev/fb2",
	"/dev/fb3",
	"/dev/fb4"
};

static int ark1668e_set_rgb_lcd_vp(int fb_fd, disp_color* vp);
static int ark1668e_set_yuv_lcd_vp(int fb_fd, disp_color* vp);

static int get_platform_info(struct ark_platform_info *platform)
{
	int ret, fb_fd;

	if (platform == NULL) {
		printf("%s: pscreen null, error.\n",__func__);
		return -EINVAL;
	}

	fb_fd = open(fb_path[0], O_RDWR);
	if (fb_fd < 0) {
		printf("%s: open %s fail.\n",__func__, fb_path[0]);
		return -ENOENT;
	}

	memset(platform, 0 ,sizeof(struct ark_platform_info));
	ret = ioctl(fb_fd, ARKFB_GET_PLATFORM_INFO, platform);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		close(fb_fd);
		return ret;
	}

	if(platform->type < 0 || platform->type >= ARK_PLATFORM_MAX){
		printf("%s: platform->type=%d, error.\n", __func__, platform->type);
		close(fb_fd);
		return -EINVAL;
	}

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	if (platform->type != ARK_PLATFORM_ARK1668) {
#elif LIBARKAPI_PLATFORM == LIBARKAPI_ARKN141
	if (platform->type != ARK_PLATFORM_ARKN141) {
#elif LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668E
	if (platform->type != ARK_PLATFORM_ARK1668E) {
#endif
		printf("%s error, the platform (%d,%d) is not match.\n",
			LIBARKAPI_PLATFORM, platform->type);
		close(fb_fd);
		return -EINVAL;
	}

	ark_dbg("%s: success, platform->type=%d\n", __func__, platform->type);

	close(fb_fd);

	return SUCCESS;
}

static int disp_handle_check(disp_handle *handle)
{
	int ret, layer, pid;

	if(!handle){
		printf("pid=%d: handle=0x%p, error.\n", handle);
		return -EINVAL;
	}

	if(handle->layer_id < 0 || handle->layer_id >= MAX_LAYERS){
		printf(" layer_id=%d, check layer_id error.\n", handle->layer_id);
		return -EINVAL;
	}

	if(handle->fd <= 0){
		printf("fd =%d, check fd error.\n", handle->fd);
		return -EINVAL;
	}

	return SUCCESS;
}

int arkapi_display_get_screen_info(screen_info *screen)
{
	int ret, fb_fd;

	if (screen == NULL) {
		printf("%s: screen null, error.\n",__func__);
		return -EINVAL;
	}

	fb_fd = open(fb_path[0], O_RDWR);
	if (fb_fd < 0) {
		printf("%s: open %s fail.\n",__func__, fb_path[0]);
		return -ENOENT;
	}

	ret = ioctl(fb_fd, ARKFB_GET_SCREEN_INFO, screen);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		close(fb_fd);
		return ret;
	}

	if(0 == screen->width || 0 == screen->height){
		printf("%s: width=%d ,height=%d, error.\n", __func__, screen->width, screen->height);
		close(fb_fd);
		return -EINVAL;
	}


	ark_dbg("%s: success, %dx%d\n", __func__, screen->width, screen->height);

	close(fb_fd);

	return SUCCESS;
}


int arkapi_display_set_screen_info(screen_info *screen)
{
	int ret, fb_fd;

	if (screen == NULL) {
		printf("%s: pscreen null, error.\n",__func__);
		return -EINVAL;
	}

	if(0 == screen->width || 0 == screen->height){
		printf("%s: width=%d ,height=%d, error.\n", __func__, screen->width, screen->height);
		return -EINVAL;
	}

	fb_fd = open(fb_path[0], O_RDWR);
	if (fb_fd < 0) {
		printf("%s: open %s fail.\n",__func__, fb_path[0]);
		return -ENOENT;
	}

	ret = ioctl(fb_fd, ARKFB_SET_SCREEN_INFO, screen);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		close(fb_fd);
		return ret;
	}

	ark_dbg("%s: success, %dx%d\n", __func__, screen->width, screen->height);

	close(fb_fd);

	return SUCCESS;
}

static disp_handle *disp_handle_init(void)
{
	disp_handle *handle = NULL;

	handle = (disp_handle *)malloc(sizeof(disp_handle));
	if(!handle){
		printf("%s: malloc handle error.\n", __func__);
		return NULL;
	}

	memset(handle, 0 ,sizeof(disp_handle));
	handle->display_mode = DISP_NONE;
	get_platform_info(&handle->platform);
	arkapi_display_get_screen_info(&handle->screen);

	return handle;
}

disp_handle *arkapi_display_open_layer(enum ark_disp_layer layer)
{
	disp_handle *handle;
	int fd = -1;

	if (layer >= MAX_LAYERS || layer < 0){
		printf("%s: layer=%d, error.\n",__func__, layer);
		return NULL;
	}

	fd = open(fb_path[layer], O_RDWR);
	if (fd < 0) {
		printf("open %s fail.\n", fb_path[layer]);
		return NULL;
	}

	handle = disp_handle_init();
	if(!handle)
		return NULL;

	handle->fd = fd;
	handle->layer_id = layer;

	ark_dbg("open %s success.\n", fb_path[layer]);

	return handle;
}

void arkapi_display_close_layer(disp_handle *handle)
{
	int layer;

	if(!handle){
		printf("%s: handle=0x%p, error.\n", __func__, handle);
		return;
	}

	layer = handle->layer_id;
	if (handle->fd > 0)
		close(handle->fd);

	free(handle);

	ark_dbg("%s: close %s success.\n", __func__, fb_path[layer]);
}

int arkapi_display_show_layer(disp_handle *handle)
{
	int ret, layer;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	ret = ioctl(handle->fd, ARKFB_SHOW_WINDOW, NULL);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
	}

	handle->show = 1;

	return ret;
}

int arkapi_display_hide_layer(disp_handle *handle)
{
	int layer, ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	ret = ioctl(handle->fd, ARKFB_HIDE_WINDOW, NULL);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
	}

	handle->show = 0;

	return ret;

}

int arkapi_display_recycle_layer(enum ark_disp_layer layer)
{


	return SUCCESS;

}

int arkapi_display_force_show_layer(enum ark_disp_layer layer)
{
	int ret, fb_fd;

	if (layer >= MAX_LAYERS || layer < 0)
		return -EINVAL;


	fb_fd = open(fb_path[layer], O_RDWR);
	if (fb_fd < 0) {
		printf("open %s fail.\n", fb_path[layer]);
		return -ENOENT;
	}

	ret = ioctl(fb_fd, ARKFB_SHOW_WINDOW, NULL);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
	}
	close(fb_fd);

	return SUCCESS;
}

int arkapi_display_force_hide_layer(enum ark_disp_layer layer)
{
	int ret, fb_fd;

	if (layer >= MAX_LAYERS || layer < 0)
		return -EINVAL;

	fb_fd = open(fb_path[layer], O_RDWR);
	if (fb_fd < 0) {
		printf("open %s fail.\n", fb_path[layer]);
		return -ENOENT;
	}

	ret = ioctl(fb_fd, ARKFB_HIDE_WINDOW, NULL);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
	}

	close(fb_fd);

	return SUCCESS;
}

int arkapi_display_set_layer_pos(disp_handle *handle, int x, int y)
{
	int layer, ret, data;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	data = (y << 16) | x;
	ret = ioctl(handle->fd, ARKFB_SET_WINDOW_POS, &data);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return ret;
	}

	handle->win.pos_x = x;
	handle->win.pos_y = y;

	return SUCCESS;
}

int arkapi_display_set_layer_size(disp_handle *handle, int width, int height)
{
	int layer, ret, data;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	data = (height << 16) | width;
	ret = ioctl(handle->fd, ARKFB_SET_WINDOW_SIZE, &data);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return ret;
	}

	handle->win.width  = width;
	handle->win.height = height;

	return SUCCESS;

}

int arkapi_display_set_layer_format(disp_handle *handle, int format)
{
	int layer, ret, data;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	data = format;
	ret = ioctl(handle->fd, ARKFB_SET_WINDOW_FORMAT, &data);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return ret;
	}

	handle->win.format = format;

	return SUCCESS;
}

int arkapi_display_set_layer_addr(disp_handle *handle, u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr)
{
	int ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	if(!pyrgbaddr){
		printf("%s: pyrgbaddr=0, error.\n", __func__);
		return -EINVAL;
	}

	handle->addr.yaddr = pyrgbaddr;
	handle->addr.cbaddr = pcbaddr;
	handle->addr.craddr = pcraddr;
	ret = ioctl(handle->fd, ARKFB_SET_WINDOW_ADDR, &handle->addr);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return ret;
	}

	return SUCCESS;
}

int arkapi_set_layer_priority(disp_handle *handle,int video_pri, int video2_pri, int win1_pri,int win2_pri,int win3_pri)
{
	int ret;
	unsigned int layer_id[5];
	if(video_pri+video2_pri+win1_pri+win2_pri+win3_pri != 10)
	{
		printf("%s: para error.\n",__func__);
		return -EINVAL;
	}

	layer_id[0] = video_pri;
	layer_id[1] = video2_pri;
	layer_id[2] = win1_pri;
	layer_id[3] = win2_pri;
	layer_id[4] = win3_pri;

	ret = ioctl(handle->fd, ARKFB_SET_WINDOW_PRIORITY, &layer_id);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return ret;
	}

	return SUCCESS;
}

int arkapi_display_get_layer_addr(disp_handle *handle, u32 *pyrgbaddr, u32 *pcbaddr, u32 *pcraddr)
{
	disp_addr addr = {0};
	int ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	if(!pyrgbaddr){
		printf("%s: pyrgbaddr=null, error.\n", __func__);
		return -EINVAL;
	}

	ret = ioctl(handle->fd, ARKFB_GET_WINDOW_ADDR, &addr);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return ret;
	}

	if (pyrgbaddr)
		*pyrgbaddr = addr.yaddr;

	if (pcbaddr)
		*pcbaddr = addr.cbaddr;

	if (pcraddr)
		*pcraddr = addr.craddr;

	return SUCCESS;
}

int arkapi_display_set_layer_scaler(disp_handle *handle, disp_scaler* spara)
{
	int ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	if(handle->platform.type == ARK_PLATFORM_ARK1668) {
		if( handle->layer_id < TVOUT_LAYER){
			printf("%s: layer <= fb2, can not scaler.\n", __func__);
			return -EINVAL;
		}

		if(!spara || spara->src_w <= 0 || spara->out_w <= 0){
			printf("%s: spara data error.\n", __func__);
			return -EINVAL;
		}

		ret = ioctl(handle->fd, ARKFB_SET_WINDOW_SCALER, spara);
		if (ret != SUCCESS) {
			printf("%s: ioctl error.\n",__func__);
			return ret;
		}

		memcpy(&handle->scaler, &spara, sizeof(disp_scaler));
	} else if(handle->platform.type == ARK_PLATFORM_ARKN141) {
#if 0
		if( handle->layer_id > 2){
			printf("%s: layer > fb2, can not scaler.\n", __func__);
			return -EINVAL;
		}

		if(!spara || !spara->iyaddr || !spara->oyaddr || spara->src_w <= 0 || spara->src_h <= 0){
			printf("%s: spara data error.\n", __func__);
			return -EINVAL;
		}

		arkapi_scalar_lock();
		ret = arkapi_scalar(
			spara->iyaddr, spara->iuaddr, spara->ivaddr,
			spara->format,
 			spara->src_w, spara->src_h,
 			spara->win_x, spara->win_y, spara->win_w, spara->win_h,
			0, 0, 0, 0,
			spara->out_w, spara->out_h,
			spara->oyaddr, spara->ouaddr, spara->ovaddr,
			spara->format, SCALE_ROTATE_0);
		arkapi_scalar_unlock();
		if(ret) {
			return ret;
		}
#else
		printf("%s: arkn141 lcd layer not support scale. please using arkapi_scalar().\n", __func__);
		return -EINVAL;
#endif
	}else if(handle->platform.type == ARK_PLATFORM_ARK1668E) {
		if(!spara || spara->src_w <= 0 || spara->out_w <= 0){
			printf("%s: spara data error.\n", __func__);
			return -EINVAL;
		}

		ret = ioctl(handle->fd, ARKFB_SET_WINDOW_SCALER, spara);
		if (ret != SUCCESS) {
			printf("%s: ioctl error.\n",__func__);
			return ret;
		}

		memcpy(&handle->scaler, &spara, sizeof(disp_scaler));
	}
	return SUCCESS;
}

int arkapi_display_wait_for_vsync(disp_handle *handle)
{

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	return ioctl(handle->fd, ARKFB_WAITFORVSYNC, NULL);
}

int arkapi_display_get_vsync_status(disp_handle *handle)
{
	unsigned int state;
	int ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	ret = ioctl(handle->fd, ARKFB_GET_VSYNC_STATUS, (unsigned long)&state);
	if(ret)
		return -EINVAL;

	return state;
}

int arkapi_display_set_mode(disp_handle *handle, int mode)
{
	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	////////////////////
	handle->display_mode = mode;

	return SUCCESS;
}


int arkapi_display_set_layer_pos_atomic(disp_handle *handle, int x, int y)
{
	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	handle->win.pos_x = x;
	handle->win.pos_y = y;
	handle->atomic_stat |= ATOMIC_SET_LAYER_POS;

	return SUCCESS;
}

int arkapi_display_set_layer_size_atomic(disp_handle *handle, int width, int height)
{

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	handle->win.width  = width;
	handle->win.height = height;
	handle->atomic_stat |= ATOMIC_SET_LAYER_SIZE;

	return SUCCESS;
}

int arkapi_display_set_layer_format_atomic(disp_handle *handle, int format)
{

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	handle->win.format = format;
	handle->atomic_stat |= ATOMIC_SET_LAYER_FMT;

	return SUCCESS;
}

int arkapi_display_set_layer_addr_atomic(disp_handle *handle, u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr)
{

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	if(!pyrgbaddr){
		printf("%s: pyrgbaddr=0 error.\n", __func__);
		return -EINVAL;
	}

	handle->addr.yaddr  = pyrgbaddr;
	handle->addr.cbaddr = pcbaddr;
	handle->addr.craddr = pcraddr;
	handle->addr.wait_vsync = 0;
	handle->atomic_stat |= ATOMIC_SET_LAYER_ADDR;

	return SUCCESS;
}

int arkapi_display_set_layer_scaler_atomic(disp_handle *handle, disp_scaler* spara)
{
	int ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	if(handle->platform.type == ARK_PLATFORM_ARK1668) {
		if( handle->layer_id < TVOUT_LAYER){
			printf("%s: layer < TVOUT_LAYER, can not scaler.\n", __func__);
			return -EINVAL;
		}


		if(!spara || spara->src_w <= 0 || spara->out_w <= 0){
			printf("%s: spara data error.\n", __func__);
			return -EINVAL;
		}

		memcpy(&handle->scaler, spara, sizeof(disp_scaler));
		handle->atomic_stat |= ATOMIC_SET_LAYER_SCALER;
	} else if(handle->platform.type == ARK_PLATFORM_ARK1668E) {
		if(!spara || spara->src_w <= 0 || spara->out_w <= 0){
			printf("%s: spara data error.\n", __func__);
			return -EINVAL;
		}
		memcpy(&handle->scaler, spara, sizeof(disp_scaler));
		handle->atomic_stat |= ATOMIC_SET_LAYER_SCALER;
	}
	return SUCCESS;
}

int arkapi_display_layer_update_commit(disp_handle *handle)
{
	disp_atomic atomic;
	int ret;

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	atomic.layer = handle->layer_id;
	atomic.pos_x = handle->win.pos_x;
	atomic.pos_y = handle->win.pos_y;
	atomic.width = handle->win.width;
	atomic.height = handle->win.height;
	atomic.format = handle->win.format;
	atomic.yuyv_order = handle->win.yuyv_order;
	atomic.rgb_order  = handle->win.rgb_order;
	atomic.atomic_stat= handle->atomic_stat;
	memcpy(&atomic.scaler, &handle->scaler, sizeof(disp_scaler));
	memcpy(&atomic.addr, &handle->addr, sizeof(disp_addr));

	ret = ioctl(handle->fd, ARKFB_SET_WINDOW_ATOMIC, &atomic);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
	}

	//ark_dbg("%s layer=%d.\n", __func__, layer);

	return ret;
}

int arkapi_display_layer_set_color(enum ark_disp_layer layer, disp_color* vp)
{
	platform_info platform;
	disp_reg reg;
	u32 data = 0;
	int ret, fb_fd;

	if ((layer >= MAX_LAYERS || layer < 0) || !vp)
		return -EINVAL;

	if (!vp->brightness && !vp->contrast && !vp->saturation && !vp->hue){
		printf("brightness contrast saturation hue = 0,error.\n");
		return -EINVAL;
	}

	fb_fd = open(fb_path[layer], O_RDWR);
	if (fb_fd < 0) {
		printf("open %s fail.\n", fb_path[layer]);
		return -ENOENT;
	}

	memset(&platform, 0, sizeof(platform_info));
	get_platform_info(&platform);

	if (platform.type == ARK_PLATFORM_ARKN141){
		data |= vp->brightness << 0;
		data |=   vp->contrast << 8;
	} else if(platform.type == ARK_PLATFORM_ARK1668) {
		data |=   vp->contrast << 0;
		data |= vp->brightness << 8;
	} else if(platform.type == ARK_PLATFORM_ARK1668E) {
	}
	data |= vp->saturation << 16;
	data |= vp->hue << 24;
	reg.value = data;

	if (platform.type == ARK_PLATFORM_ARKN141){
		reg.addr = ARKN141_LCDC_BASE + ARKN141_VP_OFFSET;

	} else if(platform.type == ARK_PLATFORM_ARK1668) {
		switch (layer) {
			case PRIMARY_LAYER:
			reg.addr = ARK1668_LCDC_BASE + PRIMARY_LAYER_VP_OFFSET;
			break;

			case VIDEO_LAYER:
			reg.addr = ARK1668_LCDC_BASE + VIDEO_LAYER_VP_OFFSET;
			break;

			case OVER_LAYER:
			reg.addr = ARK1668_LCDC_BASE + OVER_LAYER_VP_OFFSET;
			break;

			case TVOUT_LAYER:
			reg.addr = ARK1668_LCDC_BASE + TVOUT_LAYER_VP_OFFSET;
			break;

			case AUX_LAYER:
			reg.addr = ARK1668_LCDC_BASE + AUX_LAYER_VP_OFFSET;
			break;
		}
	} else if(platform.type == ARK_PLATFORM_ARK1668E) {
		int ret;
		int format;
		ret = ioctl(fb_fd, ARKFB_GET_WINDOW_FORMAT, &format);
		if (ret != SUCCESS) {
			printf("%s: ioctl error.\n",__func__);
			close(fb_fd);
			return ret;
		}
		if((format >= ARK_LCDC_FORMAT_RGBI555) && (format <= 12))
			ret = ark1668e_set_rgb_lcd_vp(fb_fd, vp);
		else
			ret = ark1668e_set_yuv_lcd_vp(fb_fd, vp);
		close(fb_fd);
		return ret;
	}

	ret = ioctl(fb_fd, ARKFB_SET_REG_VALUE, &reg);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		close(fb_fd);
		return ret;
	}

	close(fb_fd);

	return SUCCESS;

}


int arkapi_display_layer_get_color(enum ark_disp_layer layer, disp_color* vp)
{
	platform_info platform;
	disp_reg reg;
	u32 data = 0;
	int ret, fb_fd;

	if ((layer >= MAX_LAYERS || layer < 0) || !vp)
		return -EINVAL;

	fb_fd = open(fb_path[layer], O_RDWR);
	if (fb_fd < 0) {
		printf("open %s fail.\n", fb_path[layer]);
		return -ENOENT;
	}

	memset(&platform, 0, sizeof(platform_info));
	get_platform_info(&platform);

	if (platform.type == ARK_PLATFORM_ARKN141){
		reg.addr = ARKN141_LCDC_BASE + ARKN141_VP_OFFSET;
	} else if(platform.type == ARK_PLATFORM_ARK1668) {
		switch (layer) {
			case PRIMARY_LAYER:
			reg.addr = ARK1668_LCDC_BASE + PRIMARY_LAYER_VP_OFFSET;
			break;

			case VIDEO_LAYER:
			reg.addr = ARK1668_LCDC_BASE + VIDEO_LAYER_VP_OFFSET;
			break;

			case OVER_LAYER:
			reg.addr = ARK1668_LCDC_BASE + OVER_LAYER_VP_OFFSET;
			break;

			case TVOUT_LAYER:
			reg.addr = ARK1668_LCDC_BASE + TVOUT_LAYER_VP_OFFSET;
			break;

			case AUX_LAYER:
			reg.addr = ARK1668_LCDC_BASE + AUX_LAYER_VP_OFFSET;
			break;
		}
	} else if(platform.type == ARK_PLATFORM_ARK1668E) {
		struct ark_disp_vp vp_info;
		memset(&vp_info, 0, sizeof(struct ark_disp_color));
		ret = ioctl(fb_fd, ARKFB_GET_VP_INFO, &vp_info);
		if (ret != SUCCESS) {
			printf("%s: ioctl error.\n",__func__);
			close(fb_fd);
			return ret;
		}
		vp->contrast   = vp_info.color.contrast;
		vp->brightness = vp_info.color.brightness;
		vp->saturation = vp_info.color.saturation;
		vp->hue 	   = vp_info.color.hue;
		close(fb_fd);
		return SUCCESS;
	}

	ret = ioctl(fb_fd, ARKFB_GET_REG_VALUE, &reg);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		close(fb_fd);
		return ret;
	}

	if (platform.type == ARK_PLATFORM_ARKN141){
		vp->brightness = (reg.value & 0x000000FF) >> 0;
		vp->contrast   = (reg.value & 0x0000FF00) >> 8;
	}else{
		vp->contrast   = (reg.value & 0x000000FF) >> 0;
		vp->brightness = (reg.value & 0x0000FF00) >> 8;
	}

	vp->saturation = (reg.value & 0x00FF0000) >> 16;
	vp->hue        = (reg.value & 0xFF000000) >> 24;

	close(fb_fd);

	return SUCCESS;

}

/* ark1668e private interface */
#define ARK1668E_VP_BRIGHTNESS_RANGE	(400)	//range:-200~200
#define ARK1668E_VP_HUE_RANGE			(360)	//range:0~360
#define ARK1668E_VP_CONTRAST_RANGE		(4)		//range:0~4
#define ARK1668E_VP_SATURATION_RANGE	(4)		//range:0~4
static int ark1668e_set_rgb_lcd_vp(int fb_fd, disp_color* vp)
{
	float i_bright = (float)vp->brightness * ARK1668E_VP_BRIGHTNESS_RANGE / 255.0 - 200.0;
	float i_hue = (float)vp->hue * ARK1668E_VP_HUE_RANGE / 255.0;
	float i_contrast = (float)vp->contrast * ARK1668E_VP_CONTRAST_RANGE / 255.0;
	float i_saturation = (float)vp->saturation * ARK1668E_VP_SATURATION_RANGE / 255.0;
	float M00 = i_contrast;//���ƶԱȶ�
	float M03 = i_bright + (1 - i_contrast) * 128; //��������
	float M11 = i_saturation*cos(i_hue / 360 * 2 * 3.14159);
	float M12 = i_saturation*sin(i_hue / 360 * 2 * 3.14159);
	unsigned int register01_00;
	unsigned int register10_02;
	unsigned int register12_11;
	unsigned int register21_20;
	unsigned int register30_22;
	unsigned int register32_31;
	unsigned int val = 0;
	struct ark_disp_vp vp_info;
	int i, j, k;
	int ret;

	if(fb_fd < 0) {
		printf("%s: Invalid fd:%d.\n",__func__, fb_fd);
		return -1;
	}
	//��������
	float M[3][4] = {
		{ M00, 0,	 0,    M03-16*M00 },
		{ 0,   M11,  -M12,	-128*M11+128*M12 },
		{ 0,  M12,	M11,  -128*M12-128*M11 }
	};

	//yuv2rgb����
	float Mycc2rgb[3][3] = {
		{ 1.1689, 0, 1.6023 },
		{ 1.1689, -0.3933, -0.8162 },
		{ 1.1689, 2.0251, 0 }
	};
	float Mat[3][4] = {
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};

	float Mrgb2ycc[3][3] = {
		{ 0.2558, 0.5022, 0.0975 },
		{ -0.1476, -0.2899, 0.4375 },
		{ 0.4375, -0.3664, -0.0711 }
	};
	float Mat1[3][4] = {
		{ 0, 0, 0, M03},
		{ 0, 0, 0, 0},
		{ 0, 0, 0, 0}
	};
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			for (k = 0; k < 3; k++) {
				Mat1[i][j] = Mat1[i][j] + M[i][k] * Mrgb2ycc[k][j];
			}
		}

	}
	float Mat2[3][4] = {
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 4; j++) {
			for (k = 0; k < 3; k++) {
				Mat2[i][j] = Mat2[i][j] + Mycc2rgb[i][k] * Mat1[k][j];
			}
		}
	}

	int matrix2[3][4];
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			matrix2[i][j] = round(Mat2[i][j] * 256);
		}

	}
	for (i = 0; i < 3; i++) {
		matrix2[i][3] = round(Mat2[i][3]);
	}

	register01_00 = 0x01000000;
	if(matrix2[0][0]< 0) {
		matrix2[0][0] = 0 - matrix2[0][0];
		val = ~matrix2[0][0]+1;
		val = val & 0x7FF;
		register01_00 |= (1 << 11)|(val)<< 0;
	} else
		register01_00 |= (matrix2[0][0])<< 0;

	if(matrix2[0][1]< 0) {
		matrix2[0][1] = 0 - matrix2[0][1];
		val = ~matrix2[0][1]+1;
		val = val & 0x7FF;
		register01_00 |= (1 << 23)|(val)<< 12;
	} else
		register01_00 |= (matrix2[0][1])<< 12;
	//printf("register01_00 = 0x%x\n",register01_00);

	register10_02 = 0;
	if(matrix2[0][2]< 0) {
		matrix2[0][2] = 0 - matrix2[0][2];
		val = ~ matrix2[0][2]+1;
		val = val & 0x7FF;
		register10_02 |= (1 << 11)|(val)<< 0;
	} else
		register10_02 |= (matrix2[0][2])<< 0;

	if(matrix2[1][0]< 0) {
		matrix2[1][0] = 0 - matrix2[1][0];
		val = ~matrix2[1][0]+1;
		val = val & 0x7FF;
		register10_02 |= (1 << 23)|(val)<< 12;
	} else
		register10_02 |= (matrix2[1][0])<< 12;
	//printf("register10_02 = 0x%x\n",register10_02);

	register12_11 = 0;
	if(matrix2[1][1]< 0) {
		matrix2[1][1] = 0 - matrix2[1][1];
		val = ~matrix2[1][1]+1;
		val = val & 0x7FF;
		register12_11 |= (1 << 11)|(val)<< 0;
	} else
		register12_11 |= (matrix2[1][1])<< 0;

	if(matrix2[1][2]< 0) {
		matrix2[1][2] = 0 - matrix2[1][2];
		val = ~matrix2[1][2]+1;
		val = val & 0x7FF;
		register12_11 |= (1 << 23)|(val)<< 12;
	} else
		register12_11 |= (matrix2[1][2])<< 12;
	//printf("register12_11 = 0x%x\n",register12_11);

	register21_20 = 0;
	if(matrix2[2][0] < 0) {
		matrix2[2][0] = 0 - matrix2[2][0];
		val = ~matrix2[2][0]+1;
		val = val & 0x7FF;
		register21_20 |= (1 << 11)|(val)<< 0;
	} else
		register21_20 |= (matrix2[2][0])<< 0;

	if(matrix2[2][1]< 0) {
		matrix2[2][1] = 0 - matrix2[2][1];
		val = ~matrix2[2][1] +1;
		val = val & 0x7FF;
		register21_20 |= (1 << 23)|(val)<< 12;
	} else
		register21_20 |= (matrix2[2][1])<< 12;
	//printf("register21_20 = 0x%x\n",register21_20);

	register30_22 = 0;
	if(matrix2[2][2]< 0) {
		matrix2[2][2] = 0 - matrix2[2][2];
		val = ~matrix2[2][2]+1;
		val = val & 0x7FF;
		register30_22 |= (1 << 11)|(val)<< 0;
	} else
		register30_22 |= (matrix2[2][2])<< 0;

	if(matrix2[0][3]< 0) {
		matrix2[0][3] = 0 - matrix2[0][3];
		val = ~ matrix2[0][3]+1;
		val = val & 0x7FF;
		register30_22 |= (1 << 23)|(val)<< 12;
	} else
		register30_22 |= (matrix2[0][3])<< 12;
	//printf("register30_22 = 0x%x\n",register30_22);

	register32_31 = 0;
	if(matrix2[1][3]< 0) {
		matrix2[1][3] = 0 - matrix2[1][3];
		val = ~ matrix2[1][3]+1;
		val = val & 0x7FF;
		register32_31 |= (1 << 11)|(val)<< 0;
	} else
		register32_31 |= (matrix2[1][3])<< 0;

	if(matrix2[2][3]< 0) {
		matrix2[2][3] = 0 - matrix2[2][3];
		val = ~matrix2[2][3]+1;
		val = val & 0x7FF;
		register32_31 |= (1 << 23)|(val)<< 12;
	} else
		register32_31 |= (matrix2[2][3])<< 12;
	//printf("register32_31 = 0x%x\n",register32_31);
	vp_info.color.brightness = vp->brightness;
	vp_info.color.contrast = vp->contrast;
	vp_info.color.hue = vp->hue;
	vp_info.color.saturation = vp->saturation;
	vp_info.reg[0] = register01_00;
	vp_info.reg[1] = register10_02;
	vp_info.reg[2] = register12_11;
	vp_info.reg[3] = register21_20;
	vp_info.reg[4] = register30_22;
	vp_info.reg[5] = register32_31;

	ret = ioctl(fb_fd, ARKFB_SET_VP_INFO, &vp_info);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return -1;
	}
	return 0;
}

static int ark1668e_set_yuv_lcd_vp(int fb_fd, disp_color* vp)
{
	float i_bright = (float)vp->brightness * ARK1668E_VP_BRIGHTNESS_RANGE / 255.0 - 200.0;
	float i_hue = (float)vp->hue * ARK1668E_VP_HUE_RANGE / 255.0;
	float i_contrast = (float)vp->contrast * ARK1668E_VP_CONTRAST_RANGE / 255.0;
	float i_saturation = (float)vp->saturation * ARK1668E_VP_SATURATION_RANGE / 255.0;
	float M00 = i_contrast;//���ƶԱȶ�
	float M03 = i_bright + (1 - i_contrast) * 128; //��������
	float M11 = i_saturation*cos(i_hue / 360 * 2 * 3.14159);
	float M12 = i_saturation*sin(i_hue / 360 * 2 * 3.14159);
	unsigned int register01_00;
	unsigned int register10_02;
	unsigned int register12_11;
	unsigned int register21_20;
	unsigned int register30_22;
	unsigned int register32_31;
	unsigned int val= 0;
	struct ark_disp_vp vp_info;
	int i, j, k;
	int ret;
	//��������
	float M[3][4] =
	{
		{ M00, 0,    0,    M03-16*M00 },
		{ 0,   M11,  -M12,  -128*M11+128*M12 },
		{ 0,  M12,  M11,  -128*M12-128*M11 }
	};
	//yuv2rgb����
	float Mycc2rgb[3][3] =
	{
		{ 1.1689, 0, 1.6023 },
		{ 1.1689, -0.3933, -0.8162 },
		{ 1.1689, 2.0251, 0 }
	};
	float Mat[3][4] =
	{
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }
	};
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 4; j++)
		{
			for (k = 0; k < 3; k++)
			{
				Mat[i][j] = Mat[i][j] + Mycc2rgb[i][k] * M[k][j];
			}
		}
	}
	int matrix[3][4];//���յ�3*3����
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			matrix[i][j] = round(Mat[i][j] * 256);
		}

	}
	for (i = 0; i < 3; i++)
	{
			matrix[i][3] = round(Mat[i][3]);

	}

	for(i=0;i<=2;i++)
		for(j=0;j<=3;j++)
		{
			if(matrix[i][j]>2047)
			{
				matrix[i][j] = 2047;
				//printf("matrix[%d][%d] = %d\n",i,j,matrix[i][j]);
			}
			else if(matrix[i][j]<-2047)
			{
				matrix[i][j] = -2047;
				//printf("matrix[%d][%d] = %d\n",i,j,matrix[i][j]);
			}
		}


	register01_00 = 0x01000000;
	if(matrix[0][0]< 0)
	{
		matrix[0][0] = 0 - matrix[0][0];
		val = ~matrix[0][0]+1;
		val = val & 0x7FF;
		register01_00 |= (1 << 11)|(val)<< 0;

	}
	else
		register01_00 |= (matrix[0][0])<< 0;

	if(matrix[0][1]< 0)
	{
		matrix[0][1] = 0 - matrix[0][1];
		val = ~matrix[0][1]+1;
		val = val & 0x7FF;
		register01_00 |= (1 << 23)|(val)<< 12;
	}
	else
		register01_00 |= (matrix[0][1])<< 12;

//	printf("register01_00 = 0x%x\n",register01_00);
	register10_02 = 0;
	if(matrix[0][2]< 0)
	{
		matrix[0][2] = 0 - matrix[0][2];
		val = ~ matrix[0][2]+1;
		val = val & 0x7FF;
		register10_02 |= (1 << 11)|(val)<< 0;
	}
	else
		register10_02 |= (matrix[0][2])<< 0;

	if(matrix[1][0]< 0)
	{
		matrix[1][0] = 0 - matrix[1][0];
		val = ~matrix[1][0]+1;
		val = val & 0x7FF;
		register10_02 |= (1 << 23)|(val)<< 12;
	}
	else
		register10_02 |= (matrix[1][0])<< 12;

//	printf("register10_02 = 0x%x\n",register10_02);


	register12_11 = 0;
	if(matrix[1][1]< 0)
	{
		matrix[1][1] = 0 - matrix[1][1];
		val = ~matrix[1][1]+1;
		val = val & 0x7FF;
		register12_11 |= (1 << 11)|(val)<< 0;
	}
	else
		register12_11 |= (matrix[1][1])<< 0;

	if(matrix[1][2]< 0)
	{
		matrix[1][2] = 0 - matrix[1][2];
		val = ~matrix[1][2]+1;
		val = val & 0x7FF;
		register12_11 |= (1 << 23)|(val)<< 12;
	}
	else
		register12_11 |= (matrix[1][2])<< 12;

//	printf("register12_11 = 0x%x\n",register12_11);


	register21_20 = 0;
	if(matrix[2][0]< 0)
	{
		matrix[2][0] = 0 - matrix[2][0];
		val = ~matrix[2][0]+1;
		val = val & 0x7FF;
		register21_20 |= (1 << 11)|(val)<< 0;
	}
	else
		register21_20 |= (matrix[2][0])<< 0;

	if(matrix[2][1]< 0)
	{
		matrix[2][1] = 0 - matrix[2][1];
		val = ~matrix[2][1] +1;
		val = val & 0x7FF;
		register21_20 |= (1 << 23)|(val)<< 12;
	}
	else
		register21_20 |= (matrix[2][1])<< 12;

//	printf("register21_20 = 0x%x\n",register21_20);

	register30_22 = 0;
	if(matrix[2][2]< 0)
	{
		matrix[2][2] = 0 - matrix[2][2];
		val = ~matrix[2][2]+1;
		val = val & 0x7FF;
		register30_22 |= (1 << 11)|(val)<< 0;
	}
	else
		register30_22 |= (matrix[2][2])<< 0;

	if(matrix[0][3]< 0)
	{
		matrix[0][3] = 0 - matrix[0][3];
		val = ~ matrix[0][3]+1;
		val = val & 0x7FF;
		register30_22 |= (1 << 23)|(val)<< 12;
	}
	else
		register30_22 |= (matrix[0][3])<< 12;

//	printf("register30_22 = 0x%x\n",register30_22);


	register32_31 = 0;
	if(matrix[1][3]< 0)
	{
		matrix[1][3] = 0 - matrix[1][3];
		val = ~ matrix[1][3]+1;
		val = val & 0x7FF;
		register32_31 |= (1 << 11)|(val)<< 0;
	}
	else
		register32_31 |= (matrix[1][3])<< 0;

	if(matrix[2][3]< 0)
	{
		matrix[2][3] = 0 - matrix[2][3];
		val = ~matrix[2][3]+1;
		val = val & 0x7FF;
		register32_31 |= (1 << 23)|(val)<< 12;
	}
	else
		register32_31 |= (matrix[2][3])<< 12;
//	printf("register32_31 = 0x%x\n",register32_31);

	vp_info.color.brightness = vp->brightness;
	vp_info.color.contrast = vp->contrast;
	vp_info.color.hue = vp->hue;
	vp_info.color.saturation = vp->saturation;
	vp_info.reg[0] = register01_00;
	vp_info.reg[1] = register10_02;
	vp_info.reg[2] = register12_11;
	vp_info.reg[3] = register21_20;
	vp_info.reg[4] = register30_22;
	vp_info.reg[5] = register32_31;

	ret = ioctl(fb_fd, ARKFB_SET_VP_INFO, &vp_info);
	if (ret != SUCCESS) {
		printf("%s: ioctl error.\n",__func__);
		return -1;
	}
	return 0;
}

#if 0
/* arkapi_display_scalar_and_set_yuv_layer_addr:
 * user:
 *	only used for arkn141
 *  parameters:
 *  	pyrgbaddr: y phy address
 *	pcbaddr	: u phy address
 *	pcraddr	: v phy address
 *	format	: yuv420 use ARK_SCALE_FORMAT_Y_UV420
 *	width	: source width
 *	height	: source height
 *	drop		: if drop current frame when there is no frame vsync interrupt.
 *				set 0 when used for non-real time process (such as play video file, ...),it will wait for vsync interrupt(do not drop current frame).
 *				set 1 when used for real-time process(such as camera, carplay projection screen, ...),it will drop current frame .
 */
int arkapi_display_scalar_and_set_yuv_layer_addr(disp_handle *handle,
							u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr, int format,
							u32 width, u32 height, u32 drop)
{
	//static int scalar_buf_init = 0;
	static int frame_id = 0;
	int ret;
	#define SCALAR_BUF_COUNT	3

	if(disp_handle_check(handle) != SUCCESS)
		return -EINVAL;

	if(handle->platform.type == ARK_PLATFORM_ARKN141) {
		if(!pyrgbaddr){
			printf("%s: pyrgbaddr=0, error.\n", __func__);
			return -EINVAL;
		}

		if(format & (~0x303)) {
			printf("%s: Invalid format:%d\n", __func__, format);
			return -EINVAL;
		}

		if(handle->reqbuf.count) {
			ret = arkapi_display_get_vsync_status(handle);
			if(ret <= 0) {
				if(drop) {
					return -EINVAL;
				}
				int timeout = 3;
				while(timeout--) {
					if(arkapi_display_wait_for_vsync(handle) == 0)
						break;
				}
			}

			handle->addr.yaddr = handle->reqbuf.phy_addr[frame_id];
			if(pcbaddr == 0) {
				handle->addr.cbaddr = handle->addr.yaddr + width*height;
			} else {
				handle->addr.cbaddr = handle->addr.yaddr + pcbaddr - pyrgbaddr;
			}
			if(pcraddr == 0) {
				if(format == ARK_SCALE_FORMAT_Y_UV420)
					handle->addr.craddr = 0;
				else if(format == ARK_SCALE_FORMAT_Y_U_V420)
					handle->addr.craddr = handle->addr.yaddr + width*height*5/4;
				else {
					//wait for update
					handle->addr.craddr = 0;
				}
			} else {
				handle->addr.craddr = handle->addr.yaddr + pcraddr - pyrgbaddr;
			}

			arkapi_n141_scalar_lock();
			ret = arkapi_n141_scalar(pyrgbaddr, pcbaddr, pcraddr,
				0, 0,
				width, height,
				width, height,
				width, height, width,
				format,
				handle->addr.yaddr,
				handle->addr.cbaddr,
				handle->addr.craddr);
			arkapi_n141_scalar_unlock();
			if(ret) {
				return ret;
			}

			ret = ioctl(handle->fd, ARKFB_SET_WINDOW_ADDR, &handle->addr);
			if (ret != SUCCESS) {
				printf("%s: ioctl error.\n",__func__);
				return ret;
			}
			if(++frame_id >= SCALAR_BUF_COUNT) {
				frame_id = 0;
			}
		} else {
			printf("%s: handle->reqbuf.count = 0.\n",__func__);
		}
	} else if(handle->platform.type == ARK_PLATFORM_ARK1668) {
		arkapi_display_set_layer_addr(handle, pyrgbaddr, pcbaddr, pcraddr);
	} else if(handle->platform.type == ARK_PLATFORM_ARK1668E) {
		arkapi_display_set_layer_addr(handle, pyrgbaddr, pcbaddr, pcraddr);
	}
	return SUCCESS;
}
#endif

