/*
 * rn6752 - Arkmicro rn6752 video decoder driver
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

extern int dvr_get_pragressive(void);
extern void dvr_restart(void);
//#define RN6752_CVBS_PAL_CHECK_ERR	//rn6752 signal check error
#define RN6752V1_CVBS_PAL_PROGRESSIVE	

struct rn6752 {
	struct v4l2_ctrl_handler hdl;
	struct v4l2_subdev sd;
	struct gpio_desc *reset_gpio;
	struct workqueue_struct *eq_queue;
	struct work_struct eq_work;
	struct i2c_client	*client;
	struct timer_list timer;
	volatile bool timer_start;
	volatile int timer_timeout;
	struct timer_list work_timer;
	int mode;
	int itu601in;
	int camera_mode;
	int progressive;
	int curr_channel;
	int signal;
	int id;
	volatile int enter_eq_work;
	int enter_auxin;
	int dvr_start;
	int enter_carback;
	int last_source;
	u32 input;
	u8 brightness;
	u8 contrast;
	u8 saturation;
	u8 hue;
	unsigned short default_addr;
};

static bool rn6752_dbg = false;

#define VIDIOC_GET_RESOLUTION  		_IOWR('V', BASE_VIDIOC_PRIVATE + 1, int)
#define VIDIOC_GET_PROGRESSIVE  		_IOWR('V', BASE_VIDIOC_PRIVATE + 2, int)
#define VIDIOC_GET_CHIPINFO  		_IOWR('V', BASE_VIDIOC_PRIVATE + 3, int)
#define VIDIOC_ENTER_CARBACK  		_IOWR('V', BASE_VIDIOC_PRIVATE + 4, int)
#define VIDIOC_EXIT_CARBACK			_IOWR('V', BASE_VIDIOC_PRIVATE + 5, int)
#define VIDIOC_GET_ITU601_ENABLE			_IOWR('V', BASE_VIDIOC_PRIVATE + 6, int)
#define VIDIOC_SET_AVIN_MODE			_IOWR('V', BASE_VIDIOC_PRIVATE + 7, int)

#define ARK_DVR_BRIGHTNESS_MASK		(1<<0)
#define ARK_DVR_CONTRAST_MASK		(1<<1)
#define ARK_DVR_SATURATION_MASK		(1<<2)
#define ARK_DVR_HUE_MASK			(1<<3)
#define ARK_DVR_SHARPNESS_MASK		(1<<4)


#define RN6752_STATUS			0x26
#define RN6752_BUS_STATUS		0xaf
#define RN6752_CONTRAST_CTL		0xd3
#define RN6752_BRIGHT_CTL		0xd4
#define RN6752_HUE_CTL			0xd5
#define RN6752_SATURATION_CTL	0xd6
#define RN6752_INPUT_CTL		0xdc

enum {
	RN6752_MODE_NONE,
	//CVBS mode
	RN6752_MODE_CVBS_PAL,
	RN6752_MODE_CVBS_NTSC,
	//add other mode
	
	//720P mode
	RN6752_MODE_720P_25FPS,
	RN6752_MODE_720P_30FPS,
	RN6752_MODE_1080P_25FPS,
	RN6752_MODE_1080P_30FPS,
	RN6752_MODE_END,
};

enum {
	RN6752_BRIGHTNESS_ADDR	= 0x01,
	RN6752_CONTRAST_ADDR	= 0x02,
	RN6752_SATURATION_ADDR	= 0x03,
	RN6752_HUE_ADDR 		= 0x04,
	RN6752_SHARPNESS_ADDR	= 0x05
};

enum {
	RN675X_ID_UNKNOWN,
	RN675X_ID_RN6752,
	RN675X_ID_RN6752M,
	RN675X_ID_RN6752V1,
	RN675X_ID_END
};

enum dvr_source {
	DVR_SOURCE_CAMERA,
	DVR_SOURCE_AUX,
	DVR_SOURCE_DVD,
};

enum {
	TYPE_UNKNOWN = -1,
	TYPE_CVBS = 0,
	TYPE_720P,
	TYPE_1080P,
};

enum carback_camera_mode {
 	CARBACK_CAMERA_MODE_DYNAMIC,
 	CARBACK_CAMERA_MODE_CVBS_PAL,
 	CARBACK_CAMERA_MODE_CVBS_NTST,
 	CARBACK_CAMERA_MODE_720P25,
 	CARBACK_CAMERA_MODE_720P30,
 	CARBACK_CAMERA_MODE_1080P25,
 	CARBACK_CAMERA_MODE_1080P30,
 	//add others mode.

 	CARBACK_CAMERA_MODE_END
};

enum {
	TYPE_UNDEF = -1,
	TYPE_ARK7116 = 0,
	TYPE_RN6752,
};

const char rxchip_rn6752_cvbs_pal[] = {
	// 720H@50, 27MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*

	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x00, // 720H resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0x09, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x22, // PAL format
	0x2F, 0x14, // internal use
	0x5E, 0x03, // disable H-scaling control
	0x5B, 0x00, //
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping //old:0x92
	0x00, 0x00, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value
//	0x05, 0x00, // sharpness
//	0x04, 0x80, // hue
	0x11, 0x03,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFF, // enable 720H format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // 720H mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};

const char rxchip_rn6752_cvbs_ntsc[] = {
	// 720H@60, 27MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x00, // 720H resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0x09, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // NTSC format
	0x2F, 0x14, // internal use
	0x5E, 0x03, // disable H-scaling control
	0x5B, 0x00, //
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x00, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value
//	0x05, 0x00, // sharpness
//	0x04, 0x80, // hue
	0x11, 0x03,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFF, // enable 720H format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // 720H mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};

const char rxchip_rn6752_720p_pal[]=
{
	// 720P@25, 72MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x02, // 720p resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0xBD, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x02, // data for extended register	:Pal
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // 720p format
	0x2F, 0x04, // internal use*
	0x5E, 0x0B, // enable H-scaling control
	0x51, 0x44, // scale factor1
	0x52, 0x86, // scale factor2
	0x53, 0x22, // scale factor3
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x20, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value
//	0x05, 0x00, // sharpness
//	0x04, 0x80, // hue
	0x11, 0x84,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFE, // enable 720p format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};

const char rxchip_rn6752_720p_ntsc[] = {
	// 720P@30, 72MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x02, // 720p resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0xBD, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x04, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // 720p format
	0x2F, 0x04, // internal use*
	0x5E, 0x0B, // enable H-scaling control
	0x51, 0x44, // scale factor1
	0x52, 0x86, // scale factor2
	0x53, 0x22, // scale factor3
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x20, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value
//	0x05, 0x00, // sharpness
//	0x04, 0x80, // hue
	0x11, 0x84,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFE, // enable 720p format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
};

static const char rn6752_itu656_cvbs_pal[]=
{
#if 1
	//\B5\A5\B3\A1\CA\E4\B3\F6\C5\E4\D6\C3(itu656)
	// 720H@50, 27MHz, BT601 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x00, // 720H resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0x09, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x22, // PAL format
	0x2F, 0x14, // internal use
	0x5E, 0x03, // disable H-scaling control
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x47, 0xC3, // for customer project
	0x41, 0x00,
	0x42, 0x00,
	0x20, 0x24,
	0x21, 0x46,
	0x22, 0xAF,
	0x23, 0X17,
	0x24, 0X37,
	0x25, 0X17,
	0x26, 0X00,
	0x28, 0xE2, // cropping
	0x00, 0x00, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x11, 0x03,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFF, // enable 720H format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // 720H mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#else
	//\u02eb\B3\A1\CA\E4\B3\F6\C5\E4\D6\C3(itu656)
	// 720H@50, 27MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x00, // 720H resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0x09, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x22, // PAL format
	0x2F, 0x14, // internal use
	0x5E, 0x03, // disable H-scaling control
	0x5B, 0x00, //
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x00, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x11, 0x03,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFF, // enable 720H format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // 720H mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#endif

};

static const char rn6752_itu656_cvbs_ntsc[]=
{
#if 0
	//\B5\A5\B3\A1\CA\E4\B3\F6\C5\E4\D6\C3(itu656)
	// 720H@60, 27MHz, BT601 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x00, // 720H resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0x09, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // NTSC format
	0x2F, 0x14, // internal use
	0x5E, 0x03, // disable H-scaling control
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x47, 0xC3, // for customer project
	0x41, 0x00,
	0x42, 0x00,
	0x20, 0x24,
	0x21, 0x43,
	0x22, 0xAC,
	0x23, 0X11,
	0x24, 0X01,
	0x25, 0X11,
	0x26, 0X01,
	0x28, 0xE2, // cropping
	0x00, 0x00, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value 
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x11, 0x03,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFF, // enable 720H format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // 720H mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#else
	//\u02eb\B3\A1\CA\E4\B3\F6\C5\E4\D6\C3(itu656)
	// 720H@60, 27MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x00, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x00, // 720H resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0x09, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // NTSC format
	0x2F, 0x14, // internal use
	0x5E, 0x03, // disable H-scaling control
	0x5B, 0x00, //
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x00, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value 
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x11, 0x03,
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFF, // enable 720H format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x00, // 720H mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x00, // select 27MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#endif
};

static const char rn6752_itu656_720p_pal[]=
{
#if 0
	// 720P@25, 72MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x02, // 720p resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0xBD, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // 720p format
	0x2F, 0x04, // internal use*
	0x5E, 0x0B, // enable H-scaling control
	0x51, 0x44, // scale factor1
	0x52, 0x86, // scale factor2
	0x53, 0x22, // scale factor3
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x20, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value 
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFE, // enable 720p format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#else
	//add 20211104
	// 720P@25, 72MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x02, // 720p resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0xBD, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // 720p format
	0x2F, 0x04, // internal use*
	0x5E, 0x0B, // enable H-scaling control
	0x51, 0x44, // scale factor1
	0x52, 0x86, // scale factor2
	0x53, 0x22, // scale factor3
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x20, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value 
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFE, // enable 720p format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#endif
};

static const char rn6752_itu656_720p_ntsc[]=
{
#if 0
	// 720P@30, 72MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x02, // 720p resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0xBD, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x04, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // 720p format
	0x2F, 0x04, // internal use*
	0x5E, 0x0B, // enable H-scaling control
	0x51, 0x44, // scale factor1
	0x52, 0x86, // scale factor2
	0x53, 0x22, // scale factor3
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x20, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value 
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFE, // enable 720p format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#else
	//add 20211104
	// 720P@30, 72MHz, BT656 output
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN675x is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDB, 0x8F, // internal use*
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x2C, 0x30, // select sync slice points
	0x50, 0x02, // 720p resolution select for BT.601
	0x56, 0x00, // disable SAV & EAV for BT601; 0x00 enable SAV & EAV for BT656
	0x63, 0xBD, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x04, // data for extended register
	0x58, 0x01, // enable extended register write
	0x07, 0x23, // 720p format
	0x2F, 0x04, // internal use*
	0x5E, 0x0B, // enable H-scaling control
	0x51, 0x44, // scale factor1
	0x52, 0x86, // scale factor2
	0x53, 0x22, // scale factor3
	0x3A, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x3E, 0x32, // AVID & VBLK out for BT.601
	0x40, 0x04, // no channel information insertion; invert VBLK for frame valid
	0x46, 0x23, // AVID & VBLK out for BT.601
	0x28, 0x92, // cropping
	0x00, 0x20, // internal use*
	0x2D, 0xF2, // cagc adjust
	0x0D, 0x20, // cagc initial value 
	0x05, 0x00, // sharpness
	0x04, 0x80, // hue
	0x37, 0x33,
	0x61, 0x6C,

	0xDF, 0xFE, // enable 720p format
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0xC1, // enable SCLK out
	0x81, 0x01, // turn on video decoder

	0x96, 0x00, // select AVID & VBLK as status indicator
	0x97, 0x0B, // enable status indicator out on AVID,VBLK & VSYNC 
	0x98, 0x00, // video timing pin status
	0x9A, 0x40, // select AVID & VBLK as status indicator 
	0x9B, 0xE1, // enable status indicator out on HSYNC
	0x9C, 0x00, // video timing pin status
#endif
};

static const char rn6752m_itu656_cvbs_pal[]=
{
	0x81, 0x01, // turn on video decoder
	0xA3, 0x04,
	0xDF, 0x0F, // enable CVBS format
	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x00, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x62, // HD format
	0x2A, 0x81, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x00, // sync control
	0x50, 0x00, // 720p resolution
	0x56, 0x01, // 72M mode and BT656 mode
	0x5F, 0x00, // blank level
	0x63, 0x75, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x5B, 0x00, // H-scaling control
	0x5E, 0x01, // enable H-scaling control
	0x6A, 0x00, // H-scaling control
	0x28, 0xB2, // cropping
	0x20, 0x24,
	0x23, 0x17,
	0x24, 0x37,
	0x25, 0x17,
	0x26, 0x00,
	0x42, 0x00,
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x03, // sharpness
	0x57, 0x20, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out

};

static const char rn6752m_itu656_cvbs_ntsc[] = {
	0x81, 0x01, // turn on video decoder
	0xA3, 0x04,
	0xDF, 0x0F, // enable CVBS format
	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x00, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x81, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x00, // sync control
	0x50, 0x00, // 720p resolution
	0x56, 0x01, // 72M mode and BT656 mode
	0x5F, 0x00, // blank level
	0x63, 0x75, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x5B, 0x00, // H-scaling control
	0x5E, 0x01, // enable H-scaling control
	0x6A, 0x00, // H-scaling control
//	0x28, 0xB2, // cropping   // rn6752M default:0xB2	//add by helen
	0x28, 0x92, // cropping   // rn6752V1 default:0x92
	0x20, 0x24,
	0x23, 0x11,
	0x24, 0x05,
	0x25, 0x11,
	0x26, 0x00,
	0x42, 0x00,
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x03, // sharpness
	0x57, 0x20, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out
};

const char rn6752m_itu656_720p_25fps[] = {
	// 720P@25 BT656
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDF, 0xFE, // enable HD format

	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x20, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x01, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x03, // sync control
	0x50, 0x02, // 720p resolution
	0x56, 0x01, // BT 72M mode
	0x5F, 0x40, // blank level
	0x63, 0xF5, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x42, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x23, // data for extended register
	0x58, 0x01, // enable extended register write
	0x51, 0xE1, // scale factor1
	0x52, 0x88, // scale factor2
	0x53, 0x12, // scale factor3
	0x5B, 0x07, // H-scaling control
	0x5E, 0x08, // enable H-scaling control
	0x6A, 0x82, // H-scaling control
	0x28, 0x92, // cropping
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x00, // sharpness
	0x57, 0x23, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,

	// VP1
	0x8E, 0x00, // single channel output for VP1
	0x8F, 0x80, // 720p mode for VP1
	0x8D, 0x31, // enable VP1 out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out
};

static const char rn6752v1_itu656_cvbs_pal[]=
{
	// D1@50 with mipi 2 data lanes + 1 clock lane out
	// pin24/23 data lane0, pin18/17 data lane3
	// pin16/15 data lane2, pin12/11 data lane1
	// pin14/13 clock lane
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04,
	0xDF, 0x0F, // enable CVBS format

	0x88, 0x00,
	0xF6, 0x00,

	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x00, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x62, // HD format
	0x2A, 0x81, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x00, // sync control
	0x50, 0x00, // 720p resolution
	0x56, 0x01, // 72M mode and BT656 mode
	0x5F, 0x00, // blank level
	0x63, 0x75, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x5B, 0x00, // H-scaling control
	0x5E, 0x01, // enable H-scaling control
	0x6A, 0x00, // H-scaling control
	0x28, 0xB2, // cropping     //\B5\A5\B3\A1\CA\E4\B3\F6:0xB2  \u02eb\B3\A1\CA\E4\B3\F6:0x92     //only for msn
	0x20, 0x24,
	0x23, 0x17,
	0x24, 0x37,
	0x25, 0x17,
	0x26, 0x00,
	0x42, 0x00,
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x03, // sharpness
	0x57, 0x20, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,

	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out
};

static const char rn6752v1_itu656_cvbs_ntsc[] = {
	0x81, 0x01, // turn on video decoder
	0xA3, 0x04,
	0xDF, 0x0F, // enable CVBS format
	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x00, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x81, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x00, // sync control
	0x50, 0x00, // 720p resolution
	0x56, 0x01, // 72M mode and BT656 mode
	0x5F, 0x00, // blank level
	0x63, 0x75, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x00, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x02, // data for extended register
	0x58, 0x01, // enable extended register write
	0x5B, 0x00, // H-scaling control
	0x5E, 0x01, // enable H-scaling control
	0x6A, 0x00, // H-scaling control
	0x28, 0xB2, // cropping
	0x20, 0x24,
	0x23, 0x11,
	0x24, 0x05,
	0x25, 0x11,
	0x26, 0x00,
	0x42, 0x00,
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x03, // sharpness
	0x57, 0x20, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,
	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 720p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out
};

static const char rn6752v1_itu656_720p_25fps[] = {
	// 720P@25 BT656
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0xF0, 0x1F,

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDF, 0xFE, // enable HD format

	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	//0xff,0x00;0x00,0x60	//rn6752v1_video_Test:color bars test pattern output
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x20, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x01, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x03, // sync control
	0x50, 0x02, // 720p resolution
	0x56, 0x01, // BT 72M mode
	0x5F, 0x40, // blank level
	0x63, 0xF5, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x42, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x23, // data for extended register
	0x58, 0x01, // enable extended register write
	0x51, 0xE1, // scale factor1
	0x52, 0x88, // scale factor2
	0x53, 0x12, // scale factor3
	0x5B, 0x07, // H-scaling control
	0x5E, 0x08, // enable H-scaling control
	0x6A, 0x82, // H-scaling control
	0x28, 0x92, // cropping
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x00, // sharpness
	0x57, 0x23, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,

	// VP1
	0x8E, 0x00, // single channel output for VP1
	0x8F, 0x80, // 720p mode for VP1
	0x8D, 0x31, // enable VP1 out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out
};


static const char rn6752m_itu656_720p_30fps[] = {
	// 720P@30 BT656
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, // enable 72MHz sampling
	0xDF, 0xFE, // enable HD format

	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x20, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x01, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x03, // sync control
	0x50, 0x02, // 720p resolution
	0x56, 0x01, // 72M mode and BT656 mode
	0x5F, 0x40, // blank level
	0x63, 0xF5, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x44, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x23, // data for extended register
	0x58, 0x01, // enable extended register write
	0x51, 0x4E, // scale factor1
	0x52, 0x87, // scale factor2
	0x53, 0x12, // scale factor3
	0x5B, 0x07, // H-scaling control
	0x5E, 0x08, // enable H-scaling control
	0x6A, 0x82, // H-scaling control
	0x28, 0x92, // cropping
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x00, // sharpness
	0x57, 0x23, // black/white stretch
	0x68, 0x32, // coring
	0x37, 0x33,
	0x61, 0x6C,

	// VP1
	0x8E, 0x00, // single channel output for VP1
	0x8F, 0x80, // 720p mode for VP1
	0x8D, 0x31, // enable VP1 out
	0x89, 0x09, // select 72MHz for SCLK
	0x88, 0x41, // enable SCLK out
};

static const char rn6752m_itu656_1080p_25fps[] = {
	// 1080P@25 BT656
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, //
	0xDF, 0xFE, // enable HD format
	0xF0, 0xC0,

	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	//0xff,0x00;0x00,0x60	//rn6752v1_video_Test:color bars test pattern output
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x20, //0x20 internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x01, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x03, // sync control
	0x50, 0x03, // 1080p resolution
	0x56, 0x02, // 144M and BT656 mode
	0x5F, 0x44, // blank level
	0x63, 0xF8, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x48, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x23, // data for extended register
	0x58, 0x01, // enable extended register write
	0x51, 0xF4, // scale factor1
	0x52, 0x29, // scale factor2
	0x53, 0x15, // scale factor3
	0x5B, 0x01, // H-scaling control
	0x5E, 0x08, // enable H-scaling control
	0x6A, 0x87, // H-scaling control
	0x28, 0x92, // cropping
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x04, // sharpness
	0x57, 0x23, // black/white stretch
	0x68, 0x00, // coring
	0x37, 0x33,
	0x61, 0x6C,

	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 1080p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x0A, // select 144MHz for SCLK
	0x88, 0x41, // enable SCLK out
};

static const char rn6752m_itu656_1080p_30fps[] = {
	// 1080P@30 BT656
	// Slave address is 0x58
	// Register, data

	// if clock source(Xin) of RN6752 is 26MHz, please add these procedures marked first
	//0xD2, 0x85, // disable auto clock detect
	//0xD6, 0x37, // 27MHz default
	//0xD8, 0x18, // switch to 26MHz clock
	//delay(100), // delay 100ms

	0x81, 0x01, // turn on video decoder
	0xA3, 0x04, //
	0xDF, 0xFE, // enable HD format
	0xF0, 0xC0,

	0x88, 0x40, // disable SCLK0B out
	0xF6, 0x40, // disable SCLK3A out

	// ch0
	//0xff,0x00;0x00,0x60	//rn6752v1_video_Test:color bars test pattern output
	0xFF, 0x00, // switch to ch0 (default; optional)
	0x00, 0x20, // internal use*
	0x06, 0x08, // internal use*
	0x07, 0x63, // HD format
	0x2A, 0x01, // filter control
	0x3A, 0x00, // No Insert Channel ID in SAV/EAV code
	0x3F, 0x10, // channel ID
	0x4C, 0x37, // equalizer
	0x4F, 0x03, // sync control
	0x50, 0x03, // 1080p resolution
	0x56, 0x02, // 144M and BT656 mode
	0x5F, 0x44, // blank level
	0x63, 0xF8, // filter control
	0x59, 0x00, // extended register access
	0x5A, 0x49, // data for extended register
	0x58, 0x01, // enable extended register write
	0x59, 0x33, // extended register access
	0x5A, 0x23, // data for extended register
	0x58, 0x01, // enable extended register write
	0x51, 0xF4, // scale factor1
	0x52, 0x29, // scale factor2
	0x53, 0x15, // scale factor3
	0x5B, 0x01, // H-scaling control
	0x5E, 0x08, // enable H-scaling control
	0x6A, 0x87, // H-scaling control
	0x28, 0x92, // cropping
	0x03, 0x80, // saturation
	0x04, 0x80, // hue
	0x05, 0x04, // sharpness
	0x57, 0x23, // black/white stretch
	0x68, 0x00, // coring
	0x37, 0x33,
	0x61, 0x6C,

	0x8E, 0x00, // single channel output for VP
	0x8F, 0x80, // 1080p mode for VP
	0x8D, 0x31, // enable VP out
	0x89, 0x0A, // select 144MHz for SCLK
	0x88, 0x43, // enable SCLK out   		//default:0x41  clock_Invert:0x43  add:2021-11-15
};


static inline struct rn6752 *to_rn6752(struct v4l2_subdev *sd)
{
	return container_of(sd, struct rn6752, sd);
}

static inline struct v4l2_subdev *to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct rn6752, hdl)->sd;
}

#if 0
static int rn6752_read(struct v4l2_subdev *sd, unsigned char addr)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	rc = i2c_smbus_read_byte_data(client, addr);
	if (rc < 0) {
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);
		return rc;
	}

	dev_dbg(sd->dev, "rn6752: read 0x%02x = %02x\n", addr, rc);

	return rc;
}

static int rn6752_write(struct v4l2_subdev *sd, unsigned char addr,
				 unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	dev_dbg(sd->dev, "rn6752: writing %02x %02x\n", addr, value);
	rc = i2c_smbus_write_byte_data(client, addr, value);
	if (rc < 0)
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);

	return rc;
}

static unsigned char amt_read_reg(struct rn6752 *decoder, unsigned int reg)
{
	struct v4l2_subdev *sd = &decoder->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	rc = i2c_smbus_read_byte_data(client, reg & 0xff);
	client->addr = decoder->default_addr;
	if (rc < 0) {
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);
		return rc;
	}
	dev_dbg(sd->dev, "rn6752: read 0x%04x = %02x\n", reg, rc);

	return rc;
}

static int amt_write_reg(struct rn6752 *decoder, unsigned int reg, unsigned char value)
{
	struct v4l2_subdev *sd = &decoder->sd;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int rc;

	dev_dbg(sd->dev, "rn6752: writing %04x %02x\n", reg, value);
	rc = i2c_smbus_write_byte_data(client, reg & 0xff, value);
	client->addr = decoder->default_addr;
	if (rc < 0)
		dev_err(sd->dev, "i2c i/o error: rc == %d\n", rc);

	return rc;	
}

#endif


/******************************************************************************************************/

static int rn6752_write_byte(struct rn6752 *decoder,unsigned char regaddr, unsigned char regval)
{
	struct i2c_client *client;
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
	
	if(!decoder)
		return -ENODEV;
		
	client = decoder->client;

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
		printk("ERR: %s failure\n",__FUNCTION__);
		return -EBUSY;
	}

	return 0;
}

static int rn6752_read_byte(struct rn6752 *decoder,unsigned char regaddr)
{
	struct i2c_client *client;
	struct i2c_msg read_msgs[2];
	s32 ret = -1;
	s32 retries = 0;
	u8 regValue = 0x00;

	if(!decoder)
		return -ENODEV;
		
	client = decoder->client;

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
	
    if(ret != 2)
    {
		printk("ERR: %s reg:0x%x failure\n",__FUNCTION__, regaddr);
		return -EBUSY;
	}
	return regValue;
}

static void rn6752_write_reg(struct rn6752 *decoder,const char *buf, int len)
{
	int i;

	for(i=0; i<len; i++)
		rn6752_write_byte(decoder,buf[2*i],buf[2*i+1]);
}

static void rn6752_read_reg(struct rn6752 *decoder,const char *buf, int len)
{
	int i;
	int regval;
	for(i=0; i<len; i++){
		regval = rn6752_read_byte(decoder,buf[2*i]);
		printk(KERN_ALERT "++++++reg_addr:0x%x , reg_val:0x%x\n", buf[2*i],regval);
	}
}

static void rn6752m_pre_init(struct rn6752 *decoder)
{
	int rom_byte1, rom_byte2, rom_byte3, rom_byte4, rom_byte5, rom_byte6;
	rn6752_write_byte(decoder,0xE1, 0x80);
	rn6752_write_byte(decoder,0xFA, 0x81);
	rom_byte1 = rn6752_read_byte(decoder,0xFB);
	rom_byte2 = rn6752_read_byte(decoder,0xFB);
	rom_byte3 = rn6752_read_byte(decoder,0xFB);
	rom_byte4 = rn6752_read_byte(decoder,0xFB);
	rom_byte5 = rn6752_read_byte(decoder,0xFB);
	rom_byte6 = rn6752_read_byte(decoder,0xFB);

	// config. decoder accroding to rom_byte5 and rom_byte6
	if((rom_byte6 == 0x00) && (rom_byte5 == 0x00))
	{
		rn6752_write_byte(decoder,0xEF, 0xAA);
		rn6752_write_byte(decoder,0xE7, 0xFF);
		rn6752_write_byte(decoder,0xFF, 0x09);
		rn6752_write_byte(decoder,0x03, 0x0C);
		rn6752_write_byte(decoder,0xFF, 0x0B);
		rn6752_write_byte(decoder,0x03, 0x0C);
	}
	else if(((rom_byte6 == 0x34) && (rom_byte5 == 0xA9)) || ((rom_byte6 == 0x2C) && (rom_byte5 == 0xA8)))
    {
		rn6752_write_byte(decoder,0xEF, 0xAA);
		rn6752_write_byte(decoder,0xE7, 0xFF);
		rn6752_write_byte(decoder,0xFC, 0x60);
		rn6752_write_byte(decoder,0xFF, 0x09);
		rn6752_write_byte(decoder,0x03, 0x18);
		rn6752_write_byte(decoder,0xFF, 0x0B);
		rn6752_write_byte(decoder,0x03, 0x18);
	}
	else
	{
		rn6752_write_byte(decoder,0xEF, 0xAA);
		rn6752_write_byte(decoder,0xFC, 0x60);
		rn6752_write_byte(decoder,0xFF, 0x09);
		rn6752_write_byte(decoder,0x03, 0x18);
		rn6752_write_byte(decoder,0xFF, 0x0B);
		rn6752_write_byte(decoder,0x03, 0x18);
	}
}

static int rn6752v1_input_signal_check(struct rn6752 *decoder)
{
	int count_bit4 = 0, check_sig = 0, check_bit4 = 0;
	int rn6752v1_signal_format, i;
	int signal_detect = 0;

	rn6752_write_byte(decoder,0x49,0x81);
	rn6752_write_byte(decoder,0x19,0x0a);
	for (i=0; i <= 4; i++ )
	{
		check_bit4 = rn6752_read_byte(decoder,0x00)>>4;
		//printk(KERN_ALERT "check_bit4 = 0x%02x\n",check_bit4);
		if (( check_bit4 & 0x1 ) == 0x0)
		{
			if(count_bit4++ >= 4)
			{
				check_sig = 1;
				//printk(KERN_ALERT "RN6752 check signal ok,i = %d\r\n",i);
				break;
			}
		}
		else
		{
			count_bit4 = 0;
		}
		msleep(50);//50
	}

	if(check_sig)
	{
		rn6752v1_signal_format = rn6752_read_byte(decoder,0x00) & 0xf1;

		switch (rn6752v1_signal_format)
		{
			case 0x00:
			case 0x80:
				signal_detect = RN6752_MODE_CVBS_PAL;
				break;
			case 0x01:
			case 0x81:
				signal_detect = RN6752_MODE_CVBS_NTSC;
				break;
			case 0x20:
			case 0xA0:
				signal_detect = RN6752_MODE_720P_25FPS;
				break;
			case 0x21:
			case 0xA1:
				signal_detect = RN6752_MODE_720P_30FPS;
				break;
			case 0x40:
			case 0xC0:
				signal_detect = RN6752_MODE_1080P_25FPS;
				break;
			case 0x41:
			case 0xC1:
				signal_detect = RN6752_MODE_1080P_30FPS;
				break;
			default:
				break;
		}
    }
	return signal_detect;
}

static int rn6752v1_detect_signal(struct rn6752 *decoder)
{
	int count_bit4 = 0, check_sig = 0, check_bit4 = 0;
	int rn6752v1_signal_format, i;
	int signal_detect = 0;
	rn6752_write_byte(decoder,0x49,0x81);
	rn6752_write_byte(decoder,0x19,0x0a);

	for (i=0; i <= 1; i++ )
	{
		check_bit4 = rn6752_read_byte(decoder,0x00)>>4;
		if (( check_bit4 & 0x1 ) == 0x0)
		{
				signal_detect = 0;
		}
		else
		{
			signal_detect = 1;
		}
	}

	//printk(KERN_ALERT "signal_detect = %d\n",signal_detect);
	return signal_detect;
}

static int rn6752m_signal_check(struct rn6752 *decoder)
{
	int status, i;
	static int prestatus = 0;
	int count = 0;
	int resolutuon_detect = 0;
	
	for(i=0; i<50; i++) //default:50  msn_6752v1:15
	{
		status = (rn6752_read_byte(decoder,0x00) & 0x7F);
		if(status < 0)
		{
			msleep(1);
			continue;
		}
		msleep(15);//default:25 msn_6752v1:15
		if(status == prestatus)
		{
			count++;
			if(count >= 20)// \C1\AC\D0\F8\B6\C110\B4\CE״ֵ̬\B6\BCһ\D1\F9\B2\C5\C8\CFΪ״ֵ̬\CAǿɿ\BF\B5\C4  //default:20   msn_6752v1:15
			{
				break;
			}
		}
		else
		{
			count = 0;
			prestatus = status;
		}
		msleep(1);
	}

	//printk(KERN_ALERT "### COUNT:%d, status:0x%x\n", count , status);
	if(status & (1<<4))
	{
		resolutuon_detect = RN6752_MODE_NONE;
	}
	else
	{
		switch (status&0x71)
		{
			case 0X20:
				//720P 25
				resolutuon_detect = RN6752_MODE_720P_25FPS;
				break;
			case 0x21:
				//720P 30
				resolutuon_detect = RN6752_MODE_720P_30FPS;
				break;
			case 0X40:
				//1080P 25
				resolutuon_detect = RN6752_MODE_1080P_25FPS;
				break;
			case 0x41:
				//1080P 30
				resolutuon_detect = RN6752_MODE_1080P_30FPS;
				break;
			case 0x00:
				//PAL
				resolutuon_detect = RN6752_MODE_CVBS_PAL;
				break;
			case 0x01:
				//NTSC
				resolutuon_detect = RN6752_MODE_CVBS_NTSC;
				break;
			break;
				default:
				break;
		}
	}
	return resolutuon_detect;
}

static int rn6752_signal_check(struct rn6752 *decoder)
{
	u8 signal_cnt = 0;
	u8 nosignal_cnt=0;
	u8 reg_0x75,reg_0x77, reg_0x78, reg_0x79;
	u16 counter1 = 0, counter2 = 0, counter3 = 0;
	u16 PAL_MIN_COUNT = 320;//330   310
	int i;
	int ret;
	static int counter_deviation = 10;

	for(i=0; i<30; i++)
	{
		ret = rn6752_read_byte(decoder,0x00);
		//printk(">>>>>>>>>>>>>>>>>>reg 0x00 :%x\r\n", ret);
		if((ret >= 0) && ((ret&0x10) == 0x00))
		{
			signal_cnt++;
		}
		else
		{
			nosignal_cnt++;
		}

		if(signal_cnt >= 15)
		{
			//printk(KERN_ALERT "### >i:%d\r\n", i);
			reg_0x77 = rn6752_read_byte(decoder,0x77);
			reg_0x78 = rn6752_read_byte(decoder,0x78);
			reg_0x79 = rn6752_read_byte(decoder,0x79);
			reg_0x75 = rn6752_read_byte(decoder,0x75);

			//counter1 = 0;
			counter1 = reg_0x77&0x03;
			counter1 <<= 8;
			counter1 |= reg_0x78;

			//counter2 = 0;
			counter2 = reg_0x77&0xc;
			counter2 >>= 2;
			counter2 <<= 8;
			counter2 |= reg_0x79;

			counter3 = reg_0x75;

			break;
		}

		if(nosignal_cnt >= 20)
		{
			return RN6752_MODE_NONE;
		}
		msleep(1);
	}

	if(signal_cnt < 15)
		return RN6752_MODE_NONE;
	
	printk("counter1 = %d , counter2 = %d\n",counter1,counter2);
	if( (counter1 > 700) ||(counter2 > 700))
	{
		//720p pal
	    if(counter3 > 0x8c)
		      return RN6752_MODE_720P_25FPS;
		else
		     return RN6752_MODE_720P_30FPS;
	}
	else if(((counter1>PAL_MIN_COUNT) && (counter1<550)) || ((counter2>PAL_MIN_COUNT) && (counter2<550)))
	{
#ifdef RN6752_CVBS_PAL_CHECK_ERR
		rn6752_cvbs_pal_flag = true;
#endif
		//cvbs pal
		return RN6752_MODE_CVBS_PAL;
	}
	//else if( (counter1<330) && (counter2<330) )
	else if((counter1<PAL_MIN_COUNT) && (counter2<PAL_MIN_COUNT))
	{
#ifdef RN6752_CVBS_PAL_CHECK_ERR
		if(rn6752_cvbs_pal_flag)
			goto err;
		rn6752_cvbs_pal_flag = false;
#endif
		//cvbs ntsc
		return RN6752_MODE_CVBS_NTSC;
	}

#ifdef RN6752_CVBS_PAL_CHECK_ERR
err:
	if(((counter1+counter_deviation>PAL_MIN_COUNT) && (counter1+counter_deviation<550)) || ((counter2+counter_deviation>PAL_MIN_COUNT) && (counter2+10<550)))
	{
		//printk("counter1 + counter_deviation = %d , counter2 + counter_deviation = %d\n",counter1+counter_deviation,counter2+counter_deviation);
		//cvbs pal
		return RN6752_MODE_CVBS_PAL;
	}
	rn6752_cvbs_pal_flag = false;
#endif
	return RN6752_MODE_NONE ;
}

static  char * rn6752_get_mode_string (int mode)
{
	if(mode == RN6752_MODE_NONE)
	{
		return "NONE";
	}
	else if(mode == RN6752_MODE_CVBS_PAL)
	{
		return "CVBS_PAL";
	}
	else if(mode == RN6752_MODE_CVBS_NTSC)
	{
		return "CVBS_NTSC";
	}
	else if(mode == RN6752_MODE_720P_25FPS)
	{
		return "720_PAL";
	}
	else if(mode == RN6752_MODE_720P_30FPS)
	{
		return "720_NTSC";
	}
	else if(mode == RN6752_MODE_1080P_25FPS)
	{
		return "1080P_25FPS";
	}
	else if(mode == RN6752_MODE_1080P_30FPS)
	{
		return "1080P_30FPS";
	}

	return "NONE";
}

static void rn6752_test_and_dvr_restart(struct rn6752 *decoder,int mode)
{
	int progressive = dvr_get_pragressive();
	int restart = 0;

	switch(mode)
	{
		case RN6752_MODE_CVBS_PAL:
#ifdef RN6752V1_CVBS_PAL_PROGRESSIVE
		if(progressive == 0)
			restart = 1;
#else
		if(progressive == 1)
			restart = 1;
#endif
			break;
		case RN6752_MODE_CVBS_NTSC:
			if(progressive == 1)
				restart = 1;
			break;
		case RN6752_MODE_720P_25FPS:
		case RN6752_MODE_720P_30FPS:
		case RN6752_MODE_1080P_25FPS:
		case RN6752_MODE_1080P_30FPS:
			if(progressive == 0)
				restart = 1;
			break;
		default:
			break;
	}

	if(restart)
	{
		int source = decoder->enter_carback ? DVR_SOURCE_CAMERA : DVR_SOURCE_AUX;

		if(rn6752_dbg) {
			printk(KERN_ALERT "### mode(%s) does not match progressive(%d), itu656 dvr_restart(%d)\n",
				rn6752_get_mode_string(mode), progressive, source);
		}
		dvr_restart();
	}
}

static int rn6752_init_reg_cfg(struct rn6752 *decoder,int curr_cfg)
{
	static int mode_cfg = RN6752_MODE_NONE;
	
	if(decoder) 
	{
		if(decoder->camera_mode > 0){
			if(decoder->id == RN675X_ID_RN6752){
			}
			else if(decoder->id == RN675X_ID_RN6752M){
				rn6752m_pre_init(decoder);
			}
			else if(decoder->id == RN675X_ID_RN6752V1){
				rn6752m_pre_init(decoder);
			}
		}
	
		switch(curr_cfg)
		{
			case CARBACK_CAMERA_MODE_CVBS_PAL:
				{
					//printk(KERN_ALERT "++++++++++++++CARBACK_CAMERA_MODE_CVBS_PAL\n");
					if(decoder->id == RN675X_ID_RN6752)
						rn6752_write_reg(decoder,rn6752_itu656_cvbs_pal, sizeof(rn6752_itu656_cvbs_pal)/2);//new add
					else if(decoder->id == RN675X_ID_RN6752M)
						rn6752_write_reg(decoder,rn6752m_itu656_cvbs_pal, sizeof(rn6752m_itu656_cvbs_pal)/2);
					else if(decoder->id == RN675X_ID_RN6752V1)
						rn6752_write_reg(decoder,rn6752v1_itu656_cvbs_pal, sizeof(rn6752v1_itu656_cvbs_pal)/2);
					mode_cfg = RN6752_MODE_CVBS_PAL;
#ifdef RN6752V1_CVBS_PAL_PROGRESSIVE
					decoder->progressive = 1;//only for msn (1:progressive scanning  0:interlaced scanning)
#else
					decoder->progressive = 0;
#endif
				}
				break;
			case CARBACK_CAMERA_MODE_CVBS_NTST:
				{
					//printk(KERN_ALERT "++++++++++++++CARBACK_CAMERA_MODE_CVBS_NTST\n");
					if(decoder->id == RN675X_ID_RN6752)
						rn6752_write_reg(decoder,rn6752_itu656_cvbs_ntsc, sizeof(rn6752_itu656_cvbs_ntsc)/2);//new add 
					else if(decoder->id == RN675X_ID_RN6752M)
						rn6752_write_reg(decoder,rn6752m_itu656_cvbs_ntsc, sizeof(rn6752m_itu656_cvbs_ntsc)/2);
					else if(decoder->id == RN675X_ID_RN6752V1)
						rn6752_write_reg(decoder,rn6752v1_itu656_cvbs_ntsc, sizeof(rn6752v1_itu656_cvbs_ntsc)/2);
					mode_cfg = RN6752_MODE_CVBS_NTSC;
					decoder->progressive = 0;
				}
				break;
			case CARBACK_CAMERA_MODE_720P25:
				{
					//printk(KERN_ALERT "++++++++++++++CARBACK_CAMERA_MODE_720P25\n");
					if(decoder->id == RN675X_ID_RN6752){
						rn6752_write_reg(decoder,rn6752_itu656_720p_pal, sizeof(rn6752_itu656_720p_pal)/2);//add 20211104
					}
					else if(decoder->id == RN675X_ID_RN6752M){
						rn6752_write_reg(decoder,rn6752m_itu656_720p_25fps, sizeof(rn6752m_itu656_720p_25fps)/2);}
					else if(decoder->id == RN675X_ID_RN6752V1){
						//printk(KERN_ALERT "++++++++++++++write rn6752v1_itu656_720p_25fps\n");
						rn6752_write_reg(decoder,rn6752v1_itu656_720p_25fps, sizeof(rn6752v1_itu656_720p_25fps)/2);
						}
					mode_cfg = RN6752_MODE_720P_25FPS;
					decoder->progressive = 1;
				}
				break;
			case CARBACK_CAMERA_MODE_720P30:
				{
					//printk(KERN_ALERT "++++++++++++++CARBACK_CAMERA_MODE_720P30\n");
					if(decoder->id == RN675X_ID_RN6752)
						rn6752_write_reg(decoder,rn6752_itu656_720p_ntsc, sizeof(rn6752_itu656_720p_ntsc)/2);//add 20211104
					else if(decoder->id == RN675X_ID_RN6752M){
						rn6752_write_reg(decoder,rn6752m_itu656_720p_30fps, sizeof(rn6752m_itu656_720p_30fps)/2);}
					else if(decoder->id == RN675X_ID_RN6752V1){
						rn6752_write_reg(decoder,rn6752m_itu656_720p_30fps, sizeof(rn6752m_itu656_720p_30fps)/2);}
					mode_cfg = RN6752_MODE_720P_30FPS;
					decoder->progressive = 1;
				}
				break;
			case CARBACK_CAMERA_MODE_1080P25:
				{
					//printk(KERN_ALERT "++++++++++++++CARBACK_CAMERA_MODE_1080P25\n");
					if(decoder->id == RN675X_ID_RN6752V1)
						rn6752_write_reg(decoder,rn6752m_itu656_1080p_25fps, sizeof(rn6752m_itu656_1080p_25fps)/2);
					mode_cfg = RN6752_MODE_1080P_25FPS;
					decoder->progressive = 1;
				}
				break;
			case CARBACK_CAMERA_MODE_1080P30:
				{
					//printk(KERN_ALERT "++++++++++++++CARBACK_CAMERA_MODE_1080P30\n");
					if(decoder->id == RN675X_ID_RN6752V1)
						rn6752_write_reg(decoder,rn6752m_itu656_1080p_30fps, sizeof(rn6752m_itu656_1080p_30fps)/2);
					mode_cfg = RN6752_MODE_1080P_30FPS;
					decoder->progressive = 1;
				}
				break;
			case RN6752_MODE_NONE:
			default:
				{
					//printk(KERN_ALERT "++++++++++++++RN6752_MODE_NONE\n");
					if(decoder->id == RN675X_ID_RN6752)
						rn6752_write_reg(decoder,rn6752_itu656_720p_ntsc, sizeof(rn6752_itu656_720p_ntsc)/2);
					else if(decoder->id == RN675X_ID_RN6752M)
						rn6752_write_reg(decoder,rn6752m_itu656_720p_25fps, sizeof(rn6752m_itu656_720p_25fps)/2);
					else if(decoder->id == RN675X_ID_RN6752V1)
						rn6752_write_reg(decoder,rn6752v1_itu656_720p_25fps, sizeof(rn6752v1_itu656_720p_25fps)/2);
					mode_cfg = RN6752_MODE_720P_30FPS;
					decoder->progressive = 1;
				}
				break;
		}
	} 
	return mode_cfg;
}

static void rn6752_eq_work(struct work_struct *work)
{
	struct rn6752 *decoder = container_of(work, struct rn6752, eq_work);
	static int mode_cfg = RN6752_MODE_NONE;
	static int curr_cfg = RN6752_MODE_NONE;
	static int check_count = 0;
	if(!decoder)
		goto end;
	decoder->enter_eq_work = 1;
	
	if(mode_cfg == RN6752_MODE_NONE)
	{
		if(decoder->camera_mode > 0){
			mode_cfg = rn6752_init_reg_cfg(decoder,decoder->camera_mode);
			curr_cfg = mode_cfg;
			if(mode_cfg)
				goto end_1; 
		}

#if 0
		if(rn6752_dbg)
			printk(KERN_ALERT "### rn6752_eq_work rn6752x reset\n");
		//reset
		rn6752_reset(dvr_rn6752->gpio_reset);
		//check id
		if(rn6752_check_id(dvr_rn6752))
			goto end;
#endif
		//printk("----------------->rn6752 Dynamic detect mode<-----------------\n");
		//720p cfg: before auto match, we must config 720p mode, because the default clk config is based on 720P
		if(decoder->id == RN675X_ID_RN6752)
		{
			//rn6752_write_reg(decoder,rxchip_rn6752_720p_pal, sizeof(rxchip_rn6752_720p_pal)/2);
			rn6752_write_reg(decoder,rn6752_itu656_720p_ntsc, sizeof(rn6752_itu656_720p_ntsc)/2);
		}
		else if(decoder->id == RN675X_ID_RN6752M)
		{
			rn6752m_pre_init(decoder);
			rn6752_write_reg(decoder,rn6752m_itu656_720p_25fps, sizeof(rn6752m_itu656_720p_25fps)/2);
		}
		else if(decoder->id == RN675X_ID_RN6752V1)
		{
			rn6752m_pre_init(decoder);
			rn6752_write_reg(decoder,rn6752v1_itu656_720p_25fps, sizeof(rn6752v1_itu656_720p_25fps)/2);
		}
		mode_cfg = RN6752_MODE_720P_30FPS;
		decoder->progressive = 1;

#ifdef CONFIG_RN6752_LOW_POWER_MODE
		if(!decoder->enter_carback && !decoder->enter_auxin)
		{
			rn6752_power_off();
		}
#endif
		goto end;
	}

	if(decoder->id == RN675X_ID_RN6752)
	{
		curr_cfg = rn6752v1_input_signal_check(decoder);
	}
	else if(decoder->id == RN675X_ID_RN6752M)
	{
		curr_cfg = rn6752m_signal_check(decoder);
		if(curr_cfg == RN6752_MODE_NONE)
		{
			msleep(10);
			curr_cfg = rn6752m_signal_check(decoder);
		}
	}
	else if(decoder->id == RN675X_ID_RN6752V1)
	{
		curr_cfg = rn6752v1_input_signal_check(decoder);
	}

	if(!decoder->enter_carback)
	{
exit:
#ifdef CONFIG_RN6752_LOW_POWER_MODE
		rn6752_power_off();
#endif
		if(rn6752_dbg)
			printk(KERN_ALERT "### %s exit without in carback or auxin\n", __FUNCTION__);
		//avoid recognize a wrong format, so default format should be 720P.
		//if(dvr_rn6752->last_source != DVR_SOURCE_CAMERA)
		{
			if(mode_cfg != RN6752_MODE_720P_30FPS)
			{
				if(decoder->id == RN675X_ID_RN6752){
					rn6752_write_reg(decoder,rn6752_itu656_720p_ntsc, sizeof(rn6752_itu656_720p_ntsc)/2);
				}
				else if(decoder->id == RN675X_ID_RN6752M){
					rn6752_write_reg(decoder,rn6752m_itu656_720p_25fps, sizeof(rn6752m_itu656_720p_25fps)/2);}
				else if(decoder->id == RN675X_ID_RN6752V1){
					rn6752_write_reg(decoder,rn6752v1_itu656_720p_25fps, sizeof(rn6752v1_itu656_720p_25fps)/2);
					}
				mode_cfg = RN6752_MODE_720P_30FPS;
				decoder->progressive = 1;
			}
			decoder->mode = RN6752_MODE_NONE;
		}
		goto end;
	}

	if(rn6752_dbg)
		printk(KERN_ALERT "rn6752%s mode:%s, mode_cfg:%s\n", (decoder->id==RN675X_ID_RN6752M)?"m":"v1", rn6752_get_mode_string(curr_cfg), rn6752_get_mode_string(mode_cfg));

	//printk("mode_cfg = %d,curr_cfg = %d\n",mode_cfg,curr_cfg);
	if((curr_cfg == RN6752_MODE_NONE) || (mode_cfg != curr_cfg))
	{
		if(rn6752_dbg)
			printk(KERN_ALERT "### rn6752 change mode to (%s)\n", rn6752_get_mode_string(curr_cfg));
		mode_cfg = rn6752_init_reg_cfg(decoder,curr_cfg);
	}
end_1:
	rn6752_test_and_dvr_restart(decoder,mode_cfg);
eixt_1:
	decoder->mode = curr_cfg;
end:
	decoder->enter_eq_work = 0;
}

static int rn6752_check_id(struct rn6752 *decoder)
{
	int id = -1;
	int ret;

	if(!decoder)
		goto err_check_id;

	ret = rn6752_read_byte(decoder,0xfe);
	if(ret < 0)
		goto err_check_id;
	id = (ret<<8);

	ret = rn6752_read_byte(decoder,0xfd);
	if(ret < 0)
		goto err_check_id;
	id |= ret;
	if(id == 0x401) {
		decoder->id = RN675X_ID_RN6752;
		printk(KERN_ALERT "AHD IC: RN6752\n");
	} else if(id == 0x501) {
		decoder->id = RN675X_ID_RN6752M;
		printk(KERN_ALERT "AHD IC: RN6752M\n");
	}else if(id == 0x2601) {
		decoder->id = RN675X_ID_RN6752V1;//RN675X_ID_RN6752M
		printk(KERN_ALERT "AHD IC: RN6752V1\n");
	}

	return 0;
err_check_id:
	printk(KERN_ERR "***ERR: %s failed, id:%d, ret:%d\n", __FUNCTION__, id, ret);
	decoder->id = RN675X_ID_UNKNOWN;
	return -ENODEV;
}

static void rn6752_reset(struct rn6752 *decoder)
{	
	//sw reset
	rn6752_write_byte(decoder,0x80, 0x31); //soft reset
	msleep(100);
	rn6752_write_byte(decoder,0x80, 0x30); //reset complete
	
	if(decoder && (decoder->curr_channel >= 0))
		rn6752_write_byte(decoder,0xD3, decoder->curr_channel);
	rn6752_write_byte(decoder,0x1A, 0x83);	//disable blue screen       
}

static void rn6752_work_timer(struct timer_list *t)
{
	struct rn6752 *decoder = from_timer(decoder, t, work_timer);
	static int flag_signal = 0;
	static int count_signal = 0;
	static int flag_no_signal = 0;
	static int count_no_signal = 0;
	if(!decoder->signal){
		if(!flag_signal){
			if(!decoder->enter_eq_work){
				queue_work(decoder->eq_queue, &decoder->eq_work);
			}
			count_signal ++;
			if(count_signal == 20){
				flag_signal = 1;
			}
		}
		flag_no_signal = 0;
		count_no_signal = 0;
	}
	if(decoder->signal == V4L2_IN_ST_NO_SIGNAL){
		if(!flag_no_signal){
			if(!decoder->enter_eq_work){
				queue_work(decoder->eq_queue, &decoder->eq_work);
			}
			count_no_signal ++;
			if(count_no_signal == 20){
				flag_no_signal = 1;
			}
		}
		flag_signal = 0;
		count_signal  = 0;
	}
	
	mod_timer(&decoder->work_timer, jiffies +  msecs_to_jiffies(100));
}

static void rn6752_timeout_timer(struct timer_list *t)
{
	//printk(KERN_ALERT "rn6752_timeout_timer entry\n");
	struct rn6752 *decoder = from_timer(decoder, t, timer);
	if(decoder)
	{
		if(!decoder->enter_eq_work)
			queue_work(decoder->eq_queue, &decoder->eq_work);

		if(decoder->timer_timeout > 0)
		{
			decoder->timer_timeout --;
			mod_timer(&decoder->timer, jiffies +  msecs_to_jiffies(100));
		}
		else
		{
			decoder->timer_start = false;
		}
	}
}

static void rn6752_start_timer(struct rn6752 *decoder,int timeout_100ms)
{
	//speed video recognise
	if(decoder)
	{
		decoder->timer_timeout = timeout_100ms;
		if(!decoder->timer_start)
		{
			decoder->timer_start = true;
			mod_timer(&decoder->timer, jiffies + msecs_to_jiffies(1));
		}
	}
}

/* ----------------------------------------------------------------------- */

static int _rn6752_init(struct rn6752 *decoder)
{
	rn6752_reset(decoder);
	rn6752_check_id(decoder);
	return 0;
}

static int rn6752_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct v4l2_subdev *sd = to_sd(ctrl);
	struct rn6752 *decoder = to_rn6752(sd);

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		ctrl->val &= 0xFF;
		rn6752_write_byte(decoder,RN6752_BRIGHTNESS_ADDR, ctrl->val);
		return 0;
	case V4L2_CID_CONTRAST:
		ctrl->val &= 0xFF;
		rn6752_write_byte(decoder,RN6752_CONTRAST_ADDR, ctrl->val);
		return 0;
	case V4L2_CID_SATURATION:
		ctrl->val &= 0xFF;
		rn6752_write_byte(decoder,RN6752_SATURATION_ADDR, ctrl->val);
		return 0;
	case V4L2_CID_HUE:
		ctrl->val &= 0xFF;
		rn6752_write_byte(decoder,RN6752_HUE_ADDR, ctrl->val);
		return 0;
	}
	return -EINVAL;
}

static int rn6752_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct rn6752 *decoder = to_rn6752(sd);
	static int curr_cfg = RN6752_MODE_NONE;
	int ret;
	if(rn6752_dbg)
		printk(KERN_ALERT "### rn6752_detect_signal\n");

	if(decoder)
	{
		if (status) {
			*status = 0;

			if(decoder->id == RN675X_ID_RN6752)
			{
				//return ((g_dvr_rn6752->mode != RN6752_MODE_NONE) ? 1 : 0);
				if(decoder->mode != RN6752_MODE_NONE)
				{
					ret = rn6752_read_byte(decoder,0x00);
					if(ret >= 0)
					{
						if((ret & 0x10) == 0)
						{
							*status |= V4L2_IN_ST_NO_SIGNAL;
						}
					}
				}
			}
			else if(decoder->id == RN675X_ID_RN6752M)
			{
				return ((decoder->mode != RN6752_MODE_NONE) ? 1 : 0);
			}
			else if(decoder->id == RN675X_ID_RN6752V1)
			{
				if (rn6752v1_detect_signal(decoder))
				*status |= V4L2_IN_ST_NO_SIGNAL;
			}
		}
	}

	decoder->signal = *status;
	return 0;
}

static int rn6752_select_input(struct rn6752 *decoder, u32 input)
{
	int ret;
	if(rn6752_dbg)
		printk(KERN_ALERT "### rn6752_select_channel ch:%d\n", input);

	if((input >= 0) && (input <= 1))
	{
		if(decoder) {
			decoder->mode = RN6752_MODE_NONE;
			
			ret = rn6752_read_byte(decoder,0xD3);
			if(ret >= 0)
			{
				if(ret == input)
				{
					if(rn6752_dbg)
						printk(KERN_ALERT "### %s, same ch:%d, ignore\n", __FUNCTION__, input);
					return 0;
				}
			}
			if(rn6752_write_byte(decoder,0xD3, input) == 0)
				decoder->curr_channel = input;
		}
	}

	return 0;
}


static int rn6752_s_routing(struct v4l2_subdev *sd, u32 input,
			     u32 output, u32 config)
{
	struct rn6752 *decoder = to_rn6752(sd);
	//printk(KERN_ALERT "rn6752_select_channel %d \n",input);
	rn6752_select_input(decoder, input);
	decoder->input = input;
	return 0;
}

static int rn6752_init(struct v4l2_subdev *sd, u32 val)
{
	struct rn6752 *decoder = to_rn6752(sd);
	int ret;
	ret=_rn6752_init(decoder);
	if(ret){
		printk(KERN_ALERT "_rn6752_init error.\n");
	}
	return ret;
}

static long rn6752_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct rn6752 *decoder = to_rn6752(sd);
	int ret = 0;

	switch (cmd) {
	case VIDIOC_GET_RESOLUTION:
	{
		int* temp = (int *)arg;
		*temp = TYPE_1080P;
		break;
	}
	case VIDIOC_GET_PROGRESSIVE:
	{
		int* temp = (int *)arg;
		int progressive = 1;

		if(rn6752_dbg)
			printk(KERN_ALERT "### itu656 get rn6752 progressive:%d\n", progressive);
		*temp = decoder->progressive;
		break;
	}
	case VIDIOC_GET_CHIPINFO:
	{
		int* temp = (int *)arg;
		*temp = TYPE_RN6752;
		break;
	}
	case VIDIOC_ENTER_CARBACK:
	{
		rn6752_start_timer(decoder,20);
		decoder->enter_carback = 1;
		break;
	}
	case VIDIOC_EXIT_CARBACK:
	{
		decoder->enter_carback = 0;
		rn6752_start_timer(decoder,20);
		break;
	}
	case VIDIOC_GET_ITU601_ENABLE:
	{
		int* temp = (int *)arg;
		*temp = 0;
		break;
	}
	case VIDIOC_SET_AVIN_MODE:
	{
		rn6752_reset(decoder);
		break;
	}
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops rn6752_ctrl_ops = {
	.s_ctrl = rn6752_s_ctrl,
};

static const struct v4l2_subdev_video_ops rn6752_video_ops = {
	.g_input_status = rn6752_g_input_status,
	.s_routing = rn6752_s_routing,
};

static const struct v4l2_subdev_core_ops rn6752_core_ops = {
	.init = rn6752_init,
	.ioctl = rn6752_ioctl,
};

static const struct v4l2_subdev_ops rn6752_ops = {
	.core =  &rn6752_core_ops,
	.video = &rn6752_video_ops,
};

static int rn6752_parse_dt(struct rn6752 *decoder, struct device_node *np)
{
	int ret = 0;
	int value;
	if(!of_property_read_u32(np, "default-channel", &value)) {
		decoder->curr_channel = value;
	} else {
		decoder->curr_channel = 0;
	}

	if(!of_property_read_u32(np, "camera-format", &value)) {
		decoder->camera_mode = value;
	} else {
		decoder->camera_mode = RN6752_MODE_NONE;
	}
	
	if(!of_property_read_u32(np, "itu601in", &value)) {
		decoder->itu601in = value;
	} else {
		decoder->itu601in = 0;
	}
	
	_rn6752_init(decoder);

	return ret;
}

static void rn6752_set_display_effect_default(struct rn6752 *decoder)
{
	rn6752_write_byte(decoder,RN6752_BRIGHTNESS_ADDR, (0x80 & 0xFF));
	rn6752_write_byte(decoder,RN6752_CONTRAST_ADDR, (0x80 & 0xFF));
	rn6752_write_byte(decoder,RN6752_SATURATION_ADDR, (0x00 & 0xFF));
	rn6752_write_byte(decoder,RN6752_HUE_ADDR, (0x00 & 0xFF));
	//amt_write_reg(decoder,RN6752_SHARPNESS_ADDR, ((rn6752_effect.carback_sharpness | 0x80) & 0xFF));
}

static int rn6752_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct rn6752 *decoder;
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
	decoder->mode = RN6752_MODE_NONE;
	decoder->camera_mode = 0;
	decoder->signal = V4L2_IN_ST_NO_SIGNAL;
	decoder->curr_channel = 0;
	sd = &decoder->sd;
	//decoder->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
	//					   GPIOD_OUT_HIGH);
	//if (IS_ERR(decoder->reset_gpio)) {
	//	res = PTR_ERR(decoder->reset_gpio);
	//	v4l_err(client, "request for reset pin failed: %d\n", res);
	//	return res;
//	}
	
	decoder->eq_queue = create_singlethread_workqueue("rn6752_eq_queue");
	if(decoder->eq_queue)
	{
		INIT_WORK(&decoder->eq_work, rn6752_eq_work);
	}

	v4l2_i2c_subdev_init(sd, client, &rn6752_ops);

	res = rn6752_parse_dt(decoder, np);
	if (res) {
		dev_err(sd->dev, "DT parsing error: %d\n", res);
		return res;
	}
	

#if 0
	decoder->eq_queue = create_singlethread_workqueue("rn6752_eq_queue");
	if(decoder->eq_queue)
	{
		INIT_WORK(&decoder->eq_work, rn6752_eq_work);
	}

	queue_work(decoder->eq_queue, &decoder->eq_work);
#endif

	decoder->timer_start = false;
	timer_setup(&decoder->timer, rn6752_timeout_timer,0);

	timer_setup(&decoder->work_timer, rn6752_work_timer,0);
	
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	v4l2_ctrl_handler_init(&decoder->hdl, 4);
	v4l2_ctrl_new_std(&decoder->hdl, &rn6752_ctrl_ops,
			V4L2_CID_BRIGHTNESS, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &rn6752_ctrl_ops,
			V4L2_CID_CONTRAST, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &rn6752_ctrl_ops,
			V4L2_CID_SATURATION, 0, 255, 1, 128);
	v4l2_ctrl_new_std(&decoder->hdl, &rn6752_ctrl_ops,
			V4L2_CID_HUE, -128, 127, 1, 0);
	sd->ctrl_handler = &decoder->hdl;
	if (decoder->hdl.error) {
		res = decoder->hdl.error;
		goto err;
	}

	res = v4l2_async_register_subdev(sd);
	if (res < 0)
		goto err;

	mod_timer(&decoder->work_timer, jiffies +  msecs_to_jiffies(1));
	
	return 0;
err:
	v4l2_ctrl_handler_free(&decoder->hdl);
	return res;
	
	return 0;
}

static int rn6752_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct rn6752 *decoder = to_rn6752(sd);

	dev_dbg(sd->dev, 
		"rn6752.c: removing rn6752 adapter on address 0x%x\n",
		client->addr << 1);
	
	del_timer(&decoder->timer);

	del_timer(&decoder->work_timer);
	if(decoder->eq_queue)
		destroy_workqueue(decoder->eq_queue);
	v4l2_async_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&decoder->hdl);

	return 0;
}

/* ----------------------------------------------------------------------- */
/* the length of name must less than 20 */
static const struct i2c_device_id rn6752_id[] = {
	{ "rn6752_ark1668e", },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rn6752_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id rn6752_of_match[] = {
	{ .compatible = "arkmicro,ark1668e_rn6752", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, rn6752_of_match);
#endif

static struct i2c_driver rn6752_driver = {
	.driver = {
		.of_match_table = of_match_ptr(rn6752_of_match),
		.name	= "rn6752",
	},
	.probe		= rn6752_probe,
	.remove		= rn6752_remove,
	.id_table	= rn6752_id,
};

static int __init rn_6752_init(void)
{
	return i2c_add_driver(&rn6752_driver);
}

static void __exit rn_6752_exit(void)
{
	i2c_del_driver(&rn6752_driver);
}

device_initcall(rn_6752_init);

MODULE_AUTHOR("arkmicro");
MODULE_DESCRIPTION("arkmicro rn6752 decoder driver for v4l2");
MODULE_LICENSE("GPL v2");
