/*
 * ark7116h - Arkmicro ark7116h video decoder driver
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

//#define ARK7116H_DUMP_REGS

#define ARK7116H_STATUS			0xFD26
#define ARK7116H_INPUT_CTL		0xFDD4
#define ARK7116H_CONTRAST_CTL	0xFE53
#define ARK7116H_BRIGHT_CTL		0xFE54
#define ARK7116H_HUE_CTL	    0xFE55
#define ARK7116H_SATURATION_CTL	0xFE56

#define ARK7116H_AV0		0
#define ARK7116H_AV1		1
#define ARK7116H_AV2		2

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

PanlstaticPara AV1_staticPara[]=
{
//GLOBAL
    {0XFB01,0X01}, 
    {0XFB02,0X40}, 
    {0XFB03,0X16}, 
    {0XFB04,0X00}, 
    {0XFB05,0X00}, 
    {0XFB06,0XC0}, 
    {0XFB07,0X00}, 
    {0XFB08,0X04}, 
    {0XFB09,0X04}, 
    {0XFB0A,0X5C}, 
    {0XFB0B,0XFF}, 
    {0XFB0C,0XF2}, 
    {0XFB0D,0XAA}, 
    {0XFB0E,0XAA}, 
    {0XFB0F,0X2A}, 
    {0XFB10,0X01}, 
    {0XFB11,0X02}, 
    {0XFB12,0X02}, 
    {0XFB13,0X28}, 
    {0XFB14,0X7F}, 
    {0XFB15,0X00}, 
    {0XFB16,0X07}, 
    {0XFB17,0X0C}, 
    {0XFB18,0X0C}, 
    {0XFB19,0X00}, 
    {0XFB1A,0X00}, 
    {0XFB1B,0X0C}, 
    {0XFB1C,0X0C}, 
    {0XFB1D,0X13}, 
    {0XFB1E,0X2B}, 
    {0XFB1F,0X12}, 
    {0XFB20,0X80}, 
    {0XFB21,0X00}, 
    {0XFB22,0X00}, 
    {0XFB23,0X2A}, 
    {0XFB24,0X11}, 
    {0XFB25,0XE8}, 
    {0XFB26,0X03}, 
    {0XFB27,0XFF}, 
    {0XFB28,0X0C}, 
    {0XFB29,0X95}, 
    {0XFB2A,0X55}, 
    {0XFB2B,0X55}, 
    {0XFB2C,0XC0}, 
    {0XFB2D,0X00}, 
    {0XFB2E,0X55}, 
    {0XFB2F,0X55}, 
    {0XFB30,0X55}, 
    {0XFB31,0X55}, 
    {0XFB32,0X55}, 
    {0XFB33,0XCC}, 
    {0XFB34,0X1A}, 
    {0XFB35,0X00}, 
    {0XFB36,0X04}, 
    {0XFB45,0XFF}, 
    {0XFB43,0X60}, 
    {0XFB37,0X8C}, 
    {0XFB38,0X10}, 
    {0XFB3D,0X8C}, 
    {0XFB41,0X28}, 
    {0XFB42,0X20}, 
    {0XFB44,0X04}, 
    {0XFB39,0X01}, 
    {0XFB3A,0X01}, 
    {0XFB3B,0X00}, 
    {0XFB3C,0X20}, 
    {0XFB3E,0X4C}, 
    {0XFB3F,0X20}, 
    {0XFB40,0X20}, 
//DECODER
    {0XFD01,0X04}, 
    {0XFD02,0X00}, 
    {0XFD03,0X80}, 
    {0XFD04,0X80}, 
    {0XFD05,0X30}, 
    {0XFD06,0X02}, 
    {0XFD07,0X80}, 
    {0XFD08,0X00}, 
    {0XFD09,0X00}, 
    {0XFD0A,0X2F}, 
    {0XFD0B,0X40}, 
    {0XFD0C,0X12}, 
    {0XFD0D,0X03}, 
    {0XFD0E,0X72}, 
    {0XFD0F,0X07}, 
    {0XFD10,0X14}, 
    {0XFD11,0X09}, 
    {0XFD12,0X00}, 
    {0XFD13,0X1E}, 
    {0XFD14,0X22}, 
    {0XFD15,0X05}, 
    {0XFD26,0X0E}, 
    {0XFD27,0X00}, 
    {0XFD28,0X00}, 
    {0XFD2A,0X01}, 
    {0XFD35,0XAA}, 
    {0XFD36,0XAA}, 
    {0XFD37,0X60}, 
    {0XFD38,0X0F}, 
    {0XFD39,0X08}, 
    {0XFD48,0X07}, 
    {0XFD54,0X40}, 
    {0XFD55,0X8A}, 
    {0XFD5F,0XC0}, 
    {0XFD60,0X03}, 
    {0XFD61,0X2D}, 
    {0XFD62,0X41}, 
    {0XFD63,0XC0}, 
    {0XFD83,0X4F}, 
    {0XFDA0,0X0E}, 
    {0XFDAA,0X03}, 
    {0XFDAB,0X15}, 
    {0XFDAC,0X74}, 
    {0XFDB1,0X02}, 
    {0XFDB2,0X76}, 
    {0XFDB5,0X6F}, 
    {0XFDCA,0X4F}, 
    {0XFDCD,0X32}, 
    {0XFDD0,0X01}, 
    {0XFDD1,0X19}, 
    {0XFDD2,0X00}, 
    {0XFDD3,0X00}, 
    {0XFDD4,0X00}, 
    {0XFDD5,0XB1}, 
    {0XFDD6,0X08}, 
    {0XFDD7,0XF7}, 
    {0XFDD8,0XA3}, 
    {0XFDD9,0X40}, 
    {0XFDDA,0X29}, 
    {0XFDDB,0X00}, 
    {0XFDDC,0X00}, 
    {0XFDDD,0X4C}, 
    {0XFDDE,0X59}, 
    {0XFDDF,0X53}, 
    {0XFDE0,0X2E}, 
    {0XFDE1,0X17}, 
    {0XFDE2,0X00}, 
    {0XFD4F,0X40}, 
    {0XFD4F,0X40}, 
    {0XFD50,0X42}, 
    {0XFD31,0X20}, 
    {0XFD52,0X70}, 
    {0XFD4E,0X82}, 
    {0XFD7B,0XE5}, 
    {0XFD51,0X90}, 
    {0XFD16,0X4E}, 
    {0XFD98,0X0A}, 
    {0XFD1B,0X22}, 
    {0XFD1C,0X61}, 
    {0XFD41,0X8A}, 
    {0XFDCB,0X10}, 
    {0XFD43,0X80}, 
    {0XFD44,0X20}, 
    {0XFD45,0X80}, 
    {0XFD46,0X00}, 
    {0XFD56,0X07}, 
    {0XFD5D,0X03}, 
    {0XFD19,0X82}, 
    {0XFD1A,0X50}, 
    {0XFD1D,0X70}, 
    {0XFD8A,0X0A}, 
    {0XFD8B,0X00}, 
    {0XFDC8,0X00}, 
    {0XFDC9,0X01}, 
    {0XFDC6,0X00}, 
    {0XFD80,0X50}, 
    {0XFD7C,0X0A}, 
    {0XFD7D,0XFA}, 
    {0XFDB4,0XD2}, 
//deinterlace
    {0XFE00,0X10}, 
    {0XFE01,0X00}, 
    {0XFE02,0X42}, 
    {0XFE03,0X0D}, 
    {0XFE04,0X30}, 
    {0XFE05,0X70}, 
    {0XFE06,0X00}, 
    {0XFE07,0X30}, 
    {0XFE08,0X02}, 
    {0XFE09,0X0B}, 
    {0XFE0A,0X02}, 
    {0XFE0B,0X00}, 
    {0XFE0C,0X40}, 
    {0XFE0D,0X00}, 
    {0XFE0E,0X08}, 
    {0XFE0F,0X00}, 
    {0XFE10,0X00}, 
    {0XFE11,0X00}, 
    {0XFE12,0XA0}, 
    {0XFE13,0XA0}, 
    {0XFE14,0X10}, 
    {0XFE15,0XA0}, 
    {0XFE16,0X20}, 
    {0XFE17,0X90}, 
    {0XFE18,0XA0}, 
    {0XFE19,0X60}, 
    {0XFE1A,0X40}, 
    {0XFE1B,0X04}, 
    {0XFE1C,0X04}, 
    {0XFE1D,0X00}, 
    {0XFE1E,0X10}, 
    {0XFE1F,0X22}, 
    {0XFEE0,0X30}, 
    {0XFEE1,0X00}, 
    {0XFEE2,0X36}, 
    {0XFE20,0XFF}, 
    {0XFE21,0XFF}, 
    {0XFE22,0XFF}, 
    {0XFE23,0XFF}, 
    {0XFE24,0XFF}, 
    {0XFE25,0XFF}, 
    {0XFE26,0XFF}, 
//VP
    {0XFE30,0X27}, 
    {0XFE31,0X8F}, 
    {0XFE32,0X10}, 
    {0XFE33,0X10}, 
    {0XFE34,0X10}, 
    {0XFE35,0X60}, 
    {0XFE36,0X10}, 
    {0XFE37,0XD0}, 
    {0XFE38,0X10}, 
    {0XFE39,0X41}, 
    {0XFE3A,0X20}, 
    {0XFE3B,0X16}, 
    {0XFE3C,0X20}, 
    {0XFE3D,0X20}, 
    {0XFE3E,0X20}, 
    {0XFE3F,0X20}, 
    {0XFE40,0X64}, 
    {0XFE41,0X49}, 
    {0XFE42,0XFF}, 
    {0XFE43,0XFF}, 
    {0XFE44,0XFF}, 
    {0XFE45,0XFF}, 
    {0XFE46,0XFF}, 
    {0XFE47,0X4C}, 
    {0XFE48,0X1A}, 
    {0XFE49,0X15}, 
    {0XFE4A,0X55}, 
    {0XFE4B,0X80}, 
    {0XFE4C,0X80}, 
    {0XFE4D,0X00}, 
    {0XFE4E,0X00}, 
    {0XFE4F,0X80}, 
    {0XFE50,0X80}, 
    {0XFE51,0X0A}, 
    {0XFE52,0X4F}, 
    {0XFE53,0X80}, 
    {0XFE54,0X7A}, 
    {0XFE55,0X00}, 
    {0XFE56,0X38}, 
    {0XFE57,0X10}, 
    {0XFE58,0X80}, 
    {0XFE59,0X65}, 
    {0XFE5A,0X00}, 
    {0XFE5D,0XFF}, 
    {0XFE5E,0XFF}, 
    {0XFE5F,0XFF}, 
    {0XFE60,0X3C}, 
    {0XFE61,0XFF}, 
    {0XFE62,0XFF}, 
    {0XFE63,0XFF}, 
    {0XFE64,0XFF}, 
    {0XFE65,0XFF}, 
    {0XFE66,0XFF}, 
    {0XFE67,0X24}, 
    {0XFE68,0XFF}, 
    {0XFE69,0X20}, 
    {0XFE6A,0X9F}, 
    {0XFE70,0X42}, 
    {0XFE71,0XE0}, 
    {0XFE72,0XDE}, 
    {0XFE73,0XE1}, 
    {0XFE74,0XFD}, 
    {0XFE75,0X2E}, 
    {0XFE76,0XF0}, 
    {0XFE77,0XDD}, 
    {0XFE78,0XF0}, 
    {0XFE79,0XFD}, 
    {0XFE7A,0X33}, 
    {0XFE7B,0X81}, 
//vi_regfile
    {0XFF01,0XFF}, 
    {0XFF02,0XFF}, 
    {0XFF03,0XFF}, 
    {0XFF04,0XFF}, 
    {0XFF05,0XFF}, 
    {0XFF06,0XFF}, 
    {0XFF07,0XFF}, 
    {0XFF08,0XFF}, 
    {0XFF09,0XFF}, 
    {0XFF0A,0XFF}, 
    {0XFF0B,0XFF}, 
    {0XFF0C,0XFF}, 
    {0XFF0D,0XFF}, 
    {0XFF0E,0XFF}, 
    {0XFF0F,0XFF}, 
    {0XFF10,0XFF}, 
    {0XFF11,0XFF}, 
    {0XFF12,0XFF}, 
    {0XFF13,0XFF}, 
    {0XFF14,0XFF}, 
    {0XFF15,0XFF}, 
    {0XFF16,0XFF}, 
    {0XFF17,0XFF}, 
    {0XFF18,0XFF}, 
    {0XFF19,0XFF}, 
    {0XFF1A,0XFF}, 
    {0XFF1B,0XFF}, 
    {0XFF1C,0XFF}, 
    {0XFF1D,0XFF}, 
    {0XFF1E,0XFF}, 
    {0XFF1F,0XFF}, 
//scale
    {0XFF20,0XC0}, 
    {0XFF21,0X03}, 
    {0XFF22,0XA0},      //B6 ok //debug scale
    {0XFF23,0X03}, 
    {0XFF24,0XEE}, 
    {0XFF25,0X03}, 
    {0XFF26,0X07}, 
    {0XFF27,0X91}, 
    {0XFF28,0X51}, 
    {0XFF29,0X00}, 
    {0XFF2A,0X03}, 
    {0XFF2B,0X80}, 
    {0XFF2C,0X02}, 
    {0XFF2D,0XD0}, 
    {0XFF2E,0X02}, 
    {0XFF2F,0X40}, 
    {0XFF30,0X02}, 
    {0XFF31,0X02}, 
    {0XFF32,0X00}, 
    {0XFF33,0X28}, 
    {0XFF34,0X02}, 
    {0XFF35,0X00}, 
    {0XFF36,0X05}, 
    {0XFF37,0X11}, 
    {0XFF38,0X00}, 
    {0XFF39,0X27}, 
    {0XFF3A,0X03}, 
    {0XFF3B,0X00}, 
    {0XFF3C,0X00}, 
    {0XFF3D,0X37}, 
    {0XFF3E,0X02}, 
    {0XFF3F,0X00}, 
    {0XFF40,0X00}, 
    {0XFF41,0X00}, 
    {0XFF42,0X00}, 
    {0XFF43,0X00}, 
    {0XFF44,0X96}, 
    {0XFF45,0X00}, 
    {0XFF46,0X00}, 
    {0XFF47,0X00}, 
    {0XFF48,0X00}, 
    {0XFF49,0X19}, 
    {0XFF4A,0X00}, 
    {0XFF4B,0XE9}, 
    {0XFF4C,0X02}, 
    {0XFF4D,0X09}, 
    {0XFF4E,0X00}, 
    {0XFF4F,0X49}, 
    {0XFF50,0X02}, 
    {0XFF51,0X45}, 
    {0XFF52,0X00}, 
    {0XFF53,0X15}, 
    {0XFF54,0X03}, 
    {0XFF55,0X07}, 
    {0XFF56,0X00}, 
    {0XFF57,0X47}, 
    {0XFF58,0X02}, 
    {0XFF59,0XFF}, 
    {0XFF5A,0X03}, 
    {0XFF5B,0XFF}, 
    {0XFF5C,0XFD}, 
    {0XFF5D,0XFF}, 
    {0XFF68,0XFF}, 
    {0XFF69,0XFD}, 
    {0XFF6A,0XFF}, 
    {0XFF6B,0XFD}, 
    {0XFF6C,0XFD}, 
    {0XFF6D,0XFF}, 
    {0XFF6E,0XFD}, 
    {0XFF6F,0XFD}, 
//GAMMA
    {0XFF80,0XC0}, 
    {0XFF81,0X00}, 
    {0XFF82,0X00}, 
    {0XFF83,0X03}, 
    {0XFF84,0X05}, 
    {0XFF85,0X0B}, 
    {0XFF86,0X12}, 
    {0XFF87,0X19}, 
    {0XFF88,0X21}, 
    {0XFF89,0X2A}, 
    {0XFF8A,0X33}, 
    {0XFF8B,0X3E}, 
    {0XFF8C,0X48}, 
    {0XFF8D,0X54}, 
    {0XFF8E,0X5F}, 
    {0XFF8F,0X6A}, 
    {0XFF90,0X74}, 
    {0XFF91,0X7F}, 
    {0XFF92,0X89}, 
    {0XFF93,0X93}, 
    {0XFF94,0X9E}, 
    {0XFF95,0XA9}, 
    {0XFF96,0XB3}, 
    {0XFF97,0XBD}, 
    {0XFF98,0XC6}, 
    {0XFF99,0XCE}, 
    {0XFF9A,0XD5}, 
    {0XFF9B,0XDC}, 
    {0XFF9C,0XE1}, 
    {0XFF9D,0XE7}, 
    {0XFF9E,0XEB}, 
    {0XFF9F,0XF0}, 
    {0XFFA0,0XF4}, 
    {0XFFA1,0XF8}, 
//rgb_test_c
    {0XFFA8,0XFB}, 
    {0XFFA9,0X00}, 
    {0XFFAA,0X00}, 
    {0XFFAB,0X00}, 
    {0XFFAC,0X00}, 
    {0XFFAD,0X00}, 
    {0XFFC0,0X00}, 
    {0XFFC1,0X04}, 
    {0XFFC2,0X4C}, 
    {0XFFC3,0X02}, 
    {0XFFC4,0X5A}, 
    {0XFFC5,0X2C}, 
    {0XFFC6,0X74}, 
    {0XFFC7,0XDA}, 
    {0XFFC8,0X5C}, 
    {0XFFC9,0X10}, 
    {0XFFCA,0X43}, 
    {0XFFCB,0X6F}, 
    {0XFFCC,0X60}, 
    {0XFFCD,0X60}, 
    {0XFFCE,0X60}, 
    {0XFFCF,0X00}, 
    {0XFFD0,0X7E}, 
    {0XFFD1,0XEF}, 
    {0XFFD2,0XFF}, 
    {0XFFD3,0XFC}, 
    {0XFFD4,0X4E}, 
    {0XFFD5,0X22}, 
    {0XFFD6,0XE9}, 
    {0XFFD7,0X1B}, 
    {0XFFD8,0X00}, 
    {0XFFD9,0X00}, 
    {0XFFDA,0X00}, 
    {0XFFDB,0X00}, 
    {0XFFDC,0X00}, 
    {0XFFDD,0X00}, 
    {0XFFE8,0X00}, 
    {0XFFE9,0X4A}, 
    {0XFFEA,0X04}, 
    {0XFFEB,0X00}, 
    {0XFFEC,0X00}, 
    {0XFFED,0XFF}, 
    {0XFFEE,0XFF}, 
    {0XFFEF,0X00}, 
};

struct ark7116h {
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

static inline struct ark7116h *to_ark7116h(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ark7116h, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct ark7116h, hdl)->sd;
}

static unsigned char to_slave_addr(unsigned char addr)
{
	switch(addr) {
	case 0XF9:
	case 0XFD:
		return 0xB4;
    case 0XF3:
		return 0xE6; 
	case 0XFA:
		return 0xBA; 	
	case 0XFB:
		return 0xB8;
	case 0XFC:
		return 0xB6;
	case 0XFE:
		return 0xB2;
	case 0XFF:
		return 0XB0;
	case 0X00:
		return 0xBE;
	default:
		return 0xBE;
	}
}

static unsigned char amt_read_reg(struct ark7116h *decoder, unsigned int reg)
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
	dev_dbg(sd->dev, "ark7116h: read 0x%04x = %02x\n", reg, rc);

	return rc;
}

static int amt_write_reg(struct ark7116h *decoder, unsigned int reg, unsigned char value)
{
	struct v4l2_subdev *sd = &decoder->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	client->addr = to_slave_addr((reg >> 8) & 0xff) >> 1;
	rc = i2c_smbus_write_byte_data(client, reg & 0xff, value);
	client->addr = decoder->default_addr;
	if (rc < 0)
		printk(KERN_ALERT "i2c i/o error: rc == %d\n", rc);

#ifdef ARK7116H_DUMP_REGS
	client->addr = to_slave_addr((reg >> 8) & 0xff) >> 1;
	rc = i2c_smbus_read_byte_data(client, reg & 0xff);
	client->addr = decoder->default_addr;
    printk(KERN_ALERT "ark7116h: read 0x%04x = %02x\n", reg, rc);
#endif

	return rc;	
}

/****************************************************************************
			I2C Client & Driver
 ****************************************************************************/
static int ark7116h_detect_signal(struct ark7116h *decoder)
{
	return (amt_read_reg(decoder, ARK7116H_STATUS) & 0x2) == 0x2;
}

static int ark7116h_select_input(struct ark7116h *decoder, u32 input)
{
	unsigned char val;

	if (input > ARK7116H_AV2)
		return -EINVAL;

	if (input == ARK7116H_AV0)	
		val = 0;
	else if (input == ARK7116H_AV1)
		val = 0x10;
	else if (input == ARK7116H_AV2)
		val = 0x30;

	return amt_write_reg(decoder, ARK7116H_INPUT_CTL, val);
}

static int ark7116h_reset(struct ark7116h *decoder)
{
	gpiod_set_value_cansleep(decoder->reset_gpio, 0);
    mdelay(1);
    gpiod_set_value_cansleep(decoder->reset_gpio, 1);
    mdelay(1);
    gpiod_set_value_cansleep(decoder->reset_gpio, 0);
    mdelay(10);

	return 0;
};

static void ark7116h_config_common(struct ark7116h *decoder)
{	
	int i;
	for (i = 0; i < sizeof(AV1_staticPara) / sizeof(AV1_staticPara[0]); i++)
		amt_write_reg(decoder, AV1_staticPara[i].addr, AV1_staticPara[i].value);
}

static int _ark7116h_init(struct ark7116h *decoder)
{
	unsigned char val;
    
reinit:
    ark7116h_reset(decoder);
    //soft reset 7116h
    amt_write_reg(decoder, 0xFB00, 0x5A);
    mdelay(10);
    amt_write_reg(decoder,0xC6, 0x40);
    val = amt_read_reg(decoder, 0xFAC6);
    val &= 0x80;
    
    if ( val != 0 ) {
			goto reinit;
    }

	ark7116h_config_common(decoder);

     //soft reset decoder
    val = amt_read_reg(decoder,0xFDA0);
    val |= 1;
    amt_write_reg(decoder,0xFDA0, val);
    mdelay(40);
    val &= ~1;
    amt_write_reg(decoder,0xFDA0, val);
    mdelay(40);

	ark7116h_select_input(decoder, decoder->input);

	return 0;
}

/* ----------------------------------------------------------------------- */
static int ark7116h_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
    struct ark7116h *decoder = to_ark7116h(sd);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		amt_write_reg(decoder, ARK7116H_BRIGHT_CTL, ctrl->val);
		return 0;
	case V4L2_CID_CONTRAST:
		amt_write_reg(decoder, ARK7116H_CONTRAST_CTL, ctrl->val);
		return 0;
	case V4L2_CID_SATURATION:
		amt_write_reg(decoder, ARK7116H_SATURATION_CTL, ctrl->val);
		return 0;
	case V4L2_CID_HUE:
		amt_write_reg(decoder, ARK7116H_HUE_CTL, ctrl->val);
		return 0;
	}
	return -EINVAL;
}

static int ark7116h_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct ark7116h *decoder = to_ark7116h(sd);

	if (status) {
		*status = 0;
		if (!ark7116h_detect_signal(decoder))
			*status |= V4L2_IN_ST_NO_SIGNAL;
	}

	return 0;
}

static int ark7116h_s_routing(struct v4l2_subdev *sd, u32 input,
			     u32 output, u32 config)
{
	struct ark7116h *decoder = to_ark7116h(sd);
	ark7116h_select_input(decoder, input);
	decoder->input = input;
	return 0;
}
static int ark7116h_init(struct v4l2_subdev *sd, u32 val)
{
	struct ark7116h *decoder = to_ark7116h(sd);
	int ret;
	ret=_ark7116h_init(decoder);
	if(ret){
		printk(KERN_ALERT "_ark7116h_init error.\n");
	}
	return ret;
}

static long ark7116h_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
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
		*temp = 1;
		break;
	}
	case VIDIOC_GET_CHIPINFO:
	{
		int* temp = (int *)arg;
		*temp = TYPE_ARK7116H;
		break;
	}
	case VIDIOC_GET_ITU601_ENABLE:
	{
		int* temp = (int *)arg;
		*temp = 1;
		break;
	}
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops ark7116h_ctrl_ops = {
	.s_ctrl = ark7116h_s_ctrl,
};

static const struct v4l2_subdev_video_ops ark7116h_video_ops = {
	.g_input_status = ark7116h_g_input_status,
	.s_routing = ark7116h_s_routing,
};

static const struct v4l2_subdev_core_ops ark7116h_core_ops = {
	.init = ark7116h_init,
	.ioctl = ark7116h_ioctl,
};

static const struct v4l2_subdev_ops ark7116h_ops = {
	.core =  &ark7116h_core_ops,
	.video = &ark7116h_video_ops,
};

static int ark7116h_parse_dt(struct ark7116h *decoder, struct device_node *np)
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
			ret=_ark7116h_init(decoder);
			if(ret){
				printk(KERN_ALERT "_ark7116h_init error.\n");
			}
		}
	}

	return ret;
}

static int ark7116h_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct ark7116h *decoder;
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

	res = ark7116h_parse_dt(decoder, np);
	if (res) {
		dev_err(sd->dev, "DT parsing error: %d\n", res);
		return res;
	}

	v4l2_i2c_subdev_init(sd, client, &ark7116h_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	v4l2_ctrl_handler_init(&decoder->hdl, 4);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116h_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116h_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116h_ctrl_ops,
			V4L2_CID_SATURATION, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &ark7116h_ctrl_ops,
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
	
	printk("ark7116h_probe.\n");
	
	return 0;
}

static int ark7116h_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ark7116h *decoder = to_ark7116h(sd);

	dev_dbg(sd->dev, 
		"ark7116h.c: removing ark7116h adapter on address 0x%x\n",
		client->addr << 1);

	v4l2_async_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&decoder->hdl);

	return 0;
}

/* ----------------------------------------------------------------------- */
/* the length of name must less than 20 */
static const struct i2c_device_id ark7116h_id[] = {
	{ "ark7116h_1668e_devb", NULL},
	{ }
};
MODULE_DEVICE_TABLE(i2c, ark7116h_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ark7116h_of_match[] = {
	{ .compatible = "arkmicro,ark7116h_1668e_devb", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, ark7116h_of_match);
#endif

static struct i2c_driver ark7116h_driver = {
	.driver = {
		.of_match_table = of_match_ptr(ark7116h_of_match),
		.name	= "ark7116h",
	},
	.probe		= ark7116h_probe,
	.remove		= ark7116h_remove,
	.id_table	= ark7116h_id,
};

static int __init ark_7116h_init(void)
{
	return i2c_add_driver(&ark7116h_driver);
}

static void __exit ark_7116h_exit(void)
{
	i2c_del_driver(&ark7116h_driver);
}

device_initcall(ark_7116h_init);

MODULE_AUTHOR("arkmicro");
MODULE_DESCRIPTION("arkmicro 7116h decoder driver for v4l2");
MODULE_LICENSE("GPL v2");
