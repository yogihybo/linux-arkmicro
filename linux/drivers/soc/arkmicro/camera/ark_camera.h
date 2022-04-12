#ifndef __ARK_CAMERA_H__
#define __ARK_CAMERA_H__

#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/interrupt.h>

#define ISP_SYS						0x0
#define ISP_INT_STATUS				0x1a8

#define XM_printf			printk
#define	isp_debug_printf	XM_printf
#define IMX323_DCK_SYNC_MODE_ENABLE	1


typedef unsigned char		u8_t;
typedef unsigned short 		u16_t;
typedef unsigned int 		u32_t;
typedef unsigned long long 	u64_t;
typedef signed 	 char		i8_t;
typedef short 				i16_t;
typedef int 				i32_t;
typedef long long 			i64_t;

#define IMAGE_H_SZ		isp_get_video_width()
#define IMAGE_V_SZ		isp_get_video_height()

#define ISP_FRAME_NUM		4


// 0: RGGB 1: GRBG 2: BGGR 3: GBRG
enum {
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB = 0,		//
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GRBG,
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_BGGR,
	ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GBRG
};

// YUV���ݴ洢��ʽ
// 0��y_uv420 1:y_uv422 2:yuv420 3:yuv422
enum {
	ARKN141_ISP_YUV_FORMAT_Y_UV420 = 0,
	ARKN141_ISP_YUV_FORMAT_Y_UV422,
	ARKN141_ISP_YUV_FORMAT_YUV420,
	ARKN141_ISP_YUV_FORMAT_YUV422
};


// 0: 8λ 1: 10λ 2: 12λ 3: 14λ
enum {
	ARKN141_ISP_SENSOR_BIT_8 = 0,
	ARKN141_ISP_SENSOR_BIT_10,
	ARKN141_ISP_SENSOR_BIT_12,
	ARKN141_ISP_SENSOR_BIT_14
};

// auto-run��Ŀ����

enum {

	ISP_AUTO_RUN_AE = 0,
	ISP_AUTO_RUN_AWB,
	ISP_AUTO_RUN_COLOR,
	ISP_AUTO_RUN_ERIS,
	ISP_AUTO_RUN_DENOISE,
	ISP_AUTO_RUN_ENHANCE,
	ISP_AUTO_RUN_FESP,
	ISP_AUTO_RUN_SHARP,
	ISP_AUTO_RUN_GAMMA,
	ISP_AUTO_RUN_CROSSTALK,
	ISP_AUTO_RUN_EXP,			// �ع����Զ�����
	ISP_AUTO_RUN_COUNT

};

enum {
	ARKN141_VIDEO_FORMAT_1080P_30 = 0,
	ARKN141_VIDEO_FORMAT_720P_30,
#ifdef CONFIG_HONGJING_CVBS
	ARKN141_VIDEO_FORMAT_720X480P,
	ARKN141_VIDEO_FORMAT_640X480P,
	ARKN141_VIDEO_FORMAT_320X240P,
#else
	ARKN141_VIDEO_FORMAT_720P_60,
#endif
	ARKN141_VIDEO_FORMAT_COUNT
};

struct ark_camera_context {
	void __iomem *base;
	void __iomem *scalebase;
	void __iomem *sysbase;
	int irq;
	struct device *dev;
	struct gpio_desc	*sensor_reset;
	struct gpio_desc	*sensor_standby;
	spinlock_t lock;
	char frame_finish[ISP_FRAME_NUM];
	int frame_finish_count;
	wait_queue_head_t frame_finish_waitq;
	struct work_struct camera_work;
	struct workqueue_struct *camera_queue;
};

struct ark_camera_device {
	const char *driver_name;
	const char *name;
    int major;
    int minor_start;
    int minor_num;
    int num;
	int irq;
	struct cdev cdev;
	struct class *camera_class;
	struct device *camera_device;
	struct fasync_struct *async_queue_cam;
	struct ark_camera_context context;
	struct clk *isp_clk;
	struct clk *sensor_mclk;
};


/*************************************************************************
 * Ioctl command definition
 *************************************************************************/
#define ARK_CAMERA_IOCTL_BASE			0x9A

#define ARK_CAMERA_IOCTL_START				_IO(ARK_CAMERA_IOCTL_BASE, 0)
#define ARK_CAMERA_IOCTL_STOP				_IO(ARK_CAMERA_IOCTL_BASE, 1)
#define ARK_CAMERA_IOCTL_GET_YUV_BUFFER		_IOR(ARK_CAMERA_IOCTL_BASE, 2, unsigned int *)
#define ARK_CAMERA_IOCTL_SET_FRAME_READY		_IOW(ARK_CAMERA_IOCTL_BASE, 3, int)
#define ARK_CAMERA_IOCTL_GET_FRAME_SIZE		_IOR(ARK_CAMERA_IOCTL_BASE, 4, unsigned int *)
#define ARK_CAMERA_IOCTL_GET_FRAME_FORMAT	_IOR(ARK_CAMERA_IOCTL_BASE, 5, unsigned int *)
#define ARK_CAMERA_IOCTL_SET_FRAME_FORMAT	_IOW(ARK_CAMERA_IOCTL_BASE, 6, unsigned int *)
#define ARK_CAMERA_IOCTL_GET_FPS			_IOR(ARK_CAMERA_IOCTL_BASE, 7, unsigned int *)

//extern int i2c_reg8_write8 (unsigned char ucDataOffset, unsigned char data);
//extern int i2c_reg8_read8 (unsigned char ucDataOffset, unsigned char *data);
//extern int i2c_reg16_write8 (unsigned short ucDataOffset, unsigned char data);
//extern int i2c_reg16_read8 (unsigned short ucDataOffset, unsigned char *data);
extern void isp_sensor_set_reset_pin_low(void);
extern void isp_sensor_set_reset_pin_high(void);
//extern void isp_sensor_set_standby_pin_low(void);
//extern void isp_sensor_set_standby_pin_high(void);

int xm_arkn141_isp_set_flicker_freq  (int flicker_freq);

int ark_camera_dev_init(struct ark_camera_device *camera);

irqreturn_t ark_camera_intr_handler(int irq, void *dev_id);

extern u32_t isp_get_video_width (void);
extern u32_t isp_get_video_height (void);
extern unsigned int isp_get_video_format (void);
extern unsigned int isp_get_video_image_size (void);
extern void isp_set_auto_run_state (unsigned int item, unsigned int state);
extern unsigned int isp_get_auto_run_state  (unsigned int item);
extern unsigned long isp_get_clk(void);
extern unsigned long isp_get_sensor_mclk(void);
extern unsigned int isp_get_sensor_fps(void);
extern void isp_set_sensor_fps(unsigned int fps);

#endif
