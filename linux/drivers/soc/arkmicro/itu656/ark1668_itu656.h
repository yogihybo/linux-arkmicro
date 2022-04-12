/*
 * Copyright(c) 2013 Hong Kong Applied Science and Technology
 * Research Institute Company Limited (ASTRI), all rights reserved
 * Proprietary and Confidential Information.
 *
 * This source file is the property of ASTRI, and may not be copied or
 * distributed in any isomorphic form without the prior written consent
 * of ASTRI.
 *
 * Name:
 *              itu656_regs.h
 *
 * Description:
 *              This file contains ark809 SOC ITU656 register definitions
 *
 * Author:
 *              Chan Man Chi
 *
 * Remarks:
 *
 */

#ifndef _ITU656_REGS_H_
#define _ITU656_REGS_H_

#include <linux/cdev.h>
#include <linux/soc/arkmicro/ark1668_itu656_regs.h>
#include <linux/soc/arkmicro/ark1668_sys_resgs.h>
#include <linux/soc/arkmicro/ark1668_gpio.h>
#include <linux/soc/arkmicro/ark1668_lcdc_regs.h>


#define ITU656_FRAME_NUM			4//max 6
#define ITU656_BUFFER_NUM			3

#define ITU656_DEV_CLASS_CREATE                 1

enum ark_disp_tvenc_out_mode {
        ARKDISP_TVENC_OUT_YPBPR_I480HZ60       = 0,
        ARKDISP_TVENC_OUT_YPBPR_I576HZ50       = 1,
        ARKDISP_TVENC_OUT_YPBPR_P480HZ60       = 2,
        ARKDISP_TVENC_OUT_YPBPR_P576HZ50       = 3,
        ARKDISP_TVENC_OUT_YPBPR_P720HZ60       = 4,
        ARKDISP_TVENC_OUT_YPBPR_P720HZ50       = 5,
        ARKDISP_TVENC_OUT_YPBPR_I1080HZ60      = 6,
        ARKDISP_TVENC_OUT_YPBPR_I1080HZ50      = 7,
        ARKDISP_TVENC_OUT_YPBPR_I1080HZ50_1250 = 8,
        ARKDISP_TVENC_OUT_YPBPR_P1080HZ60      = 9,
        ARKDISP_TVENC_OUT_YPBPR_P1080HZ50      = 10,
        ARKDISP_TVENC_OUT_CVBS_PAL             = 11,
        ARKDISP_TVENC_OUT_CVBS_NTSC            = 12,
        ARKDISP_TVENC_OUT_ITU656_PAL           = 13,
        ARKDISP_TVENC_OUT_ITU656_NTSC          = 14,
        ARKDISP_TVENC_OUT_VGA_640x480HZ60      = 15,
        ARKDISP_TVENC_OUT_VGA_800x600HZ60      = 16,
        ARKDISP_TVENC_OUT_VGA_1024x768HZ60     = 17,
        ARKDISP_TVENC_OUT_VGA_1280x1024HZ60    = 18,
        ARKDISP_TVENC_OUT_VGA_1900x1200HZ60    = 19, // bandwidth  limit
        ARKDISP_TVENC_OUT_VGA_1280x1024HZ75    = 20,
        ARKDISP_TVENC_OUT_VGA_1280x960HZ85     = 21, // bandwidth  limit
        ARKDISP_TVENC_OUT_VGA_1280x720HZ60     = 22,
};

enum {
	TYPE_UNKNOWN = -1,
	TYPE_CVBS = 0,
	TYPE_720P,
	TYPE_1080P,
};

enum ark7116_channel {
	ARK7116_AV0,
	ARK7116_AV1,
	ARK7116_AV2,
};

enum itu656_channel {
	ITU656_CH0,
	ITU656_CH1,
	ITU656_CH2,
};

enum dvr_source {
	DVR_SOURCE_CAMERA,
	DVR_SOURCE_AUX,
	DVR_SOURCE_DVD,
};

enum dvr_ic_type {
	IC_TYPE_UNKNOWN,
	IC_TYPE_ARK7116,
	IC_TYPE_TP2825B,
	IC_TYPE_RN6752,
	IC_TYPE_UB934,
	IC_TYPE_END
};

struct itu656in_para{
        int xpos;
        int ypos;
        int width;
        int height;
        int source;
        int tvout;
        int hori_mirror;
        int vert_mirror;
        int left_blank;
        int right_blank;
        int top_blank;
        int bottom_blank;
        int progressive;
        int itu601en;
};

enum ui_scaler_type_id {
	UI_SCALER_NONE,//normal mode: no caler ,cannt set posx posy
	UI_POSITION_CVBS,//no scaler, can set posx posy throught arkdata.ini	
	UI_SCALER_VGA, //scaler mode, can set posx posy throught arkdata.ini
	UI_SCALER_CVBS, //scaler mode, can set posx posy throught arkdata.ini
	UI_SCALER_END,
};

enum mirror_type {
	MIRROR_TYPE_NONE,
	MIRROR_TYPE_L2R,//mirror left to right 
	MIRROR_TYPE_U2D,//mirror up to down 
	MIRROR_TYPE_L2R_U2D,//mirror left to right and  up to down 
	MIRROR_TYPE_END,
};

struct arkfb_update_window {
	unsigned int win_x, win_y;
	unsigned int win_width, win_height;
	unsigned int width, height;
	unsigned int format;
	unsigned int rgb_order;
	unsigned int yuyv_order;
	unsigned int out_x, out_y;
	unsigned int out_width, out_height;
	unsigned int interlace_out;
	unsigned int show_tv;
};


struct arkfb_window_addr {
	unsigned int  phy_addr;
	unsigned int  phy_cbaddr;
	unsigned int  phy_craddr;
	unsigned int  wait_vsync;
};

struct ark_disp_tvenc_cfg_arg {
        unsigned int enable;
        unsigned int out_mode; /* enum ark_disp_tvenc_out_mode */
        unsigned int backcolor_y;
        unsigned int backcolor_cb;
        unsigned int backcolor_cr;
};
struct ark_frame_info {
	unsigned int  frame_max;//mxa = 6
	unsigned int  framebuf_phyaddr[6];
};

struct ark_private_data {
        int init;
	int support_max_resolution;
	int (*select_channel)(int ch);
	int (*detect_signal)(void);
	int (*get_progressive)(void);
	int (*display_effect)(int cmd, unsigned long arg);
	void (*dvr_start_cb)(void);
	void (*dvr_stop_cb)(void);
	int (*enter_carback_cb)(void);
	int (*exit_carback_cb)(void);
	int (*dvr_config)(void);
	int ic_type;
        int channel;
};

struct ark_carback_effect_para {
	volatile unsigned int carback_mask;	//bit0=1:carback_brightness active,bit0=0:carback_brightness not active; ... ; bit5=1:carback_sharpness active,bit5=0:carback_sharpness not active
	volatile int carback_brightness;
	volatile int carback_contrast;
	volatile int carback_saturation;
	volatile int carback_hue;
	volatile int carback_sharpness;
};

struct ark_itu656in_context {
        int itu656_irq;
        int deinterlace_irq;
        struct device *dev;
        void __iomem *itu656_base;
        void __iomem *sys_base;
        void __iomem *deinterlace_base;
        void __iomem *lcd_base;
};

struct dvr_dev{
	char *name;
	int deinter_odd_even;
	int work_status;
	int channel;
	int carback_break;
	int start_carback_exit;
	int itu_channel;
	int system;		//ntsc or pal
	int interlace;
	int itu601en;
	int discard_frame;
	int cur_buffer;
	int write_buffer;
	int display_buffer;
	int video_reinit;
	int show_video;
	int display_odd_even;
	int deinter_status;
	int src_width;
	int src_height;
	u8 *buffer_virtaddr;
	unsigned int buffer_size;
	unsigned int buffer_phyaddr;
	unsigned int buf_status[ITU656_BUFFER_NUM];
	unsigned int oddbuf_phyaddr[ITU656_BUFFER_NUM];
	unsigned int evenbuf_phyaddr[ITU656_BUFFER_NUM];
	unsigned int fieldbuf_phyaddr[4];

	int carback_signal;
	int enter_carback;
	int clock_scale_store;	//save clock configure when enter carback
	//struct mutex mutex_lock;
        spinlock_t spin_lock;
	int dev_major, dev_minor;
	struct cdev cdev;
	struct i2c_client *client;
	struct itu656in_para itu656in;
	struct itu656in_para itu656in_back;
	struct timer_list timer;
	struct fasync_struct *fasync_queue;	
	
	//for vbox
	unsigned int framebuf_phyaddr[ITU656_FRAME_NUM];
	unsigned int framebuf_status[ITU656_FRAME_NUM];
	int write_framebuf;
	int get_framebuf;
	int deinter_indirect_show;
	char frame_finish[ITU656_FRAME_NUM];//store frame id
	int frame_finish_count;
	wait_queue_head_t frame_finish_waitq;

	//mirror
	struct work_struct mirror_work;
	struct workqueue_struct *mirror_queue;
	unsigned int mirror_type;
	unsigned int oddbuf_virtaddr[ITU656_BUFFER_NUM];
	unsigned int evenbuf_virtaddr[ITU656_BUFFER_NUM];
	int write_mirror;

	struct list_head frame_list;

	void (*start)(struct dvr_dev *dvr_dev);
	void (*stop)(struct dvr_dev *dvr_dev);
	struct ark_private_data priv_data;

	int ic_type;		//7116, tp2825b, rn6752 ...
	int old_cvbs_type;	//cvbs type before change

        struct ark_itu656in_context context;
        struct class *itu656_class;
        struct device *itu656_device;
    
};

#define DEBUG
#ifdef DEBUG
#define itu656_printk(...) printk(__VA_ARGS__)
#else
#define itu656_printk(...) 
#endif

/* IOCTL CMD */
#define ARK_DVR_IOC_MAGIC			'n'

#define ARK_DVR_START			_IO(ARK_DVR_IOC_MAGIC,  0x10)
#define ARK_DVR_STOP			_IO(ARK_DVR_IOC_MAGIC,  0x20)
#define ARK_DVR_GETFRAME		_IO(ARK_DVR_IOC_MAGIC,  0x30)
#define ARK_DVR_SWITCH_CHANNEL		_IOWR(ARK_DVR_IOC_MAGIC,  0x40, unsigned long)
#define ARK_DVR_CHANNEL_STATUS		_IOWR(ARK_DVR_IOC_MAGIC,  0x50, unsigned long)
#define ARK_DVR_INIT			_IOWR(ARK_DVR_IOC_MAGIC,  0x60, struct itu656in_para)
#define ARK_DVR_TVOUT			_IOWR(ARK_DVR_IOC_MAGIC,  0x70, unsigned long)
#define ARK_DVR_DETECT_SIGNAL		_IOWR(ARK_DVR_IOC_MAGIC,  0x80, unsigned long)
#define ARK_DVR_INDIRECT_LCD		_IOWR(ARK_DVR_IOC_MAGIC,  0x90, unsigned long)
#define ARK_DVR_GET_FRAME_INFO		_IOWR(ARK_DVR_IOC_MAGIC,  0xA0, unsigned long)
#define ARK_DVR_SET_FRAME_COMPLETE	_IOWR(ARK_DVR_IOC_MAGIC,  0xB0, unsigned long)
#define ARK_DVR_SET_FRAME_SHOW		_IOWR(ARK_DVR_IOC_MAGIC,  0xC0, unsigned long)
#define ARK_DVR_SET_MIRROR_TYPE		_IOWR(ARK_DVR_IOC_MAGIC,  0xD0, unsigned long)
#define ARK_DVR_GET_CVBS_TYPE		_IOWR(ARK_DVR_IOC_MAGIC,  0xD1, unsigned long)
#define ARK_DVR_GET_BRIGHTNESS		_IOWR(ARK_DVR_IOC_MAGIC,  0xE0, unsigned long)
#define ARK_DVR_SET_BRIGHTNESS		_IOWR(ARK_DVR_IOC_MAGIC,  0xE1, unsigned long)
#define ARK_DVR_GET_CONTRAST		_IOWR(ARK_DVR_IOC_MAGIC,  0xE2, unsigned long)
#define ARK_DVR_SET_CONTRAST		_IOWR(ARK_DVR_IOC_MAGIC,  0xE3, unsigned long)
#define ARK_DVR_GET_SATURATION		_IOWR(ARK_DVR_IOC_MAGIC,  0xE4, unsigned long)
#define ARK_DVR_SET_SATURATION		_IOWR(ARK_DVR_IOC_MAGIC,  0xE5, unsigned long)
#define ARK_DVR_GET_HUE			_IOWR(ARK_DVR_IOC_MAGIC,  0xE6, unsigned long)
#define ARK_DVR_SET_HUE			_IOWR(ARK_DVR_IOC_MAGIC,  0xE7, unsigned long)
#define ARK_DVR_GET_SHARPNESS		_IOWR(ARK_DVR_IOC_MAGIC,  0xE8, unsigned long)
#define ARK_DVR_SET_SHARPNESS		_IOWR(ARK_DVR_IOC_MAGIC,  0xE9, unsigned long)

#define ARK_DVR_BRIGHTNESS_MASK		(1<<0)
#define ARK_DVR_CONTRAST_MASK		(1<<1)
#define ARK_DVR_SATURATION_MASK		(1<<2)
#define ARK_DVR_HUE_MASK		(1<<3)
#define ARK_DVR_SHARPNESS_MASK		(1<<4)


#define ITU656_INT	                (12+16)
#define DEINTERLACE_INT		        (31+16)


#endif
