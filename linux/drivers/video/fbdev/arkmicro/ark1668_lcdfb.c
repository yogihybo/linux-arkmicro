/*
 * Arkmicro ark1668 lcd driver
 *
 * Licensed under GPLv2 or later.
 */

#define pr_fmt(fmt) "ark1668-lcdfb: " fmt

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
#include <linux/workqueue.h>
#include <linux/ktime.h>
#include <linux/ratelimit.h>
#include <video/of_display_timing.h>
#include <video/display_timing.h>
#include <video/videomode.h>
#include "ark1668_lcdc.h"

#define BACKLIGHT_PWM_PERIOD		50000
#define BACKLIGHT_MAX_BRIGHTNESS	100

#define SYS_REG_BASE		0xe4900000
#define SYS_CLK_DELAY		0x70
#define SYS_PIXEL_CLK_INV_OFFSET	16
#define SYS_ANALOG_REG1		0x144
#define SYS_LVDS_CTRL_CFG	0x190

#define BALCK_BACKCOLOR		0x108080

/* Must match U-Boot's BOOTLOGO_SD_ADDR (board/arkmicro/ark1668_limcet_p305/
 * ark1668_display_cfg.c) -- the physical address U-Boot draws the real
 * splash image into, deliberately NOT the same address as this driver's
 * own smem_start (see the probe()-time copy that uses this). */
#define ARK1668_BOOTLOGO_PHYS_ADDR	0x0b400000

struct ark1668_lcdfb_power_ctrl_gpio {
	int gpio;
	int active_low;

	struct list_head list;
};

#define lcdc_readl(sinfo, reg)		__raw_readl((sinfo)->mmio+(reg))
#define lcdc_writel(sinfo, reg, val)	__raw_writel((val), (sinfo)->mmio+(reg))
#define lcdc_readl_sys(sinfo, reg)		__raw_readl((sinfo)->sysreg+(reg))
#define lcdc_writel_sys(_syssinfo, reg, val)	__raw_writel((val), (sinfo)->sysreg+(reg))

#define	ARK1668_LCDFB_FBINFO_DEFAULT	(FBINFO_DEFAULT \
					 | FBINFO_HWACCEL_NONE)


extern void ark_itu656_display_int_handler(void);


static int ark1668_set_backlight(struct ark1668_lcdfb_pdata *pdata,
								 int brightness)
{
	int duty = brightness * BACKLIGHT_PWM_PERIOD / BACKLIGHT_MAX_BRIGHTNESS;

	pwm_enable(pdata->pwm);
	pwm_config(pdata->pwm, duty, BACKLIGHT_PWM_PERIOD);
	pdata->backlight_value = brightness;

	return 0;
}

/* some bl->props field just changed */
static int ark1668_bl_update_status(struct backlight_device *bl)
{
	struct ark1668_lcdfb_info *sinfo = bl_get_data(bl);
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;
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

	ark1668_set_backlight(pdata, brightness);

	bl->props.fb_blank = bl->props.power = sinfo->bl_power = power;

	return 0;
}

static int ark1668_bl_get_brightness(struct backlight_device *bl)
{
	struct ark1668_lcdfb_info *sinfo = bl_get_data(bl);
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;

	return pdata->backlight_value;
}

static const struct backlight_ops ark1668_lcdc_bl_ops = {
	.update_status = ark1668_bl_update_status,
	.get_brightness = ark1668_bl_get_brightness,
};

static void init_backlight(struct ark1668_lcdfb_info *sinfo)
{
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;
	struct backlight_properties props;
	struct backlight_device	*bl;

	sinfo->bl_power = FB_BLANK_UNBLANK;

	if (sinfo->backlight)
		return;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = BACKLIGHT_MAX_BRIGHTNESS;
	bl = backlight_device_register("backlight", &sinfo->pdev->dev, sinfo,
				       &ark1668_lcdc_bl_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&sinfo->pdev->dev, "error %ld on backlight register\n",
				PTR_ERR(bl));
		return;
	}
	sinfo->backlight = bl;

	bl->props.power = FB_BLANK_UNBLANK;
	bl->props.fb_blank = FB_BLANK_UNBLANK;
	bl->props.brightness = ark1668_bl_get_brightness(bl);

	ark1668_set_backlight(pdata, pdata->backlight_value);
}

static void exit_backlight(struct ark1668_lcdfb_info *sinfo)
{
	if (!sinfo->backlight)
		return;

	if (sinfo->backlight->ops) {
		sinfo->backlight->props.power = FB_BLANK_POWERDOWN;
		sinfo->backlight->ops->update_status(sinfo->backlight);
	}
	backlight_device_unregister(sinfo->backlight);
}

static inline void ark1668_lcdfb_power_control(struct ark1668_lcdfb_info *sinfo, int on)
{
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;

	if (pdata->ark1668_lcdfb_power_control)
		pdata->ark1668_lcdfb_power_control(pdata, on);
}

static const struct fb_fix_screeninfo ark1668_lcdfb_fix __initconst = {
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_TRUECOLOR,
	.xpanstep	= 0,
	.ypanstep	= 1,
	.ywrapstep	= 0,
	.accel		= FB_ACCEL_NONE,
};

/* static void ark1668_lcdfb_stop(struct ark1668_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ARK1668_LCDC_EN, 0);
} */

static void ark1668_lcdfb_start(struct ark1668_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ARK1668_LCDC_EN, 1);
}

static inline void ark1668_lcdfb_free_video_memory(struct ark1668_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;

	dma_free_wc(info->device, info->fix.smem_len, info->screen_base,
		    info->fix.smem_start);
}

/**
 *	ark1668_lcdfb_alloc_video_memory - Allocate framebuffer memory
 *	@sinfo: the frame buffer to allocate memory for
 *
 * 	This function is called only from the ark1668_lcdfb_probe()
 * 	so no locking by fb_info->mm_lock around smem_len setting is needed.
 */
static int ark1668_lcdfb_alloc_video_memory(struct ark1668_lcdfb_info *sinfo)
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

static const struct fb_videomode *ark1668_lcdfb_choose_mode(struct fb_var_screeninfo *var,
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
 *      ark1668_lcdfb_check_var - Validates a var passed in.
 *      @var: frame buffer variable screen structure
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Checks to see if the hardware supports the state requested by
 *	var passed in. This function does not alter the hardware
 *	state!!!  This means the data stored in struct fb_info and
 *	struct ark1668_lcdfb_info do not change. This includes the var
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
static int ark1668_lcdfb_check_var(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	struct device *dev = info->device;
	struct ark1668_lcdfb_info *sinfo = info->par;
	unsigned long clk_value_khz;

	clk_value_khz = clk_get_rate(sinfo->lcdc_clk) / 1000;

	dev_dbg(dev, "%s:\n", __func__);

	if (!(var->pixclock && var->bits_per_pixel)) {
		/* choose a suitable mode if possible */
		if (!ark1668_lcdfb_choose_mode(var, info)) {
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

	//if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres * 3;

	/* Force same alignment for each line */
	var->xres = (var->xres + 3) & ~3UL;
	var->xres_virtual = (var->xres_virtual + 3) & ~3UL;

	var->red.msb_right = var->green.msb_right = var->blue.msb_right = 0;
	var->transp.msb_right = 0;
	var->transp.offset = var->transp.length = 0;

	/* Was an unconditional var->xoffset = var->yoffset = 0 here --
	 * check_var() is called on every FBIOPUT_VSCREENINFO, not just
	 * once at startup (confirmed via strace: 12 FBIOPUT_VSCREENINFO
	 * calls interleaved with 23 FBIOPAN_DISPLAY frame flips in a
	 * single DirectFB session, docs/logs/directfb_strace.txt). Forcing
	 * yoffset back to 0 (page 1) on every one of those benign
	 * mode-set calls stomps whichever page DirectFB's own
	 * triple-buffer rotation was actually displaying, regardless of
	 * whether that page held current content -- validate/clamp
	 * instead of unconditionally overwriting. See
	 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 19.
	 */
	var->xoffset = 0;
	if (var->yoffset + var->yres > var->yres_virtual ||
	    var->yoffset % var->yres != 0)
		var->yoffset = 0;

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
	var->vsync_len = min_t(u32, var->vsync_len,
			(ARK1668_LCDC_VPW >> ARK1668_LCDC_VPW_OFFSET) + 1);
	var->upper_margin = min_t(u32, var->upper_margin,
			ARK1668_LCDC_VBP);
	var->lower_margin = min_t(u32, var->lower_margin,
			ARK1668_LCDC_VFP >> ARK1668_LCDC_VFP_OFFSET);
	var->right_margin = min_t(u32, var->right_margin,
			ARK1668_LCDC_HFP + 1);
	var->hsync_len = min_t(u32, var->hsync_len,
			(ARK1668_LCDC_HPW >> ARK1668_LCDC_HPW_OFFSET) + 1);
	var->left_margin = min_t(u32, var->left_margin,
			(ARK1668_LCDC_HBP >> ARK1668_LCDC_HBP_OFFSET) + 1);

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
		/* Deliberately NOT declaring a transp field here (left at
		 * offset=length=0 from the unconditional reset above, same
		 * as every other depth). Qt4 QWS's QLinuxFbScreen::
		 * setPixelFormat() (libQtGui.so.4.7.4, decompiled) does an
		 * exact byte-for-byte memcmp of the reported red/green/
		 * blue/transp fb_bitfield triples against a small table of
		 * known layouts: a full 48-byte match against its "RGB
		 * 16/8/0 + transp 24/8" template selects QImage::
		 * Format_ARGB32 == 5, i.e. *straight* (non-premultiplied)
		 * alpha -- Qt's LinuxFB backend never offers
		 * Format_ARGB32_Premultiplied for this template set. Qt4's
		 * raster paint engine's fast SourceOver composition paths
		 * assume premultiplied alpha; painting translucent content
		 * (anti-aliased icons/widgets) onto a straight-ARGB32
		 * on-screen surface produces exactly the "opaque pixels
		 * fine, alpha-blended pixels skewed" symptom this project
		 * has been chasing at the LCDC register level -- this bug
		 * never touches LCDC hardware at all, which is why the
		 * earlier register-level investigation (see
		 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b)
		 * conclusively found the LCDC register state to be
		 * byte-identical to stock's correctly-rendering config.
		 *
		 * Previously this case set transp.offset=24/length=8,
		 * which (matched byte-for-byte against our own declared
		 * red/green/blue below) hits that exact template. Leaving
		 * transp undeclared instead makes Qt's 48-byte match fail
		 * and fall through to its 36-byte RGB-only comparison,
		 * which still matches on red/green/blue and selects
		 * Format_RGB32 == 4 (fully opaque) -- forcing Qt's own
		 * software compositor to flatten all translucent content
		 * before it ever reaches /dev/fb0, matching how stock's
		 * DirectFB path produces fully opaque scanout data and
		 * never actually depends on this SoC's LCDC hardware alpha
		 * blend circuit (already shown to behave unpredictably
		 * regardless of blend_mode/rgb_order register value).
		 * NOT YET HARDWARE-TESTED.
		 */
		/* fall through */
	case 24:
		/* FIXED AGAIN 2026-07-25 (checklist section 66): hardcoded to
		 * a fixed red.offset=16/blue.offset=0, no longer derived from
		 * pdata->lcd_wiring_mode at all. This whole function went
		 * through several wiring_mode-conditional revisions (see git
		 * history), all of them built on the wrong assumption that
		 * arkdata.ini's real RgbMode is 0 (BGR) -- it's actually 5
		 * (RGB), confirmed directly from a real boot log ("[arkdata.ini]
		 * -> DTB .../lcd-wiring-mode=RGB"). The one thing genuinely
		 * proven correct on real hardware is U-Boot's own bootlogo
		 * (board/arkmicro/ark1668_limcet_p305/ark1668_display_cfg.c),
		 * which hardcodes red@16/green@8/blue@0 packing (see
		 * build_tools/convert_bootlogo.py's own documented convention)
		 * paired with a hardcoded rgb_order=0 (now also hardcoded to
		 * match in fb_set_par()/ARKFB_INIT_DISPLAY, see those
		 * comments) -- unconditionally, never reading RgbMode at all.
		 * Matching that fixed pairing here too, instead of deriving
		 * from wiring_mode. See
		 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 66. */
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
/* static void ark1668_lcdfb_reset(struct ark1668_lcdfb_info *sinfo)
{
	might_sleep();

	ark1668_lcdfb_stop(sinfo);
	ark1668_lcdfb_start(sinfo);
} */

static int ark1668_lcdfb_pan_display(struct fb_var_screeninfo *var,
			       struct fb_info *info)
{
	struct ark1668_lcdfb_info *sinfo = info->par;
	struct fb_fix_screeninfo *fix = &info->fix;
	u32 addr;
	unsigned long flags;

	addr = fix->smem_start + var->yoffset * fix->line_length
		+ var->xoffset * info->var.bits_per_pixel / 8;

	/* Stock's real ark_disp_fb_pan_display (vmlinux.elf @ 0x802e2900)
	 * wraps its equivalent OSD1_ADDR register write in an IRQ-disabled
	 * critical section (ark_disp_set_next_buf_start_addr ->
	 * ark_disp_set_osd_data_addr, confirmed via disassembly) -- matched
	 * here. Note: this does NOT add any wait for GPU render completion;
	 * stock's write is just as immediate as ours was. It only protects
	 * against a concurrent interrupt (the vsync IRQ handler) observing
	 * a torn update -- see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
	 * section 15/16 for why the actual red/black-screen race is not
	 * fixable at this layer.
	 */
	local_irq_save(flags);
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_ADDR, addr);
	local_irq_restore(flags);

	return 0;
}

/**
 *      ark1668_lcdfb_set_par - Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Using the fb_var_screeninfo in fb_info we set the resolution
 *	of the this particular framebuffer. This function alters the
 *	par AND the fb_fix_screeninfo stored in fb_info. It doesn't
 *	not alter var in fb_info since we are using that data. This
 *	means we depend on the data in var inside fb_info to be
 *	supported by the hardware.  ark1668_lcdfb_check_var is always called
 *	before ark1668_lcdfb_set_par to ensure this.  Again if you can't
 *	change the resolution you don't need this function.
 *
 */
static int ark1668_lcdfb_set_par(struct fb_info *info)
{
	struct ark1668_lcdfb_info *sinfo = info->par;
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long value;
	unsigned long bits_per_line;
        static int set_par = 0;

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

	/* Now, the LCDC core... */
	value = lcdc_readl_sys(sinfo, SYS_CLK_DELAY);
	if (pdata->pixelclk_active_high)
		value |= (1 << SYS_PIXEL_CLK_INV_OFFSET);
	else
		value &= ~(1 << SYS_PIXEL_CLK_INV_OFFSET);
	lcdc_writel_sys(sinfo, SYS_CLK_DELAY, value);

	/* Initialize control register */
	if(!set_par){
		value = (6 << 23) | (1 << 0);
		/* set interrupt at the start of the front porch when vfp is not zero */
		if (info->var.lower_margin)
			value |= (3 << 21);
		/* FIXED AGAIN 2026-07-25 (checklist section 66): neither the
		 * wiring_mode-direct-passthrough model (section 58's revert)
		 * nor the translated version before it (section 52, wrong)
		 * actually produces correct colors on real hardware --
		 * confirmed by the user after testing section 58's revert.
		 * The one thing that IS empirically, hardware-confirmed
		 * correct is U-Boot's own real bootlogo code
		 * (board/arkmicro/ark1668_limcet_p305/ark1668_display_cfg.c),
		 * which hardcodes rgb_order=0 (DISP_RGB_888, order nibble 0)
		 * UNCONDITIONALLY -- it never reads RgbMode/lcd_wiring_mode at
		 * all. Since arkdata.ini's real deployed RgbMode is 5 (RGB),
		 * not 0 (BGR) as earlier sessions assumed throughout this
		 * whole investigation, the direct-passthrough model was
		 * writing rgb_order=5 here -- different from the bootlogo's
		 * proven-correct 0. Hardcoding rgb_order=0 unconditionally,
		 * matching the bootlogo exactly, instead of deriving it from
		 * lcd_wiring_mode at all. See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
		 * section 66. */
		lcdc_writel(sinfo, ARK1668_LCDC_CONTROL, value);
	}

	/* Initialize color convert table */
	value = (425 << 20) | (91 << 10) | (298 << 0);
	lcdc_writel(sinfo, ARK1668_LCDC_Y2R_COEF321, value);

	value = (465 << 20) | (184 << 10) | (96 << 0);
	lcdc_writel(sinfo, ARK1668_LCDC_Y2R_COEF654, value);

	value = lcdc_readl(sinfo, ARK1668_LCDC_Y2R_COEF7);
	value |= (1 << 12) | (41 << 0);
	lcdc_writel(sinfo, ARK1668_LCDC_Y2R_COEF7, value);

	/* timing */
	value = (info->var.hsync_len - 1) << ARK1668_LCDC_HPW_OFFSET;
	value |= (info->var.left_margin - 1) << ARK1668_LCDC_HBP_OFFSET;
	value |= (info->var.right_margin - 1);
	lcdc_writel(sinfo, ARK1668_LCDC_TIMING0, value);

	value = info->var.lower_margin << ARK1668_LCDC_VFP_OFFSET;
	value |= (info->var.vsync_len - 1) << ARK1668_LCDC_VPW_OFFSET;
	value |= (info->var.xres - 1);
	lcdc_writel(sinfo, ARK1668_LCDC_TIMING1, value);

	value = pdata->de_active_high << ARK1668_LCDC_IOE_OFFSET;
	value |= !!(info->var.sync & FB_SYNC_HOR_HIGH_ACT) << ARK1668_LCDC_IHS_OFFSET;
	value |= !!(info->var.sync & FB_SYNC_VERT_HIGH_ACT) << ARK1668_LCDC_IVS_OFFSET;
	value |= (info->var.yres - 1) << ARK1668_LCDC_LPS_OFFSET;
	value |= info->var.upper_margin;
	lcdc_writel(sinfo, ARK1668_LCDC_TIMING2, value);

	/* Initialize specific screen type */
	if (pdata->interface_type == ARK1668_LCDC_INTERFACE_LVDS) {
		//value = lcdc_readl_sys(sinfo, SYS_ANALOG_REG1);
		//value |= (1 << 26);
		//lcdc_writel_sys(sinfo, SYS_ANALOG_REG1, value);

		lcdc_writel_sys(sinfo, SYS_LVDS_CTRL_CFG, pdata->lvds_con);
	}

        lcdc_writel(sinfo, ARK1668_LCDC_DITHERING, pdata->dithering_con);

	/* Display osd layer1(fb0) size,pos,format,addr... */
	ark1668_lcdfb_pan_display(&info->var, info);
	value = (info->var.yres << ARK1668_LCDC_HEIGHT_OFFSET) | info->var.xres;
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_SIZE, value);
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_SOURCE_SIZE, value);
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_POS, 0);
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_WIN_POINT, 0);
	/* rgb_order (bits 18-20) is derived from pdata->lcd_wiring_mode, not
	 * preserved from whatever was already in the register. An earlier
	 * fix (2026-07-19) changed this from a flat overwrite (which
	 * unconditionally zeroed rgb_order on every fb_set_par() call) to
	 * an RMW that preserves bits 18-22 -- that stopped this function
	 * from clobbering a value set elsewhere, but nothing in this driver
	 * ever actually SETS rgb_order to a value matching the wiring mode
	 * in the first place: ARKFB_INIT_DISPLAY (what libarkcmn.so's
	 * arkapi_init_fb_display() actually calls at startup, see
	 * ark1668_lcdc_funcs.c) only sets position/size, never format: so
	 * the preserved value was always just whatever U-Boot or hardware
	 * reset happened to leave behind, unrelated to lcd_wiring_mode.
	 * Confirmed via decompile of stock's real ark_disp_fb_set_par()
	 * (vmlinux.elf @ 0x802e2a40): it strictly validates the userspace-
	 * supplied var.red/green/blue offsets and derives a matching
	 * rgb_order value (0 or 5 in the traced branches -- exactly
	 * ARK_LCDC_WIRING_BGR/RGB) which it then writes through
	 * ark_disp_set_layer_cfg() on every set_par call. This is the
	 * missing piece behind wrong colors persisting even after
	 * check_var()'s var.red/blue.offset fix: userspace was writing
	 * pixels in one byte order while the LCDC's blend-unit compositor
	 * math used a stale/unrelated rgb_order value, unaffected by
	 * anything var.red/blue.offset says. See
	 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 33. */
	/* FIXED AGAIN 2026-07-25 (checklist section 66): rgb_order hardcoded
	 * to 0, matching U-Boot's real bootlogo exactly -- see the CONTROL
	 * register comment above for the full explanation. Not derived
	 * from pdata->lcd_wiring_mode at all anymore. */
	value = lcdc_readl(sinfo, ARK1668_LCDC_OSD1_CTL);
	value &= ~(0x7ff << 12);
	value |= (1 << 17) | (ARK1668_LCDC_FORMAT_RGBA888 << 12) | 0xff;
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_CTL, value);

	/* RE-ENABLED 2026-07-27 (checklist section 61-67's "AA video runs
	 * but screen stays black" investigation) -- the disable below was
	 * wrong. Real mechanism, confirmed via disassembly of
	 * libMsnCarAuto.so: CarAutoWindow::isVideoAppBkTransparent() (a
	 * stored flag, offset 0x55) and MsnCoreApp's own
	 * "background:transparent;" Qt stylesheet strings drive
	 * CarAutoWindow to paint literal RGB black where Android
	 * Auto/CarPlay video should show through -- it relies entirely on
	 * OSD1's *colorkey* comparator to punch that hole, not on real
	 * ARGB alpha (which ark1668_lcdfb_check_var()'s Format_RGB32
	 * forcing, above/elsewhere in this file, makes impossible to rely
	 * on anyway -- see that fix's own comment for why). "Nothing in
	 * this driver's userspace ABI ever asks for a colorkey" (the
	 * previous reasoning for disabling this) missed that userspace
	 * asks for it *implicitly*, by painting the exact colorkey color
	 * and depending on it already being enabled from init -- same as
	 * stock's own real, unconditional behavior (see the original
	 * finding this comment used to describe, still accurate):
	 * U-Boot's ark_display_initialize_common()
	 * (board/arkmicro/ark1668_limcet_p305/ark1668_lcd.c) unconditionally
	 * enables a BLACK colorkey on OSD1, gated by
	 * `#ifdef BOOT_CONFIG_PIXEL_ALPHA`, referenced but never #define'd
	 * anywhere in the vendor tree so the branch always compiles in.
	 * Register offset (LCDC_BASE+0xec) and bit-24 enable flag confirmed
	 * against stock's real ark_disp_set_osd_colorkey() (vmlinux.elf @
	 * 0x802debb8). Value: (1<<24)|(BLACK_Y<<16)|(BLACK_U<<8)|BLACK_V,
	 * BLACK_Y/U/V = 0x10/0x80/0x80 (board/arkmicro/ark1668_limcet_p305/
	 * ark1668_lcd.h) -- limited-range YCbCr black, the standard BT.601
	 * conversion of RGB (0,0,0), confirming the hardware comparator
	 * runs post-YCbCr-conversion (the driver's earlier comment here
	 * left this unconfirmed). NOT YET HARDWARE-TESTED. */
	lcdc_writel(sinfo, ARK1668_LCDC_COLOR_KEY_MASK_VALUE_OSD1,
		    (1 << 24) | (0x10 << 16) | (0x80 << 8) | 0x80);

	/* Open OSD1 layer. Deliberately unconditional (not gated by set_par
	 * like the wiring-mode block above): this is a read-modify-write of
	 * a single bit, so re-running it on every fb_set_par call is
	 * harmless, and guarantees OSD1 is (re-)enabled every time this
	 * function runs regardless of whatever earlier disabled it -- unlike
	 * the wiring-mode block above, which does a full non-RMW overwrite
	 * of ARK1668_LCDC_CONTROL and must stay latched to avoid clobbering
	 * this very bit on a second call. Previously this was also gated by
	 * set_par, making it a one-shot: only the very first fb_set_par call
	 * of the boot (from any caller) could ever enable OSD1, and nothing
	 * -- including a userspace FBIOPUT_VSCREENINFO -- could re-enable it
	 * if something later disabled it. See docs/DISPLAY_SUBSYSTEM.md.
	 */
	value = lcdc_readl(sinfo, ARK1668_LCDC_CONTROL);
	value |= (1 << ARK1668_LCDC_OSD1_EN_OFFSET);
	lcdc_writel(sinfo, ARK1668_LCDC_CONTROL, value);

	/* Clear all interrupts */
	lcdc_writel(sinfo, ARK1668_LCDC_INT_STATUS, 0);

	/* Enable frame interrupt */
	lcdc_writel(sinfo, ARK1668_LCDC_INT_CTL, ARK1668_LCDC_INT_LCD_FRAME);

	ark1668_lcdfb_start(sinfo);

	dev_dbg(info->device, "  * DONE\n");

        
        set_par = 1;

	return 0;
}

/* static int ark1668_lcdfb_blank(int blank_mode, struct fb_info *info)
{
	struct ark1668_lcdfb_info *sinfo = info->par;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		ark1668_lcdfb_start(sinfo);
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		break;
	case FB_BLANK_POWERDOWN:
		ark1668_lcdfb_stop(sinfo);
		break;
	default:
		return -EINVAL;
	}

	return ((blank_mode == FB_BLANK_NORMAL) ? 1 : 0);
} */


static int ark1668_lcdfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned int offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset < info->fix.smem_len) {
		return dma_mmap_wc(info->device, vma, info->screen_base,
				   info->fix.smem_start, info->fix.smem_len);
	}

	return -EINVAL;
}

static struct fb_ops ark1668_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= ark1668_lcdfb_check_var,
	.fb_set_par	= ark1668_lcdfb_set_par,
	//.fb_blank	= ark1668_lcdfb_blank,
	.fb_pan_display	= ark1668_lcdfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_ioctl       = ark1668_lcdfb_ioctl,
	.fb_mmap	= ark1668_lcdfb_mmap,
};

static irqreturn_t ark1668_lcdfb_interrupt(int irq, void *dev_id)
{
	struct fb_info *info = dev_id;
	struct ark1668_lcdfb_info *sinfo = info->par;
	u32 status;
	int i;

	status = lcdc_readl(sinfo, ARK1668_LCDC_INT_STATUS);

	/* clear intr except scale writeback intr */
	lcdc_writel(sinfo, ARK1668_LCDC_INT_STATUS, ARK1668_LCDC_INT_SCAL_WB);

	if (status & ARK1668_LCDC_INT_LCD_FRAME) {
		for (i = 0; i < ARK1668_LAYER_MAX; i++) {
			if (sinfo->render_addr[i].yaddr) {
				if(i <= OSD_LAYER3)
					ark1668_lcdc_set_osd_addr(i, sinfo->render_addr[i].yaddr);
				else
					ark1668_lcdc_set_video_addr(i-OSD_LAYER_MAX, sinfo->render_addr[i].yaddr,
						sinfo->render_addr[i].cbaddr, sinfo->render_addr[i].craddr);
				sinfo->render_addr[i].yaddr = 0;
			}
		}

		sinfo->vsync_flag = 1;
		wake_up_interruptible(&sinfo->vsync_waitq);
		schedule_work(&sinfo->task);
	}

	return IRQ_HANDLED;
}

/*
 * LCD controller task (to reset the LCD)
 */
static void ark1668_lcdfb_task(struct work_struct *work)
{
        /*struct ark1668_lcdfb_info *sinfo =
                container_of(work, struct ark1668_lcdfb_info, task);*/
#ifdef CONFIG_ARK1668_ITU656
	/*
	 * This runs on every single LCDC vsync frame interrupt (panel
	 * refresh rate, content-independent), for as long as the display
	 * is on -- not just when the backup camera is active. Timed to
	 * correlate against the audio XRUN/mute-flap logging in
	 * pcm_dmaengine.c / ark1668-sddac-codec.c: if this work item's
	 * runtime or scheduling delay lines up with play:225/digital_mute
	 * events, it's a real candidate for stealing CPU time from audio
	 * on this single-core system, independent of AA video content.
	 */
	{
		ktime_t __start = ktime_get();
		s64 __ns;
		static DEFINE_RATELIMIT_STATE(itu656_task_rs, HZ, 5);

		ark_itu656_display_int_handler();

		__ns = ktime_to_ns(ktime_sub(ktime_get(), __start));
		if (__ns > 1000000 && __ratelimit(&itu656_task_rs))
			pr_warn("itu656 display task took %lldns\n", __ns);
	}
#endif
}

static int __init ark1668_lcdfb_init_fbinfo(struct ark1668_lcdfb_info *sinfo)
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

static void ark1668_lcdfb_start_clock(struct ark1668_lcdfb_info *sinfo)
{
	clk_prepare_enable(sinfo->lcdc_clk);
}

static void ark1668_lcdfb_stop_clock(struct ark1668_lcdfb_info *sinfo)
{
	clk_disable_unprepare(sinfo->lcdc_clk);
}

static const struct of_device_id ark1668_lcdfb_dt_ids[] = {
	{ .compatible = "arkmicro,ark1668-lcdc",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, ark1668_lcdfb_dt_ids);

static const char *ark1668_lcdfb_interface_types[] = {
	[ARK1668_LCDC_INTERFACE_TTL]	= "TTL",
	[ARK1668_LCDC_INTERFACE_LVDS]	= "LVDS",
};

static int ark1668_lcdfb_get_of_interface_types(struct device_node *np)
{
	const char *type;
	int err, i;

	err = of_property_read_string(np, "interface-type", &type);
	if (err < 0)
		return ARK1668_LCDC_INTERFACE_TTL;

	for (i = 0; i < ARRAY_SIZE(ark1668_lcdfb_interface_types); i++)
		if (!strcasecmp(type, ark1668_lcdfb_interface_types[i]))
			return i;

	return -ENODEV;
}

static const char *ark1668_lcdfb_wiring_modes[] = {
	[ARK_LCDC_WIRING_BGR]	= "BGR",
	[ARK_LCDC_WIRING_GBR]	= "GBR",
	[ARK_LCDC_WIRING_RBG]	= "RBG",
	[ARK_LCDC_WIRING_BRG]	= "BRG",
	[ARK_LCDC_WIRING_GRB]	= "GRB",
	[ARK_LCDC_WIRING_RGB]	= "RGB",
};

static int ark1668_lcdfb_get_of_wiring_modes(struct device_node *np)
{
	const char *mode;
	int err, i;

	err = of_property_read_string(np, "lcd-wiring-mode", &mode);
	if (err < 0)
		return ARK_LCDC_WIRING_BGR;

	for (i = 0; i < ARRAY_SIZE(ark1668_lcdfb_wiring_modes); i++)
		if (!strcasecmp(mode, ark1668_lcdfb_wiring_modes[i]))
			return i;

	return -ENODEV;
}

static void ark1668_lcdfb_power_control_gpio(struct ark1668_lcdfb_pdata *pdata, int on)
{
	struct ark1668_lcdfb_power_ctrl_gpio *og;

	list_for_each_entry(og, &pdata->pwr_gpios, list)
		gpio_set_value(og->gpio, on ? !og->active_low : og->active_low);
}

/*
 * Timer function for delayed backlight power up/down
 */
static void ark1668_backlight_timer_func(struct timer_list *t)
{
	struct ark1668_lcdfb_info *sinfo = from_timer(sinfo, t, pdata.backlight_timer);
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;

	ark1668_lcdfb_power_control(sinfo, 1);
	del_timer_sync(&pdata->backlight_timer);
}

static int ark1668_lcdfb_of_init(struct ark1668_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	struct ark1668_lcdfb_pdata *pdata = &sinfo->pdata;
	struct fb_var_screeninfo *var = &info->var;
	struct device *dev = &sinfo->pdev->dev;
	struct device_node *np =dev->of_node;
	struct device_node *display_np;
	struct device_node *timings_np;
	struct display_timings *timings;
	enum of_gpio_flags flags;
	struct ark1668_lcdfb_power_ctrl_gpio *og;
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
		pdata->ark1668_lcdfb_power_control = ark1668_lcdfb_power_control_gpio;

	ret = ark1668_lcdfb_get_of_interface_types(display_np);
	if (ret < 0) {
		dev_err(dev, "invalid interface-type\n");
		goto put_display_node;
	}
	pdata->interface_type = ret;
	if (pdata->interface_type == ARK1668_LCDC_INTERFACE_LVDS) {
		ret = of_property_read_u32(display_np, "lvds-con", &pdata->lvds_con);
		if (ret < 0) {
			dev_err(dev, "failed to get property lvds-con\n");
			goto put_display_node;
		}
	}

        //set dithering_con vp
        ret = of_property_read_u32(display_np, "dithering-con", &pdata->dithering_con);
        if (ret < 0) {
                pdata->dithering_con = 0;
        }
        
        ret = of_property_read_u32(display_np, "osd1-vp", &pdata->osd1_vp);
        if (ret < 0) {
                pdata->osd1_vp = 0x00408080;
        }

        ret = of_property_read_u32(display_np, "osd2-vp", &pdata->osd2_vp);
        if (ret < 0) {
                pdata->osd2_vp = 0x00408080;
        }

        ret = of_property_read_u32(display_np, "osd3-vp", &pdata->osd3_vp);
        if (ret < 0) {
                pdata->osd3_vp = 0x00408080;
        }

        ret = of_property_read_u32(display_np, "video1-vp", &pdata->video1_vp);
        if (ret < 0) {
                pdata->video1_vp = 0x00408080;
        }

        ret = of_property_read_u32(display_np, "video2-vp", &pdata->video2_vp);
        if (ret < 0) {
                pdata->video2_vp = 0x00408080;
        }

	ret = ark1668_lcdfb_get_of_wiring_modes(display_np);
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

		timer_setup(&pdata->backlight_timer, ark1668_backlight_timer_func, 0);

		/* Deferred power up the LCDC screen */
		mod_timer(&pdata->backlight_timer,
			   jiffies +
			   msecs_to_jiffies(pdata->backlight_delay));
	}

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

static int ark1668_lcdc_dev_init(struct ark1668_lcdfb_info *sinfo)
{
    struct ark1668_lcdfb_pdata *p = &sinfo->pdata;
	struct device *dev = &sinfo->pdev->dev;
	u32 prio[5];

	/* set lcd back color */
	lcdc_writel(sinfo, ARK1668_LCDC_BACK_COLOR, BALCK_BACKCOLOR);

	/* set layer1(fb0) vp */
	lcdc_writel(sinfo, ARK1668_LCDC_OSD1_VP_REG1, p->osd1_vp);
        lcdc_writel(sinfo, ARK1668_LCDC_OSD2_VP_REG_1, p->osd2_vp);
        lcdc_writel(sinfo, ARK1668_LCDC_OSD3_VP_REG_1, p->osd3_vp);
        lcdc_writel(sinfo, ARK1668_LCDC_VIDEO_VP_REG_1, p->video1_vp);
        lcdc_writel(sinfo, ARK1668_LCDC_VIDEO2_VP_REG_1, p->video2_vp);

	/* set layer priority and blend mode.
	 *
	 * bits[15:12] of MODE_LCD_REG0 are OSD1's 4-bit "blend mode" field.
	 * An earlier session (2026-07-19) spent considerable effort trying
	 * to find the "correct" non-zero value here via Ghidra tracing and
	 * live hardware register sweeps (finding 9/10/14 "enable" visible
	 * per-pixel blending but with wrong hues on every candidate
	 * rgb_order/yuv_order/format value tried) before finally probing
	 * REAL STOCK FIRMWARE directly (msn_autocopy telnetd payload,
	 * `devmem` while stock correctly renders a blended UI) and finding
	 * stock's own LIVE register value is MODE_LCD_REG0=0x03000204 --
	 * blend_mode=0, the exact value this driver already produces via
	 * the flat literal below. OSD1_CTL also matched byte-for-byte
	 * (0x260ff on both). This proves the LCDC hardware register
	 * configuration was never the bug -- our original, unmodified
	 * register state already matched stock's working configuration
	 * exactly. The real difference is almost certainly at the pixel-
	 * data level: stock uses QWS_DISPLAY=directfb (software
	 * compositing, writes fully pre-blended opaque pixels to the
	 * framebuffer, never exercises real hardware alpha blending at
	 * all), while this build uses QWS_DISPLAY=linuxfb (switched away
	 * from directfb specifically to avoid a GPU/galcore crash class --
	 * see firmware_overlay/prado/README.md) -- Qt's LinuxFB path was
	 * writing genuinely semi-transparent pixel data, confirmed via
	 * decompiling QLinuxFbScreen::setPixelFormat() in libQtGui.so.4.7.4:
	 * declaring a transp field here made Qt select QImage::Format_ARGB32
	 * (straight, non-premultiplied alpha -- Qt4 QWS never offers
	 * Format_ARGB32_Premultiplied for a LinuxFB screen), and Qt's raster
	 * compositor's fast paths assume premultiplied alpha, producing
	 * skewed colors on every translucent pixel while leaving opaque
	 * pixels untouched. Fixed in ark1668_lcdfb_check_var() (this file)
	 * by no longer declaring transp for 32bpp, forcing Qt to select
	 * Format_RGB32 instead so it flattens alpha in software before
	 * writing to /dev/fb0 -- see the comment there for the full
	 * decompile evidence. See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
	 * section 1b for the full investigation history. Do NOT re-attempt
	 * a non-zero blend_mode fix without new evidence -- that axis was
	 * already tried exhaustively and ruled out. */
	/* ark1668 fix (2026-07-25): fields are 3 bits wide (mask 0x7), not 4
	 * (0xf) -- confirmed against genuine U-Boot source
	 * (board/arkmicro/ark1668_limcet_p305/ark1668_lcd.c's real
	 * ark_set_video_priority()/ark_set_win1_priority()/etc, which use
	 * the identical shift positions 0/8/16/24 but a 0x7 mask) and
	 * cross-checked against this driver's own runtime
	 * ARKFB_SET_WINDOW_PRIORITY ioctl handlers
	 * (ark1668_lcdc_funcs.c's ark1668_lcdc_set_video_priority() etc,
	 * which already correctly use 0x7). This path only fires if a
	 * "lcd-priority" DTS property is ever added -- none currently
	 * exists for this board, so this was a dormant bug, not yet
	 * observed live. See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
	 * section 61. */
	if (!of_property_read_u32_array(dev->of_node, "lcd-priority", prio, ARRAY_SIZE(prio))) {
		u32 val = (prio[0] & 0x7) | ((prio[1] & 0x7) << 8) | ((prio[2] & 0x7) << 16)
					| ((prio[3] & 0x7) << 24);
		lcdc_writel(sinfo, ARK1668_LCDC_MODE_LCD_REG0, val);
		lcdc_writel(sinfo, ARK1668_LCDC_MODE_LCD_REG1, 0x00003000 | (prio[4] & 0x7));
	}
	else {
		lcdc_writel(sinfo, ARK1668_LCDC_MODE_LCD_REG0, 0x03000204);
		lcdc_writel(sinfo, ARK1668_LCDC_MODE_LCD_REG1, 0x00003001);
	}

	return 0;
}

/* LCD RGB888 pin-share reclaim (2026-07-26): r0/r1 (pins 2/3) and r7
 * (pin 9) are the same physical pads as i2c-gpio-0's SCL/SDA (RN6752)
 * and i2c-gpio-1's SDA (BD37033) -- see docs/DISPLAY_SUBSYSTEM.md's
 * I2C_GPIO0_LCD_PIN_CONFLICT section and
 * docs/LCD_PIN_CONFLICT_TEST_PROCEDURE.md. Confirmed on real hardware
 * (tools/pin-force/): at boot, these pins end up PERMANENTLY stuck in
 * GPIO input mode -- i2c-gpio's generic driver claims the pinmux once,
 * at gpio_request() time (pinctrl-ark.c's ark_gpio_request_enable()),
 * and never gives it back (ark_gpio_disable_free() is an empty
 * function). This driver's own pinctrl-0 (lcd-base-0 + lcd-rgb-0) gets
 * applied automatically by the driver core just before probe() runs,
 * but if i2c-gpio-0/-1 probe afterward, they silently win the pins for
 * the rest of the boot. r7 stuck-at-(whatever BD37033's idle SDA level
 * is) explains the dramatic, value-dependent LCDTest color corruption
 * (confirmed: manually re-selecting pinctrl-0 at runtime via
 * tools/pin-force made the screen match stock exactly, immediately,
 * with no further drift -- i2c-gpio never re-steals the pin after its
 * one-time claim, since direction_output()/gpio_set_value() only touch
 * the GPIO direction/data registers, not the pinmux function-select
 * bits). r0/r1 are stuck the same way but too visually minor to
 * matter on their own (2 LSBs of the R channel, max +/-3 of 255).
 *
 * Fix, take 1 (didn't work): re-select the same DT-specified
 * pinctrl-0 default state via devm_pinctrl_get_select_default(), a few
 * seconds after our own probe. Confirmed on hardware this does NOT
 * stick -- pin-force still reads GPIO afterward. Root cause: the
 * generic pinctrl core tracks "currently selected state" in software
 * (struct pinctrl_state *state on the pinctrl handle) and
 * pinctrl_select_state() short-circuits as a no-op when the requested
 * state matches what it already believes is selected. i2c-gpio's theft
 * happens via a completely different, lower-level path
 * (pinmux_ops.gpio_request_enable() in pinctrl-ark.c, called from
 * gpio_request() -- never touches pinctrl_select_state()'s tracking at
 * all), so the core's belief ("LCD default is still selected") goes
 * out of sync with the real hardware register the moment i2c-gpio
 * steals the pin -- and our "re-select the same state" call gets
 * silently skipped because the core doesn't know anything changed.
 *
 * Fix, take 2: bypass the pinctrl subsystem's state tracking entirely
 * and directly rewrite the same physical register tools/pin-force/
 * already proved fixes this on real hardware -- pinctrl0's pad-mux
 * register for pins 2-9 (r0-r7), one 4-bit nibble per pin, LSB-first
 * (pin N -> bits [4*(N-2)+3 : 4*(N-2)]), at PINCTRL_BASE + 0x1c0 (see
 * tools/pinmux-watch/README.md for the full derivation). Forces every
 * nibble to 1 (LCD/ARK_PVAL_1, dt-bindings/pinctrl/ark-pinfunc.h) --
 * not just the 3 pins (r0/r1/r7) confirmed shared, since restoring the
 * whole group matches what pinctrl_lcd_rgb888 itself specifies and
 * costs nothing extra. Same one-shot delayed_work timing as take 1 --
 * that part was never the problem. */
#define ARK1668_LCDFB_PINCTRL_BASE 0xE4900000UL
#define ARK1668_LCDFB_PINCTRL_R_REG 0x1c0UL

static struct delayed_work g_ark1668_lcdfb_reclaim_work;

static void ark1668_lcdfb_reclaim_pinctrl(struct work_struct *work)
{
	void __iomem *base;
	u32 val;

	base = ioremap(ARK1668_LCDFB_PINCTRL_BASE, 0x200);
	if (!base) {
		pr_err("pinctrl reclaim: ioremap failed\n");
		return;
	}

	/* force every pin-2..9 (r0-r7) nibble to 0x1 = LCD/ARK_PVAL_1 */
	val = 0x11111111U;
	writel(val, base + ARK1668_LCDFB_PINCTRL_R_REG);

	pr_info("LCD RGB888 r0-r7 pinmux reclaimed from any i2c-gpio pin theft (now 0x%08x)\n",
		readl(base + ARK1668_LCDFB_PINCTRL_R_REG));

	iounmap(base);
}

static int ark1668_lcdfb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fb_info *info;
	struct ark1668_lcdfb_info *sinfo;
	struct ark1668_lcdfb_pdata *pdata = NULL;
	struct resource *regs = NULL;
	struct resource *map = NULL;
	struct fb_modelist *modelist;
	int ret;

	dev_dbg(dev, "%s BEGIN\n", __func__);

	info = framebuffer_alloc(sizeof(struct ark1668_lcdfb_info), dev);
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

	ret = ark1668_lcdfb_of_init(sinfo);
	if (ret)
		goto free_info;

	info->flags = ARK1668_LCDFB_FBINFO_DEFAULT;
	info->fbops = &ark1668_lcdfb_ops;

	info->fix = ark1668_lcdfb_fix;
	strcpy(info->fix.id, sinfo->pdev->name);

	/* Enable LCDC Clocks */
	sinfo->lcdc_clk = clk_get(dev, "lcdc_clk");
	if (IS_ERR(sinfo->lcdc_clk)) {
		ret = PTR_ERR(sinfo->lcdc_clk);
		goto free_info;
	}
	ark1668_lcdfb_start_clock(sinfo);

	modelist = list_first_entry(&info->modelist,
			struct fb_modelist, list);
	fb_videomode_to_var(&info->var, &modelist->mode);

	/* Set pixel clock */
	clk_set_rate(sinfo->lcdc_clk, PICOS2KHZ(info->var.pixclock) * 1000);
	info->var.pixclock = KHZ2PICOS(clk_get_rate(sinfo->lcdc_clk) / 1000);

	ark1668_lcdfb_check_var(&info->var, info);

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

		/*
		 * CORRECTED 2026-08-06: the previous version of this comment
		 * claimed U-Boot draws the bootlogo "to the start of this
		 * same reserved region" and skipped zeroing the first screen
		 * on that basis. That's wrong -- this resource is
		 * 0xf000000 (see ark1668.dtsi's lcd@e0500000 second `reg`
		 * entry), but U-Boot's real bootlogo buffer is a completely
		 * different physical address, BOOTLOGO_SD_ADDR = 0x0b400000
		 * (board/arkmicro/ark1668_limcet_p305/ark1668_display_cfg.c).
		 * They were never the same buffer, so "don't clear the first
		 * screen" was leaving genuinely uninitialized DRAM visible
		 * the moment set_par() below points OSD1 at it -- the likely
		 * cause of garbage/white flashes seen between kernel boot
		 * and MsnCoreApp's first real frame.
		 *
		 * U-Boot can't be changed to draw directly into this
		 * region instead: it already tried addresses up here once
		 * (0xfc00000) and hit real memory corruption, because
		 * U-Boot's own allocator/relocation only trusts up to its
		 * declared 64MB DRAM (CONFIG_SYS_SDRAM_SIZE) -- see
		 * BOOTLOGO_SD_ADDR's own comment. The kernel has no such
		 * restriction, so instead we reach out and copy the real
		 * bootlogo pixels from U-Boot's buffer into this one before
		 * OSD1 ever gets pointed here -- same visible image, no
		 * flash, and no dependence on it happening to already be
		 * "left over" in DRAM.
		 *
		 * This resource is the FULL reserved carve-out
		 * (info->fix.smem_len, 16MB on this board), while check_var()
		 * below sets up triple-buffering (yres_virtual = yres*3)
		 * using ark1668_lcdfb_pan_display's yoffset to select between
		 * 3 stacked pages within it. Everything past the first
		 * screen (pages 2/3, plus OSD2/OSD3/VIDEO1/VIDEO2 space) is
		 * still zeroed -- see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
		 * section 17 for why that part still matters.
		 */
		{
			size_t screen_bytes = info->var.xres * info->var.yres * 4;
			void __iomem *uboot_logo = ioremap_wc(ARK1668_BOOTLOGO_PHYS_ADDR,
							       screen_bytes);

			if (uboot_logo) {
				memcpy_fromio(info->screen_base, uboot_logo, screen_bytes);
				iounmap(uboot_logo);
			} else {
				dev_warn(dev, "couldn't map U-Boot bootlogo buffer at 0x%x, "
					 "first frame will be black instead of the splash\n",
					 ARK1668_BOOTLOGO_PHYS_ADDR);
				memset(info->screen_base, 0, screen_bytes);
			}
			memset(info->screen_base + screen_bytes, 0,
			       info->fix.smem_len - screen_bytes);
		}
	} else {
		if(map && !map->start) {
			sinfo->smem_len = resource_size(map);
		}
		/* allocate memory buffer */
		ret = ark1668_lcdfb_alloc_video_memory(sinfo);
		if (ret < 0) {
			dev_err(dev, "cannot allocate framebuffer: %d\n", ret);
			goto stop_clk;
		}
	}

	/* Initialize PWM for contrast or backlight ("off") */
	if (pdata->lcdcon_is_backlight)
		init_backlight(sinfo);

	/* interrupt */
	ret = request_irq(sinfo->irq_base, ark1668_lcdfb_interrupt, 0, pdev->name, info);
	if (ret) {
		dev_err(dev, "request_irq failed: %d\n", ret);
		goto unmap_sysreg;
	}

	/* Some operations on the LCDC might sleep and
	 * require a preemptible task context */
	INIT_WORK(&sinfo->task, ark1668_lcdfb_task);

	init_waitqueue_head(&sinfo->vsync_waitq);

	ret = ark1668_lcdfb_init_fbinfo(sinfo);
	if (ret < 0) {
		dev_err(dev, "init fbinfo failed: %d\n", ret);
		goto unregister_irqs;
	}

	ark1668_lcdc_funcs_init(sinfo);

	ret = ark1668_lcdc_dev_init(sinfo);
	if (ret < 0) {
		dev_err(dev, "init lcdc dev failed: %d\n", ret);
		goto unregister_irqs;
	}

	ret = ark1668_lcdfb_set_par(info);
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

	dev_info(dev, "fb%d: ARK1668 LCDC at 0x%08lx (mapped at %p), irq %d\n",
		       info->node, info->fix.mmio_start, sinfo->mmio, sinfo->irq_base);

#if 1////tmp test 
{
        int i;
        struct fb_info *info_tmp;
        for(i = 1; i < 5; i++){
                info_tmp = framebuffer_alloc(sizeof(struct fb_info), dev);
                if (!info_tmp) {
                        dev_err(dev, "cannot allocate memory\n");
                        ret = -ENOMEM;
                        goto out;
                }
                memcpy(info_tmp, info, sizeof(struct fb_info));
                //strcpy(info->fix.id, "ark-fb2");
                register_framebuffer(info_tmp);
        }

        sinfo->atomic_flag = 0;
        memset(&sinfo->patomic, 0, sizeof(struct ark_disp_atomic)*ARK1668_LAYER_MAX);
}
#endif

	INIT_DELAYED_WORK(&g_ark1668_lcdfb_reclaim_work, ark1668_lcdfb_reclaim_pinctrl);
	schedule_delayed_work(&g_ark1668_lcdfb_reclaim_work, msecs_to_jiffies(3000));

	return 0;

reset_drvdata:
	dev_set_drvdata(dev, NULL);
	fb_dealloc_cmap(&info->cmap);
unregister_irqs:
	cancel_work_sync(&sinfo->task);
	free_irq(sinfo->irq_base, info);
unmap_sysreg:
	iounmap(sinfo->sysreg);
unmap_mmio:
	exit_backlight(sinfo);
	iounmap(sinfo->mmio);
release_mem:
 	release_mem_region(info->fix.mmio_start, info->fix.mmio_len);
free_fb:
	if (map)
		iounmap(info->screen_base);
	else
		ark1668_lcdfb_free_video_memory(sinfo);

release_intmem:
	if (map)
		release_mem_region(info->fix.smem_start, info->fix.smem_len);
stop_clk:
	ark1668_lcdfb_stop_clock(sinfo);
	clk_put(sinfo->lcdc_clk);
free_info:
	framebuffer_release(info);
out:
	dev_dbg(dev, "%s FAILED ret=%d.\n", __func__, ret);
	return ret;
}

static int __exit ark1668_lcdfb_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fb_info *info = dev_get_drvdata(dev);
	struct ark1668_lcdfb_info *sinfo;
	struct ark1668_lcdfb_pdata *pdata;

	if (!info || !info->par)
		return 0;
	sinfo = info->par;
	pdata = &sinfo->pdata;

	cancel_work_sync(&sinfo->task);
	exit_backlight(sinfo);
	ark1668_lcdfb_power_control(sinfo, 0);
	unregister_framebuffer(info);
	ark1668_lcdfb_stop_clock(sinfo);
	clk_put(sinfo->lcdc_clk);
	fb_dealloc_cmap(&info->cmap);
	free_irq(sinfo->irq_base, info);
	iounmap(sinfo->sysreg);
	iounmap(sinfo->mmio);
 	release_mem_region(info->fix.mmio_start, info->fix.mmio_len);
	if (platform_get_resource(pdev, IORESOURCE_MEM, 1)) {
		iounmap(info->screen_base);
		release_mem_region(info->fix.smem_start, info->fix.smem_len);
	} else {
		ark1668_lcdfb_free_video_memory(sinfo);
	}

	framebuffer_release(info);

	return 0;
}

#ifdef CONFIG_PM

static int ark1668_lcdfb_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int ark1668_lcdfb_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define ark1668_lcdfb_suspend	NULL
#define ark1668_lcdfb_resume	NULL
#endif

static struct platform_driver ark1668_lcdfb_driver = {
	.probe		= ark1668_lcdfb_probe,
	.remove		= __exit_p(ark1668_lcdfb_remove),
	.suspend	= ark1668_lcdfb_suspend,
	.resume		= ark1668_lcdfb_resume,
	.driver		= {
		.name	= "ark1668-lcdfb",
		.of_match_table	= of_match_ptr(ark1668_lcdfb_dt_ids),
	},
};
//module_platform_driver(ark1668_lcdfb_driver);

static int __init ark1668_lcdfb_init(void)
{
    int ret;

    ret = platform_driver_register(&ark1668_lcdfb_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s %d: failed to register ark1668_lcdfb_driver\n",
            __FUNCTION__, __LINE__);
    }

    return ret;
}
subsys_initcall(ark1668_lcdfb_init);

MODULE_DESCRIPTION("Ark1668 LCD Controller framebuffer driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
