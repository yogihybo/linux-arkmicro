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
#include "include/ark_api.h"

#define TEST_ARK1668     1
#define TEST_ARKN141     2
#define TEST_ARK1668E    3

//#define FILE_FORMAT_RGB    1
//#define FILE_FORMAT_YUV    2
//#define FILE_FORMAT_NV12   3
      
#define DISP_WIDTH              1024
#define DISP_HEIGHE             600

struct display_info{
	disp_handle *display_handle;
	int lay_id;

	unsigned int disp_posx;
	unsigned int disp_posy;
	unsigned int disp_width;
	unsigned int disp_height;
	struct ark_reqbuf reqbuf;
};

struct test_info{
	int platform;
	int pic_format;
	char filepath[64];
};

static struct display_info *overlay = NULL;
static struct display_info *overlay2 = NULL;
static memalloc_handle * mem_handle = NULL;
static struct test_info test;


extern void carback_work_start(void);
extern void carback_work_exit(void);

static void generate_argb_pic(void *dst, int width, int height, unsigned int argb)
{
	unsigned int *p = dst;
	int i,j;
	for(i = 0; i < height; i++){
		for(j = 0; j < width; j++){
			*p++ = argb;
		}
	}

}

static int get_file_argb_picture(void *dst, int width, int height)
{
	int fd;
	//static int init = 0;

	//if(init)
	//	return 0;
	//1. read file data
	fd = open(test.filepath, O_RDWR);
	if (fd < 0) {
		printf("open %s fail.\n",test.filepath);
		return 0;
	} else {
		read(fd,dst,width*height*4);
		close(fd);
		//init = 1;
		printf("open %s  success.\n",test.filepath);
	}
	return 0;
}

static int disp_color_init_test(void){
	struct ark_disp_color color;
	int i, layer_max;

	if(test.platform == TEST_ARKN141)
		layer_max = 2;
	else
		layer_max = 5;
	    

	for(i = PRIMARY_LAYER; i < layer_max; i++){
		color.contrast   = 0x80;
		color.brightness = 0x80;
		color.saturation = 0x40;
		color.hue        = 0;
		arkapi_display_layer_set_color(i, &color);
		arkapi_display_layer_get_color(i, &color);
		printf("%s-->layer=%d, %d %d %d %d.\n", __func__, i,color.contrast,color.brightness,color.saturation,color.hue);
	}
}

static struct display_info * disp_layer_init(enum ark_disp_layer layer, int format, int width, int height, int buf_cnt)
{
	struct display_info *pinfo = NULL;
	struct ark_reqbuf reqbuf;
	unsigned int addr, ret;
	disp_handle *display_handle;

	pinfo = (struct display_info *)malloc(sizeof(struct display_info));
	if (pinfo == NULL) {
		printf("no enough memory be alloc");
		return NULL;
	}

	if(buf_cnt >= FRAME_MAX){
		printf("buf_cnt max=%d, error.",FRAME_MAX);

	}
    
	memset(pinfo, 0, sizeof(struct display_info));
	pinfo->disp_posx   = 0;
	pinfo->disp_posy   = 0;
	pinfo->disp_width  = width;
	pinfo->disp_height = height;
	pinfo->lay_id = layer;

	display_handle = arkapi_display_open_layer(pinfo->lay_id);
	if(!display_handle){
		printf("open fb%d error.",pinfo->lay_id);
		free(pinfo);
		return NULL;
	}
	pinfo->display_handle = display_handle;
	arkapi_display_hide_layer(display_handle);
	reqbuf.count= buf_cnt;
	reqbuf.size= pinfo->disp_width * pinfo->disp_height * 4;
	ret = arkapi_memalloc_reqbuf(mem_handle, &reqbuf);
	if(ret != 0){
		printf("buffer request error ,ret=%d.\n",ret);
		return NULL;
	}

	memcpy(&pinfo->reqbuf, &reqbuf, sizeof(struct ark_reqbuf));

#if 1
	arkapi_display_set_layer_pos_atomic(display_handle, pinfo->disp_posx, pinfo->disp_posy);
	arkapi_display_set_layer_size_atomic(display_handle, pinfo->disp_width, pinfo->disp_height);
	arkapi_display_set_layer_format_atomic(display_handle, format);
	arkapi_display_layer_update_commit(display_handle);
#else
	arkapi_display_set_layer_pos(display_handle, pinfo->disp_posx, pinfo->disp_posy);
	arkapi_display_set_layer_size(display_handle, pinfo->disp_width, pinfo->disp_height);
	arkapi_display_set_layer_format(display_handle, format);
#endif
	return pinfo;

}

static void disp_layer_deinit(struct display_info *pinfo)
{
	if(pinfo){
		arkapi_display_close_layer(pinfo->display_handle);
		free(pinfo);
	}
}

static int overlay_update(struct display_info *pinfo)
{
	static int id = 0;
	static int pos_x = 0;
	static int pos_y = 0; 
	static int width; 
	static int height;
	static unsigned int rgb = 0;
	int value;

	if(!pinfo) 
		return -EINVAL;

	if(pos_x == 0 && pos_y == 0){
		width = pinfo->disp_width;
		height = pinfo->disp_height;
	}

	if(id >= pinfo->reqbuf.count)
		id = 0;
	
	if(rgb%3 == 0) {
		//value = 0xa0800080;//a  r  g  b
		value = 0xa0ff0000;
	} else if(rgb%3 == 1){
		//value = 0xa0ffff00;//a  r  g  b
		value = 0xa000ff00;
	//} else if(rgb++%3 == 2) {
	} else {
		//value = 0xa0ffff00;//a  r  g  b
		value = 0xa00000ff;
	}
	rgb++;
	printf("generate-->0x%08x.\n", value);
	generate_argb_pic(pinfo->reqbuf.virt_addr[id], pinfo->disp_width, pinfo->disp_height, value);
	
	//arkapi_display_set_layer_format(pinfo->fd, format);
	arkapi_display_set_layer_pos(pinfo->display_handle, pos_x, pos_y);
	arkapi_display_set_layer_size(pinfo->display_handle, width, height);
	if(pinfo->reqbuf.count > 1) {
		int count = 3;
		while(count--) {
			int yaddr, uaddr, vaddr;
			arkapi_display_get_layer_addr(pinfo->display_handle, &yaddr, &uaddr, &vaddr);
			if(yaddr != pinfo->reqbuf.phy_addr[id])
				break;
			arkapi_display_wait_for_vsync(pinfo->display_handle);
		}
	}
	arkapi_display_set_layer_addr(pinfo->display_handle, pinfo->reqbuf.phy_addr[id], 0, 0);
	//change test
	pos_x += 10*pinfo->disp_width/pinfo->disp_height;
	pos_y += 10;
	width  -= 20*pinfo->disp_width/pinfo->disp_height;
	height -= 20;
	if((width <= 0) || (height <= 30)){
		pos_x = 0;
		pos_y = 0;
		width = pinfo->disp_width;
		height = pinfo->disp_height;
	}
	id++;

	return 0;
}

static int overlay2_update(struct display_info *pinfo)
{
	int value;
	int yaddr, uaddr, vaddr;
	static int id = 0;

	if(!pinfo) 
		return -EINVAL;
	if(id >= pinfo->reqbuf.count)
		id = 0;
	
	yaddr = pinfo->reqbuf.phy_addr[id];
	uaddr = yaddr + pinfo->disp_width * pinfo->disp_height;

	//value = 0xa0005050;//a  r  g  b
	//printf("generate-->0x%08x.\n",value);
	//generate_argb_pic((void*)pinfo->buf[0].yaddr, pinfo->disp_width, pinfo->disp_height, value);
	get_file_argb_picture(pinfo->reqbuf.virt_addr[id], pinfo->disp_width, pinfo->disp_height);
	if(test.pic_format == ARK_LCDC_FORMAT_Y_UV420) {
		vaddr = 0;
	} else if(test.pic_format == ARK_LCDC_FORMAT_YUV420){
		vaddr = uaddr+ pinfo->disp_width * pinfo->disp_height / 4;
	} else if(test.pic_format == ARK_LCDC_FORMAT_Y_UV422) {
		vaddr = 0;
	} else if(test.pic_format == ARK_LCDC_FORMAT_YUV422) {
		vaddr = uaddr+ pinfo->disp_width * pinfo->disp_height / 2;
	} else {
		uaddr = vaddr = 0;
	}

	if(pinfo->reqbuf.count > 1) {
		int count = 3;
		while(count--) {
			int y, u, v;
			arkapi_display_get_layer_addr(pinfo->display_handle, &y, &u, &v);
			if(yaddr != y)
				break;
			arkapi_display_wait_for_vsync(pinfo->display_handle);
		}
	}
	arkapi_display_set_layer_addr(pinfo->display_handle, yaddr, uaddr, vaddr);

	//ui  显存默认地址 已经设置，如果没有切换需要，可以不用重新设置
	id++;

	return 0;
}

static int parser_command_set_para(char* pcmd0,char* pcmd1)
{
	char* p;
	int i;
	char *format_tail = NULL;

	/* Parse platform. */
	printf("pcmd0 parser==>%s\n",pcmd0);
	if(strstr(pcmd0,"n141")){
		test.platform = TEST_ARKN141;
	} else if(strncmp(pcmd0, "ark1668e", 8) == 0){
		test.platform = TEST_ARK1668E;
		printf("platform is ark1668e\n");
	} else if(strncmp(pcmd0, "ark1668", 7) == 0){
		test.platform = TEST_ARK1668;
		printf("platform is ark1668\n");
	} else{
		printf("cmd0 parser error.\n");
		return -1;
	}

	/* Parse format. */
	printf("pcmd1 parser==>%s\n",pcmd1);
	//new add as follows:
	if(strstr(pcmd1,".vyuy")) {
		test.pic_format = ARK_LCDC_FORMAT_VYUY;
		format_tail = ".vyuy";
	} else if(strstr(pcmd1,".y_uv420")) {
		test.pic_format = ARK_LCDC_FORMAT_Y_UV420;
		format_tail = ".y_uv420";
	} else if(strstr(pcmd1,".yuv420")) {
		test.pic_format = ARK_LCDC_FORMAT_YUV420;
		format_tail = ".yuv420";
	} else if(strstr(pcmd1,".y_uv422")) {
		test.pic_format = ARK_LCDC_FORMAT_Y_UV422;
		format_tail = ".y_uv422";
	} else if(strstr(pcmd1,".yuv422")) {
		test.pic_format = ARK_LCDC_FORMAT_YUV422;
		format_tail = ".yuv422";

	//Old format supported as follows.
	} else if(strstr(pcmd1,".rgb")) {
		test.pic_format = ARK_LCDC_FORMAT_RGBA888 | ARK_LCDC_ORDER_RGB;
		format_tail = ".rgb";
	} else if(strstr(pcmd1,".yuv")) {
		if (test.pic_format == TEST_ARKN141) {
			test.pic_format = ARK_LCDC_FORMAT_Y_UV420;
			format_tail = ".y_uv420";
		} else {
			test.pic_format = ARK_LCDC_FORMAT_VYUY;
			format_tail = ".vyuy";
		}
	} else {
		printf("cmd1 parser error.\n");
		return -1;
	}

	if(strlen(format_tail) == 0) {
		printf("Not recognize picture format\n");
		return -1;
	}
	printf("platform=%d, pic_format=%d.\n",test.platform, test.pic_format);

	//get file name format: 123.rgb  
	p = strtok(pcmd1,".");
	i = 0;
	while(*p != '\0'){
		test.filepath[i++] = *p++;
	}
	sprintf(&test.filepath[i], "%s", format_tail);

	return 0;
}


static int display_layer_scale_test(struct display_info *pinfo)
{
	static int id = 0;
	static int pos_x = 0;
	static int pos_y = 0; 
	static int width; 
	static int height;
	int yaddr, uaddr, vaddr;

	if(!pinfo) {
		printf("pinfo == NULL\n");
		return -EINVAL;
	}

	if(test.platform == TEST_ARK1668E) {
		if(pos_x == 0 && pos_y == 0){
			width = pinfo->disp_width;
			height = pinfo->disp_height;
		}

		if(id >= pinfo->reqbuf.count)
			id = 0;
		
		printf("generate-->id[%d]: 0x%08x.\n", id, pinfo->reqbuf.virt_addr[id]);
		get_file_argb_picture(pinfo->reqbuf.virt_addr[id], pinfo->disp_width, pinfo->disp_height);

		yaddr = pinfo->reqbuf.phy_addr[id];
		uaddr = yaddr + pinfo->disp_width * pinfo->disp_height;
		if(test.pic_format == ARK_LCDC_FORMAT_Y_UV420) {
			vaddr = 0;
		} else if(test.pic_format == ARK_LCDC_FORMAT_YUV420){
			vaddr = uaddr+ pinfo->disp_width * pinfo->disp_height / 4;
		} else if(test.pic_format == ARK_LCDC_FORMAT_Y_UV422) {
			vaddr = 0;
		} else if(test.pic_format == ARK_LCDC_FORMAT_YUV422) {
			vaddr = uaddr+ pinfo->disp_width * pinfo->disp_height / 2;
		} else {
			uaddr = vaddr = 0;
		}

		arkapi_display_set_layer_pos(pinfo->display_handle, pos_x, pos_y);
		if(pinfo->reqbuf.count > 1) {
			int count = 3;
			while(count--) {
				int y, u, v;
				arkapi_display_get_layer_addr(pinfo->display_handle, &y, &u, &v);
				if(yaddr != y)
					break;
				arkapi_display_wait_for_vsync(pinfo->display_handle);
			}
		}
		arkapi_display_set_layer_addr(pinfo->display_handle, yaddr, uaddr, vaddr);

		disp_scaler scale;
		scale.src_w = pinfo->disp_width;
		scale.src_h = pinfo->disp_height;
		scale.win_x = 0;
		scale.win_y = 0;
		scale.win_w = pinfo->disp_width;
		scale.win_h = pinfo->disp_height;
		scale.out_x = pos_x;
		scale.out_y = pos_y;
		scale.out_w = width;
		scale.out_h = height;
		scale.cut_top = 0;
		scale.cut_bottom = 0;
		scale.cut_left = 0;
		scale.cut_right = 0;
		arkapi_display_set_layer_scaler(pinfo->display_handle, &scale);

		//change test
		pos_x += 8*(pinfo->disp_width/pinfo->disp_height);
		pos_y += 8;
		width  -= 16*(pinfo->disp_width/pinfo->disp_height);
		height -= 16;
		if((width <= 0) || (height <= 30)) {
			pos_x = 0;
			pos_y = 0;
			width = pinfo->disp_width;
			height = pinfo->disp_height;
		}
		id++;
	} else {
		printf("%s, Do not support platform:%d\n", __func__, test.platform);
	}
	return 0;
}

static int axi_scale_test(struct display_info *pinfo)
{
	int format, oformat;
	int iyaddr, iuaddr, ivaddr;
	int oyaddr, ouaddr, ovaddr;
	int iwidth = 1920;
	int iheight = 720;
	static int owidth; 
	static int oheight;
	static int pos_x = 0;
	static int pos_y = 0;
	static unsigned int id = 0;
	static int rotate = 0;
	int disp_width, disp_height;
	int ret;

	if((pos_x ==0) && (pos_y == 0)) {
		owidth = pinfo->disp_width;
		oheight = pinfo->disp_height;
	}
#if 0
	pos_x = 0;
	pos_y = 0;
	owidth = 1920;
	oheight = 720;
#endif

	if(test.pic_format == ARK_LCDC_FORMAT_VYUY) {
		format = ARK_SCALE_FORMAT_YUYV;
		oformat = ARK_SCALE_OUT_FORMAT_YUYV;
	} else if(test.pic_format == ARK_LCDC_FORMAT_Y_UV420) {
		format = ARK_SCALE_FORMAT_Y_UV420;
		oformat = ARK_SCALE_OUT_FORMAT_Y_UV420;
	} else if(test.pic_format == ARK_LCDC_FORMAT_YUV420) {
		format = ARK_SCALE_FORMAT_Y_U_V420;
		oformat = ARK_SCALE_OUT_FORMAT_Y_UV420;
	} else if(test.pic_format == ARK_LCDC_FORMAT_Y_UV422) {
		format = ARK_SCALE_FORMAT_Y_UV422;
		oformat = ARK_SCALE_OUT_FORMAT_Y_UV422;
	} else if(test.pic_format == ARK_LCDC_FORMAT_YUV422) {
		format = ARK_SCALE_FORMAT_YUV422;
		oformat = ARK_SCALE_OUT_FORMAT_Y_UV422;
	} else {
		printf("axi scalar not support format:%d\n", test.pic_format);
		return -1;
	}

	if(test.platform == TEST_ARK1668E) {
		printf("format:0x%x\n", format);
		printf("iwidth:%d, iheight:%d\n", iwidth, iheight);
		printf("owidth:%d, oheight:%d\n", owidth, oheight);

		get_file_argb_picture(pinfo->reqbuf.virt_addr[0], iwidth, iheight);
		iyaddr = (unsigned int)pinfo->reqbuf.phy_addr[0];
		if(id++%2) {
			oyaddr = (unsigned int)pinfo->reqbuf.phy_addr[1];
		} else {
			oyaddr = (unsigned int)pinfo->reqbuf.phy_addr[2];
		}
		
		iuaddr = iyaddr + iwidth * iheight;
		ouaddr = oyaddr + owidth * oheight;

		if(format == ARK_SCALE_FORMAT_Y_UV420) {
			ivaddr = 0;
			ovaddr = 0;
			printf("### axi-scale mode y_uv420\n");
		} else if(format == ARK_SCALE_FORMAT_Y_U_V420){
			ivaddr = iuaddr+ iwidth * iheight / 4;
			ovaddr = ouaddr+ owidth * oheight / 4;
			printf("### axi-scale mode yuv420\n");
		} else if(format == ARK_SCALE_FORMAT_Y_UV422) {
			ivaddr = 0;
			ovaddr = 0;
			printf("### axi-scale mode y_uv422\n");
		} else if(format == ARK_SCALE_FORMAT_YUV422) {
			ivaddr = iuaddr+ iwidth * iheight / 2;
			ovaddr = ouaddr+ owidth * oheight / 2;
			printf("### axi-scale mode yuv422\n");
		} else {
			printf("### axi-scale mode other:%d\n", format);
			ivaddr = 0;
			ovaddr = 0;
		}

		//printf("%s, iyaddr:0x%x, iuaddr:0x%x, ivaddr:0x%x,\n", __func__, iyaddr, iuaddr, ivaddr);
		//printf("%s, oyaddr:0x%x, ouaddr:0x%x, ovaddr:0x%x,\n", __func__, oyaddr, ouaddr, ovaddr);
		printf("### rotate:%d\n", rotate);

		ret = arkapi_scalar(iyaddr, iuaddr, ivaddr, format,
			iwidth,
			iheight,
			0, 0,
			iwidth,
			iheight,
			0, 0, 0, 0,
			owidth,
			oheight,
			oyaddr,
			ouaddr,
			ovaddr,
			oformat,
			//SCALE_ROTATE_0);
			SCALE_ROTATE_0 + rotate);
		if(ret)
			printf("arkapi_scalar failed, ret:%d\n", ret);


		if((rotate == SCALE_ROTATE_90) || (rotate == SCALE_ROTATE_270) ||
			(rotate == SCALE_ROTATE_90_MIRROR) || (rotate == SCALE_ROTATE_270_MIRROR)) {
			disp_width = oheight;
			disp_height = owidth;
		} else {
			disp_width = owidth;
			disp_height = oheight;
		}

		ovaddr = 0;

		if(oformat == ARK_SCALE_OUT_FORMAT_Y_UV420) {
			format = ARK_LCDC_FORMAT_Y_UV420;
			arkapi_display_set_layer_format(pinfo->display_handle, format);
		} else if(oformat == ARK_SCALE_OUT_FORMAT_Y_UV422) {
			format = ARK_LCDC_FORMAT_Y_UV422;
			arkapi_display_set_layer_format(pinfo->display_handle, format);
		} else if(oformat == ARK_SCALE_OUT_FORMAT_YUYV) {
			//format = ARK_LCDC_FORMAT_VYUY;
			//arkapi_display_set_layer_format(pinfo->display_handle, format);
		}

		arkapi_display_set_layer_pos(pinfo->display_handle, pos_x, pos_y);
		arkapi_display_set_layer_size(pinfo->display_handle, disp_width, disp_height);

		if(pinfo->reqbuf.count > 1) {
			int count = 3;
			while(count--) {
				int yaddr, uaddr, vaddr;
				arkapi_display_get_layer_addr(pinfo->display_handle, &yaddr, &uaddr, &vaddr);
				if(yaddr != oyaddr)
					break;
				arkapi_display_wait_for_vsync(pinfo->display_handle);
			}
		}
		arkapi_display_set_layer_addr(pinfo->display_handle, oyaddr, ouaddr, ovaddr);
#if 1
		pos_x += 8*(pinfo->disp_width/pinfo->disp_height);
		pos_y += 8;
		owidth  -= 16*(pinfo->disp_width/pinfo->disp_height);
		oheight -= 16;
		if((owidth <= 0) || (oheight <= 30)) {
			pos_x = 0;
			pos_y = 0;
			owidth = pinfo->disp_width;
			oheight = pinfo->disp_height;
		}
#else
		if(++rotate >= SCALE_ROTATE_END)
			rotate = SCALE_ROTATE_0;
#endif
	} else {
		printf("%s, Do not support platform:%d\n", __func__, test.platform);
	}
	return 0;
}

static int vp_test(int layer, int brightness, int contrast, int saturation, int hue)
{
	disp_color color;
	color.contrast = contrast;
	color.brightness = brightness;
	color.saturation = saturation;
	color.hue = hue;
	printf("%s, write:-->layer=%d, %d %d %d %d.\n", __func__,
		layer, color.contrast,color.brightness,color.saturation,color.hue);
	arkapi_display_layer_set_color(layer, &color);
	arkapi_display_layer_get_color(layer, &color);
	printf("%s, read: -->layer=%d, %d %d %d %d.\n", __func__,
		layer, color.contrast,color.brightness,color.saturation,color.hue);
	return 0;
}

static void printf_command_help(void)
{
	printf("\n");
	printf("***********************command help*****************************\n");
	printf("*                                                              *\n");
	printf("*run cmd: demo-display  [cmd1]   [cmd2]                        *\n");
	printf("*                                                              *\n");
	printf("*cmd1 option: 1.ark1668	2.n141	3.ark1668e                     *\n");
	printf("*                                                              *\n");
	printf("*cmd2 option: filename.type                                    *\n");
	printf("*     type: y_uv420/yuv420/y_uv422/yuv422/vyuy/yuv/rgb         *\n");
	printf("*                                                              *\n");
	printf("*example:                                                      *\n");
	printf("*         demo-display  ark1668   /usr/share/pic_1024x600.rgb  *\n");
	printf("*         demo-display  n141      /media/123.yuv               *\n");
	printf("*                                                              *\n");
	printf("****************************************************************\n");
	printf("\n");

	
}


int main(int argc, char *argv[])
{
	int ret, screen_width, screen_height;
	char filepath[32] = "/usr/share/pic_1024x600.rgb";
	screen_info screen;
	int format;

	printf_command_help();
	if(argc == 1){
		test.platform = TEST_ARK1668;
		test.pic_format = ARK_LCDC_FORMAT_RGBA888;
		memcpy(&test.filepath, filepath, 32);
	}else if(argc == 3){
		printf("==>get run cmd: %s  %s  %s \n",argv[0],argv[1],argv[2]);
		ret = parser_command_set_para(argv[1],argv[2]);
		if(ret < 0){
			printf("run para error, exit1.\n");
			return 0;
		}
	} else if(argc == 8) {	//vp test.
		printf("\n### cmd: %s  %s  %s %s %s %s %s %s\n\n",argv[0],argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[6]);
		parser_command_set_para(argv[1],argv[2]);
	}else{
		printf("run para error, exit2.\n");
		return 0;
	}

	if (test.platform == TEST_ARK1668)
		carback_work_start();

	mem_handle = arkapi_memalloc_init();

	ret = arkapi_display_get_screen_info(&screen);
	if(ret == 0){
		screen_width  = screen.width;
		screen_height = screen.height;
	}else{
		screen_width  = DISP_WIDTH;
		screen_height = DISP_HEIGHE;
	}
	printf("screen_width=%d, screen_height=%d \n",screen_width, screen_height);

#if 0
	/* display video/osd layer vp test */
	if(strncmp(argv[2], "vp_test", 7) == 0) {
		if(test.platform == TEST_ARK1668E) {
			int layer = atoi(argv[3]);
			int brightness = atoi(argv[4]);
			int contrast = atoi(argv[5]);
			int saturation = atoi(argv[6]);
			int hue = atoi(argv[7]);
			vp_test(layer, brightness, contrast, saturation, hue);
		}
		return 0;
	}
#endif

#if 0
	/* video layer scale test. */
	format = test.pic_format | ARK_LCDC_ORDER_VYUY;
	struct display_info *pinfo = disp_layer_init(TVOUT_LAYER, format, screen_width, screen_height, 2);
	arkapi_display_show_layer(pinfo->display_handle);
	while(1) {
		display_layer_scale_test(pinfo);
		sleep(1);
	}
#endif

#if 0
	/* axi scale test. */
	format = test.pic_format;
	if(format == ARK_LCDC_FORMAT_YUV420)
		format = ARK_LCDC_FORMAT_Y_UV420;
	else if(format == ARK_LCDC_FORMAT_YUV422)
		format = ARK_LCDC_FORMAT_Y_UV422;
	else
		format = ARK_LCDC_FORMAT_VYUY;
	//scale output format only support: Y_UV420, YUV422, VYUY.
	struct display_info *pinfo = disp_layer_init(VIDEO_LAYER, format, screen_width, screen_height, 3);
	arkapi_display_show_layer(pinfo->display_handle);

	do {
		axi_scale_test(pinfo);
		sleep(2);
		//goto exit;
	} while(1);
#endif

	/* display test. */
#if 1
	if(test.platform == TEST_ARKN141){
		format = ARK_LCDC_FORMAT_RGBA888 | ARK_LCDC_ORDER_RGB;
		overlay = disp_layer_init(VIDEO_LAYER,format, screen_width, screen_height, 3);
		format = test.pic_format | ARK_LCDC_ORDER_VYUY;
		overlay2 = disp_layer_init(PRIMARY_LAYER, format, screen_width, screen_height, 1);
	}else if(test.platform == TEST_ARK1668){
		format = ARK_LCDC_FORMAT_RGBA888 | ARK_LCDC_ORDER_RGB;
		overlay = disp_layer_init(OVER_LAYER,format, screen_width, screen_height, 3);
		format = test.pic_format | ARK_LCDC_ORDER_VYUY;
		overlay2 = disp_layer_init(VIDEO_LAYER, format, screen_width, screen_height, 1);
	} else if(test.platform == TEST_ARK1668E){
		format = ARK_LCDC_FORMAT_RGBA888 | ARK_LCDC_ORDER_RGB;
		overlay = disp_layer_init(OVER_LAYER,format, screen_width, screen_height, 3);
		format = test.pic_format | ARK_LCDC_ORDER_VYUY;
		overlay2 = disp_layer_init(VIDEO_LAYER, format, screen_width, screen_height, 1);
	} else {
		printf("Invalid platfrom:%d, exit.\n", test.platform);
		return 0;
	}

	//disp_color_init_test();	//Call after set format.
	arkapi_display_show_layer(overlay->display_handle);
	arkapi_display_show_layer(overlay2->display_handle);
	while(1){
		overlay_update(overlay);
		overlay2_update(overlay2);
		sleep(1);
	}
#endif
exit:
	if(overlay)
		disp_layer_deinit(overlay);
	if(overlay2)
		disp_layer_deinit(overlay2);

	if (test.platform == TEST_ARK1668)
		carback_work_exit();
	if(mem_handle)
		arkapi_memalloc_release(mem_handle);

	return 0;
}
