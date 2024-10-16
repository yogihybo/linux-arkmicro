/*
 *  Header file for ARK1668E LCD Controller
 *
 */

#ifndef __ARK1668E_LCDC_H__
#define __ARK1668E_LCDC_H__

#include <linux/workqueue.h>
#include <linux/pwm.h>
#include <linux/soc/arkmicro/ark1668e_lcdc_regs.h>
#include "ark_lcdc_common.h"


enum ark1668e_interface_type {
	ARK1668E_LCDC_INTERFACE_TTL,
	ARK1668E_LCDC_INTERFACE_LVDS,
	ARK1668E_LCDC_INTERFACE_DUAL_LVDS,
};


typedef enum _ark1668e_lcdc_format {
	ARK1668E_LCDC_FORMAT_OSD_PALETTE_VIDEO_YUV422	= 0,	//osd layer is palette, video layer is y_u_v422.
	ARK1668E_LCDC_FORMAT_OSD_BMP24BIT_VIDEO_YUV420	= 1,	//osd layer isbmp24bit, video layer is y_u_v420.
	ARK1668E_LCDC_FORMAT_YUYV		= 2,	//Both osd and video layer support.
	ARK1668E_LCDC_FORMAT_YUV 		= 3,
	ARK1668E_LCDC_FORMAT_RGBI555	= 4,
	ARK1668E_LCDC_FORMAT_R5G6B5		= 5,
	ARK1668E_LCDC_FORMAT_RGBA888	= 6,
	ARK1668E_LCDC_FORMAT_RGB888		= 7,
	ARK1668E_LCDC_FORMAT_RGBA1555	= 8,
	ARK1668E_LCDC_FORMAT_RGBA1888	= 9,
	ARK1668E_LCDC_FORMAT_RGBA4888	= 10,
	ARK1668E_LCDC_FORMAT_RGB666		= 11,
	ARK1668E_LCDC_FORMAT_ARGA1666	= 12,
	ARK1668E_LCDC_FORMAT_MAX,

	//add which is not belong to lcdc register.Only used for video layer.
	ARK1668E_LCDC_FORMAT_Y_UV422	= 0x10,
	ARK1668E_LCDC_FORMAT_Y_UV420	= 0x11,
	ARK1668E_LCDC_FORMAT_END
}ARK1668E_LCDC_FORMAT;

typedef enum _ark1668e_lcdc_ycbcr_foramt {
	ARK1668E_LCDC_YCBCR_FORMAT_Y_U_V,	//Y_U_V422 or Y_U_V420
	ARK1668E_LCDC_YCBCR_FORMAT_Y_UV,	//Y_UV422 or Y_UV420
	ARK1668E_LCDC_YCBCR_FORMAT_END
}ARK1668E_LCDC_YCBCR_FORMAT;

#if 0
enum ark1668e_lcdc_osdlayer {
	OSD_LAYER1,
	OSD_LAYER2,
	OSD_LAYER3,
	OSD_LAYER_MAX,
};

enum ark1668e_lcdc_videolayer {
	VIDEO_LAYER1,
	VIDEO_LAYER2,
	VIDEO_LAYER_MAX,
};
#else
enum ark1668e_lcdc_layer {
	ARK1668E_LCDC_LAYER_VIDEO1,
	ARK1668E_LCDC_LAYER_VIDEO2,
	ARK1668E_LCDC_LAYER_OSD1,
	ARK1668E_LCDC_LAYER_OSD2,
	ARK1668E_LCDC_LAYER_OSD3,
	ARK1668E_LCDC_LAYER_MAX
};
#endif

/* Way LCD wires are connected to the chip:
 * A swapped wiring onboard can bring to RGB mode.
 */
#define ARK1668E_LCDC_WIRING_BGR	0
#define ARK1668E_LCDC_WIRING_GBR	1
#define ARK1668E_LCDC_WIRING_RBG	2
#define ARK1668E_LCDC_WIRING_BRG	3
#define ARK1668E_LCDC_WIRING_GRB	4
#define ARK1668E_LCDC_WIRING_RGB	5

 /* LCD Controller info data structure, stored in device platform_data */
struct ark1668e_lcdfb_pdata {
	u8	default_bpp;
	u8	lcd_wiring_mode;
	u8  interface_type;
	u32	lvds_con;
	u32	lvds_con2;
	bool	de_active_high;
	bool	pixelclk_active_high;
	bool	lcdcon_is_backlight;
    struct pwm_device	*pwm;
	struct timer_list	backlight_timer;
    int	backlight_value;
	int	backlight_delay;
	int fb_buffer_nums;
	u32 *osd3_buffer_virtaddr;
	unsigned int osd3_buffer_phyaddr;
	void (*ark1668e_lcdfb_power_control)(struct ark1668e_lcdfb_pdata *pdata, int on);
	struct list_head	pwr_gpios;
};

 /* LCD Controller info data structure, stored in device platform_data */
struct ark1668e_lcdfb_info {
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

	struct ark1668e_lcdfb_pdata pdata;
	u8 atomic_flag;
    struct ark_disp_atomic patomic[ARK1668E_LCDC_LAYER_MAX];
	struct ark_disp_addr render_addr[ARK1668E_LCDC_LAYER_MAX];
};

int ark1668e_lcdc_funcs_init(struct ark1668e_lcdfb_info *info);
int ark1668e_lcdc_wait_for_vsync(void);
int ark1668e_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);
int ark1668e_lcdc_set_osd_addr(int layer, int addr);
int ark1668e_lcdc_set_video_addr(int layer,  unsigned int yaddr,unsigned int cbaddr, unsigned int craddr);
void ark1668e_lcdc_display_update_atomic(struct ark1668e_lcdfb_info* sinfo);

#endif /* __ARK1668E_LCDC_H__ */
