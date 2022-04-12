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
#include <pthread.h>
#include "include/ark_api.h"

//#define DEBUG

#ifdef DEBUG
#define V4L2_DBG(...) printf(__VA_ARGS__)
#else
#define V4L2_DBG(...)
#endif

#define BUF_NUM 				4
#define DISPLAY 				1
#define GETDATA 				2
#define ARK_BUFFER_NUM			3

struct VideoDevice {
    int Vfd;
	int VpixelFormat;
    int VideoBufCurIndex;
	int VideoBufCnt;
	int VideoBufMaxLen;
	int src_width;
	int src_height;
	unsigned char *Cam_vbuf[BUF_NUM];           //video data virtual  address 
	unsigned char* Video_Data;					//video data physical address
};

struct display_info{
	disp_handle *display_handle;
	int lay_id;

	unsigned int disp_posx;
	unsigned int disp_posy;
	unsigned int disp_width;
	unsigned int disp_height;
};

enum work_mode {
	WORK_MODE_DISPLAY,
	WORK_MODE_GETDATA,
	WORK_MODE_END,
};

enum ark1668e_lcdc_layer {
	ARK1668E_LCDC_LAYER_VIDEO1,
	ARK1668E_LCDC_LAYER_VIDEO2,
	ARK1668E_LCDC_LAYER_OSD1,
	ARK1668E_LCDC_LAYER_OSD2,
	ARK1668E_LCDC_LAYER_OSD3,
	ARK1668E_LCDC_LAYER_MAX
};

enum dvr_source {
	DVR_SOURCE_CAMERA,
	DVR_SOURCE_AUX,
	DVR_SOURCE_DVD,
};


struct VideoDevice *g_ark_Vdev;
struct display_info *pinfo = NULL;
static pthread_t v4l2_work_id;
static reqbuf_info reqbuf;
int t_cmd;

/* This function first makes the query judgmentThe space is then allocated and queued */
static int V4l2InitDevice(struct VideoDevice *Vdec)
{
	struct v4l2_fmtdesc tFmtDesc;
	struct v4l2_requestbuffers tV4l2buf;
	struct v4l2_format  tV4l2Fmt;
	struct v4l2_buffer tV4l2Buf;
	struct v4l2_capability tV4l2Cap;
	struct v4l2_input tV4l2inp;
	int Fd,Ret,Error,i;
	
	Fd = open("/dev/video0",O_RDWR);
	if( Fd < 0 )
	{	
		V4L2_DBG("open error.\n");
		return -1;
	}
	Vdec->Vfd = Fd;
	
	memset(&tV4l2Cap, 0, sizeof(struct v4l2_capability));
    Error = ioctl(Fd, VIDIOC_QUERYCAP, &tV4l2Cap);
    if (Error) {
    	V4L2_DBG("unable to query device.\n");
    	return -1;
    }

	if (!(tV4l2Cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
    	V4L2_DBG("device is not a video capture device.\n");
		return -1;
    }

	tFmtDesc.index = 0;
	tFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(Fd,VIDIOC_ENUM_FMT,&tFmtDesc);
	Vdec->VpixelFormat = tFmtDesc.pixelformat;
	
	memset(&tV4l2Fmt, 0, sizeof(struct v4l2_format));
	tV4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2Fmt.fmt.pix.pixelformat = Vdec->VpixelFormat;
	tV4l2Fmt.fmt.pix.width       = Vdec->src_width;
	tV4l2Fmt.fmt.pix.height      = Vdec->src_height;
	tV4l2Fmt.fmt.pix.field       = V4L2_FIELD_ANY;
	Ret = ioctl(Fd, VIDIOC_S_FMT, &tV4l2Fmt); 
	if (Ret) 
		V4L2_DBG("Unable to set format\n");


	tV4l2inp.index = DVR_SOURCE_CAMERA;                  //set channel
	Ret = ioctl(Fd, VIDIOC_S_INPUT, &tV4l2inp.index); 
	if (Ret) 
		V4L2_DBG("Unable to set input\n");
	
	tV4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2buf.memory = 1;
	tV4l2buf.count = BUF_NUM;
	ioctl(Fd,VIDIOC_REQBUFS,&tV4l2buf);
	
	Vdec->VideoBufCnt = tV4l2buf.count;
	for(i =0; i<Vdec->VideoBufCnt; i++)
	{
		memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
		tV4l2Buf.index = i;
		tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tV4l2Buf.memory = V4L2_MEMORY_MMAP;
		ioctl(Fd,VIDIOC_QUERYBUF,&tV4l2Buf);
		Vdec->VideoBufMaxLen = tV4l2Buf.length;
		V4L2_DBG("Vdec->VideoBufMaxLen = %d \n",Vdec->VideoBufMaxLen);
		/* 0 is start anywhere */
		Vdec->Cam_vbuf[i] = mmap(0,tV4l2Buf.length, PROT_READ, MAP_SHARED,Fd,tV4l2Buf.m.offset);
		if (Vdec->Cam_vbuf[i] == MAP_FAILED) 
		{
			V4L2_DBG("Unable to map buffer\n");
		}
	}
	for(i =0; i<Vdec->VideoBufCnt; i++)
	{
		memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
    	tV4l2Buf.index = i;
    	tV4l2Buf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
    	Error = ioctl(Fd, VIDIOC_QBUF, &tV4l2Buf);
    	if (Error)
       	{
    	    V4L2_DBG("Unable to queue buffer.\n");
    	}
	}
	
    return 0;    
}

static int V4l2ExitDevice(struct VideoDevice *Vdec)
{
    int i;
	for (i = 0; i < Vdec->VideoBufCnt; i++)
 	{
    	if (Vdec->Cam_vbuf[i])
    	{
    		munmap(Vdec->Cam_vbuf[i], Vdec->VideoBufMaxLen);
    		Vdec->Cam_vbuf[i] = NULL;
    	}
    }
	close(Vdec->Vfd);
    return 0;
}
	
static int V4l2GetDataForStreaming(struct VideoDevice *Vdec)
{
	struct pollfd Fds[1];
	int Ret;
	struct v4l2_buffer tV4l2Buf;
	/*wait data*/
	Fds[0].fd     = Vdec->Vfd;
	Fds[0].events = POLLIN;				
	Ret = poll(Fds, 1, -1);
	if (Ret <= 0)
	{
		V4L2_DBG("poll error!\n");
		return -1;
	}
	
	memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer)); 
	tV4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
	Ret = ioctl(Vdec->Vfd, VIDIOC_DQBUF, &tV4l2Buf);
	if (Ret < 0) 
	{
		V4L2_DBG("Unable to dequeue buffer.\n");
		return -1;
	}
	Vdec->VideoBufCurIndex = tV4l2Buf.index;
	Vdec->VideoBufMaxLen = tV4l2Buf.length;
	V4L2_DBG("DQBUF-->app get buf index : %d, buf : %p\n",tV4l2Buf.index,tV4l2Buf.m.offset);
	/*get physics data*/
	Vdec->Video_Data = tV4l2Buf.m.offset;
	return 0;
}


static int V4l2PutDataForStreaming(struct VideoDevice *Vdec)
{
    struct v4l2_buffer tV4l2Buf;
    int Error;
	memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
	tV4l2Buf.index  = Vdec->VideoBufCurIndex;
	tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
	V4L2_DBG("QBUF -->app put buf index : %d\n",tV4l2Buf.index);
	/*Put the extracted buf in the queue*/
	Error = ioctl(Vdec->Vfd, VIDIOC_QBUF, &tV4l2Buf);
	if (Error) 
   	{
	    V4L2_DBG("Unable to queue buffer.\n");
	    return -1;
	}
    return 0;
}

static int V4l2StartDevice(struct VideoDevice *Vdec)
{
    int Type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int Error;

    Error = ioctl(Vdec->Vfd, VIDIOC_STREAMON, &Type);
    if (Error) 
    {
    	V4L2_DBG("Unable to start capture.\n");
    	return -1;
    }
    return 0;
}
    
static int V4l2StopDevice(struct VideoDevice *Vdec)
{
    int Type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int Error;

    Error = ioctl(Vdec->Vfd, VIDIOC_STREAMOFF, &Type);
    if (Error) 
    {
    	V4L2_DBG("Unable to stop capture.\n");
    	return -1;
    }
	
    return 0;
}

void display_layer_init(struct VideoDevice *Vdec)
{
	screen_info screen;
	int Format,screen_width, screen_height;;
	unsigned int addr, ret;
	disp_handle *display_handle;
	pinfo = (struct display_info *)malloc(sizeof(struct display_info));
	if (pinfo == NULL) {
		printf("no enough memory be alloc");
	}
    
	memset(pinfo, 0, sizeof(struct display_info));
	pinfo->disp_posx   = 600;
	pinfo->disp_posy   = 216;
	pinfo->disp_width  = g_ark_Vdev->src_width;
	pinfo->disp_height = g_ark_Vdev->src_height;
	pinfo->lay_id = ARK1668E_LCDC_LAYER_VIDEO2;
	Format = ARK_LCDC_FORMAT_Y_UV420 | ARK_LCDC_ORDER_VYUY;

	display_handle = arkapi_display_open_layer(pinfo->lay_id);
	if(!display_handle){
		printf("open fb%d error.",pinfo->lay_id);
		free(pinfo);
	}
	pinfo->display_handle = display_handle;
	arkapi_display_hide_layer(display_handle);

	arkapi_display_set_layer_pos_atomic(display_handle, pinfo->disp_posx, pinfo->disp_posy);
	arkapi_display_set_layer_size_atomic(display_handle, pinfo->disp_width, pinfo->disp_height);
	arkapi_display_set_layer_format_atomic(display_handle, Format);
	arkapi_display_layer_update_commit(display_handle);
}

void display_set_addr(u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr)
{
	V4L2_DBG("app write buf  : %p\n",g_ark_Vdev->Video_Data);
	struct ark_disp_addr addr;
	addr.yaddr = pyrgbaddr;
	addr.cbaddr = pcbaddr;
	addr.craddr = pcraddr;
	arkapi_display_set_layer_addr(pinfo->display_handle, addr.yaddr, addr.cbaddr, addr.craddr);
}

void display_show_layer(void)
{
	if(pinfo){
		arkapi_display_show_layer(pinfo->display_handle);
	}
}

void display_hide_layer(void)
{
	if(pinfo){
		arkapi_display_close_layer(pinfo->display_handle);
		free(pinfo);
	}
}

static void* display_work_thread(void *param)
{
	int fd_t,Ret,i;
	if(t_cmd == DISPLAY){
	    while(1){
			V4l2GetDataForStreaming(g_ark_Vdev);
			display_set_addr(g_ark_Vdev->Video_Data,g_ark_Vdev->Video_Data+pinfo->disp_width*pinfo->disp_height,0);
			V4l2PutDataForStreaming(g_ark_Vdev);
	   }
	}
	else if(t_cmd == GETDATA){
		fd_t = open("stream.yuv",O_RDWR|O_CREAT);
		if(fd_t < 0){
			V4L2_DBG("open error.\n");
			return 0;
		}
		while(1){
			V4l2GetDataForStreaming(g_ark_Vdev);
			Ret = write(fd_t,g_ark_Vdev->Cam_vbuf[g_ark_Vdev->VideoBufCurIndex],g_ark_Vdev->VideoBufMaxLen);
			V4l2PutDataForStreaming(g_ark_Vdev);
		}
	}
	else{
		V4L2_DBG("mode error t_cmd = %d.\n",t_cmd);
		return 0;
	}
}

void display_video_start(void)
{
	V4L2_DBG("video_start cmd = %d \n",t_cmd);
	pthread_create(&v4l2_work_id, NULL, display_work_thread,NULL);
}

int display_demo(void)
{
	int Ret;
	t_cmd = DISPLAY;

	display_layer_init(g_ark_Vdev);
	/* start device && open display layer*/
	Ret = V4l2StartDevice(g_ark_Vdev);
	if(Ret < 0){
		V4L2_DBG("start v4l2 device error.\n");
		return 0;
	}
	display_show_layer();
	display_video_start();
	while(1){
		usleep(100*1000);
	}
}

int getdata_demo(void)
{
	char flag_quit[10];
	int Ret;
	t_cmd = GETDATA;
	/* start device && open display layer*/
	Ret = V4l2StartDevice(g_ark_Vdev);
	if(Ret < 0){
		V4L2_DBG("start v4l2 device error.\n");
		return 0;
	}
	display_video_start();
	while(1){
		usleep(100*1000);
	}
}

static void printf_command_help(void)
{
	V4L2_DBG("\n");
	V4L2_DBG("***********************command help*****************************\n");
	V4L2_DBG("*                                                              *\n");
	V4L2_DBG("*run cmd: demo-v4l2  [cmd]                                     *\n");
	V4L2_DBG("*                                                              *\n");
	V4L2_DBG("*cmd1 option: 1.display         2.getdata                      *\n");
	V4L2_DBG("*                                                              *\n");
	V4L2_DBG("*example:                                                      *\n");
	V4L2_DBG("*         demo-v4l2   display                                  *\n");
	V4L2_DBG("*         demo-v4l2   getdata                                  *\n");
	V4L2_DBG("*                                                              *\n");
	V4L2_DBG("****************************************************************\n");
	V4L2_DBG("\n");
}

int main(int argc, char *argv[])
{
	struct VideoDevice Ark_Vdev;
	int work_mode; 
	int Ret;

	Ark_Vdev.src_width = 720;                //set pal
	Ark_Vdev.src_height = 288;

	/* init device */
	Ret = V4l2InitDevice(&Ark_Vdev);
	if(Ret < 0){
		V4L2_DBG("init v4l2 device error.\n");
		return -1;
	}

	printf_command_help();
	g_ark_Vdev = &Ark_Vdev;
	
	if(argc == 1){
		work_mode = WORK_MODE_DISPLAY;
		printf("==>get run cmd: %s display \n",argv[0]);	
	}
	else if(argc == 2){
		if(strcmp("display",argv[1]) == 0){
				work_mode = WORK_MODE_DISPLAY;
				printf("==>get run cmd: %s %s \n",argv[0],argv[1]);
			}
		else if(strcmp("getdata",argv[1]) == 0){
				work_mode = WORK_MODE_GETDATA;
				printf("==>get run cmd: %s %s \n",argv[0],argv[1]);
			}
		else{
			printf("cmd = %s, error!\n",argv[1]);
			return 0;
		}
	}
	else{
		printf("argc=%d, error!\n",argc);
		return 0;
	}
	switch (work_mode){
		case WORK_MODE_DISPLAY:
			display_demo();
			break;
		case WORK_MODE_GETDATA:
			getdata_demo();
			break;
		default:
			V4L2_DBG("work mode none, exit!\n");
			break;
	}

	display_hide_layer();
	/*close device*/
	V4l2StopDevice(&Ark_Vdev);
	V4l2ExitDevice(&Ark_Vdev);
	printf("V4l2ExitDevice done.\n");
	return 0;	
}
