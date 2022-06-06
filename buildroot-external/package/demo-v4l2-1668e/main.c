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

//#define DEBUG

#ifdef DEBUG
#define V4L2_DBG(...) printf(__VA_ARGS__)
#else
#define V4L2_DBG(...)
#endif

#define BUF_NUM 				8
#define DISPLAY 				1
#define GETDATA 				2 

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

enum work_mode { 
	WORK_MODE_GETDATA,
	WORK_MODE_END,
}; 

enum dvr_source {
	DVR_SOURCE_CAMERA,
	DVR_SOURCE_AUX,
	DVR_SOURCE_DVD,
};


struct VideoDevice *g_ark_Vdev;
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

static void* display_work_thread(void *param)
{
	int fd_t,Ret,i;
	if(t_cmd == GETDATA){
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
	printf("\n");
	printf("***********************command help*****************************\n");
	printf("*                                                              *\n");
	printf("*run cmd: demo-v4l2  [width]  [height]                         *\n");
	printf("*                                                              *\n");
	printf("*                                                              *\n");
	printf("*example:                                                      *\n");
	printf("*         cvbs  :   ntsc   demo-v4l2 720 240                   *\n");
	printf("*                   pal    demo-v4l2 720 288                   *\n");
	printf("*         720p  :          demo-v4l2 1280 720                  *\n");
	printf("*         1080p :          demo-v4l2 1920 1080                 *\n");
	printf("*                                                              *\n");
	printf("****************************************************************\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	struct VideoDevice Ark_Vdev;
	int work_mode; 
	int Ret,width,height;

	if(argc == 1){	
		printf("cmd error.\n");
		printf_command_help();
		return -1;
	}

	width = atoi(argv[1]);
	height = atoi(argv[2]);

	Ark_Vdev.src_width = width;
	Ark_Vdev.src_height = height;

	/* init device */
	Ret = V4l2InitDevice(&Ark_Vdev);
	if(Ret < 0){
		V4L2_DBG("init v4l2 device error.\n");
		return -1;
	}
 
	g_ark_Vdev = &Ark_Vdev;
	work_mode = WORK_MODE_GETDATA;
	
	switch (work_mode){
		case WORK_MODE_GETDATA:
			getdata_demo();
			break;
		default:
			V4L2_DBG("work mode none, exit!\n");
			break;
	}

	/*close device*/
	V4l2StopDevice(&Ark_Vdev);
	V4l2ExitDevice(&Ark_Vdev);
	printf("V4l2ExitDevice done.\n");
	return 0;	
}
