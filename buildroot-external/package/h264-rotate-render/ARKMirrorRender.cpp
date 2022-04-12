#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>


#include <iostream>
using namespace std; 

#include "ARKMirrorRender.h"

#if (ARK_COMPILE_MODE == COMPILE_USE_ARK_VIDEO_API)
#include "ark_api.h"
//#include "memalloc.h"

//#define MEMALLOC_MODULE_PATH	"/dev/memalloc"

#define MAX_STREAM_BUFFER_SIZE	(1024*1024)

#define RAW_STRM_TYPE_H264		104

ARKMirrorRender::ARKMirrorRender()
{

	cur_direction = ECMirrorRender::VERTICAL;
}

ARKMirrorRender::~ARKMirrorRender()
{
}

void ARKMirrorRender::initialize()
{
	video_cfg cfg;

	printf("run mode==>USE_ARK_VIDEO_API.\n");
	//hide ui layer to display video image
	arkapi_display_force_hide_layer(PRIMARY_LAYER);
	arkapi_display_force_hide_layer(OVER_LAYER);

	handle_vid = arkapi_video_init(RAW_STRM_TYPE_H264);

	//arkapi_video_get_config(handle_vid, &cfg);
	cfg.disp_x     = 0;
	cfg.disp_y     = 0;
	cfg.disp_width = 1024;
	cfg.disp_height= 600;
	cfg.direction  = ECMirrorRender::VERTICAL;
	arkapi_video_set_config(handle_vid, &cfg);
	arkapi_video_show(handle_vid, 1);

}

void ARKMirrorRender::play(const void *data, int len, MirrorDirection direction)
{
	video_cfg cfg;

	if(cur_direction != direction){
		arkapi_video_get_config(handle_vid, &cfg);
		cfg.direction = direction;
		cur_direction = direction;
		arkapi_video_set_config(handle_vid, &cfg);
	}

	arkapi_video_play(handle_vid, data, len);

}

void ARKMirrorRender::show()
{
	arkapi_video_show(handle_vid, 1);
}

void ARKMirrorRender::hide()
{
	arkapi_video_show(handle_vid, 0);
}

void ARKMirrorRender::release()
{
	arkapi_video_release(handle_vid);

	//show ui layer to display video image
	arkapi_display_force_show_layer(PRIMARY_LAYER);
	arkapi_display_force_show_layer(OVER_LAYER);
}

#elif (ARK_COMPILE_MODE == COMPILE_MFC_STORE_FILE_NO_ARK_API)

#define MEMALLOC_MODULE_PATH	"/tmp/dev/memalloc"

#define MAX_STREAM_BUFFER_SIZE	(1024*1024)

#define RAW_STRM_TYPE_H264		104

ARKMirrorRender::ARKMirrorRender()
{
	initialized = false;
	fd_memalloc = -1;
	//fd_fb = -1;
	last_display_addr = 0;
	first_show = 1;
	dec_first_frame = 1;
	rotate_buffer_index = 0;
	cur_direction = ECMirrorRender::VERTICAL;
	in_buffer.virtualAddress = (u32*)MAP_FAILED;
	in_buffer.busAddress = 0;
	for (int i = 0; i < 3; i++) {
		rotate_buffer[i].virtualAddress = (u32*)MAP_FAILED;
		rotate_buffer[i].busAddress = 0;
	}
}

ARKMirrorRender::~ARKMirrorRender()
{
}

void ARKMirrorRender::initialize()
{
	if (initialized) return;

	printf("run mode==>MFC_STORE_FILE_NO_ARK_API.\n");
	//initalize buffer
	fd_memalloc = open(MEMALLOC_MODULE_PATH, O_RDWR | O_SYNC);
	if (fd_memalloc < 0) {
		printf("open memalloc device fail.\n");
		return;	
	}   

	if (alloc_buffer() < 0)
		return;

	handle_mfc = mfc_init(RAW_STRM_TYPE_H264);

	initialized = true;
}

void ARKMirrorRender::play(const void *data, int len, MirrorDirection direction)
{
	int count = 3;
	unsigned int addr = 0;
	memcpy(in_buffer.virtualAddress, data, len);
	in_buffer.size = len;
	out_buffer.num = 0;
	mfc_decode(handle_mfc,&in_buffer, &out_buffer);
	if (dec_first_frame) 
		dec_first_frame = 0;

	if (direction != cur_direction) {
		first_show = 1;
		cur_direction = direction;
	}

	for (int i = 0; i < out_buffer.num; i++) {
		mMirrorW = out_buffer.frameWidth;
		mMirrorH = out_buffer.frameHeight;
		//存储数据文件需要传入虚拟地址
		data_process((unsigned int)out_buffer.buffer[i].pyVirAddress, (unsigned int)rotate_buffer[rotate_buffer_index].virtualAddress, cur_direction);
		render(rotate_buffer[rotate_buffer_index].busAddress);
		rotate_buffer_index = (rotate_buffer_index + 1) % 3;
	}
}

void ARKMirrorRender::show()
{
	return;
}

void ARKMirrorRender::hide()
{
	return;
}

void ARKMirrorRender::release()
{
	hide();
	free_buffer();

	if (fd_memalloc > 0) {
		close(fd_memalloc);
		fd_memalloc = -1;
	}

	mfc_uninit(handle_mfc);
}

int ARKMirrorRender::alloc_buffer(void)
{
	u32 pgsize = getpagesize();
	MemallocParams params;
	DWLLinearMem_t *info = &in_buffer;
	int fd_mem;

	fd_mem = open("/dev/mem", O_RDWR | O_SYNC);

	if (fd_mem == -1) {
		printf("Failed to open: %s\n", "/dev/mem");
		return -1;
	}

	/* get memory linear memory buffers */
	params.size = info->size = MAX_STREAM_BUFFER_SIZE;
	ioctl(fd_memalloc, MEMALLOC_IOCXGETBUFFER, &params);
	if (params.busAddress == 0) {
		printf("Memalloc: get buffer failure\n");
		close(fd_mem);
		return -1;
	}

	info->busAddress = params.busAddress;

	/* Map the bus address to virtual address */
	info->virtualAddress = (u32 *) mmap(0, info->size, PROT_READ | PROT_WRITE,
	                                            MAP_SHARED, fd_mem, params.busAddress);
	if (info->virtualAddress == MAP_FAILED) {
		close(fd_mem);
		return -1;
	}

	for (int i = 0; i < 3; i++) {
		info = &rotate_buffer[i];

		/* get memory linear memory buffers */
		params.size = info->size = mScreenW * mScreenH * 2;
		ioctl(fd_memalloc, MEMALLOC_IOCXGETBUFFER, &params);
		if (params.busAddress == 0) {
			printf("Memalloc: get buffer failure\n");
			close(fd_mem);
			return -1;
		}

		info->busAddress = params.busAddress;

		/* Map the bus address to virtual address */
		info->virtualAddress = (u32 *) mmap(0, info->size, PROT_READ | PROT_WRITE,
	                                          MAP_SHARED, fd_mem, params.busAddress);
		if (info->virtualAddress == MAP_FAILED) {
			close(fd_mem);
			return -1;
		}       
	}

	close(fd_mem);

	return 0;
}

void ARKMirrorRender::free_buffer(void)
{
	if (in_buffer.busAddress)
		ioctl(fd_memalloc, MEMALLOC_IOCSFREEBUFFER, &in_buffer.busAddress);

	if (in_buffer.virtualAddress != MAP_FAILED)
		munmap(in_buffer.virtualAddress, MAX_STREAM_BUFFER_SIZE);	

	for (int i = 0; i < 3; i++) {
		if (rotate_buffer[i].busAddress)
			ioctl(fd_memalloc, MEMALLOC_IOCSFREEBUFFER, &rotate_buffer[i].busAddress);

		if (rotate_buffer[i].virtualAddress != MAP_FAILED)
			munmap(rotate_buffer[i].virtualAddress, mScreenW * mScreenH * 2);
	}
}
    
void ARKMirrorRender::data_process(unsigned int src_addr, unsigned int dst_addr, MirrorDirection direction)
{
	static int index = 0;
	char file_name[32];
	int fd, size, ret;

	size = (mMirrorW*mMirrorH*3)/2;

	if (++index % 50 == 0){
		snprintf(file_name, 20, "mfc_raw%d.yuv", index);

		printf("+++save data-->%s, mMirrorW=%d,mMirrorH=%d,size=%d.\n", file_name, mMirrorW, mMirrorH,size);

		fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC);
		if (fd < 0) {
			printf("store raw,open %s fail.\n",file_name);
			return;
		} 

		ret = write(fd, (void *)src_addr, size);
		printf("write ret=%d.\n",ret);
		close(fd);
		system("sync");

		printf("---save data.\n");
	}

	return;
}

void ARKMirrorRender::render(unsigned int addr)
{
	return;
}

#else

#define MEMALLOC_MODULE_PATH	"/tmp/dev/memalloc"

#define MAX_STREAM_BUFFER_SIZE	(1024*1024)

#define RAW_STRM_TYPE_H264		104

extern int ark_test_mode;

ARKMirrorRender::ARKMirrorRender()
{
    initialized = false;
	fd_memalloc = -1;
	//fd_fb = -1;
	last_display_addr = 0;
	first_show = 1;
	dec_first_frame = 1;
	rotate_buffer_index = 0;
	cur_direction = ECMirrorRender::VERTICAL;
	in_buffer.virtualAddress = (u32*)MAP_FAILED;
	in_buffer.busAddress = 0;
	for (int i = 0; i < 3; i++) {
		rotate_buffer[i].virtualAddress = (u32*)MAP_FAILED;
		rotate_buffer[i].busAddress = 0;
	}
}

ARKMirrorRender::~ARKMirrorRender()
{
}

void ARKMirrorRender::initialize()
{
	enum ark_disp_layer layer = VIDEO_LAYER;

	if (initialized) return;

	//hide ui layer to display video image
	arkapi_display_force_hide_layer(PRIMARY_LAYER);
	arkapi_display_force_hide_layer(VIDEO_LAYER);
	arkapi_display_force_hide_layer(OVER_LAYER);

	printf("ark_test_mode=%d.\n", ark_test_mode);
	//initialize display
	if(ark_test_mode == USE_ARK_VIDEO_API){
		video_cfg cfg;

		printf("run mode==>USE_ARK_VIDEO_API.\n");
		handle_vid = arkapi_video_init(RAW_STRM_TYPE_H264);

		//arkapi_video_get_config(handle_vid, &cfg);
		cfg.disp_x	   = 0;
		cfg.disp_y	   = 0;
		cfg.disp_width = 1024;
		cfg.disp_height= 600;
		cfg.direction  = ECMirrorRender::VERTICAL;
		arkapi_video_set_config(handle_vid, &cfg);
		arkapi_video_show(handle_vid, 1);

		return;
	}
	else if(ark_test_mode == ARKN141_MFC_DISPLAY_API){
		printf("run mode==>ARKN141_MFC_DISPLAY_API.\n");
		layer = PRIMARY_LAYER;
	}
	else if(ark_test_mode == ARK1668_MFC_DISPLAY_API){
		printf("run mode==>ARK1668_MFC_DISPLAY_API.\n");
		layer = AUX_LAYER;
	}
	else if(ark_test_mode == USE_ARK_DISPLAY_2D_API){
		ark2d_cfg cfg_2d;

		printf("run mode==>USE_ARK_DISPLAY_2D_API.\n");
		handle_2d = arkapi_2d_init();

		cfg_2d.win_src_x = 0;
		cfg_2d.win_src_y = 0;
		cfg_2d.src_width = cfg_2d.win_src_width = mMirrorW;
		cfg_2d.src_height= cfg_2d.win_src_height= mMirrorH;
		cfg_2d.win_dst_x = 0;
		cfg_2d.win_dst_y = 0;
		cfg_2d.dst_width = cfg_2d.win_dst_width = mScreenW;
		cfg_2d.dst_height= cfg_2d.win_dst_height= mScreenH;
		cfg_2d.src_format = ARK_gcvSURF_NV12;
		cfg_2d.dst_format = ARK_gcvSURF_R5G6B5;
		cfg_2d.direction  = VERTICAL;

		arkapi_2d_set_config(handle_2d, &cfg_2d);

		layer = VIDEO_LAYER;
	}
	else if(ark_test_mode == USE_ARK_RAW2D_DISPLAY_API){
		//initialize 2D
		/* Construct the gcoOS object. */
		gceSTATUS status;

		status = gcoOS_Construct(gcvNULL, &g_os);
		if (status < 0)
		{
			printf("*ERROR* Failed to construct OS object (status = %d)\n", status);
			return;
		}

		/* Construct the gcoHAL object. */
		status = gcoHAL_Construct(gcvNULL, g_os, &g_hal);
		if (status < 0)
		{
			printf("*ERROR* Failed to construct GAL object (status = %d)\n", status);
			return;
		}	

		status = gcoHAL_Get2DEngine(g_hal, &g_engine2d);
		if (status < 0)
		{
			printf("*ERROR* Failed to get 2D engine object (status = %d)\n", status);
			return;
		}	

		printf("run mode==>USE_ARK_RAW2D_DISPLAY_API.\n");
		layer = VIDEO_LAYER;

	}
	else{
		printf("run mode==>error, exit.\n");
		return;
	}

		//initialize display
	handle_display= arkapi_display_open_layer(layer);
	if (!handle_display) {
		printf("open fb fail.\n");
		return;	
	}

	//initalize buffer
	handle_mem = arkapi_memalloc_init();
	if (!handle_mem) {
		return;
	}  

	if (alloc_buffer() < 0)
		return;

	handle_mfc = mfc_init(RAW_STRM_TYPE_H264);

	initialized = true;

}

void ARKMirrorRender::play(const void *data, int len, MirrorDirection direction)
{
	int count = 3;
	unsigned int addr = 0;

	if(ark_test_mode == USE_ARK_VIDEO_API){
		video_cfg cfg;

		if(cur_direction != direction){
			arkapi_video_get_config(handle_vid, &cfg);
			cfg.direction = direction;
			cur_direction = direction;
			arkapi_video_set_config(handle_vid, &cfg);
		}

		arkapi_video_play(handle_vid, data, len);
		return;
	}
	else if(ark_test_mode == ARKN141_MFC_DISPLAY_API){
		memcpy(in_buffer.virtualAddress, data, len);
		//usleep(25000);
	}else{
		memcpy(in_buffer.virtualAddress, data, len);
		arkapi_display_get_layer_addr(handle_display, &addr, NULL, NULL);
		while (last_display_addr && addr == rotate_buffer[rotate_buffer_index].busAddress && count--) {
			arkapi_display_wait_for_vsync(handle_display);
			arkapi_display_get_layer_addr(handle_display, &addr, NULL, NULL);
		}
	}
	
	in_buffer.size = len;
	out_buffer.num = 0;
	mfc_decode(handle_mfc,&in_buffer, &out_buffer);
	if (dec_first_frame) 
		dec_first_frame = 0;

	if (direction != cur_direction) {
		first_show = 1;
		cur_direction = direction;
	}

	for (int i = 0; i < out_buffer.num; i++) {
		if(ark_test_mode == USE_ARK_RAW2D_DISPLAY_API){
			data_process(out_buffer.buffer[i].yBusAddress, rotate_buffer[rotate_buffer_index].busAddress, cur_direction);
			render(rotate_buffer[rotate_buffer_index].busAddress);
		}
		else if(ark_test_mode == USE_ARK_DISPLAY_2D_API){
			ark2d_cfg cfg_2d;
			arkapi_2d_get_config(handle_2d, &cfg_2d);
			cfg_2d.src_width = cfg_2d.win_src_width = out_buffer.frameWidth;
			cfg_2d.src_height= cfg_2d.win_src_height= out_buffer.frameHeight;
			cfg_2d.direction = cur_direction;
			arkapi_2d_set_config(handle_2d, &cfg_2d);

			arkapi_2d_process(handle_2d, out_buffer.buffer[i].yBusAddress, rotate_buffer[rotate_buffer_index].busAddress);
			out_width = handle_2d->out_width;
			out_height= handle_2d->out_height;
			render(rotate_buffer[rotate_buffer_index].busAddress);
		}
		else{
			out_width  = out_buffer.frameWidth; 
			out_height = out_buffer.frameHeight; 
			render(out_buffer.buffer[i].yBusAddress);
		}
		rotate_buffer_index = (rotate_buffer_index + 1) % 3;
	}

}

void ARKMirrorRender::show()
{
	if(ark_test_mode == USE_ARK_VIDEO_API){
		arkapi_video_show(handle_vid, 1);
	}else{
		arkapi_display_show_layer(handle_display);
	}
}

void ARKMirrorRender::hide()
{	
	if(ark_test_mode == USE_ARK_VIDEO_API){
		arkapi_video_show(handle_vid, 0);
	}else{
		arkapi_display_hide_layer(handle_display);
	}
}

void ARKMirrorRender::release()
{
	if(ark_test_mode == USE_ARK_VIDEO_API){
		arkapi_video_release(handle_vid);

		//show ui layer to display video image
		arkapi_display_force_show_layer(PRIMARY_LAYER);
		arkapi_display_force_show_layer(OVER_LAYER);
		return;
	}
	else if(ark_test_mode == USE_ARK_RAW2D_DISPLAY_API){
		if (g_hal != gcvNULL) {
			gcoHAL_Commit(g_hal, gcvTRUE);
			gcoHAL_Destroy(g_hal);
		}

		if (g_os != gcvNULL) {
			gcoOS_Destroy(g_os);
		}
	}else if(ark_test_mode == USE_ARK_DISPLAY_2D_API){
		arkapi_2d_release(handle_2d);
	}

	hide();
	free_buffer();

	arkapi_display_close_layer(handle_display);
	mfc_uninit(handle_mfc);

	//show ui layer to display video image
	arkapi_display_force_show_layer(PRIMARY_LAYER);
	arkapi_display_force_show_layer(VIDEO_LAYER);
	arkapi_display_force_show_layer(OVER_LAYER);
	
}

int ARKMirrorRender::alloc_buffer(void)
{
	reqbuf_info reqbuf;
	int ret, i;

	if(!handle_mem){   
		printf("%s: handle null, error.\n", __func__);
		return -EINVAL;
	}

	reqbuf.size  = MAX_STREAM_BUFFER_SIZE;
	reqbuf.count = 1;
	ret = arkapi_memalloc_reqbuf(handle_mem, &reqbuf);
	if (ret < 0) {
		printf("memalloc: get buffer failure 1.\n");
		return -1;
	}
	in_buffer.size           = reqbuf.size;
	in_buffer.busAddress     = reqbuf.phy_addr[0];
	in_buffer.virtualAddress = (unsigned int *)reqbuf.virt_addr[0];

	reqbuf.count = ROTATE_BUF_MAX;
	reqbuf.size  = mScreenW * mScreenH * 2;
	ret = arkapi_memalloc_reqbuf(handle_mem, &reqbuf);
	if (ret < 0) {
		printf("memalloc: get buffer failure.\n");
		return -1;
	}
	for(i = 0; i < reqbuf.count; i++){
		rotate_buffer[i].size           = reqbuf.size;
		rotate_buffer[i].busAddress     = reqbuf.phy_addr[i];
		rotate_buffer[i].virtualAddress = (unsigned int *)reqbuf.virt_addr[i];
	}

	printf("%s: <---success.\n", __func__);

	return 0;
}

void ARKMirrorRender::free_buffer(void)
{
	if (handle_mem)
		arkapi_memalloc_release(handle_mem);
}
    
void ARKMirrorRender::data_process(unsigned int src_addr, unsigned int dst_addr, MirrorDirection direction)
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
	int width = mMirrorW, height = mMirrorH;
	    
	srcWidth = (mMirrorW + 15) & ~15;
	srcHeight = (mMirrorH + 15) & ~15;
    
	if (direction == LANDSCAPE) {
		rotation = gcvSURF_270_DEGREE; 
		width = mMirrorH;
		height = mMirrorW;
	}

	float hcoff, vcoff;
	hcoff = (float) width / mScreenW;
	vcoff = (float) height / mScreenH;
	if (hcoff > vcoff) {
		out_width = mScreenW;
		out_height = (height * mScreenW / width) & ~1;
	} else {
		out_height = mScreenH;
		out_width = (width * mScreenH / height) & ~1;
	}

	dstWidth = out_width;
	dstHeight = out_height;

	if (dstFormat == gcvSURF_A8R8G8B8 || dstFormat == gcvSURF_A8B8G8R8)
		dstStride[0] = dstWidth * 4;
	else if (dstFormat == gcvSURF_B5G6R5 || dstFormat == gcvSURF_R5G6B5)
		dstStride[0] = dstWidth * 2;

	srcAddress[0] = src_addr;
	dstAddress[0] = dst_addr;
	
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
	srcRect.top = 0;
	srcRect.right = mMirrorW;
	srcRect.bottom = mMirrorH;

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
	gcmONERROR(gco2D_SetClipping(g_engine2d, &srcRect));

	// set kernel size
	gcmONERROR(gco2D_EnableUserFilterPasses(g_engine2d, gcvTRUE, gcvTRUE));
	gcmONERROR(gco2D_SetKernelSize(g_engine2d, horKernel, verKernel));

	gcmONERROR(gco2D_FilterBlitEx2(g_engine2d,
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

	                               gcmONERROR(gco2D_Flush(g_engine2d));

	                               gcmONERROR(gcoHAL_Commit(g_hal, gcvTRUE));

	return;

OnError:
	printf("2d func failure.\n");
	return;
}

void ARKMirrorRender::render(unsigned int addr)
{
	int offx, offy, width, height, format, ycbaddr, ycraddr;

	if(ark_test_mode == USE_ARK_RAW2D_DISPLAY_API || ark_test_mode == USE_ARK_DISPLAY_2D_API){
		offx   = (mScreenW - out_width) / 2;
		offy   = (mScreenH - out_height) / 2;
		width  = out_width;
		height = out_height;
		format = ARK_LCDC_FORMAT_R5G6B5;
		ycbaddr= 0;
		ycraddr= 0;
	}else{

		width = out_width;
		if(mScreenH < out_height) 
			height = mScreenH;
		else 
			height = out_height; 

		offx   = (mScreenW - width) / 2;
		offy   = (mScreenH - height) / 2;
		format = ARK_LCDC_FORMAT_Y_UV420;
		ycbaddr= addr + out_width*out_height;
		ycraddr= 0;
	}

	if (first_show) {
		arkapi_display_set_layer_pos_atomic(handle_display, offx, offy);
		arkapi_display_set_layer_size_atomic(handle_display, width, height);
		arkapi_display_set_layer_format_atomic(handle_display, format);
		arkapi_display_set_layer_addr_atomic(handle_display, addr, ycbaddr, ycraddr);
		arkapi_display_layer_update_commit(handle_display);
		last_display_addr = addr;
		usleep(20000);
		arkapi_display_show_layer(handle_display);
		first_show = 0;
	} else {
		arkapi_display_set_layer_addr(handle_display, addr, ycbaddr, ycraddr);
		last_display_addr = addr;
	}
}
#endif

