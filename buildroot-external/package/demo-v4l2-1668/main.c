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

#define DISP_WIDTH              1024
#define DISP_HEIGHE             600
#define DISP_SOURCE_WIDTH       720
#define DISP_SOURCE_HEIGHE      240
#define BUF_NUM 				4
#define DISPLAY 				1
#define GETDATA 				2

struct vin_display_outpara{
	unsigned int layer;
	unsigned int pos_data;
	unsigned int source_size;
	unsigned int layer_size;
	unsigned int format;
	struct ark_disp_addr disp_addr;
	struct ark_disp_scaler scaler;
};

struct VideoDevice {
    int Vfd;
	int VpixelFormat;
    int VideoBufCurIndex;
	int VideoBufCnt;
	int VideoBufMaxLen;
	unsigned char *Cam_vbuf[BUF_NUM];           //video data virtual  address 
	unsigned char* Video_Data;					//video data physical address
};

enum work_mode {
	WORK_MODE_DISPLAY,
	WORK_MODE_GETDATA,
	WORK_MODE_END,
};

struct VideoDevice *g_ark_Vdev;
struct vin_display_outpara* g_ark_para;
static pthread_t v4l2_work_id;
int t_cmd;

/* This function first makes the query judgmentThe space is then allocated and queued */
static int V4l2InitDevice(struct VideoDevice *Vdec)
{
	struct v4l2_fmtdesc tFmtDesc;
	struct v4l2_requestbuffers tV4l2buf;
	struct v4l2_format  tV4l2Fmt;
	struct v4l2_buffer tV4l2Buf;
	struct v4l2_capability tV4l2Cap;
	int Fd,Ret,Error,i;
	
	Fd = open("/dev/video0",O_RDWR);
	if( Fd < 0 )
	{	
		printf("open error.\n");
		return -1;
	}
	Vdec->Vfd = Fd;
	
	memset(&tV4l2Cap, 0, sizeof(struct v4l2_capability));
    Error = ioctl(Fd, VIDIOC_QUERYCAP, &tV4l2Cap);
    if (Error) {
    	printf("unable to query device.\n");
    	return -1;
    }

	if (!(tV4l2Cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
    	printf("device is not a video capture device.\n");
		return -1;
    }

	tFmtDesc.index = 0;
	tFmtDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(Fd,VIDIOC_ENUM_FMT,&tFmtDesc);
	Vdec->VpixelFormat = tFmtDesc.pixelformat;
	
	memset(&tV4l2Fmt, 0, sizeof(struct v4l2_format));
	tV4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2Fmt.fmt.pix.pixelformat = Vdec->VpixelFormat;
	tV4l2Fmt.fmt.pix.width       = 720;
	tV4l2Fmt.fmt.pix.height      = 240;
	tV4l2Fmt.fmt.pix.field       = V4L2_FIELD_ANY;
	Ret = ioctl(Fd, VIDIOC_S_FMT, &tV4l2Fmt); 
	if (Ret) 
		printf("Unable to set format\n");
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
		//printf("Vdec->VideoBufMaxLen = %d \n",Vdec->VideoBufMaxLen);
		/* 0 is start anywhere */
		Vdec->Cam_vbuf[i] = mmap(0,tV4l2Buf.length, PROT_READ, MAP_SHARED,Fd,tV4l2Buf.m.offset);
		if (Vdec->Cam_vbuf[i] == MAP_FAILED) 
		{
			printf("Unable to map buffer\n");
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
    	    printf("Unable to queue buffer.\n");
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
		printf("poll error!\n");
		return -1;
	}
	
	memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer)); 
	tV4l2Buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
	Ret = ioctl(Vdec->Vfd, VIDIOC_DQBUF, &tV4l2Buf);
	if (Ret < 0) 
	{
		printf("Unable to dequeue buffer.\n");
		return -1;
	}
	Vdec->VideoBufCurIndex = tV4l2Buf.index;
	Vdec->VideoBufMaxLen = tV4l2Buf.length;
	//printf("app get buf index : %d\n",tV4l2Buf.index);
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
	/*Put the extracted buf in the queue*/
	Error = ioctl(Vdec->Vfd, VIDIOC_QBUF, &tV4l2Buf);
	if (Error) 
   	{
	    printf("Unable to queue buffer.\n");
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
    	printf("Unable to start capture.\n");
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
    	printf("Unable to stop capture.\n");
    	return -1;
    }
	
    return 0;
}

void display_layer_init(struct VideoDevice *Vdec)
{
	screen_info screen;
	disp_scaler out_para;
	int Ret,Format;
	Ret = arkapi_display_get_screen_info(&screen);
	if(Ret == 0){
		g_ark_para->layer_size= (screen.height << 16) | screen.width;
	}else{
		g_ark_para->layer_size = (DISP_HEIGHE << 16) | DISP_WIDTH;
	}
	g_ark_para->source_size = (DISP_SOURCE_HEIGHE << 16) | DISP_SOURCE_WIDTH;
	g_ark_para->pos_data = 0;
	g_ark_para->format = ARK_LCDC_FORMAT_VYUY;    //| ARK_LCDC_ORDER_VYUY;

	memset(&out_para, 0, sizeof(disp_scaler));
	out_para.src_w=DISP_SOURCE_WIDTH;
	out_para.src_h=DISP_SOURCE_HEIGHE;
	out_para.out_w=screen.width;
	out_para.out_h=screen.height;

	printf("display_layer_init : out_w = %d out_h = %d  \n",screen.width,screen.height);
	g_ark_para->scaler = out_para;
	
	display_hide_layer();

	ioctl(g_ark_Vdev->Vfd, VIN_SET_WINDOW_POS, g_ark_para);
	ioctl(g_ark_Vdev->Vfd, VIN_SET_SOURCE_SIZE, g_ark_para);
	ioctl(g_ark_Vdev->Vfd, VIN_SET_LAYER_SIZE, g_ark_para);
	ioctl(g_ark_Vdev->Vfd, VIN_SET_WINDOW_FORMAT, g_ark_para);
	ioctl(g_ark_Vdev->Vfd, VIN_SET_WINDOW_SCALER, g_ark_para);
	
}


void display_set_addr(u32 pyrgbaddr, u32 pcbaddr, u32 pcraddr)
{
	//printf("app write buf  : %p\n",Vbuf->Video_Data);
	struct ark_disp_addr addr;
	addr.yaddr = pyrgbaddr;
	addr.cbaddr = pcbaddr;
	addr.craddr = pcraddr;
	g_ark_para->disp_addr = addr;
	ioctl(g_ark_Vdev->Vfd, VIN_SET_WINDOW_ADDR, g_ark_para);
}

void display_show_layer(void)
{
	ioctl(g_ark_Vdev->Vfd, VIN_SHOW_WINDOW, g_ark_para);
}

void display_hide_layer(void)
{
	ioctl(g_ark_Vdev->Vfd, VIN_HIDE_WINDOW, g_ark_para);
}

static void* v4l2_work_thread(void *param)
{
	int fd_t,Ret,i;
	if(t_cmd == DISPLAY){
	    while(1){
			V4l2GetDataForStreaming(g_ark_Vdev);
			display_set_addr(g_ark_Vdev->Video_Data,0,0);
			V4l2PutDataForStreaming(g_ark_Vdev);
	   }
	}
	else if(t_cmd == GETDATA){
		fd_t = open("stream.bin",O_RDWR|O_CREAT);
		if(fd_t < 0){
			printf("open error.\n");
			return 0;
		}
		while(1){
			V4l2GetDataForStreaming(g_ark_Vdev);
			Ret = write(fd_t,g_ark_Vdev->Cam_vbuf[g_ark_Vdev->VideoBufCurIndex],g_ark_Vdev->VideoBufMaxLen);
			V4l2PutDataForStreaming(g_ark_Vdev);
		}
	}
	else{
		printf("mode error t_cmd = %d.\n",t_cmd);
		return 0;
	}
}

void video_start(void)
{
	//printf("video_start cmd = %d \n",t_cmd);
	pthread_create(&v4l2_work_id, NULL, v4l2_work_thread,NULL);
}

int display_demo(void)
{
	char flag_quit[10];
	int Ret;
	t_cmd = DISPLAY;
	display_layer_init(g_ark_Vdev);
	/* start device && open display layer*/
	Ret = V4l2StartDevice(g_ark_Vdev);
	if(Ret < 0){
		printf("start v4l2 device error.\n");
		return 0;
	}

	display_show_layer();
	
	video_start();
	while(1){
		printf("exit <enter quit>:\n");
		scanf("%s",&flag_quit);
		if(strncmp(flag_quit,"quit",5) == 0){
			display_hide_layer();
			break;
		}
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
		printf("start v4l2 device error.\n");
		return 0;
	}
	video_start();
	while(1){
		printf("exit <enter quit>:\n");
		scanf("%s",&flag_quit);
		if(strncmp(flag_quit,"quit",5) == 0){
			break;
		}
	}
}

static void printf_command_help(void)
{
	printf("\n");
	printf("***********************command help*****************************\n");
	printf("*                                                              *\n");
	printf("*run cmd: demo-v4l2  [cmd]                                     *\n");
	printf("*                                                              *\n");
	printf("*cmd1 option: 1.display         2.getdata                      *\n");
	printf("*                                                              *\n");
	printf("*example:                                                      *\n");
	printf("*         demo-v4l2   display                                  *\n");
	printf("*         demo-v4l2   getdata                                  *\n");
	printf("*                                                              *\n");
	printf("****************************************************************\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct VideoDevice Ark_Vdev;
	struct vin_display_outpara Ark_para;
	int work_mode; 
	int Ret;
	/* init device */
	Ret = V4l2InitDevice(&Ark_Vdev);
	if(Ret < 0){
		printf("init v4l2 device error.\n");
		return -1;
	}

	printf_command_help();
	g_ark_Vdev = &Ark_Vdev;
	g_ark_para = &Ark_para;
	
	Ark_para.layer = AUX_LAYER;
	
	if(argc == 1){
		work_mode = WORK_MODE_DISPLAY;	
	}
	else if(argc == 2){
		if(strcmp("display",argv[1]) == 0)
			work_mode = WORK_MODE_DISPLAY;
		else if(strcmp("getdata",argv[1]) == 0)
			work_mode = WORK_MODE_GETDATA;
		else{
			printf("cmd = %s, error!\n",argv[1]);
			return 0;
		}
	}
	else{
		printf("argc=%d, error!\n",argc);
		return 0;
	}
	printf("==>get run cmd: %s %s \n",argv[0],argv[1]);
	switch (work_mode){
		case WORK_MODE_DISPLAY:
			display_demo();
			break;
		case WORK_MODE_GETDATA:
			getdata_demo();
			break;
		default:
			printf("work mode none, exit!\n");
			break;
	}
	/*close device*/
	V4l2StopDevice(&Ark_Vdev);
	V4l2ExitDevice(&Ark_Vdev);
	return 0;	
}
