#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h> 
#include <asm/setup.h>
#include <linux/zlib.h>
#include <linux/cdev.h>
#include "ark_track.h" 
#include "ark1668e_carback.h"
#include "ark_mcu.h"
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

const idmap_info radar_idmap[RADAR_MAX] = 
{
	//FL
	{ "FL_00",FL_00,0},
	{ "FL_01",FL_01,0},
	{ "FL_02",FL_02,0},
	{ "FL_03",FL_03,0},
	{ "FL_04",FL_04,0},
	{ "FL_05",FL_05,0},
	{ "FL_06",FL_06,0},
	{ "FL_10",FL_10,0},
	{ "FL_11",FL_11,0},
	{ "FL_12",FL_12,0},
	{ "FL_13",FL_13,0},
	{ "FL_14",FL_14,0},
	{ "FL_15",FL_15,0},
	{ "FL_16",FL_16,0},
	{ "FL_20",FL_20,0},
	{ "FL_21",FL_21,0},
	{ "FL_22",FL_22,0},
	{ "FL_23",FL_23,0},
	{ "FL_24",FL_24,0},
	{ "FL_25",FL_25,0},
	{ "FL_26",FL_26,0},
	{ "FL_30",FL_30,0},
	{ "FL_31",FL_31,0},
	{ "FL_32",FL_32,0},
	{ "FL_33",FL_33,0},
	{ "FL_34",FL_34,0},
	{ "FL_35",FL_35,0},
	{ "FL_36",FL_36,0},
	{ "FL_40",FL_40,0},
	{ "FL_41",FL_41,0},
	{ "FL_42",FL_42,0},
	{ "FL_43",FL_43,0},
	{ "FL_44",FL_44,0},
	{ "FL_45",FL_45,0},
	{ "FL_46",FL_46,0},
	{ "FL_50",FL_50,0},
	{ "FL_51",FL_51,0},
	{ "FL_52",FL_52,0},
	{ "FL_53",FL_53,0},
	{ "FL_54",FL_54,0},
	{ "FL_55",FL_55,0},
	{ "FL_56",FL_56,0},
	{ "FL_60",FL_60,0},
	{ "FL_61",FL_61,0},
	{ "FL_62",FL_62,0},
	{ "FL_63",FL_63,0},
	{ "FL_64",FL_64,0},
	{ "FL_65",FL_65,0},
	{ "FL_66",FL_66,0},

	//FR
	{ "FR_00",FR_00,1},
	{ "FR_01",FR_01,1},
	{ "FR_02",FR_02,1},
	{ "FR_03",FR_03,1},
	{ "FR_04",FR_04,1},
	{ "FR_05",FR_05,1},
	{ "FR_06",FR_06,1},
	{ "FR_10",FR_10,1},
	{ "FR_11",FR_11,1},
	{ "FR_12",FR_12,1},
	{ "FR_13",FR_13,1},
	{ "FR_14",FR_14,1},
	{ "FR_15",FR_15,1},
	{ "FR_16",FR_16,1},
	{ "FR_20",FR_20,1},
	{ "FR_21",FR_21,1},
	{ "FR_22",FR_22,1},
	{ "FR_23",FR_23,1},
	{ "FR_24",FR_24,1},
	{ "FR_25",FR_25,1},
	{ "FR_26",FR_26,1},
	{ "FR_30",FR_30,1},
	{ "FR_31",FR_31,1},
	{ "FR_32",FR_32,1},
	{ "FR_33",FR_33,1},
	{ "FR_34",FR_34,1},
	{ "FR_35",FR_35,1},
	{ "FR_36",FR_36,1},
	{ "FR_40",FR_40,1},
	{ "FR_41",FR_41,1},
	{ "FR_42",FR_42,1},
	{ "FR_43",FR_43,1},
	{ "FR_44",FR_44,1},
	{ "FR_45",FR_45,1},
	{ "FR_46",FR_46,1},
	{ "FR_50",FR_50,1},
	{ "FR_51",FR_51,1},
	{ "FR_52",FR_52,1},
	{ "FR_53",FR_53,1},
	{ "FR_54",FR_54,1},
	{ "FR_55",FR_55,1},
	{ "FR_56",FR_56,1},
	{ "FR_60",FR_60,1},
	{ "FR_61",FR_61,1},
	{ "FR_62",FR_62,1},
	{ "FR_63",FR_63,1},
	{ "FR_64",FR_64,1},
	{ "FR_65",FR_65,1},
	{ "FR_66",FR_66,1},
	//RL
	{ "RL_00",RL_00,2},
	{ "RL_01",RL_01,2},
	{ "RL_02",RL_02,2},
	{ "RL_03",RL_03,2},
	{ "RL_04",RL_04,2},
	{ "RL_05",RL_05,2},
	{ "RL_06",RL_06,2},
	{ "RL_10",RL_10,2},
	{ "RL_11",RL_11,2},
	{ "RL_12",RL_12,2},
	{ "RL_13",RL_13,2},
	{ "RL_14",RL_14,2},
	{ "RL_15",RL_15,2},
	{ "RL_16",RL_16,2},
	{ "RL_20",RL_20,2},
	{ "RL_21",RL_21,2},
	{ "RL_22",RL_22,2},
	{ "RL_23",RL_23,2},
	{ "RL_24",RL_24,2},
	{ "RL_25",RL_25,2},
	{ "RL_26",RL_26,2},
	{ "RL_30",RL_30,2},
	{ "RL_31",RL_31,2},
	{ "RL_32",RL_32,2},
	{ "RL_33",RL_33,2},
	{ "RL_34",RL_34,2},
	{ "RL_35",RL_35,2},
	{ "RL_36",RL_36,2},
	{ "RL_40",RL_40,2},
	{ "RL_41",RL_41,2},
	{ "RL_42",RL_42,2},
	{ "RL_43",RL_43,2},
	{ "RL_44",RL_44,2},
	{ "RL_45",RL_45,2},
	{ "RL_46",RL_46,2},
	{ "RL_50",RL_50,2},
	{ "RL_51",RL_51,2},
	{ "RL_52",RL_52,2},
	{ "RL_53",RL_53,2},
	{ "RL_54",RL_54,2},
	{ "RL_55",RL_55,2},
	{ "RL_56",RL_56,2},
	{ "RL_60",RL_60,2},
	{ "RL_61",RL_61,2},
	{ "RL_62",RL_62,2},
	{ "RL_63",RL_63,2},
	{ "RL_64",RL_64,2},
	{ "RL_65",RL_65,2},
	{ "RL_66",RL_66,2},

	//RR
	{ "RR_00",RR_00,3},
	{ "RR_01",RR_01,3},
	{ "RR_02",RR_02,3},
	{ "RR_03",RR_03,3},
	{ "RR_04",RR_04,3},
	{ "RR_05",RR_05,3},
	{ "RR_06",RR_06,3},
	{ "RR_10",RR_10,3},
	{ "RR_11",RR_11,3},
	{ "RR_12",RR_12,3},
	{ "RR_13",RR_13,3},
	{ "RR_14",RR_14,3},
	{ "RR_15",RR_15,3},
	{ "RR_16",RR_16,3},
	{ "RR_20",RR_20,3},
	{ "RR_21",RR_21,3},
	{ "RR_22",RR_22,3},
	{ "RR_23",RR_23,3},
	{ "RR_24",RR_24,3},
	{ "RR_25",RR_25,3},
	{ "RR_26",RR_26,3},
	{ "RR_30",RR_30,3},
	{ "RR_31",RR_31,3},
	{ "RR_32",RR_32,3},
	{ "RR_33",RR_33,3},
	{ "RR_34",RR_34,3},
	{ "RR_35",RR_35,3},
	{ "RR_36",RR_36,3},
	{ "RR_40",RR_40,3},
	{ "RR_41",RR_41,3},
	{ "RR_42",RR_42,3},
	{ "RR_43",RR_43,3},
	{ "RR_44",RR_44,3},
	{ "RR_45",RR_45,3},
	{ "RR_46",RR_46,3},
	{ "RR_50",RR_50,3},
	{ "RR_51",RR_51,3},
	{ "RR_52",RR_52,3},
	{ "RR_53",RR_53,3},
	{ "RR_54",RR_54,3},
	{ "RR_55",RR_55,3},
	{ "RR_56",RR_56,3},
	{ "RR_60",RR_60,3},
	{ "RR_61",RR_61,3},
	{ "RR_62",RR_62,3},
	{ "RR_63",RR_63,3},
	{ "RR_64",RR_64,3},
	{ "RR_65",RR_65,3},
	{ "RR_66",RR_66,3},

};	

extern int dvr_detect_carback_signal(void); 
extern  int vbox_track_paint;

track_context *g_ptrack_context;
static user_header* g_pheader_buf;
static z_stream stream;
static int initialized;
unsigned int track_pic_id;
unsigned int radar_pic_id;
unsigned int car_pic_id;
unsigned int track2_pic_id;//small track for vbox
extern struct carback_context* g_carback_context;
extern int first_draw_track;

static user2_header* g_pheader2_buf;
track_context2 *g_ptrack_context2 = NULL;


static unsigned int get_time_ms(void)
{
    struct timeval tv;
    //float s;
    do_gettimeofday(&tv);
    //s = tv.tv_usec; s *= 0.000001; s += tv.tv_sec;
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void show_header_info(void)
{
	user_header* p = g_pheader_buf;

	if(p == NULL) {
		printk(KERN_ERR "header_info_show  error!\n");
		return;
	}

	ARKTRACK_DBGPRTK("header_buf ===>identity=0x%0x track_total=%d car_total=%d  signal_total= %d radar_total=%d file_type=0x%0x file_size=0x%0x\n",\
								p->identity,p->track_total,p->car_total,p->signal_total,p->radar_total,p->file_type,p->file_size);

	ARKTRACK_DBGPRTK("track rect ===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->track_rect.pos_x,\
								p->track_rect.pos_y,p->track_rect.width,p->track_rect.height);
	ARKTRACK_DBGPRTK("car rect   ===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->car_rect.pos_x,\
								p->car_rect.pos_y,p->car_rect.width,p->car_rect.height);
	ARKTRACK_DBGPRTK("signal rect   ===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->signal_rect.pos_x,\
								p->signal_rect.pos_y,p->signal_rect.width,p->signal_rect.height);
	ARKTRACK_DBGPRTK("radar0 rect===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->radar_rect[0].pos_x,\
								p->radar_rect[0].pos_y,p->radar_rect[0].width,p->radar_rect[0].height);
	ARKTRACK_DBGPRTK("radar1 rect===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->radar_rect[1].pos_x,\
								p->radar_rect[1].pos_y,p->radar_rect[1].width,p->radar_rect[1].height);
	ARKTRACK_DBGPRTK("radar2 rect===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->radar_rect[2].pos_x,\
								p->radar_rect[2].pos_y,p->radar_rect[2].width,p->radar_rect[2].height);
	ARKTRACK_DBGPRTK("radar3 rect===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->radar_rect[3].pos_x,\
								p->radar_rect[3].pos_y,p->radar_rect[3].width,p->radar_rect[3].height);
	ARKTRACK_DBGPRTK("track2 rect===> pos_x=%d  pos_y=%d  width=%d  height=%d\n",p->track2_rect.pos_x,\
								p->track2_rect.pos_y,p->track2_rect.width,p->track2_rect.height);

	ARKTRACK_DBGPRTK("track[0] image===> image_type=%d  image_store_id=%d  image_id=0x%0x  image_offset=0x%0x image_size=0x%0x\n",\
								p->track[0].image_type,p->track[0].image_store_id,p->track[0].image_id,p->track[0].image_offset,p->track[0].image_size);
 	ARKTRACK_DBGPRTK("car[0]   image===> image_type=%d  image_store_id=%d  image_id=0x%0x  image_offset=0x%0x image_size=0x%0x\n",\
								p->car[0].image_type,p->car[0].image_store_id,p->car[0].image_id,p->car[0].image_offset,p->car[0].image_size);
	ARKTRACK_DBGPRTK("signal[0]   image===> image_type=%d  image_store_id=%d  image_id=0x%0x  image_offset=0x%0x image_size=0x%0x\n",\
								p->signal[0].image_type,p->signal[0].image_store_id,p->signal[0].image_id,p->signal[0].image_offset,p->signal[0].image_size);
 	ARKTRACK_DBGPRTK("radar[1] image===> image_type=%d  image_store_id=%d  image_id=0x%0x  image_offset=0x%0x image_size=0x%0x\n",\
								p->radar[1].image_type,p->radar[1].image_store_id,p->radar[1].image_id,p->radar[1].image_offset,p->radar[1].image_size);
	ARKTRACK_DBGPRTK("track2[0]image===> image_type=%d  image_store_id=%d  image_id=0x%0x  image_offset=0x%0x image_size=0x%0x\n",\
								p->track2[0].image_type,p->track2[0].image_store_id,p->track2[0].image_id,p->track2[0].image_offset,p->track2[0].image_size);
}

static void set_rect_default(void)
{
	user_header* p = g_pheader_buf;

	p->car_total   = 0;
	p->radar_total = 0;

	p->track_rect.pos_x = 0;
	p->track_rect.pos_y = 0;
	p->track_rect.width = g_carback_context->screen_width;
	p->track_rect.height= g_carback_context->screen_height;

	p->car_rect.pos_x = 0;
	p->car_rect.pos_y = 0;
	p->car_rect.width = 0;
	p->car_rect.height= 0;	

	p->radar_rect[0].pos_x = 0;
	p->radar_rect[0].pos_y = 0;
	p->radar_rect[0].width = 0;
	p->radar_rect[0].height= 0;

	p->radar_rect[1].pos_x = 0;
	p->radar_rect[1].pos_y = 0;
	p->radar_rect[1].width = 0;
	p->radar_rect[1].height= 0;

	p->radar_rect[2].pos_x = 0;
	p->radar_rect[2].pos_y = 0;
	p->radar_rect[2].width = 0;
	p->radar_rect[2].height= 0;

	p->radar_rect[3].pos_x = 0;
	p->radar_rect[3].pos_y = 0;
	p->radar_rect[3].width = 0;
	p->radar_rect[3].height= 0;

	
	p->track2_rect.pos_x = 0;
	p->track2_rect.pos_y = 0;
	p->track2_rect.width = 0;
	p->track2_rect.height= 0;
	
}

static int check_header_info(void)
{
	int i;
	int fail = -1;
	user_header* p = g_pheader_buf;

	//check header info
	if(p->identity !=ARK_IDENTITY || p->track_total == 0 || p->file_size <= sizeof(user_header)){
		printk(KERN_ERR "identity info error!\n");
		return fail;
	}

	if(p->track_rect.width > g_carback_context->screen_width || p->track_rect.width == 0){
		printk(KERN_ERR "track rect width error!\n");
		return fail;
	}
	if(p->track_rect.height > g_carback_context->screen_height || p->track_rect.height == 0){
		printk(KERN_ERR "track rect height error!\n");
		return fail;
	}

	for(i = 0;i < TRACK_MAX; i++){
		if(TRACK_MAX < p->track[i].image_store_id){
			printk(KERN_ERR "track[%d].image_store_id error!\n",i);
			return fail;
		}
	}
	
	for(i = 0;i < CAR_MAX; i++){
		if(CAR_MAX < p->car[i].image_store_id){
			printk(KERN_ERR "track[%d].image_store_id error!\n",i);
			return fail;
		}
	}
	
	for(i = 0;i < p->radar_total; i++){
		if(RADAR_MAX <  p->radar[i].image_store_id){
			printk(KERN_ERR "track[%d].image_store_id error!\n",i);
			return fail;
		}
	}
	
	for(i = 0;i < TRACK_MAX; i++){
		if(TRACK_MAX < p->track2[i].image_store_id){
			printk(KERN_ERR "track2[%d].image_store_id error!\n",i);
			return fail;
		}
	}

	return 0;
}

static bool is_radar_image_id(unsigned int image_id){
	int i;
	for(i = 0;i < RADAR_MAX; i++){
		if(radar_idmap[i].image_id == image_id){
			return true;
		}
		
	}
	
	return false;
}

int set_disp_track_id(unsigned int track_id)
{
	static unsigned int pre_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;

	if(p == NULL){
		printk(KERN_ALERT "%s: g_ptrack_context NULL, error!\n", __FUNCTION__);
		return -1;
	}
	
	if(track_id >= TRACK_MAX && track_id < IMAGE_ID_NONE){
		printk(KERN_ERR "set disp track image id error!\n" );
		return -1;
	}
	
	if(pre_id != track_id || IMAGE_ID_NONE == track_id){
		p->disp_track_id = track_id;
		pre_id = track_id;
	}

	return 0;
}

int set_disp_car_id(unsigned int car_id)
{
	static unsigned int pre_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;

	if(p == NULL){
		printk(KERN_ALERT "%s: g_ptrack_context NULL, error!\n", __FUNCTION__);
		return -1;
	}

	if(car_id >= CAR_MAX && car_id < IMAGE_ID_NONE){
		printk(KERN_ERR "set disp car image id error!\n" );
		return -1;
	}
	
	if(pre_id != car_id || IMAGE_ID_NONE == car_id){
		p->disp_car_id = car_id;
		pre_id = car_id;
	}
	
	return 0;
}

int set_disp_radar_id(unsigned int radar_id)
{
	static unsigned int pre_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
	unsigned int image_id;
	unsigned char channel;
	unsigned int mask = 0xff000000;
	unsigned int disp_id = 0;
        
	if(p == NULL){
		printk(KERN_ALERT "%s: g_ptrack_context NULL, error!\n", __FUNCTION__);
		return -1;
	}

	if(IMAGE_ID_NONE == radar_id){
		p->disp_radar_id = IMAGE_ID_NONE;
		pre_id = IMAGE_ID_NONE;
		return 0;
	}
	
	if(pre_id == radar_id){
		return 0;
	}
	
	for(channel = RADAR_CHANNEL_FL; channel <= RADAR_CHANNEL_RR;channel++){
		
		image_id = radar_id & (mask >> 8*channel);
		if(!is_radar_image_id(image_id)){
			printk(KERN_ERR "set disp radar id channel=%d ,error!\n",channel);
			continue;
		}
		
		disp_id |= image_id;
	}

	p->disp_radar_id = disp_id;
	pre_id = disp_id;
	//printk(KERN_ALERT "set disp radar id disp_id=0x%0x.\n",disp_id);
	
	return 0;
}

int set_disp_signal_id(unsigned int disp_signal_id)
{
	static unsigned int pre_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;

	if(p == NULL){
		printk(KERN_ALERT "%s: g_ptrack_context NULL, error!\n", __FUNCTION__);
		return -1;
	}

	if(disp_signal_id >= SIGNAL_MAX && disp_signal_id < IMAGE_ID_NONE){
		printk(KERN_ERR "set disp signal image id error!\n" );
		return -1;
	}
	
	if(pre_id != disp_signal_id || IMAGE_ID_NONE == disp_signal_id){
		p->disp_signal_id = disp_signal_id;
		pre_id = disp_signal_id;
	}
	//printk(KERN_ALERT "++++++p->disp_signal_id = %d\n" ,p->disp_signal_id);
	return 0;
}
int set_disp_track2_id(unsigned int track2_id)
{
	static unsigned int pre_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
        
	if(p == NULL){
		printk(KERN_ALERT "%s: g_ptrack_context NULL, error!\n", __FUNCTION__);
		return -1;
	}
	
	if(track2_id >= TRACK_MAX && track2_id < IMAGE_ID_NONE){
		printk(KERN_ERR "set disp track2 image id error!\n" );
		return -1;
	}
	
	if(pre_id != track2_id || IMAGE_ID_NONE == track2_id){
		p->disp_track2_id = track2_id;
		pre_id = track2_id;
	}

	return 0;
}
/*
*set_disp_mradar_id:   radar new solution
*
*-------------------------front-----------------------
*		
*	           mradar0 mradar1 mradar2 mradar3
*				+-----------------+
*				|                             |
*				|                             |
*		mradar8	|                             | mradar12
*				|                             | 
*		 mradar9 |                             | mradar13
*				|                             |
*	       mradar10|                             | mradar14
*				|                             |
*	       mradar11|                             | mradar15
*				|                             |
*				|                             |
*				+-----------------+
*                 mradar4 mradar5 mradar6 mradar7
*			  
*-------------------------rear-----------------------
*/
int set_disp_mradar_id(unsigned char *pimage_id)
{

	static unsigned char pre_mradar_id[MRADAR_MAX]={0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF};	
	unsigned char mradar_id[MRADAR_MAX];
	track_context2 *p = g_ptrack_context2;
	int i;

	if(p == NULL){
		//printk(KERN_ALERT "%s: no header2 ,set mradar id invalid!\n", __FUNCTION__);
		return 0;
	}

	memcpy(mradar_id, pimage_id, MRADAR_MAX);	
	for(i = 0; i < MRADAR_MAX; i++){
		ARKTRACK_DBGPRTK("mradar_id[%d]=%d\n", i, mradar_id[i]);
		if(mradar_id[i] > 0xF){
			printk(KERN_ALERT "%s: mradar_id[%d]=%d, MRADAR_MAX=0xF, error.\n", __FUNCTION__, i, mradar_id[i]);
			return -1;
		}
	}
	
	for(i = 0; i < MRADAR_MAX; i++){
		if(pre_mradar_id[i] != mradar_id[i]){
			break;
		}
	}

	if(i < MRADAR_MAX){
		memcpy(p->disp_mradar_id, mradar_id, MRADAR_MAX);
		memcpy(pre_mradar_id, mradar_id, MRADAR_MAX);
		p->disp_mradar_id_change = 1;
	}

	return 0;
}

static int init_track_context(unsigned int addr)
{
	int ret = -1;
	int size;
	int i;
	unsigned char *p;
	//printk(KERN_ALERT "init_track_context      ...\n");

	if(addr == 0){ 
		printk(KERN_ERR "addr error!\n");
		return ret;
	}
	
	g_pheader_buf = (user_header*)addr;

	show_header_info();

	if(check_header_info < 0){

		printk(KERN_ERR "identity info error!\n");
		return ret;
	}
	
	p = vzalloc(sizeof(track_context));
	if (p == NULL) {
		printk(KERN_ERR "%s %d: failed to allocate track_context\n",__FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	g_ptrack_context = (track_context *)p;
	g_ptrack_context->disp_track_size = 4 * g_pheader_buf->track_rect.width * g_pheader_buf->track_rect.height;

	if(g_pheader_buf->track_total > 0){
		size = 4* g_pheader_buf->track_rect.width * g_pheader_buf->track_rect.height;
		p = vzalloc(size);
		if (p == NULL) {
			printk(KERN_ERR "%s %d: failed to allocate track\n",__FUNCTION__, __LINE__);
			ret = -ENOMEM;
			goto err_track2;
		}
		g_ptrack_context->disp_track_buf = p;
		g_ptrack_context->disp_track_size= size;
	}

	if(g_pheader_buf->car_total > 0){
		size = 4* g_pheader_buf->car_rect.width * g_pheader_buf->car_rect.height;
		p = vzalloc(size);
		if (p == NULL) {
			printk(KERN_ERR "%s %d: failed to allocate car\n",__FUNCTION__, __LINE__);
			ret = -ENOMEM;
			goto err_car;
		}
		g_ptrack_context->disp_car_buf = p;
		g_ptrack_context->disp_car_size= size;
	}

	if(g_pheader_buf->signal_total> 0){
		size = 4* g_pheader_buf->signal_rect.width * g_pheader_buf->signal_rect.height;
		p = vzalloc(size);
		if (p == NULL) {
			printk(KERN_ERR "%s %d: failed to allocate car\n",__FUNCTION__, __LINE__);
			ret = -ENOMEM;
			goto err_car;
		}
		g_ptrack_context->disp_signal_buf = p;
		g_ptrack_context->disp_signal_size= size;
	}

	if(g_pheader_buf->radar_total > 0){
		size = 4* g_pheader_buf->radar_rect[0].width * g_pheader_buf->radar_rect[0].height;
		for(i = 0; i < 4;i++){
			p = vzalloc(size);
			if (p == NULL) {
				printk(KERN_ERR "%s %d: failed to allocate radar\n",__FUNCTION__, __LINE__);
				ret = -ENOMEM;
				while(i-- >= 0){
					vfree(g_ptrack_context->disp_radar_buf[i]);
				}
				goto err_radar;
			}
			g_ptrack_context->disp_radar_buf[i] = p;
			g_ptrack_context->disp_radar_size   = size;
		}
	}

	if(g_pheader_buf->track2_total > 0){
		size = 4* g_pheader_buf->track2_rect.width * g_pheader_buf->track2_rect.height;
		p = vzalloc(size);
		if (p == NULL) {
			printk(KERN_ERR "%s %d: failed to allocate track2\n",__FUNCTION__, __LINE__);
			ret = -ENOMEM;
			goto err_track2;
		}
		g_ptrack_context->disp_track2_buf = p;
		g_ptrack_context->disp_track2_size= size;
	}

	g_ptrack_context->pheader_buf = g_pheader_buf;
	g_ptrack_context->disp_xpos   = g_pheader_buf->track_rect.pos_x;
	g_ptrack_context->disp_ypos   = g_pheader_buf->track_rect.pos_y;
	g_ptrack_context->disp_width  = g_carback_context->screen_width;
	g_ptrack_context->disp_height = g_carback_context->screen_height;
	memcpy(&g_ptrack_context->track_param,&g_pheader_buf->track_rect,sizeof(track_param_context));
	g_ptrack_context->file_type   = g_pheader_buf->file_type;
    
	g_ptrack_context->disp_track_id = IMAGE_ID_NONE;
	g_ptrack_context->disp_car_id   = IMAGE_ID_NONE;
	g_ptrack_context->disp_radar_id = IMAGE_ID_NONE;
	g_ptrack_context->disp_track2_id= IMAGE_ID_NONE;

	return 0;
	
err_track2:
	for(i = 0; i < 4;i++)
		if(g_ptrack_context->disp_radar_buf[i])
			vfree(g_ptrack_context->disp_radar_buf[i]);
err_radar:
	if(g_ptrack_context->disp_car_buf)
		vfree(g_ptrack_context->disp_car_buf);
err_car:
	if(g_ptrack_context)
		vfree(g_ptrack_context);
	
	return ret;

}


static void show_header2_info(void)
{
	user2_header* p = g_pheader2_buf;
	rect_info *prect;
	item_info *pitem;
	int i;

	if(p == NULL) {
		printk(KERN_ERR "header_info_show error!\n");
		return;
	}

	ARKTRACK_DBGPRTK("header2_buf ===>identity=0x%0x,file_type=0x%0x file_size=0x%0x\n", p->identity,p->file_type,p->file_size);
	for(i = 0; i < MRADAR_MAX; i++){
		prect = &p->mradar_rect[i];
		pitem = &p->mradar[i][0];
		ARKTRACK_DBGPRTK("mradar_rect[%d]===>total=%d,  pos_x=%d  pos_y=%d  width=%d  height=%d\n",\
		                 i, p->mradar_total[i], prect->pos_x, prect->pos_y, prect->width, prect->height);
		ARKTRACK_DBGPRTK("mradar[%d][0]  ===> image_type=%d  image_store_id=%d  image_id=0x%0x  image_offset=0x%0x  image_size=0x%0x\n",\
		       i, pitem->image_type, pitem->image_store_id, pitem->image_id, pitem->image_offset, pitem->image_size);
	}
}

static int check_header2_info(void)
{
	int i, j;
	user2_header* p = g_pheader2_buf;

	if(p == NULL) {
		printk(KERN_ERR "check_header2_info error!\n");
		return -1;
	}

	//check header info
	if(p->identity != ARK_IDENTITY || p->file_size <= sizeof(user2_header)){
		printk(KERN_ERR "header2 identity info error!\n");
		return -1;
	}

	for(j = 0 ;j < MRADAR_MAX; j++){
		for(i = 0;i < MRADAR_ITEM_MAX; i++){
			if(MRADAR_MAX <  p->mradar[j][i].image_id){
				printk(KERN_ALERT "track[%d].image_id error!\n",i);
				return -1;
			}
		}
	}
	
	printk(KERN_ALERT "check_header2_info success!\n");
	return 0;
}

static int init_track_context2(unsigned int addr)
{
	unsigned int header2_addr_start;
	unsigned char *p;
	int ret = -1;
	int size;
	int i;

	//printk(KERN_ALERT "file_type=%0x  ---> HEADER2_FILE_FLAG=%0x.\n", g_pheader_buf->file_type, HEADER2_FILE_FLAG);
	if((g_pheader_buf->file_type & 0x80000000) != HEADER2_FILE_FLAG){
		printk(KERN_ALERT "only header file.\n");
		return 0;
	}

	printk(KERN_ALERT "%s: start.\n", __func__);
	
	if(addr == 0){ 
		printk(KERN_ERR "addr error!\n");
		return -1;
	}

	
	//g_pheader2_buf = (user2_header*)(addr + g_pheader_buf->file_size);
	p = vzalloc(sizeof(user2_header));
	if (p == NULL) {
		printk(KERN_ERR "%s: failed to user2_header.\n",__FUNCTION__);
		return -1;
	}
	header2_addr_start = addr + g_pheader_buf->file_size;
	printk(KERN_ALERT "header2_addr_start=0x%0x.\n", header2_addr_start);
	memcpy(p, (void *)header2_addr_start, sizeof(user2_header));
	g_pheader2_buf = (user2_header*)p;
	
	show_header2_info();
	if(check_header2_info() < 0){
		printk(KERN_ALERT "check_header2_info failed!\n");
		return -1;
	}

	p = vzalloc(sizeof(track_context2));
	if (p == NULL) {
		printk(KERN_ERR "%s %d: failed to allocate track_context2\n",__FUNCTION__, __LINE__);
		return -1;
	}
	memset(p, 0 ,sizeof(track_context2));
	g_ptrack_context2                          = (track_context2 *)p;
	g_ptrack_context2->pheader2_buf            = g_pheader2_buf;
	g_ptrack_context2->file_type               = g_pheader2_buf->file_type;
	memset(g_ptrack_context2->disp_mradar_id, 0x0F ,MRADAR_MAX);
	
	for(i = 0; i < MRADAR_MAX; i++){
		size = 4* g_pheader2_buf->mradar_rect[i].width * g_pheader2_buf->mradar_rect[i].height;
		p = vzalloc(size);
		if (p == NULL) {
			printk(KERN_ERR "%s %d: failed to allocate radar\n",__FUNCTION__, __LINE__);
			ret = -ENOMEM;
			while(i-- >= 0){
				vfree(g_ptrack_context2->disp_mradar_buf[i]);
			}
			vfree(g_pheader2_buf);
			return -1;
		}
		g_ptrack_context2->disp_mradar_buf[i] = p;
		g_ptrack_context2->disp_mradar_size[i]= size;
		memcpy(&g_ptrack_context2->mradar_param.mradar_rect[i], &g_pheader2_buf->mradar_rect[i], sizeof(rect_info));
		ARKTRACK_DBGPRTK("mradar_buf[%d]=0x%0x, size=0x%0x \n",i, p, size);
	}
		
	printk(KERN_ALERT "%s: success.\n", __func__);

	return 0;
}

static int get_radar_channel(unsigned int image_id){
	int i;
	int channel = -1;
	for(i = 0;i < RADAR_MAX; i++){
		if(radar_idmap[i].image_id == image_id){
			channel = radar_idmap[i].dis_channel;
			break;
		}
		
	}
	
	return channel;

}
static unsigned int get_picture_addr(enum image_type type, unsigned int id)
{
	unsigned int offset;
	int i;
	
	if(type == IMAGE_TYPE_TRACK){
		offset = g_pheader_buf->track[id].image_offset;
	}
	else if(type == IMAGE_TYPE_CAR){
		offset = g_pheader_buf->car[id].image_offset;
	}
	else if(type == IMAGE_TYPE_SIGNAL){
		offset = g_pheader_buf->signal[id].image_offset;
	}
	else if(type == IMAGE_TYPE_RADAR){
		for(i = 0; i < RADAR_MAX;i++){
			if(g_pheader_buf->radar[i].image_id == id){
				offset = g_pheader_buf->radar[i].image_offset;
				break;
			}

		}
	}
	else if(type == IMAGE_TYPE_TRACK2){
			offset = g_pheader_buf->track2[id].image_offset;
	}

	return g_carback_context->track_data_virtaddr +offset;
}

static unsigned int get_picture_offset(enum image_type type, unsigned int id)
{
	unsigned int offset = 0;
	int i;
	
	if(type == IMAGE_TYPE_TRACK){
		offset = g_pheader_buf->track[id].image_offset;
	}
	else if(type == IMAGE_TYPE_CAR){
		offset = g_pheader_buf->car[id].image_offset;
	}
	else if(type == IMAGE_TYPE_SIGNAL){
		offset = g_pheader_buf->signal[id].image_offset;
	}
	else if(type == IMAGE_TYPE_RADAR){
		for(i = 0; i < RADAR_MAX;i++){
			if(g_pheader_buf->radar[i].image_id == id){
				offset = g_pheader_buf->radar[i].image_offset;
				break;
			}

		}
	}
	else if(type == IMAGE_TYPE_TRACK2){
			offset = g_pheader_buf->track2[id].image_offset;
	}

	return offset;
}


static unsigned int get_picture_size(enum image_type type, unsigned int id)
{
	unsigned int image_size = 0;
	int i;
	
	if(type == IMAGE_TYPE_TRACK){
		image_size = g_pheader_buf->track[id].image_size;
	}
	else if(type == IMAGE_TYPE_CAR){
		image_size = g_pheader_buf->car[id].image_size;
	}
	else if(type == IMAGE_TYPE_SIGNAL){
		image_size = g_pheader_buf->signal[id].image_size;
	}
	else if(type == IMAGE_TYPE_RADAR){
		for(i = 0; i < RADAR_MAX;i++){
			if(g_pheader_buf->radar[i].image_id == id){
				image_size = g_pheader_buf->radar[i].image_size;
				break;
			}

		}
	}
	else if(type == IMAGE_TYPE_TRACK2){
		image_size = g_pheader_buf->track2[id].image_size;
	}
	return image_size;
}

static unsigned int get_picture_num(enum image_type type)
{
	unsigned int picture_num = 0;

	if(type == IMAGE_TYPE_TRACK){
		picture_num = g_pheader_buf->track_total;
	}
	else if(type == IMAGE_TYPE_CAR){
		picture_num = g_pheader_buf->car_total;
	}
	else if(type == IMAGE_TYPE_RADAR){
		picture_num = g_pheader_buf->radar_total;
	}
	else if(type == IMAGE_TYPE_TRACK2){
		picture_num = g_pheader_buf->track2_total;
	}
	//printk("get image_type = %d, num = %d !",type,picture_num);
	return picture_num;
}


/* Returns length of decompressed data. */
static int zlib_uncompress_block(void *dst, int dstlen, void *src, int srclen)
{
	int err;

	stream.next_in = src;
	stream.avail_in = srclen;

	stream.next_out = dst;
	stream.avail_out = dstlen;

	err = zlib_inflateReset(&stream);
	if (err != Z_OK) {
		printk("zlib_inflateReset error %d\n", err);
		zlib_inflateEnd(&stream);
		zlib_inflateInit(&stream);
	}

	err = zlib_inflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END)
		goto err;
	return stream.total_out;

err:
	printk(KERN_ERR "Error %d while decompressing!\n", err);
	printk(KERN_ERR "%p(%d)->%p(%d)\n", src, srclen, dst, dstlen);
	return -EIO;
}

static int zlib_uncompress_init(void)
{
	if (!initialized++) {
		stream.workspace = vmalloc(zlib_inflate_workspacesize());
		if ( !stream.workspace ) {
			initialized = 0;
			return -ENOMEM;
		}
		stream.next_in = NULL;
		stream.avail_in = 0;
		zlib_inflateInit(&stream);
	}
	return 0;
}

static void zlib_uncompress_exit(void)
{
	if (!--initialized) {
		zlib_inflateEnd(&stream);
		vfree(stream.workspace);
	}
}

static void copy_line_data(unsigned int *dest,unsigned int *src,unsigned int width,enum cover_type cover)
{
	unsigned int i;
	unsigned int* psrc  = (unsigned int*)src;
	unsigned int* pdest = (unsigned int*)dest;
	
	if(cover == SRC_COVER_DST){	
		memcpy(pdest,psrc,width*PIXEL_DATA_SIZE);
	}else if(cover == DST_COVER_SRC2){
		for(i = 0;i < width;i++){
			if(*pdest == PIXEL_DATA_NONE || *pdest == PIXEL_DATA_ZERO) 
				*pdest = *psrc;
			
			psrc++;
			pdest++;
		}
	}
	else if(cover == SRC_COVER_DST2){
		for(i = 0;i < width;i++){
			if(*psrc == 0xFFFFFFFF || *psrc == PIXEL_DATA_NONE || *psrc == PIXEL_DATA_ZERO){
				psrc++;
				pdest++;
				continue;
			} 
			*pdest = *psrc;
			psrc++;
			pdest++;
		}
	}
}

static int copy_pic_data(void *dest,void *src,enum image_type type,unsigned int id,enum cover_type cover)
{
	unsigned int width;
	unsigned int height;
	unsigned int pos_x;
	unsigned int pos_y;
	track_param_context* p = &g_ptrack_context->track_param;
	unsigned int* dest_tmp;	
	unsigned int* src_tmp;	
	int radar_channel;
	int i;
	
	if(dest == NULL || src == NULL){
		printk(KERN_ERR "paint car dest src error!\n");
		return -1;
	}

	if(type == IMAGE_TYPE_TRACK){
		width = p->track_rect.width;
		height= p->track_rect.height;
		pos_x = p->track_rect.pos_x;
		pos_y = p->track_rect.pos_y;	
	}
	else if(type == IMAGE_TYPE_CAR){
		
		width = p->car_rect.width;
		height= p->car_rect.height;
		pos_x = p->car_rect.pos_x;
		pos_y = p->car_rect.pos_y;	
	}
	else if(type == IMAGE_TYPE_RADAR){
		
		radar_channel = get_radar_channel(id);
		if(radar_channel < 0){
			printk(KERN_ERR "get radar_channel error!\n");
			return -1;
		}
		width = p->radar_rect[radar_channel].width;
		height= p->radar_rect[radar_channel].height;
		pos_x = p->radar_rect[radar_channel].pos_x;
		pos_y = p->radar_rect[radar_channel].pos_y;
	}
	else if(type == IMAGE_TYPE_SIGNAL){
		width = p->signal_rect.width;
		height= p->signal_rect.height;
		pos_x = p->signal_rect.pos_x;
		pos_y = p->signal_rect.pos_y;	
	}
	else if(type == IMAGE_TYPE_TRACK2){
		
		width = p->track2_rect.width;
		height= p->track2_rect.height;
		pos_x = p->track2_rect.pos_x;
		pos_y = p->track2_rect.pos_y;	
	}
	else if(type == IMAGE_TYPE_MRADAR){
		mradar_param_context* p2 = &g_ptrack_context2->mradar_param;
		width = p2->mradar_rect[id].width;
		height= p2->mradar_rect[id].height;
		pos_x = p2->mradar_rect[id].pos_x;
		pos_y = p2->mradar_rect[id].pos_y;	
	}
	else{
		printk(KERN_ERR "paint car type error!\n");
		return -1;
	}
	
	for(i = 0; i < height;i++){
		dest_tmp = dest + g_ptrack_context->disp_width * (pos_y + i) * PIXEL_DATA_SIZE + pos_x * PIXEL_DATA_SIZE;
		src_tmp  = src + i * width * PIXEL_DATA_SIZE;
		copy_line_data(dest_tmp,src_tmp,width,cover);
	}
	
	return 0;
}

static unsigned int subjoin_car_pic(void *dest)
{	
	static unsigned int last_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
	void *dest_car = p->disp_car_buf;
	unsigned int dest_size  = p->disp_car_size;
	unsigned int fill_size = 0;
	unsigned int source_size;
	unsigned int source_offset;
	void *source;
		
	if(p->disp_car_id  > CAR_MAX)
		return 0;
	
	if(first_draw_track) 
		last_id = IMAGE_ID_NONE;
		
	source_offset = get_picture_offset(IMAGE_TYPE_CAR,p->disp_car_id);
	source_size   = get_picture_size(IMAGE_TYPE_CAR,p->disp_car_id);
	//printk(KERN_ALERT "source_offset=%0x ,source_size=%0x\n",source_offset,source_size );
	if(source_size == 0 || source_offset < sizeof(user_header)){
		printk(KERN_ERR "subjoin car pic source error, disp_car_id=0x%0x.\n",p->disp_car_id);
		return 0;
	}

	if(last_id == IMAGE_ID_NONE || last_id != p->disp_car_id){
		source = (void*)get_picture_addr(IMAGE_TYPE_CAR,p->disp_car_id);
		fill_size = zlib_uncompress_block(dest_car, dest_size, source, source_size);
		if(fill_size != dest_size){
			printk(KERN_ERR " zlib uncompress car error!\n");
			return fill_size;
		}
	}
	
	copy_pic_data(dest,dest_car,IMAGE_TYPE_CAR,p->disp_car_id,SRC_COVER_DST);
	last_id = p->disp_car_id;

	return fill_size;
}

static unsigned int subjoin_signal_pic(void *dest)
{	
	static unsigned int last_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
	void *dest_signal = p->disp_signal_buf;
	unsigned int dest_size  = p->disp_signal_size;
	unsigned int fill_size = 0;
	unsigned int source_size;
	unsigned int source_offset;
	void *source;
 
	if(p->disp_signal_id  > SIGNAL_MAX)
		return 0;
	
	if(first_draw_track) 
		last_id = IMAGE_ID_NONE;
		
	source_offset = get_picture_offset(IMAGE_TYPE_SIGNAL,p->disp_signal_id);
	source_size   = get_picture_size(IMAGE_TYPE_SIGNAL,p->disp_signal_id);
	//printk(KERN_ALERT "~~~~~~source_offset=%0x ,source_size=%0x\n",source_offset,source_size );
	if(source_size == 0 || source_offset < sizeof(user_header)){
		printk(KERN_ERR "subjoin signal pic source error, disp_signal_id=0x%0x.\n",p->disp_signal_id);
		return 0;
	}

	if(last_id == IMAGE_ID_NONE || last_id != p->disp_signal_id){
		source = (void*)get_picture_addr(IMAGE_TYPE_SIGNAL,p->disp_signal_id);
		fill_size = zlib_uncompress_block(dest_signal, dest_size, source, source_size);
		if(fill_size != dest_size){
			printk(KERN_ERR " zlib uncompress signal error!\n");
			return fill_size;
		}
	}
	
	copy_pic_data(dest,dest_signal,IMAGE_TYPE_SIGNAL,p->disp_signal_id,SRC_COVER_DST);
	last_id = p->disp_signal_id; 

	return fill_size;
}

static unsigned int subjoin_track_pic(void *dest)
{	
	static unsigned int last_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
	void *dest_track = p->disp_track_buf;
	unsigned int dest_size  = p->disp_track_size;
	unsigned int fill_size = 0;
	unsigned int source_size;
	unsigned int source_offset;
	void *source;
		
	if(p->disp_track_id  > TRACK_MAX)
		return 0;
	
	if(first_draw_track) 
		last_id = IMAGE_ID_NONE;
	
	source_offset = get_picture_offset(IMAGE_TYPE_TRACK,p->disp_track_id);
	source_size   = get_picture_size(IMAGE_TYPE_TRACK,p->disp_track_id);
	if(source_size == 0 || source_offset < sizeof(user_header)){
		printk(KERN_ERR "subjoin track pic source error, disp_track_id=0x%0x.\n",p->disp_track_id);
		return 0;
	}
	
	if(last_id == IMAGE_ID_NONE || last_id != p->disp_track_id){
		source = (void*)get_picture_addr(IMAGE_TYPE_TRACK,p->disp_track_id);
		fill_size = zlib_uncompress_block(dest_track, dest_size, source, source_size);
		if(fill_size != dest_size){
			printk(KERN_ERR " zlib uncompress track error!\n");
			return fill_size;
		}
	}
	
	copy_pic_data(dest,dest_track,IMAGE_TYPE_TRACK,p->disp_track_id,SRC_COVER_DST);
	last_id = p->disp_track_id;

	//printk(KERN_ALERT "p->disp_track_id = %d",p->disp_track_id);
	return fill_size;
}

static unsigned int subjoin_radar_pic(void *dest)
{	
	static unsigned int last_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
	void *dest_radar = p->disp_radar_buf[0];
	unsigned int dest_size  = p->disp_radar_size;
	unsigned int mask = 0xff000000;
	unsigned int image_id;
	unsigned char channel;
	unsigned int fill_size = 0;
	unsigned int source_size;
	unsigned int source_offset;
	void *source;

	if(p->disp_radar_id == IMAGE_ID_NONE)
		return 0;
	
	if(first_draw_track) 
		last_id = IMAGE_ID_NONE;
			
	for(channel = RADAR_CHANNEL_FL; channel <= RADAR_CHANNEL_RR;channel++){
		image_id = p->disp_radar_id & (mask >> 8*channel);	
		if(!is_radar_image_id(image_id)){
			printk(KERN_ERR " subjoin radar id channel=%d image_id=0x%0x, error!\n",channel,image_id);
			continue;
		}

		dest_radar = p->disp_radar_buf[channel];
		source_size   = get_picture_size(IMAGE_TYPE_RADAR,image_id);
		source_offset = get_picture_offset(IMAGE_TYPE_RADAR,image_id);
		//printk(KERN_ALERT "channel=%d image_id=0x%0x ---> source_offset=%0x ,source_size=%0x \n",channel,image_id,source_offset,source_size );
		if(source_size == 0 || source_offset < sizeof(user_header)){
			printk(KERN_ERR "subjoin radar pic source error,channel=%d image_id=0x%0x.\n",channel,image_id);
			continue;
		}
		
		if(last_id == IMAGE_ID_NONE || last_id != p->disp_radar_id){
			source = (void*)get_picture_addr(IMAGE_TYPE_RADAR,image_id);
			fill_size = zlib_uncompress_block(dest_radar, dest_size, source, source_size);
			if(fill_size != dest_size){
				printk(KERN_ERR " zlib uncompress radar error!\n");
				return fill_size;
			}
		}
		
		if(vbox_track_paint){
			copy_pic_data(dest,dest_radar,IMAGE_TYPE_RADAR,image_id,SRC_COVER_DST2);//wheel paint
		}else{
			if(channel == RADAR_CHANNEL_RR){
				copy_pic_data(dest,dest_radar,IMAGE_TYPE_RADAR,image_id,SRC_COVER_DST);
				//copy_pic_data(dest,dest_radar,IMAGE_TYPE_RADAR,image_id,DST_COVER_SRC2);
			}
			else{
				copy_pic_data(dest,dest_radar,IMAGE_TYPE_RADAR,image_id,SRC_COVER_DST);
			}
		}
		
	}
	
	last_id = p->disp_radar_id;

	return fill_size;
}

static unsigned int subjoin_track2_pic(void *dest)
{	
	static unsigned int last_id = IMAGE_ID_NONE;
	track_context *p = g_ptrack_context;
	void *dest_track2 = p->disp_track2_buf;
	unsigned int dest_size  = p->disp_track2_size;
	unsigned int fill_size = 0;
	unsigned int source_size;
	unsigned int source_offset;
	void *source;
		
	if(p->disp_track2_id  > TRACK_MAX)
		return 0;
	
	if(first_draw_track) 
		last_id = IMAGE_ID_NONE;
	
	source_offset = get_picture_offset(IMAGE_TYPE_TRACK2,p->disp_track2_id);
	source_size   = get_picture_size(IMAGE_TYPE_TRACK2,p->disp_track2_id);
	if(source_size == 0 || source_offset < sizeof(user_header)){
		printk(KERN_ERR "subjoin track2 pic source error, disp_track2_id=0x%0x.\n",p->disp_track2_id);
		return 0;
	}
	
	if(last_id == IMAGE_ID_NONE || last_id != p->disp_radar_id){
		source = (void*)get_picture_addr(IMAGE_TYPE_TRACK2,p->disp_track2_id);
		fill_size = zlib_uncompress_block(dest_track2, dest_size, source, source_size);
		if(fill_size != dest_size){
			printk(KERN_ERR " zlib uncompress track2 error!\n");
			return fill_size;
		}
	}
	
	copy_pic_data(dest,dest_track2,IMAGE_TYPE_TRACK2,p->disp_track2_id,SRC_COVER_DST);
	last_id = p->disp_track2_id;
	
	return fill_size;
}

static unsigned int subjoin_mradar_pic(void *dest)
{	
	static unsigned char last_mradar_id[MRADAR_MAX]={0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF,0xF};
	track_context2 *p = g_ptrack_context2;
	void *dest_buf;
	unsigned int dest_size;
	unsigned int fill_size = 0;
	unsigned int source_size;
	unsigned int source_offset;
	void *source;
	int i,item_id;
		
	if(p == NULL)
		return -1;
	
	if(first_draw_track) {
		for(i = 0; i < MRADAR_MAX; i++){
			last_mradar_id[i] = 0x0F;
		}
	}

	for(i = 0; i < MRADAR_MAX; i++){
		
		if(p->disp_mradar_id[i] == 0x0F)
			continue;
		
		item_id       = p->disp_mradar_id[i];
		dest_buf      = p->disp_mradar_buf[i];
		dest_size     = p->disp_mradar_size[i];
		source_offset = g_pheader2_buf->mradar[i][item_id].image_offset + g_pheader_buf->file_size;
		source_size   = g_pheader2_buf->mradar[i][item_id].image_size;
		//printk(KERN_ALERT "mradar[%d][%d]: source_offset=0x%0x ,source_size=0x%0x \n",i,item_id,source_offset,source_size);

		if(source_size == 0 || source_offset < sizeof(user2_header)){
			//printk(KERN_ERR "subjoin mradar pic source error, mradar[%d][%d].\n",i, item_id);
			continue;
		}
		
		if(last_mradar_id[i] == 0x0F || last_mradar_id[i] != p->disp_mradar_id[i]){  
			source =(void*)(g_carback_context->track_data_virtaddr+source_offset);
			fill_size = zlib_uncompress_block(dest_buf, dest_size, source, source_size);
			if(fill_size != dest_size){
				printk(KERN_ERR " zlib uncompress mradar error, fill_size=%0x, dest_size=%0x.\n", fill_size, dest_size);
				return fill_size;
			}
		}
		
		copy_pic_data(dest, dest_buf, IMAGE_TYPE_MRADAR, i, SRC_COVER_DST2);
	
	}
	
	for(i = 0; i < MRADAR_MAX; i++)
		last_mradar_id[i] = p->disp_mradar_id[i];
	
	p->disp_mradar_id_change = 0;
	
	return fill_size;
}


int track_paint_init(void)
{ 
	int ret = 0;
	if(zlib_uncompress_init()){
		printk(KERN_ERR "zlib uncompress init failed!\n" );
		return -1;
	}
	
    if (*(volatile unsigned int*)g_carback_context->track_data_virtaddr != MKTAG('R', 'S', 'T', 'K')) {
            printk(KERN_ALERT "reservingtrack check failed!\n" );
            return -1;
    }
	
	if(init_track_context(g_carback_context->track_data_virtaddr) < 0){
        printk(KERN_ERR "init track context failed.\n" );
		return -1;
	}
	if(init_track_context2(g_carback_context->track_data_virtaddr) < 0){
        printk(KERN_ERR "init track context2 failed.\n" );
		g_ptrack_context2 = NULL;
		return -1;
	}

	if(g_carback_context && g_carback_context->screen_width >= g_ptrack_context->disp_width && g_carback_context->screen_height>= g_ptrack_context->disp_height){
		g_carback_context->track_disp_width = g_ptrack_context->disp_width;
		g_carback_context->track_disp_height= g_ptrack_context->disp_height;
		g_carback_context->track_disp_xpos  = g_ptrack_context->disp_xpos;
		g_carback_context->track_disp_ypos  = g_ptrack_context->disp_ypos;
		g_carback_context->ptrack_param     = &g_ptrack_context->track_param;
		g_carback_context->file_type        = g_ptrack_context->file_type;
		g_carback_context->pmradar_param    = NULL;

		if(g_ptrack_context2)
			g_carback_context->pmradar_param = &g_ptrack_context2->mradar_param;
		
		set_disp_track_id(TRACK_STRAIGHT_FORWARD_ID);
		set_disp_car_id(CAR_NORMAL_STATUS_ID);
		set_disp_radar_id(RADAR_NORMAL_STATUS_ID);
		set_disp_track2_id(TRACK2_STRAIGHT_FORWARD_ID);
		set_disp_signal_id(IMAGE_ID_NONE);
		
		printk("track_paint init width=%d, height=%d,xpos=%d,ypos=%d.\n",g_carback_context->track_disp_width,\
			g_carback_context->track_disp_height,g_carback_context->track_disp_xpos,g_carback_context->track_disp_ypos);
	}
	else{
		printk(KERN_ALERT "track paint init out width or height fail!\n" );
		ret = -1;
	}
	
	if(vbox_track_paint){
		return ret;
	} 

	return ret; 
}

void track_paint_deinit(void)
{
	int i;
	
	zlib_uncompress_exit();
	
	if(g_ptrack_context->disp_track2_buf)
		vfree(g_ptrack_context->disp_track2_buf);

	for(i = 0; i < 4;i++)
		if(g_ptrack_context->disp_radar_buf[i])
			vfree(g_ptrack_context->disp_radar_buf[i]);
	
	if(g_ptrack_context->disp_car_buf)
		vfree(g_ptrack_context->disp_car_buf);
	
	if(g_ptrack_context)
		vfree(g_ptrack_context);
}

unsigned int track_paint_fill(void *dest, unsigned int width, unsigned int height)
{
	unsigned int fill_size, source_size, dest_size, source_offset;
	static unsigned int last_track_id = 0;
	static unsigned int last_car_id   = 0;
	static unsigned int last_radar_id = 0;
	static unsigned int last_signal_id = -1;
	static unsigned int last_track2_id= 0;
	track_context *p = g_ptrack_context;
	user_header* ph = g_pheader_buf;
	void *source;
 
	if(p == NULL || g_carback_context == NULL){
		printk(KERN_ERR "track_paint_fill g_ptrack_context or carback_context null\n" );
		return 0;
	}

	if( width > p->disp_width || height > p->disp_height){
		printk(KERN_ERR "track paint fill width height error!\n" );
		return 0;
	}
	
	if(p->disp_track_id > TRACK_MAX && p->disp_track_id < IMAGE_ID_NONE) 
		return 0;

	if(!g_carback_context->track_setting){
		memset(dest,0,g_carback_context->track_display_size);
		last_track_id = 0;
		last_car_id   = 0;
		last_radar_id = 0;
		last_signal_id = -1;
		last_track2_id= 0;
		return g_carback_context->track_display_size;
	}

	if (p->disp_track_id == IMAGE_ID_NONE){
		memset(dest,0,p->disp_track_size);
		return p->disp_track_size;
	}
	
	if( g_carback_context->carback_signal ){
		if(last_track_id == p->disp_track_id && last_car_id == p->disp_car_id   &&
	   last_radar_id == p->disp_radar_id &&last_track2_id == p->disp_track2_id && last_signal_id == p->disp_signal_id ){
		if(g_ptrack_context2 == NULL || !g_ptrack_context2->disp_mradar_id_change)
	        return 0;
	 }
	}

	if( !g_carback_context->carback_signal ){
		if(last_signal_id != p->disp_signal_id)
			memset(dest,0,g_carback_context->track_display_size);
		if(ph->signal_total)    
		subjoin_signal_pic(dest);
	}else{
		if(g_carback_context->layer_status){
			if(ph->track_total )
				subjoin_track_pic(dest);
		 
			if(ph->car_total)
				subjoin_car_pic(dest);

			if(ph->radar_total)  
				subjoin_radar_pic(dest);
		 
			if(ph->track2_total) 
				subjoin_track2_pic(dest);
			
			if(g_ptrack_context2 && vbox_track_paint) {
				subjoin_mradar_pic(dest);
			}
		}
	}
	if(g_carback_context->layer_status){
		last_track_id = p->disp_track_id;
		last_car_id   = p->disp_car_id;
		last_radar_id = p->disp_radar_id;
		last_track2_id= p->disp_track2_id; 
	}
	
	last_signal_id = p->disp_signal_id;
 
	return fill_size; 
}


