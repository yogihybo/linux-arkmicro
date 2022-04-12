/*
 *  Header file for ARK1668 LCD Controller
 *
 */

#ifndef __ARK1668_LCDC_H__
#define __ARK1668_LCDC_H__

#include <linux/workqueue.h>
#include <linux/pwm.h>
#include <linux/soc/arkmicro/ark1668_lcdc_regs.h>
#include "ark_lcdc_common.h"


#define ARK1668_LAYER_MAX        5


enum ark1668_interface_type {
	ARK1668_LCDC_INTERFACE_TTL,
	ARK1668_LCDC_INTERFACE_LVDS,
};

enum ark1668_lcdc_format {
	ARK1668_LCDC_FORMAT_PALETTE,
	ARK1668_LCDC_FORMAT_BMP24BIT,
	ARK1668_LCDC_FORMAT_VYUY,
	ARK1668_LCDC_FORMAT_YUV,
	ARK1668_LCDC_FORMAT_RGBI555,
	ARK1668_LCDC_FORMAT_R5G6B5,
	ARK1668_LCDC_FORMAT_RGBA888,
	ARK1668_LCDC_FORMAT_RGB888,
};

 enum ark1668_lcdc_osdlayer {
	 OSD_LAYER1,
	 OSD_LAYER2,
	 OSD_LAYER3,
	 OSD_LAYER_MAX,
 };
 
 enum ark1668_lcdc_videolayer {
	 VIDEO_LAYER1,
	 VIDEO_LAYER2,
	 VIDEO_LAYER_MAX,
 };

 /* LCD Controller info data structure, stored in device platform_data */
struct ark1668_lcdfb_pdata {
	u8	default_bpp;
	u8	lcd_wiring_mode;
	u8  interface_type;
	u32	lvds_con;
	bool	de_active_high;
	bool	pixelclk_active_high;
	bool	lcdcon_is_backlight;
    struct pwm_device	*pwm;
	struct timer_list	backlight_timer;
    int	backlight_value;
	int	backlight_delay;
	void (*ark1668_lcdfb_power_control)(struct ark1668_lcdfb_pdata *pdata, int on);
	struct list_head	pwr_gpios;
        u32	dithering_con;
        u32	osd1_vp;
        u32	osd2_vp;
        u32	osd3_vp;
        u32	video1_vp;
        u32	video2_vp;
};

 /* LCD Controller info data structure, stored in device platform_data */
struct ark1668_lcdfb_info {
	spinlock_t		lock;
	struct fb_info		*info;
	void __iomem		*mmio;
	void __iomem		*sysreg;
	int			irq_base;
	struct work_struct	task;
	wait_queue_head_t vsync_waitq;
	int			vsync_flag;

	unsigned int		smem_len;
	struct platform_device	*pdev;
	struct clk		*lcdc_clk;

	struct backlight_device	*backlight;
	u8			bl_power;
	u8			saved_lcdcon;
	bool		have_intensity_bit;
	struct ark1668_lcdfb_pdata pdata;
    u8 atomic_flag;
    struct ark_disp_atomic patomic[ARK1668_LAYER_MAX];
	struct ark_disp_addr render_addr[ARK1668_LAYER_MAX];
};

int ark1668_lcdc_funcs_init(struct ark1668_lcdfb_info *info);
int ark1668_lcdc_wait_for_vsync(void);
int ark1668_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
void ark1668_lcdc_set_osd_addr(int id, unsigned int addr);
void ark1668_lcdc_set_video_addr(int id,  unsigned int yaddr,unsigned int cbaddr, unsigned int craddr);

#endif /* __ARK1668_LCDC_H__ */
