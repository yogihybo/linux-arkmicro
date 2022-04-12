#ifndef _ARK168_VIDEO_H_
#define _ARK168_VIDEO_H_

#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/soc/arkmicro/ark1668_itu656_regs.h>
#include <linux/soc/arkmicro/ark1668_sys_resgs.h>
#include <linux/soc/arkmicro/ark1668_gpio.h>
#include <linux/soc/arkmicro/ark1668_lcdc_regs.h>

//#define ARK_VIN_DBG
 
#ifdef ARK_VIN_DBG 
#define ARKVIN_DBGPRTK(...) printk(KERN_ALERT __VA_ARGS__)
#else
#define ARKVIN_DBGPRTK(...)
#endif


#define ITU656_FRAME_NUM			4//max 6
#define ITU656_BUFFER_NUM			3

#define ITU656_DEV_CLASS_CREATE                 1

#define ARK_VIN_NAME		"ark1668_vin"
#define PAL		0
#define NTSC	        1
#define BLOCK_HEIGHT    32
#define ITU656_MAX_FRAME    4
#define PAL_WIDTH	720
#define PAL_HEIGHT	288
#define NTSC_WIDTH	720
#define NTSC_HEIGHT	240
#define CVBS_WIDTH 	PAL_WIDTH
#define CVBS_HEIGHT     PAL_HEIGHT

#define DEINTERLACE_MAX_FRAME    8
#define DEINTERLACE_WIDTH  720
#define DEINTERLACE_HEIGHT 576
#define DVR_MAJOR  243

#define ITU656_BUFFER_EMPTY			0x0 	// buffer is readed by lcd/tv, ITU656 can write
#define ITU656_BUFFER_FULL_LCD		0x1 	// buffer is writed ok by itu656. lcd can read
#define ITU656_BUFFER_FULL_TV 		0x10 	//  buffer is writed ok by itu656. tv can read
#define ITU656_BUFFER_FULL_APP		0x20	// buffer is writed ok by itu656. app can read
#define ITU656_BUFFER_FULL_MIRROR	0x40	// buffer is writed ok by itu656. mirror can read
#define DISCARD_FRAME_SYS_CHANGE		5

#define ITU656_FIELD_SIZE			(CVBS_WIDTH*CVBS_HEIGHT*2)
#define DISPLAY_WIDTH				1024
#define DISPLAY_HEIGHT				600
#define ITU656_FRAME_SIZE			(ITU656_FIELD_SIZE*2)
#define ITU656_PROGRESSIVE_FRAME_SIZE	        (1280*720*2)
#define ITU656_PROGRESSIVE_FRAME_SIZE_1080P	(1920*1080*2)

#define ARK_IOW(num, dtype)		_IOW('O', num, dtype)
#define ARK_IOR(num, dtype)		_IOR('O', num, dtype)
#define ARK_IOWR(num, dtype)	        _IOWR('O', num, dtype)
#define ARK_IO(num)			_IO('O', num)

#define ARKDISP_IOCTL_BASE		0xa0
#define ARKDISP_SET_TVENC_CFG           _IOW(ARKDISP_IOCTL_BASE, 10, unsigned long)

#define ARK_DISP_VIDEO_PIXFMT_YUYV     	2
#define ARKPRESCAL_WORKMODE_MIX      	3
#define ARKPRESCAL_HFORMAT_YUV422	3
#define DISPLAY_LAYER			4
#define TVOUT_LAYER			3
#define DISPLAY_FB_PATH			"/dev/fb4"
#define TVOUT_FB_PATH			"/dev/fb3"
#define ITU656_USE_DEINTERLACE

#define vin_readl(reg)		            __raw_readl(g_ark168_vin->dvr_dev->context.itu656_base+(reg))
#define vin_writel(reg, val)			    __raw_writel((val), g_ark168_vin->dvr_dev->context.itu656_base+(reg))
#define vin_readl_sys(reg)			    __raw_readl(g_ark168_vin->dvr_dev->context.sys_base+(reg))
#define vin_writel_sys(reg, val)		    __raw_writel((val), g_ark168_vin->dvr_dev->context.sys_base+(reg))
#define vin_readl_dein(reg)			    __raw_readl(g_ark168_vin->dvr_dev->context.deinterlace_base+(reg))
#define vin_writel_dein(reg, val)	            __raw_writel((val), g_ark168_vin->dvr_dev->context.deinterlace_base+(reg))
#define vin_readl_lcd(reg)			    __raw_readl(g_ark168_vin->dvr_dev->context.lcd_base+(reg))
#define vin_writel_lcd(reg, val)	            __raw_writel((val), g_ark168_vin->dvr_dev->context.lcd_base+(reg))


#define GetPingPongNextBuf(index,bufcnt) {index++;if(index == bufcnt) index = 0;}
#define GetPingPongPreBuf(index,bufcnt) {index--; if(index <0) index = bufcnt - 1;}
#define GetFrameNum(framecnt,oddeven,bufcnt) { framecnt = (oddeven == DEINTERLACE_FIELD_ODD) ? bufcnt*2:(bufcnt*2+1);}

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

struct vin_para{
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
	struct vin_para itu656in;
	struct vin_para itu656in_back;
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

	int ic_type;		//7116, tp2825b, rn6752 ...
	int old_cvbs_type;	//cvbs type before change

        struct ark_itu656in_context context;
        struct class *itu656_class;
        struct device *itu656_device;
    
};

struct vin_subdev_entity {
	struct v4l2_subdev		*sd;
	struct v4l2_async_subdev	*asd;
	struct v4l2_async_notifier      notifier;

	u32 pfe_cfg0;

	struct list_head list;
};

struct vin_buffer {
	struct vb2_v4l2_buffer  vb;
	struct list_head	list;
};

struct ark_subdev_data {
	struct ark_private_data *g_arkvin_priv;
	struct i2c_client *g_arkvin_client;
};

struct ark1668_vin_device {
	struct device		*dev;
	struct v4l2_device	v4l2_dev;
	struct video_device	video_dev;

	struct vb2_queue	vb2_vidq;
	spinlock_t		ark_queue_lock;
	struct list_head	ark_queue;
	struct vin_buffer	*cur_frm;
	unsigned int		sequence;
	unsigned int		vin_status;
	bool			stop;
	bool 			stream_flag;
	struct completion	comp;

	struct v4l2_format	fmt;

	struct work_struct	awb_work;

	struct mutex		lock;

	struct vin_subdev_entity	*current_subdev;
	struct list_head		subdev_entities;
	struct ark_subdev_data pdata;
	struct dvr_dev *dvr_dev;
};

struct ark_disp_addr {
	unsigned int  yaddr;
	unsigned int  cbaddr;
	unsigned int  craddr;
	unsigned int  wait_vsync;
};

struct ark_disp_scaler {
	int src_w;
	int src_h;
	int win_x;
	int win_y;
	int win_w;
	int win_h;
	int out_x;
	int out_y;
	int out_w;
	int out_h;
	int cut_top;
	int cut_bottom;
	int cut_left;
	int cut_right;
};

struct vin_display_outpara{
	unsigned int layer;
	unsigned int pos_data;
	unsigned int source_size;
	unsigned int layer_size;
	unsigned int format;
	struct ark_disp_addr disp_addr;
	struct ark_disp_scaler scaler;
};

#define VIN_SHOW_WINDOW	        			ARK_IO(55)
#define VIN_HIDE_WINDOW	        			ARK_IO(56)
#define VIN_SET_WINDOW_POS					ARK_IO(57)
#define VIN_SET_SOURCE_SIZE					ARK_IO(58)
#define VIN_SET_WINDOW_FORMAT					ARK_IO(59)
#define VIN_SET_WINDOW_ADDR        		    ARK_IO(60)
#define VIN_SET_WINDOW_SCALER         		ARK_IO(61)
#define VIN_SET_LAYER_SIZE					ARK_IO(62)

#define ARK_DVR_BRIGHTNESS_MASK		(1<<0)
#define ARK_DVR_CONTRAST_MASK		(1<<1)
#define ARK_DVR_SATURATION_MASK		(1<<2)
#define ARK_DVR_HUE_MASK		(1<<3)
#define ARK_DVR_SHARPNESS_MASK		(1<<4)


#define ITU656_INT	                (12+16)
#define DEINTERLACE_INT		        (31+16)


#endif

