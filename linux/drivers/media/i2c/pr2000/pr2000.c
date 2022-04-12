/*
 * ark1668 driver for pr2000 video codec.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include <asm/gpio.h>  
//#include "ark_itu656.h"
//#include "../../platform/arkmicro/ark1668e_vin.h"
#include "pr2000.h"

#define SENSOR_NAME "pr2000_csi0"

#define Camera_PlugIn     			0x01
#define Camera_PlugOut    			0x00

#define PAGE_ADDR				0xff
#define PAGE_0_DEC				0x00
#define PAGE_1_DEC				0x01
#define PR2000_CONTRAST_CTL		0x20
#define PR2000_BRIGHTNESS_CTL	0x21
#define PR2000_HUE_CTL			0x23
#define PR2000_SATURATION_CTL	0x22

enum SENSOR_DPI{
	PR2000_SENSOR_NONE,
	PR2000_CVBS_NTSC,
	PR2000_CVBS_PAL,
	PR2000_AHD_720P_25PFS,
	PR2000_AHD_720P_30PFS,
	PR2000_AHD_1080P_25PFS,
	PR2000_AHD_1080P_30PFS
};

enum pr2000_ic_type {
	DVR_IC_TYPE_UNKNOWN,
	DVR_IC_TYPE_PR2000,
	DVR_IC_TYPE_END
};

struct dvr_pr2000{
	struct v4l2_subdev sd;
	struct v4l2_ctrl_handler hdl;
	struct i2c_client *client;
	struct gpio_desc *reset_gpio;
	int gpio_irq;
	//int gpio_reset;
	int enter_carback;
	int id;
	u32 input;
};

enum {
	TYPE_UNKNOWN = -1,
	TYPE_CVBS = 0,
	TYPE_720P,
	TYPE_1080P,
};

enum {
	TYPE_UNDEF = 1,
	TYPE_PR2000,
};

#define VIDIOC_PR2000_GET_RESOLUTION  		_IOWR('V', BASE_VIDIOC_PRIVATE + 1, int)
#define VIDIOC_PR2000_GET_PROGRESSIVE  		_IOWR('V', BASE_VIDIOC_PRIVATE + 2, int)
#define VIDIOC_PR2000_GET_CHIPINFO  			_IOWR('V', BASE_VIDIOC_PRIVATE + 3, int)
#define VIDIOC_PR2000_GET_ITU601_ENABLE  	_IOWR('V', BASE_VIDIOC_PRIVATE + 6, int)

#define DBG_MODULE		0x00000001
#define DBG_I2C			0x00000002
#define DBG_INPUT		0x00000004

static unsigned int DbgLevel = DBG_MODULE|DBG_I2C|DBG_INPUT;
#define PR2000_DBG(level, fmt, args...)  do{ if( (level&DbgLevel)>0 ) \
					printk( KERN_DEBUG "[PR2000_DBG]: " fmt, ## args); }while(0)


static struct dvr_pr2000 *g_dvr_pr2000 = NULL;

//extern void ark_clear_carback_signal(void);
//extern int ark_itu656_probe(struct i2c_client *client, struct ark_private_data *pdata);
//extern int ark_itu656_remove(void);
//extern void dvr_restart(int source);
//static int pr2000_read_byte(unsigned char regaddr);
//static int pr2000_write_byte(unsigned char regaddr, unsigned char regval);
static int pr2000_read_byte(struct dvr_pr2000 *dvr_pr2000,unsigned char regaddr);
static int pr2000_write_byte(struct dvr_pr2000 *dvr_pr2000, unsigned char regaddr, unsigned char regval);
static int pr2000_read(struct v4l2_subdev *sd, unsigned char addr);
static int pr2000_write(struct v4l2_subdev *sd, unsigned char addr,unsigned char value);
//static int pr2000_reset(int gpio);
static int pr2000_reset(struct dvr_pr2000 *dvr_pr2000);
static int pr2000_detect(struct dvr_pr2000 *dvr_pr2000);
//static int pr2000_init(struct dvr_pr2000 *dvr_pr2000);
static int pr2000_init(struct v4l2_subdev *sd, u32 val);
static int pr2000_dvr_start_cb(void);
static int pr2000_dvr_stop_cb(void);

int pr2000_detect_camera_plug_status(struct dvr_pr2000 *dvr_pr2000);
int pr2000_signal_check(struct dvr_pr2000 *dvr_pr2000);
int pr2000_resolution_detect(struct dvr_pr2000 *dvr_pr2000);

/*
 * The default register settings
 *
 */
//Test 720p_30fps
u8 sensor_regs[] = {
//		Page0 vdec
0xff, 0x00,
0x10, 0x92,
0x11, 0x07,
0x12, 0x00,
0x13, 0x00,
0x14, 0x21,    //b[1:0}, => Select Camera Input. VinP(1), VinN(3), Differ(0).
0x15, 0x44,
0x16, 0x0d,
0x40, 0x00,
0x47, 0x3a,
0x4e, 0x3f,
0x80, 0x56,
0x81, 0x0e,
0x82, 0x0d,
0x84, 0x30,
0x86, 0x20,
0x87, 0x00,
0x8a, 0x00,
0x90, 0x00,
0x91, 0x00,
0x92, 0x00,
0x94, 0xff,
0x95, 0xff,
0x96, 0xff,
0xa0, 0x00,
0xa1, 0x20,
0xa4, 0x01,
0xa5, 0xe3,
0xa6, 0x00,
0xa7, 0x52,//default:0x12		Polarity Inversion:0x52 
0xa8, 0x04,//default:0x00		Polarity Inversion:0x04

0xd0, 0x30,
0xd1, 0x08,
0xd2, 0x21,
0xd3, 0x00,
0xd8, 0x31,//default:0x31  add:0x30
0xd9, 0x08,
0xda, 0x21,
0xe0, 0x39,
0xe1, 0x90,//default:0x90  add:0x80  Inversion:0xd0
0xe2, 0x38,//default:0x38  add:0x18  
0xe3, 0x19,//default:0x19  add:0x00
0xe4, 0x19,//default:0x19  add:0x00
0xea, 0x01,
0xeb, 0xff,
0xf1, 0x44,
0xf2, 0x01,

//                      Page1 vdec
0xff, 0x01,
0x00, 0xe4,////0xe4->black,0xe5->blue  
0x01, 0x61,
0x02, 0x00,
0x03, 0x57,
0x04, 0x0c,
0x05, 0x88,
0x06, 0x04,
0x07, 0xb2,
0x08, 0x44,
0x09, 0x34,
0x0a, 0x02,
0x0b, 0x14,
0x0c, 0x04,
0x0d, 0x08,
0x0e, 0x5e,
0x0f, 0x5e,
0x10, 0x26,
0x11, 0x00,
0x12, 0x45,
0x13, 0xfc,
0x14, 0x00,
0x15, 0x18,
0x16, 0xd0,
0x17, 0x00,
0x18, 0x41,
0x19, 0x46,
0x1a, 0x22,
0x1b, 0x05,
0x1c, 0xea,
0x1d, 0x45,
0x1e, 0x40,
0x1f, 0x00,
0x20, 0x80,
0x21, 0x80,
0x22, 0x90,
0x23, 0x80,
0x24, 0x80,
0x25, 0x80,
0x26, 0x84,
0x27, 0x82,
0x28, 0x00,
0x29, 0x7b,
0x2a, 0xa6,
0x2b, 0x00,
0x2c, 0x00,
0x2d, 0x00,
0x2e, 0x00,
0x2f, 0x00,
0x30, 0x00,
0x31, 0x00,
0x32, 0xc0,
0x33, 0x14,
0x34, 0x14,
0x35, 0x80,
0x36, 0x80,
0x37, 0xaa,
0x38, 0x48,
0x39, 0x08,
0x3a, 0x27,
0x3b, 0x02,
0x3c, 0x01,
0x3d, 0x23,
0x3e, 0x02,
0x3f, 0xc4,
0x40, 0x05,
0x41, 0x55,
0x42, 0x01,
0x43, 0x33,
0x44, 0x6a,
0x45, 0x00,
0x46, 0x09,
0x47, 0xdc,
0x48, 0xa0,
0x49, 0x00,
0x4a, 0x7b,
0x4b, 0x60,
0x4c, 0x00,
0x4d, 0x4a,
0x4e, 0x00,
0x4f, 0x24,
0x50, 0x01,
0x51, 0x28,
0x52, 0x40,
0x53, 0x0c,
0x54, 0x0f,
0x55, 0x8d,
0x70, 0x06,
0x71, 0x08,
0x72, 0x0a,
0x73, 0x0c,
0x74, 0x0e,
0x75, 0x10,
0x76, 0x12,
0x77, 0x14,
0x78, 0x06,
0x79, 0x08,
0x7a, 0x0a,
0x7b, 0x0c,
0x7c, 0x0e,
0x7d, 0x10,
0x7e, 0x12,
0x7f, 0x14,
0x80, 0x00,
0x81, 0x09,
0x82, 0x00,
0x83, 0x07,
0x84, 0x00,
0x85, 0x17,
0x86, 0x03,
0x87, 0xe5,
0x88, 0x08,
0x89, 0x91,
0x8a, 0x08,
0x8b, 0x91,
0x8c, 0x0b,
0x8d, 0xe0,
0x8e, 0x05,
0x8f, 0x47,
0x90, 0x05,
0x91, 0xa0,
0x92, 0x73,
0x93, 0xe8,
0x94, 0x0f,
0x95, 0x5e,
0x96, 0x07,
0x97, 0x90,
0x98, 0x17,
0x99, 0x34,
0x9a, 0x13,
0x9b, 0x56,
0x9c, 0x0b,
0x9d, 0x9a,
0x9e, 0x09,
0x9f, 0xab,
0xa0, 0x01,
0xa1, 0x74,
0xa2, 0x01,
0xa3, 0x6b,
0xa4, 0x00,
0xa5, 0xba,
0xa6, 0x00,
0xa7, 0xa3,
0xa8, 0x01,
0xa9, 0x39,
0xaa, 0x01,
0xab, 0x39,
0xac, 0x00,
0xad, 0xc1,
0xae, 0x00,
0xaf, 0xc1,
0xb0, 0x09,
0xb1, 0xaa,
0xb2, 0x0f,
0xb3, 0xae,
0xb4, 0x00,
0xb5, 0x17,
0xb6, 0x08,
0xb7, 0xe8,
0xb8, 0xb0,
0xb9, 0xce,
0xba, 0x90,
0xbb, 0x00,
0xbc, 0x00,
0xbd, 0x04,
0xbe, 0x05,
0xbf, 0x00,
0xc0, 0x00,
0xc1, 0x18,
0xc2, 0x02,
0xc3, 0xd0,

0xff, 0x01,
0x54, 0x0e,
0xff, 0x01,
0x54, 0x0f,

};

static inline struct dvr_pr2000 *to_pr2000(struct v4l2_subdev *sd)
{
	return container_of(sd, struct dvr_pr2000, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct dvr_pr2000, hdl)->sd;
}

//static int pr2000_reset(int gpio)
static int pr2000_reset(struct dvr_pr2000 *dvr_pr2000)
{
	PR2000_DBG(DBG_INPUT,"%s\n", __FUNCTION__);
	//if(gpio >= 0)
	//{
		//gpio_direction_output(gpio,1);
		//msleep(1);
		//gpio_direction_output(gpio,0);
		//msleep(100);
		//gpio_direction_output(gpio,1);
		//msleep(100);
	//}

	gpiod_direction_output(dvr_pr2000->reset_gpio,0);
	msleep(50);
	gpiod_direction_output(dvr_pr2000->reset_gpio,1);
	msleep(50);
	return 0;
}

static int pr2000_check_id(struct dvr_pr2000 *pr2000)
{
	PR2000_DBG(DBG_INPUT,"%s\n", __FUNCTION__);
	
	int ret,dvr_id = -1;
	int nReg;
	unsigned char id_lsb = 0, id_msb = 0;
	dvr_id = pr2000_write_byte(pr2000,0xff,0x00);
	if(dvr_id < 0)
		goto err_check_id;
	
	id_msb = pr2000_read_byte(pr2000,0xfc);
	if(id_msb < 0)
		goto err_check_id;
	
	id_lsb = pr2000_read_byte(pr2000,0xfd);
	if(dvr_id < 0)
		goto err_check_id;

	dvr_id = (id_msb << 8) |id_lsb;
	PR2000_DBG(DBG_INPUT,"pr2000_detect. Chip_ID=0x%04x\n", dvr_id);

	if(dvr_id == 0x2000){
		pr2000->id = DVR_IC_TYPE_PR2000;//jianchuang
		//PR2000_DBG(DBG_INPUT,"------------------>PR2000_init_cfg_720p_30pfs<------------------\n");
		for(nReg=0; nReg <  ARRAY_SIZE(PR2000_init_cfg_720p_30pfs); nReg+=2) {
	     			ret = pr2000_write_byte(pr2000,PR2000_init_cfg_720p_30pfs[nReg], PR2000_init_cfg_720p_30pfs[nReg+1]);
         			//printk("[pr2000_init] Reg[0x%02x,0x%02x]\r\n",PR2000_init_cfg_720p_30pfs[nReg],PR2000_init_cfg_720p_30pfs[nReg+1]);
        	}
	}
	else{
		PR2000_DBG(DBG_INPUT,"chip found is not an target chip.\n");
	}	

	return dvr_id;
	
err_check_id:
	PR2000_DBG(DBG_INPUT,"###ERR: %s failed, dvr_id:%d\n", __FUNCTION__,dvr_id);
	pr2000->id = DVR_IC_TYPE_UNKNOWN;
	return -ENODEV;

}

//static void pr2000_write_reg(const char *buf, int len)
//{
//	int i;
//	for(i=0; i<len; i++)
//		pr2000_write_byte(g_dvr_pr2000,buf[2*i],buf[2*i+1]);
//}

//static void pr2000_read_reg(const char *buf, int len)
//{
//	int i;
//	int regval;
//	for(i=0; i<len; i++){
//		regval = pr2000_read_byte(g_dvr_pr2000,buf[2*i]);
//		PR2000_DBG(DBG_INPUT,"reg:val [0x%x ,0x%x]\n", buf[2*i],regval);
//	}
//}

static void pr2000_dvr_get(void)
{
	u32 ret;	
	int i,nReg;
#if 0
	pr2000_read_reg(sensor_regs,sizeof(sensor_regs)/2);
#else
	for(nReg=0; nReg <  ARRAY_SIZE(sensor_regs); nReg+=2)
	{
		//pr2000_write_byte(0xff, 0x00);
		ret = pr2000_read_byte(g_dvr_pr2000,sensor_regs[nReg]);
		PR2000_DBG(DBG_INPUT,"Reg[0x%02x,0x%02x]\r\n",sensor_regs[nReg],ret);
	}
#endif
}

static void pr2000_dvr_set(const char *buf)
{
	//printk("write:echo write 0xff 0x00 > /sys/devices/platform/i2c-gpio.1/i2c-1/1-005c/dvr\n");		//Switch page0 ...
	//printk("write:echo write 0xe0 0x01 > /sys/devices/platform/i2c-gpio.1/i2c-1/1-005c/dvr\n");
	//printk("read:echo read 0x00 > /sys/devices/platform/i2c-gpio.1/i2c-1/1-005c/dvr\n");
	//printk("cat:cat /sys/devices/platform/i2c-gpio.1/i2c-1/1-005c/dvr\n");
	if(!strncmp(buf, "write", 5))
	{
		unsigned int reg,val;
		sscanf(buf,"%*s%x%x",&reg,&val);
		pr2000_write_byte(g_dvr_pr2000,reg,val);
		PR2000_DBG(DBG_INPUT,"write reg[0x%02x]:0x%02x\n",reg,val);
	}

	if(!strncmp(buf, "read", 4))
	{
		unsigned int reg,val;
		sscanf(buf,"%*s%x",&reg);
		val = pr2000_read_byte(g_dvr_pr2000,reg);
		PR2000_DBG(DBG_INPUT,"read reg[0x%02x]:0x%02x\n",reg,val);
	}
}

static void pr2000_device_init(struct dvr_pr2000 *pr2000,int Resolutuon)
{
	u32 ret;	
	int nReg;

	PR2000_DBG(DBG_INPUT,"Resolution is %d\n",Resolutuon);

	switch(Resolutuon)
	{
		case  PR2000_CVBS_NTSC:
		case  PR2000_CVBS_PAL:
		case  PR2000_AHD_720P_25PFS:
			PR2000_DBG(DBG_INPUT,"%s:PR2000_AHD_720P_25PFS\n", __FUNCTION__);
			for(nReg=0; nReg <  ARRAY_SIZE(PR2000_init_cfg_720p_25pfs); nReg+=2) {
	     			ret = pr2000_write_byte(pr2000,PR2000_init_cfg_720p_25pfs[nReg], PR2000_init_cfg_720p_25pfs[nReg+1]);
         			//printk("[pr2000_init] Reg[0x%02x,0x%02x]\r\n",PR2000_init_cfg_720p_25pfs[nReg],PR2000_init_cfg_720p_25pfs[nReg+1]);
        		}
			break;
		case  PR2000_AHD_720P_30PFS:
			PR2000_DBG(DBG_INPUT,"%s:PR2000_AHD_720P_30PFS\n", __FUNCTION__);
			for(nReg=0; nReg <  ARRAY_SIZE(PR2000_init_cfg_720p_30pfs); nReg+=2) {
	     			ret = pr2000_write_byte(pr2000,PR2000_init_cfg_720p_30pfs[nReg], PR2000_init_cfg_720p_30pfs[nReg+1]);
         			//printk("[pr2000_init] Reg[0x%02x,0x%02x]\r\n",PR2000_init_cfg_720p_30pfs[nReg],PR2000_init_cfg_720p_30pfs[nReg+1]);
        		}
			break;
		case  PR2000_AHD_1080P_25PFS:
		case  PR2000_AHD_1080P_30PFS:
		case  PR2000_SENSOR_NONE:
			PR2000_DBG(DBG_INPUT,"%s:PR2000_SENSOR_NONE\n", __FUNCTION__);
			for(nReg=0; nReg <  ARRAY_SIZE(PR2000_init_cfg_720p_30pfs); nReg+=2) {
	     			ret = pr2000_write_byte(pr2000,PR2000_init_cfg_720p_30pfs[nReg], PR2000_init_cfg_720p_30pfs[nReg+1]);
         			//printk("[pr2000_init] Reg[0x%02x,0x%02x]\r\n",PR2000_init_cfg_720p_30pfs[nReg],PR2000_init_cfg_720p_30pfs[nReg+1]);
        		}
			break;
	}
}

static int pr2000_init(struct v4l2_subdev *sd, u32 val)
{
	PR2000_DBG(DBG_INPUT,"%s\n", __FUNCTION__);
	struct dvr_pr2000 *dvr_pr2000 = to_pr2000(sd);
	u32 ret;	
	int nReg,id;
	int resolutuon_detect;

	//set channel
	
	//Make sure it is a target sensor
//	id = pr2000_check_id(dvr_pr2000);
//	if(id !=0x2000) 
//	{
//		PR2000_DBG(DBG_INPUT,"chip found is not an target chip.\n");
//		goto err_gpio_request;
//	}else{
//		//PR2000_DBG(DBG_INPUT,"------------------>PR2000_init_cfg_720p_30pfs<------------------\n");
//		for(nReg=0; nReg <  ARRAY_SIZE(PR2000_init_cfg_720p_30pfs); nReg+=2) {
//	     			ret = pr2000_write_byte(dvr_pr2000,PR2000_init_cfg_720p_30pfs[nReg], PR2000_init_cfg_720p_30pfs[nReg+1]);
//         			//printk("[pr2000_init] Reg[0x%02x,0x%02x]\r\n",PR2000_init_cfg_720p_30pfs[nReg],PR2000_init_cfg_720p_30pfs[nReg+1]);
//        	}
//	}

	//Detec Resolution
	resolutuon_detect = pr2000_resolution_detect(dvr_pr2000);
	//PR2000_DBG(DBG_INPUT,"Resolution Signal = %d\n", resolutuon_detect);

	//pr2000 reg init
	pr2000_device_init(dvr_pr2000,resolutuon_detect);
	//PR2000_DBG(DBG_INPUT,"%s end \n", __FUNCTION__);

	return 0;
	
err_gpio_request:
	return -1;
}

static long pr2000_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{printk("++++++++++++++++++[%s]\n", __func__);
	int ret = 0;
	switch (cmd) {
	case VIDIOC_PR2000_GET_RESOLUTION:
	{
		int* temp = (int *)arg;
		*temp = TYPE_720P;//TYPE_720P
		break;
	}
	case VIDIOC_PR2000_GET_PROGRESSIVE:
	{
		int* temp = (int *)arg;
		*temp = 1;//Ä¬ÈÏ1: ÖðÐÐÏÔÊ¾
		break;
	}
	case VIDIOC_PR2000_GET_CHIPINFO:
	{
		int* temp = (int *)arg;
		*temp = TYPE_PR2000;
		break;
	}
	case VIDIOC_PR2000_GET_ITU601_ENABLE:
	{
		int* temp = (int *)arg;
		*temp = 0;//1:enable	0:disabled
		break;
	}
	default:
		return -ENOIOCTLCMD;
	}
	return ret;
}

static int pr2000_read_byte(struct dvr_pr2000 *dvr_pr2000,unsigned char regaddr)
{
	//struct i2c_client *client;
	struct v4l2_subdev *sd = &dvr_pr2000->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	struct i2c_msg read_msgs[2];
	s32 ret = -1;
	s32 retries = 0;
	u8 regValue = 0x00;

	if(!g_dvr_pr2000)
		return -ENODEV;
		
	client = g_dvr_pr2000->client;

    read_msgs[0].flags = !I2C_M_RD;
    read_msgs[0].addr  = client->addr;
    read_msgs[0].len   = 1;
    read_msgs[0].buf   = &regaddr;

    read_msgs[1].flags = I2C_M_RD;
    read_msgs[1].addr  = client->addr;
    read_msgs[1].len   = 1;
    read_msgs[1].buf   = &regValue;//low byte

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, read_msgs, 2);
        if(ret == 2)
			break;
        retries++;
    }
	//printk("regValue=%d.\n", regValue);
    //if (regValue == 0xFF)
	//	regValue = 0;
    if(ret != 2)
    {
		PR2000_DBG(DBG_I2C,"ERR: [%s] reg:0x%x failure\n",__FUNCTION__, regaddr);
		return -EBUSY;
	}
	return regValue;
}

static int pr2000_write_byte(struct dvr_pr2000 *dvr_pr2000, unsigned char regaddr, unsigned char regval)
{
	//struct i2c_client *client;
	struct v4l2_subdev *sd = &dvr_pr2000->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
	
	if(!g_dvr_pr2000)
		return -ENODEV;
		
	client = g_dvr_pr2000->client;

	buf[0] = regaddr;
	buf[1] = regval;

	msg.flags = 0;
	msg.addr  = client->addr;
	msg.len   = 2;
	msg.buf   = buf;

	while(retries < 5)
	{
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)
			break;
		retries++;
	}
	if((retries >= 5))
	{
		PR2000_DBG(DBG_I2C,"ERR: %s failure\n",__FUNCTION__);
		return -EBUSY;
	}

	return 0;
}

int pr2000_detect_camera_plug_status(struct dvr_pr2000 *dvr_pr2000)
{
    	unsigned char detvideo;
	unsigned char lockstatus;

	pr2000_write_byte(dvr_pr2000,0xff, 0x00);
	
	detvideo = pr2000_read_byte(dvr_pr2000,0x00);
	lockstatus = pr2000_read_byte(dvr_pr2000,0x01);
	//printk("detvideo is 0x%02x\n",detvideo);
	//printk("lockstatus is 0x%02x\n",lockstatus);
	if((((lockstatus & 0x18) == 0x18) && ((detvideo & 0x08) == 0x08))||\
		((lockstatus & 0x90) == 0x90))
		//((lockstatus & 0x90) == 0x90)||(((lockstatus & 0x10) == 0x10) && ((detvideo & 0x83) == 0x83)))
	//if(((lockstatus & 0x18) == 0x18) && ((detvideo & 0x08) == 0x08))
	{
		PR2000_DBG(DBG_INPUT,"Camera Plug In\n");
		return Camera_PlugIn;
	}
	else if (((lockstatus & 0x18) == 0) && ((detvideo & 0x08) == 0))
	{
		PR2000_DBG(DBG_INPUT,"Camera Plug Out\n");
		return Camera_PlugOut;
	}
 	return 0;
}

int pr2000_signal_check(struct dvr_pr2000 *dvr_pr2000)
{
	return(pr2000_detect_camera_plug_status(dvr_pr2000));
}

int pr2000_resolution_detect(struct dvr_pr2000 *dvr_pr2000)
{
	int ret;
	unsigned char detvideo;
	int resolutuon_detect=0;
	ret = pr2000_detect_camera_plug_status(dvr_pr2000);
	if(ret == Camera_PlugIn)
	{
	    	pr2000_write_byte(dvr_pr2000,0xff, 0x00);
		detvideo = pr2000_read_byte(dvr_pr2000,0x00);

		PR2000_DBG(DBG_INPUT,"detvideo = 0x%02x,detvideo & 0x03 = 0x%02x\n",detvideo,detvideo & 0x03);
		
		if((detvideo & 0x03) == 0x00)
		{PR2000_DBG(DBG_INPUT,"PR2000_CVBS_NTSC\n");
		  	resolutuon_detect=PR2000_CVBS_NTSC; 
		}
		else if((detvideo & 0x03) == 0x01)
		{PR2000_DBG(DBG_INPUT,"PR2000_CVBS_PAL\n");
		   	resolutuon_detect=PR2000_CVBS_PAL;
		}
		else if(((detvideo & 0x03) == 0x02) &&((detvideo & 0x30) == 0x00))
		{PR2000_DBG(DBG_INPUT,"PR2000_AHD_720P_25PFS\n");
		  	resolutuon_detect=PR2000_AHD_720P_25PFS;
		}
		else if(((detvideo & 0x03) == 0x02) &&((detvideo & 0x30) == 0x10))
		{PR2000_DBG(DBG_INPUT,"PR2000_AHD_720P_30PFS\n");
		    	resolutuon_detect=PR2000_AHD_720P_30PFS;
		}
		
		else if(((detvideo & 0x03) == 0x03) &&((detvideo & 0x30) == 0x00))
		{PR2000_DBG(DBG_INPUT,"PR2000_AHD_1080P_25PFS\n");
		     	resolutuon_detect=PR2000_AHD_1080P_25PFS;
		}
		else if(((detvideo & 0x03) == 0x03) &&((detvideo & 0x30) == 0x10))
		{PR2000_DBG(DBG_INPUT,"PR2000_AHD_1080P_30PFS\n");
		   	resolutuon_detect=PR2000_AHD_1080P_30PFS;
		}
	}
	else
	{
		PR2000_DBG(DBG_INPUT,"02:PR2000_SENSOR_NONE\n");
	 	resolutuon_detect=PR2000_SENSOR_NONE;
	}
	//PR2000_DBG(DBG_INPUT,"resolutuon_detect is %d\n",resolutuon_detect);
	return   resolutuon_detect;
}

static int pr2000_get_progressive(void)
{
	return 0;
}

//Select itu656 input channel
static int pr2000_select_channel(int channel)
{
	u8 temp;
	int nReg;
	u32 ret;
	//PR2000_DBG(DBG_INPUT,"%s\n", __FUNCTION__);
	if((channel >= 0) && (channel <= 2))
	{
		PR2000_DBG(DBG_INPUT,"%s channel:%d\n", __FUNCTION__,channel);
	}
	return 0;
}

static int pr2000_detect_signal(void)
{
	//PR2000_DBG(DBG_INPUT,"%s\n", __FUNCTION__);
	if(g_dvr_pr2000)
	{
		if(g_dvr_pr2000->enter_carback)	//carback
		{
			
		}
		else	//auxin 
		{
		}
		//PR2000_DBG(DBG_INPUT,"%s CHIP_ID = %d\n", __FUNCTION__,g_dvr_pr2000->id);
		if(g_dvr_pr2000->id == DVR_IC_TYPE_PR2000)
			return 1;
	}
	return 0;
}

static int pr2000_get_itu601en(void)
{
	int enable_itu601 = 1;
	PR2000_DBG(DBG_INPUT,"%s:%d\n", __FUNCTION__,enable_itu601);
	return enable_itu601;
}
/* ----------------------------------------------------------------------- */
static int pr2000_read(struct v4l2_subdev *sd, unsigned char addr)
{
	//struct v4l2_subdev *sd = &dvr_pr2000->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = i2c_smbus_read_byte_data(client, addr);
	if (ret < 0) {
		PR2000_DBG(DBG_I2C,"i2c i/o error: ret = %s %d\n", __FUNCTION__,ret);
		return ret;
	}

	PR2000_DBG(DBG_I2C,"pr2000: read 0x%02x = %02x\n", addr, ret);
	return ret;
}

static int pr2000_write(struct v4l2_subdev *sd, unsigned char addr,
				 unsigned char value)
{
	//struct v4l2_subdev *sd = &dvr_pr2000->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	PR2000_DBG(DBG_I2C,"pr2000: writing %02x %02x\n", addr, value);
	ret = i2c_smbus_write_byte_data(client, addr, value);
	if (ret < 0)
		PR2000_DBG(DBG_I2C,"i2c i/o error: ret == %d\n", ret);

	return ret;
}


/* ----------------------------------------------------------------------- */
static int pr2000_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		pr2000_write(sd, PAGE_ADDR, PAGE_1_DEC);//(0xff,0x01)
		pr2000_write(sd, PR2000_BRIGHTNESS_CTL, ctrl->val);
		return 0;
	case V4L2_CID_CONTRAST:
		pr2000_write(sd, PAGE_ADDR, PAGE_1_DEC);//(0xff,0x01)
		pr2000_write(sd, PR2000_CONTRAST_CTL, ctrl->val);
		return 0;
	case V4L2_CID_SATURATION:
		pr2000_write(sd, PAGE_ADDR, PAGE_1_DEC);//(0xff,0x01)
		pr2000_write(sd, PR2000_SATURATION_CTL, ctrl->val);
		return 0;
	case V4L2_CID_HUE:
		pr2000_write(sd, PAGE_ADDR, PAGE_1_DEC);//(0xff,0x01)
		pr2000_write(sd, PR2000_HUE_CTL, ctrl->val);
		return 0;
	}
	return -EINVAL;
}

static int pr2000_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct dvr_pr2000 *pr2000 = to_pr2000(sd);
	int status_reg;
	if (status) {
		*status = 0;
	}
	status_reg = pr2000_signal_check(pr2000);
	//printk("++++++++++++++++++[%s] status_reg = %d\n", __func__,status_reg);
	*status = status_reg? 0 : V4L2_IN_ST_NO_SIGNAL;
	
	return 0;
}

//set pr2000 channel 
static int pr2000_s_routing(struct v4l2_subdev *sd,
			    u32 input, u32 output, u32 config)
{//printk("++++++++++++++++++[%s]\n", __func__);
	struct dvr_pr2000 *pr2000 = to_pr2000(sd);
	PR2000_DBG(DBG_INPUT,"%s select channel:%d\n", __FUNCTION__,input);
	pr2000->input = input;
	return 0;
}

/* ----------------------------------------------------------------------- */
static const struct v4l2_ctrl_ops pr2000_ctrl_ops = {
	.s_ctrl = pr2000_s_ctrl,
};

static const struct v4l2_subdev_video_ops pr2000_video_ops = {
	.g_input_status = pr2000_g_input_status,
	.s_routing = pr2000_s_routing,
};

static const struct v4l2_subdev_core_ops pr2000_core_ops = {
	.init = pr2000_init,
	.ioctl = pr2000_ioctl,
};

static const struct v4l2_subdev_ops pr2000_ops = {
	.core =  &pr2000_core_ops,
	.video = &pr2000_video_ops,
};

/* ----------------------------------------------------------------------- */
static int dvr_pr2000_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct dvr_pr2000 *dvr_pr2000;
	struct v4l2_subdev *sd;
	struct device_node *np = client->dev.of_node;
	int res;
	int nReg,ret;
       	PR2000_DBG(DBG_MODULE,"%s probe start \n", __FUNCTION__);
		
	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter,
	     I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -EIO;

	v4l_info(client, "chip found @ 0x%x (%s)\n",
			client->addr, client->adapter->name);
	
	dvr_pr2000 = devm_kzalloc(&client->dev, sizeof(*dvr_pr2000), GFP_KERNEL);
	if (dvr_pr2000 == NULL)
		return -ENOMEM;

	dvr_pr2000->client = client;
	dvr_pr2000->gpio_irq = -1;
	dvr_pr2000->enter_carback = 1;
	dvr_pr2000->id = 0;
	dvr_pr2000->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
						   GPIOD_OUT_HIGH);
	if (IS_ERR(dvr_pr2000->reset_gpio)) {
		res = PTR_ERR(dvr_pr2000->reset_gpio);
		v4l_err(client, "request for reset pin failed: %d\n", res);
		return res;
	}

	//pr2000_reset(dvr_pr2000);
	
	sd = &dvr_pr2000->sd;
	v4l2_i2c_subdev_init(sd, client, &pr2000_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	v4l2_ctrl_handler_init(&dvr_pr2000->hdl, 4);
	
	v4l2_ctrl_new_std(&dvr_pr2000->hdl, &pr2000_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&dvr_pr2000->hdl, &pr2000_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&dvr_pr2000->hdl, &pr2000_ctrl_ops,
			V4L2_CID_SATURATION, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&dvr_pr2000->hdl, &pr2000_ctrl_ops,
			V4L2_CID_HUE, -128, 127, 1, 0);
	sd->ctrl_handler = &dvr_pr2000->hdl;
	if (dvr_pr2000->hdl.error) {
		res = dvr_pr2000->hdl.error;
		goto err;
	}

	g_dvr_pr2000 = dvr_pr2000;
	res = v4l2_async_register_subdev(sd);
	if (res)
		goto err;

	id = pr2000_check_id(dvr_pr2000);
	if(id !=0x2000) 
		return -1;

	PR2000_DBG(DBG_MODULE,"%s probe done\n", __FUNCTION__);

	return 0;

err:
	v4l2_ctrl_handler_free(&dvr_pr2000->hdl);
	PR2000_DBG(DBG_MODULE,"******************[%s]\n", __FUNCTION__);
	return res;
}

static int dvr_pr2000_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct dvr_pr2000 *dvr_pr2000_pdata = to_pr2000(sd);

	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&dvr_pr2000_pdata->hdl);
	return 0;
}
MODULE_DEVICE_TABLE(i2c, dvr_pr2000_id);


/* ----------------------------------------------------------------------- */
static const struct i2c_device_id dvr_pr2000_id[] = {
	{"pr2000", 0},
	{}
};

static const struct of_device_id pr2000_of_match[] = {
	{ .compatible = "arkmicro,pr2000", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, pr2000_of_match);

static struct i2c_driver dvr_pr2000_driver = {
	.driver = {
		.name = "pr2000",
		.of_match_table = of_match_ptr(pr2000_of_match),
	},
	.probe = dvr_pr2000_probe,
	.remove = dvr_pr2000_remove,
	.id_table = dvr_pr2000_id,
};
static __init int dvr_pr2000_init(void)
{
	return i2c_add_driver(&dvr_pr2000_driver);
}

static __exit void dvr_pr2000_exit(void)
{
	i2c_del_driver(&dvr_pr2000_driver);
}


//subsys_initcall(dvr_pr2000_init);
device_initcall(dvr_pr2000_init);
//module_init(dvr_pr2000_init);
module_exit(dvr_pr2000_exit);

MODULE_AUTHOR("Arkmicro");
MODULE_DESCRIPTION("pr2000 decoder driver for v4l2");
MODULE_LICENSE("GPL");

