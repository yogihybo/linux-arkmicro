/*
 * Arkmicro arkn141 lcd driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/backlight.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <video/of_display_timing.h>
#include <video/display_timing.h>
#include <video/videomode.h>

#include "arkn141_lcdc.h"

#define BACKLIGHT_PWM_PERIOD		50000
#define BACKLIGHT_MAX_BRIGHTNESS	100

#define SYS_REG_BASE		0x40408000
#define SYS_LCD_CLK_CFG		0x54
#define SYS_PIXEL_CLK_INV_OFFSET	8

struct arkn141_lcdfb_power_ctrl_gpio {
	int gpio;
	int active_low;

	struct list_head list;
};

#define lcdc_readl(sinfo, reg)		__raw_readl((sinfo)->mmio+(reg))
#define lcdc_writel(sinfo, reg, val)	__raw_writel((val), (sinfo)->mmio+(reg))
#define lcdc_readl_sys(sinfo, reg)		__raw_readl((sinfo)->sysreg+(reg))
#define lcdc_writel_sys(sinfo, reg, val)	__raw_writel((val), (sinfo)->sysreg+(reg))

#define	ARKN141_LCDFB_FBINFO_DEFAULT	(FBINFO_DEFAULT \
					 | FBINFO_HWACCEL_NONE)

static int arkn141_set_backlight(struct arkn141_lcdfb_pdata *pdata,
								 int brightness)
{
	int duty = brightness * BACKLIGHT_PWM_PERIOD / BACKLIGHT_MAX_BRIGHTNESS;

	pwm_enable(pdata->pwm);
	pwm_config(pdata->pwm, duty, BACKLIGHT_PWM_PERIOD);
	pdata->backlight_value = brightness;

	return 0;
}

/* some bl->props field just changed */
static int arkn141_bl_update_status(struct backlight_device *bl)
{
	struct arkn141_lcdfb_info *sinfo = bl_get_data(bl);
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	int			power = sinfo->bl_power;
	int			brightness = bl->props.brightness;

	/* REVISIT there may be a meaningful difference between
	 * fb_blank and power ... there seem to be some cases
	 * this doesn't handle correctly.
	 */
	if (bl->props.fb_blank != sinfo->bl_power)
		power = bl->props.fb_blank;
	else if (bl->props.power != sinfo->bl_power)
		power = bl->props.power;

	if (brightness < 0 && power == FB_BLANK_UNBLANK)
		brightness = pdata->backlight_value;
	else if (power != FB_BLANK_UNBLANK)
		brightness = 0;

	arkn141_set_backlight(pdata, brightness);

	bl->props.fb_blank = bl->props.power = sinfo->bl_power = power;

	return 0;
}

static int arkn141_bl_get_brightness(struct backlight_device *bl)
{
	struct arkn141_lcdfb_info *sinfo = bl_get_data(bl);
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;

	return pdata->backlight_value;
}

static const struct backlight_ops arkn141_lcdc_bl_ops = {
	.update_status = arkn141_bl_update_status,
	.get_brightness = arkn141_bl_get_brightness,
};

static void init_backlight(struct arkn141_lcdfb_info *sinfo)
{
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	struct backlight_properties props;
	struct backlight_device	*bl;

	sinfo->bl_power = FB_BLANK_UNBLANK;

	if (sinfo->backlight)
		return;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = BACKLIGHT_MAX_BRIGHTNESS;
	bl = backlight_device_register("backlight", &sinfo->pdev->dev, sinfo,
				       &arkn141_lcdc_bl_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&sinfo->pdev->dev, "error %ld on backlight register\n",
				PTR_ERR(bl));
		return;
	}
	sinfo->backlight = bl;

	bl->props.power = FB_BLANK_UNBLANK;
	bl->props.fb_blank = FB_BLANK_UNBLANK;
	bl->props.brightness = arkn141_bl_get_brightness(bl);

	arkn141_set_backlight(pdata, pdata->backlight_value);
}

static void exit_backlight(struct arkn141_lcdfb_info *sinfo)
{
	if (!sinfo->backlight)
		return;

	if (sinfo->backlight->ops) {
		sinfo->backlight->props.power = FB_BLANK_POWERDOWN;
		sinfo->backlight->ops->update_status(sinfo->backlight);
	}
	backlight_device_unregister(sinfo->backlight);
}

static inline void arkn141_lcdfb_power_control(struct arkn141_lcdfb_info *sinfo, int on)
{
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;

	if (pdata->arkn141_lcdfb_power_control)
		pdata->arkn141_lcdfb_power_control(pdata, on);
}

static const struct fb_fix_screeninfo arkn141_lcdfb_fix __initconst = {
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_TRUECOLOR,
	.xpanstep	= 0,
	.ypanstep	= 1,
	.ywrapstep	= 0,
	.accel		= FB_ACCEL_NONE,
};

/* static void arkn141_lcdfb_stop(struct arkn141_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, 0);
} */

static void arkn141_lcdfb_start(struct arkn141_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, lcdc_readl(sinfo, ARKN141_LCDC_PARAM0) | 1);
}

static inline void arkn141_lcdfb_free_video_memory(struct arkn141_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;

	dma_free_wc(info->device, info->fix.smem_len, info->screen_base,
		    info->fix.smem_start);
}

/**
 *	arkn141_lcdfb_alloc_video_memory - Allocate framebuffer memory
 *	@sinfo: the frame buffer to allocate memory for
 *
 * 	This function is called only from the arkn141_lcdfb_probe()
 * 	so no locking by fb_info->mm_lock around smem_len setting is needed.
 */
static int arkn141_lcdfb_alloc_video_memory(struct arkn141_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	struct fb_var_screeninfo *var = &info->var;
	unsigned int smem_len;

	smem_len = (var->xres_virtual * var->yres_virtual
		    * ((var->bits_per_pixel + 7) / 8));
	info->fix.smem_len = max(smem_len, sinfo->smem_len);

	info->screen_base = dma_alloc_wc(info->device, info->fix.smem_len,
					 (dma_addr_t *)&info->fix.smem_start,
					 GFP_KERNEL);

	if (!info->screen_base) {
		return -ENOMEM;
	}

	memset(info->screen_base, 0, info->fix.smem_len);

	return 0;
}

static const struct fb_videomode *arkn141_lcdfb_choose_mode(struct fb_var_screeninfo *var,
						     struct fb_info *info)
{
	struct fb_videomode varfbmode;
	const struct fb_videomode *fbmode = NULL;

	fb_var_to_videomode(&varfbmode, var);
	fbmode = fb_find_nearest_mode(&varfbmode, &info->modelist);
	if (fbmode)
		fb_videomode_to_var(var, fbmode);
	return fbmode;
}

/**
 *      arkn141_lcdfb_check_var - Validates a var passed in.
 *      @var: frame buffer variable screen structure
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Checks to see if the hardware supports the state requested by
 *	var passed in. This function does not alter the hardware
 *	state!!!  This means the data stored in struct fb_info and
 *	struct arkn141_lcdfb_info do not change. This includes the var
 *	inside of struct fb_info.  Do NOT change these. This function
 *	can be called on its own if we intent to only test a mode and
 *	not actually set it. The stuff in modedb.c is a example of
 *	this. If the var passed in is slightly off by what the
 *	hardware can support then we alter the var PASSED in to what
 *	we can do. If the hardware doesn't support mode change a
 *	-EINVAL will be returned by the upper layers. You don't need
 *	to implement this function then. If you hardware doesn't
 *	support changing the resolution then this function is not
 *	needed. In this case the driver would just provide a var that
 *	represents the static state the screen is in.
 *
 *	Returns negative errno on error, or zero on success.
 */
static int arkn141_lcdfb_check_var(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	struct device *dev = info->device;
	struct arkn141_lcdfb_info *sinfo = info->par;
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long clk_value_khz;

	if(pdata->lcd_clk_freq_autoupdate) {
		if(var->pixclock != info->var.pixclock)
			clk_set_rate(sinfo->lcdc_clk, PICOS2KHZ(var->pixclock) * 1000);
	}
	clk_value_khz = clk_get_rate(sinfo->lcdc_clk) / 1000;
	//printk("### %s clk:%ldKHZ, pixclock:%ldKHZ\n", __FUNCTION__, clk_value_khz, PICOS2KHZ(var->pixclock));

	dev_dbg(dev, "%s:\n", __func__);

	if (!(var->pixclock && var->bits_per_pixel)) {
		/* choose a suitable mode if possible */
		if (!arkn141_lcdfb_choose_mode(var, info)) {
			dev_err(dev, "needed value not specified\n");
			return -EINVAL;
		}
	}

	dev_dbg(dev, "  resolution: %ux%u\n", var->xres, var->yres);
	dev_dbg(dev, "  pixclk:     %lu KHz\n", PICOS2KHZ(var->pixclock));
	dev_dbg(dev, "  bpp:        %u\n", var->bits_per_pixel);
	dev_dbg(dev, "  clk:        %lu KHz\n", clk_value_khz);
	/* Do not allow to have real resoulution larger than virtual */
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;

	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres * 2;

	/* Force same alignment for each line */
	var->xres = (var->xres + 3) & ~3UL;
	var->xres_virtual = (var->xres_virtual + 3) & ~3UL;

	var->red.msb_right = var->green.msb_right = var->blue.msb_right = 0;
	var->transp.msb_right = 0;
	var->transp.offset = var->transp.length = 0;
	var->xoffset = var->yoffset = 0;

	if (info->fix.smem_len) {
		unsigned int smem_len = (var->xres_virtual * var->yres_virtual
					 * ((var->bits_per_pixel + 7) / 8));
		if (smem_len > info->fix.smem_len) {
			dev_err(dev, "Frame buffer is too small (%u) for screen size (need at least %u)\n",
				info->fix.smem_len, smem_len);
			return -EINVAL;
		}
	}

	/* Saturate vertical and horizontal timings at maximum values */
	var->vsync_len = min_t(u32, var->vsync_len, 1024);
	var->upper_margin = min_t(u32, var->upper_margin, 1024);
	var->lower_margin = min_t(u32, var->lower_margin, 1024);
	var->right_margin = min_t(u32, var->right_margin, 1024);
	var->hsync_len = min_t(u32, var->hsync_len, 1024);
	var->left_margin = min_t(u32, var->left_margin, 1024);

	/* Some parameters can't be zero */
	var->vsync_len = max_t(u32, var->vsync_len, 1);
	var->right_margin = max_t(u32, var->right_margin, 1);
	var->hsync_len = max_t(u32, var->hsync_len, 1);
	var->left_margin = max_t(u32, var->left_margin, 1);

	switch (var->bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
		var->red.offset = var->green.offset = var->blue.offset = 0;
		var->red.length = var->green.length = var->blue.length
			= var->bits_per_pixel;
		break;
	case 16:
		var->green.length = 6;
		var->red.offset = var->green.length + 5;
		var->blue.offset = 0;
		var->green.offset = 5;
		var->red.length = var->blue.length = 5;
		break;
	case 32:
		var->transp.offset = 24;
		var->transp.length = 8;
		/* fall through */
	case 24:
		var->red.offset = 16;
		var->blue.offset = 0;
		var->green.offset = 8;
		var->red.length = var->green.length = var->blue.length = 8;
		break;
	default:
		dev_err(dev, "color depth %d not supported\n",
					var->bits_per_pixel);
		return -EINVAL;
	}

	return 0;
}

/*
 * LCD reset sequence
 */
/* static void arkn141_lcdfb_reset(struct arkn141_lcdfb_info *sinfo)
{
	might_sleep();

	arkn141_lcdfb_stop(sinfo);
	arkn141_lcdfb_start(sinfo);
} */

static int arkn141_lcdfb_pan_display(struct fb_var_screeninfo *var,
			       struct fb_info *info)
{
	struct arkn141_lcdfb_info *sinfo = info->par;
	struct fb_fix_screeninfo *fix = &info->fix;
	unsigned long addr;

	addr = fix->smem_start + var->yoffset * fix->line_length
		+ var->xoffset * info->var.bits_per_pixel / 8;

	lcdc_writel(sinfo, ARKN141_LCDC_OSD1_PARAM2, addr);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD_COEF_SYNC,
		lcdc_readl(sinfo, ARKN141_LCDC_OSD_COEF_SYNC) | (1 << 1));

	return 0;
}

static int arkn141_lcd_itu601_init(struct fb_info *info)
{
	struct arkn141_lcdfb_info *sinfo = info->par;
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long value;
	unsigned int direct_enable = 0;
	unsigned int mem_lcd_enable = 0;
	unsigned int range_coeff_y = 0;
	unsigned int range_coeff_uv = 0;
	unsigned int dac_for_video = 0;
	unsigned int lcd_done_intr_enable = 1;  //ʹ\C4\DC\D6ж\CF=1 \BD\FBֹ\D6ж\CF=0 lcd_done_intr
	unsigned int dac_for_cvbs = 0;
	unsigned int lcd_interlace_flag = 0;
	unsigned int screen_type = 2; //pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
	unsigned int VSW1_enable = 0;
	unsigned int LcdVComp = 0;
	unsigned int itu_pal_ntsc = 0;
	//unsigned int hsync_ivs  = 0;
	//unsigned int vsync_ivs   = 0;
	unsigned int lcd_ac_ivs = 0;
	unsigned int test_on_flag = 0;
	unsigned int back_color = 0;//(137<<16)|(143<<8)|(105); //(200<<16)|(100<<8)|(16)
	unsigned int cpu_screen_csn_bit = 0;
	unsigned int cpu_screen_wen_bit = 9;
	unsigned int stop_lcd = 0;

	// ITU656ʱ\D0\F2\C9\FA\B3ɼĴ\E6\C6\F7
	unsigned int sav_pos = 556;
	unsigned int f1_vblk_start = 5;
	unsigned int f1_vblk_end = 45;
	unsigned int f2_vblk_start = 5;          //pal 315   ntsc 265
	unsigned int f2_vblk_end = 45;
	unsigned int f2_start = 313;
	unsigned int f2_end = 625;
	unsigned int binary_thres = 128;
	unsigned int binary_big = 240;
	unsigned int binary_small = 16;

	value = 
		(info->var.xres << 1) |
		(pdata->lcd_wiring_mode << 13) |
		(direct_enable << 16) |
		(mem_lcd_enable << 17) |
		(range_coeff_y << 18) |
		(range_coeff_uv << 22) |
		(lcd_done_intr_enable << 26) |
		(dac_for_video << 27) |
		(dac_for_cvbs << 28) |
		(1<<30) |
		(stop_lcd<<31);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, value);

	value  =
		(lcd_interlace_flag << 0) |
		//(pdata->interface_type << 1) |
		(screen_type << 1) |
		(VSW1_enable << 13) |
		(LcdVComp << 14) |
		(itu_pal_ntsc << 15) |
		(!!(info->var.sync & FB_SYNC_HOR_HIGH_ACT) << 17) |
		(!!(info->var.sync & FB_SYNC_VERT_HIGH_ACT) << 18) |
		(lcd_ac_ivs << 19) |
		(0 << 20) |
		(1 << 21) |
		(1 << 22) |	//itu656_out_sel(0:normal itur=656; 1: prograssive itu656)
		(1 << 23) |
		(test_on_flag << 31);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM1, value);

	value  =
		(back_color << 0) |
		(cpu_screen_csn_bit << 24) |
		(cpu_screen_wen_bit << 28);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM2, value);

	/* Initialize color convert table */
	value = 66 | (129 << 9) | (25 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM12, value);
	value = 38 | (74 << 9) | (112 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM13, value);
	value = 112 | (94 << 9) | (18 << 18); 
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM14, value);
	value = 298 | (0 << 9) | (409 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM15, value);
	value = 299 |  (100 << 9) | (208 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM16, value);
	value = 298 | (517 << 9) | (0 << 19);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM17, value);
	value = (sav_pos << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM18, value);
	value = (f1_vblk_start << 0) | (f1_vblk_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM19, value);
	value = (f2_vblk_start << 0) | (f2_vblk_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM20, value);
	value = (f2_start << 0) | (f2_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM21, value);
	value = (binary_thres << 16) | (binary_big << 8) | (binary_small << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM22, value);

	/* timing */
#if 0
	value = info->var.hsync_len + info->var.left_margin + info->var.right_margin;
	value = ((value + info->var.xres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM3, value);
	value = info->var.vsync_len + info->var.lower_margin + info->var.upper_margin;
	value = ((value + info->var.yres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM4, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM5, value);
#else
	value = info->var.hsync_len + info->var.left_margin + info->var.right_margin - 5;
	value = ((value + info->var.xres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM3, value);
	value = info->var.vsync_len + info->var.lower_margin + info->var.upper_margin - 1;
	value = ((value + info->var.yres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM4, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM5, value);
#endif
	value = ((info->var.vsync_len + info->var.lower_margin + info->var.upper_margin + info->var.yres) << 12) |
		(info->var.hsync_len + info->var.left_margin + info->var.right_margin + info->var.xres);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM6, value);
	value = ((info->var.hsync_len + info->var.right_margin) << 12) | info->var.right_margin;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM7, value);
	value = ((info->var.vsync_len + info->var.lower_margin) << 12) | info->var.lower_margin;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM8, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM10, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM9, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM11, value);

	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_012_PARAM);
	value |= (1<<12);	//osd0: 1:y_uv420 mode, UV store togeter.
	lcdc_writel(sinfo, ARKN141_LCDC_OSD_012_PARAM, value);

	/*  */
	value	 = (0 << 11)	 // Srgb_disable
									 // 	 1: srgb output disable, 0 is output
									 // 	 0: srgb output enable.
				 | (1 << 10)	 // srgb_yuv_sel
									 // 	 1: yuv data is select;
									 // 	 0: rgb data is select.
				 | (2 << 0) 	 // Srgb_mode
									 // 	 00: through mode, R G B R G B\A1\AD
									 // 	 01: sRGB dummy, R G B 0 R G B 0\A1\AD
									 // 	 10: sYUV422, Cb Y Cr Y Cb Y Cr Y\A1\AD
									 // 	 11: 0 is output
				 | (3 << 2)
				 | (3 << 5)
				 | (0 << 25)
				 ;
	lcdc_writel(sinfo, ARKN141_LCDC_SRGB_CFG, value);

	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF0, 0x10650242);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF1, 0x01c09426);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF2, 0x0048bc70);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_CONTROL, (0xF|(64 << 8)));	// VDE
	lcdc_writel(sinfo, ARKN141_LCDC_VP_ADJUSTEMENT, 0x00308080);	// ȱʡֵ

	/* Initialize dithing */
	value = (1 << 7) | (1 << 15) | (5 << 24);
	lcdc_writel(sinfo, ARKN141_LCDC_DITHING, value);

	/* osd0 used for video display layer */
	value = (1 << 0) | (ARKN141_LCDC_FORMAT_YUV420 <<3) | (info->var.xres << 6) | (info->var.yres <<18) | (1 <<31);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM0, value);
	value = (63 << 24) | info->var.xres;
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM5, value);
	//value = lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM1);
	//value &= ~0xffffff;
	value = (255<<24);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM1, value);

	/* enable/disable osd layer0 */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM0);
	value |= (0 << 1);	//0:disable, 1:enable
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM0, value);

	/* sync osd layer0 param */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_COEF_SYNC);
	value |= (1 << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD_COEF_SYNC, value);

	return 0;
}

static int arkn141_lcd_srgb_init(struct fb_info *info)
{
	struct arkn141_lcdfb_info *sinfo = info->par;
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long value;
	unsigned int direct_enable = 0;
	unsigned int mem_lcd_enable = 0;
	unsigned int range_coeff_y = 0;
	unsigned int range_coeff_uv = 0;
	unsigned int dac_for_video = 0;
	unsigned int lcd_done_intr_enable = 1;  //ʹ\C4\DC\D6ж\CF=1 \BD\FBֹ\D6ж\CF=0 lcd_done_intr
	unsigned int dac_for_cvbs = 0;
	unsigned int lcd_interlace_flag = 0;
	unsigned int screen_type = 2; //pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
	unsigned int VSW1_enable = 0;
	unsigned int LcdVComp = 0;
	unsigned int itu_pal_ntsc = 0;
	unsigned int hsync_ivs  = 1;
	unsigned int vsync_ivs  = 1;
	unsigned int lcd_ac_ivs = 0;
	unsigned int test_on_flag = 0;
	unsigned int back_color = (0x22<<16)|(0x44<<8)|(0x88);
	unsigned int cpu_screen_csn_bit = 0;
	unsigned int cpu_screen_wen_bit = 9;
	unsigned int stop_lcd = 0;

#if 0
	// ITU656ʱ\D0\F2\C9\FA\B3ɼĴ\E6\C6\F7
	unsigned int sav_pos = 288;
	unsigned int f1_vblk_start = 624;
	unsigned int f1_vblk_end = 23;
	unsigned int f2_vblk_start = 311;          //pal 315   ntsc 265
	unsigned int f2_vblk_end = 336;
	unsigned int f2_start = 313;
	unsigned int f2_end = 625;
	unsigned int binary_thres = 128;
	unsigned int binary_big = 240;
	unsigned int binary_small = 16;
#endif

	value =
		(info->var.xres << 1) |
		(pdata->lcd_wiring_mode << 13) |
		(direct_enable << 16) |
		(mem_lcd_enable << 17) |
		(range_coeff_y << 18) |
		(range_coeff_uv << 22) |
		(lcd_done_intr_enable << 26) |
		(dac_for_video << 27) |
		(dac_for_cvbs << 28) |
		(1<<30) |
		(stop_lcd<<31);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, value);

	value  =
		(lcd_interlace_flag << 0) |
		(screen_type << 1) |
		(VSW1_enable << 13) |
		(LcdVComp << 14) |
		(itu_pal_ntsc << 15) |
		(hsync_ivs << 17) |
		(vsync_ivs << 18) |
		(lcd_ac_ivs << 19) |
		(1 << 21) |
		(1 << 23) |
		(test_on_flag << 31);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM1, value);

	value  =
		(back_color << 0) |
		(cpu_screen_csn_bit << 24) |
		(cpu_screen_wen_bit << 28);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM2, value);

	/* Initialize color convert table */
	value = 66 | (129 << 9) | (25 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM12, value);
	value = 38 | (74 << 9) | (112 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM13, value);
	value = 112 | (94 << 9) | (18 << 18); 
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM14, value);
	value = 256 | (0 << 9) | (394 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM15, value);
	value = 256 |  (47 << 9) | (118 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM16, value);
	value = 256 | (465 << 9) | (0 << 19);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM17, value);
#if 0
	value = (sav_pos << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM18, value);
	value = (f1_vblk_start << 0) | (f1_vblk_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM19, value);
	value = (f2_vblk_start << 0) | (f2_vblk_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM20, value);
	value = (f2_start << 0) | (f2_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM21, value);
	value = (binary_thres << 16) | (binary_big << 8) | (binary_small << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM22, value);
#endif
	/* timing */
	value = info->var.hsync_len + info->var.left_margin + info->var.right_margin;
	value = ((value + info->var.xres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM3, value);
	value = info->var.vsync_len + info->var.lower_margin + info->var.upper_margin;
	value = ((value + info->var.yres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM4, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM5, value);

	value = ((info->var.vsync_len + info->var.lower_margin + info->var.upper_margin + info->var.yres) << 12) |
		(info->var.hsync_len + info->var.left_margin + info->var.right_margin + info->var.xres);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM6, value);
	value = ((info->var.hsync_len + info->var.right_margin) << 12) | info->var.right_margin;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM7, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM9, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM11, value);
	value = ((info->var.vsync_len + info->var.lower_margin) << 12) | info->var.lower_margin;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM8, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM10, value);

	//value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_012_PARAM);
	//value |= (1<<12);	//osd0: 1:y_uv420 mode, UV store togeter.
	//lcdc_writel(sinfo, ARKN141_LCDC_OSD_012_PARAM, value);

	/*  */
	value	 = (0 << 11)	 // Srgb_disable
									 // 	 1: srgb output disable, 0 is output
									 // 	 0: srgb output enable.
				 | (0 << 10)	 // srgb_yuv_sel
									 // 	 1: yuv data is select;
									 // 	 0: rgb data is select.
				 | (0 << 0) 	 // Srgb_mode
									 // 	 00: through mode, R G B R G B\A1\AD
									 // 	 01: sRGB dummy, R G B 0 R G B 0\A1\AD
									 // 	 10: sYUV422, Cb Y Cr Y Cb Y Cr Y\A1\AD
									 // 	 11: 0 is output
				 | (2 << 2)
				 | (5 << 5)
				 | (0 << 25)
				 ;
	lcdc_writel(sinfo, ARKN141_LCDC_SRGB_CFG, value);

	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF0, 0x10650242);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF1, 0x01c09426);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF2, 0x0048bc70);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_CONTROL, (0xF|(64 << 8)));	// VDE
	lcdc_writel(sinfo, ARKN141_LCDC_VP_ADJUSTEMENT, 0x00408080);	// ȱʡֵ

	/* Initialize dithing */
	value = (1 << 7) | (1 << 15) | (5 << 24);
	lcdc_writel(sinfo, ARKN141_LCDC_DITHING, value);

	/* osd0 used for video display layer */
	value = (1 << 0) | (ARKN141_LCDC_FORMAT_YUV420 <<3) | (info->var.xres << 6) | (info->var.yres <<18) | (1 <<31);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM0, value);
	value = (63 << 24) | info->var.xres;
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM5, value);
	//value = lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM1);
	//value &= ~0xffffff;
	value = (255<<24);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM1, value);

	/* enable/disable osd layer0 */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM0);
	value |= (0 << 1);	//0:disable, 1:enable
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM0, value);

	/* sync osd layer0 param */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_COEF_SYNC);
	value |= (1 << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD_COEF_SYNC, value);

	return 0;
}

static int arkn141_lcd_rgb_init(struct fb_info *info)
{
	struct arkn141_lcdfb_info *sinfo = info->par;
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long value;
	/* Initialize control register */
	unsigned int direct_enable = 0;
	unsigned int mem_lcd_enable = 0;
	unsigned int range_coeff_y = 0;
	unsigned int range_coeff_uv = 0;
	unsigned int lcd_done_intr_enable = 1;
	unsigned int dac_for_video = 0;
	unsigned int dac_for_cvbs = 0;

	unsigned int lcd_interlace_flag = 0;
	unsigned int screen_type = 1; //pRGB_i   0  pRGB_p   1   sRGB_p   2  ITU656   3
	unsigned int VSW1_enable = 0;
	unsigned int LcdVComp = 0;
	unsigned int itu_pal_ntsc = 0;
	unsigned int lcd_ac_ivs = 0;
	unsigned int test_on_flag = 0;

	unsigned int back_color = (0x22<<16)|(0x44<<8)|(0x88 );	// \u65e0\u80cc\u666f\u8272
	unsigned int cpu_screen_csn_bit = 0;
	unsigned int cpu_screen_wen_bit = 9;
#if 0
	unsigned int sav_pos = 288;
	unsigned int f1_vblk_start = 624;
	unsigned int f1_vblk_end = 23;
	unsigned int f2_vblk_start = 311;
	unsigned int f2_vblk_end = 336;
	unsigned int f2_start = 313;
	unsigned int f2_end = 625;
	unsigned int binary_thres = 128;
	unsigned int binary_big = 240;
	unsigned int binary_small = 16;
#endif
	value =
		(info->var.xres << 1) |
		(pdata->lcd_wiring_mode << 13) |
		(direct_enable << 16) |
		(mem_lcd_enable << 17) |
		(range_coeff_y << 18) |
		(range_coeff_uv << 22) |
		(lcd_done_intr_enable << 26) |
		(dac_for_video << 27) |
		(dac_for_cvbs << 28);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, value);

	value  =
		(lcd_interlace_flag << 0) |
		(screen_type << 1) |
		(VSW1_enable << 13) |
		(LcdVComp << 14) |
		(itu_pal_ntsc << 15) |
		(!!(info->var.sync & FB_SYNC_HOR_HIGH_ACT) << 17) |
		(!!(info->var.sync & FB_SYNC_VERT_HIGH_ACT) << 18) |
		(lcd_ac_ivs << 19) |
		(1 << 21) |
		(1 << 23) |
		(test_on_flag << 31);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM1, value);

	value  =
		(back_color << 0) |
		(cpu_screen_csn_bit << 24) |
		(cpu_screen_wen_bit << 28);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM2, value);

	/* Initialize color convert table */
	value = 66 | (129 << 9) | (25 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM12, value);
	value = 38 | (74 << 9) | (112 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM13, value);
	value = 112 | (94 << 9) | (18 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM14, value);
	value = 256 | (0 << 9) | (394 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM15, value);
	value = 256 |  (47 << 9) | (118 << 18);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM16, value);
	value = 256 | (465 << 9) | (0 << 19);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM17, value);
#if 0
	value = (sav_pos << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM18, value);
	value = (f1_vblk_start << 0) | (f1_vblk_end << 12) ;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM19, value);
	value = (f2_vblk_start << 0) | (f2_vblk_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM20, value);
	value = (f2_start << 0) | (f2_end << 12);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM21, value);
	value = (binary_thres << 16) | (binary_big << 8) | (binary_small << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM22, value);
#endif

	/* Initialize dithing */
	value = (1 << 7) | (1 << 15) | (5 << 24);
	lcdc_writel(sinfo, ARKN141_LCDC_DITHING, value);

	/* timing */
	value = info->var.hsync_len + info->var.left_margin + info->var.right_margin;
	value = ((value + info->var.xres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM3, value);
	value = info->var.vsync_len + info->var.lower_margin + info->var.upper_margin;
	value = ((value + info->var.yres) << 12) | value;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM4, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM5, value);
	value = ((info->var.vsync_len + info->var.lower_margin + info->var.upper_margin +
				info->var.yres) << 12) | (info->var.hsync_len + info->var.left_margin +
				info->var.right_margin + value + info->var.xres);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM6, value);
	value = ((info->var.hsync_len + info->var.right_margin) << 12) | info->var.right_margin;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM7, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM9, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM11, value);
	value = ((info->var.vsync_len + info->var.lower_margin) << 12) | info->var.lower_margin;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM8, value);
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM10, value);

	//value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_012_PARAM);
	//value |= (1<<12);	//osd0: 1:y_uv420 mode, UV store togeter.
	//lcdc_writel(sinfo, ARKN141_LCDC_OSD_012_PARAM, value);

	/*  */
	value	 = (0 << 11)	 // Srgb_disable
									 // 	 1: srgb output disable, 0 is output
									 // 	 0: srgb output enable.
				 | (0 << 10)	 // srgb_yuv_sel
									 // 	 1: yuv data is select;
									 // 	 0: rgb data is select.
				 | (0 << 0) 	 // Srgb_mode
									 // 	 00: through mode, R G B R G B\A1\AD
									 // 	 01: sRGB dummy, R G B 0 R G B 0\A1\AD
									 // 	 10: sYUV422, Cb Y Cr Y Cb Y Cr Y\A1\AD
									 // 	 11: 0 is output
				 | (2 << 2)
				 | (5 << 5)
				 | (0 << 25)
				 ;
	lcdc_writel(sinfo, ARKN141_LCDC_SRGB_CFG, value);

	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF0, 0x10650242);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF1, 0x01c09426);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_RGB2YCBCR_COEF2, 0x0048bc70);
	lcdc_writel(sinfo, ARKN141_LCDC_VP_CONTROL, (0xF|(64 << 8)));	// VDE
	lcdc_writel(sinfo, ARKN141_LCDC_VP_ADJUSTEMENT, 0x00408080);	// ȱʡֵ

	/* Initialize dithing */
	value = (1 << 7) | (1 << 15) | (5 << 24);
	lcdc_writel(sinfo, ARKN141_LCDC_DITHING, value);

	/* osd0 used for video display layer */
	value = (1 << 0) | (ARKN141_LCDC_FORMAT_YUV420 <<3) | (info->var.xres << 6) | (info->var.yres <<18) | (1 <<31);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM0, value);
	value = (63 << 24) | info->var.xres;
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM5, value);
	//value = lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM1);
	//value &= ~0xffffff;
	value = (255<<24);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM1, value);

	/* enable/disable osd layer0 */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM0);
	value |= (0 << 1);	//0:disable, 1:enable
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM0, value);

	/* sync osd layer0 param */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_COEF_SYNC);
	value |= (1 << 0);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD_COEF_SYNC, value);

	return 0;
}
/**
 *      arkn141_lcdfb_set_par - Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Using the fb_var_screeninfo in fb_info we set the resolution
 *	of the this particular framebuffer. This function alters the
 *	par AND the fb_fix_screeninfo stored in fb_info. It doesn't
 *	not alter var in fb_info since we are using that data. This
 *	means we depend on the data in var inside fb_info to be
 *	supported by the hardware.  arkn141_lcdfb_check_var is always called
 *	before arkn141_lcdfb_set_par to ensure this.  Again if you can't
 *	change the resolution you don't need this function.
 *
 */
static int arkn141_lcdfb_set_par(struct fb_info *info)
{
	struct arkn141_lcdfb_info *sinfo = info->par;
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long value;
	unsigned long bits_per_line;

	might_sleep();

	dev_dbg(info->device, "%s:\n", __func__);
	dev_dbg(info->device, "  * resolution: %ux%u (%ux%u virtual)\n",
		 info->var.xres, info->var.yres,
		 info->var.xres_virtual, info->var.yres_virtual);

	if (info->var.bits_per_pixel == 1)
		info->fix.visual = FB_VISUAL_MONO01;
	else if (info->var.bits_per_pixel <= 8)
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
	else
		info->fix.visual = FB_VISUAL_TRUECOLOR;

	bits_per_line = info->var.xres_virtual * info->var.bits_per_pixel;
	info->fix.line_length = DIV_ROUND_UP(bits_per_line, 8);

	if (pdata->interface_type == ARKN141_LCDC_INTERFACE_SRGBP) {	//itu601out
		// set itu601 outpiut clock: 720P-->74.25MHz
		value =	lcdc_readl_sys(sinfo, SYS_LCD_CLK_CFG);
		value &= ~((0x7 << 15) | (0x3 << 8));
		value |= (2 << 15) | (1 << 9);
		lcdc_writel_sys(sinfo, SYS_LCD_CLK_CFG, value);

		//itu601 init
		arkn141_lcd_itu601_init(info);
	} else if (pdata->interface_type == ARKN141_LCDC_INTERFACE_SRGB) {	//srgb
		//srgb init
		value =	lcdc_readl_sys(sinfo, SYS_LCD_CLK_CFG);
		value &= ~((0x7 << 15) | (0x3 << 8));
		value |= (3 << 15) | (1 << 9) | (1 << 8);	//bit8:lcd_clk(bit clk) inversal; bit9:lcd_clk(pixel clk) inversal
		lcdc_writel_sys(sinfo, SYS_LCD_CLK_CFG, value);

		arkn141_lcd_srgb_init(info);
	} else  if (pdata->interface_type == ARKN141_LCDC_INTERFACE_RGB) {	//rgb888
		//rgb init
		value =	lcdc_readl_sys(sinfo, SYS_LCD_CLK_CFG);
		value &= ~((0x7 << 15) | (0x3 << 8));
		value |= (1 << 15);	//bit8:lcd_clk(bit clk) inversal; bit9:lcd_clk(pixel clk) inversal
		lcdc_writel_sys(sinfo, SYS_LCD_CLK_CFG, value);

		arkn141_lcd_rgb_init(info);
	} else {	//argb888
		/* Initialize control register */
		unsigned int direct_enable = 0;
		unsigned int mem_lcd_enable = 0;
		unsigned int range_coeff_y = 0;
		unsigned int range_coeff_uv = 0;
		unsigned int lcd_done_intr_enable = 1;
		unsigned int dac_for_video = 0;
		unsigned int dac_for_cvbs = 0;

		unsigned int lcd_interlace_flag = 0;
		unsigned int VSW1_enable = 0;
		unsigned int LcdVComp = 0;
		unsigned int itu_pal_ntsc = 0;
		unsigned int lcd_ac_ivs = 0;
		unsigned int test_on_flag = 0;

		unsigned int back_color = (0x22<<16)|(0x44<<8)|(0x88 );	// \u65e0\u80cc\u666f\u8272
		unsigned int cpu_screen_csn_bit = 0;
		unsigned int cpu_screen_wen_bit = 9;

		/* Now, the LCDC core... */
		value = lcdc_readl_sys(sinfo, SYS_LCD_CLK_CFG);
		if (pdata->pixelclk_active_high)
			value |= (1 << SYS_PIXEL_CLK_INV_OFFSET);
		else
			value &= ~(1 << SYS_PIXEL_CLK_INV_OFFSET);
		lcdc_writel_sys(sinfo, SYS_LCD_CLK_CFG, value);

		value =
			(info->var.xres << 1) |
			(pdata->lcd_wiring_mode << 13) |
			(direct_enable << 16) |
			(mem_lcd_enable << 17) |
			(range_coeff_y << 18) |
			(range_coeff_uv << 22) |
			(lcd_done_intr_enable << 26) |
			(dac_for_video << 27) |
			(dac_for_cvbs << 28);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, value);

		value  =
			(lcd_interlace_flag << 0) |
			(pdata->interface_type << 1) |
			(VSW1_enable << 13) |
			(LcdVComp << 14) |
			(itu_pal_ntsc << 15) |
			(!!(info->var.sync & FB_SYNC_HOR_HIGH_ACT) << 17) |
			(!!(info->var.sync & FB_SYNC_VERT_HIGH_ACT) << 18) |
			(lcd_ac_ivs << 19) |
			(1 << 21) |
			(1 << 23) |
			(test_on_flag << 31);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM1, value);

		value  =
			(back_color << 0) |
			(cpu_screen_csn_bit << 24) |
			(cpu_screen_wen_bit << 28);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM2, value);

		/* Initialize color convert table */
		value = 66 | (129 << 9) | (25 << 18);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM12, value);
		value = 38 | (74 << 9) | (112 << 18);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM13, value);
		value = 112 | (94 << 9) | (18 << 18);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM14, value);
		value = 256 | (0 << 9) | (394 << 18);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM15, value);
		value = 256 |  (47 << 9) | (118 << 18);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM16, value);
		value = 256 | (465 << 9) | (0 << 19);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM17, value);

		/* Initialize dithing */
		value = (1 << 7) | (1 << 15) | (5 << 24);
		lcdc_writel(sinfo, ARKN141_LCDC_DITHING, value);

		/* timing */
		value = info->var.hsync_len + info->var.left_margin + info->var.right_margin;
		value = ((value + info->var.xres) << 12) | value;
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM3, value);
		value = info->var.vsync_len + info->var.lower_margin + info->var.upper_margin;
		value = ((value + info->var.yres) << 12) | value;
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM4, value);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM5, value);
		value = ((info->var.vsync_len + info->var.lower_margin + info->var.upper_margin +
					info->var.yres + 21) << 12) | (info->var.hsync_len + info->var.left_margin +
					info->var.right_margin + value + info->var.xres + 1);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM6, value);
		value = ((info->var.hsync_len + info->var.right_margin) << 12) | info->var.right_margin;
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM7, value);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM9, value);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM11, value);
		value = ((info->var.vsync_len + info->var.lower_margin) << 12) | info->var.lower_margin;
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM8, value);
		lcdc_writel(sinfo, ARKN141_LCDC_PARAM10, value);

		/* Initialize specific screen type */
		if (pdata->interface_type == ARKN141_LCDC_INTERFACE_SRGB) {

		}
	}

	/* Display osd layer1(fb0) size,pos,format,addr... */
	arkn141_lcdfb_pan_display(&info->var, info);
	value = (ARKN141_LCDC_FORMAT_ARGB888 << 3) | (info->var.xres << 6) | (info->var.yres << 18) | 1;
	lcdc_writel(sinfo, ARKN141_LCDC_OSD1_PARAM0, value);
	value = (63 << 24) | info->var.xres;
	lcdc_writel(sinfo, ARKN141_LCDC_OSD1_PARAM5, value);
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD1_PARAM1);
	value &= ~0xffffff;
	lcdc_writel(sinfo, ARKN141_LCDC_OSD1_PARAM1, value);

	/* open osd layer1 */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD1_PARAM0);
	value |= (1 << 1);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD1_PARAM0, value);

	/* sync osd layer1 param */
	value = lcdc_readl(sinfo, ARKN141_LCDC_OSD_COEF_SYNC);
	value |= (1 << 1);
	lcdc_writel(sinfo, ARKN141_LCDC_OSD_COEF_SYNC, value);

	/* Clear all interrupts */
	lcdc_writel(sinfo, ARKN141_LCDC_INTR_CLR, 7);

	/* Enable frame interrupt */
	value = lcdc_readl(sinfo, ARKN141_LCDC_PARAM0);
	value |= 1 << 30;
	lcdc_writel(sinfo, ARKN141_LCDC_PARAM0, value);

	arkn141_lcdfb_start(sinfo);

	dev_dbg(info->device, "  * DONE\n");

	return 0;
}

/* static int arkn141_lcdfb_blank(int blank_mode, struct fb_info *info)
{
	struct arkn141_lcdfb_info *sinfo = info->par;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		arkn141_lcdfb_start(sinfo);
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		break;
	case FB_BLANK_POWERDOWN:
		arkn141_lcdfb_stop(sinfo);
		break;
	default:
		return -EINVAL;
	}

	return ((blank_mode == FB_BLANK_NORMAL) ? 1 : 0);
} */

static struct fb_ops arkn141_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= arkn141_lcdfb_check_var,
	.fb_set_par	= arkn141_lcdfb_set_par,
	//.fb_blank	= arkn141_lcdfb_blank,
	.fb_pan_display	= arkn141_lcdfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_ioctl       = arkn141_lcdfb_ioctl,
};

volatile int arkn141_lcdc_frame_sync = 0;
static irqreturn_t arkn141_lcdfb_interrupt(int irq, void *dev_id)
{
	struct fb_info *info = dev_id;
	struct arkn141_lcdfb_info *sinfo = info->par;
	unsigned long flags;
	u32 status;

	status = lcdc_readl(sinfo, ARKN141_LCDC_STATUS);

	/* clear intr except scale writeback intr */
	lcdc_writel(sinfo, ARKN141_LCDC_INTR_CLR, 0x7);

	if (status & ARKN141_LCDC_INT_LCD_FRAME) {
		sinfo->vsync_flag = 1;
		spin_lock_irqsave(&sinfo->lock, flags);
		sinfo->frame_vsync = 1;
		spin_unlock_irqrestore(&sinfo->lock, flags);
		wake_up_interruptible(&sinfo->vsync_waitq);
		schedule_work(&sinfo->task);
	}

	return IRQ_HANDLED;
}

/*
 * LCD controller task (to reset the LCD)
 */
static void arkn141_lcdfb_task(struct work_struct *work)
{
	/* struct arkn141_lcdfb_info *sinfo =
		container_of(work, struct arkn141_lcdfb_info, task); */
}

static int __init arkn141_lcdfb_init_fbinfo(struct arkn141_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	int ret = 0;

	info->var.activate |= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;

	dev_info(info->device,
	       "%luKiB frame buffer at %08lx (mapped at %p)\n",
	       (unsigned long)info->fix.smem_len / 1024,
	       (unsigned long)info->fix.smem_start,
	       info->screen_base);

	/* Allocate colormap */
	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0)
		dev_err(info->device, "Alloc color map failed\n");

	return ret;
}

static void arkn141_lcdfb_start_clock(struct arkn141_lcdfb_info *sinfo)
{
	clk_prepare_enable(sinfo->lcdc_clk);
}

static void arkn141_lcdfb_stop_clock(struct arkn141_lcdfb_info *sinfo)
{
	clk_disable_unprepare(sinfo->lcdc_clk);
}

static const struct of_device_id arkn141_lcdfb_dt_ids[] = {
	{ .compatible = "arkmicro,arkn141-lcdc",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, arkn141_lcdfb_dt_ids);

static const char *arkn141_lcdfb_interface_types[] = {
	[ARKN141_LCDC_INTERFACE_TTL]	= "TTL",
	[ARKN141_LCDC_INTERFACE_RGB]	= "RGB",
	[ARKN141_LCDC_INTERFACE_SRGB]	= "SRGB",
	[ARKN141_LCDC_INTERFACE_SRGBP]	= "SRGB-P",
};

static int arkn141_lcdfb_get_of_interface_types(struct device_node *np)
{
	const char *type;
	int err, i;

	err = of_property_read_string(np, "interface-type", &type);
	if (err < 0)
		return ARKN141_LCDC_INTERFACE_TTL;

	for (i = 0; i < ARRAY_SIZE(arkn141_lcdfb_interface_types); i++)
		if (!strcasecmp(type, arkn141_lcdfb_interface_types[i]))
			return i;

	return -ENODEV;
}

static const char *arkn141_lcdfb_wiring_modes[] = {
	[ARK_LCDC_WIRING_BGR]	= "BGR",
	[ARK_LCDC_WIRING_GBR]	= "GBR",
	[ARK_LCDC_WIRING_RBG]	= "RBG",
	[ARK_LCDC_WIRING_BRG]	= "BRG",
	[ARK_LCDC_WIRING_GRB]	= "GRB",
	[ARK_LCDC_WIRING_RGB]	= "RGB",
};

static int arkn141_lcdfb_get_of_wiring_modes(struct device_node *np)
{
	const char *mode;
	int err, i;

	err = of_property_read_string(np, "lcd-wiring-mode", &mode);
	if (err < 0)
		return ARK_LCDC_WIRING_BGR;

	for (i = 0; i < ARRAY_SIZE(arkn141_lcdfb_wiring_modes); i++)
		if (!strcasecmp(mode, arkn141_lcdfb_wiring_modes[i]))
			return i;

	return -ENODEV;
}

static void arkn141_lcdfb_power_control_gpio(struct arkn141_lcdfb_pdata *pdata, int on)
{
	struct arkn141_lcdfb_power_ctrl_gpio *og;

	list_for_each_entry(og, &pdata->pwr_gpios, list)
		gpio_set_value(og->gpio, on ? !og->active_low : og->active_low);
}

/*
 * Timer function for delayed backlight power up/down
 */
static void arkn141_backlight_timer_func(struct timer_list *t)
{
	struct arkn141_lcdfb_info *sinfo = from_timer(sinfo, t, pdata.backlight_timer);
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;

	arkn141_lcdfb_power_control(sinfo, 1);
	del_timer_sync(&pdata->backlight_timer);
}

static int arkn141_lcdfb_of_init(struct arkn141_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	struct arkn141_lcdfb_pdata *pdata = &sinfo->pdata;
	struct fb_var_screeninfo *var = &info->var;
	struct device *dev = &sinfo->pdev->dev;
	struct device_node *np =dev->of_node;
	struct device_node *display_np;
	struct device_node *timings_np;
	struct display_timings *timings;
	enum of_gpio_flags flags;
	struct arkn141_lcdfb_power_ctrl_gpio *og;
	bool is_gpio_power = false;
	int ret = -ENOENT;
	int i, gpio;

	display_np = of_parse_phandle(np, "display", 0);
	if (!display_np) {
		dev_err(dev, "failed to find display phandle\n");
		return -ENOENT;
	}

	ret = of_property_read_u32(display_np, "bits-per-pixel", &var->bits_per_pixel);
	if (ret < 0) {
		dev_err(dev, "failed to get property bits-per-pixel\n");
		goto put_display_node;
	}

	INIT_LIST_HEAD(&pdata->pwr_gpios);
	ret = -ENOMEM;
	for (i = 0; i < of_gpio_named_count(display_np, "power-control-gpio"); i++) {
		gpio = of_get_named_gpio_flags(display_np, "power-control-gpio",
					       i, &flags);
		if (gpio < 0)
			continue;

		og = devm_kzalloc(dev, sizeof(*og), GFP_KERNEL);
		if (!og)
			goto put_display_node;

		og->gpio = gpio;
		og->active_low = flags & OF_GPIO_ACTIVE_LOW;
		is_gpio_power = true;
		ret = devm_gpio_request(dev, gpio, "lcd-power-control-gpio");
		if (ret) {
			dev_err(dev, "request gpio %d failed\n", gpio);
			goto put_display_node;
		}

		ret = gpio_direction_output(gpio, og->active_low);
		if (ret) {
			dev_err(dev, "set direction output gpio %d failed\n", gpio);
			goto put_display_node;
		}
		list_add(&og->list, &pdata->pwr_gpios);
	}

	if (is_gpio_power)
		pdata->arkn141_lcdfb_power_control = arkn141_lcdfb_power_control_gpio;

	ret = arkn141_lcdfb_get_of_interface_types(display_np);
	if (ret < 0) {
		dev_err(dev, "invalid interface-type\n");
		goto put_display_node;
	}
	pdata->interface_type = ret;

	ret = arkn141_lcdfb_get_of_wiring_modes(display_np);
	if (ret < 0) {
		dev_err(dev, "invalid lcd-wiring-mode\n");
		goto put_display_node;
	}
	pdata->lcd_wiring_mode = ret;

	pdata->lcdcon_is_backlight = of_property_read_bool(display_np, "lcdcon-backlight");
	if (pdata->lcdcon_is_backlight) {
		pdata->pwm = devm_of_pwm_get(dev, display_np, NULL);
		if (IS_ERR(pdata->pwm) && PTR_ERR(pdata->pwm) != -EPROBE_DEFER) {
			dev_err(dev, "unable to request PWM\n");
			goto put_display_node;
		}

		ret = of_property_read_u32(display_np, "backlight-value", &pdata->backlight_value);
		if (ret < 0) {
			pdata->backlight_value = 30;
		}

		ret = of_property_read_u32(display_np, "backlight-delay", &pdata->backlight_delay);
		if (ret < 0) {
			pdata->backlight_delay = 100;
		}

		timer_setup(&pdata->backlight_timer, arkn141_backlight_timer_func, 0);

		/* Deferred power up the LCDC screen */
		mod_timer(&pdata->backlight_timer,
			   jiffies +
			   msecs_to_jiffies(pdata->backlight_delay));
	}

	pdata->lcd_clk_freq_autoupdate = of_property_read_bool(display_np, "clk-freq-autoupdate");

	timings = of_get_display_timings(display_np);
	if (!timings) {
		dev_err(dev, "failed to get display timings\n");
		ret = -EINVAL;
		goto del_timer;
	}

	timings_np = of_get_child_by_name(display_np, "display-timings");
	if (!timings_np) {
		dev_err(dev, "failed to find display-timings node\n");
		ret = -ENODEV;
		goto del_timer;
	}

	for (i = 0; i < of_get_child_count(timings_np); i++) {
		struct videomode vm;
		struct fb_videomode fb_vm;

		ret = videomode_from_timings(timings, &vm, i);
		if (ret < 0)
			goto put_timings_node;
		ret = fb_videomode_from_videomode(&vm, &fb_vm);
		if (ret < 0)
			goto put_timings_node;

		fb_add_videomode(&fb_vm, &info->modelist);

		if (vm.flags & DISPLAY_FLAGS_DE_HIGH)
			pdata->de_active_high = true;

		if (vm.flags & DISPLAY_FLAGS_PIXDATA_POSEDGE)
			pdata->pixelclk_active_high = true;
	}

	/*
	 * FIXME: Make sure we are not referencing any fields in display_np
	 * and timings_np and drop our references to them before returning to
	 * avoid leaking the nodes on probe deferral and driver unbind.
	 */

	return 0;

put_timings_node:
	of_node_put(timings_np);
del_timer:
	if (pdata->lcdcon_is_backlight)
		del_timer_sync(&pdata->backlight_timer);
put_display_node:
	of_node_put(display_np);
	return ret;
}

static int arkn141_lcdc_dev_init(struct arkn141_lcdfb_info *sinfo)
{
	/* set layer blend coff */
	lcdc_writel(sinfo, ARKN141_LCDC_OSD0_PARAM1, 
			lcdc_readl(sinfo, ARKN141_LCDC_OSD0_PARAM1) | (0xFF << 24));
	lcdc_writel(sinfo, ARKN141_LCDC_OSD1_PARAM1, 
			lcdc_readl(sinfo, ARKN141_LCDC_OSD1_PARAM1) | (0xFF << 24));
	lcdc_writel(sinfo, ARKN141_LCDC_OSD2_PARAM1, 
			lcdc_readl(sinfo, ARKN141_LCDC_OSD2_PARAM1) | (0xFF << 24));

	return 0;
}

static int arkn141_lcdfb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fb_info *info, *info_tmp;
	struct arkn141_lcdfb_info *sinfo;
	struct arkn141_lcdfb_pdata *pdata = NULL;
	struct resource *regs = NULL;
	struct resource *map = NULL;
	struct fb_modelist *modelist;
	int ret, i;

	dev_dbg(dev, "%s BEGIN\n", __func__);

	info = framebuffer_alloc(sizeof(struct arkn141_lcdfb_info), dev);
	if (!info) {
		dev_err(dev, "cannot allocate memory\n");
		ret = -ENOMEM;
		goto out;
	}

	sinfo = info->par;
	sinfo->pdev = pdev;
	sinfo->info = info;
	pdata = &sinfo->pdata;

	INIT_LIST_HEAD(&info->modelist);

	ret = arkn141_lcdfb_of_init(sinfo);
	if (ret)
		goto free_info;

	info->flags = ARKN141_LCDFB_FBINFO_DEFAULT;
	info->fbops = &arkn141_lcdfb_ops;

	info->fix = arkn141_lcdfb_fix;
	strcpy(info->fix.id, sinfo->pdev->name);

	/* Enable LCDC Clocks */
	sinfo->lcdc_clk = clk_get(dev, "lcdc_clk");
	if (IS_ERR(sinfo->lcdc_clk)) {
		ret = PTR_ERR(sinfo->lcdc_clk);
		goto free_info;
	}
	arkn141_lcdfb_start_clock(sinfo);

	modelist = list_first_entry(&info->modelist,
			struct fb_modelist, list);
	fb_videomode_to_var(&info->var, &modelist->mode);

	/* Set pixel clock */
	clk_set_rate(sinfo->lcdc_clk, PICOS2KHZ(info->var.pixclock) * 1000);
	info->var.pixclock = KHZ2PICOS(clk_get_rate(sinfo->lcdc_clk) / 1000);

	if (arkn141_lcdfb_check_var(&info->var, info) < 0) {
		dev_err(dev, "arkn141_lcdfb_check_var fail.\n");
		goto stop_clk;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(dev, "resources unusable\n");
		ret = -ENXIO;
		goto stop_clk;
	}

	/* LCDC registers */
	info->fix.mmio_start = regs->start;
	info->fix.mmio_len = resource_size(regs);

	if (!request_mem_region(info->fix.mmio_start,
				info->fix.mmio_len, pdev->name)) {
		ret = -EBUSY;
		goto free_fb;
	}

	sinfo->mmio = ioremap(info->fix.mmio_start, info->fix.mmio_len);
	if (!sinfo->mmio) {
		dev_err(dev, "cannot map LCDC registers\n");
		ret = -ENOMEM;
		goto release_mem;
	}

	sinfo->sysreg = ioremap(SYS_REG_BASE, 0x400);
	if (!sinfo->sysreg) {
		dev_err(dev, "cannot map sys registers\n");
		ret = -ENOMEM;
		goto unmap_mmio;
	}

	sinfo->irq_base = platform_get_irq(pdev, 0);
	if (sinfo->irq_base < 0) {
		dev_err(dev, "unable to get irq\n");
		ret = sinfo->irq_base;
		goto stop_clk;
	}

	/* Initialize video memory */
	map = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (map && map->start) {
		/* use a pre-allocated memory buffer */
		info->fix.smem_start = map->start;
		info->fix.smem_len = resource_size(map);
		if (!request_mem_region(info->fix.smem_start,
					info->fix.smem_len, pdev->name)) {
			ret = -EBUSY;
			goto stop_clk;
		}

		info->screen_base = ioremap_wc(info->fix.smem_start,
					       info->fix.smem_len);
		if (!info->screen_base) {
			ret = -ENOMEM;
			goto release_intmem;
		}
		//memset(info->screen_base, 0, info->var.xres * info->var.yres * 4);
		memset(info->screen_base, 0, info->fix.smem_len);

		/*
		 * Don't clear the framebuffer -- someone may have set
		 * up a splash image.
		 */
	} else {
		if(map && !map->start) {
			sinfo->smem_len = resource_size(map);
		}
		/* allocate memory buffer */
		ret = arkn141_lcdfb_alloc_video_memory(sinfo);
		if (ret < 0) {
			dev_err(dev, "cannot allocate framebuffer: %d\n", ret);
			goto stop_clk;
		}
	}

	/* Initialize PWM for contrast or backlight ("off") */
	if (pdata->lcdcon_is_backlight)
		init_backlight(sinfo);
	
	spin_lock_init(&sinfo->lock);
	mutex_init(&sinfo->mutex_lock);

	/* interrupt */
	ret = request_irq(sinfo->irq_base, arkn141_lcdfb_interrupt, 0, pdev->name, info);
	if (ret) {
		dev_err(dev, "request_irq failed: %d\n", ret);
		goto unmap_sysreg;
	}

	/* Some operations on the LCDC might sleep and
	 * require a preemptible task context */
	INIT_WORK(&sinfo->task, arkn141_lcdfb_task);

	init_waitqueue_head(&sinfo->vsync_waitq);

	ret = arkn141_lcdfb_init_fbinfo(sinfo);
	if (ret < 0) {
		dev_err(dev, "init fbinfo failed: %d\n", ret);
		goto unregister_irqs;
	}

	arkn141_lcdc_funcs_init(sinfo);

	ret = arkn141_lcdc_dev_init(sinfo);
	if (ret < 0) {
		dev_err(dev, "init lcdc dev failed: %d\n", ret);
		goto unregister_irqs;
	}

	ret = arkn141_lcdfb_set_par(info);
	if (ret < 0) {
		dev_err(dev, "set par failed: %d\n", ret);
		goto unregister_irqs;
	}

	dev_set_drvdata(dev, info);

	/*
	 * Tell the world that we're ready to go
	 */
	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(dev, "failed to register framebuffer device: %d\n", ret);
		goto reset_drvdata;
	}

	dev_info(dev, "fb%d: ARKN141 LCDC at 0x%08lx (mapped at %p), irq %d\n",
		       info->node, info->fix.mmio_start, sinfo->mmio, sinfo->irq_base);

	for(i = 0; i < 2; i++){
			info_tmp = framebuffer_alloc(sizeof(struct fb_info), dev);
			if (!info_tmp) {
					dev_err(dev, "cannot allocate memory\n");
					ret = -ENOMEM;
					goto out;
			}
			memcpy(info_tmp, info, sizeof(struct fb_info));
			register_framebuffer(info_tmp);
	}
	sinfo->atomic_flag = 0;
	memset(&sinfo->patomic, 0, sizeof(struct ark_disp_atomic) * OSD_LAYER_MAX);
	printk("%s success\n", __FUNCTION__);
	return 0;

reset_drvdata:
	dev_set_drvdata(dev, NULL);
	fb_dealloc_cmap(&info->cmap);
unregister_irqs:
	cancel_work_sync(&sinfo->task);
	free_irq(sinfo->irq_base, info);
unmap_sysreg:
	iounmap(sinfo->sysreg);
	mutex_destroy(&sinfo->mutex_lock);
unmap_mmio:
	exit_backlight(sinfo);
	iounmap(sinfo->mmio);
release_mem:
 	release_mem_region(info->fix.mmio_start, info->fix.mmio_len);
free_fb:
	if (map)
		iounmap(info->screen_base);
	else
		arkn141_lcdfb_free_video_memory(sinfo);

release_intmem:
	if (map)
		release_mem_region(info->fix.smem_start, info->fix.smem_len);
stop_clk:
	arkn141_lcdfb_stop_clock(sinfo);
	clk_put(sinfo->lcdc_clk);
free_info:
	framebuffer_release(info);
out:
	dev_dbg(dev, "%s FAILED ret=%d.\n", __func__, ret);
	printk(KERN_ERR "### %s failed\n", __FUNCTION__);
	return ret;
}

static int __exit arkn141_lcdfb_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fb_info *info = dev_get_drvdata(dev);
	struct arkn141_lcdfb_info *sinfo;
	struct arkn141_lcdfb_pdata *pdata;

	if (!info || !info->par)
		return 0;
	sinfo = info->par;
	pdata = &sinfo->pdata;

	cancel_work_sync(&sinfo->task);
	exit_backlight(sinfo);
	arkn141_lcdfb_power_control(sinfo, 0);
	unregister_framebuffer(info);
	arkn141_lcdfb_stop_clock(sinfo);
	clk_put(sinfo->lcdc_clk);
	fb_dealloc_cmap(&info->cmap);
	free_irq(sinfo->irq_base, info);
	iounmap(sinfo->sysreg);
	iounmap(sinfo->mmio);
	mutex_destroy(&sinfo->mutex_lock);
 	release_mem_region(info->fix.mmio_start, info->fix.mmio_len);
	if (platform_get_resource(pdev, IORESOURCE_MEM, 1)) {
		iounmap(info->screen_base);
		release_mem_region(info->fix.smem_start, info->fix.smem_len);
	} else {
		arkn141_lcdfb_free_video_memory(sinfo);
	}

	framebuffer_release(info);

	return 0;
}

#ifdef CONFIG_PM

static int arkn141_lcdfb_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int arkn141_lcdfb_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define arkn141_lcdfb_suspend	NULL
#define arkn141_lcdfb_resume	NULL
#endif

static struct platform_driver arkn141_lcdfb_driver = {
	.probe		= arkn141_lcdfb_probe,
	.remove		= __exit_p(arkn141_lcdfb_remove),
	.suspend	= arkn141_lcdfb_suspend,
	.resume		= arkn141_lcdfb_resume,
	.driver		= {
		.name	= "arkn141_lcdfb",
		.of_match_table	= of_match_ptr(arkn141_lcdfb_dt_ids),
	},
};
module_platform_driver(arkn141_lcdfb_driver);

/* static int __init arkn141_lcdfb_init(void)
{
    int ret;

    ret = platform_driver_register(&arkn141_lcdfb_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s %d: failed to register arkn141_lcdfb_driver\n",
            __FUNCTION__, __LINE__);
    }

    return ret;
}
arch_initcall(arkn141_lcdfb_init); */

MODULE_DESCRIPTION("Arkn141 LCD Controller framebuffer driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
