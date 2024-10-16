/*
 * Arkmicro ark1668 lcd driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/poll.h>
#include "ark1668e_lcdc.h"


static struct ark1668e_lcdfb_info *lcdfb_info = NULL;
static void *lcdc_base = NULL;
static int lcdc_width = 0;
static int lcdc_height = 0;
static struct ark_disp_vp lcdc_vp = {0};



static void ark1668e_lcdc_global_enable(int enable)
{
	writel((enable?1:0), lcdc_base + ARK1668E_LCDC_EANBLE);
}

static int ark1668e_lcdc_layer_enable(int layer, int enable)
{
	unsigned int reg;
	unsigned int val;
	int offset = 0;

	reg = ARK1668E_LCDC_CONTROL;
	val = readl(lcdc_base + reg);

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			offset = 5;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			offset = 6;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			offset = 7;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			offset = 8;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			offset = 9;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	if(enable)
		val |= (1<<offset);
	else
		val &= ~(1<<offset);
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_backcolor(int y, int cb, int cr)
{
	unsigned int val = ((y&0xFF) << 16) |((cb&0xFF) << 8) | (cr&0xFF);

	writel(val, lcdc_base + ARK1668E_LCDC_BACK_COLOR);
	return 0;
}

static int ark1668e_lcdc_set_backcolor_tvout(int y, int cb, int cr)
{
	unsigned int val = ((y&0xFF) << 16) |((cb&0xFF) << 8) | (cr&0xFF);

	writel(val, lcdc_base + ARK1668E_LCDC_BACK_COLOR_TV);
	return 0;
}

static int ark1668e_lcdc_set_priority(int video1_prio, int video2_prio, int osd1_prio, int osd2_prio, int osd3_prio)
{
	unsigned int reg;
	unsigned int val;

	if((video1_prio + video2_prio + osd1_prio + osd2_prio + osd3_prio) != 10) {
		printk(KERN_ERR "%s, Invalid priority value, video1_prio:%d, video2_prio:%d, osd1_prio:%d, osd2_prio:%d, osd3_prio:%d\n",
			__FUNCTION__, video1_prio, video2_prio, osd1_prio, osd2_prio, osd3_prio);
		return -1;
	}

	reg = ARK1668E_LCDC_BLD_MODE_LCD_REG0;
	val = readl(lcdc_base + reg);
	val &= ~((0x7<<0) | (0x7<8) | (0x7<<16) | (0x7<24));
	val |= ((video1_prio<<0) | (osd1_prio<8) | (osd2_prio<<16) | (osd3_prio<24));
	writel(val, lcdc_base + reg);

	reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
	val = readl(lcdc_base + reg);
	val &= ~(0x7<<0);
	val |= (video2_prio<<0);
	writel(val, lcdc_base + reg);
	return 0;
}

static int ark1668e_lcdc_set_priority_tvout(int video1_prio, int video2_prio, int osd1_prio, int osd2_prio, int osd3_prio)
{
	unsigned int reg;
	unsigned int val;

	if((video1_prio + video2_prio + osd1_prio + osd2_prio + osd3_prio) != 10) {
		printk(KERN_ERR "%s, Invalid priority value, video1_prio:%d, video2_prio:%d, osd1_prio:%d, osd2_prio:%d, osd3_prio:%d\n",
			__FUNCTION__, video1_prio, video2_prio, osd1_prio, osd2_prio, osd3_prio);
		return -1;
	}

	reg = ARK1668E_LCDC_BLD_MODE_TV_REG0;
	val = readl(lcdc_base + reg);
	val &= ~((0x7<<0) | (0x7<8) | (0x7<<16) | (0x7<24));
	val |= ((video1_prio<<0) | (osd1_prio<8) | (osd2_prio<<16) | (osd3_prio<24));
	writel(val, lcdc_base + reg);

	reg = ARK1668E_LCDC_BLD_MODE_TV_REG1;
	val = readl(lcdc_base + reg);
	val &= ~(0x7<<0);
	val |= (video2_prio<<0);
	writel(val, lcdc_base + reg);
	return 0;
}

static int ark1668e_lcdc_alpha_blend_per_pix_mode_enable(int layer, int enable)
{
	unsigned int pos;
	unsigned int val;

	if ((layer < 0) || (layer > ARK1668E_LCDC_LAYER_MAX))
		return -1;

	val = readl(lcdc_base + ARK1668E_LCDC_BLD_MODE_LCD_REG1);

	switch (layer)
	{
		case ARK1668E_LCDC_LAYER_VIDEO1:
			pos = 10;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			pos = 8;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			pos = 12;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			pos = 14;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			pos = 16;
			break;
		default:
			return -1;
	}

	if (enable)
		val |= 1<<pos;
	else
		val &= ~(1 << pos);
	writel(val, lcdc_base + ARK1668E_LCDC_BLD_MODE_LCD_REG1);
	return 0;
}

static int ark1668e_lcdc_alpha_blend_with_backcolor_enable(int layer, int enable)
{
	unsigned int reg;
	unsigned int val;
	int offset = 0;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
			offset = 11;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
			offset = 9;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
			offset = 13;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
			offset = 15;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
			offset = 17;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = readl(lcdc_base + reg);
	if(enable)
		val |= (0x1<<offset);
	else
		val &= ~(0x1<<offset);
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_alpha_blend_mode(int layer, int mode)
{
	unsigned int reg;
	unsigned int val;
	int offset = 0;

	if((mode < 0) || (mode > 0xE0)) {
		printk(KERN_ERR "%s, Invalid mode:%d\n", __FUNCTION__, mode);
		return -1;
	}

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG0;
			offset = 4;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG1;
			offset = 4;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG0;
			offset = 12;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG0;
			offset = 20;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_BLD_MODE_LCD_REG0;
			offset = 28;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = readl(lcdc_base + reg);
	val &= ~(0xF<<offset);
	val |= (mode<<offset);
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_alpha_blend_mode_tvout(int layer, int mode)
{
	unsigned int reg;
	unsigned int val;
	int offset = 0;

	if((mode < 0) || (mode > 0xE0)) {
		printk(KERN_ERR "%s, Invalid mode:%d\n", __FUNCTION__, mode);
		return -1;
	}

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_BLD_MODE_TV_REG0;
			offset = 4;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_BLD_MODE_TV_REG1;
			offset = 4;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_BLD_MODE_TV_REG0;
			offset = 12;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_BLD_MODE_TV_REG0;
			offset = 20;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_BLD_MODE_TV_REG0;
			offset = 28;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = readl(lcdc_base + reg);
	val &= ~(0xF<<offset);
	val |= (mode<<offset);
	writel(val, lcdc_base + reg);

	return 0;
}


/**************************************************************************************************
 *		Video and Osd common interface.
 *
 **************************************************************************************************/
static int ark1668e_lcdc_set_video_osd_format(int layer, ARK1668E_LCDC_FORMAT format, int yuv_order, int rgb_order)
{
	int rgb_ycbcr_bypass;
	int yuv_ycbcr_bypass;
	int y_uv_order;
	int colour_matrix_reg0;
	int colour_matrix_reg1;
	int colour_matrix_reg2;
	int colour_matrix_reg3;
	int colour_matrix_reg4;
	int colour_matrix_reg5;
	unsigned int reg;
	unsigned int val;

	if((format < 0) || ((format >= ARK1668E_LCDC_FORMAT_MAX) && (format != ARK1668E_LCDC_FORMAT_Y_UV422)&&
		(format != ARK1668E_LCDC_FORMAT_Y_UV420))) {
		printk(KERN_ERR "%s, Invalid fromat:%d\n", __FUNCTION__, format);
		return -1;
	}

#if 0
	if(format <= ARK1668E_LCDC_FORMAT_OSD_BMP24BIT_VIDEO_YUV420) {
		if((layer >= ARK1668E_LCDC_LAYER_VIDEO1) && (layer <= ARK1668E_LCDC_LAYER_VIDEO2)) {
			rgb_ycbcr_bypass = 1;
		} else {
			rgb_ycbcr_bypass = 0;
		}
	} else if(format <= ARK1668E_LCDC_FORMAT_YUV) {
		rgb_ycbcr_bypass = 1;
	} else {
		rgb_ycbcr_bypass = 0;
	}

#else
	rgb_ycbcr_bypass = 1;
	yuv_ycbcr_bypass = 1;
#endif

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_CTL;
			colour_matrix_reg0 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG0;
			colour_matrix_reg1 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG1;
			colour_matrix_reg2 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG2;
			colour_matrix_reg3 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG3;
			colour_matrix_reg4 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG4;
			colour_matrix_reg5 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_CTL;
			colour_matrix_reg0 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG0;
			colour_matrix_reg1 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG1;
			colour_matrix_reg2 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG2;
			colour_matrix_reg3 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG3;
			colour_matrix_reg4 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG4;
			colour_matrix_reg5 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_CTL;
			colour_matrix_reg0 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG0;
			colour_matrix_reg1 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG1;
			colour_matrix_reg2 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG2;
			colour_matrix_reg3 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG3;
			colour_matrix_reg4 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG4;
			colour_matrix_reg5 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_CTL;
			colour_matrix_reg0 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG0;
			colour_matrix_reg1 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG1;
			colour_matrix_reg2 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG2;
			colour_matrix_reg3 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG3;
			colour_matrix_reg4 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG4;
			colour_matrix_reg5 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_CTL;
			colour_matrix_reg0 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG0;
			colour_matrix_reg1 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG1;
			colour_matrix_reg2 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG2;
			colour_matrix_reg3 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG3;
			colour_matrix_reg4 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG4;
			colour_matrix_reg5 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG5;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	if((format >= ARK1668E_LCDC_FORMAT_RGBI555) && (format < ARK1668E_LCDC_FORMAT_MAX)) {
		writel(0, lcdc_base + colour_matrix_reg0);
		writel(0, lcdc_base + colour_matrix_reg1);
		writel(0, lcdc_base + colour_matrix_reg2);
		writel(0, lcdc_base + colour_matrix_reg3);
		writel(0, lcdc_base + colour_matrix_reg4);
		writel(0, lcdc_base + colour_matrix_reg5);
	} else {
		writel(0x01000100, lcdc_base + colour_matrix_reg0);
		writel(0x100167, lcdc_base + colour_matrix_reg1);
		writel(0xf4afa8, lcdc_base + colour_matrix_reg2);
		writel(0x12d100, lcdc_base + colour_matrix_reg3);
		writel(0xf4d000, lcdc_base + colour_matrix_reg4);
		writel(0xf69086, lcdc_base + colour_matrix_reg5);
	}

	val = readl(lcdc_base + reg);
	if((layer >= ARK1668E_LCDC_LAYER_VIDEO1) && (layer <= ARK1668E_LCDC_LAYER_VIDEO2)) {
		int scal_bypass_sel = 1;	//1:bypass; 0:if scale based on input/ouput size.
		int scal_bypass_mode = 1;	//1:disable scale; 0:enable, ignore scal_bypass_sel.
		if(format == ARK1668E_LCDC_FORMAT_Y_UV420) {
			y_uv_order = 1;	//0:y_u_v order, 1:y_uv order.
			format = ARK1668E_LCDC_FORMAT_OSD_BMP24BIT_VIDEO_YUV420;
		} else if(format == ARK1668E_LCDC_FORMAT_Y_UV422) {
			y_uv_order = 1;
			format = ARK1668E_LCDC_FORMAT_OSD_PALETTE_VIDEO_YUV422;
		} else {
			y_uv_order = 0;
		}
		val &= ~((1<<21) | (3<<17) | (1<<9) | (1<<8) | (7<<14) | (1<<5) | (1<<4) | (0xF<<0));
		val |= ((y_uv_order<<21) | (yuv_order<<17) | (rgb_order<<14) | (scal_bypass_sel<<9) | (scal_bypass_mode<<8) | (yuv_ycbcr_bypass<<5) | (rgb_ycbcr_bypass<<4) | (format<<0));
	} else {
		val &= ~((3<<21) | (7<<18) | (1<<17) | (1<<16) | (0xF<<12));
		val |= ((yuv_order<<21) | (rgb_order<<18) | (yuv_ycbcr_bypass<<17) | (rgb_ycbcr_bypass<<16) | (format<<12));
	}
	writel(val, lcdc_base + reg);
	//printk(KERN_ALERT "%s, layer:%d, reg:0x%x, val:0x%x, format:0x%x\n", __FUNCTION__, layer, reg, val, format);

	return 0;
}

static int ark1668e_lcdc_get_video_osd_format(int layer)
{
	unsigned int reg;
	unsigned int val;
	int format;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_CTL;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_CTL;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_CTL;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_CTL;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_CTL;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = readl(lcdc_base + reg);
	if((layer >= ARK1668E_LCDC_LAYER_VIDEO1) && (layer <= ARK1668E_LCDC_LAYER_VIDEO2)) {
		int y_uv_order = (val>>21) & 0x1;;
		format = val & 0xF;
		if(y_uv_order) {
			if(format == ARK1668E_LCDC_FORMAT_OSD_BMP24BIT_VIDEO_YUV420)
				format = ARK1668E_LCDC_FORMAT_Y_UV420;
			else if(format == ARK1668E_LCDC_FORMAT_OSD_PALETTE_VIDEO_YUV422)
				format = ARK1668E_LCDC_FORMAT_Y_UV422;
		}
	} else {
		format = (val>>12)&0xF;
	}

	return format;
}

static int ark1668e_lcdc_set_video_osd_alpha(int layer, int alpha)
{
	unsigned int reg;
	unsigned int val;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_ALPHA1_ALPHA0_BLENDING_COEFF;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_ALPHA1_ALPHA0_BLENDING_COEFF;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_CTL;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_CTL;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_CTL;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = readl(lcdc_base + reg);
	val &= ~0xFF;
	val |= alpha;
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_video_osd_source_size(int layer, int width, int height)
{
	unsigned int val;
	int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_SOURCE_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_SOURCE_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_SOURCE_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_SOURCE_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_SOURCE_SIZE;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = ((height&0xFFF) << 12) | (width&0xFFF);
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_video_osd_size(int layer, int width, int height)
{
	unsigned int val;
	int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_SIZE;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}
	val = ((height&0xFFF) << 12) | (width&0xFFF);

	writel(val, lcdc_base + reg);
	return 0;
}

static int ark1668e_lcdc_set_video_osd_win_point(int layer, int x, int y)
{
	unsigned int val;
	int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_WIN_POINT;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_WIN_POINT;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_WIN_POINT;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_WIN_POINT;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_WIN_POINT;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}
	val = ((y&0xFFF) << 12) | (x&0xFFF);

	writel(val, lcdc_base + reg);
	return 0;
}

static int ark1668e_lcdc_set_video_osd_layer_point(int layer, int x, int y)
{
	unsigned int val;
	int reg;
	int sign_x = 0;
	int sign_y = 0;

	if (x < 0) {
		sign_x = 1;
		x = -x;	
	}

	if (y < 0) {
		sign_y = 1;
		y = -y;
	}

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_POSITION;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_POSITION;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_POSITION;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_POSITION;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_POSITION;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}
	val = (sign_y << 25) | ((y&0xFFF) << 13) | (sign_x << 12) | (x&0xFFF);

	writel(val, lcdc_base + reg);
	return 0;
}

static int ark1668e_lcdc_set_video_osd_blend_win_cut(int layer, int left, int right, int up, int down)
{
	unsigned int reg_cut_lr, reg_cut_ud;
	unsigned int val;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg_cut_lr = ARK1668E_LCDC_BLD_CUT_LEFT_RIGHT_VIDEO1;
			reg_cut_ud = ARK1668E_LCDC_BLD_CUT_UP_DOWN_VIDEO1;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg_cut_lr = ARK1668E_LCDC_BLD_CUT_LEFT_RIGHT_VIDEO2;
			reg_cut_ud = ARK1668E_LCDC_BLD_CUT_UP_DOWN_VIDEO2;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg_cut_lr = ARK1668E_LCDC_BLD_CUT_LEFT_RIGHT_OSD1;
			reg_cut_ud = ARK1668E_LCDC_BLD_CUT_UP_DOWN_OSD1;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg_cut_lr = ARK1668E_LCDC_BLD_CUT_LEFT_RIGHT_OSD2;
			reg_cut_ud = ARK1668E_LCDC_BLD_CUT_UP_DOWN_OSD2;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg_cut_lr = ARK1668E_LCDC_BLD_CUT_LEFT_RIGHT_OSD3;
			reg_cut_ud = ARK1668E_LCDC_BLD_CUT_UP_DOWN_OSD3;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = ((right&0xFFF)<<12) | (left&0xFFF);
	writel(val, lcdc_base + reg_cut_lr);
	val = ((down&0xFFF)<<12) | (up&0xFFF);
	writel(val, lcdc_base + reg_cut_ud);

	return 0;
}

static int ark1668e_lcdc_set_video_osd_colorkey_mask_value(int layer, int y, int cb, int cr, int enable)
{
	unsigned int reg;
	unsigned int val;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_VALUE_VIDEO1;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_VALUE_VIDEO2;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_VALUE_OSD1;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_VALUE_OSD2;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_VALUE_OSD3;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = ((!!enable)<<24) | ((y&0xFF)<<16) | ((cb&0xFF)<<8) | (cr&0xFF);
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_video_osd_colorkey_mask_thld(int layer, int y, int cb, int cr)
{
	unsigned int reg;
	unsigned int val;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_THLD_VIDEO1;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_THLD_VIDEO2;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_THLD_OSD1;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_THLD_OSD2;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_COLOR_KEY_MASK_THLD_OSD3;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = ((y&0xFF)<<16) | ((cb&0xFF)<<8) | (cr&0xFF);
	writel(val, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_set_video_osd_color_matrix(int layer, int reg0_val, int reg1_val, int reg2_val, int reg3_val, int reg4_val, int reg5_val)
{
	unsigned int reg0, reg1, reg2, reg3, reg4, reg5;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg0 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG0;
			reg1 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG1;
			reg2 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG2;
			reg3 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG3;
			reg4 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG4;
			reg5 = ARK1668E_LCDC_VIDEO1_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg0 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG0;
			reg1 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG1;
			reg2 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG2;
			reg3 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG3;
			reg4 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG4;
			reg5 = ARK1668E_LCDC_VIDEO2_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_OSD1:
			reg0 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG0;
			reg1 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG1;
			reg2 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG2;
			reg3 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG3;
			reg4 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG4;
			reg5 = ARK1668E_LCDC_OSD1_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg0 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG0;
			reg1 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG1;
			reg2 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG2;
			reg3 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG3;
			reg4 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG4;
			reg5 = ARK1668E_LCDC_OSD2_COLOUR_MATRIX_REG5;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg0 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG0;
			reg1 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG1;
			reg2 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG2;
			reg3 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG3;
			reg4 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG4;
			reg5 = ARK1668E_LCDC_OSD3_COLOUR_MATRIX_REG5;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	writel(reg0_val, lcdc_base + reg0);
	writel(reg1_val, lcdc_base + reg1);
	writel(reg2_val, lcdc_base + reg2);
	writel(reg3_val, lcdc_base + reg3);
	writel(reg4_val, lcdc_base + reg4);
	writel(reg5_val, lcdc_base + reg5);

	return 0;
}

/**************************************************************************************************
 *		Video interface.
 *
 **************************************************************************************************/

static int ark1668e_lcdc_set_video_win_size(int layer, int width, int height)
{
	unsigned int val;
	int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_WIN_SIZE;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_WIN_SIZE;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}
	val = ((height & 0xFFF) << 12) | (width & 0xFFF);

	writel(val, lcdc_base + reg);
	return 0;
}

int ark1668e_lcdc_set_video_addr(int layer,  unsigned int yaddr,unsigned int cbaddr, unsigned int craddr)
{
	unsigned int reg_addr1, reg_addr2, reg_addr3;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg_addr1 = ARK1668E_LCDC_VIDEO1_ADDR1;
			reg_addr2 = ARK1668E_LCDC_VIDEO1_ADDR2;
			reg_addr3 = ARK1668E_LCDC_VIDEO1_ADDR3;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg_addr1 = ARK1668E_LCDC_VIDEO2_ADDR1;
			reg_addr2 = ARK1668E_LCDC_VIDEO2_ADDR2;
			reg_addr3 = ARK1668E_LCDC_VIDEO2_ADDR3;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	writel(yaddr, lcdc_base + reg_addr1);
	if(cbaddr)
		writel(cbaddr, lcdc_base + reg_addr2);
	if(craddr)
		writel(craddr, lcdc_base + reg_addr3);

	return 0;
}

static int ark1668e_lcdc_get_video_addr(int layer, unsigned int *yaddr,unsigned int *cbaddr, unsigned int *craddr)
{
	unsigned int reg_addr1, reg_addr2, reg_addr3;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg_addr1 = ARK1668E_LCDC_VIDEO1_ADDR1;
			reg_addr2 = ARK1668E_LCDC_VIDEO1_ADDR2;
			reg_addr3 = ARK1668E_LCDC_VIDEO1_ADDR3;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg_addr1 = ARK1668E_LCDC_VIDEO2_ADDR1;
			reg_addr2 = ARK1668E_LCDC_VIDEO2_ADDR2;
			reg_addr3 = ARK1668E_LCDC_VIDEO2_ADDR3;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	*yaddr = readl(lcdc_base + reg_addr1);
	*cbaddr = readl(lcdc_base + reg_addr2);
	*craddr = readl(lcdc_base + reg_addr3);

	return 0;
}

#if 0
static int ark1668e_lcdc_set_video_addr_group1(int layer,  unsigned int yaddr,unsigned int cbaddr, unsigned int craddr)
{
	unsigned int reg_addr1, reg_addr2, reg_addr3;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg_addr1 = ARK1668E_LCDC_VIDEO1_ADDR1_GROUP1;
			reg_addr2 = ARK1668E_LCDC_VIDEO1_ADDR2_GROUP1;
			reg_addr3 = ARK1668E_LCDC_VIDEO1_ADDR3_GROUP1;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg_addr1 = ARK1668E_LCDC_VIDEO2_ADDR1_GROUP1;
			reg_addr2 = ARK1668E_LCDC_VIDEO2_ADDR2_GROUP1;
			reg_addr3 = ARK1668E_LCDC_VIDEO2_ADDR3_GROUP1;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	writel(yaddr, lcdc_base + reg_addr1);
	if(cbaddr)
		writel(cbaddr, lcdc_base + reg_addr2);
	if(craddr)
		writel(craddr, lcdc_base + reg_addr3);

	return 0;
}

static int ark1668e_lcdc_set_video_ycbcr_format(int layer, ARK1668E_LCDC_YCBCR_FORMAT format)
{
	unsigned int reg;
	unsigned int val;
	int offset = 0;

	if((format < 0) || (format >= ARK1668E_LCDC_YCBCR_FORMAT_END)) {
		printk(KERN_ERR "%s, Invalid YCBCR fromat:%d\n", __FUNCTION__, format);
		return -1;
	}

	switch (layer) {
		case ARK1668E_LCDC_LAYER_VIDEO1:
			reg = ARK1668E_LCDC_VIDEO1_CTL;
			offset = 21;
			break;
		case ARK1668E_LCDC_LAYER_VIDEO2:
			reg = ARK1668E_LCDC_VIDEO2_CTL;
			offset = 21;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	val = readl(lcdc_base + reg);
	val &= ~(0x1 << offset);
	val |= ((format&0x1) << offset);
	writel(val, lcdc_base + reg);

	return 0;
}
#endif

static int ark1668e_lcdc_set_video1_scal(
		int layer,
		unsigned int win_width, unsigned int win_height,
		unsigned int left_blank, unsigned int right_blank,
		unsigned int top_blank, unsigned int bottom_blank,
		unsigned int dst_width, unsigned int dst_height,
		int interlace_out_en // 1=interlace, 0=progressive
		)
{
	unsigned int vblank = top_blank + bottom_blank;
	unsigned int hblank = left_blank + right_blank;
	unsigned int reg_ctl, reg_ctl0, reg_ctl1, reg_cut;
	unsigned int val;
	unsigned int format;

	if (layer != ARK1668E_LCDC_LAYER_VIDEO1) {
		printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
		return -EINVAL;
	}

	if (dst_width == 0 || dst_height == 0) {
		printk(KERN_ERR "%s, Invalid dst_width:%d, dst_height:%d\n", __FUNCTION__, dst_width, dst_height);
		return -EINVAL;;
	}

	val = readl(lcdc_base + ARK1668E_LCDC_VIDEO1_CTL);
	format = (val & 0xF);
	if(format == ARK1668E_LCDC_FORMAT_OSD_BMP24BIT_VIDEO_YUV420) {
		int src_width = (readl(lcdc_base + ARK1668E_LCDC_VIDEO1_SOURCE_SIZE) & 0xFFF);
		if(src_width&7) {
			printk(KERN_ERR "Video layer scaler didn't support the width which is not the multiple of 8 when the format is YUV420.\n");
			return -EINVAL;
		}
	}
	if(((val >> 8) & 0x3) != 1) {
		val &= ~(1<<9);	//scale bypass select(1:not bypass).
		val |= (1<<8);	//1: enable scale when scale is not bypass and disable scale when scale bypass; 0: enable scale no matter if scale is bypass.
		writel(val, lcdc_base + ARK1668E_LCDC_VIDEO1_CTL);
	}

	reg_ctl  = ARK1668E_LCDC_VIDEO1_SCALE_CTL;
	reg_ctl0 = ARK1668E_LCDC_VIDEO1_SCAL_CTL0;
	reg_ctl1 = ARK1668E_LCDC_VIDEO1_SCAL_CTL1;
	reg_cut  = ARK1668E_LCDC_VIDEO1_RIGHT_BOTTOM_CUT_NUM;

	val =	0<<11| // 0=addr update per field
			0<<9 | // 10-9: 00=
			0<<8 | // 0=not line chroma
			1<<7 | // 1=YUV
			0<<6 | // 0=disable horizontal filter (use for down scale)
			1<<5 | // 1=auto set coef of h-filter when down scale
			0<<4 | // 0=normal scale de-interlace mode
			0<<3 | // 0=current field is field=0
			0<<2 | // 0=field=0 is odd, field=1 is even
			0<<1 | // 0=de-interlace disable
			0<<0;  // 0=use 2 line buffers

	if ((dst_width + hblank) < win_width)
		val |= 1<<6;
	writel(val, lcdc_base + reg_ctl);

	val = (right_blank<<8) | bottom_blank;
	writel(val, lcdc_base + reg_cut);

	val = (left_blank<<18) |(win_width * 1024 / (dst_width + hblank));
	writel(val, lcdc_base + reg_ctl0);

	val = (top_blank<<18) | (win_height * 1024 / (dst_height + vblank));
	writel(val, lcdc_base + reg_ctl1);

	if (interlace_out_en) {
		val = readl(lcdc_base + ARK1668E_LCDC_TV_CONTROL);
		val &= ~(1<<8);
		writel(val, lcdc_base + ARK1668E_LCDC_TV_CONTROL);

		/* when v scaler cof is 0x400,v scaler  bypass, now we  should change the cof to
			force v scaler, otherwise there was sawtooth on picture */
		val = readl(lcdc_base + reg_ctl1);
		if ((val & 0x3FFFF) == 0x400) {
			val -= 1;
			writel(val, lcdc_base + reg_ctl1);
		}

		val = readl(lcdc_base + reg_ctl);
		val &= ~(7<<9);
		val |= (1<<11) | (1<<9);
		writel(val, lcdc_base + reg_ctl);
	}
	return 0;
}

/**************************************************************************************************
 *		Osd interface.
 *
 **************************************************************************************************/
int ark1668e_lcdc_set_osd_addr(int layer, int addr)
{
	unsigned int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_ADDR;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_ADDR;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_ADDR;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	writel(addr, lcdc_base + reg);

	return 0;
}

static int ark1668e_lcdc_get_osd_addr(int layer)
{
	unsigned int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_ADDR;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_ADDR;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_ADDR;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	return readl(lcdc_base + reg);
}

static int ark1668e_lcdc_set_osd_addr_group1(int layer, int addr)
{
	unsigned int reg;

	switch (layer) {
		case ARK1668E_LCDC_LAYER_OSD1:
			reg = ARK1668E_LCDC_OSD1_ADDR_GROUP1;
			break;
		case ARK1668E_LCDC_LAYER_OSD2:
			reg = ARK1668E_LCDC_OSD2_ADDR_GROUP1;
			break;
		case ARK1668E_LCDC_LAYER_OSD3:
			reg = ARK1668E_LCDC_OSD3_ADDR_GROUP1;
			break;
		default:
			printk(KERN_ERR "%s, Invalid layer:%d\n", __FUNCTION__, layer);
			return -1;
	}

	writel(addr, lcdc_base + reg);

	return 0;
}


/**************************************************************************************************
 *		Ioctl interface.
 *
 **************************************************************************************************/
void ark1668e_lcdc_display_update_atomic(struct ark1668e_lcdfb_info* sinfo)
{
	unsigned int format, yuv_order, rgb_order, i, layer;
	struct ark_disp_atomic *p = NULL;

	if(!sinfo->atomic_flag)
		return;
	for(i = 0; i < ARK1668E_LCDC_LAYER_MAX; i++) {
		if(!(sinfo->atomic_flag & (1 << i)))
			continue;

		p = &sinfo->patomic[i];
		if(!p->atomic_stat || (p->layer < 0) || (p->layer) > ARK1668E_LCDC_LAYER_MAX){
			sinfo->atomic_flag &= ~(1 << i);
			memset(&sinfo->patomic[i], 0 ,sizeof(struct ark_disp_atomic));
			continue;
		}

		//printk(KERN_ALERT "%s: atomic_stat=0x%0x, layer=%d.\n ",__func__, p->atomic_stat, p->layer);
		layer = p->layer;
		if(p->layer >= ARK1668E_LCDC_LAYER_OSD1 && p->layer <= ARK1668E_LCDC_LAYER_OSD3){
			if(p->atomic_stat & ATOMIC_SET_LAYER_POS) {
				ark1668e_lcdc_set_video_osd_layer_point(layer, p->pos_x, p->pos_y);
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_SIZE) {
				ark1668e_lcdc_set_video_osd_size(layer, p->width, p->height);
				ark1668e_lcdc_set_video_osd_source_size(layer, p->width, p->height);
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_FMT) {
				format    = (p->format >> 0)  & 0xFF;
				yuv_order = (p->format >> 16) & 0xF;
				rgb_order = (p->format >> 24) & 0xF;
				ark1668e_lcdc_set_video_osd_format(layer, format, yuv_order, rgb_order);
				if(format == ARK1668E_LCDC_FORMAT_RGBA888){
					ark1668e_lcdc_alpha_blend_with_backcolor_enable(layer, 1);
					ark1668e_lcdc_alpha_blend_per_pix_mode_enable(layer, 1);
				}
				//printk(KERN_ALERT "%s: format=%d, yuv_order=%d, rgb_order=%d.\n ",__func__, format, yuv_order,rgb_order);
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_ADDR)
				ark1668e_lcdc_set_osd_addr(layer, p->addr.yaddr);
		}else{
			if(p->atomic_stat & ATOMIC_SET_LAYER_POS) {
				ark1668e_lcdc_set_video_osd_layer_point(layer, p->pos_x, p->pos_y);
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_SIZE) {
				ark1668e_lcdc_set_video_osd_source_size(layer, p->width, p->height);
				ark1668e_lcdc_set_video_win_size(layer, p->width, p->height);
				ark1668e_lcdc_set_video_osd_win_point(layer, 0, 0);
				ark1668e_lcdc_set_video_osd_size(layer, p->width, p->height);
				//if(!(p->atomic_stat & ATOMIC_SET_LAYER_SCALER)) {
				//	if(layer == ARK1668E_LCDC_LAYER_VIDEO1) {
				//		ark1668e_lcdc_set_video1_scal(layer, p->width, p->height,
				//			0, 0, 0, 0, p->width, p->height, 0);
				//	}
				//}
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_FMT){
				format    = (p->format >> 0)  & 0xFF;
				yuv_order = (p->format >> 16) & 0xF;
				rgb_order = (p->format >> 24) & 0xF;
				ark1668e_lcdc_set_video_osd_format(layer, format, yuv_order, rgb_order);
			}
			if(p->atomic_stat & ATOMIC_SET_LAYER_ADDR)
				ark1668e_lcdc_set_video_addr(layer, p->addr.yaddr, p->addr.cbaddr, p->addr.craddr);
			if(p->atomic_stat & ATOMIC_SET_LAYER_SCALER) {
				if(layer == ARK1668E_LCDC_LAYER_VIDEO1) {
					ark1668e_lcdc_set_video1_scal(layer, p->scaler.src_w, p->scaler.src_h,
						p->scaler.cut_left, p->scaler.cut_right, p->scaler.cut_top, p->scaler.cut_bottom,
						p->scaler.out_w, p->scaler.out_h, 0);
				}
			}
		}

		sinfo->atomic_flag &= ~(1 << i);
		memset(&sinfo->patomic[i], 0 ,sizeof(struct ark_disp_atomic));
	}
}

int ark1668e_lcdc_wait_for_vsync(void)
{
	struct ark1668e_lcdfb_info *sinfo = lcdfb_info;
	int ret;

	if(!sinfo)
		return -EINVAL;

	sinfo->vsync_flag = 0;
	ret = wait_event_interruptible_timeout(sinfo->vsync_waitq,
					sinfo->vsync_flag != 0,
					msecs_to_jiffies(100)); // 100ms at most
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -ETIMEDOUT;
	return 0;
}
EXPORT_SYMBOL(ark1668e_lcdc_wait_for_vsync);

int ark_vin_get_screen_info(int* width,int* height)
{
    if(lcdc_base == NULL){
            return -1;
    }
	*width = lcdc_width;
	*height = lcdc_height;
	return 0;
}
EXPORT_SYMBOL(ark_vin_get_screen_info);

int ark_vin_display_init(int layer,int src_width, int src_height,int out_posx, int out_posy)
{
	if(lcdc_base == NULL)
		return -1;
	ark1668e_lcdc_set_video_osd_source_size(layer, src_width, src_height);
	ark1668e_lcdc_set_video_win_size(layer, src_width, src_height);
	ark1668e_lcdc_set_video_osd_layer_point(layer, out_posx, out_posy);
	ark1668e_lcdc_set_video_osd_win_point(layer, 0, 0);
	ark1668e_lcdc_set_video_osd_size(layer, src_width, src_height);
	ark1668e_lcdc_set_video_osd_format(layer,ARK_LCDC_FORMAT_VYUY,ARK_LCDC_ORDER_VYUY,0);
	return 0;
}
EXPORT_SYMBOL(ark_vin_display_init);
int ark_vin_display_addr(unsigned int addr)
{
    if(lcdc_base == NULL || addr == 0){
            return -1;
    }
	//ark1668e_lcdc_set_video_addr(ARK1668E_LCDC_LAYER_VIDEO2, addr, 0, 0);
	lcdfb_info->render_addr[ARK1668E_LCDC_LAYER_VIDEO2].yaddr = addr;
	return 0;
}
EXPORT_SYMBOL(ark_vin_display_addr);


int ark_vin_get_display_addr(void)
{
	int yaddr,uaddr,vaddr;
    if(lcdc_base == NULL){
            return -1;
    }
	ark1668e_lcdc_get_video_addr(ARK1668E_LCDC_LAYER_VIDEO2, &yaddr,&uaddr,&vaddr);
	
	return yaddr;
}
EXPORT_SYMBOL(ark_vin_get_display_addr);


int ark_bootanimation_display_init(int width, int height, unsigned int Yaddr,unsigned int Uaddr,unsigned int Vaddr,unsigned int format)
{
//	ark1668_lcdc_set_osd_size(OSD_LAYER2, width, height);
//	ark1668_lcdc_set_osd_pos(OSD_LAYER2, (lcdc_width - width) / 2, (lcdc_height - height) / 2);
//	ark1668_lcdc_set_osd_format(OSD_LAYER2, ARK1668_LCDC_FORMAT_VYUY, ARK_LCDC_ORDER_UYVY, 0);
//	ark1668_lcdc_set_osd_addr(OSD_LAYER2, addr);
//	ark1668_lcdc_set_osd_en(OSD_LAYER2, 1);

	if(lcdc_base == NULL)
		return -1;
	ark1668e_lcdc_set_video_osd_source_size(ARK1668E_LCDC_LAYER_VIDEO1, width, height);
	ark1668e_lcdc_set_video_win_size(ARK1668E_LCDC_LAYER_VIDEO1, width, height);
	ark1668e_lcdc_set_video_osd_layer_point(ARK1668E_LCDC_LAYER_VIDEO1, 0, 0);
	ark1668e_lcdc_set_video_osd_win_point(ARK1668E_LCDC_LAYER_VIDEO1, 0, 0);
	ark1668e_lcdc_set_video_osd_size(ARK1668E_LCDC_LAYER_VIDEO1, width, height);
	ark1668e_lcdc_set_video_osd_format(ARK1668E_LCDC_LAYER_VIDEO1,/*ARK1668E_LCDC_FORMAT_Y_UV420*/format,ARK_LCDC_ORDER_VYUY,0);
	ark1668e_lcdc_set_video_addr(ARK1668E_LCDC_LAYER_VIDEO1, Yaddr, Uaddr,Vaddr);
	ark1668e_lcdc_layer_enable(ARK1668E_LCDC_LAYER_VIDEO1, 1);
	return 0;
}
EXPORT_SYMBOL(ark_bootanimation_display_init);

int ark_bootanimation_display_uninit(void)
{
	ark1668e_lcdc_layer_enable(ARK1668E_LCDC_LAYER_VIDEO1, 0);
	return 0;
}
EXPORT_SYMBOL(ark_bootanimation_display_uninit);

int ark_bootanimation_set_display_addr(unsigned int Yaddr,unsigned int Uaddr,unsigned int Vaddr,unsigned int format)
{
	ark1668e_lcdc_set_video_addr(ARK1668E_LCDC_LAYER_VIDEO1, Yaddr, Uaddr, Vaddr);
//	ark1668e_lcdc_wait_for_vsync();

	return 0;
}
EXPORT_SYMBOL(ark_bootanimation_set_display_addr);





static int ark1668e_lcdc_convert_layer(int layer)
{
	switch(layer) {
		case 0:	//fb0 for UI.
			layer = ARK1668E_LCDC_LAYER_OSD2;
			break;
		case 1:	//fb1 for video/carback/phonelink
			layer = ARK1668E_LCDC_LAYER_VIDEO2;
			break;
		case 2:	//overlay for UI(carback track/radar)
			layer = ARK1668E_LCDC_LAYER_OSD1;
			break;
		case 3:	//tvout
			layer = ARK1668E_LCDC_LAYER_VIDEO1;
			break;
		case 4:	//aux for(itu601/itu656). Here is reserved.
			layer = ARK1668E_LCDC_LAYER_OSD3;
			break;
		default:
			layer = -1;
			break;
	}

	return layer;

}

int ark_disp_set_layer_en(int layer_id, int enable)
{
	if(lcdc_base == NULL){
	        return -1;
	}
	if(layer_id > 4 || layer_id < 0){
	        return -1;
	}
	ark1668e_lcdc_layer_enable(layer_id, enable);
	return 0;
}
EXPORT_SYMBOL(ark_disp_set_layer_en);

/*******************************************************************************/

int ark_track_display_init(int width,int height)
{ 
	if(lcdc_base == NULL)
		return -1;
	ark1668e_lcdc_set_video_osd_source_size(ARK1668E_LCDC_LAYER_OSD1, width, height);
	ark1668e_lcdc_set_video_osd_layer_point(ARK1668E_LCDC_LAYER_OSD1, 0, 0);
	ark1668e_lcdc_set_video_osd_win_point(ARK1668E_LCDC_LAYER_OSD1, 0, 0);
	ark1668e_lcdc_set_video_osd_size(ARK1668E_LCDC_LAYER_OSD1, width, height);
	ark1668e_lcdc_set_video_osd_format(ARK1668E_LCDC_LAYER_OSD1,ARK_LCDC_FORMAT_RGBA888,ARK_LCDC_ORDER_YUYV,0);
	return 0;
}
EXPORT_SYMBOL(ark_track_display_init);

int ark_track_set_display_addr(unsigned int addr)
{ 
	//ark1668e_lcdc_set_osd_addr(ARK1668E_LCDC_LAYER_OSD1, addr);
	//ark1668e_lcdc_wait_for_vsync();
	
	lcdfb_info->render_addr[ARK1668E_LCDC_LAYER_OSD1].yaddr = addr;
	return 0;
}
EXPORT_SYMBOL(ark_track_set_display_addr);

int ark_track_alpha_blend(void)
{ 
	unsigned int val;

	val = readl(lcdc_base + ARK1668E_LCDC_BLD_MODE_LCD_REG0);
	val &= ~(0xF << 12);
	writel(val, lcdc_base + ARK1668E_LCDC_BLD_MODE_LCD_REG0);

	val = readl(lcdc_base + ARK1668E_LCDC_BLD_MODE_LCD_REG1);
	val |= (3 << 14);
	writel(val, lcdc_base + ARK1668E_LCDC_BLD_MODE_LCD_REG1); 

	return 0;
}
EXPORT_SYMBOL(ark_track_alpha_blend);

int ark_track_get_screen_info(int* width,int* height)
{
    if(lcdc_base == NULL){
            return -1;
    }
	*width = lcdc_width;
	*height = lcdc_height;
	return 0;
}
EXPORT_SYMBOL(ark_track_get_screen_info);

/*******************************************************************************/

int ark1668e_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct ark1668e_lcdfb_info *sinfo;
    int layer;
	int error = 0;

	if(!info || !info->par) {
		printk(KERN_ERR "ERR: %s, Invalid info:%p or info->par:%p\n", __FUNCTION__, info, info->par);
		error = -EINVAL;
		goto end;
	}
	if(!lcdc_base) {
		printk(KERN_ERR "ERR: %s, Invalid lcdc_base(NULL)\n", __FUNCTION__);
		error = -EINVAL;
		goto end;
	}
	sinfo = info->par;
	//layer = info->node;
	layer = ark1668e_lcdc_convert_layer(info->node);
	if(layer < 0) {
		printk(KERN_ERR "ERR: %s, Invalid layer:%d\n", __FUNCTION__, layer);
		error = -EINVAL;
		goto end;
	}

	/* printk("ark1668e_lcdfb_ioctl layer=%d, cmd=0x%x.\n", layer, cmd); */
	switch (cmd) {
		case FBIO_WAITFORVSYNC:
		case ARKFB_WAITFORVSYNC:
			error = ark1668e_lcdc_wait_for_vsync();
			break;
		case ARKFB_SHOW_WINDOW:
			error = ark1668e_lcdc_layer_enable(layer, 1);
			break;
		case ARKFB_HIDE_WINDOW:
			error = ark1668e_lcdc_layer_enable(layer, 0);
			break;
		case ARKFB_SET_WINDOW_POS: {
			unsigned int x, y, data;
			if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))) {
					printk("ERR: %s, copy from user para error\n", __func__);
					error = -EFAULT;
					goto end;
			}
			x =  data & 0xFFFF;
			y = (data >> 16) & 0xFFFF;
			error = ark1668e_lcdc_set_video_osd_layer_point(layer, x, y);
			break;
		}
        case ARKFB_SET_WINDOW_SIZE: {
			unsigned int width, height, data;
			if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))){
					printk("ERR: %s, copy from user para error\n", __func__);
					error = -EFAULT;
					goto end;
			}
			width  = data & 0xFFFF;
			height = (data >> 16) & 0xFFFF;
			if((layer >= ARK1668E_LCDC_LAYER_OSD1) && (layer <= ARK1668E_LCDC_LAYER_OSD3)) {
				error += ark1668e_lcdc_set_video_osd_size(layer, width, height);
				error += ark1668e_lcdc_set_video_osd_source_size(layer, width, height);
			} else if ((layer >= ARK1668E_LCDC_LAYER_VIDEO1) && (layer <= ARK1668E_LCDC_LAYER_VIDEO2)) {
				error += ark1668e_lcdc_set_video_osd_source_size(layer, width, height);
				error += ark1668e_lcdc_set_video_osd_win_point(layer, 0, 0);
				error += ark1668e_lcdc_set_video_win_size(layer, width, height);
				error += ark1668e_lcdc_set_video_osd_size(layer, width, height);
				//scale
				//if(layer == ARK1668E_LCDC_LAYER_VIDEO1) {
				//	error += ark1668e_lcdc_set_video1_scal(ARK1668E_LCDC_LAYER_VIDEO1, width, height,
				//		0, 0, 0, 0, width, height, 0);
				//}
			}
			break;
        }
		case ARKFB_SET_WINDOW_FORMAT: {
			unsigned int data, format, yuv_order, rgb_order;
			if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))) {
					printk("ERR: %s, copy from user para error\n", __func__);
					error = -EFAULT;
					goto end;
			}
			format	  = (data >> 0)  & 0xFF;
			yuv_order = (data >> 16) & 0xF;
			rgb_order = (data >> 24) & 0xF;
			error = ark1668e_lcdc_set_video_osd_format(layer, format, yuv_order, rgb_order);
			//if((layer >= ARK1668E_LCDC_LAYER_OSD1) && (layer <= ARK1668E_LCDC_LAYER_OSD3)) {
				//error += ark1668e_lcdc_alpha_blend_with_backcolor_enable(layer, 1);
				//error += ark1668e_lcdc_alpha_blend_per_pix_mode_enable(layer, 1);
			//}
			if(format == ARK1668E_LCDC_FORMAT_RGBA888){
				error += ark1668e_lcdc_alpha_blend_with_backcolor_enable(layer, 1);
				error += ark1668e_lcdc_alpha_blend_per_pix_mode_enable(layer, 1);
			}
			printk(KERN_DEBUG "layer=%d: format=%d, yuv_order:%d, rgb_order:%d\n",
				layer, format, yuv_order, rgb_order);
			break;
		}
		case ARKFB_SET_WINDOW_ADDR: {
			struct ark_disp_addr addr;
			if(copy_from_user(&addr, (void *)arg, sizeof(struct ark_disp_addr))){
				printk("ERR: %s, copy from user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			memcpy(&sinfo->render_addr[layer], &addr, sizeof(struct ark_disp_addr));
			//printk(KERN_ALERT "layer=%d: yaddr=0x%0x, cbaddr=0x%0x, craddr=0x%0x.\n ",layer, addr.yaddr, addr.cbaddr, addr.craddr);
			break;
		}
		case ARKFB_SET_WINDOW_SCALER: {
			struct ark_disp_scaler scaler;
			if(layer != ARK1668E_LCDC_LAYER_VIDEO1){
				error = -EINVAL;
				printk("ERR: %s, Only video1 layer support scaler\n", __func__);
				goto end;
			}
			if(copy_from_user(&scaler, (void *)arg, sizeof(struct ark_disp_scaler))){
				printk("ERR: %s, copy from user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			error += ark1668e_lcdc_set_video_osd_size(layer, scaler.out_w, scaler.out_h);
			error += ark1668e_lcdc_set_video1_scal(layer, scaler.src_w, scaler.src_h,
				scaler.cut_left, scaler.cut_right, scaler.cut_top, scaler.cut_bottom,
				scaler.out_w, scaler.out_h, 0);
			//printk(KERN_DEBUG "layer=%d: scaler src_w=%d, src_h=%d, out_w=%d, out_h=%d.\n ",
			//	layer, scaler.src_w, scaler.src_h, scaler.out_w, scaler.out_h);
			break;
		}
		case ARKFB_SET_WINDOW_ATOMIC: {
			struct ark_disp_atomic atomic;
			if(copy_from_user(&atomic, (void *)arg, sizeof(struct ark_disp_atomic))){
				printk("ERR: %s, copy from user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			atomic.layer = ark1668e_lcdc_convert_layer(atomic.layer);
			if(!atomic.atomic_stat || atomic.layer != layer){
				printk("ERR: %s, atomic_stat:%d or layer:%d error\n", __func__, atomic.atomic_stat, atomic.layer);
				error = -EFAULT;
				goto end;
			}
			printk(KERN_DEBUG "%s===>layer=%d, atomic_stat=0x%0x.\n ",__func__, layer, atomic.atomic_stat);
			sinfo->atomic_flag |= (1 << layer);
			memcpy(&sinfo->patomic[layer], &atomic, sizeof(struct ark_disp_atomic));
			break;
		}
		case ARKFB_GET_WINDOW_ADDR: {
			struct ark_disp_addr addr;
			memset(&addr, 0, sizeof(struct ark_disp_addr));
			if((layer >= ARK1668E_LCDC_LAYER_OSD1) && (layer <= ARK1668E_LCDC_LAYER_OSD3)) {
				addr.yaddr = ark1668e_lcdc_get_osd_addr(layer);
				if(addr.yaddr < 0) {
					addr.yaddr = 0;
					goto end;
				}
			} else {
				error += ark1668e_lcdc_get_video_addr(layer, &addr.yaddr, &addr.cbaddr, &addr.craddr);
				if(error < 0) {
					printk("%s: ark1668e_lcdc_get_video_addr failed\n", __func__);
					error = -EFAULT;
					goto end;
				}
			}
			if(copy_to_user((void  *)arg, &addr, sizeof(struct ark_disp_addr))){
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_GET_SCREEN_INFO: {
			struct ark_screen screen;
			memset(&screen, 0, sizeof(struct ark_screen));
			screen.width  = screen.disp_width  = lcdc_width;
			screen.height = screen.disp_height = lcdc_height;
			if(copy_to_user((void  *)arg, &screen, sizeof(struct ark_screen))){
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_SET_SCREEN_INFO: {
			struct ark_screen screen;
			if(copy_from_user(&screen, (void  *)arg, sizeof(struct ark_screen))){
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			///////////////// Reserved///////////////////
			break;
		}
		case ARKFB_GET_PLATFORM_INFO: {
			struct ark_platform_info platform;
			memset(&platform, 0, sizeof(struct ark_platform_info));
			platform.type = ARK_PLATFORM_ARK1668E;
			if(copy_to_user((void  *)arg, &platform, sizeof(struct ark_platform_info))){
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_GET_WINDOW_FORMAT: {
			int format = ark1668e_lcdc_get_video_osd_format(layer);
			if(format < 0) {
				printk("%s: get format failed\n", __func__);
				error = -EFAULT;
				goto end;
			}
			if(copy_to_user((void  *)arg, &format, sizeof(int))){
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_SET_VP_INFO: {
			struct ark_disp_vp vp;
			memset(&vp, 0, sizeof(struct ark_disp_vp));
			if(copy_from_user(&vp, (void  *)arg, sizeof(struct ark_disp_vp))) {
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			error += ark1668e_lcdc_set_video_osd_color_matrix(layer,
				vp.reg[0], vp.reg[1], vp.reg[2], vp.reg[3], vp.reg[4], vp.reg[5]);
			if(error == 0) {
				memcpy(&lcdc_vp, &vp, sizeof(struct ark_disp_vp));
			}
			break;
		}
		case ARKFB_GET_VP_INFO: {
			if(copy_to_user((void  *)arg, &lcdc_vp, sizeof(struct ark_disp_vp))) {
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			break;
		}
		case ARKFB_SET_REG_VALUE: {
			struct ark_disp_reg reg;

			if(copy_from_user(&reg, (void *)arg, sizeof(struct ark_disp_reg))){
				printk("%s: copy from user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}

			if((reg.addr & 0xffff0000) == 0xe0500000){        
				writel(reg.value, sinfo->mmio + (reg.addr&0xffff));
				printk("arkfb write reg:0x%0x=0x%0x.\n ", reg.addr, reg.value);
			}else{
				error = -EINVAL;
				goto end;
			}
			break;
		}
        case ARKFB_GET_REG_VALUE: {
			struct ark_disp_reg reg;
			if(copy_from_user(&reg, (void *)arg, sizeof(struct ark_disp_reg))){
				printk("%s: copy from user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			if((reg.addr & 0xffff0000) == 0xe0500000){        
				reg.value = readl(sinfo->mmio + (reg.addr&0xffff));
				printk("arkfb read reg:0x%0x=0x%0x.\n ", reg.addr, reg.value);
			}else{
				error = -EINVAL;
				goto end;
			}
			if(copy_to_user((void  *)arg, &reg, sizeof(struct ark_disp_reg))){
				printk("%s: copy to user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			break;
		}
		default:
			break;
	}
end:
    return error;
}
EXPORT_SYMBOL(ark1668e_lcdfb_ioctl);

int ark1668e_lcdc_funcs_init(struct ark1668e_lcdfb_info *sinfo)
{
	struct fb_info *info = NULL;
	struct fb_var_screeninfo *var = NULL;


	if(!sinfo) {
		printk(KERN_ERR "ERR: %s, Invalid sinfo(NULL)\n", __func__);
		return -EINVAL;
	}
	info = sinfo->info;
	var = &info->var;

	lcdfb_info	= sinfo;
	lcdc_base	= sinfo->mmio;
	lcdc_width	= var->xres;
	lcdc_height	= var->yres;

	return 0;
}
EXPORT_SYMBOL(ark1668e_lcdc_funcs_init);

