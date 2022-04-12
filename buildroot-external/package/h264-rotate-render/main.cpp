#include <iostream>
#include <fstream> 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>

using namespace std;
#include "ARKMirrorRender.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


#define TEST_H264_FILE        1
#define TEST_H264_STREAM      2

struct test_info{
	int type;
	int mode;
	char filepath[64];
};


static const int FRAME_NUM = 305;
//static const int FRAME_NUM = 100;
static const int MAX_FILE_SIZE = 0x100000;

struct test_info test;
int ark_test_mode;

int64_t get_current_time_us(void)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec * 1000000ll + tv.tv_usec;
}


#if (ARK_COMPILE_MODE == COMPILE_TEST_ARK_API)
static void *proc_h264_stream(void)
{
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame;
	AVPacket *packet;
	int ret, got_picture;
	char *filepath = test.filepath;
	//char *filepath = "/media/test.h264";

	//////////////////////add////////////////////////
	ARKMirrorRender render;
	int screen_width, screen_height;
	screen_info screen;
	ECMirrorRender::MirrorDirection dir = ECMirrorRender::VERTICAL;
	char buf[64];

	ret = arkapi_display_get_screen_info(&screen);
	if(ret == 0){
		screen_width  = screen.width;
		screen_height = screen.height;
	}else{
		screen_width  = 1024;
		screen_height = 600;
	}

	printf("screen_width=%d, screen_height=%d \n",screen_width, screen_height);

	render.setScreenSize(screen_width, screen_height);
	render.setMirrorSize(800, 480);
	render.initialize();

	////////////////////////////////////////////////////
	while(1){
		
		av_register_all();
		avformat_network_init();
		pFormatCtx = avformat_alloc_context();
		ret = avformat_open_input(&pFormatCtx, filepath,NULL,NULL);
		if(ret!=0){
			av_strerror(ret, buf, 64);
			printf("Couldn't open file %s: %d(%s)", filepath, ret, buf);
			return NULL;
		}

		printf("open input stream sucess, %s.\n",filepath);

		if(avformat_find_stream_info(pFormatCtx,NULL)<0){
			printf("Couldn't find stream information.\n");
			avformat_close_input(&pFormatCtx);
			return NULL;
		}
		videoindex=-1;

		for(i=0; i<pFormatCtx->nb_streams; i++) {
			if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
				videoindex=i;
				break;
			}
		}
		if(videoindex==-1){
			printf("Didn't find a video stream.\n");
			avformat_close_input(&pFormatCtx);
			return NULL;
		}

		pCodecCtx=pFormatCtx->streams[videoindex]->codec;
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL){
			printf("Codec not found.\n");
			avcodec_close(pCodecCtx);
			avformat_close_input(&pFormatCtx);
			return NULL;
		}
		if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
			printf("Could not open codec.\n");
			avcodec_close(pCodecCtx);
			avformat_close_input(&pFormatCtx);
			return NULL;
		}

		pFrame=av_frame_alloc();

		packet=(AVPacket *)av_malloc(sizeof(AVPacket));

		av_dump_format(pFormatCtx,0,filepath,0);

		unsigned char *tmp = NULL;
		int payload_len = 0;
		int count = 0;
		int time1 = get_current_time_us()/1000;
		int time2 = 0;

		while(av_read_frame(pFormatCtx, packet)>=0){
			if(packet->stream_index==videoindex){
				//Decode
				tmp = (unsigned char *)packet->data;//packet->size
				//printf("size=%6d ", packet->size);
				//for (i = 0; i < 8; i++)
				//	printf("%02x ", tmp[i]);
				//printf("\n");

				//add 
				render.play(tmp, packet->size, dir);
				usleep(30000);

				if(count%300 == 0){
					time2 = get_current_time_us()/1000;
					printf("count=%d time=%d.\n",count, time2-time1);
					time1 = time2;
				}

				count++;
			}
			av_free_packet(packet);
		}	
		if (pCodecCtx)		
			avcodec_close(pCodecCtx);	
		if (pCodecCtx)		
			avformat_close_input(&pFormatCtx);
	}

	//add
	render.hide();
	render.release();

	return NULL;
}

#endif
int process_h264_file(void)
{
	ARKMirrorRender render;
	int ret, screen_width, screen_height;
	screen_info screen;

	ret = arkapi_display_get_screen_info(&screen);
	if(ret == 0){
		screen_width  = screen.width;
		screen_height = screen.height;
	}else{
		screen_width  = 1024;
		screen_height = 600;
	}
	printf("screen_width=%d, screen_height=%d \n",screen_width, screen_height);

	render.setScreenSize(screen_width, screen_height);
	render.setMirrorSize(720, 1280);
	render.initialize();

	ifstream infile;
	char filename[32];
	int filelen = 0;
	char *filedata = new char[MAX_FILE_SIZE];

	if (filedata == NULL) {
		cout << "alloc buffer fail." << endl;
		return -1;
	}

	
#if (ARK_COMPILE_MODE == COMPILE_TEST_ARK_API)
	while(1){
#endif
		for (int i = 0; i < FRAME_NUM; i++) {
		sprintf(filename, "%s/%.3d.h264",test.filepath, i);
		//sprintf(filename, "/usr/share/pieces/%.3d.h264", i);
		infile.open(filename, ios::binary);
		if (!infile.is_open()) {
			cout << "open file " << filename << " fail." << endl;
			continue;
		}
		infile.seekg(0, ios::end);
		filelen = infile.tellg();
		infile.seekg(0, ios::beg);
		infile.read(filedata, filelen);
		ECMirrorRender::MirrorDirection dir = ECMirrorRender::VERTICAL;

		if (i >= 264)
			dir = ECMirrorRender::VERTICAL;
		else if (i >= 212)
			dir = ECMirrorRender::LANDSCAPE;
		else if (i >= 168)
			dir = ECMirrorRender::VERTICAL;
		else if (i >= 90)
			dir = ECMirrorRender::LANDSCAPE;

		render.play(filedata, filelen, dir);
		infile.close();

		if(ark_test_mode == ARKN141_MFC_DISPLAY_API)
			usleep(25000);

		}
#if (ARK_COMPILE_MODE == COMPILE_TEST_ARK_API)
	}
#endif
	delete []filedata;

	render.hide();
	render.release();

}

static int parser_command_set_para(char* pcmd0, char* pcmd1)
{
	int i;
	char *p;

	printf("pcmd0 parser==>%s\n",pcmd0);
	if(strstr(pcmd0,"video-api")){
		test.mode = USE_ARK_VIDEO_API;	
	}
	else if(strstr(pcmd0,"2d-api")){
		test.mode = USE_ARK_DISPLAY_2D_API;
	}
	else if(strstr(pcmd0,"2draw-api")){
		test.mode = USE_ARK_RAW2D_DISPLAY_API;
	}
	else if(strstr(pcmd0,"display-api")){
		test.mode = ARK1668_MFC_DISPLAY_API;
	}
	else if(strstr(pcmd0,"n141-display")){
		test.mode = ARKN141_MFC_DISPLAY_API;
	}else{
		test.mode = USE_ARK_VIDEO_API;	
	}

	printf("pcmd1 parser==>%s\n",pcmd1);
	if(strstr(pcmd1,"pieces")){
		test.type = TEST_H264_FILE;	
	}else if(strstr(pcmd1,".h264")){
		test.type= TEST_H264_STREAM;
	}else{
		printf("cmd0 parser error.\n");
		test.type= TEST_H264_STREAM;
		//return -1;
	}

	p = pcmd1;
	i = 0;
	while(*p != '\0'){
		test.filepath[i++] = *p++;
	}


	return 0;
}


static void printf_command_help(void)
{

	printf("\n");
	printf("***********************command help*****************************\n");
	printf("*                                                              *\n");
	printf("*run cmd: h264-rotate-render  [cmd1]  [cmd2]                   *\n");
	printf("*                                                              *\n");
	printf("*cmd1 option: 1 video-api             2 2d-api                 *\n");
	printf("*             3 2draw-api             4 display-api            *\n");
	printf("*             5 n141-display                                   *\n");
	printf("*                                                              *\n");
	printf("*cmd2 option: filepath                                         *\n");
	printf("*                                                              *\n");
	printf("*example:                                                      *\n");
	printf("*    h264-rotate-render video-api     /media/pieces            *\n");
	printf("*    h264-rotate-render n141-display /media/test_800x480.h264  *\n");
	printf("*                                                              *\n");
	printf("****************************************************************\n");
	printf("\n");

	
}

int main(int argc, char *argv[])
{
	char filepath[32]= "/media/pieces/";
	int ret, compile_mode;

	printf_command_help();

	if(argc == 1){
		test.type = TEST_H264_FILE;
		test.mode = USE_ARK_VIDEO_API;
		memcpy(&test.filepath, filepath, 32);
	}else if(argc == 3){
		printf("==>get run cmd: %s  %s  %s\n",argv[0],argv[1],argv[2]);
		ret = parser_command_set_para(argv[1],argv[2]);
		if(ret < 0){
			printf("run para error, exit1.\n");
			return 0;
		}
	}else{
		printf("run para error, exit2.\n");
		return 0;
	}

	ark_test_mode = test.mode;	
	compile_mode = ARK_COMPILE_MODE;
	printf("ARK_COMPILE_MODE=%d, ark_test_mode=%d.\n", compile_mode, ark_test_mode);
#if (ARK_COMPILE_MODE == COMPILE_TEST_ARK_API)
	if (test.type == TEST_H264_STREAM)
		proc_h264_stream();
	else
#endif
		process_h264_file();


	return 0;
}


