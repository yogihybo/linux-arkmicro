/*
 *  Header file for ARKn141s LCD Controller
 *
 */

#ifndef __ARKn141s_LCDC_H__
#define __ARKn141s_LCDC_H__

#include <linux/workqueue.h>
#include <linux/pwm.h>
#include <linux/soc/arkmicro/arkn141s_lcdc_regs.h>
#include "ark_lcdc_common.h"


enum arkn141s_interface_type {
	ARKN141S_LCDC_INTERFACE_TTL,
	ARKN141S_LCDC_INTERFACE_LVDS,
};

enum arkn141s_lcdc_format {
	ARKN141S_LCDC_FORMAT_PALETTE,
	ARKN141S_LCDC_FORMAT_BMP24BIT,
	ARKN141S_LCDC_FORMAT_VYUY,
	ARKN141S_LCDC_FORMAT_YUV,
	ARKN141S_LCDC_FORMAT_RGBI555,
	ARKN141S_LCDC_FORMAT_R5G6B5,
	ARKN141S_LCDC_FORMAT_RGBA888,
	ARKN141S_LCDC_FORMAT_RGB888,
};

/* Way LCD wires are connected to the chip:
 * A swapped wiring onboard can bring to RGB mode.
 */
#define ARKN141S_LCDC_WIRING_BGR	0
#define ARKN141S_LCDC_WIRING_GBR	1
#define ARKN141S_LCDC_WIRING_RBG	2
#define ARKN141S_LCDC_WIRING_BRG	3
#define ARKN141S_LCDC_WIRING_GRB	4
#define ARKN141S_LCDC_WIRING_RGB	5

 /* LCD Controller info data structure, stored in device platform_data */
struct arkn141s_lcdfb_pdata {
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
	void (*arkn141s_lcdfb_power_control)(struct arkn141s_lcdfb_pdata *pdata, int on);
	struct list_head	pwr_gpios;
};

 /* LCD Controller info data structure, stored in device platform_data */
struct arkn141s_lcdfb_info {
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

	bool			have_intensity_bit;

	struct arkn141s_lcdfb_pdata pdata;
};

enum arkn141s_lcdc_osdlayer {
	OSD_LAYER1,
	OSD_LAYER_MAX,
};

enum arkn141s_lcdc_videolayer {
	VIDEO_LAYER1,
	VIDEO_LAYER_MAX,
};

int arkn141s_lcdc_funcs_init(struct arkn141s_lcdfb_info *info);
int arkn141s_lcdc_wait_for_vsync(struct arkn141s_lcdfb_info *sinfo);
int arkn141s_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);



#endif /* __ARKn141s_LCDC_H__ */
