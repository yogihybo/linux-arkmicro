#ifndef _ARK1668_VIDEO_H_
#define _ARK1668_VIDEO_H_

#include <linux/cdev.h>
#include <linux/i2c.h>

#include <linux/soc/arkmicro/ark1668e_lcdc_regs.h>
#include <linux/soc/arkmicro/ark1668e_sys_resgs.h>
#include <linux/soc/arkmicro/ark1668e_itu656_regs.h>
#include <linux/soc/arkmicro/ark_scale.h>
//#include <linux/soc/arkmicro/ark1668e_gpio.h>

#define ITU656_FRAME_NUM			4
#define ITU656_BUFFER_NUM			3
#define ITU656_SCALE_FRAME_NUM		3
#define ITU656_FRAME_NUM_MAX		8

#define ARK_VIN_NAME				"ark1668e_vin"
#define PAL							0
#define NTSC	        			1
#define PAL_WIDTH					720
#define PAL_HEIGHT					288
#define NTSC_WIDTH					720
#define NTSC_HEIGHT					240
#define CVBS_WIDTH 					PAL_WIDTH
#define CVBS_HEIGHT     			PAL_HEIGHT
#define START_DISCARD_FRAME			6
#define DISPLAY_WIDTH				1024
#define DISPLAY_HEIGHT				600

#define ITU656_FIELD_SIZE			(CVBS_WIDTH*CVBS_HEIGHT*2)
#define ITU656_FRAME_SIZE			(ITU656_FIELD_SIZE*2)
#define ITU656_PROGRESSIVE_FRAME_SIZE	        (1280*720*2)
#define ITU656_PROGRESSIVE_FRAME_SIZE_1080P	(1920*1080*3/2)

#define ARK_IOW(num, dtype)		_IOW('O', num, dtype)
#define ARK_IOR(num, dtype)		_IOR('O', num, dtype)
#define ARK_IOWR(num, dtype)	        _IOWR('O', num, dtype)
#define ARK_IO(num)			_IO('O', num)

//for v4l2 sd
#define VIDIOC_GET_RESOLUTION  				_IOWR('V', BASE_VIDIOC_PRIVATE + 1, int)
#define VIDIOC_GET_PROGRESSIVE  			_IOWR('V', BASE_VIDIOC_PRIVATE + 2, int)
#define VIDIOC_GET_CHIPINFO  				_IOWR('V', BASE_VIDIOC_PRIVATE + 3, int)
#define VIDIOC_ENTER_CARBACK  				_IOWR('V', BASE_VIDIOC_PRIVATE + 4, int)
#define VIDIOC_EXIT_CARBACK					_IOWR('V', BASE_VIDIOC_PRIVATE + 5, int)
#define VIDIOC_GET_ITU601_ENABLE			_IOWR('V', BASE_VIDIOC_PRIVATE + 6, int)
#define VIDIOC_SET_AVIN_MODE				_IOWR('V', BASE_VIDIOC_PRIVATE + 7, int)

#define TVOUT_LAYER			0
#define DISPLAY_LAYER			1
#define TRACK_LAYER			2

#define vin_readl(reg)		            __raw_readl(g_ark1668e_vin->dvr_dev->context.itu656_base+(reg))
#define vin_writel(reg, val)			    __raw_writel((val), g_ark1668e_vin->dvr_dev->context.itu656_base+(reg))
#define vin_readl_sys(reg)			    __raw_readl(g_ark1668e_vin->dvr_dev->context.sys_base+(reg))
#define vin_writel_sys(reg, val)		    __raw_writel((val), g_ark1668e_vin->dvr_dev->context.sys_base+(reg))
#define vin_readl_dein(reg)			    __raw_readl(g_ark1668e_vin->dvr_dev->context.deinterlace_base+(reg))
#define vin_writel_dein(reg, val)	            __raw_writel((val), g_ark1668e_vin->dvr_dev->context.deinterlace_base+(reg))
#define vin_readl_lcd(reg)			    __raw_readl(g_ark1668e_vin->dvr_dev->context.lcd_base+(reg))
#define vin_writel_lcd(reg, val)	            __raw_writel((val), g_ark1668e_vin->dvr_dev->context.lcd_base+(reg))

enum vin_framebuf_status{
    FRAMEBUF_STATUS_FREE,
    FRAMEBUF_STATUS_BUSY,
    FRAMEBUF_STATUS_READY,
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

enum itu656_mirror{
	MIRROR_NO = 0,
	MIRROR_LEVEL,
	MIRROR_VERTICAL,
	MIRROR_LEVEL_VERTICAL,
	MIRROR_END,
};


enum {
	TYPE_UNDEF = -1,
	TYPE_ARK7116 = 0,
	TYPE_RN6752,
	TYPE_PR2000,
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
	int work_status;
	int channel;
	int itu_channel;
	int system;		//ntsc or pal
	int interlace;
	int itu601en;
	int chip_info;
	int discard_frame;
	int cur_buffer;
	int show_video;
	int deinter_status;
	int src_width;
	int src_height;
	int screen_width;
	int screen_height;
	int screen_xpos;
	int screen_ypos;
	int carback_exit_status;
	u8 *buffer_virtaddr;
	unsigned int buffer_size;
	unsigned int buffer_phyaddr;
	unsigned int scale_buffer_size;
	int scale_alloc_width;
	int scale_alloc_height;
	int *scale_out_yaddr;
	int scale_out_yphyaddr;
	int scale_in_yphyaddr;
	
	int carback_signal;
	int layer_status;
	int signal_flag;
	int vin_mirror_config;
	int vin_buffer_status;
	int first_show_flag;
	int dev_major, dev_minor;
    spinlock_t spin_lock;
	struct work_struct scale_work;
	struct workqueue_struct *scale_queue;
	struct work_struct detect_work;
	struct workqueue_struct *detect_queue;
	struct cdev cdev;
	struct i2c_client *client;
	struct vin_para itu656in;
	struct vin_para itu656in_back;
	struct timer_list timer;
	struct timer_list signal_timer;
	
	unsigned int framebuf_phyaddr[ITU656_FRAME_NUM_MAX];
	unsigned int framebuf_status[ITU656_FRAME_NUM_MAX];
	unsigned int scalebuf_phyaddr[ITU656_SCALE_FRAME_NUM];
	unsigned int framebuf_num;
	unsigned int scale_framebuf_num;
	unsigned int scale_framebuf_index;
	unsigned int cur_frame;
	unsigned int frame_finish[ITU656_FRAME_NUM_MAX];//store frame id
	int frame_finish_count;
	wait_queue_head_t frame_finish_waitq;
    struct ark_itu656in_context context;
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

struct ark1668e_vin_device {
	struct device		*dev;
	struct v4l2_device	v4l2_dev;
	struct video_device	video_dev;

	struct vb2_queue	vb2_vidq;
	spinlock_t		ark_queue_lock;
	struct list_head	ark_queue;
	struct vin_buffer	*cur_frm;
	unsigned int		sequence;
	unsigned int		vin_status;
	unsigned int		sd_init;
	unsigned int		app_channel_set;
	bool			stop;
	bool 			stream_flag;
	int 			aux_flag;
	int 			aux_status;
	struct completion	comp;

	struct v4l2_format	fmt;

	struct work_struct	awb_work;

	struct mutex		lock;

	struct vin_subdev_entity	*current_subdev;
	struct list_head		subdev_entities;
	struct dvr_dev *dvr_dev;
};

struct vin_screen {
	unsigned int  disp_x;
	unsigned int  disp_y;
	unsigned int  disp_width;
	unsigned int  disp_height;
};

#define ARK_DVR_IOC_MAGIC			'n'

#define VIN_UPDATE_WINDOW		_IOWR(ARK_DVR_IOC_MAGIC, 50, struct vin_screen)
#define VIN_START				_IO(ARK_DVR_IOC_MAGIC, 51)
#define VIN_STOP				_IO(ARK_DVR_IOC_MAGIC, 52)
#define VIN_SWITCH_CHANNEL		_IOWR(ARK_DVR_IOC_MAGIC, 53, int)
#define VIN_CONFIG				_IOWR(ARK_DVR_IOC_MAGIC, 54, struct vin_para)

#endif

