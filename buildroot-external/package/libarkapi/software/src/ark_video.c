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
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

#include "ark_common.h"
#include "ark_video.h"
#include "mfcapi.h"

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
#include "ark_2d.h"
#endif

struct display_data *pdd = NULL;

static int get_shared_display_data(void)
{
	if (pdd == NULL) {
		int shmid;

		shmid = shmget(DISPLAY_DATA_ID, sizeof(struct display_data), IPC_CREAT | 0666);
		if (shmid == -1) {
			perror("shmget err");
			return errno;
		}
		struct display_data *p = NULL;
		p = shmat(shmid, NULL, 0);
		if (p == (void *)-1 ) {
			perror("shmat err");
			return errno;
		}

		if (p->init == 0) {
			if (sem_init(&p->sem, 1, 1) < 0) {
				printf("sem_init fail, err is %d-%s.\n", errno, strerror(errno));
				return errno;
			}
			p->kernel_used = 0;
			p->display_mode = DISP_NONE;
			p->active_handle = NULL;
			p->init = 1;
			arkapi_display_get_screen_info(&p->screen);
			ark_dbg("%s %s-%s.\n", __FUNCTION__, __DATE__, __TIME__);
		}

		pdd = p;
	}

	return 0;
}

static int display_lock(void)
{
	if (pdd) {
		if (sem_wait(&pdd->sem) < 0 ) {
			printf("sem_wait fail.\n");
			return -1;
		}
	}

	return 0;
}

static int display_unlock(void)
{
	if (pdd) {
		if (sem_post(&pdd->sem) < 0 ) {
			printf("sem_post fail.\n");
			return -1;
		}
	}

	return 0;
}

static void video_handle_default(video_handle *handle)
{
	int i;

	if(!handle){
		printf("%s: handle null, error.\n", __func__);
		return;
	}

	memset(handle, 0 ,sizeof(video_handle));
	handle->handle_disp        = NULL;
	handle->last_display_addr  = 0;
	handle->last_render_yaddr  = 0;
	handle->last_render_uaddr  = 0;
	handle->last_render_vaddr  = 0;
	handle->first_show         = 1;
	handle->dec_first_frame    = 1;
	handle->rotate_buffer_index= 0;
	handle->cur_direction      = VERTICAL;
	handle->in_buffer.virtualAddress = (unsigned int *)MAP_FAILED;
	handle->in_buffer.busAddress = 0;
	for (i = 0; i < ROTATE_BUF_MAX; i++) {
		handle->rotate_buffer[i].virtualAddress = (unsigned int *)MAP_FAILED;
		handle->rotate_buffer[i].busAddress     = 0;
	}

	handle->cfg_vid.win_src_x = 0;
	handle->cfg_vid.win_src_y = 0;
	handle->cfg_vid.win_src_width = 0;
	handle->cfg_vid.win_src_height = 0;

	handle->cfg_vid.disp_x = 0;
	handle->cfg_vid.disp_y = 0;
	handle->cfg_vid.disp_width  = pdd->screen.width;
	handle->cfg_vid.disp_height = pdd->screen.height;
	handle->cfg_vid.direction   = VERTICAL;
    handle->cfg_vid.keep_aspect_ratio = 0;
}

int video_alloc_buffer(video_handle *handle)
{
	reqbuf_info *reqbuf;
	int ret, i;

	if(!handle){
		printf("%s: handle null, error.\n", __func__);
		return -EINVAL;
	}

	reqbuf = arkapi_memalloc_reqbuf(handle->handle_mem, MAX_STREAM_BUFFER_SIZE, 1);
	if (!reqbuf) {
		printf("memalloc: get buffer failure 1.\n");
		return -1;
	}
	handle->in_buffer.size           = reqbuf->size;
	handle->in_buffer.busAddress     = reqbuf->phy_addr[0];
	handle->in_buffer.virtualAddress = (unsigned int *)reqbuf->virt_addr[0];

	reqbuf = arkapi_memalloc_reqbuf(handle->handle_mem,
								handle->cfg_vid.disp_width * handle->cfg_vid.disp_height * 2, ROTATE_BUF_MAX);
	if (!reqbuf) {
		printf("memalloc: get buffer failure.\n");
		return -1;
	}
	for(i = 0; i < reqbuf->count; i++){
		handle->rotate_buffer[i].size           = reqbuf->size;
		handle->rotate_buffer[i].busAddress     = reqbuf->phy_addr[i];
		handle->rotate_buffer[i].virtualAddress = (unsigned int *)reqbuf->virt_addr[i];
	}

	ark_dbg("%s: <---success.\n", __func__);

	return 0;
}

static void video_display_process(video_handle *handle, unsigned int yaddr, unsigned int uaddr, unsigned int vaddr)
{
	int off_x, off_y;
	int out_w, out_h;
	int winx = 0, winy = 0, winwidth = 0, winheight = 0;
	unsigned int dst_phyaddr, read_addr = 0;

	if(!handle || !handle->handle_disp){
		printf("%s: handle null, error.\n", __func__);
		return;
	}

	if (!handle->active) {
		printf("%s not active.\n", __func__);
		return;
	}

	display_lock();
	if (pdd->active_handle != handle) {
		ark_dbg("%s: active_handle=0x%p != handle=0x%p, exit.\n", __func__, pdd->active_handle, handle);
		display_unlock();
		return;
	}
	if (pdd->kernel_used) {
		ark_dbg("%s display used by kernel.\n", __func__);
		display_unlock();
		return;
	}
	display_unlock();

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	if (yaddr == 0)
		return;
	dst_phyaddr = handle->rotate_buffer[handle->rotate_buffer_index].busAddress;
	handle->rotate_buffer_index = (handle->rotate_buffer_index + 1) % ROTATE_BUF_MAX;
	arkapi_display_get_layer_addr(handle->handle_disp, &read_addr, NULL, NULL);
	if (read_addr == dst_phyaddr)
		arkapi_display_wait_for_vsync(handle->handle_disp);
	arkapi_2d_process(handle->handle_2d, yaddr, dst_phyaddr);
	handle->last_display_addr = dst_phyaddr;
	out_w = handle->handle_2d->out_width;
	out_h = handle->handle_2d->out_height;

	if(handle->cfg_vid.disp_x == 0 && handle->cfg_vid.disp_y == 0){
		off_x = (handle->cfg_vid.disp_width  - out_w) / 2;
		off_y = (handle->cfg_vid.disp_height - out_h) / 2;
	}else{
		off_x = handle->cfg_vid.disp_x;
		off_y = handle->cfg_vid.disp_y;
	}
#else
	off_x = handle->cfg_vid.disp_x;
	off_y = handle->cfg_vid.disp_y;
	out_w = handle->cfg_vid.disp_width;
	out_h = handle->cfg_vid.disp_height;
#endif

	if (handle->last_display_addr) {
		unsigned int display_addr = handle->last_display_addr;
		if (handle->first_show) {
			arkapi_display_set_layer_pos_atomic(handle->handle_disp, off_x, off_y);
			arkapi_display_set_layer_size_atomic(handle->handle_disp, out_w, out_h);
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
			arkapi_display_set_layer_format_atomic(handle->handle_disp, ARK_LCDC_FORMAT_R5G6B5);
			arkapi_display_set_layer_addr_atomic(handle->handle_disp, display_addr, 0, 0);
#else
			arkapi_display_set_layer_format_atomic(handle->handle_disp, ARK_LCDC_FORMAT_Y_UV420);
			arkapi_display_set_layer_addr_atomic(handle->handle_disp, display_addr,
				display_addr + out_w * out_h, 0);
#endif
			arkapi_display_layer_update_commit(handle->handle_disp);
			usleep(20000);
			arkapi_display_show_layer(handle->handle_disp);
			handle->first_show = 0;
		} else {
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
			arkapi_display_set_layer_addr(handle->handle_disp, display_addr, 0, 0);
#else
			arkapi_display_set_layer_addr(handle->handle_disp, display_addr,
				display_addr + out_w * out_h, 0);
#endif
		}
		handle->last_display_addr = 0;
	}

#if LIBARKAPI_PLATFORM != LIBARKAPI_ARK1668
	if (yaddr == 0)
		return;

	if(handle->cfg_vid.win_src_width && handle->cfg_vid.win_src_height){
		winx = handle->cfg_vid.win_src_x;
		winy = handle->cfg_vid.win_src_y;
		winwidth = handle->cfg_vid.win_src_width;
		winheight = handle->cfg_vid.win_src_height;
	}else{
		winwidth = handle->out_buffer.codedWidth;
		winheight = handle->out_buffer.codedHeight;
	}

	arkapi_scalar_wait_idle();
	dst_phyaddr = handle->rotate_buffer[handle->rotate_buffer_index].busAddress;
	handle->rotate_buffer_index = (handle->rotate_buffer_index + 1) % ROTATE_BUF_MAX;
	arkapi_display_get_layer_addr(handle->handle_disp, &read_addr, NULL, NULL);
	if (read_addr == dst_phyaddr)
		arkapi_display_wait_for_vsync(handle->handle_disp);
	arkapi_scalar_nowait(yaddr, uaddr, vaddr,
					ARK_SCALE_FORMAT_Y_UV420,
					handle->out_buffer.frameWidth, handle->out_buffer.frameHeight,
					winx, winy, winwidth, winheight,
					0, 0, 0, 0,
					handle->cfg_vid.disp_width, handle->cfg_vid.disp_height,
					dst_phyaddr, dst_phyaddr + out_w * out_h, 0,
					ARK_SCALE_OUT_FORMAT_Y_UV420,
					SCALE_ROTATE_0);
	handle->last_display_addr = dst_phyaddr;

#endif
	//ark_dbg("%s: <---end.\n", __func__);
}

void sig_handler(int sigio)
{
	video_handle *handle;

	display_lock();
	handle = pdd->active_handle;
	display_unlock();

	if (sigio == SIGUSR1) {
		ark_dbg("rev SIGUSR1\n");
		if (handle && handle->handle_disp)
			arkapi_display_hide_layer(handle->handle_disp);
	} else if (sigio == SIGUSR2) {
		ark_dbg("rev SIGUSR2\n");
		handle->first_show = 1;
		if (handle)
			video_display_process(handle, handle->last_render_yaddr, handle->last_render_uaddr,
				handle->last_render_vaddr);
#if LIBARKAPI_PLATFORM != LIBARKAPI_ARK1668
		video_display_process(handle, 0, 0, 0);
#endif
	}
}

video_handle *arkapi_video_init(int stream_type)
{
	memalloc_handle *handle_mem = NULL;
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	ark2d_handle *handle_2d = NULL;
#endif
	disp_handle *handle_disp= NULL;
	video_handle *handle = NULL;
	struct list_head *list;
	int ret;

	ark_dbg("%s: stream_type=%d.\n", __func__, stream_type);
	printf("arkapi_video_init -20220406\n");

	if(stream_type < RAW_STRM_TYPE_H264 || (stream_type > RAW_STRM_TYPE_MP4_CUSTOM && \
	                                       stream_type != RAW_STRM_TYPE_H264_NOREORDER)){
		printf("stream_type, error.\n");
		return NULL;
	}

	get_shared_display_data();

	handle = (video_handle *)malloc(sizeof(video_handle));
	if(!handle){
		printf("%s: malloc video handle error.\n", __func__);
		goto err;
	}
	memset(handle, 0, sizeof(video_handle));
	video_handle_default(handle);

	handle_disp = arkapi_display_open_layer(VIDEO_LAYER);
	if (!handle_disp) {
		printf("open VIDEO_LAYER fail.\n");
		goto err;
	}
	handle->handle_disp = handle_disp;

	handle_mem = arkapi_memalloc_init();
	if (!handle_mem) {
		goto err1;
	}
	handle->handle_mem = handle_mem;

	if(video_alloc_buffer(handle) < 0){
		goto err1;
	}

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	handle->handle_2d = arkapi_2d_init();
	if(!handle->handle_2d){
		printf("%s: 2d init fail.\n", __func__);
		goto err1;
	}
#endif

	handle->handle_mfc = mfc_init(stream_type);
	if(!handle->handle_mfc){
		printf("%s: mfc init fail.\n", __func__);
		goto err2;
	}

	if (sem_init(&handle->sem_lock, 1, 1) < 0) {
		printf("%s sem_init fail\n", __func__);
		goto err2;
	}

	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);

	ark_dbg("%s: success.\n", __func__);

	return handle;
err2:
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	arkapi_2d_release(handle->handle_2d);
#endif
err1:
	arkapi_memalloc_release(handle->handle_mem);
err:
	if(handle) free(handle);

	return NULL;
}

static int arkapi_video_lock(video_handle *handle)
{
	int ret = 0;

	ret = sem_wait(&handle->sem_lock);
	if (ret < 0 ) {
		printf("%s sem_wait fail, error=%d.\n", __func__, ret);
	}

	return ret;
}

static int arkapi_video_unlock(video_handle *handle)
{
	int ret = 0;

	ret = sem_post(&handle->sem_lock);
	if (ret < 0) {
		printf("%s sem_post fail, error=%d.\n", __func__, ret);
	}

	return ret;
}

int arkapi_video_set_config(video_handle *handle, video_cfg *cfg_vid)
{
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	ark2d_cfg cfg_2d;
#endif

	if(!handle || !cfg_vid){
		printf("%s: handle=0x%p, cfg_vid=0x%p, error.\n", __func__, handle, cfg_vid);
		return -EINVAL;
	}

	arkapi_video_lock(handle);

	memcpy(&handle->cfg_vid, cfg_vid, sizeof(video_cfg));
	if (handle->cfg_vid.direction != handle->cur_direction) {
		handle->first_show = 1;
		handle->cur_direction = handle->cfg_vid.direction;
	}

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	arkapi_2d_get_config(handle->handle_2d, &cfg_2d);
	cfg_2d.dst_width = cfg_2d.win_dst_width = cfg_vid->disp_width;
	cfg_2d.dst_height= cfg_2d.win_dst_height= cfg_vid->disp_height;
	cfg_2d.direction = cfg_vid->direction;
	cfg_2d.keep_aspect_ratio = cfg_vid->keep_aspect_ratio;
	arkapi_2d_set_config(handle->handle_2d, &cfg_2d);
#endif

	arkapi_video_unlock(handle);

	ark_dbg("%s: disp_x=%d, disp_y=%d, disp_width=%d, disp_height=%d, direction=%d.\n", \
	  __func__, cfg_vid->disp_x, cfg_vid->disp_y, cfg_vid->disp_width, cfg_vid->disp_height, cfg_vid->direction);

	return SUCCESS;
}

int arkapi_video_get_config(video_handle *handle, video_cfg *cfg_vid)
{

	if(!handle || !cfg_vid){
		printf("%s: handle=0x%p, cfg_vid=0x%p, error.\n", __func__, handle, cfg_vid);
		return -EINVAL;
	}

	arkapi_video_lock(handle);
	memcpy(cfg_vid, &handle->cfg_vid, sizeof(video_cfg));
	arkapi_video_unlock(handle);

	//ark_dbg("%s: disp_x=%d, disp_y=%d, disp_width=%d, disp_height=%d, direction=%d.\n", \
	//__func__, cfg_vid->disp_x, cfg_vid->disp_y, cfg_vid->disp_width, cfg_vid->disp_height, cfg_vid->direction);

	return SUCCESS;
}

int arkapi_video_set_display_mode(int mode)
{
	if (mode < DISP_NONE || mode >= DISP_MODE_NUM) {
		printf("Ivalid parameter mode(%d).\n", mode);
		return -1;
	}
	get_shared_display_data();
	display_lock();
	pdd->display_mode = mode;
	display_unlock();

	return 0;
}

int arkapi_video_show(video_handle *handle, int enable)
{
	int ret = 0;

	if(!handle || !handle->handle_disp){
		printf("%s: handle=0x%p, handle->0x%p, error.\n", __func__, handle, handle->handle_disp);
		return -EINVAL;
	}

	arkapi_video_lock(handle);

	if(enable){
		handle->active = 1;
		printf("%s: set video handle=0x%p active=1.\n", __func__, handle);
		if (!handle->first_show)
			ret = arkapi_display_show_layer(handle->handle_disp);
		display_lock();
		pdd->active_handle = handle;
		pdd->active_pid = getpid();
		display_unlock();
	}else{
		handle->active = 0;
		display_lock();
		if(pdd->active_handle == handle)
			pdd->active_handle = NULL;
		display_unlock();
		ret = arkapi_display_hide_layer(handle->handle_disp);
	}

	arkapi_video_unlock(handle);

	ark_dbg("%s: enable=%d, ret=%d.\n", __func__, enable, ret);

	return ret;
}

int arkapi_video_show_mode(video_handle *handle, int mode, int enable)
{
	int ret = 0;

	if(!handle || !handle->handle_disp){
		printf("%s: handle=0x%p, handle->0x%p, error.\n", __func__, handle, handle->handle_disp);
		return -EINVAL;
	}

	display_lock();
	if (pdd->display_mode != mode) {
		printf("%s: unmatch display mode %d:%d.\n", __func__, pdd->display_mode, mode);
		display_unlock();
		return -1;
	}
	display_unlock();

	arkapi_video_show(handle, enable);
}

void arkapi_video_release(video_handle *handle)
{
	struct list_head *pos;
	struct list_head *del_tmp;
	video_handle *vid_tmp;

	if(!handle){
		printf("%s: handle null, error.\n", __func__);
		return;
	}

	display_lock();
	if(pdd->active_handle == handle)
		pdd->active_handle = NULL;
	display_unlock();

	arkapi_video_lock(handle);

	arkapi_display_hide_layer(handle->handle_disp);
	if (handle->handle_disp)
		arkapi_display_close_layer(handle->handle_disp);

	if (handle->handle_mem)
		arkapi_memalloc_release(handle->handle_mem);

#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
	if (handle->handle_2d)
		arkapi_2d_release(handle->handle_2d);
#endif

	if (handle->handle_mfc)
		mfc_uninit(handle->handle_mfc);

	arkapi_video_unlock(handle);

	sem_destroy(&handle->sem_lock);
	free(handle);

	ark_dbg("%s: <---sucess.\n", __func__);

	return;
}

int arkapi_video_play(video_handle *handle, const void *src_addr, int len, int fps)
{
	static int mirror_w = 0, mirror_h = 0;
	int count = ROTATE_BUF_MAX;
	ark2d_cfg cfg_2d;
    int i, time_delay = 0;
    struct timeval start, end;
    unsigned long usetime = 0;
	int ret;

	if(!handle){
		printf("%s: handle null, error.\n", __func__);
		return -EINVAL;
	}
	if(!handle->handle_mfc || !handle->handle_mem
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
		|| !handle->handle_2d
#endif
	) {
		printf("%s: handle_mfc=0x%p, handle_2d=0x%p,handle_mfc=0x%p, error.\n",
		                __func__, handle->handle_mfc, handle->handle_2d, handle->handle_mem);
		return -EINVAL;
	}

	arkapi_video_lock(handle);

	memcpy(handle->in_buffer.virtualAddress, src_addr, len);
	//ark_dbg("%s: src_addr=0x%p, len=%d.\n", __func__, src_addr, len);

	if (len > MAX_STREAM_BUFFER_SIZE)
		len = MAX_STREAM_BUFFER_SIZE;
	handle->in_buffer.size = len;
	handle->out_buffer.num = 0;
	ret = mfc_decode(handle->handle_mfc, &handle->in_buffer, &handle->out_buffer);
	if (ret) {
		printf("mfc_decode fail, ret=%d.\n", ret);
		arkapi_video_unlock(handle);
		return ret;
	}
	if (handle->dec_first_frame)
		handle->dec_first_frame = 0;

	if(mirror_w != handle->out_buffer.frameWidth || mirror_h != handle->out_buffer.frameHeight){
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
		arkapi_2d_get_config(handle->handle_2d, &cfg_2d);
		cfg_2d.src_width = mirror_w = handle->out_buffer.frameWidth;
		cfg_2d.src_height = mirror_h = handle->out_buffer.frameHeight;
		cfg_2d.win_src_width = handle->out_buffer.codedWidth;
		cfg_2d.win_src_height = handle->out_buffer.codedHeight;
		arkapi_2d_set_config(handle->handle_2d, &cfg_2d);
#endif
	}

	if (fps)
		gettimeofday(&start, NULL);

	for (i = 0; i < handle->out_buffer.num; i++) {
		if (fps && i > 0) {
			gettimeofday(&end, NULL);
			usetime = ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec)/1000;
			time_delay = 1000 / fps * i - usetime;
			if (time_delay > 0)
				usleep(time_delay * 1000);
		}
		handle->last_render_yaddr = handle->out_buffer.buffer[i].yBusAddress;
		handle->last_render_uaddr = handle->last_render_yaddr + handle->out_buffer.frameWidth * handle->out_buffer.frameHeight;
		video_display_process(handle, handle->last_render_yaddr, handle->last_render_uaddr, 0);
	}

	arkapi_video_unlock(handle);
	//ark_dbg("%s: <---end.\n", __func__);
	return handle->out_buffer.num;
}

int arkapi_video_play_eof(video_handle *handle, int fps)
{
	unsigned int src_phyaddr;
    int time_delay;
    struct timeval start, end;
    unsigned long usetime = 0;

	if(!handle->handle_mfc || !handle->handle_mem
#if LIBARKAPI_PLATFORM == LIBARKAPI_ARK1668
		|| !handle->handle_2d
#endif
	) {
		printf("%s: handle_mfc=0x%p, handle_2d=0x%p,handle_mfc=0x%p, error.\n",
		                __func__, handle->handle_mfc, handle->handle_2d, handle->handle_mem);
		return -EINVAL;
	}

	arkapi_video_lock(handle);

#if LIBARKAPI_PLATFORM != LIBARKAPI_ARK1668
	video_display_process(handle, 0, 0, 0);
#endif

	while (mfc_decode_eof(handle->handle_mfc, &handle->out_buffer))
	{
		if (fps)
			gettimeofday(&start, NULL);
		handle->last_render_yaddr = handle->out_buffer.buffer[0].yBusAddress;
		handle->last_render_uaddr = handle->last_render_yaddr + handle->out_buffer.frameWidth * handle->out_buffer.frameHeight;
		video_display_process(handle, handle->last_render_yaddr, handle->last_render_uaddr, 0);
		if (fps) {
			gettimeofday(&end, NULL);
			usetime = ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec)/1000;
			time_delay = 1000 / fps - usetime;
			if (time_delay > 0)
				usleep(time_delay * 1000);
		}
	}

#if LIBARKAPI_PLATFORM != LIBARKAPI_ARK1668
	video_display_process(handle, 0, 0, 0);
#endif

	arkapi_video_unlock(handle);

	return 0;
}

int arkapi_enter_carback(void)
{
	get_shared_display_data();

	display_lock();
	if (pdd->active_handle == NULL) {
		pdd->kernel_used = 1;
		display_unlock();
		return 0;
	}

	if (pdd->active_pid > 0)
		kill(pdd->active_pid, SIGUSR1);
	pdd->kernel_used = 1;
	ark_dbg("%s enter.\n", __FUNCTION__);
	display_unlock();

	return 0;
}

int arkapi_exit_carback(void)
{
	get_shared_display_data();

	display_lock();
	pdd->kernel_used = 0;
	if (pdd->active_handle && pdd->active_pid > 0)
		kill(pdd->active_pid, SIGUSR2);
	display_unlock();

	return 0;
}

int arkapi_set_vin_start(int start)
{
	get_shared_display_data();
	display_lock();
	if (start){
		if (pdd->active_handle == NULL) {
			pdd->kernel_used = 1;
			display_unlock();
			return 0;
		}

		if (pdd->active_pid > 0)
			kill(pdd->active_pid, SIGUSR1);
		pdd->kernel_used = 1;
	}else{
		pdd->kernel_used = 0;
		if (pdd->active_handle && pdd->active_pid > 0)
			kill(pdd->active_pid, SIGUSR2);
	}
	display_unlock();
	return 0;
}
