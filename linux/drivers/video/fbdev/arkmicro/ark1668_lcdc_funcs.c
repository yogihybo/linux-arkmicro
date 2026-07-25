/*
 * Arkmicro ark1668 lcd driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/poll.h>
#include "ark1668_lcdc.h"

#define DEBUG

static int lcdc_width;
static int lcdc_height;
static void *lcdc_base = NULL;

static struct ark1668_lcdfb_info *lcdfb_info;

int ark1668_lcdc_funcs_init(struct ark1668_lcdfb_info *sinfo)
{
	struct fb_info *info = sinfo->info;
	struct fb_var_screeninfo *var = &info->var;
	
	lcdfb_info = sinfo;

	lcdc_width = var->xres;
	lcdc_height = var->yres;
	lcdc_base = sinfo->mmio;

	return 0;
}

static void ark1668_lcdc_set_video_source_size(int id, int width, int height)
{
	unsigned int val;
	int source_size_reg;
    
    if (id < 0 || id > VIDEO_LAYER2) return;
    
	val = ((height & 0xfff) << 12) | (width & 0xfff);
    
	switch (id) {
	case VIDEO_LAYER1:
		source_size_reg = ARK1668_LCDC_VIDEO_SOURCE_SIZE;
		break;
	case VIDEO_LAYER2:
		source_size_reg = ARK1668_LCDC_VIDEO2_SOURCE_SIZE;
		break;
	}

    writel(val, lcdc_base + source_size_reg);
}

static void ark1668_lcdc_set_video_layer_size(int id, int width, int height)
{
	unsigned int val;
	int size_reg;
    
        if (id < 0 || id > VIDEO_LAYER2) return;
    
	val = ((height & 0xfff) << 12) | (width & 0xfff);
    
	switch (id) {
	case VIDEO_LAYER1:
		size_reg = ARK1668_LCDC_VIDEO_SIZE;
		break;
	case VIDEO_LAYER2:
		size_reg = ARK1668_LCDC_VIDEO2_SIZE;
		break;
	}

        writel(val, lcdc_base + size_reg);
}


static void ark1668_lcdc_set_video_layer_pos(int id, int x, int y)
{
	int reg;
	int sign_x = 0, sign_y = 0;
	unsigned int val;

	if (id < 0 || id > VIDEO_LAYER2) return;

	if (x < 0) {
		sign_x = 1;
		x = -x;	
	}

	if (y < 0) {
		sign_y = 1;
		y = -y;
	}

	x &= 0xfff;
	y &= 0xfff;

	switch (id) {
	case VIDEO_LAYER1:
		reg = ARK1668_LCDC_VIDEO_POSITION;
		break;
	case VIDEO_LAYER2:
		reg = ARK1668_LCDC_VIDEO2_POSITION;
		break;
	}

	val = (sign_y << 25) | (y << 13) | (sign_x << 12) | x;

	writel(val, lcdc_base + reg);
}

static void ark1668_lcdc_set_video_win_size(int id, int width, int height)
{
	unsigned int val;
	int size_reg;
    
        if (id < 0 || id > VIDEO_LAYER2) return;
    
	val = ((height & 0xfff) << 12) | (width & 0xfff);
    
	switch (id) {
	case VIDEO_LAYER1:
		size_reg = ARK1668_LCDC_VIDEO_WIN_SIZE;
		break;
	case VIDEO_LAYER2:
		size_reg = ARK1668_LCDC_VIDEO2_WIN_SIZE;
		break;
	}

        writel(val, lcdc_base + size_reg);
}

static void ark1668_lcdc_set_video_win_point(int id, int x, int y)
{
	unsigned int val;
	int reg;
    
        if (id < 0 || id > VIDEO_LAYER2) return;
    
	val = ((y & 0xfff) << 12) | (x & 0xfff);
    
	switch (id) {
	case VIDEO_LAYER1:
		reg = ARK1668_LCDC_VIDEO_WIN_POINT;
		break;
	case VIDEO_LAYER2:
		reg = ARK1668_LCDC_VIDEO2_WIN_POINT;
		break;
	}

        writel(val, lcdc_base + reg);
}


static void ark1668_lcdc_set_video_en(int id, int enable)
{
	unsigned int val;
	int offset;

	if (id < 0 || id > VIDEO_LAYER2) return;

	val = readl(lcdc_base + ARK1668_LCDC_CONTROL);

	switch (id) {
	case VIDEO_LAYER1:
		offset = 5;
		break;
	case VIDEO_LAYER2:
		offset = 6;
		break;	
	}

	if (enable)
		val |= (1 << offset);
	else 
		val &= ~(1 << offset);

	writel(val, lcdc_base + ARK1668_LCDC_CONTROL);
}

static void ark1668_lcdc_set_video_format(int id, unsigned int format,
		unsigned int yuyv_order, unsigned int rgb_order, unsigned int scal_bypass)
{
	unsigned int yuv_ycbcr_bypass, rgb_ycbcr_bypass;
	unsigned int y_uv_order;
	unsigned int pixfmt;
	int reg;
	unsigned val;
    
	if (id < 0 || id > VIDEO_LAYER2) return;
    
	switch (id) {
	case VIDEO_LAYER1:
		reg = ARK1668_LCDC_VIDEO_CTL;
		break;
	case VIDEO_LAYER2:
		reg = ARK1668_LCDC_VIDEO2_CTL;
		break;
	}	
    
	switch (format)
	{
		case ARK_LCDC_FORMAT_RGBI555:
		case ARK_LCDC_FORMAT_R5G6B5:
		case ARK_LCDC_FORMAT_RGBA888:	
		case ARK_LCDC_FORMAT_RGB888:

			yuv_ycbcr_bypass = 1;
			rgb_ycbcr_bypass = 0;	
			y_uv_order = 0;
			break;
		case ARK_LCDC_FORMAT_Y_UV422:
		case ARK_LCDC_FORMAT_Y_UV420:
			yuv_ycbcr_bypass = 1;
			rgb_ycbcr_bypass = 1;	
			y_uv_order = 1;
			break;	
		case ARK_LCDC_FORMAT_VYUY:
			yuv_ycbcr_bypass = 0;
			rgb_ycbcr_bypass = 0;
			y_uv_order = 0;
			break;
		default:
			yuv_ycbcr_bypass = 1;
			rgb_ycbcr_bypass = 1;
			y_uv_order = 0;
			break;
	}
	pixfmt = format & 0x0F;
    
    //val = readl(lcdc_base + reg);
	if (id == 0) {
	    val = 
            0<<23 | // 0=disable to get more data bytes on each line
			1<<22 | // 1=select video size specified on CLCD_VIDEO_SIZE 
			y_uv_order<<21 | // 1=Y_UV, 0=Y_U_V
			0<<20 | // 0=double buffer mode disabled
			0<<19 | // 0=use address group 0 for display
			yuyv_order<<17 | // 18-17: 0=yuyv in case YUV422
			rgb_order<<14 | // 16-14: 000=RGB data order or YUV order for YUV444
			0<<11 | // 0=do not keep UV data for next line in case 
			0<<10 | // 0=use 2-step FIR low-pass filter to do data transfer
			scal_bypass<<9 |  // 1=enable video scalar by pass mode 
			1<<8 |  // 1=whole scaler is disable when scaler is in bypass mode
			0<<7 |  // 0=field mode for interlace data capture of video 
			0<<6 |  // 0=not invert field 
			yuv_ycbcr_bypass<<5 | // 1=no YUV to YCbCr, 0=YUV to YCbCr 
			rgb_ycbcr_bypass<<4 | // 1=no RGB to YCbCr, 0=RGB to YCbCr
			pixfmt<<0;   // 3-0: video layer source format

		if (pixfmt == ARK_LCDC_FORMAT_YUV420)
			val |= 1<<11;
	} else if (id == 1) {
	    val = 
            0<<26 | // 0=u/v write first in doing data write back
			0<<25 | // 0=v first in doing data write back 
			0<<24 | // wb_start, set this bit from 0->1 will start write back
			0<<23 | // 0=disable to get more data bytes for each line read
			1<<22 | // 1= select video size on REG0x330 rLCD_VIDEO2_SIZE
			y_uv_order<<21 | // 0=Y_U_V, 1=Y_UV
			0<<20 | // 0=double buffer mode disable
			0<<19 | // 0=use addr group 0 when double buffer mode disable
			yuyv_order<<17 | // 18-17: 00=yuyv when format is YUV422
			rgb_order<<14 | // 16-14: 000=RGB when format is YUV444 or RGB
			0<<13 | // 0=not save UV data for displaying next line
			0<<12 | // 0=use 2-step FIR low-pass filter for processing data
			0<<11 | // 0=use axi_clk
			0<<10 | // 0=?v????? progressive read in doing write back
			scal_bypass<<9 |  // 1=enable scale by pass mode
			1<<8 |  // 1=disable whole scale in video layer when scaler is 
			0<<7 |  // 0=field mode select for interlace data capture of 
			0<<6 |  // 0=field not invert for frame mode and interlace data
			yuv_ycbcr_bypass<<5 | // 1=no yuv2ycbcr
			rgb_ycbcr_bypass<<4 | // 1=no rgb2ycbcr, 0=do rgb2ycbcr
			pixfmt<<0;   // 3-0: video layer src format
		if (pixfmt == ARK_LCDC_FORMAT_YUV420)         
			val |= 1<<13;
	}
    
	writel(val, lcdc_base + reg);     
}

static void ark1668_lcdc_set_video_scal(
		int id,
		unsigned int win_width, unsigned int win_height,
		unsigned int left_blank, unsigned int right_blank,
		unsigned int top_blank, unsigned int bottom_blank,
		unsigned int dst_width, unsigned int dst_height,
		int interlace_out_en,
		int interlace_read_en // 1=interlace, 0=progressive
		)
{
	unsigned int vblank = top_blank + bottom_blank;
	unsigned int hblank = left_blank + right_blank;
        unsigned int reg_ctl, reg_ctl0, reg_ctl1, reg_cut;
	unsigned int val;
    
	if (id < 0 || id > VIDEO_LAYER2) return;

	if (dst_width == 0 || dst_height == 0)
		return;
    
	switch (id) {
	case VIDEO_LAYER1:
        	reg_ctl  = ARK1668_LCDC_VIDEO_SCALE_CTL;
                reg_ctl0 = ARK1668_LCDC_VIDEO_SCAL_CTL0;
                reg_ctl1 = ARK1668_LCDC_VIDEO_SCAL_CTL1;
                reg_cut  = ARK1668_LCDC_VIDEO_RIGHT_BOTTOM_CUT_NUM;
		break;
	case VIDEO_LAYER2:
		reg_ctl  = ARK1668_LCDC_VIDEO2_SCALE_CTL;
		reg_ctl0 = ARK1668_LCDC_VIDEO2_SCAL_CTL0;
		reg_ctl1 = ARK1668_LCDC_VIDEO2_SCAL_CTL1;
                reg_cut  = ARK1668_LCDC_VIDEO2_RIGHT_BOTTOM_CUT_NUM;
		break;
	}

        val = 0<<11| // 0=addr update per field
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
    
        if ((dst_width + hblank) * 3 / 2 < win_width)
                val |= 1<<6;
        writel(val, lcdc_base + reg_ctl);
    
        val = right_blank<<8 | bottom_blank;
        writel(val, lcdc_base + reg_cut);
    
        val = left_blank<<18 |(win_width * 1024 / (dst_width + hblank));
        writel(val, lcdc_base + reg_ctl0);
    
        val = top_blank<<18 | (win_height * 1024 / (dst_height + vblank));
        writel(val, lcdc_base + reg_ctl1);
        if ((val & 0x3FFFF) == 0x400){
                val = readl(lcdc_base + reg_ctl);
                val |= (1 << 12) | (1 << 0);
                writel(val, lcdc_base + reg_ctl);
        }
    
        if (interlace_out_en) {
                val = readl(lcdc_base + ARK1668_LCDC_TV_CONTROL);
                val &= ~(1<<8);
                writel(val, lcdc_base + ARK1668_LCDC_TV_CONTROL);

                val = readl(lcdc_base + reg_ctl1);
                if ((val & 0x3FFFF) == 0x400){
                        val -= 1;
                        writel(val, lcdc_base + reg_ctl1);
                }
    
                val = readl(lcdc_base + reg_ctl);
                val &= ~(7<<9); 
                val |= 1<<11 | 1<<9;
                writel(val, lcdc_base + reg_ctl);
        }

}

void ark1668_lcdc_set_video_addr(int id,  unsigned int yaddr,unsigned int cbaddr, unsigned int craddr)
{
	unsigned int reg_addr1, reg_addr2, reg_addr3;

	if (id < 0 || id > VIDEO_LAYER2) return;

	switch (id) {
	case VIDEO_LAYER1:
		reg_addr1 = ARK1668_LCDC_VIDEO_ADDR1;                
		reg_addr2 = ARK1668_LCDC_VIDEO_ADDR2;
		reg_addr3 = ARK1668_LCDC_VIDEO_ADDR3;
		break;
	case VIDEO_LAYER2:
		reg_addr1 = ARK1668_LCDC_VIDEO2_ADDR1;
		reg_addr2 = ARK1668_LCDC_VIDEO2_ADDR2;
		reg_addr3 = ARK1668_LCDC_VIDEO2_ADDR3;
		break;
	}

	writel(yaddr, lcdc_base + reg_addr1);

	if(cbaddr) writel(cbaddr, lcdc_base + reg_addr2);
	if(craddr) writel(craddr, lcdc_base + reg_addr3);
}

static void ark1668_lcdc_get_video_addr(int id, unsigned int *yaddr, 
        unsigned int *cbaddr, unsigned int *craddr)
{

	if (id < 0 || id > VIDEO_LAYER2) return;
    
	switch (id) {
	case VIDEO_LAYER1:
		*yaddr  = readl(lcdc_base + ARK1668_LCDC_VIDEO_ADDR1);
                *cbaddr = readl(lcdc_base + ARK1668_LCDC_VIDEO_ADDR2);
                *craddr = readl(lcdc_base + ARK1668_LCDC_VIDEO_ADDR3);
		break;
	case VIDEO_LAYER2:
                *yaddr  = readl(lcdc_base + ARK1668_LCDC_VIDEO2_ADDR1);
                *cbaddr = readl(lcdc_base + ARK1668_LCDC_VIDEO2_ADDR2);
                *craddr = readl(lcdc_base + ARK1668_LCDC_VIDEO2_ADDR3);
		break;
	}

}

static void ark1668_lcdc_set_osd_size(int id, int width, int height)
{
	unsigned int val;
	int size_reg, source_size_reg;

	if (id < 0 || id > OSD_LAYER3) return;

	val = ((height & 0xfff) << 12) | (width & 0xfff);

	switch (id) {
	case OSD_LAYER1:
		size_reg = ARK1668_LCDC_OSD1_SIZE;
		source_size_reg = ARK1668_LCDC_OSD1_SOURCE_SIZE;
		break;
	case OSD_LAYER2:
		size_reg = ARK1668_LCDC_OSD2_SIZE;
		source_size_reg = ARK1668_LCDC_OSD2_SOURCE_SIZE;
		break;
	case OSD_LAYER3:
		size_reg = ARK1668_LCDC_OSD3_SIZE;
		source_size_reg = ARK1668_LCDC_OSD3_SOURCE_SIZE;
		break;	
	}

	writel(val, lcdc_base + size_reg);
	writel(val, lcdc_base + source_size_reg);
}

static void ark1668_lcdc_set_osd_pos(int id, int x, int y)
{
	int reg;
	int sign_x = 0, sign_y = 0;
	unsigned int val;

	if (id < 0 || id > OSD_LAYER3) return;

	if (x < 0) {
		sign_x = 1;
		x = -x;	
	}

	if (y < 0) {
		sign_y = 1;
		y = -y;
	}

	x &= 0xfff;
	y &= 0xfff;

	switch (id) {
	case OSD_LAYER1:
		reg = ARK1668_LCDC_OSD1_POS;
		break;
	case OSD_LAYER2:
		reg = ARK1668_LCDC_OSD2_POS;
		break;
	case OSD_LAYER3:
		reg = ARK1668_LCDC_OSD3_POS;
		break;	
	}

	val = (sign_y << 25) | (y << 13) | (sign_x << 12) | x;

	writel(val, lcdc_base + reg);
}

static void ark1668_lcdc_set_osd_format(int id, int format, int yuv_order, int rgb_order)
{
	int reg;
	unsigned val;
	int yuv_ycbcr_bypass, rgb_ycbcr_bypass;

	if (id < 0 || id > OSD_LAYER3) return;

        switch (format) {
        case ARK1668_LCDC_FORMAT_VYUY:
                yuv_ycbcr_bypass = 1;
                rgb_ycbcr_bypass = 1;	
                break;
        case ARK1668_LCDC_FORMAT_YUV:
                yuv_ycbcr_bypass = 1;
                rgb_ycbcr_bypass = 1;	
                break;
        case ARK1668_LCDC_FORMAT_RGBI555:
        case ARK1668_LCDC_FORMAT_R5G6B5:
        case ARK1668_LCDC_FORMAT_RGBA888:
        case ARK1668_LCDC_FORMAT_RGB888:
                yuv_ycbcr_bypass = 1;
                rgb_ycbcr_bypass = 0;	
                break;
        default:
                return;
        }

	switch (id) {
	case OSD_LAYER1:
		reg = ARK1668_LCDC_OSD1_CTL;
		break;
	case OSD_LAYER2:
		reg = ARK1668_LCDC_OSD2_CTL;
		break;
	case OSD_LAYER3:
		reg = ARK1668_LCDC_OSD3_CTL;
		break;	
	}	

	val = readl(lcdc_base + reg);
	val &= ~(0x7ff << 12);
	val |= ((yuv_order & 3) << 21) | ((rgb_order & 7) << 18) | (yuv_ycbcr_bypass << 17) |
		(rgb_ycbcr_bypass << 16) | ((format & 0xf) << 12);
	writel(val, lcdc_base + reg);
}

void ark1668_lcdc_set_osd_addr(int id, unsigned int addr)
{
	int reg;

	if (id < 0 || id > OSD_LAYER3) return;

	switch (id) {
	case OSD_LAYER1:
		reg = ARK1668_LCDC_OSD1_ADDR;
		break;
	case OSD_LAYER2:
		reg = ARK1668_LCDC_OSD2_ADDR;
		break;
	case OSD_LAYER3:
		reg = ARK1668_LCDC_OSD3_ADDR;
		break;	
	}

	writel(addr, lcdc_base + reg);
}


unsigned int ark1668_lcdc_get_osd_addr(int id)
{
    int reg;
    
    if (id < 0 || id > OSD_LAYER3) return 0;
    
    switch (id) {
    case OSD_LAYER1:
            reg = ARK1668_LCDC_OSD1_ADDR;
            break;
    case OSD_LAYER2:
            reg = ARK1668_LCDC_OSD2_ADDR;
            break;
    case OSD_LAYER3:
            reg = ARK1668_LCDC_OSD3_ADDR;
            break;  
    }
    
    return readl(lcdc_base + reg);
}


static void ark1668_lcdc_set_osd_en(int id, int enable)
{
	unsigned int val;
	int offset;

	if (id < 0 || id > OSD_LAYER3) return;

	val = readl(lcdc_base + ARK1668_LCDC_CONTROL);

	switch (id) {
	case OSD_LAYER1:
		offset = 7;
		break;
	case OSD_LAYER2:
		offset = 8;
		break;
	case OSD_LAYER3:
		offset = 9;
		break;	
	}

	if (enable)
		val |= (1 << offset);
	else 
		val &= ~(1 << offset);

	writel(val, lcdc_base + ARK1668_LCDC_CONTROL);
}

static void ark1668_lcdc_set_video_priority(int level)
{
	unsigned int val;
	val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
	val &= ~(0x7<<0);
	val |= (level<<0);
	writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
}
		 
static void ark1668_lcdc_set_video2_priority(int level)
{
	unsigned int val;
	val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
	val &= ~(0x7<<0);
	val |= (level<<0);
	writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
}

static void ark1668_lcdc_set_win1_priority(int level)
{
	unsigned int val;
	val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
	val &= ~(0x7<<8);
	val |= (level<<8);
	writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
}

static void ark1668_lcdc_set_win2_priority(int level)
{
	unsigned int val;
	val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
	val &= ~(0x7<<16);
	val |= (level<<16);
	writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
}

static void ark1668_lcdc_set_win3_priority(int level)
{
	unsigned int val;
	val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
	val &= ~(0x7<<24);
	val |= (level<<24);
	writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
}

static void ark1668_lcdc_set_osd_alpha_blend_en_lcd(int id, int enable)
{
        unsigned int pos;
        unsigned int val;

        if (id < 0 || id > OSD_LAYER3) return;

        val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
 
        switch (id)
        {
        case OSD_LAYER1:
                pos = 13;
                break;
        case OSD_LAYER2:
                pos = 15;
                break;
        case OSD_LAYER3:
                pos = 17;
                break;
        default:
                return;
        }

        if (enable)
                val |= 1 << pos;
        else
                val &= ~(1 << pos);

        writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG1); 
}

static void ark1668_lcdc_set_osd_per_pix_alpha_blend_en_lcd(int id, int enable)
{
        unsigned int pos;
        unsigned int val;

        if (id < 0 || id > OSD_LAYER3) return;

        val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);

        switch (id)
        {
        case OSD_LAYER1:
                pos = 12;
                break;
        case OSD_LAYER2:
                pos = 14;
                break;
        case OSD_LAYER3:
                pos = 16;
                break;
        default:
                return;
        }

        if (enable)
                val |= 1 << pos;
        else
                val &= ~(1 << pos);
        writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
}

/* ark1668 fix (2026-07-25): real vendor ARKFB_SET_BLEND ioctl (0x40104f29,
 * confirmed via ARM disassembly of stock's real ark_fb_set_blend() in
 * vmlinux.elf -- see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 65)
 * was entirely unhandled by this driver. libarkcmn.so's real callers were
 * never traced directly, but stock's kernel only ever calls the
 * per-layer alpha_blend_en/per_pix_alpha_blend_en/blend_mode/alpha
 * setters below (which already existed for OSD, added here for VIDEO)
 * from inside this exact ioctl and a debug-only sscanf interface --
 * meaning these fields are never set anywhere else in stock's own boot
 * path (U-Boot doesn't touch them for VIDEO at all, confirmed via its
 * real source), so this ioctl not existing meant VIDEO's blend config
 * was stuck at raw hardware-reset default on our reconstruction,
 * unlike stock which (if this ioctl is genuinely called by real
 * userspace) ends up at a confirmed-live value of alpha_blend_en=0,
 * per_pix_alpha_blend_en=0 (use fixed layer alpha, not pixel alpha --
 * video frame data has no meaningful alpha channel), blend_mode=0,
 * alpha=0xff (fully opaque) -- i.e. VIDEO drawn as a simple, always-
 * opaque layer with no blending consideration at all. Below are the
 * missing primitives; the ioctl handler itself is further down in
 * ark1668_lcdfb_ioctl(). */

static void ark1668_lcdc_set_video_alpha_blend_en_lcd(int id, int enable)
{
        unsigned int pos;
        unsigned int val;

        if (id < 0 || id > VIDEO_LAYER2) return;

        val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);

        /* video=bit11, video2=bit9 -- confirmed via disassembly, NOT a
         * simple id*2+9 pattern (video2 is lower, not higher, than video). */
        pos = (id == VIDEO_LAYER1) ? 11 : 9;

        if (enable)
                val |= 1 << pos;
        else
                val &= ~(1 << pos);

        writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
}

static void ark1668_lcdc_set_video_per_pix_alpha_blend_en_lcd(int id, int enable)
{
        unsigned int pos;
        unsigned int val;

        if (id < 0 || id > VIDEO_LAYER2) return;

        val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);

        /* video=bit10, video2=bit8 -- confirmed via disassembly. */
        pos = (id == VIDEO_LAYER1) ? 10 : 8;

        if (enable)
                val |= 1 << pos;
        else
                val &= ~(1 << pos);

        writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
}

static void ark1668_lcdc_set_osd_blend_mode(int id, unsigned int mode)
{
        unsigned int pos;
        unsigned int val;

        switch (id)
        {
        case OSD_LAYER1:
                pos = 12;
                break;
        case OSD_LAYER2:
                pos = 20;
                break;
        case OSD_LAYER3:
                pos = 28;
                break;
        default:
                return;
        }

        val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
        val &= ~(0xf << pos);
        val |= (mode & 0xf) << pos;
        writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
}

static void ark1668_lcdc_set_video_blend_mode(int id, unsigned int mode)
{
        unsigned int val;

        if (id < 0 || id > VIDEO_LAYER2) return;

        /* video's blend_mode lives in MODE_LCD_REG0 bits[7:4]; video2's
         * lives in MODE_LCD_REG1 bits[7:4] -- a different register per
         * layer, confirmed via disassembly, not just a different bit
         * offset within the same register like every other field here. */
        if (id == VIDEO_LAYER1) {
                val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
                val &= ~(0xf << 4);
                val |= (mode & 0xf) << 4;
                writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG0);
        } else {
                val = readl(lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
                val &= ~(0xf << 4);
                val |= (mode & 0xf) << 4;
                writel(val, lcdc_base + ARK1668_LCDC_MODE_LCD_REG1);
        }
}

static void ark1668_lcdc_set_osd_alpha(int id, unsigned int alpha)
{
        unsigned int reg;
        unsigned int val;

        switch (id)
        {
        case OSD_LAYER1:
                reg = ARK1668_LCDC_OSD1_CTL;
                break;
        case OSD_LAYER2:
                reg = ARK1668_LCDC_OSD2_CTL;
                break;
        case OSD_LAYER3:
                reg = ARK1668_LCDC_OSD3_CTL;
                break;
        default:
                return;
        }

        val = readl(lcdc_base + reg);
        val &= ~0xff;
        val |= alpha & 0xff;
        writel(val, lcdc_base + reg);
}

static void ark1668_lcdc_set_video_alpha(int id, unsigned int alpha)
{
        unsigned int val;

        if (id < 0 || id > VIDEO_LAYER2) return;

        val = readl(lcdc_base + ARK1668_LCDC_VIDEO_VIDEO2_BLD_COEF);
        if (id == VIDEO_LAYER1) {
                val &= ~0xff;
                val |= alpha & 0xff;
        } else {
                val &= ~(0xff << 8);
                val |= (alpha & 0xff) << 8;
        }
        writel(val, lcdc_base + ARK1668_LCDC_VIDEO_VIDEO2_BLD_COEF);
}


int ark_bootanimation_display_init(int width, int height, unsigned int addr)
{
	ark1668_lcdc_set_osd_size(OSD_LAYER2, width, height);
	ark1668_lcdc_set_osd_pos(OSD_LAYER2, (lcdc_width - width) / 2, (lcdc_height - height) / 2);
	ark1668_lcdc_set_osd_format(OSD_LAYER2, ARK1668_LCDC_FORMAT_VYUY, ARK_LCDC_ORDER_UYVY, 0);
	ark1668_lcdc_set_osd_addr(OSD_LAYER2, addr);
	ark1668_lcdc_set_osd_en(OSD_LAYER2, 1);

	return 0;
}
EXPORT_SYMBOL(ark_bootanimation_display_init);

int ark_bootanimation_display_uninit(void)
{
	ark1668_lcdc_set_osd_en(OSD_LAYER2, 0);
	return 0;
}
EXPORT_SYMBOL(ark_bootanimation_display_uninit);

int ark_bootanimation_set_display_addr(unsigned int addr)
{
	ark1668_lcdc_set_osd_addr(OSD_LAYER2, addr);
	ark1668_lcdc_wait_for_vsync();

	return 0;
}
EXPORT_SYMBOL(ark_bootanimation_set_display_addr);

int ark_itu656_display_init(int src_width, int src_height,int out_posx, int out_posy,int out_width, int out_height,int interlace)
{
        int win_x, win_y, win_width, win_height;
        
        if(lcdc_base == NULL)
        return -1;

        if(interlace){
                win_x = 0;
                win_y = 2;
                win_width = src_width - 12;
                win_height = src_height - win_y -4;
        }else{
                win_x = 2;
                win_y = 2;
                win_width = src_width - win_x -2;
                win_height = src_height;
        }
        ark1668_lcdc_set_video_source_size(VIDEO_LAYER2, src_width, src_height);
        ark1668_lcdc_set_video_win_size(VIDEO_LAYER2, win_width, win_height);
        ark1668_lcdc_set_video_win_point(VIDEO_LAYER2, 0, 0);
        ark1668_lcdc_set_video_layer_pos(VIDEO_LAYER2, out_posx, out_posy);    
        ark1668_lcdc_set_video_layer_size(VIDEO_LAYER2, out_width, out_height);
        ark1668_lcdc_set_video_format(VIDEO_LAYER2, ARK_LCDC_FORMAT_VYUY, 0 ,ARK_LCDC_ORDER_VYUY, 0);
        ark1668_lcdc_set_video_scal(VIDEO_LAYER2, win_width, win_height, 0, 0, 0, 0, out_width, out_height, 0, 0);  

        return 0;
}
EXPORT_SYMBOL(ark_itu656_display_init);

int ark_itu656_display_addr(unsigned int addr)
{
        if(lcdc_base == NULL || addr == 0){
                return -1;
        }
    
	ark1668_lcdc_set_video_addr(VIDEO_LAYER2, addr, 0, 0);

	return 0;
}
EXPORT_SYMBOL(ark_itu656_display_addr);

int ark_itu656_display_uninit(void)
{
        if(lcdc_base == NULL){
                return -1;
        }

        ark1668_lcdc_set_video_en(VIDEO_LAYER2, 0);
    
	return 0;
}
EXPORT_SYMBOL(ark_itu656_display_uninit);


int ark_disp_set_layer_en(int layer_id, int enable)
{
        if(lcdc_base == NULL){
                return -1;
        }

        if(layer_id > 4 || layer_id < 0){
                return -1;
        }

        if(layer_id < 3){

                ark1668_lcdc_set_osd_en(layer_id, enable);
        }else{

                ark1668_lcdc_set_video_en(layer_id-3, enable);
        }
    
	return 0;
}
EXPORT_SYMBOL(ark_disp_set_layer_en);

int ark168vin_set_scal(int layer,int src_w, int src_h,int out_w,int out_h)
{
    ark1668_lcdc_set_video_scal(layer-OSD_LAYER_MAX, src_w, src_h, 0, 0, 0, 0, out_w, out_h, 0, 0);
    return 0;
}
EXPORT_SYMBOL(ark168vin_set_scal);

int ark168vin_set_display(int layer, unsigned int cmd, void *arg)
{
	unsigned long error = 0;
	switch (cmd) {
        case VIN_SHOW_WINDOW:
		{
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_en(layer, 1);
                else
                        ark1668_lcdc_set_video_en(layer-OSD_LAYER_MAX, 1);

                printk(KERN_DEBUG "layer=%d: show window.\n ",layer);
                break;
        }
        case VIN_HIDE_WINDOW:
		{
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_en(layer, 0);
                else
                        ark1668_lcdc_set_video_en(layer-OSD_LAYER_MAX, 0);

                printk(KERN_DEBUG "layer=%d: hide window.\n ",layer);
                break;
        }
        case VIN_SET_WINDOW_POS:
        {
                unsigned int x,y,data;
                data = *(unsigned int*)arg;
                x =  data & 0xFFFF;
                y = (data >> 16) & 0xFFFF;
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_pos(layer, x, y);
                else
                        ark1668_lcdc_set_video_layer_pos(layer-OSD_LAYER_MAX, x, y);

               	printk(KERN_DEBUG "layer=%d: x=%d, y=%d.\n ",layer, x, y);   
				break;
        }
            
        case VIN_SET_SOURCE_SIZE:
        {
                unsigned int width, height, data;
                data = *(unsigned int*)arg;
                width  = data & 0xFFFF;
                height = (data >> 16) & 0xFFFF;
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_size(layer, width, height);
                else{
                        ark1668_lcdc_set_video_source_size(layer-OSD_LAYER_MAX, width, height);
                        ark1668_lcdc_set_video_win_size(layer-OSD_LAYER_MAX, width, height);
                        ark1668_lcdc_set_video_win_point(layer-OSD_LAYER_MAX, 0, 0);
                        ark1668_lcdc_set_video_layer_size(layer-OSD_LAYER_MAX, width, height);
			ark1668_lcdc_set_video_scal(layer-OSD_LAYER_MAX, width, height,
				                              0, 0, 0, 0, width, height, 0, 0);
                }

                printk(KERN_DEBUG "layer=%d: width=%d, height=%d.\n ",layer, width, height);
				break;
        }
		
		case VIN_SET_LAYER_SIZE:
        {
                unsigned int width, height, data;
                data = *(unsigned int*)arg;
                width  = data & 0xFFFF;
                height = (data >> 16) & 0xFFFF;
                ark1668_lcdc_set_video_layer_size(layer-OSD_LAYER_MAX, width, height);
				break;
        }
            
        case VIN_SET_WINDOW_FORMAT:
        {
                unsigned int data, format, yuv_order, rgb_order;
                data = *(unsigned int*)arg;
                format    = (data >> 0)  & 0xFF;
                yuv_order = (data >> 16) & 0xF;
                rgb_order = (data >> 24) & 0xF;
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_format(layer, format, yuv_order, rgb_order);
                else
                        ark1668_lcdc_set_video_format(layer-OSD_LAYER_MAX, format, yuv_order, rgb_order, 0);

                if(layer == OSD_LAYER3){
                        ark1668_lcdc_set_osd_alpha_blend_en_lcd(layer, 1);
                        ark1668_lcdc_set_osd_per_pix_alpha_blend_en_lcd(layer, 1);
                }

                printk(KERN_DEBUG "layer=%d: format=%d.\n ",layer, format);      
				break;
        }
            
        case VIN_SET_WINDOW_ADDR:
        {
				struct ark_disp_addr* addr = (struct ark_disp_addr*)arg;
				//printk(KERN_ALERT "VIN_SET_WINDOW_ADDR :layer=%d: yaddr=0x%0x, cbaddr=0x%0x, craddr=0x%0x.\n ",layer, addr->yaddr, addr->cbaddr, addr->craddr);
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_addr(layer, addr->yaddr);
                else
                        ark1668_lcdc_set_video_addr(layer-OSD_LAYER_MAX, addr->yaddr, addr->cbaddr, addr->craddr);

                break;
        }
            
        case VIN_SET_WINDOW_SCALER:
        {
                struct ark_disp_scaler* scaler = (struct ark_disp_scaler*)arg;
                if(layer <= OSD_LAYER3){
                        error = -EINVAL;
                        printk("%s: layer<=OSD_LAYER3 can not scaler\n", __func__);
                        goto end;
                }
                ark1668_lcdc_set_video_scal(layer-OSD_LAYER_MAX, scaler->src_w, scaler->src_h, 0, 0, 0, 0, scaler->out_w, scaler->out_h, 0, 0);
                printk(KERN_DEBUG "layer=%d: scaler src_w=%d, src_h=%d, out_w=%d, out_h=%d.\n ",layer, scaler->src_w, scaler->src_h, scaler->out_w, scaler->out_h);
				break;
        }
		
        default:
            printk("%s %d: unknown ioctl %08x\n",__FUNCTION__, __LINE__, cmd);
            break;   
        }
    return 0;
end:    
    return error;
}
EXPORT_SYMBOL(ark168vin_set_display);

void ark1668_lcdc_display_update_atomic(struct ark1668_lcdfb_info* sinfo)
{
        unsigned int format, yuv_order, rgb_order, i, layer;
        struct ark_disp_atomic* p;

        if(!sinfo->atomic_flag)
                return;

        for(i = 0; i < ARK1668_LAYER_MAX;i++){
                if(!(sinfo->atomic_flag & (1 << i)))
                        continue;
                
                p = &sinfo->patomic[i];
                if(!p->atomic_stat || p->layer < 0 || p->layer > 4){
                        sinfo->atomic_flag &= ~(1 << i);
                        memset(&sinfo->patomic[i], 0 ,sizeof(struct ark_disp_atomic));
                        continue;
                }

                //printk(KERN_ALERT "%s: atomic_stat=0x%0x, layer=%d.\n ",__func__, p->atomic_stat, p->layer);
                if(p->layer >= OSD_LAYER1 && p->layer <= OSD_LAYER3){
                        layer = p->layer;
                        if(p->atomic_stat & ATOMIC_SET_LAYER_POS)
                                ark1668_lcdc_set_osd_pos(layer, p->pos_x, p->pos_y);
                        if(p->atomic_stat & ATOMIC_SET_LAYER_SIZE)
                                ark1668_lcdc_set_osd_size(layer, p->width, p->height);
                        if(p->atomic_stat & ATOMIC_SET_LAYER_FMT){
                                format    = (p->format >> 0)  & 0xFF;
                                yuv_order = (p->format >> 16) & 0xF;
                                rgb_order = (p->format >> 24) & 0xF;
                                ark1668_lcdc_set_osd_format(layer, format, yuv_order, rgb_order);
                                if(format == ARK1668_LCDC_FORMAT_RGBA888){
                                        ark1668_lcdc_set_osd_alpha_blend_en_lcd(layer, 1);
                                        ark1668_lcdc_set_osd_per_pix_alpha_blend_en_lcd(layer, 1);
                                }
                                //printk(KERN_ALERT "%s: format=%d, yuv_order=%d, rgb_order=%d.\n ",__func__, format, yuv_order,rgb_order);
                        }
                        if(p->atomic_stat & ATOMIC_SET_LAYER_ADDR)
                                ark1668_lcdc_set_osd_addr(layer, p->addr.yaddr);
                }else{
                        layer = p->layer-OSD_LAYER_MAX;
                        if(p->atomic_stat & ATOMIC_SET_LAYER_POS)
                                ark1668_lcdc_set_video_layer_pos(layer, p->pos_x, p->pos_y);
                        if(p->atomic_stat & ATOMIC_SET_LAYER_SIZE){
                                ark1668_lcdc_set_video_source_size(layer, p->width, p->height);
                                ark1668_lcdc_set_video_win_size(layer, p->width, p->height);
                                ark1668_lcdc_set_video_win_point(layer, 0, 0);
                                ark1668_lcdc_set_video_layer_size(layer, p->width, p->height);
				if(!(p->atomic_stat & ATOMIC_SET_LAYER_SCALER))
					ark1668_lcdc_set_video_scal(layer, p->width, p->height,
					                       0, 0, 0, 0, p->width, p->height, 0, 0);
                        }
                        if(p->atomic_stat & ATOMIC_SET_LAYER_FMT){
                                format    = (p->format >> 0)  & 0xFF;
                                yuv_order = (p->format >> 16) & 0xF;
                                rgb_order = (p->format >> 24) & 0xF;
                                ark1668_lcdc_set_video_format(layer, format, yuv_order, rgb_order, 0);
                        }
                        if(p->atomic_stat & ATOMIC_SET_LAYER_ADDR)
                                ark1668_lcdc_set_video_addr(layer, p->addr.yaddr, p->addr.cbaddr, p->addr.craddr);
                        if(p->atomic_stat & ATOMIC_SET_LAYER_SCALER)
                                ark1668_lcdc_set_video_scal(layer, p->scaler.src_w, p->scaler.src_h,
                                                       0, 0, 0, 0, p->scaler.out_w, p->scaler.out_h, 0, 0);
                }

                sinfo->atomic_flag &= ~(1 << i);
                memset(&sinfo->patomic[i], 0 ,sizeof(struct ark_disp_atomic));

        }


}


int ark1668_lcdc_wait_for_vsync(void)
{
        struct ark1668_lcdfb_info *sinfo = lcdfb_info;
        int ret;

        if(!sinfo) return -EINVAL;
        
	sinfo->vsync_flag = 0;
	ret = wait_event_interruptible_timeout(sinfo->vsync_waitq,
			sinfo->vsync_flag != 0,
			msecs_to_jiffies(100)); // 100ms at most
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -ETIMEDOUT;

        
        if(sinfo->atomic_flag)
                ark1668_lcdc_display_update_atomic(sinfo);

	return 0;
}
EXPORT_SYMBOL(ark1668_lcdc_wait_for_vsync);

int ark1668_lcdfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct ark1668_lcdfb_info *sinfo = info->par;
	unsigned long error = 0;
        int layer = info->node;
    
	switch (cmd) {
		case FBIO_WAITFORVSYNC:
        case ARKFB_WAITFORVSYNC:
                ark1668_lcdc_wait_for_vsync();
                //printk(KERN_DEBUG "ark1668 lcdc wait for vsync.\n ");
            break;
            
        case ARKFB_SHOW_WINDOW:
        case ARKFB_SHOW_WINDOW_REAL:

                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_en(layer, 1);
                else
                        ark1668_lcdc_set_video_en(layer-OSD_LAYER_MAX, 1);

                printk(KERN_DEBUG "layer=%d: show window.\n ",layer);
                break;
            
        case ARKFB_HIDE_WINDOW:
        case ARKFB_HIDE_WINDOW_REAL:
                /* ARKFB_HIDE_WINDOW_REAL (real vendor ioctl 0x4f2c) is stock's
                 * actual HIDE_WINDOW number; stock gates it on carback (reversing
                 * camera) status, refusing to hide while carback is active. We
                 * have no CONFIG_ARK_CARBACK support built in (see .config), so
                 * always hide -- matches stock behavior for the non-carback case,
                 * which is the only case this build can hit anyway.
                 */
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_en(layer, 0);
                else
                        ark1668_lcdc_set_video_en(layer-OSD_LAYER_MAX, 0);

                printk(KERN_DEBUG "layer=%d: hide window.\n ",layer);
                break;
            
        case ARKFB_SET_WINDOW_POS:
        {
                unsigned int x,y,data;
                if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                x =  data & 0xFFFF;
                y = (data >> 16) & 0xFFFF;
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_pos(layer, x, y);
                else
                        ark1668_lcdc_set_video_layer_pos(layer-OSD_LAYER_MAX, x, y);

                printk(KERN_DEBUG "layer=%d: x=%d, y=%d.\n ",layer, x, y);                
        }
        break;
            
        case ARKFB_SET_WINDOW_SIZE:
        {
                unsigned int width, height, data;
                if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                width  = data & 0xFFFF;
                height = (data >> 16) & 0xFFFF;
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_size(layer, width, height);
                else{
                        ark1668_lcdc_set_video_source_size(layer-OSD_LAYER_MAX, width, height);
                        ark1668_lcdc_set_video_win_size(layer-OSD_LAYER_MAX, width, height);
                        ark1668_lcdc_set_video_win_point(layer-OSD_LAYER_MAX, 0, 0);
                        ark1668_lcdc_set_video_layer_size(layer-OSD_LAYER_MAX, width, height);
			ark1668_lcdc_set_video_scal(layer-OSD_LAYER_MAX, width, height,
				                              0, 0, 0, 0, width, height, 0, 0);
                }

                printk(KERN_DEBUG "layer=%d: width=%d, height=%d.\n ",layer, width, height);
        }
        break;
            
        case ARKFB_SET_WINDOW_FORMAT:
        {
                unsigned int data, format, yuv_order, rgb_order;
                if(copy_from_user(&data, (void *)arg, sizeof(unsigned int))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }

                format    = (data >> 0)  & 0xFF;
                yuv_order = (data >> 16) & 0xF;
                rgb_order = (data >> 24) & 0xF;
                if(layer <= OSD_LAYER3)
                        ark1668_lcdc_set_osd_format(layer, format, yuv_order, rgb_order);
                else
                        ark1668_lcdc_set_video_format(layer-OSD_LAYER_MAX, format, yuv_order, rgb_order, 0);

                if(layer == OSD_LAYER3){
                        ark1668_lcdc_set_osd_alpha_blend_en_lcd(layer, 1);
                        ark1668_lcdc_set_osd_per_pix_alpha_blend_en_lcd(layer, 1);
                }

                printk(KERN_DEBUG "layer=%d: format=%d.\n ",layer, format);                
        }
        break;
            
        case ARKFB_SET_WINDOW_ADDR:
        {
                struct ark_disp_addr addr;
                if(copy_from_user(&addr, (void *)arg, sizeof(struct ark_disp_addr))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
				memcpy(&sinfo->render_addr[layer], &addr, sizeof(struct ark_disp_addr));

                //printk(KERN_ALERT "layer=%d: yaddr=0x%0x, cbaddr=0x%0x, craddr=0x%0x.\n ",layer, addr.yaddr, addr.cbaddr, addr.craddr);
        }
        break;
            
        case ARKFB_SET_WINDOW_SCALER:
        {
                struct ark_disp_scaler scaler;
                if(layer <= OSD_LAYER3){
                        error = -EINVAL;
                        printk("%s: layer<=OSD_LAYER3 can not scaler\n", __func__);
                        goto end;
                }
                if(copy_from_user(&scaler, (void *)arg, sizeof(struct ark_disp_scaler))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                ark1668_lcdc_set_video_scal(layer-OSD_LAYER_MAX, scaler.src_w, scaler.src_h, 0, 0, 0, 0, scaler.out_w, scaler.out_h, 0, 0);
                printk(KERN_DEBUG "layer=%d: scaler src_w=%d, src_h=%d, out_w=%d, out_h=%d.\n ",layer, scaler.src_w, scaler.src_h, scaler.out_w, scaler.out_h);
        }
        break;

		case ARKFB_SET_WINDOW_PRIORITY:
        {
			unsigned int layer_id[5];
				
			if(copy_from_user(&layer_id, (void*)arg, sizeof(unsigned int)*5)){
				printk(KERN_ALERT "ARKFB_SET_WINDOW_PRIORITY error\n");
				return -EFAULT;
			}

			printk(KERN_ALERT "++++++ARKFB_SET_WINDOW_PRIORITY layer_id: %d.%d,%d,%d,%d\n",layer_id[0],layer_id[1],layer_id[2],layer_id[3],layer_id[4]);

			ark1668_lcdc_set_video_priority(layer_id[0]);
			ark1668_lcdc_set_video2_priority(layer_id[1]);
			ark1668_lcdc_set_win1_priority(layer_id[2]);
			ark1668_lcdc_set_win2_priority(layer_id[3]);
			ark1668_lcdc_set_win3_priority(layer_id[4]);
        }
        break;
        
        case ARKFB_SET_WINDOW_ATOMIC:
        {
                struct ark_disp_atomic atomic;
                
                if(copy_from_user(&atomic, (void *)arg, sizeof(struct ark_disp_atomic))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }

                if(!atomic.atomic_stat || atomic.layer != layer){
                        printk("%s: atomic_stat or layer  error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                
                printk(KERN_DEBUG "%s===>layer=%d, atomic_stat=0x%0x.\n ",__func__, layer, atomic.atomic_stat);
                sinfo->atomic_flag |= (1 << layer);
                memcpy(&sinfo->patomic[layer], &atomic, sizeof(struct ark_disp_atomic));
                ark1668_lcdc_wait_for_vsync();
        }
        break;
        
        case ARKFB_SET_REG_VALUE:
        {
                struct ark_disp_reg reg;
                
                if(copy_from_user(&reg, (void *)arg, sizeof(struct ark_disp_reg))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                
                if((reg.addr & 0xffff0000) == 0xe0500000){        
                        writel(reg.value, lcdc_base + (reg.addr&0xffff));
                        printk("arkfb write reg:0x%0x=0x%0x.\n ", reg.addr, reg.value);
                }else{
                        error = -EINVAL;
                        goto end;
                }
        }
        break;
        
        case ARKFB_GET_REG_VALUE:
        {
                struct ark_disp_reg reg;
                
                if(copy_from_user(&reg, (void *)arg, sizeof(struct ark_disp_reg))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                
                if((reg.addr & 0xffff0000) == 0xe0500000){        
                        reg.value = readl(lcdc_base + (reg.addr&0xffff));
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
        }
        break;

        case ARKFB_GET_WINDOW_ADDR:
        {
                struct ark_disp_addr addr;
                memset(&addr, 0, sizeof(struct ark_disp_addr));

                if(layer <= OSD_LAYER3)
                        addr.yaddr = ark1668_lcdc_get_osd_addr(layer);
                else
                        ark1668_lcdc_get_video_addr(layer-OSD_LAYER_MAX, &addr.yaddr, &addr.cbaddr, &addr.craddr);

                if(copy_to_user((void  *)arg, &addr, sizeof(struct ark_disp_addr))){
                        printk("%s: copy to user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                //printk(KERN_DEBUG "%s: layer=%d: yaddr=0x%0x, cbaddr=0x%0x, craddr=0x%0x.\n ",__func__, layer, addr.yaddr, addr.cbaddr, addr.craddr);
        }
        break;
        
        case ARKFB_GET_SCREEN_INFO:
        {
                struct ark_screen screen;
                
                memset(&screen, 0, sizeof(struct ark_screen));
                screen.width  = screen.disp_width  = lcdc_width;
                screen.height = screen.disp_height = lcdc_height;
                if(copy_to_user((void  *)arg, &screen, sizeof(struct ark_screen))){
                        printk("%s: copy to user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                //printk(KERN_ALERT "%s: type=%d, width=%d, height=%d.\n ", __func__, screen.type, screen.width,screen.height);

        }
        break;
        
        case ARKFB_SET_SCREEN_INFO:
        {
                struct ark_screen screen;
                if(copy_from_user(&screen, (void  *)arg, sizeof(struct ark_screen))){
                        printk("%s: copy to user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }
                ///////////////// Reserved///////////////////

        }
        break;
        
        case ARKFB_GET_PLATFORM_INFO:
        {
                struct ark_platform_info platform;
                
                memset(&platform, 0, sizeof(struct ark_platform_info));
                platform.type = ARK_PLATFORM_ARK1668;
                if(copy_to_user((void  *)arg, &platform, sizeof(struct ark_platform_info))){
                        printk("%s: copy to user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }

        }
        break;

        case ARKFB_INIT_DISPLAY:
        case ARKFB_INIT_VIDEO_DISPLAY:
        {
                /* Real init call from libarkcmn.so's arkapi_init_fb_display()/
                 * arkapi_init_fb_video_display(), see ark_fb_init_display
                 * comment in ark_lcdc_common.h. Drive it through the same
                 * position/size/scaler primitives ARKFB_SET_WINDOW_POS and
                 * ARKFB_SET_WINDOW_SIZE already use.
                 */
                struct ark_fb_init_display init;

                if(copy_from_user(&init, (void *)arg, sizeof(init))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }

                if(init.win_width == 0 || init.win_height == 0){
                        printk("%s: ARKFB_INIT_DISPLAY zero window size\n", __func__);
                        error = -EINVAL;
                        goto end;
                }

                if(layer <= OSD_LAYER3){
                        /* Real gap found 2026-07-24: this is the ONLY init-time
                         * hook that actually runs for OSD2/OSD3 (/dev/fb1,
                         * /dev/fb2) -- ark1668_lcdfb_probe()'s registration loop
                         * for fb1-fb4 memcpy's fb0's struct fb_info and calls
                         * register_framebuffer() directly, never fb_set_par(),
                         * so the OSD1-specific rgb_order fix in
                         * ark1668_lcdfb_set_par() (see that file) never runs
                         * for these layers at all. Set format+rgb_order here
                         * too, matching the same wiring_mode derivation, so
                         * every OSD layer that goes through this real call path
                         * (libarkcmn.so's arkapi_init_fb_display()) ends up
                         * correctly configured regardless of which /dev/fbN it
                         * targets. RGBA888 is the only format OSD layers (UI
                         * overlay content) are ever used with -- unlike
                         * VIDEO_LAYER1/2 below, whose format is genuinely
                         * content-dependent (RGB or YUV depending on the
                         * decoder), so deliberately NOT touched here. See
                         * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 36.
                         */
                        /* FIXED AGAIN 2026-07-25 (checklist section 66):
                         * rgb_order hardcoded to 0, matching U-Boot's real
                         * bootlogo -- see ark1668_lcdfb.c's matching comment.
                         * Not derived from lcd_wiring_mode at all anymore.
                         */
                        ark1668_lcdc_set_osd_format(layer, ARK1668_LCDC_FORMAT_RGBA888,
                                                     0, 0);
                        ark1668_lcdc_set_osd_pos(layer, init.x, init.y);
                        ark1668_lcdc_set_osd_size(layer, init.win_width, init.win_height);
                }else{
                        int vlayer = layer - OSD_LAYER_MAX;

                        ark1668_lcdc_set_video_layer_pos(vlayer, init.x, init.y);
                        ark1668_lcdc_set_video_source_size(vlayer, init.win_width, init.win_height);
                        ark1668_lcdc_set_video_win_size(vlayer, init.win_width, init.win_height);
                        ark1668_lcdc_set_video_win_point(vlayer, 0, 0);
                        ark1668_lcdc_set_video_layer_size(vlayer, init.win_width, init.win_height);
                        ark1668_lcdc_set_video_scal(vlayer, init.win_width, init.win_height,
                                                     0, 0, 0, 0, init.win_width, init.win_height, 0, 0);

                        /* ark1668 fix (2026-07-25): apply stock's confirmed-live
                         * blend config as a static default here too, not just
                         * via the new ARKFB_SET_BLEND ioctl handler -- U-Boot
                         * never touches VIDEO's alpha_blend_en/per_pix_alpha_blend_en
                         * bits at all (confirmed absent from its real source),
                         * so without this, VIDEO_LAYER's blend state would be
                         * raw hardware-reset default (unknown, possibly wrong)
                         * unless userspace happens to call ARKFB_SET_BLEND
                         * itself, which was never confirmed either way. This
                         * belt-and-braces default matches stock's real live
                         * MODE_LCD_REG1 dump exactly: alpha_blend_en=0,
                         * per_pix_alpha_blend_en=0 (fixed layer alpha, not
                         * pixel alpha -- decoded video frame data has no
                         * meaningful alpha channel), blend_mode=0, alpha=0xff
                         * (fully opaque) -- VIDEO drawn as a simple, always-
                         * visible layer with no blending consideration. See
                         * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 65.
                         */
                        ark1668_lcdc_set_video_alpha_blend_en_lcd(vlayer, 0);
                        ark1668_lcdc_set_video_per_pix_alpha_blend_en_lcd(vlayer, 0);
                        ark1668_lcdc_set_video_blend_mode(vlayer, 0);
                        ark1668_lcdc_set_video_alpha(vlayer, 0xff);
                }

                printk(KERN_DEBUG "layer=%d: init display x=%d, y=%d, w=%d, h=%d.\n ",
                       layer, init.x, init.y, init.win_width, init.win_height);
        }
        break;

        case ARKFB_SET_VIDEO_ADDR_RAW:
        {
                /* Real per-frame update from libarkcmn.so's
                 * arkapi_set_fb_video_addr(), see ark_fb_set_video_addr
                 * comment in ark_lcdc_common.h. Stock queues this through an
                 * IRQ-driven wait-queue buffer pipeline (ark_video_update_window
                 * in vmlinux.elf); we don't replicate that private kernel-side
                 * machinery -- only the userspace ioctl ABI has to match. Write
                 * the address directly so the latest frame is shown as soon as
                 * possible, which is the right tradeoff for a live video feed.
                 */
                struct ark_fb_set_video_addr vaddr;
                unsigned long flags;

                if(layer <= OSD_LAYER3){
                        printk("%s: ARKFB_SET_VIDEO_ADDR_RAW on non-video layer\n", __func__);
                        error = -EINVAL;
                        goto end;
                }

                if(copy_from_user(&vaddr, (void *)arg, sizeof(vaddr))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }

                spin_lock_irqsave(&sinfo->lock, flags);
                ark1668_lcdc_set_video_addr(layer - OSD_LAYER_MAX, vaddr.y_addr, vaddr.cb_addr, vaddr.cr_addr);
                spin_unlock_irqrestore(&sinfo->lock, flags);
        }
        break;

        case ARKFB_SET_BLEND:
        {
                /* Real vendor ioctl, previously entirely unhandled -- see
                 * the block comment above ark1668_lcdc_set_video_alpha_blend_en_lcd()
                 * earlier in this file for the full trace and why this
                 * matters for VIDEO_LAYER (CarPlay/Android Auto video)
                 * specifically. Target layer is whichever /dev/fbN this
                 * fd was opened against, same as every other ioctl here.
                 */
                struct ark_fb_blend blend;

                if(copy_from_user(&blend, (void *)arg, sizeof(blend))){
                        printk("%s: copy from user para error\n", __func__);
                        error = -EFAULT;
                        goto end;
                }

                if(layer <= OSD_LAYER3){
                        ark1668_lcdc_set_osd_alpha_blend_en_lcd(layer, blend.alpha_blend_en);
                        ark1668_lcdc_set_osd_per_pix_alpha_blend_en_lcd(layer, blend.per_pix_alpha_blend_en);
                        ark1668_lcdc_set_osd_blend_mode(layer, blend.blend_mode);
                        ark1668_lcdc_set_osd_alpha(layer, blend.alpha);
                }else{
                        int vlayer = layer - OSD_LAYER_MAX;

                        ark1668_lcdc_set_video_alpha_blend_en_lcd(vlayer, blend.alpha_blend_en);
                        ark1668_lcdc_set_video_per_pix_alpha_blend_en_lcd(vlayer, blend.per_pix_alpha_blend_en);
                        ark1668_lcdc_set_video_blend_mode(vlayer, blend.blend_mode);
                        ark1668_lcdc_set_video_alpha(vlayer, blend.alpha);
                }
        }
        break;

        default:
            printk("%s %d: unknown ioctl %08x\n",__FUNCTION__, __LINE__, cmd);
            break;
    }

end:
        
    return error;
}

