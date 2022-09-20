/*
 * ark7116 - Arkmicro ark7116 video decoder driver
 *
 * Copyright (c) 2020,2021 Arkmicro, Inc.
 * This code is placed under the terms of the GNU General Public License v2
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

#ifdef DEBUG
#define ARK7116_DUMP_REGS
#endif

#define ARK7116_STATUS			0x26
#define ARK7116_BUS_STATUS		0xaf
#define ARK7116_CONTRAST_CTL	0xd3
#define ARK7116_BRIGHT_CTL		0xd4
#define ARK7116_HUE_CTL			0xd5
#define ARK7116_SATURATION_CTL	0xd6
#define ARK7116_INPUT_CTL		0xdc

#define ARK7116_ENH_PLL			0XFD0E

#define ARK7116_AV0		0
#define ARK7116_AV1		1
#define ARK7116_AV2		2

typedef enum {
	 DISP_16_9 = 0,
	 DISP_4_3,
}ConfigDisplayMode;

enum {
	TYPE_UNKNOWN = -1,
	TYPE_CVBS = 0,
	TYPE_720P,
	TYPE_1080P,
};

enum {
	TYPE_UNDEF = -1,
	TYPE_ARK7116 = 0,
	TYPE_ARK7116H,
	TYPE_RN6752,
	TYPE_PR2000,
};

typedef struct {
	unsigned int addr;
	unsigned char value;
} PanlstaticPara;

typedef struct {
    unsigned int addr;
    unsigned char value[6];
}PanlPosDynPara;

PanlstaticPara AV1_staticPara[]= {
//GLOBAL
    {0XFD0A,0X30}, 
    {0XFD0B,0X27},     
    {0XFD0D,0XF0}, 
    {0XFD0F,0X03}, 
    {0XFD10,0X04},  
    {0XFD11,0XFF}, 
    {0XFD12,0XFF}, 
    {0XFD13,0XFF}, 
    {0XFD14,0X02}, 
    {0XFD15,0X02}, 
    {0XFD16,0X0A}, 
    {0XFD1A,0X40}, 
//DECODER
	{0XFE00,0X90}, 
    {0XFE83,0X7F},
    {0XFE26,0X0E},
    {0XFE27,0X00},
    {0XFE28,0X00},
    {0XFE2A,0X01},
    {0XFE42,0X00},
	{0XFE44,0X28},
    {0XFE83,0X7F}, 
    {0XFEAB,0X3E},
    {0XFEAC,0X77},
    {0XFEB1,0X01},
    {0XFEC9,0X00}, 	
    {0XFED0,0X41},
    {0XFED7,0XF7},
    {0XFEE0,0X2E},
    {0XFEE1,0X94},
    {0XFEE2,0X01},
//TCON
    {0XFC00,0X40}, 
//SCALE
    {0XFC90,0X02}, 
    {0XFC91,0X01}, 
    {0XFC92,0X00}, 
    {0XFC93,0X0C}, 
    {0XFC94,0X00}, 
    {0XFC95,0X00}, 
    {0XFC98,0X00}, 
    {0XFC99,0X04}, 
    {0XFC9A,0X59}, 
    {0XFC9B,0X03}, 
    {0XFC9C,0X01}, 
    {0XFC9D,0X00}, 
    {0XFC9E,0X06}, 
    {0XFC9F,0X00}, 
    {0XFCA0,0X23}, 
    {0XFCA1,0X00}, 
    {0XFCA2,0XF5}, 
    {0XFCA3,0X02}, 
    {0XFCA4,0X03}, 
    {0XFCA5,0X00}, 
    {0XFCA6,0X05}, 
    {0XFCA7,0X00}, 
    {0XFCA8,0X0E}, 
    {0XFCA9,0X00}, 
    {0XFCAA,0X06}, 
    {0XFCAB,0X01}, 
    {0XFCB1,0X14}, 
    {0XFCB2,0X00}, 
    {0XFCB3,0X00}, 
    {0XFCB4,0X00}, 
    {0XFCB5,0X00}, 
    {0XFCB7,0X07}, 
    {0XFCB8,0X01}, 
    {0XFCBB,0X37}, 
    {0XFCBC,0X01}, 
    {0XFCBD,0X01}, 
    {0XFCBE,0X00}, 
    {0XFCBF,0X0C}, 
    {0XFCC0,0X00}, 
    {0XFCC1,0X00}, 
    {0XFCC4,0X00}, 
    {0XFCC5,0X04}, 
    {0XFCC6,0X62}, 
    {0XFCC7,0X03}, 
    {0XFCC8,0X01}, 
    {0XFCC9,0X00}, 
    {0XFCCA,0X06}, 
    {0XFCCB,0X00}, 
    {0XFCCC,0X20}, 
    {0XFCCD,0X00}, 
    {0XFCCE,0XF2}, 
    {0XFCCF,0X02}, 
    {0XFCD1,0X00}, 
    {0XFCD2,0X08}, 
    {0XFCD3,0X00}, 
    {0XFCD4,0X08}, 
    {0XFCD5,0X00}, 
    {0XFCD6,0X28}, 
    {0XFCD7,0X01}, 
    {0XFCDD,0X14}, 
    {0XFCDE,0X00}, 
    {0XFCDF,0X00}, 
    {0XFCE0,0X00}, 
    {0XFCE1,0X00}, 
    {0XFCD0,0X03}, 
    {0XFCE2,0X00}, 
    {0XFCB6,0X00}, 
    {0XFB35,0X00}, 
    {0XFB89,0X00}, 	
};

PanlPosDynPara  AV1_posDynPara[]=
{
//dispmode:  16:9  4:3  DM_EX0  DM_EX1  DM_EX2  DM_EX3
//GLOBAL
//PAD MUX
//DECODER
//VP
//TCON
//SCALE
    {0XFC96,{0XDE,0XD4,0XD4,0XD4,0XD4,0XD4}},
    {0XFC97,{0X03,0X03,0X03,0X03,0X03,0X03}},
    {0XFCAC,{0X1E,0X20,0X20,0X20,0X20,0X20}},
    {0XFCAD,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCAE,{0X02,0X02,0X02,0X02,0X02,0X02}},
    {0XFCAF,{0X04,0X04,0X04,0X04,0X04,0X04}},
    {0XFCB0,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCC2,{0XE0,0XE0,0XE0,0XE0,0XE0,0XE0}},
    {0XFCC3,{0X03,0X03,0X03,0X03,0X03,0X03}},
    {0XFCD8,{0X0F,0X0F,0X0F,0X0F,0X0F,0X0F}},
    {0XFCD9,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCDA,{0X0A,0X0A,0X0A,0X0A,0X0A,0X0A}},
    {0XFCDB,{0X05,0X05,0X05,0X05,0X05,0X05}},
    {0XFCDC,{0X00,0X00,0X00,0X00,0X00,0X00}},
};

PanlstaticPara  AMT_PadMuxStaticPara[]=
{
//PAD MUX
    {0XFD32,0X11}, 
    {0XFD33,0X11}, 
    {0XFD34,0X00}, 
    {0XFD35,0X40}, 
    {0XFD36,0X44}, 
    {0XFD37,0X44}, 
    {0XFD38,0X44}, 
    {0XFD39,0X44}, 
    {0XFD3A,0X00}, 
    {0XFD3B,0X00}, 
    {0XFD3C,0X00}, 
    {0XFD3D,0X00}, 
    {0XFD3E,0X00}, 
    {0XFD3F,0X00}, 
    {0XFD40,0X00}, 
    {0XFD41,0X00}, 
    {0XFD44,0X01}, 
    {0XFD45,0X00}, 
    {0XFD46,0X00}, 
    {0XFD47,0X00}, 
    {0XFD48,0X00}, 
    {0XFD49,0X00}, 
    {0XFD4A,0X00}, 
    {0XFD4B,0X00}, 
    {0XFD50,0X09},
};

PanlstaticPara ark169_custom_para[] = {
//VP
    {0XFFB0,0X23}, 
    {0XFFB1,0X0F}, 
    {0XFFB2,0X12}, 
    {0XFFB3,0X15}, 
    {0XFFB4,0X15}, 
    {0XFFB7,0X90}, 
    {0XFFB8,0X10}, 
    {0XFFB9,0X62}, 
    {0XFFBA,0X20}, 
    {0XFFBB,0XAA}, 
    {0XFFBC,0X20}, 
	{0XFFBD,0X20}, 
    {0XFFC7,0X31}, 
    {0XFFC8,0X06}, 
    {0XFFC9,0X08}, 
    {0XFFCB,0XC0}, 
    {0XFFCC,0X80}, 
    {0XFFCD,0X2D}, 
    {0XFFCE,0X10}, 
    {0XFFCF,0X80}, 
    {0XFFD0,0X80}, 
    {0XFFD2,0X4F}, 
    {0XFFD3,0X80}, 
    {0XFFD4,0X80}, 
    {0XFFD7,0X1A}, 
    {0XFFD8,0X80}, 
    {0XFFE7,0X50}, 
    {0XFFE8,0XFF}, 
    {0XFFE9,0X22}, 
    {0XFFEA,0X20}, 
    {0XFFF0,0X4C}, 
    {0XFFF1,0XE8}, 
    {0XFFF2,0XE8}, 
    {0XFFF3,0XD7}, 
    {0XFFF4,0XFD}, 
    {0XFFF5,0X44}, 
    {0XFFF6,0XFA}, 
    {0XFFF7,0XE4}, 
    {0XFFF8,0XED}, 
    {0XFFF9,0XFD}, 
    {0XFFFA,0X4C}, 
    {0XFFFB,0X81}, 
    {0XFFD5,0X00}, 
    {0XFFD6,0X40}, 
//GAMMA
    {0XFF00,0X03}, 
    {0XFF01,0X2A}, 
    {0XFF02,0X3C}, 
    {0XFF03,0X4A}, 
    {0XFF04,0X57}, 
    {0XFF05,0X63}, 
    {0XFF06,0X6E}, 
    {0XFF07,0X78}, 
    {0XFF08,0X80}, 
    {0XFF09,0X88}, 
    {0XFF0A,0X91}, 
    {0XFF0B,0X99}, 
    {0XFF0C,0XA1}, 
    {0XFF0D,0XA9}, 
    {0XFF0E,0XB0}, 
    {0XFF0F,0XB6}, 
    {0XFF10,0XBC}, 
    {0XFF11,0XC2}, 
    {0XFF12,0XC7}, 
    {0XFF13,0XCC}, 
    {0XFF14,0XD0}, 
    {0XFF15,0XD4}, 
    {0XFF16,0XD8}, 
    {0XFF17,0XDC}, 
    {0XFF18,0XE1}, 
    {0XFF19,0XE5}, 
    {0XFF1A,0XE9}, 
    {0XFF1B,0XEC}, 
    {0XFF1C,0XF0}, 
    {0XFF1D,0XF4}, 
    {0XFF1E,0XF8}, 
    {0XFF1F,0XFB}, 
    {0XFF20,0X2A}, 
    {0XFF21,0X3C}, 
    {0XFF22,0X4A}, 
    {0XFF23,0X57}, 
    {0XFF24,0X63}, 
    {0XFF25,0X6E}, 
    {0XFF26,0X78}, 
    {0XFF27,0X80}, 
    {0XFF28,0X88}, 
    {0XFF29,0X91}, 
    {0XFF2A,0X99}, 
    {0XFF2B,0XA1}, 
    {0XFF2C,0XA9}, 
    {0XFF2D,0XB0}, 
    {0XFF2E,0XB6}, 
    {0XFF2F,0XBC}, 
    {0XFF30,0XC2}, 
    {0XFF31,0XC7}, 
    {0XFF32,0XCC}, 
    {0XFF33,0XD0}, 
    {0XFF34,0XD4}, 
    {0XFF35,0XD8}, 
    {0XFF36,0XDC}, 
    {0XFF37,0XE1}, 
    {0XFF38,0XE5}, 
    {0XFF39,0XE9}, 
    {0XFF3A,0XEC}, 
    {0XFF3B,0XF0}, 
    {0XFF3C,0XF4}, 
    {0XFF3D,0XF8}, 
    {0XFF3E,0XFB}, 
    {0XFF3F,0X2A}, 
    {0XFF40,0X3C}, 
    {0XFF41,0X4A}, 
    {0XFF42,0X57}, 
    {0XFF43,0X63}, 
    {0XFF44,0X6E}, 
    {0XFF45,0X78}, 
    {0XFF46,0X80}, 
    {0XFF47,0X88}, 
    {0XFF48,0X91}, 
    {0XFF49,0X99}, 
    {0XFF4A,0XA1}, 
    {0XFF4B,0XA9}, 
    {0XFF4C,0XB0}, 
    {0XFF4D,0XB6}, 
    {0XFF4E,0XBC}, 
    {0XFF4F,0XC2}, 
    {0XFF50,0XC7}, 
    {0XFF51,0XCC}, 
    {0XFF52,0XD0}, 
    {0XFF53,0XD4}, 
    {0XFF54,0XD8}, 
    {0XFF55,0XDC}, 
    {0XFF56,0XE1}, 
    {0XFF57,0XE5}, 
    {0XFF58,0XE9}, 
    {0XFF59,0XEC}, 
    {0XFF5A,0XF0}, 
    {0XFF5B,0XF4}, 
    {0XFF5C,0XF8}, 
    {0XFF5D,0XFB}, 
    {0XFF5E,0XFF}, 
    {0XFF5F,0XFF}, 
    {0XFF60,0XFF}, 
};

PanlstaticPara ark1668e_devb_custom_para[] = {
//VP
    {0XFFB0,0X23}, 
    {0XFFB1,0X0F}, 
    {0XFFB2,0X12}, 
    {0XFFB3,0X15}, 
    {0XFFB4,0X15}, 
    {0XFFB7,0X90}, 
    {0XFFB8,0X10}, 
    {0XFFB9,0X62}, 
    {0XFFBA,0X20}, 
    {0XFFBB,0XAA}, 
    {0XFFBC,0X20}, 
	{0XFFBD,0X20}, 
    {0XFFC7,0X31}, 
    {0XFFC8,0X06}, 
    {0XFFC9,0X08}, 
    {0XFFCB,0XC0}, 
    {0XFFCC,0X80}, 
    {0XFFCD,0X2D}, 
    {0XFFCE,0X10}, 
    {0XFFCF,0X80}, 
    {0XFFD0,0X80}, 
    {0XFFD2,0X4F}, 
    {0XFFD3,0X80}, 
    {0XFFD4,0X80}, 
    {0XFFD7,0X1A}, 
    {0XFFD8,0X80}, 
    {0XFFE7,0X50}, 
    {0XFFE8,0XFF}, 
    {0XFFE9,0X22}, 
    {0XFFEA,0X20}, 
    {0XFFF0,0X4C}, 
    {0XFFF1,0XE8}, 
    {0XFFF2,0XE8}, 
    {0XFFF3,0XD7}, 
    {0XFFF4,0XFD}, 
    {0XFFF5,0X44}, 
    {0XFFF6,0XFA}, 
    {0XFFF7,0XE4}, 
    {0XFFF8,0XED}, 
    {0XFFF9,0XFD}, 
    {0XFFFA,0X4C}, 
    {0XFFFB,0X81}, 
    {0XFFD5,0X00}, 
    {0XFFD6,0X40}, 
//GAMMA
    {0XFF00,0X03}, 
    {0XFF01,0X2A}, 
    {0XFF02,0X3C}, 
    {0XFF03,0X4A}, 
    {0XFF04,0X57}, 
    {0XFF05,0X63}, 
    {0XFF06,0X6E}, 
    {0XFF07,0X78}, 
    {0XFF08,0X80}, 
    {0XFF09,0X88}, 
    {0XFF0A,0X91}, 
    {0XFF0B,0X99}, 
    {0XFF0C,0XA1}, 
    {0XFF0D,0XA9}, 
    {0XFF0E,0XB0}, 
    {0XFF0F,0XB6}, 
    {0XFF10,0XBC}, 
    {0XFF11,0XC2}, 
    {0XFF12,0XC7}, 
    {0XFF13,0XCC}, 
    {0XFF14,0XD0}, 
    {0XFF15,0XD4}, 
    {0XFF16,0XD8}, 
    {0XFF17,0XDC}, 
    {0XFF18,0XE1}, 
    {0XFF19,0XE5}, 
    {0XFF1A,0XE9}, 
    {0XFF1B,0XEC}, 
    {0XFF1C,0XF0}, 
    {0XFF1D,0XF4}, 
    {0XFF1E,0XF8}, 
    {0XFF1F,0XFB}, 
    {0XFF20,0X2A}, 
    {0XFF21,0X3C}, 
    {0XFF22,0X4A}, 
    {0XFF23,0X57}, 
    {0XFF24,0X63}, 
    {0XFF25,0X6E}, 
    {0XFF26,0X78}, 
    {0XFF27,0X80}, 
    {0XFF28,0X88}, 
    {0XFF29,0X91}, 
    {0XFF2A,0X99}, 
    {0XFF2B,0XA1}, 
    {0XFF2C,0XA9}, 
    {0XFF2D,0XB0}, 
    {0XFF2E,0XB6}, 
    {0XFF2F,0XBC}, 
    {0XFF30,0XC2}, 
    {0XFF31,0XC7}, 
    {0XFF32,0XCC}, 
    {0XFF33,0XD0}, 
    {0XFF34,0XD4}, 
    {0XFF35,0XD8}, 
    {0XFF36,0XDC}, 
    {0XFF37,0XE1}, 
    {0XFF38,0XE5}, 
    {0XFF39,0XE9}, 
    {0XFF3A,0XEC}, 
    {0XFF3B,0XF0}, 
    {0XFF3C,0XF4}, 
    {0XFF3D,0XF8}, 
    {0XFF3E,0XFB}, 
    {0XFF3F,0X2A}, 
    {0XFF40,0X3C}, 
    {0XFF41,0X4A}, 
    {0XFF42,0X57}, 
    {0XFF43,0X63}, 
    {0XFF44,0X6E}, 
    {0XFF45,0X78}, 
    {0XFF46,0X80}, 
    {0XFF47,0X88}, 
    {0XFF48,0X91}, 
    {0XFF49,0X99}, 
    {0XFF4A,0XA1}, 
    {0XFF4B,0XA9}, 
    {0XFF4C,0XB0}, 
    {0XFF4D,0XB6}, 
    {0XFF4E,0XBC}, 
    {0XFF4F,0XC2}, 
    {0XFF50,0XC7}, 
    {0XFF51,0XCC}, 
    {0XFF52,0XD0}, 
    {0XFF53,0XD4}, 
    {0XFF54,0XD8}, 
    {0XFF55,0XDC}, 
    {0XFF56,0XE1}, 
    {0XFF57,0XE5}, 
    {0XFF58,0XE9}, 
    {0XFF59,0XEC}, 
    {0XFF5A,0XF0}, 
    {0XFF5B,0XF4}, 
    {0XFF5C,0XF8}, 
    {0XFF5D,0XFB}, 
    {0XFF5E,0XFF}, 
    {0XFF5F,0XFF}, 
    {0XFF60,0XFF}, 
};

struct ark7116 {
	struct v4l2_ctrl_handler hdl;
	struct v4l2_subdev sd;
	struct gpio_desc *reset_gpio;
	u32 input;
	u8 brightness;
	u8 contrast;
	u8 saturation;
	u8 hue;
	struct i2c_client	*client;
	unsigned short default_addr;
	PanlstaticPara *custom_para;
};

#define VIDIOC_GET_RESOLUTION  		_IOWR('V', BASE_VIDIOC_PRIVATE + 1, int)
#define VIDIOC_GET_PROGRESSIVE  	_IOWR('V', BASE_VIDIOC_PRIVATE + 2, int)
#define VIDIOC_GET_CHIPINFO  		_IOWR('V', BASE_VIDIOC_PRIVATE + 3, int)
#define VIDIOC_GET_ITU601_ENABLE	_IOWR('V', BASE_VIDIOC_PRIVATE + 6, int)

static inline struct ark7116 *to_ark7116(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ark7116, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct ark7116, hdl)->sd;
}

static int ark7116_read(struct v4l2_subdev *sd, unsigned char addr)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	rc = i2c_smbus_read_byte_data(client, addr);
	if (rc < 0) {
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);
		return rc;
	}

	dev_dbg(sd->dev, "ark7116: read 0x%02x = %02x\n", addr, rc);

	return rc;
}

static int ark7116_write(struct v4l2_subdev *sd, unsigned char addr,
				 unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	dev_dbg(sd->dev, "ark7116: writing %02x %02x\n", addr, value);
	rc = i2c_smbus_write_byte_data(client, addr, value);
	if (rc < 0)
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);

#ifdef ARK7116_DUMP_REGS
	rc = i2c_smbus_read_byte_data(client, addr);
	dev_dbg(sd->dev, "ark7116: read 0x%02x = %02x\n", addr, rc);
#endif

	return rc;
}

static unsigned char to_slave_addr(unsigned char addr)
{
	switch(addr) {
	case 0XF9:
	case 0XFD:
		return 0xb0;
	case 0XFA:
		return 0xbe; 	
	case 0XFB:
		return 0xb6;
	case 0XFC:
		return 0xb8;
	case 0XFE:
		return 0xb2;
	case 0XFF:
		return 0Xb4;
	case 0X00:
		return 0xbe;
	default:
		return 0xb0;
	}
}

static unsigned char amt_read_reg(struct ark7116 *decoder, unsigned int reg)
{
	struct v4l2_subdev *sd = &decoder->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	client->addr = to_slave_addr((reg >> 8) & 0xff) >> 1;
	rc = i2c_smbus_read_byte_data(client, reg & 0xff);
	client->addr = decoder->default_addr;
	if (rc < 0) {
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);
		return rc;
	}
	dev_dbg(sd->dev, "ark7116: read 0x%04x = %02x\n", reg, rc);

	return rc;
}

static int amt_write_reg(struct ark7116 *decoder, unsigned int reg, unsigned char value)
{
	struct v4l2_subdev *sd = &decoder->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	dev_dbg(sd->dev, "ark7116: writing %04x %02x\n", reg, value);
	client->addr = to_slave_addr((reg >> 8) & 0xff) >> 1;
	rc = i2c_smbus_write_byte_data(client, reg & 0xff, value);
	client->addr = decoder->default_addr;
	if (rc < 0)
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);

#ifdef ARK7116_DUMP_REGS
	client->addr = to_slave_addr((reg >> 8) & 0xff) >> 1;
	rc = i2c_smbus_read_byte_data(client, reg & 0xff);
	client->addr = decoder->default_addr;
	dev_dbg(sd->dev, "ark7116: read 0x%04x = %02x\n", reg, rc);
#endif

	return rc;	
}

/****************************************************************************
			I2C Client & Driver
 ****************************************************************************/
static int ark7116_detect_signal(struct ark7116 *decoder)
{
	return (ark7116_read(&decoder->sd, ARK7116_STATUS) & 0x6) == 0x6;
}

static int ark7116_select_input(struct ark7116 *decoder, u32 input)
{
	unsigned char val;

	if (input > ARK7116_AV2)
		return -EINVAL;

	if (input == ARK7116_AV0)	
		val = 0;
	else if (input == ARK7116_AV1)
		val = 0x10;
	else if (input == ARK7116_AV2)
		val = 0x30;

	return ark7116_write(&decoder->sd, ARK7116_INPUT_CTL, val);
}

static int ark7116_reset(struct ark7116 *decoder)
{
	gpiod_set_value_cansleep(decoder->reset_gpio, 0);
    mdelay(1);
    gpiod_set_value_cansleep(decoder->reset_gpio, 1);
    mdelay(1);
    gpiod_set_value_cansleep(decoder->reset_gpio, 0);
    mdelay(10);

	return 0;
};

static void ark7116_config_slave_mode(struct ark7116 *decoder)
{   
	unsigned char addr_buf[6] = {0xa1,0xa2,0xa3,0xa4,0xa5,0xa6};
	unsigned char data_buf[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char i;

	data_buf[0] = 0X55;
	data_buf[1] = 0xaa;
	data_buf[2] = 0X03;
	data_buf[3] = 0X50;  //slave mode
	data_buf[4] = 0;     // crc val
	data_buf[5] = data_buf[2] ^ data_buf[3] ^ data_buf[4];

	amt_write_reg(decoder, ARK7116_BUS_STATUS, 0x00);  //I2c Write Start

	for(i =0;i < 6;i++)
		amt_write_reg(decoder, addr_buf[i], data_buf[i]);

	amt_write_reg(decoder, ARK7116_BUS_STATUS, 0x11);  //I2c Write End
	udelay(10);

	amt_write_reg(decoder, 0xFAC6, 0x20);
}

static void ark7116_config_common(struct ark7116 *decoder)
{	
	int i;

	amt_write_reg(decoder, ARK7116_ENH_PLL, 0x20);

	for (i = 0; i < sizeof(AV1_staticPara) / sizeof(AV1_staticPara[0]); i++)
		amt_write_reg(decoder, AV1_staticPara[i].addr, AV1_staticPara[i].value);
	
	for (i = 0; i < sizeof(AV1_posDynPara) / sizeof(AV1_posDynPara[0]); i++)
		amt_write_reg(decoder, AV1_posDynPara[i].addr, AV1_posDynPara[i].value[DISP_16_9]);

	for (i = 0; i < sizeof(AMT_PadMuxStaticPara) / sizeof(AMT_PadMuxStaticPara[0]); i++)
		amt_write_reg(decoder, AMT_PadMuxStaticPara[i].addr, AMT_PadMuxStaticPara[i].value);

	amt_write_reg(decoder, ARK7116_ENH_PLL, 0x2C);
}

static int ark7116_config_custom(struct ark7116 *decoder)
{
	int i;
	int paranum = 0;

	if (decoder->custom_para == ark169_custom_para)
		paranum = sizeof(ark169_custom_para) / sizeof(ark169_custom_para[0]);
	else if (decoder->custom_para == ark1668e_devb_custom_para)
		paranum = sizeof(ark1668e_devb_custom_para) / sizeof(ark1668e_devb_custom_para[0]);

	for (i = 0; i < paranum; i++)
		amt_write_reg(decoder, decoder->custom_para[i].addr, decoder->custom_para[i].value);

	return 0;
}

static int _ark7116_init(struct ark7116 *decoder)
{
	int slave_mode, rom_sel;
	int retry_times = 10;
	unsigned char val;

reinit:
	ark7116_reset(decoder);

    ark7116_config_slave_mode(decoder);
    //soft reset 7116
    amt_write_reg(decoder, 0xFD00, 0x5A);
    mdelay(10);

    ark7116_config_slave_mode(decoder);
    val = amt_read_reg(decoder, 0xFAC6);
    slave_mode = (val & 0x80) ? 0 : 1;
    rom_sel = (val & 0x02) ? 1 : 0;
    
    if (rom_sel || !slave_mode) {
		if (retry_times-- > 0) {
			goto reinit;
		} else {
			printk(KERN_ALERT "ark7116 slave config error.\n");
			return -1;
		}
    }

	ark7116_config_common(decoder);
	ark7116_config_custom(decoder);

    //soft reset decoder
    val = amt_read_reg(decoder, 0xFEA0);
    val |= 1;
    amt_write_reg(decoder, 0xFEA0, val);
    mdelay(10);
    val &= ~1;
    amt_write_reg(decoder, 0xFEA0, val);
    mdelay(1);

	ark7116_select_input(decoder, decoder->input);

	return 0;
}

/* ----------------------------------------------------------------------- */
static int ark7116_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		ark7116_write(sd, ARK7116_BRIGHT_CTL, ctrl->val);
		return 0;
	case V4L2_CID_CONTRAST:
		ark7116_write(sd, ARK7116_CONTRAST_CTL, ctrl->val);
		return 0;
	case V4L2_CID_SATURATION:
		ark7116_write(sd, ARK7116_SATURATION_CTL, ctrl->val);
		return 0;
	case V4L2_CID_HUE:
		ark7116_write(sd, ARK7116_HUE_CTL, ctrl->val);
		return 0;
	}
	return -EINVAL;
}

static int ark7116_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct ark7116 *decoder = to_ark7116(sd);

	if (status) {
		*status = 0;
		if (!ark7116_detect_signal(decoder))
			*status |= V4L2_IN_ST_NO_SIGNAL;
	}

	return 0;
}

static int ark7116_s_routing(struct v4l2_subdev *sd, u32 input,
			     u32 output, u32 config)
{
	struct ark7116 *decoder = to_ark7116(sd);
	ark7116_select_input(decoder, input);
	decoder->input = input;
	return 0;
}
static int ark7116_init(struct v4l2_subdev *sd, u32 val)
{
	struct ark7116 *decoder = to_ark7116(sd);
	int ret;
	ret=_ark7116_init(decoder);
	if(ret){
		printk(KERN_ALERT "_ark7116_init error.\n");
	}
	return ret;
}

static long ark7116_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	case VIDIOC_GET_RESOLUTION:
	{
		int* temp = (int *)arg;
		*temp = TYPE_CVBS;
		break;
	}
	case VIDIOC_GET_PROGRESSIVE:
	{
		int* temp = (int *)arg;
		*temp = 0;
		break;
	}
	case VIDIOC_GET_CHIPINFO:
	{
		int* temp = (int *)arg;
		*temp = TYPE_ARK7116;
		break;
	}
	case VIDIOC_GET_ITU601_ENABLE:
	{
		int* temp = (int *)arg;
		*temp = 0;
		break;
	}
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops ark7116_ctrl_ops = {
	.s_ctrl = ark7116_s_ctrl,
};

static const struct v4l2_subdev_video_ops ark7116_video_ops = {
	.g_input_status = ark7116_g_input_status,
	.s_routing = ark7116_s_routing,
};

static const struct v4l2_subdev_core_ops ark7116_core_ops = {
	.init = ark7116_init,
	.ioctl = ark7116_ioctl,
};

static const struct v4l2_subdev_ops ark7116_ops = {
	.core =  &ark7116_core_ops,
	.video = &ark7116_video_ops,
};

static int ark7116_parse_dt(struct ark7116 *decoder, struct device_node *np)
{
	int ret = 0;
	int value;
	if (of_property_read_u32(np, "default-channel", &decoder->input))
		decoder->input = 0;
	if(!of_property_read_u32(np, "carback-config", &value)) {
		if(value == 1){
			printk("Initialize in carback.\n");
		}
		else if(value == 0){
			ret=_ark7116_init(decoder);
			if(ret){
				printk(KERN_ALERT "_ark7116_init error.\n");
			}
		}
	}

	return ret;
}

static int ark7116_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct ark7116 *decoder;
	struct v4l2_subdev *sd;
	struct device_node *np = client->dev.of_node;
	int res;

	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter,
	     I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -EIO;

	decoder = devm_kzalloc(&client->dev, sizeof(*decoder), GFP_KERNEL);
	if (!decoder)
		return -ENOMEM;
	decoder->client = client;
	decoder->default_addr = client->addr;
	decoder->custom_para = (PanlstaticPara *)id->driver_data;
	sd = &decoder->sd;
	decoder->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
						   GPIOD_OUT_HIGH);
	if (IS_ERR(decoder->reset_gpio)) {
		res = PTR_ERR(decoder->reset_gpio);
		v4l_err(client, "request for reset pin failed: %d\n", res);
		return res;
	}

	res = ark7116_parse_dt(decoder, np);
	if (res) {
		dev_err(sd->dev, "DT parsing error: %d\n", res);
		return res;
	}

	v4l2_i2c_subdev_init(sd, client, &ark7116_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	v4l2_ctrl_handler_init(&decoder->hdl, 4);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116_ctrl_ops,
			V4L2_CID_SATURATION, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116_ctrl_ops,
			V4L2_CID_HUE, -128, 127, 1, 0);
	sd->ctrl_handler = &decoder->hdl;
	if (decoder->hdl.error) {
		res = decoder->hdl.error;
		goto err;
	}

	res = v4l2_async_register_subdev(sd);
	if (res < 0)
		goto err;
	
	return 0;
err:
	v4l2_ctrl_handler_free(&decoder->hdl);
	return res;
	
	printk("ark7116_probe.\n");
	
	return 0;
}

static int ark7116_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ark7116 *decoder = to_ark7116(sd);

	dev_dbg(sd->dev, 
		"ark7116.c: removing ark7116 adapter on address 0x%x\n",
		client->addr << 1);

	v4l2_async_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&decoder->hdl);

	return 0;
}

/* ----------------------------------------------------------------------- */
/* the length of name must less than 20 */
static const struct i2c_device_id ark7116_id[] = {
	{ "ark7116_ark169", (kernel_ulong_t)ark169_custom_para },
	{ "ark7116_1668e_devb", (kernel_ulong_t)ark1668e_devb_custom_para },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ark7116_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ark7116_of_match[] = {
	{ .compatible = "arkmicro,ark7116_ark169", },
	{ .compatible = "arkmicro,ark7116_1668e_devb", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ark7116_of_match);
#endif

static struct i2c_driver ark7116_driver = {
	.driver = {
		.of_match_table = of_match_ptr(ark7116_of_match),
		.name	= "ark7116",
	},
	.probe		= ark7116_probe,
	.remove		= ark7116_remove,
	.id_table	= ark7116_id,
};

static int __init ark_7116_init(void)
{
	return i2c_add_driver(&ark7116_driver);
}

static void __exit ark_7116_exit(void)
{
	i2c_del_driver(&ark7116_driver);
}

device_initcall(ark_7116_init);

MODULE_AUTHOR("arkmicro");
MODULE_DESCRIPTION("arkmicro 7116 decoder driver for v4l2");
MODULE_LICENSE("GPL v2");
