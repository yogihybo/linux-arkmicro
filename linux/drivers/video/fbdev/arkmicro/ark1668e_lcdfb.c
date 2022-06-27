/*
 * Arkmicro ark1668e lcd driver
 *
 * Licensed under GPLv2 or later.
 */
#define DEBUG
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
#include "ark1668e_lcdc.h"

#define BACKLIGHT_PWM_PERIOD		50000
#define BACKLIGHT_MAX_BRIGHTNESS	100

#define SYS_REG_BASE		0xe4900000
#define SYS_CLK_DELAY		0x70
#define SYS_CTL_2A			0xa8
#define SYS_PIXEL_CLK_INV_OFFSET	16
#define SYS_ANALOG_REG1		0x144
#define SYS_LVDS_CTRL_CFG	0x190
#define SYS_LVDS_CTRL_CFG1	0x194

#define SWITCH_LVDS_TO_TTL	0x80000000
#define SWITCH_TTL_TO_LVDS	0x00000000
#define BALCK_BACKCOLOR		0x000000
#define	RED_BACKCOLOR		0xFF0000

#ifdef CONFIG_VIDEO_ARK1668E_VIN
extern int ark_vin_display_int_handler(void);
#endif

struct ark1668e_lcdfb_power_ctrl_gpio {
	int gpio;
	int active_low;

	struct list_head list;
};

#define lcdc_readl(sinfo, reg)		__raw_readl((sinfo)->mmio+(reg))
#define lcdc_writel(sinfo, reg, val)	__raw_writel((val), (sinfo)->mmio+(reg))
#define lcdc_readl_sys(sinfo, reg)		__raw_readl((sinfo)->sysreg+(reg))
#define lcdc_writel_sys(_syssinfo, reg, val)	__raw_writel((val), (sinfo)->sysreg+(reg))

#define	ARK1668E_LCDFB_FBINFO_DEFAULT	(FBINFO_DEFAULT \
					 | FBINFO_HWACCEL_NONE)

static int ark1668e_set_backlight(struct ark1668e_lcdfb_pdata *pdata,
								 int brightness)
{
	int duty = brightness * BACKLIGHT_PWM_PERIOD / BACKLIGHT_MAX_BRIGHTNESS;

	pwm_enable(pdata->pwm);
	pwm_config(pdata->pwm, duty, BACKLIGHT_PWM_PERIOD);
	pdata->backlight_value = brightness;

	return 0;
}

/* some bl->props field just changed */
static int ark1668e_bl_update_status(struct backlight_device *bl)
{
	struct ark1668e_lcdfb_info *sinfo = bl_get_data(bl);
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;
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

	ark1668e_set_backlight(pdata, brightness);

	bl->props.fb_blank = bl->props.power = sinfo->bl_power = power;

	return 0;
}

static int ark1668e_bl_get_brightness(struct backlight_device *bl)
{
	struct ark1668e_lcdfb_info *sinfo = bl_get_data(bl);
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;

	return pdata->backlight_value;
}

static const struct backlight_ops ark1668e_lcdc_bl_ops = {
	.update_status = ark1668e_bl_update_status,
	.get_brightness = ark1668e_bl_get_brightness,
};

static void init_backlight(struct ark1668e_lcdfb_info *sinfo)
{
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;
	struct backlight_properties props;
	struct backlight_device	*bl;

	sinfo->bl_power = FB_BLANK_UNBLANK;

	if (sinfo->backlight)
		return;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = BACKLIGHT_MAX_BRIGHTNESS;
	bl = backlight_device_register("backlight", &sinfo->pdev->dev, sinfo,
				       &ark1668e_lcdc_bl_ops, &props);
	if (IS_ERR(bl)) {
		dev_err(&sinfo->pdev->dev, "error %ld on backlight register\n",
				PTR_ERR(bl));
		return;
	}
	sinfo->backlight = bl;

	bl->props.power = FB_BLANK_UNBLANK;
	bl->props.fb_blank = FB_BLANK_UNBLANK;
	bl->props.brightness = ark1668e_bl_get_brightness(bl);

	ark1668e_set_backlight(pdata, pdata->backlight_value);
}

static void exit_backlight(struct ark1668e_lcdfb_info *sinfo)
{
	if (!sinfo->backlight)
		return;

	if (sinfo->backlight->ops) {
		sinfo->backlight->props.power = FB_BLANK_POWERDOWN;
		sinfo->backlight->ops->update_status(sinfo->backlight);
	}
	backlight_device_unregister(sinfo->backlight);
}

static inline void ark1668e_lcdfb_power_control(struct ark1668e_lcdfb_info *sinfo, int on)
{
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;

	if (pdata->ark1668e_lcdfb_power_control)
		pdata->ark1668e_lcdfb_power_control(pdata, on);
}

static const struct fb_fix_screeninfo ark1668e_lcdfb_fix __initconst = {
	.type		= FB_TYPE_PACKED_PIXELS,
	.visual		= FB_VISUAL_TRUECOLOR,
	.xpanstep	= 0,
	.ypanstep	= 1,
	.ywrapstep	= 0,
	.accel		= FB_ACCEL_NONE,
};

/* static void ark1668e_lcdfb_stop(struct ark1668e_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ARK1668E_LCDC_EANBLE, 0);
} */

static void ark1668e_lcdfb_start(struct ark1668e_lcdfb_info *sinfo)
{
	lcdc_writel(sinfo, ARK1668E_LCDC_EANBLE, 1);
}

static inline void ark1668e_lcdfb_free_video_memory(struct ark1668e_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;

	dma_free_wc(info->device, info->fix.smem_len, info->screen_base,
		    info->fix.smem_start);
}

/**
 *	ark1668e_lcdfb_alloc_video_memory - Allocate framebuffer memory
 *	@sinfo: the frame buffer to allocate memory for
 *
 * 	This function is called only from the ark1668e_lcdfb_probe()
 * 	so no locking by fb_info->mm_lock around smem_len setting is needed.
 */
static int ark1668e_lcdfb_alloc_video_memory(struct ark1668e_lcdfb_info *sinfo)
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

static const struct fb_videomode *ark1668e_lcdfb_choose_mode(struct fb_var_screeninfo *var,
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
 *      ark1668e_lcdfb_check_var - Validates a var passed in.
 *      @var: frame buffer variable screen structure
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Checks to see if the hardware supports the state requested by
 *	var passed in. This function does not alter the hardware
 *	state!!!  This means the data stored in struct fb_info and
 *	struct ark1668e_lcdfb_info do not change. This includes the var
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
static int ark1668e_lcdfb_check_var(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	struct device *dev = info->device;
	struct ark1668e_lcdfb_info *sinfo = info->par;
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;

	dev_dbg(dev, "%s:\n", __func__);

	if (!(var->pixclock && var->bits_per_pixel)) {
		/* choose a suitable mode if possible */
		if (!ark1668e_lcdfb_choose_mode(var, info)) {
			dev_err(dev, "needed value not specified\n");
			return -EINVAL;
		}
	}

	dev_dbg(dev, "  resolution: %ux%u\n", var->xres, var->yres);
	dev_dbg(dev, "  pixclk:     %lu KHz\n", PICOS2KHZ(var->pixclock));
	dev_dbg(dev, "  bpp:        %u\n", var->bits_per_pixel);

	/* Do not allow to have real resoulution larger than virtual */
	if (var->xres >= var->xres_virtual)
		var->xres_virtual = var->xres;

	if (var->yres >= var->yres_virtual) {
		if (pdata->fb_buffer_nums > 0)
			var->yres_virtual = var->yres * pdata->fb_buffer_nums;
		else
			var->yres_virtual = var->yres * 3;
	}

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
	var->vsync_len = min_t(u32, var->vsync_len,
			(ARK1668E_LCDC_VPW >> ARK1668E_LCDC_VPW_OFFSET) + 1);
	var->upper_margin = min_t(u32, var->upper_margin,
			ARK1668E_LCDC_VBP);
	var->lower_margin = min_t(u32, var->lower_margin,
			ARK1668E_LCDC_VFP >> ARK1668E_LCDC_VFP_OFFSET);
	var->right_margin = min_t(u32, var->right_margin,
			ARK1668E_LCDC_HFP + 1);
	var->hsync_len = min_t(u32, var->hsync_len,
			(ARK1668E_LCDC_HPW >> ARK1668E_LCDC_HPW_OFFSET) + 1);
	var->left_margin = min_t(u32, var->left_margin,
			(ARK1668E_LCDC_HBP >> ARK1668E_LCDC_HBP_OFFSET) + 1);

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
/* static void ark1668e_lcdfb_reset(struct ark1668e_lcdfb_info *sinfo)
{
	might_sleep();

	ark1668e_lcdfb_stop(sinfo);
	ark1668e_lcdfb_start(sinfo);
} */

static int ark1668e_lcdfb_pan_display(struct fb_var_screeninfo *var,
			       struct fb_info *info)
{
	struct ark1668e_lcdfb_info *sinfo = info->par;
	struct fb_fix_screeninfo *fix = &info->fix;
	unsigned long addr;

	addr = fix->smem_start + var->yoffset * fix->line_length
		+ var->xoffset * info->var.bits_per_pixel / 8;

	sinfo->render_addr[ARK1668E_LCDC_LAYER_OSD2].yaddr = addr;

	//printk(KERN_ALERT "ark1668e_lcdfb_pan_display 0x%x.\n", addr);

	return 0;
}

/**
 *      ark1668e_lcdfb_set_par - Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 *	Using the fb_var_screeninfo in fb_info we set the resolution
 *	of the this particular framebuffer. This function alters the
 *	par AND the fb_fix_screeninfo stored in fb_info. It doesn't
 *	not alter var in fb_info since we are using that data. This
 *	means we depend on the data in var inside fb_info to be
 *	supported by the hardware.  ark1668e_lcdfb_check_var is always called
 *	before ark1668e_lcdfb_set_par to ensure this.  Again if you can't
 *	change the resolution you don't need this function.
 *
 */
static int ark1668e_lcdfb_set_par(struct fb_info *info)
{
	//struct ark1668e_lcdfb_info *sinfo = info->par;
	//struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;

	might_sleep();

	dev_dbg(info->device, "%s:\n", __func__);

	dev_dbg(info->device, "  * DONE\n");

	return 0;
}

/* static int ark1668e_lcdfb_blank(int blank_mode, struct fb_info *info)
{
	struct ark1668e_lcdfb_info *sinfo = info->par;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
		ark1668e_lcdfb_start(sinfo);
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		break;
	case FB_BLANK_POWERDOWN:
		ark1668e_lcdfb_stop(sinfo);
		break;
	default:
		return -EINVAL;
	}

	return ((blank_mode == FB_BLANK_NORMAL) ? 1 : 0);
} */

static struct fb_ops ark1668e_lcdfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= ark1668e_lcdfb_check_var,
	.fb_set_par	= ark1668e_lcdfb_set_par,
	//.fb_blank	= ark1668e_lcdfb_blank,
	.fb_pan_display	= ark1668e_lcdfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_ioctl       = ark1668e_lcdfb_ioctl,
};

static irqreturn_t ark1668e_lcdfb_interrupt(int irq, void *dev_id)
{
	struct fb_info *info = dev_id;
	struct ark1668e_lcdfb_info *sinfo = info->par;
	u32 status;
	int i;

	status = lcdc_readl(sinfo, ARK1668E_LCDC_INTERRUPT_STATUS);

	/* clear intr */
	lcdc_writel(sinfo, ARK1668E_LCDC_INTERRUPT_STATUS, 0);

	if (status & ARK1668E_LCDC_INT_LCD_FRAME) {
		for (i = 0; i < ARK1668E_LCDC_LAYER_MAX; i++) {
			if (sinfo->render_addr[i].yaddr) {
				if((i >= ARK1668E_LCDC_LAYER_OSD1) && (i <= ARK1668E_LCDC_LAYER_OSD3))
					ark1668e_lcdc_set_osd_addr(i, sinfo->render_addr[i].yaddr);
				else
					ark1668e_lcdc_set_video_addr(i, sinfo->render_addr[i].yaddr,
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
static void ark1668e_lcdfb_task(struct work_struct *work)
{
	/* struct ark1668e_lcdfb_info *sinfo =
		container_of(work, struct ark1668e_lcdfb_info, task); */
#ifdef CONFIG_VIDEO_ARK1668E_VIN
		ark_vin_display_int_handler();
#endif
}

static int __init ark1668e_lcdfb_init_fbinfo(struct ark1668e_lcdfb_info *sinfo)
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

/* static void ark1668e_lcdfb_start_clock(struct ark1668e_lcdfb_info *sinfo)
{
	clk_prepare_enable(sinfo->lcdc_clk);
}

static void ark1668e_lcdfb_stop_clock(struct ark1668e_lcdfb_info *sinfo)
{
	clk_disable_unprepare(sinfo->lcdc_clk);
} */

static const struct of_device_id ark1668e_lcdfb_dt_ids[] = {
	{ .compatible = "arkmicro,ark1668e-lcdc",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, ark1668e_lcdfb_dt_ids);

static const char *ark1668e_lcdfb_interface_types[] = {
	[ARK1668E_LCDC_INTERFACE_TTL]	= "TTL",
	[ARK1668E_LCDC_INTERFACE_LVDS]	= "LVDS",
	[ARK1668E_LCDC_INTERFACE_DUAL_LVDS]	= "DLVDS",
};

static int ark1668e_lcdfb_get_of_interface_types(struct device_node *np)
{
	const char *type;
	int err, i;

	err = of_property_read_string(np, "interface-type", &type);
	if (err < 0)
		return ARK1668E_LCDC_INTERFACE_TTL;
	
	for (i = 0; i < ARRAY_SIZE(ark1668e_lcdfb_interface_types); i++)
		if (!strcasecmp(type, ark1668e_lcdfb_interface_types[i]))
			return i;

	return -ENODEV;
}

static const char *ark1668e_lcdfb_wiring_modes[] = {
	[ARK1668E_LCDC_WIRING_BGR]	= "BGR",
	[ARK1668E_LCDC_WIRING_GBR]	= "GBR",
	[ARK1668E_LCDC_WIRING_RBG]	= "RBG",
	[ARK1668E_LCDC_WIRING_BRG]	= "BRG",
	[ARK1668E_LCDC_WIRING_GRB]	= "GRB",
	[ARK1668E_LCDC_WIRING_RGB]	= "RGB",
};

static int ark1668e_lcdfb_get_of_wiring_modes(struct device_node *np)
{
	const char *mode;
	int err, i;

	err = of_property_read_string(np, "lcd-wiring-mode", &mode);
	if (err < 0)
		return ARK1668E_LCDC_WIRING_BGR;

	for (i = 0; i < ARRAY_SIZE(ark1668e_lcdfb_wiring_modes); i++)
		if (!strcasecmp(mode, ark1668e_lcdfb_wiring_modes[i]))
			return i;

	return -ENODEV;
}

static void ark1668e_lcdfb_power_control_gpio(struct ark1668e_lcdfb_pdata *pdata, int on)
{
	struct ark1668e_lcdfb_power_ctrl_gpio *og;

	list_for_each_entry(og, &pdata->pwr_gpios, list)
		gpio_set_value(og->gpio, on ? !og->active_low : og->active_low);
}

/*
 * Timer function for delayed backlight power up/down
 */
static void ark1668e_backlight_timer_func(struct timer_list *t)
{
	struct ark1668e_lcdfb_info *sinfo = from_timer(sinfo, t, pdata.backlight_timer);
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;

	ark1668e_lcdfb_power_control(sinfo, 1);
	//del_timer_sync(&pdata->backlight_timer);
	del_timer(&pdata->backlight_timer);
}

static int ark1668e_lcdfb_of_init(struct ark1668e_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;
	struct fb_var_screeninfo *var = &info->var;
	struct device *dev = &sinfo->pdev->dev;
	struct device_node *np =dev->of_node;
	struct device_node *display_np;
	struct device_node *timings_np;
	struct display_timings *timings;
	enum of_gpio_flags flags;
	struct ark1668e_lcdfb_power_ctrl_gpio *og;
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

	ret = of_property_read_u32(display_np, "fb-buffer-nums", &pdata->fb_buffer_nums);

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
		pdata->ark1668e_lcdfb_power_control = ark1668e_lcdfb_power_control_gpio;
	ret = ark1668e_lcdfb_get_of_interface_types(display_np);
	if (ret < 0) {
		dev_err(dev, "invalid interface-type\n");
		goto put_display_node;
	}
	pdata->interface_type = ret;
	if (pdata->interface_type == ARK1668E_LCDC_INTERFACE_LVDS) {
		ret = of_property_read_u32(display_np, "lvds-con", &pdata->lvds_con);
		if (ret < 0) {
			dev_err(dev, "failed to get property lvds-con\n");
			goto put_display_node;
		}

		ret = of_property_read_u32(display_np, "lvds-con2", &pdata->lvds_con2);
		if (ret < 0) {
			dev_err(dev, "failed to get property lvds-con2\n");
			goto put_display_node;
		}
	} else if (pdata->interface_type == ARK1668E_LCDC_INTERFACE_DUAL_LVDS) {
		ret = of_property_read_u32(display_np, "lvds-con", &pdata->lvds_con);
		if (ret < 0) {
			dev_err(dev, "failed to get property lvds-con\n");
			goto put_display_node;
		}

		ret = of_property_read_u32(display_np, "lvds-con2", &pdata->lvds_con2);
		if (ret < 0) {
			dev_err(dev, "failed to get property lvds-con2\n");
			goto put_display_node;
		}
	}

	ret = ark1668e_lcdfb_get_of_wiring_modes(display_np);
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

		timer_setup(&pdata->backlight_timer, ark1668e_backlight_timer_func, 0);

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

static int ark1668e_lcdc_dev_init(struct fb_info *info)
{
	struct ark1668e_lcdfb_info *sinfo = info->par;
	struct ark1668e_lcdfb_pdata *pdata = &sinfo->pdata;
	unsigned long value;
	unsigned long bits_per_line;

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

	/* set lcd back color */
	lcdc_writel(sinfo, ARK1668E_LCDC_BACK_COLOR, BALCK_BACKCOLOR);

	/* Now, the LCDC core... */
	value = lcdc_readl_sys(sinfo, SYS_CLK_DELAY);
	if (pdata->pixelclk_active_high)
		value |= (1 << SYS_PIXEL_CLK_INV_OFFSET);
	else
		value &= ~(1 << SYS_PIXEL_CLK_INV_OFFSET);
	lcdc_writel_sys(sinfo, SYS_CLK_DELAY, value);

	/* Initialize control register */
	value = (6 << 23) | (1 << 0);
	/* set interrupt at the start of the front porch when vfp is not zero */
	if (info->var.lower_margin)
		value |= (3 << 21);
	value |= pdata->lcd_wiring_mode << 18;
	lcdc_writel(sinfo, ARK1668E_LCDC_CONTROL, value);

	/* timing */
	value = (info->var.hsync_len - 1) << ARK1668E_LCDC_HPW_OFFSET;
	value |= (info->var.left_margin - 1) << ARK1668E_LCDC_HBP_OFFSET;
	value |= (info->var.right_margin - 1);
	lcdc_writel(sinfo, ARK1668E_LCDC_TIMING0, value);

	value = info->var.lower_margin << ARK1668E_LCDC_VFP_OFFSET;
	value |= (info->var.vsync_len - 1) << ARK1668E_LCDC_VPW_OFFSET;
	value |= (info->var.xres - 1);
	lcdc_writel(sinfo, ARK1668E_LCDC_TIMING1, value);

	value = pdata->de_active_high << ARK1668E_LCDC_IOE_OFFSET;
	value |= !!(info->var.sync & FB_SYNC_HOR_HIGH_ACT) << ARK1668E_LCDC_IHS_OFFSET;
	value |= !!(info->var.sync & FB_SYNC_VERT_HIGH_ACT) << ARK1668E_LCDC_IVS_OFFSET;
	value |= (info->var.yres - 1) << ARK1668E_LCDC_LPS_OFFSET;
	value |= info->var.upper_margin;
	lcdc_writel(sinfo, ARK1668E_LCDC_TIMING2, value);

	/* Initialize specific screen type */
	if (pdata->interface_type == ARK1668E_LCDC_INTERFACE_LVDS) {
		lcdc_writel_sys(sinfo, SYS_CTL_2A, SWITCH_TTL_TO_LVDS);
		/* value = lcdc_readl_sys(sinfo, SYS_ANALOG_REG1);
		value |= (1 << 26);
		lcdc_writel_sys(sinfo, SYS_ANALOG_REG1, value);

		lcdc_writel_sys(sinfo, SYS_LVDS_CTRL_CFG, pdata->lvds_con); */
		lcdc_writel_sys(sinfo, SYS_LVDS_CTRL_CFG, pdata->lvds_con);
		lcdc_writel_sys(sinfo, SYS_LVDS_CTRL_CFG1, pdata->lvds_con2);
	} else if (pdata->interface_type == ARK1668E_LCDC_INTERFACE_DUAL_LVDS) {
		lcdc_writel_sys(sinfo, SYS_CTL_2A, SWITCH_TTL_TO_LVDS);
		lcdc_writel_sys(sinfo, SYS_LVDS_CTRL_CFG, pdata->lvds_con);
		lcdc_writel_sys(sinfo, SYS_LVDS_CTRL_CFG1, pdata->lvds_con2);
	}
	else if(pdata->interface_type == ARK1668E_LCDC_INTERFACE_TTL)
	{
		lcdc_writel_sys(sinfo, SYS_CTL_2A, SWITCH_LVDS_TO_TTL);
	}

	/* sync always on */
	lcdc_writel(sinfo, ARK1668E_LCDC_PARAMTERS_SYNC_SWITCH, 0x7f);

	/* Display osd layer1(fb0) size,pos,format,addr... */
	ark1668e_lcdfb_pan_display(&info->var, info);
	value = (info->var.yres << ARK1668E_LCDC_HEIGHT_OFFSET) | info->var.xres;
	lcdc_writel(sinfo, ARK1668E_LCDC_OSD2_SIZE, value);
	lcdc_writel(sinfo, ARK1668E_LCDC_OSD2_SOURCE_SIZE, value);
	lcdc_writel(sinfo, ARK1668E_LCDC_OSD2_POSITION, 0);
	lcdc_writel(sinfo, ARK1668E_LCDC_OSD2_WIN_POINT, 0);
	value = (1 << 17) | (ARK1668E_LCDC_FORMAT_RGBA888 << 12) | 0xff;
	lcdc_writel(sinfo, ARK1668E_LCDC_OSD2_CTL, value);

	/* open osd layer1 */
	value = lcdc_readl(sinfo, ARK1668E_LCDC_CONTROL);
	value |= (1 << ARK1668E_LCDC_OSD2_EN_OFFSET);
	lcdc_writel(sinfo, ARK1668E_LCDC_CONTROL, value);

	/* Clear all interrupts */
	lcdc_writel(sinfo, ARK1668E_LCDC_INTERRUPT_STATUS, 0);

	/* Enable frame interrupt */
	lcdc_writel(sinfo, ARK1668E_LCDC_INTERRUPT_CTL, ARK1668E_LCDC_INT_LCD_FRAME);

	/* set layer1(fb0) vp */


	/* set layer priority and blend mode */
	lcdc_writel(sinfo, ARK1668E_LCDC_BLD_MODE_LCD_REG0, 0x04030200);
	lcdc_writel(sinfo, ARK1668E_LCDC_BLD_MODE_LCD_REG1, 0x0003f001);

	ark1668e_lcdfb_start(sinfo);

	return 0;
}

static int ark1668e_lcdfb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fb_info *info;
	struct ark1668e_lcdfb_info *sinfo;
	struct ark1668e_lcdfb_pdata *pdata = NULL;
	struct resource *regs = NULL;
	struct resource *map = NULL;
	struct fb_modelist *modelist;
	int ret;

	dev_dbg(dev, "%s BEGIN\n", __func__);

	info = framebuffer_alloc(sizeof(struct ark1668e_lcdfb_info), dev);
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

	ret = ark1668e_lcdfb_of_init(sinfo);
	if (ret)
		goto free_info;

	info->flags = ARK1668E_LCDFB_FBINFO_DEFAULT;
	info->fbops = &ark1668e_lcdfb_ops;

	info->fix = ark1668e_lcdfb_fix;
	strcpy(info->fix.id, sinfo->pdev->name);

	/* Enable LCDC Clocks */
	sinfo->lcdc_clk = clk_get(dev, "lcdc_clk");
	if (IS_ERR(sinfo->lcdc_clk)) {
		ret = PTR_ERR(sinfo->lcdc_clk);
		goto free_info;
	}
	/* ark1668e_lcdfb_start_clock(sinfo); */

	modelist = list_first_entry(&info->modelist,
			struct fb_modelist, list);
	fb_videomode_to_var(&info->var, &modelist->mode);

	/* Set pixel clock */
	clk_set_rate(sinfo->lcdc_clk, PICOS2KHZ(info->var.pixclock) * 1000);
	info->var.pixclock = KHZ2PICOS(clk_get_rate(sinfo->lcdc_clk) / 1000);

	ark1668e_lcdfb_check_var(&info->var, info);

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
        memset(info->screen_base, 0, info->var.xres * info->var.yres * 4);

		/*
		 * Don't clear the framebuffer -- someone may have set
		 * up a splash image.
		 */
	} else {
		if(map && !map->start) {
			sinfo->smem_len = resource_size(map);
		}
		/* allocate memory buffer */
		ret = ark1668e_lcdfb_alloc_video_memory(sinfo);
		if (ret < 0) {
			dev_err(dev, "cannot allocate framebuffer: %d\n", ret);
			goto stop_clk;
		}
	}

	/* Initialize PWM for contrast or backlight ("off") */
	if (pdata->lcdcon_is_backlight)
		init_backlight(sinfo);

	/* interrupt */
	ret = request_irq(sinfo->irq_base, ark1668e_lcdfb_interrupt, 0, pdev->name, info);
	if (ret) {
		dev_err(dev, "request_irq failed: %d\n", ret);
		goto unmap_sysreg;
	}

	/* Some operations on the LCDC might sleep and
	 * require a preemptible task context */
	INIT_WORK(&sinfo->task, ark1668e_lcdfb_task);

	init_waitqueue_head(&sinfo->vsync_waitq);

	ret = ark1668e_lcdfb_init_fbinfo(sinfo);
	if (ret < 0) {
		dev_err(dev, "init fbinfo failed: %d\n", ret);
		goto unregister_irqs;
	}

	ark1668e_lcdc_funcs_init(sinfo);

	ret = ark1668e_lcdc_dev_init(info);
	if (ret < 0) {
		dev_err(dev, "init lcdc dev failed: %d\n", ret);
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

	dev_info(dev, "fb%d: Arkmicro LCDC at 0x%08lx (mapped at %p), irq %d\n",
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
}
#endif
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
unmap_mmio:
	exit_backlight(sinfo);
	iounmap(sinfo->mmio);
release_mem:
 	release_mem_region(info->fix.mmio_start, info->fix.mmio_len);
free_fb:
	if (map)
		iounmap(info->screen_base);
	else
		ark1668e_lcdfb_free_video_memory(sinfo);

release_intmem:
	if (map)
		release_mem_region(info->fix.smem_start, info->fix.smem_len);
stop_clk:
	/* ark1668e_lcdfb_stop_clock(sinfo); */
	clk_put(sinfo->lcdc_clk);
free_info:
	framebuffer_release(info);
out:
	dev_dbg(dev, "%s FAILED ret=%d.\n", __func__, ret);
	printk(KERN_ERR "***ERR: %s failed\n", __FUNCTION__);
	return ret;
}

static int __exit ark1668e_lcdfb_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fb_info *info = dev_get_drvdata(dev);
	struct ark1668e_lcdfb_info *sinfo;
	struct ark1668e_lcdfb_pdata *pdata;

	if (!info || !info->par)
		return 0;
	sinfo = info->par;
	pdata = &sinfo->pdata;

	cancel_work_sync(&sinfo->task);
	exit_backlight(sinfo);
	ark1668e_lcdfb_power_control(sinfo, 0);
	unregister_framebuffer(info);
	/* ark1668e_lcdfb_stop_clock(sinfo); */
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
		ark1668e_lcdfb_free_video_memory(sinfo);
	}

	framebuffer_release(info);

	return 0;
}

#ifdef CONFIG_PM

static int ark1668e_lcdfb_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int ark1668e_lcdfb_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define ark1668e_lcdfb_suspend	NULL
#define ark1668e_lcdfb_resume	NULL
#endif

static struct platform_driver ark1668e_lcdfb_driver = {
	.probe		= ark1668e_lcdfb_probe,
	.remove		= __exit_p(ark1668e_lcdfb_remove),
	.suspend	= ark1668e_lcdfb_suspend,
	.resume		= ark1668e_lcdfb_resume,
	.driver		= {
		.name	= "ark1668e_lcdfb",
		.of_match_table	= of_match_ptr(ark1668e_lcdfb_dt_ids),
	},
};
//module_platform_driver(ark1668e_lcdfb_driver);

static int __init ark1668e_lcdfb_init(void)
{
    int ret;

    ret = platform_driver_register(&ark1668e_lcdfb_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s %d: failed to register ark1668e_lcdfb_driver\n",
            __FUNCTION__, __LINE__);
    }

    return ret;
}
subsys_initcall(ark1668e_lcdfb_init);

MODULE_DESCRIPTION("Ark1668 LCD Controller framebuffer driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
