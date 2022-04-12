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
#include "ark_common.h"

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668

#include "ark_2d.h"

static struct ark_2d_head g_2d_head = {0};

void ark2d_handle_default(ark2d_handle *handle)
{
	unsigned int width = SCREEN_WIDTH;
	unsigned int height = SCREEN_HEIGHT;
	screen_info screen;
	int ret; 

	if(handle == NULL){
		printf("%s: handle null.\n", __func__);
		return;
	}

	memset(handle, 0 ,sizeof(ark2d_handle));
	ret = arkapi_display_get_screen_info(&screen);
	if(ret == SUCCESS){
		width  = screen.width;
		height = screen.height;
	}

	handle->cfg_2d.src_width  = handle->cfg_2d.win_src_width  = width;
	handle->cfg_2d.src_height = handle->cfg_2d.win_src_height = height;
	handle->cfg_2d.dst_width  = handle->cfg_2d.win_dst_width  = width;
	handle->cfg_2d.dst_height = handle->cfg_2d.win_dst_height = height;
	handle->cfg_2d.src_format = gcvSURF_NV12;
	handle->cfg_2d.dst_format = gcvSURF_R5G6B5;
	handle->cfg_2d.direction  = VERTICAL;	/* no rotation */
        
}

ark2d_handle *arkapi_2d_init(void)
{
	ark2d_handle *handle = NULL;
	struct ark_2d_head *p2d_head = &g_2d_head;
	gceSTATUS st;
	int ret;

	handle = (ark2d_handle *)malloc(sizeof(ark2d_handle));
	if(!handle){
		printf("%s: malloc handle error.\n", __func__);
		return NULL;
	}

	ark2d_handle_default(handle);
	if(p2d_head->init == 1) {
		p2d_head->user_cnt++;
		handle->head_2d = (void *)p2d_head;
		printf("ark 2d gc init success, user_cnt=%d.\n", p2d_head->user_cnt);
		return handle;
	}

	st = gcoOS_Construct(gcvNULL, &p2d_head->g_os);
	if (st < 0){
		printf("error! failed to construct OS object (status = %d)\n", st);
		return NULL;
	}

	/* Construct the gcoHAL object. */
	st = gcoHAL_Construct(gcvNULL, p2d_head->g_os, &p2d_head->g_hal);
	if (st < 0){
		printf("error! failed to construct GAL object (status = %d)\n", st);
		return NULL;
	}	

	st = gcoHAL_Get2DEngine(p2d_head->g_hal, &p2d_head->g_engine2d);
	if (st < 0){
		printf("error! failed to get 2D engine object (status = %d)\n", st);
		return NULL;
	}

	p2d_head->user_cnt = 1;
	INIT_LIST_HEAD(&(p2d_head->list));

	handle->head_2d = p2d_head; 
	list_add(&(handle->list), &(p2d_head->list));

	printf("%s: success, user_cnt=%d.\n", __func__, p2d_head->user_cnt);
	            
	return handle;
        
}

int arkapi_2d_set_config(ark2d_handle *handle, ark2d_cfg *cfg_2d)
{

	if(handle == NULL || !handle->head_2d || !cfg_2d){
		printf("%s: handle=0x%p, handle->head_2d=0x%p, cfg_2d=0x%p.\n", __func__, handle, handle->head_2d, cfg_2d);
		return -EINVAL;
	}

	memcpy(&handle->cfg_2d, cfg_2d, sizeof(ark2d_cfg));

	return SUCCESS;
}

int arkapi_2d_get_config(ark2d_handle *handle, ark2d_cfg *cfg_2d)
{

	if(handle == NULL || !handle->head_2d){
		printf("%s: handle=0x%p, handle->head_2d=0x%p, cfg_2d=0x%p.\n", __func__, handle, handle->head_2d, cfg_2d);
		return -EINVAL;
	}

	memcpy(cfg_2d, &handle->cfg_2d, sizeof(ark2d_cfg));

	return SUCCESS;
}


int  arkapi_2d_process(ark2d_handle *handle, unsigned int src_phyaddr, unsigned int dst_phyaddr)
{
	gceSTATUS status;
	gctUINT8 horKernel = 5, verKernel = 7;
	int srcWidth, srcHeight, dstWidth, dstHeight;
	gcoSURF srcSurf, dstSurf; 
	gctUINT32 srcAddress[3] = {0, 0, 0};
	gctUINT32 dstAddress[3] = {0, 0, 0};
	gctUINT32 srcStride[3] = {0, 0, 0};
	gctUINT32 dstStride[3] = {0, 0, 0};
	gcsRECT srcRect, dstRect;
	gceSURF_FORMAT srcFormat = gcvSURF_NV12, dstFormat = gcvSURF_R5G6B5;//gcvSURF_A8R8G8B8;
	gceSURF_ROTATION rotation = gcvSURF_0_DEGREE;
	int width, height;
	float hcoff, vcoff;
	struct ark_2d_head *p2d_head;

	if(handle == NULL || !handle->head_2d){
		printf("%s: handle=0x%p, handle->head_2d=0x%p.\n", __func__, handle, handle->head_2d);
		return -EINVAL;
	}

	if(src_phyaddr == 0 || dst_phyaddr == 0){
		printf("%s: src_phyaddr=0x%0x, dst_phyaddr=0x%0x, error.\n", __func__, src_phyaddr, dst_phyaddr);
		return -EINVAL;
	}
        
	p2d_head = (struct ark_2d_head *)handle->head_2d;

	if (handle->cfg_2d.win_src_width)
		width = handle->cfg_2d.win_src_width;
	else
		width  = handle->cfg_2d.src_width;

	if (handle->cfg_2d.win_src_height)
		height = handle->cfg_2d.win_src_height;
	else
		height = handle->cfg_2d.src_height;

	srcWidth  = (handle->cfg_2d.src_width + 15) & ~15;
	srcHeight = (handle->cfg_2d.src_height + 15) & ~15;
    
	if (handle->cfg_2d.direction == LANDSCAPE) {
		int tmp = width;
		rotation = gcvSURF_270_DEGREE;
		width = height;
		height = tmp;
	}

	if (handle->cfg_2d.keep_aspect_ratio) {
		hcoff = (float) width / handle->cfg_2d.dst_width;
		vcoff = (float) height / handle->cfg_2d.dst_height;
		if (hcoff > vcoff) {
			handle->out_width = handle->cfg_2d.dst_width;
			handle->out_height = (height * handle->cfg_2d.dst_width / width) & ~1;
		} else {
			handle->out_height = handle->cfg_2d.dst_height;
			handle->out_width = (width * handle->cfg_2d.dst_height / height) & ~1;
		}		
	} else {
		handle->out_width = handle->cfg_2d.dst_width;
		handle->out_height = handle->cfg_2d.dst_height;
	}

	dstWidth  = handle->out_width;
	dstHeight = handle->out_height;

	if (dstFormat == gcvSURF_A8R8G8B8 || dstFormat == gcvSURF_A8B8G8R8)
		dstStride[0] = dstWidth * 4;
	else if (dstFormat == gcvSURF_B5G6R5 || dstFormat == gcvSURF_R5G6B5)
		dstStride[0] = dstWidth * 2;

	srcAddress[0] = src_phyaddr;
	dstAddress[0] = dst_phyaddr;

	//not support gcvSURF_VYUY scaler
	if (srcFormat == gcvSURF_UYVY) {
		srcAddress[2] = srcAddress[1] = 0xFFFFFFFF;
		srcStride[2] = srcStride[1] = srcStride[0] = srcWidth * 2;
	} else if (srcFormat == gcvSURF_NV21 || srcFormat == gcvSURF_NV12) {
		srcAddress[2] = 0xFFFFFFFF;
		srcAddress[1] = srcAddress[0] + srcWidth*srcHeight;
		srcStride[2] = srcStride[1] = srcStride[0] = srcWidth;
	}

	srcRect.left = 0;
	srcRect.top  = 0;
	srcRect.right  = handle->cfg_2d.src_width;
	srcRect.bottom = handle->cfg_2d.src_height;

	dstRect.left = 0;
	dstRect.top = 0;
	if (rotation == gcvSURF_90_DEGREE || rotation == gcvSURF_270_DEGREE) {
		dstRect.right = dstHeight;
		dstRect.bottom = dstWidth;
	} else {
		dstRect.right = dstWidth;
		dstRect.bottom = dstHeight;
	}
	
	// set clippint rect
	gcmONERROR(gco2D_SetClipping(p2d_head->g_engine2d, &srcRect));

	// set kernel size
	gcmONERROR(gco2D_EnableUserFilterPasses(p2d_head->g_engine2d, gcvTRUE, gcvTRUE));
	gcmONERROR(gco2D_SetKernelSize(p2d_head->g_engine2d, horKernel, verKernel));

	gcmONERROR(gco2D_FilterBlitEx2(p2d_head->g_engine2d,
	                               srcAddress, 2,
	                               srcStride, 2,
	                               gcvLINEAR, srcFormat,
	                               gcvSURF_0_DEGREE, srcWidth,
	                               srcHeight, &srcRect,
	                               dstAddress, 1,
	                               dstStride, 1,
	                               gcvLINEAR, dstFormat,
	                               rotation, dstWidth,
	                               dstHeight,
	                               &dstRect, gcvNULL));
	
	gcmONERROR(gco2D_Flush(p2d_head->g_engine2d));

	gcmONERROR(gcoHAL_Commit(p2d_head->g_hal, gcvTRUE));


	//printf("%s: <---end.\n", __func__);

	return SUCCESS;

OnError:
	printf("2d func failure.\n");
	return -1;
}

void arkapi_2d_release(ark2d_handle *handle)
{
	struct ark_2d_head *p2d_head = &g_2d_head;
	struct list_head *pos;
	struct list_head *del_tmp;
	ark2d_handle *ark2d_tmp;

	if(!handle || !handle->head_2d){   
		printf("%s: handle=0x%p, handle->head_2d=0x%p.\n", __func__, handle, handle->head_2d);
		return;
	}
        
	if(p2d_head->user_cnt <= 0){   
		printf("%s: user_cnt=%d, error.\n", __func__, p2d_head->user_cnt);
		return;
	}

	list_for_each_safe(pos, del_tmp, &(p2d_head->list)){
		ark2d_tmp = list_entry(pos, ark2d_handle, list);
		if(ark2d_tmp == handle) {  
			list_del_init(pos);
			free(handle);
			p2d_head->user_cnt--;
			printf("%s: <---success, user_cnt=%d.\n", __func__, p2d_head->user_cnt);
			break;
		}
	} 
        
	if(p2d_head->user_cnt >=1)
		return;

	if (p2d_head->g_hal != gcvNULL){
		gcoHAL_Commit(p2d_head->g_hal, gcvTRUE);
		gcoHAL_Destroy(p2d_head->g_hal);
	}

	if (p2d_head->g_os != gcvNULL){
		gcoOS_Destroy(p2d_head->g_os);
	}

	p2d_head->init = 0;

	printf("%s: init=0.\n", __func__);

}

#endif
