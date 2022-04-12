/*
 *  Header file for ARKN141 LCD Controller
 *
 */

#ifndef __ARKN141_LCDC_H__
#define __ARKN141_LCDC_H__

#include <linux/workqueue.h>
#include <linux/pwm.h>
#include "ark_lcdc_common.h"

enum arkn141_interface_type {
	ARKN141_LCDC_INTERFACE_TTL,
	ARKN141_LCDC_INTERFACE_RGB,
	ARKN141_LCDC_INTERFACE_SRGB,
	ARKN141_LCDC_INTERFACE_SRGBP	//itu601out
};

enum arkn141_lcdc_format {
	ARKN141_LCDC_FORMAT_YUV420,
	ARKN141_LCDC_FORMAT_ARGB888,
	ARKN141_LCDC_FORMAT_RGB565,
	ARKN141_LCDC_FORMAT_RGB454,
	ARKN141_LCDC_FORMAT_AYUV444,	//add
	ARKN141_LCDC_FORMAT_Y_UV420,	//add
	ARKN141_LCDC_FORMAT_END
};

enum arkn141_lcdc_osdlayer {
	OSD_LAYER0 = 0,
	OSD_LAYER1,
	OSD_LAYER2,
	OSD_LAYER_MAX
};

 /* LCD Controller info data structure, stored in device platform_data */
struct arkn141_lcdfb_pdata {
	u8	default_bpp;
	u8	lcd_wiring_mode;
	u8  interface_type;
	u32	lvds_con;
	bool	de_active_high;
	bool	pixelclk_active_high;
	bool	lcdcon_is_backlight;
	bool	lcd_clk_freq_autoupdate;
    struct pwm_device	*pwm;
	struct timer_list	backlight_timer;
    int	backlight_value;
	int	backlight_delay;
	void (*arkn141_lcdfb_power_control)(struct arkn141_lcdfb_pdata *pdata, int on);
	struct list_head	pwr_gpios;
};

 /* LCD Controller info data structure, stored in device platform_data */
struct arkn141_lcdfb_info {
	spinlock_t		lock;
	struct fb_info		*info;
	void __iomem		*mmio;
	void __iomem		*sysreg;
	int			irq_base;
	struct work_struct	task;
	wait_queue_head_t	vsync_waitq;
	int			vsync_flag;
	unsigned int		smem_len;
	struct platform_device	*pdev;
	struct clk		*lcdc_clk;
	struct backlight_device	*backlight;
	u8			bl_power;
	u8			saved_lcdcon;
	bool		have_intensity_bit;
	u8 			atomic_flag;
	struct ark_disp_atomic patomic[OSD_LAYER_MAX];
	struct arkn141_lcdfb_pdata pdata;
	struct mutex mutex_lock;
	volatile u32 frame_vsync;
};


#define  ARKN141_LCDC_PARAM0		0x00
#define  ARKN141_LCDC_PARAM1		0x04
#define  ARKN141_LCDC_PARAM2		0x08
#define  ARKN141_LCDC_PARAM3		0x0c
#define  ARKN141_LCDC_PARAM4		0x10
#define  ARKN141_LCDC_PARAM5		0x14
#define  ARKN141_LCDC_PARAM6		0x18
#define  ARKN141_LCDC_PARAM7		0x1c
#define  ARKN141_LCDC_PARAM8		0x20
#define  ARKN141_LCDC_PARAM9		0x24
#define  ARKN141_LCDC_PARAM10		0x28
#define  ARKN141_LCDC_PARAM11		0x2c
#define  ARKN141_LCDC_PARAM12		0x30
#define  ARKN141_LCDC_PARAM13		0x34
#define  ARKN141_LCDC_PARAM14		0x38
#define  ARKN141_LCDC_PARAM15		0x3c
#define  ARKN141_LCDC_PARAM16		0x40
#define  ARKN141_LCDC_PARAM17		0x44
#define  ARKN141_LCDC_PARAM18		0x48
#define  ARKN141_LCDC_PARAM19		0x4c
#define  ARKN141_LCDC_PARAM20		0x50
#define  ARKN141_LCDC_PARAM21		0x54
#define  ARKN141_LCDC_PARAM22		0x58

//OSD
#define  ARKN141_LCDC_OSD_BASE		0x5c
#define  ARKN141_LCDC_OSD0_PARAM0	0x5c
#define  ARKN141_LCDC_OSD0_PARAM1	0x60
#define  ARKN141_LCDC_OSD0_PARAM2	0x64
#define  ARKN141_LCDC_OSD0_PARAM3	0x68
#define  ARKN141_LCDC_OSD0_PARAM4	0x6c
#define  ARKN141_LCDC_OSD0_PARAM5	0x70
#define  ARKN141_LCDC_OSD1_PARAM0	0x74
#define  ARKN141_LCDC_OSD1_PARAM1	0x78
#define  ARKN141_LCDC_OSD1_PARAM2	0x7c
#define  ARKN141_LCDC_OSD1_PARAM3	0x80
#define  ARKN141_LCDC_OSD1_PARAM4	0x84
#define  ARKN141_LCDC_OSD1_PARAM5	0x88
#define  ARKN141_LCDC_OSD2_PARAM0	0x8c
#define  ARKN141_LCDC_OSD2_PARAM1	0x90
#define  ARKN141_LCDC_OSD2_PARAM2	0x94
#define  ARKN141_LCDC_OSD2_PARAM3	0x98
#define  ARKN141_LCDC_OSD2_PARAM4	0x9c
#define  ARKN141_LCDC_OSD2_PARAM5	0xa0

#define  ARKN141_LCDC_INTR_CLR      0xc8
#define  ARKN141_LCDC_STATUS        0xcc
#define  ARKN141_LCDC_VP_RGB2YCBCR_COEF0	0x200
#define  ARKN141_LCDC_VP_RGB2YCBCR_COEF1	0x204
#define  ARKN141_LCDC_VP_RGB2YCBCR_COEF2	0x208
#define  ARKN141_LCDC_VP_ADJUSTEMENT		0x20c
#define  ARKN141_LCDC_VP_CONTROL			0x210
#define  ARKN141_LCDC_DITHING				0x214
#define  ARKN141_LCDC_SRGB_CFG				0x218
#define  ARKN141_LCDC_OSD_012_PARAM			0x268
#define  ARKN141_LCDC_OSD_COEF_SYNC			0x26c



#define ARKN141_LCDC_INT_LCD_FRAME  	(1 << 1)
#define ARKN141_LAYER_IS_VALID(layer)	((layer >= OSD_LAYER0) && (layer < OSD_LAYER_MAX))


int arkn141_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
int arkn141_lcdc_funcs_init(struct arkn141_lcdfb_info *sinfo);
int arkn141_lcdc_osd_addr_update(void);


#endif /* __ARKN141_LCDC_H__ */
