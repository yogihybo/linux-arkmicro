// SPDX-License-Identifier: GPL-2.0+
/*
 */

#include <common.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#include <fdtdec.h>
#include <lcd.h>
#include <display.h>
#include <pwm.h>
#include <mach/clock.h>
#include <mach/ark1668e_lcdc_regs.h>
#include <mach/ark1668e-sysreg.h>

DECLARE_GLOBAL_DATA_PTR;

/* enum {
	LCD_MAX_WIDTH		= 1920,
	LCD_MAX_HEIGHT		= 1080,
	LCD_MAX_LOG2_BPP	= VIDEO_BPP32,
}; */

#define BACKLIGHT_PWM_PERIOD		50000
#define BACKLIGHT_MAX_BRIGHTNESS	100

#define SYS_REG_BASE		0xe4900000
#define SYS_LCD_CLK_CFG		0x54
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

enum ark1668e_interface_type {
	ARK1668E_LCDC_INTERFACE_TTL,
	ARK1668E_LCDC_INTERFACE_LVDS,
	ARK1668E_LCDC_INTERFACE_DUAL_LVDS,
};

typedef enum ark1668e_lcdc_format {
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
} ARK1668E_LCDC_FORMAT;

typedef enum ark1668e_lcdc_ycbcr_foramt {
	ARK1668E_LCDC_YCBCR_FORMAT_Y_U_V,	//Y_U_V422 or Y_U_V420
	ARK1668E_LCDC_YCBCR_FORMAT_Y_UV,	//Y_UV422 or Y_UV420
	ARK1668E_LCDC_YCBCR_FORMAT_END
} ARK1668E_LCDC_YCBCR_FORMAT;

/* Way LCD wires are connected to the chip:
 * A swapped wiring onboard can bring to RGB mode.
 */
#define ARK1668E_LCDC_WIRING_BGR	0
#define ARK1668E_LCDC_WIRING_GBR	1
#define ARK1668E_LCDC_WIRING_RBG	2
#define ARK1668E_LCDC_WIRING_BRG	3
#define ARK1668E_LCDC_WIRING_GRB	4
#define ARK1668E_LCDC_WIRING_RGB	5

struct ark_lcdc_priv {
	void __iomem *mmio;
	void __iomem *sysreg;
	unsigned int fb_addr;
	struct display_timing timing;
	struct gpio_desc bl_power_gpio;
	int bl_power;
	unsigned int bl_pwm;
	unsigned int bl_val;
	unsigned int bl_delay;
	int lcd_wiring_mode;
	int interface_type;
	unsigned int lvds_con;
	unsigned int lvds_con2;
	ulong clk_rate;
};

#define lcdc_readl(priv, reg)			readl((priv)->mmio+(reg))
#define lcdc_writel(priv, reg, val)		writel((val), (priv)->mmio+(reg))
#define lcdc_readl_sys(priv, reg)		readl((priv)->sysreg+(reg))
#define lcdc_writel_sys(priv, reg, val)	writel((val), (priv)->sysreg+(reg))

static int ark_lcdc_set_clk(struct udevice *dev)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);
	unsigned int srcclk = ark_get_lcdpll_clock(); 
	unsigned int val;
	int div;

	val = lcdc_readl_sys(priv, SYS_LCD_CLK_CFG);
	/* select lcdpll src */
	val &= ~(0x7f << 4) | (0xf << 19);
	div = DIV_ROUND_UP(srcclk, priv->timing.pixelclock.typ) & 0xf;
	val |= div << 19;
	lcdc_writel_sys(priv, SYS_LCD_CLK_CFG, val);

	printf("lcdpll %dHz, div=%d, lcdclk %dHz.\n", srcclk, div, srcclk / div);

	return 0;
}

static const char *ark1668e_lcdfb_interface_types[] = {
	[ARK1668E_LCDC_INTERFACE_TTL]	= "TTL",
	[ARK1668E_LCDC_INTERFACE_LVDS]	= "LVDS",
	[ARK1668E_LCDC_INTERFACE_DUAL_LVDS]	= "DLVDS",
};

static int ark1668e_lcdfb_get_of_interface_types(struct udevice *dev)
{
	const char *type;
	int i;

	type = dev_read_string(dev, "interface-type");
	if (!type)
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

static int ark1668e_lcdfb_get_of_wiring_modes(struct udevice *dev)
{
	const char *str;
	int i;

	str = dev_read_string(dev, "lcd-wiring-mode");
	if (!str)
		return ARK1668E_LCDC_WIRING_BGR;

	for (i = 0; i < ARRAY_SIZE(ark1668e_lcdfb_wiring_modes); i++)
		if (!strcasecmp(str, ark1668e_lcdfb_wiring_modes[i]))
			return i;

	return -ENODEV;
}

static void ark_lcdc_init(struct udevice *dev)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);
	struct display_timing *timing = &priv->timing;
	unsigned long value;

	ark_lcdc_set_clk(dev);

	/* set lcd back color */
	lcdc_writel(priv, ARK1668E_LCDC_BACK_COLOR, BALCK_BACKCOLOR);

	/* set layer1(fb0) vp */

	/* reserve the layer enable */
	value = lcdc_readl(priv, ARK1668E_LCDC_CONTROL);
	value &= 0x1f << 5;
	value |= (6 << 23) | (1 << 0);
	/* set interrupt at the start of the front porch when vfp is not zero */
	if (timing->vfront_porch.typ)
		value |= (3 << 21);
	value |= priv->lcd_wiring_mode << 18;
	lcdc_writel(priv, ARK1668E_LCDC_CONTROL, value);

	/* timing */
	value = lcdc_readl_sys(priv, SYS_CLK_DELAY);
	if (!!(timing->flags & DISPLAY_FLAGS_PIXDATA_POSEDGE))
		value |= (1 << SYS_PIXEL_CLK_INV_OFFSET);
	else
		value &= ~(1 << SYS_PIXEL_CLK_INV_OFFSET);
	lcdc_writel_sys(priv, SYS_CLK_DELAY, value);

	value = (timing->hsync_len.typ - 1) << ARK1668E_LCDC_HPW_OFFSET;
	value |= (timing->hback_porch.typ - 1) << ARK1668E_LCDC_HBP_OFFSET;
	value |= (timing->hfront_porch.typ - 1);
	lcdc_writel(priv, ARK1668E_LCDC_TIMING0, value);

	value = timing->vfront_porch.typ << ARK1668E_LCDC_VFP_OFFSET;
	value |= (timing->vsync_len.typ - 1) << ARK1668E_LCDC_VPW_OFFSET;
	value |= (timing->hactive.typ - 1);
	lcdc_writel(priv, ARK1668E_LCDC_TIMING1, value);

	value = !!(timing->flags & DISPLAY_FLAGS_DE_HIGH) << ARK1668E_LCDC_IOE_OFFSET;
	value |= !!(timing->flags & DISPLAY_FLAGS_HSYNC_HIGH) << ARK1668E_LCDC_IHS_OFFSET;
	value |= !!(timing->flags & DISPLAY_FLAGS_VSYNC_HIGH) << ARK1668E_LCDC_IVS_OFFSET;
	value |= (timing->vactive.typ - 1) << ARK1668E_LCDC_LPS_OFFSET;
	value |= timing->vback_porch.typ;
	lcdc_writel(priv, ARK1668E_LCDC_TIMING2, value);

	/* Initialize specific screen type */
	if (priv->interface_type == ARK1668E_LCDC_INTERFACE_LVDS) {
		lcdc_writel_sys(priv, SYS_CTL_2A, SWITCH_TTL_TO_LVDS);
		/* value = lcdc_readl_sys(priv, SYS_ANALOG_REG1);
		value |= (1 << 26);
		lcdc_writel_sys(priv, SYS_ANALOG_REG1, value);

		lcdc_writel_sys(priv, SYS_LVDS_CTRL_CFG, priv->lvds_con); */
	} else if (priv->interface_type == ARK1668E_LCDC_INTERFACE_DUAL_LVDS) {
		lcdc_writel_sys(priv, SYS_CTL_2A, SWITCH_TTL_TO_LVDS);
		lcdc_writel_sys(priv, SYS_LVDS_CTRL_CFG, priv->lvds_con);
		lcdc_writel_sys(priv, SYS_LVDS_CTRL_CFG1, priv->lvds_con2);
	}
	else if(priv->interface_type == ARK1668E_LCDC_INTERFACE_TTL)
	{
		lcdc_writel_sys(priv, SYS_CTL_2A, SWITCH_LVDS_TO_TTL);
	}

	/* sync always on */
	lcdc_writel(priv, ARK1668E_LCDC_PARAMTERS_SYNC_SWITCH, 0x7f);

	/* Display osd layer1(fb0) size,pos,format,addr... */
	lcdc_writel(priv, ARK1668E_LCDC_OSD2_ADDR, priv->fb_addr);
	value = (timing->vactive.typ << ARK1668E_LCDC_HEIGHT_OFFSET) | timing->hactive.typ;
	lcdc_writel(priv, ARK1668E_LCDC_OSD2_SIZE, value);
	lcdc_writel(priv, ARK1668E_LCDC_OSD2_SOURCE_SIZE, value);
	lcdc_writel(priv, ARK1668E_LCDC_OSD2_POSITION, 0);
	lcdc_writel(priv, ARK1668E_LCDC_OSD2_WIN_POINT, 0);
	value = (1 << 17) | (ARK1668E_LCDC_FORMAT_RGBA888 << 12) | 0xff;
	lcdc_writel(priv, ARK1668E_LCDC_OSD2_CTL, value);

	/* open osd layer1 */
	value = lcdc_readl(priv, ARK1668E_LCDC_CONTROL);
	value |= (1 << ARK1668E_LCDC_OSD2_EN_OFFSET);
	lcdc_writel(priv, ARK1668E_LCDC_CONTROL, value);

	/* Clear all interrupts */
	lcdc_writel(priv, ARK1668E_LCDC_INTERRUPT_STATUS, 0);

	/* Disable interrupt */
	lcdc_writel(priv, ARK1668E_LCDC_INTERRUPT_CTL, 0);

	/* set layer priority and blend mode */
	lcdc_writel(priv, ARK1668E_LCDC_BLD_MODE_LCD_REG0, 0x04030200);
	lcdc_writel(priv, ARK1668E_LCDC_BLD_MODE_LCD_REG1, 0x0003f001);
}

static int ark_lcdc_probe(struct udevice *dev)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);
	int ret;

	priv->sysreg = (void __iomem *)SYS_REG_BASE;
	if (!priv->sysreg) {
		printf("%s: Warning: cannot get sys base addr\n",
		      __func__);
		return -1;
	}

	priv->mmio = (void __iomem *)dev_read_addr_index(dev, 0);
	if (!priv->mmio) {
		printf("%s: Warning: cannot get lcd base addr\n",
		      __func__);
		return -1;
	}

	priv->fb_addr = dev_read_addr_index(dev, 1);
	if (!priv->fb_addr) {
		printf("%s: Warning: cannot get lcd framebuffer addr\n",
		      __func__);
		return -1;
	}

	ret = gpio_request_by_name(dev, "power-control-gpio", 0, &priv->bl_power_gpio,
				   GPIOD_IS_OUT);
	if (ret) {
		printf("%s: Warning: cannot get GPIO: ret=%d\n",
		      __func__, ret);
	} else {
		priv->bl_power = 1;	
	}

	ret = dev_read_u32(dev, "backlight-pwm", &priv->bl_pwm);
	if (!ret) {
		u32 duty;
		priv->bl_val = dev_read_u32_default(dev, "backlight-value", 30);
		duty = priv->bl_val * BACKLIGHT_PWM_PERIOD / BACKLIGHT_MAX_BRIGHTNESS;
		pwm_config(priv->bl_pwm, duty, BACKLIGHT_PWM_PERIOD);
		pwm_enable(priv->bl_pwm);
		dev_read_u32(dev, "backlight-delay", &priv->bl_delay);
	}

	priv->lcd_wiring_mode = ark1668e_lcdfb_get_of_wiring_modes(dev);
	priv->interface_type = ark1668e_lcdfb_get_of_interface_types(dev);
	if (priv->interface_type == ARK1668E_LCDC_INTERFACE_LVDS) {
		ret = dev_read_u32(dev, "lvds-con", &priv->lvds_con);
		if (ret < 0) {
			printf("failed to get property lvds-con\n");
		}
	} else if (priv->interface_type == ARK1668E_LCDC_INTERFACE_DUAL_LVDS) {
		ret = dev_read_u32(dev, "lvds-con", &priv->lvds_con);
		if (ret < 0) {
			printf("failed to get property lvds-con\n");
		}
		ret = dev_read_u32(dev, "lvds-con2", &priv->lvds_con2);
		if (ret < 0) {
			printf("failed to get property lvds-con2\n");
		}
	}
	
	ark_lcdc_init(dev);

	return 0;
}

static int ark_lcdc_ofdata_to_platdata(struct udevice *dev)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);
	const void *blob = gd->fdt_blob;

	if (fdtdec_decode_display_timing(blob, dev_of_offset(dev),
					 0, &priv->timing)) {
		printf("%s: Failed to decode display timing\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int ark_lcdc_bind(struct udevice *dev)
{
	return 0;
}

static int ark_lcdc_read_timing(struct udevice *dev,
				 struct display_timing *timing)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);

	memcpy(timing, &priv->timing, sizeof(struct display_timing));

	return 0;
}

static int ark_lcdc_enable(struct udevice *dev, int panel_bpp,
		     const struct display_timing *timing)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);

	lcdc_writel(priv, ARK1668E_LCDC_EANBLE, 1);
	
	if (priv->bl_power) {
		mdelay(priv->bl_delay);
		dm_gpio_set_value(&priv->bl_power_gpio, 1);
	}

	return 0;
}

static int ark_lcdc_disable(struct udevice *dev)
{
	struct ark_lcdc_priv *priv = dev_get_priv(dev);
	
	if (priv->bl_power)
		dm_gpio_set_value(&priv->bl_power_gpio, 0);

	if (priv->bl_val)
		pwm_disable(priv->bl_pwm);

	lcdc_writel(priv, ARK1668E_LCDC_EANBLE, 0);

	return 0;
}

static const struct dm_display_ops ark_lcdc_ops = {
	.read_timing = ark_lcdc_read_timing,
	.enable = ark_lcdc_enable,
	.disable = ark_lcdc_disable,
};

static const struct udevice_id ark_lcdc_ids[] = {
	{ .compatible = "arkmicro,ark1668e-lcdc" },
	{ }
};

U_BOOT_DRIVER(ark1668e_lcd) = {
	.name	= "ark1668e_lcdc",
	.id	= UCLASS_DISPLAY,
	.of_match = ark_lcdc_ids,
	.bind	= ark_lcdc_bind,
	.probe	= ark_lcdc_probe,
	.ops	= &ark_lcdc_ops,
	.ofdata_to_platdata = ark_lcdc_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct ark_lcdc_priv),
};
