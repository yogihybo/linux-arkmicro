#define pr_fmt(fmt) "ark1668-itu656: " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <asm/setup.h>
#include <linux/dma-mapping.h>
#include <linux/ktime.h>
#include <linux/ratelimit.h>
#include "ark1668_itu656.h"


#define PAL		0
#define NTSC	        1

#define BLOCK_HEIGHT    32
#define ITU656_MAX_FRAME    4
#define PAL_WIDTH	720
#define PAL_HEIGHT	288
#define NTSC_WIDTH	720
#define NTSC_HEIGHT	240
#define CVBS_WIDTH 	PAL_WIDTH
#define CVBS_HEIGHT     PAL_HEIGHT

#define DEINTERLACE_MAX_FRAME    8
#define DEINTERLACE_WIDTH  720
#define DEINTERLACE_HEIGHT 576
#define DVR_MAJOR  243

#define ITU656_BUFFER_EMPTY			0x0 	// buffer is readed by lcd/tv, ITU656 can write
#define ITU656_BUFFER_FULL_LCD		0x1 	// buffer is writed ok by itu656. lcd can read
#define ITU656_BUFFER_FULL_TV 		0x10 	//  buffer is writed ok by itu656. tv can read
#define ITU656_BUFFER_FULL_APP		0x20	// buffer is writed ok by itu656. app can read
#define ITU656_BUFFER_FULL_MIRROR	0x40	// buffer is writed ok by itu656. mirror can read
#define DISCARD_FRAME_SYS_CHANGE		5

#define ITU656_FIELD_SIZE			(CVBS_WIDTH*CVBS_HEIGHT*2)
#define DISPLAY_WIDTH				800
#define DISPLAY_HEIGHT				480
#define ITU656_FRAME_SIZE			(ITU656_FIELD_SIZE*2)
#define ITU656_PROGRESSIVE_FRAME_SIZE	        (1280*720*2)
#define ITU656_PROGRESSIVE_FRAME_SIZE_1080P	(1920*1080*2)

#define ARK_IOW(num, dtype)		_IOW('O', num, dtype)
#define ARK_IOR(num, dtype)		_IOR('O', num, dtype)
#define ARK_IOWR(num, dtype)	        _IOWR('O', num, dtype)
#define ARK_IO(num)			_IO('O', num)

#define ARKFB_SHOW_WINDOW	    	ARK_IO(43)
#define ARKFB_HIDE_WINDOW	    	ARK_IO(44)
#define ARKFB_UPDATE_WINDOW		ARK_IOW(39, struct arkfb_update_window)
#define ARKFB_UPDATE_VIDEO_WINDOW 	ARK_IOW(55, struct arkfb_update_window)
#define ARKFB_SET_VIDEO_WINDOW_ADDR     ARK_IOW(56, struct arkfb_window_addr)

#define ARKDISP_IOCTL_BASE		0xa0
#define ARKDISP_SET_TVENC_CFG           _IOW(ARKDISP_IOCTL_BASE, 10, unsigned long)

#define ARK_DISP_VIDEO_PIXFMT_YUYV     	2
#define ARKPRESCAL_WORKMODE_MIX      	3
#define ARKPRESCAL_HFORMAT_YUV422	3
#define DISPLAY_LAYER			4
#define TVOUT_LAYER			3
#define DISPLAY_FB_PATH			"/dev/fb4"
#define TVOUT_FB_PATH			"/dev/fb3"
#define ITU656_USE_DEINTERLACE

extern int ark_itu656_display_init(int src_width, int src_height,int out_posx, int out_posy,int out_width, int out_height,int interlace);
extern int ark_itu656_display_addr(unsigned int addr);
extern int ark_itu656_display_uninit(void);
extern int ark_disp_set_layer_en(int layer_id, int enable);

#define itu656_readl(reg)		            __raw_readl(g_dvr_dev->context.itu656_base+(reg))
#define itu656_writel(reg, val)			    __raw_writel((val), g_dvr_dev->context.itu656_base+(reg))
#define itu656_readl_sys(reg)			    __raw_readl(g_dvr_dev->context.sys_base+(reg))
#define itu656_writel_sys(reg, val)		    __raw_writel((val), g_dvr_dev->context.sys_base+(reg))
#define itu656_readl_dein(reg)			    __raw_readl(g_dvr_dev->context.deinterlace_base+(reg))
#define itu656_writel_dein(reg, val)	            __raw_writel((val), g_dvr_dev->context.deinterlace_base+(reg))
#define itu656_readl_lcd(reg)			    __raw_readl(g_dvr_dev->context.lcd_base+(reg))
#define itu656_writel_lcd(reg, val)	            __raw_writel((val), g_dvr_dev->context.lcd_base+(reg))


static struct dvr_dev *g_dvr_dev = NULL;
extern struct i2c_client *g_itu656in_client;
extern struct ark_private_data *g_itu656in_priv;

#define GetPingPongNextBuf(index,bufcnt) {index++;if(index == bufcnt) index = 0;}
#define GetPingPongPreBuf(index,bufcnt) {index--; if(index <0) index = bufcnt - 1;}
#define GetFrameNum(framecnt,oddeven,bufcnt) { framecnt = (oddeven == DEINTERLACE_FIELD_ODD) ? bufcnt*2:(bufcnt*2+1);}

extern void carback_first_enter(void);

/* 2026-08-03: probe-order fix, see ark-carback.c's own probe for the
 * other half. g_dvr_dev is only ever non-NULL once this driver's own
 * probe has set it (line ~2039, well before the carback_first_enter()
 * call at the end of that same probe) -- lets ark-carback's probe
 * safely check "has itu656 already come up" before calling into
 * dvr_enter_carback() itself, which dereferences g_dvr_dev
 * unconditionally and would Oops if called too early. */
int ark1668_itu656_is_probed(void)
{
	return g_dvr_dev != NULL;
}
EXPORT_SYMBOL(ark1668_itu656_is_probed);

static void ark_disp_set_tvout_next_oddfield_bufaddr(unsigned int addr)
{

}
static void ark_disp_set_tvout_next_evenfield_bufaddr(unsigned int addr)
{

}

static int ark_disp_set_tvenc_cfg(struct ark_disp_tvenc_cfg_arg *arg)
{
        return 0;
}

static int ark_fb_update_window_by_layer_id(int layer_id, struct arkfb_update_window *arg)
{
        return 0;
}
	
static int ark_disp_set_gui_tvout(int enable)
{
        return 0;
}

void mirror_paint_work(struct work_struct *work)
{
	struct dvr_dev* dvr_dev = g_dvr_dev;
	int width,height;
	int i,j;
	unsigned char *addr_src;
	unsigned char *addr_dest;
	unsigned short *psrc;
	unsigned short *pdest;
	unsigned int right_cut;
        unsigned int value;

	if(!dvr_dev->interlace) return;
	if(dvr_dev->mirror_type == MIRROR_TYPE_NONE) return;

	GetPingPongNextBuf(dvr_dev->write_mirror,ITU656_BUFFER_NUM);
	dvr_dev->buf_status[dvr_dev->write_mirror] &= ~ITU656_BUFFER_FULL_MIRROR;

	if (dvr_dev->system == PAL) {
		width = 720;
		height = 576;
		right_cut = 12;//must even ,as :2 ,4, 6..., set point to uy
	} else {
		width = 720;
		height = 480;
		right_cut = 12;//must even ,as :2 ,4, 6...,set point to uy
	}

	addr_src  = (unsigned char *)dvr_dev->evenbuf_virtaddr[dvr_dev->write_mirror];
	addr_dest = (unsigned char *)dvr_dev->oddbuf_virtaddr[dvr_dev->write_mirror];
	dma_map_single(NULL,(void *)addr_src, width*height*2, DMA_FROM_DEVICE);

	//srcdata : vyuy vyuy....vyuy vyuy 
	if(dvr_dev->mirror_type == MIRROR_TYPE_L2R){
		for(i = 0;i < height; i++){
			psrc  = (unsigned short *)(addr_src + width*2*i + (width-1-right_cut)*2);
			pdest = (unsigned short *)(addr_dest + width*2*i);
			for(j = 0;j < width-right_cut; j++){
				*pdest = *psrc;
				pdest++;
				psrc--;
			}
		}
	}
	else if(dvr_dev->mirror_type == MIRROR_TYPE_U2D){
		for(i = 0;i < height; i++){
			psrc  = (unsigned short *)(addr_src + width*2*i);
			pdest = (unsigned short *)(addr_dest + width*2*(height-1-i));
			memcpy(pdest,psrc,width*2);
		}
	}
	else if(dvr_dev->mirror_type == MIRROR_TYPE_L2R_U2D){
		for(i = 0;i < height; i++){
			psrc  = (unsigned short *)(addr_src + width*2*i + (width-1-right_cut)*2);
			pdest = (unsigned short *)(addr_dest + width*2*(height-1-i));
			for(j = 0;j < width-right_cut; j++){
				*pdest = *psrc;
				pdest++;
				psrc--;
			}
		}
	}
	else{
		memcpy(addr_dest,addr_src,width*height*2);
	}
	
	//for mirror
	value = itu656_readl_lcd(ARK1668_LCDC_VIDEO2_CTL);
	if(dvr_dev->mirror_type & MIRROR_TYPE_L2R) value |= (0x01<<17);
	else value &= ~(0x03<<17);
        itu656_writel_lcd(ARK1668_LCDC_VIDEO2_CTL,value);

	dma_map_single(NULL,(void *)addr_dest, width*height*2, DMA_TO_DEVICE);
	dvr_dev->buf_status[dvr_dev->write_mirror] |= ITU656_BUFFER_FULL_LCD;
}

void ark_sys_pad_config(unsigned int reg_offset, unsigned int mask, 
	unsigned int bit_offset, unsigned int val)
{
	unsigned int reg;

	reg = itu656_readl_sys(reg_offset);
	reg &= ~(mask << bit_offset);
	reg |= ((val & mask) << bit_offset);
	itu656_writel_sys(reg_offset, reg);
}

static void itu656_pad(void)
{
        unsigned int val;

        val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0B);
        val |= (0x1FF<<16);
        itu656_writel_sys(ARK1668_SYS_PAD_CTRL0B,val);

        val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0A);
        val &= ~(0xF<<4);
        val |= 5<<4;
        itu656_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);
}

static void ark_itu656_set_direct_data_to_lcd_enable(int enable)
{
        unsigned int val;

	itu656_pad(); 
	itu656_writel(ARK1668_ITU656_INPUT_SEL, itu656_readl(ARK1668_ITU656_INPUT_SEL)|(1<<0));
	if (enable) {
                val = 1<<2 | // 1=disable itu656 data write back to ddr2 
                      1<<1;  // 1=enable itu656 data and timing by pass to lcd
                itu656_writel(ARK1668_ITU656_MODULE_EN, itu656_readl(ARK1668_ITU656_MODULE_EN)| val);
                itu656_writel(ARK1668_ITU656_PIX_LINE_NUM_DELTA, 30<<8 | 10);
                // rITU656IN_IMR = 1<<7; 
	} else {
                val = 1<<2 | // 0=enable itu656 data write back to ddr2 
                      1<<1;  // 1=disable itu656 data and timing by pass to lcd
                itu656_writel(ARK1668_ITU656_MODULE_EN, itu656_readl(ARK1668_ITU656_MODULE_EN)& ~val);
                itu656_writel(ARK1668_ITU656_IMR, 0); // disable interrupt outputs
	}
}

static inline void ark_itu656_set_global_enable(int enable)
{
        if (enable)
                itu656_writel(ARK1668_ITU656_ENABLE_REG, itu656_readl(ARK1668_ITU656_ENABLE_REG)|(1<<0));// global enable
        else
                itu656_writel(ARK1668_ITU656_ENABLE_REG, itu656_readl(ARK1668_ITU656_ENABLE_REG)& ~(1<<0));// global disable
}

int ark_itu656_direct_data_to_lcd_start(void)
{
        itu656_pad(); 
        ark_itu656_set_direct_data_to_lcd_enable(1);
        return 0;
}
EXPORT_SYMBOL(ark_itu656_direct_data_to_lcd_start); 

void ark_itu656_stop(void)
{
        ark_itu656_set_global_enable(0);
        itu656_writel(ARK1668_ITU656_IMR ,0); // disable all interrupt outputs    
}
EXPORT_SYMBOL(ark_itu656_stop);

int ark_sys_pad_config_gpio_mode(int gpio)
{
	if((gpio<=GPIO1) && (gpio>=GPIO0))
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x3, (gpio-GPIO0)*2, 0);
	else if(gpio<=GPIO9)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL00, 0xf, (gpio-GPIO2)*4, 0);
	else if(gpio<=GPIO17)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL01, 0xf, (gpio-GPIO10)*4, 0);
	else if(gpio<=GPIO25)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL02, 0xf, (gpio-GPIO18)*4, 0);
	else if(gpio<=GPIO29)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL03, 0xf, (gpio-GPIO26)*4, 0);
	else if(gpio<=GPIO38)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL07, 0x3, (gpio-GPIO30)*2, 0);
	else if(gpio<=GPIO46)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL04, 0xf, (gpio-GPIO39)*4, 0);
	else if(gpio<=GPIO54)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL05, 0xf, (gpio-GPIO47)*4, 0);
	else if(gpio<=GPIO61)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL06, 0xf, (gpio-GPIO55)*4, 0);
	else if(gpio<=GPIO71)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL08, 0x3, (gpio-GPIO62)*2, 0);
	else if(gpio<=GPIO84)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x3, (gpio-GPIO72)*2+4, 0);
	else if(gpio<=GPIO85)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x1, 30, 0);
	else if(gpio<=GPIO86)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x1, 31, 0);
	else if(gpio<=GPIO116)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL0B, 0x1, (gpio-GPIO85), 0);
	else if(gpio<=GPIO127)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL0C, 0x1, (gpio-GPIO117), 0);
	else
		return  -1;

	return 0;
}
EXPORT_SYMBOL(ark_sys_pad_config_gpio_mode);

int detect_cvbs_mode(void)
{
	unsigned int pnline;
	unsigned int pnpixel;
	int ntsc = 0;
		
	pnline	= itu656_readl(ARK1668_ITU656_LINE_NUM_PER_FIELD) & 0x7FE;
	pnpixel = (itu656_readl(ARK1668_ITU656_PIX_NUM_PER_LINE) & 0xFFE) >> 1;
		
	if ((pnline > 230) && (pnline < 250)) {
		ntsc = 1;
	} else if((pnline > 278) && (pnline < 298)) {
		ntsc = 0;
	} else {
		ntsc = 1;
	}
		
	return ntsc;
}

static void ark_itu656_pad_select(struct dvr_dev *dvr_dev)
{
	unsigned int val;

	if (dvr_dev->itu601en) {
		//hsync, vsync
		val = (1 << 19) | (1 << 18);
        itu656_writel_sys(ARK1668_SYS_PAD_CTRL07, itu656_readl_sys(ARK1668_SYS_PAD_CTRL07)|val);
	}
	
	if (dvr_dev->itu_channel== ITU656_CH0) {
		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0A);
		val &= ~(0xF<<4);
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);	            

		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL07);
		val &= ~(0x1FFFF<<0);
		val |= 0x15555;
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL07, val);
	} else if (dvr_dev->itu_channel == ITU656_CH1) {
		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0A);
		val &= ~(0xF<<4);
		val |= 5<<4;
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);	
    
		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0B);
		val |= (0x1FF<<16);
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL0B, val);	
	} else if(dvr_dev->itu_channel == ITU656_CH2){
		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0A);
		val &= ~(0xF<<4);
		val |= 0xA<<4;
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);	
    
		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0B);
		val |= (0x7F<<25);
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL0B, val);

		val = itu656_readl_sys(ARK1668_SYS_PAD_CTRL0C);
		val |= (0x3<<0);
		itu656_writel_sys(ARK1668_SYS_PAD_CTRL0C, val);
	}   	
}

static void ark_itu656_reg_init(struct dvr_dev *dvr_dev)
{
        unsigned int val;

        itu656_printk("%s in--->\n", __func__);
	
	// Clock On
	itu656_writel_sys(ARK1668_SYS_PER_CLK_EN, itu656_readl_sys(ARK1668_SYS_PER_CLK_EN)|1 << 12);
        itu656_writel_sys(ARK1668_SYS_AXI_CLK_EN, itu656_readl_sys(ARK1668_SYS_AXI_CLK_EN)|1 << 2);
        itu656_writel_sys(ARK1668_SYS_AHB_CLK_EN, itu656_readl_sys(ARK1668_SYS_AHB_CLK_EN)|1 << 10);

        //soft reset
        itu656_writel_sys(ARK1668_SYS_SOFT_RSTNA, itu656_readl_sys(ARK1668_SYS_SOFT_RSTNA)& ~(1 << 9));
        msleep(1);
        itu656_writel_sys(ARK1668_SYS_SOFT_RSTNA, itu656_readl_sys(ARK1668_SYS_SOFT_RSTNA)| (1 << 9));

        itu656_writel(ARK1668_ITU656_MODULE_EN, 1<<2);
	if (dvr_dev->itu601en) {
                itu656_writel(ARK1668_ITU656_MODULE_EN, itu656_readl(ARK1668_ITU656_MODULE_EN)| 1 );
		itu656_writel(ARK1668_ITU656_INPUT_SEL, 0);
                //for vbox r601
                if(dvr_dev->priv_data.ic_type == IC_TYPE_UB934){
            		val = (0 << 13) | (0 << 12) | (1 << 4)| (1 << 3) | (1 << 2);
            		itu656_printk("vbox---progressive, set CBCR_YVYU.\n");
                }else{
            		val = (1 << 13) | (1 << 12) | (1 << 4);
                }
                itu656_writel(ARK1668_ITU656_SEP_MODE_SEL, val);

	}else{
		itu656_writel(ARK1668_ITU656_INPUT_SEL, 0x01);
        }

	itu656_writel(ARK1668_ITU656_ICR, 0xff);

	val	= (10<<16) /*debounce*/
		| (DELTA_LINE<<8) 
		| 0xFF/*DELTA_PIX*/;

        itu656_writel(ARK1668_ITU656_PIX_LINE_NUM_DELTA, val);

	itu656_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->buffer_phyaddr);
	itu656_writel(ARK1668_ITU656_DRAM_DEST2, dvr_dev->buffer_phyaddr);		

	if (dvr_dev->interlace) {	
#ifdef ITU656_USE_DEINTERLACE
		itu656_writel(ARK1668_ITU656_IMR, FIELD_INTERRUPT);
		val = WRITE_MEMORY_TWO_FIELD
        		|CBCR_YUYV
        		|H_FILTER_COEF_AUTO
        		|YCBCR444_422_FILTER_ENABLE
        		|CB_FIRST
        		|GLOBAL_DISABLE;
#else
		itu656_writel(ARK1668_ITU656_IMR, FRAME_INTERRUPT_INTERRUPT);
		val = WRITE_MEMORY_TWO_FIELD
			|CBCR_YUYV
			|H_FILTER_COEF_AUTO
			|STORE_DATA_2FIELD_2ADDR
			|YCBCR444_422_FILTER_ENABLE
			|CB_FIRST
			|GLOBAL_DISABLE;
#endif	
                itu656_writel(ARK1668_ITU656_ENABLE_REG, val);

	} else {
		itu656_writel(ARK1668_ITU656_IMR, FIELD_INTERRUPT);
		val = WRITE_MEMORY_TWO_FIELD
			|CBCR_YUYV
			|H_FILTER_COEF_AUTO
			|YCBCR444_422_FILTER_ENABLE
			|CB_FIRST
			|GLOBAL_DISABLE;
                itu656_writel(ARK1668_ITU656_ENABLE_REG, val);

		val	= (10<<16) /*debounce*/
			| (DELTA_LINE<<8) 
			| DELTA_PIX;
                itu656_writel(ARK1668_ITU656_PIX_LINE_NUM_DELTA, val);
	}

	//for vbox
	if(dvr_dev->deinter_indirect_show){
		itu656_printk("vbox---deinter_indirect_show, set CBCR_YVYU\n");
		itu656_writel(ARK1668_ITU656_ENABLE_REG, itu656_readl(ARK1668_ITU656_ENABLE_REG)& ~(CBCR_YUYV));
	}

	itu656_printk("%s out<---\n", __func__);
}

static void ark_itu656_reg_uninit(void)
{
        // Clock Off
        itu656_writel_sys(ARK1668_SYS_PER_CLK_EN, itu656_readl_sys(ARK1668_SYS_PER_CLK_EN)& ~(1 << 12));
        itu656_writel_sys(ARK1668_SYS_AXI_CLK_EN, itu656_readl_sys(ARK1668_SYS_AXI_CLK_EN)& ~(1 << 2));
        itu656_writel_sys(ARK1668_SYS_AHB_CLK_EN, itu656_readl_sys(ARK1668_SYS_AHB_CLK_EN)& ~(1 << 10));
}

static void ark_itu656_enable(struct dvr_dev *dvr_dev)
{
        itu656_writel(ARK1668_ITU656_ENABLE_REG, itu656_readl(ARK1668_ITU656_ENABLE_REG)| (1 << 0)); 
        itu656_writel(ARK1668_ITU656_IMR, itu656_readl(ARK1668_ITU656_IMR)| (TOTAL_LINE_CHANGED_INTERRUPT));
        if (!dvr_dev->interlace)
        itu656_writel(ARK1668_ITU656_IMR, itu656_readl(ARK1668_ITU656_IMR)| (ACTIVE_PIX_CHANGED_INTERRUPT));

        itu656_writel_sys(ARK1668_SYS_DEVICE_CLK_CFG1, itu656_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG1)| 1);
}

void ark_itu656_disable(void)
{
        itu656_writel(ARK1668_ITU656_IMR, 0);
        itu656_writel(ARK1668_ITU656_ENABLE_REG,itu656_readl(ARK1668_ITU656_ENABLE_REG) & ~(1 << 0));
}

static inline void ark_itu656_enable_write(void)
{
        itu656_writel(ARK1668_ITU656_MODULE_EN,itu656_readl(ARK1668_ITU656_MODULE_EN) & ~(1 << 2));
}

static inline void ark_itu656_disable_write(void)
{
        itu656_writel(ARK1668_ITU656_MODULE_EN,itu656_readl(ARK1668_ITU656_MODULE_EN) | (1 << 2));
}

static void deinterlace_reset(void)
{
	itu656_writel_dein(ARK1668_DEINTERLACE_CTRL0, (1 << 0));
	ndelay(100);
	itu656_writel_dein(ARK1668_DEINTERLACE_CTRL0, (0 << 0));
}

static void deinterlace_init(void)
{
	deinterlace_reset();	
	itu656_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x3);		
	itu656_writel_dein(ARK1668_DEINTERLACE_INT_MASK, 0x3);
}

static int deinterlace_process (unsigned int deinterlace_size,  unsigned int data_mode, unsigned int deinterlace_type,
	unsigned int deinterlace_field, unsigned int src_field_addr_0, unsigned int src_field_addr_1,unsigned int src_field_addr_2,
	unsigned int dst_y_addr, unsigned int dst_u_addr, unsigned int dst_v_addr)
{
	unsigned int pixel_per_line;
	unsigned int total_line;
	unsigned int pn;
	unsigned int denoise_bypass;
	unsigned int stride;
	unsigned int only_wr_1_field;
	unsigned int field;
	unsigned int out_mode;
	unsigned int vary_level_th;
	int ret = DEINTERLACE_SUCCESS;
        unsigned int val;
	
	if(deinterlace_field != DEINTERLACE_FIELD_ODD
		&&	deinterlace_field != DEINTERLACE_FIELD_EVEN){
		pr_warn("invalid deinterlace field (%d)\n", deinterlace_field);
		return DEINTERLACE_PARA_ERROR;				
	}
	field = deinterlace_field;
	
	if(deinterlace_size == DEINTERLACE_LINE_SIZE_960H){
		pixel_per_line = 120 * (1 + data_mode);
	}else if(deinterlace_size == DEINTERLACE_LINE_SIZE_720H){
		pixel_per_line = 90 * (1 + data_mode);
	}else{
		pr_warn("invalid deinterlace size (%d)\n", deinterlace_size);
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(data_mode == DEINTERLACE_DATA_MODE_420){
		denoise_bypass = 1;
		stride = pixel_per_line;
		only_wr_1_field = 1;
	}else if(data_mode == DEINTERLACE_DATA_MODE_422){
		if(dst_y_addr == 0){
			pr_warn("illegal deinterlace dst Y address\n");
			return DEINTERLACE_PARA_ERROR;
		}
		denoise_bypass = 1;
		stride = 0;
		only_wr_1_field = 0;
	}else{
		pr_warn("invalid deinterlace data mode (%d)\n", data_mode);
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(deinterlace_type == DEINTERLACE_TYPE_PAL){
		pn = 0;
		total_line = 288;
		vary_level_th = 0x800;
	}else if(deinterlace_type == DEINTERLACE_TYPE_NTSC){
		pn = 1;
		total_line = 240;
		vary_level_th = 0x8;
	}else{
		pr_warn("invalid deinterlace type (%d)\n", deinterlace_type);
		return DEINTERLACE_PARA_ERROR;		
	}		

	out_mode = DEINTERLACE_DATA_MODE_422;
	
	val = (out_mode<<31)
		|  (0x0 << 29)
		|  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field                      	
		|  (pixel_per_line << 20)       // pixel_pl
		|  (total_line << 11)           // total_line
		|  (stride << 3)                // stride
		|  (data_mode << 2)             // data_mode
		|  (field << 1);                 // field
	itu656_writel_dein(ARK1668_DEINTERLACE_CTRL0, val);	

 	val = (out_mode<<31)
		|  (0x0 << 29)
		|  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field                      	
		|  (pixel_per_line << 20)       // pixel_pl
		|  (total_line << 11)           // total_line
		|  (stride << 3)                // stride
		|  (data_mode << 2)             // data_mode
		|  (field << 1);                 // field
	itu656_writel_dein(ARK1668_DEINTERLACE_CTRL0, val);	
 
	val = (0x8 << 24 ) 
		|(vary_level_th << 8)
		|(0X8 << 0);
        itu656_writel_dein(ARK1668_DEINTERLACE_CTRL1, val);		
	val = 0x300A4230; 
        itu656_writel_dein(ARK1668_DEINTERLACE_CTRL2, val);	

	// denoise bypass  pn: 1:n display_motion line_intra global_cnt display_mv_0
	val = (0x07 << 16)				//motion_ctrl_reg5\uff1a\u964d\u566a\u95e8\u9650
		| (denoise_bypass << 15)  
		| (pn << 13) 
		| (0x1 << 11)
		| (0x1 << 10)
		| (0x1 << 9)
		| (0x0 << 6)
		| (0x0 << 5)
		| (0x1 << 3)
		| (0x0 << 2)  // max_ycbcr_ena :use for control the max control   
		| (0x1 << 1)
		| (0x0 << 0);
        itu656_writel_dein(ARK1668_DEINTERLACE_CTRL3, val);	
     
	itu656_writel_dein(ARK1668_DEINTERLACE_FILM_MODECTRL, (0x0 << 31));

	itu656_writel_dein(ARK1668_DEINTERLACE_SADDR0, src_field_addr_0);	// s0_0                             
	itu656_writel_dein(ARK1668_DEINTERLACE_SADDR1, src_field_addr_1);	// s1_0
	itu656_writel_dein(ARK1668_DEINTERLACE_SADDR2, src_field_addr_2);	// s2_0
     
	itu656_writel_dein(ARK1668_DEINTERLACE_DADDRY, dst_y_addr);	// dy_0  when filed = 1 ,data_mode = 0  then dy_0 = s0_0 , when filed = 0 ,data_mode = 0  then dy_0 = s2_0 
	itu656_writel_dein(ARK1668_DEINTERLACE_DADDRU, dst_u_addr);	// du_0
	itu656_writel_dein(ARK1668_DEINTERLACE_DADDRV, dst_v_addr);	// dv_0
     
	//	pingpong addr fetch 
	itu656_writel_dein(ARK1668_DEINTERLACE_ADDR_SWITCHMODE, 0x0);
   
	// \u6e05\u9664\u4e2d\u65ad
	itu656_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x3); 
	
	// start de-interlace
	itu656_writel_dein(ARK1668_DEINTERLACE_START, 0x1); 
 
	return ret;
}

static void dvr_tvout_enable(struct dvr_dev *dvr_dev, int enable)
{
	struct ark_disp_tvenc_cfg_arg tvenc_cfg = {0};	

	if (enable) {
		tvenc_cfg.enable = enable;
		if (dvr_dev->system == PAL)
			tvenc_cfg.out_mode = ARKDISP_TVENC_OUT_CVBS_PAL;
		else
			tvenc_cfg.out_mode = ARKDISP_TVENC_OUT_CVBS_NTSC;
		tvenc_cfg.backcolor_y = 0x10;
		tvenc_cfg.backcolor_cb = 0x80;
		tvenc_cfg.backcolor_cr = 0x80;
		ark_disp_set_tvenc_cfg(&tvenc_cfg);	
	}
}

static void dvr_config_tvout(struct dvr_dev *dvr_dev)
{
	struct arkfb_update_window tvout_arg = {0};
	int width, height;
	int out_width, out_height;

	if(dvr_dev->interlace) {
		if (dvr_dev->system == PAL) {
			width = 720;
			height = 288;
		} else {
			width = 720;
			height = 240;
		}
		out_width = width;
		out_height = height;
	} else {
		width = dvr_dev->src_width;
		height = dvr_dev->src_height;
		out_width = 720;
		out_height = 480;
	}
	
	tvout_arg.win_width = width;
	tvout_arg.win_height  = height; //cut 2 lines on the bottom
	tvout_arg.width = width;
	tvout_arg.height = height;
	tvout_arg.out_width = out_width;
	tvout_arg.out_height = out_height;
	tvout_arg.interlace_out = !dvr_dev->interlace;
	tvout_arg.format = ARK_DISP_VIDEO_PIXFMT_YUYV;
	
	ark_fb_update_window_by_layer_id(TVOUT_LAYER, &tvout_arg);	
}

static void dvr_start(struct dvr_dev *dvr_dev)
{ 
	if(!dvr_dev->work_status){
		deinterlace_init();
		if(dvr_dev->priv_data.select_channel)
			dvr_dev->priv_data.select_channel(dvr_dev->channel);
		ark_itu656_pad_select(dvr_dev);
		ark_itu656_reg_init(dvr_dev);
		ark_itu656_enable(dvr_dev);
		dvr_dev->work_status = 1;
		dvr_dev->old_cvbs_type = TYPE_UNKNOWN;
		itu656_printk("%s in--->\n", __func__);
	}
}

static void dvr_stop(struct dvr_dev *dvr_dev)
{
	dvr_dev->work_status = 0;
	del_timer(&dvr_dev->timer);
	dvr_dev->show_video = 0;
	ark_disp_set_tvout_next_oddfield_bufaddr(0);
	ark_disp_set_tvout_next_evenfield_bufaddr(0);
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	ark_itu656_disable_write(); /*stop write data back*/
	ark_itu656_disable();
	if (dvr_dev->itu656in.tvout) {
		//dvr_tvout_enable(dvr_dev, 0);
		ark_disp_set_layer_en(TVOUT_LAYER, 0);	
		ark_disp_set_gui_tvout(1);
	}	
	msleep(100);
	ark_itu656_reg_uninit();
	itu656_printk("%s out<---\n", __func__);
}

static void dvr_init(struct dvr_dev *dvr_dev, struct itu656in_para *para)
{
	int i;

	memcpy(&dvr_dev->itu656in, para, sizeof(struct itu656in_para));
	dvr_dev->interlace = !dvr_dev->itu656in.progressive;
	dvr_dev->itu601en = dvr_dev->itu656in.itu601en;
	
	if (dvr_dev->interlace) {
		
		for(i=0; i<ITU656_BUFFER_NUM; i++)
		{
			dvr_dev->oddbuf_phyaddr[i]  = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*2*i;
			dvr_dev->evenbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*(2*i+1);
			//for mirror 
			dvr_dev->oddbuf_virtaddr[i] = (unsigned int)dvr_dev->buffer_virtaddr + ITU656_FRAME_SIZE*2*i;
			dvr_dev->evenbuf_virtaddr[i]= (unsigned int)dvr_dev->buffer_virtaddr + ITU656_FRAME_SIZE*(2*i+1);
		}

		for(i=0; i<4; i++)
			dvr_dev->fieldbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*6 + ITU656_FIELD_SIZE*i;

		//for vbox
		for(i=0; i<ITU656_FRAME_NUM; i++)
			dvr_dev->framebuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*i;
		
	} else {
		for(i=0; i<ITU656_BUFFER_NUM; i++)
		{
			if(dvr_dev->priv_data.support_max_resolution == TYPE_1080P)
				dvr_dev->oddbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE_1080P*i;
			else
				dvr_dev->oddbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE*i;		
		}
		
		//for vbox
		for(i=0; i<ITU656_FRAME_NUM; i++)
			dvr_dev->framebuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE*i;
	}
	
	switch (para->source) {
	case DVR_SOURCE_DVD:
		dvr_dev->channel = ARK7116_AV2;
		break;
	case DVR_SOURCE_AUX:
		dvr_dev->channel = ARK7116_AV1;
		break;
	case DVR_SOURCE_CAMERA:
	default:
		dvr_dev->channel = ARK7116_AV0;
		break;
	}
}

#if 0
static unsigned int GetTimerMS(void)
{
    struct timeval tv;
    //float s;
    do_gettimeofday(&tv);
    //s = tv.tv_usec; s *= 0.000001; s += tv.tv_sec;
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static unsigned int GetTimer(void)
{
    struct timeval tv;
    //float s;
    do_gettimeofday(&tv);
    //s = tv.tv_usec; s *= 0.000001; s += tv.tv_sec;
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

static void cvbs_type_change_detect(struct dvr_dev *dvr_dev)
{
	if(dvr_dev->ic_type == IC_TYPE_TP2825B)
	{
		if(dvr_dev->system == NTSC)
		{
			if(dvr_dev->old_cvbs_type != NTSC)
			{
                itu656_writel(ARK1668_ITU656_ENABLE_REG,itu656_readl(ARK1668_ITU656_ENABLE_REG) & ~(2<<5));
				dvr_dev->old_cvbs_type = NTSC;
			}
		}
		else
		{
			if(dvr_dev->old_cvbs_type != PAL)
			{
                itu656_writel(ARK1668_ITU656_ENABLE_REG,itu656_readl(ARK1668_ITU656_ENABLE_REG) | (2<<5));
				dvr_dev->old_cvbs_type = PAL;
			}
		}
	}
}

static void dither_timeout_timer(struct timer_list *t)
{
	struct dvr_dev *dvr_dev = from_timer(dvr_dev, t, timer);
	struct arkfb_update_window display_arg = {0};
	int width, height;
	int line,pixel;
	int activeLine, activePix;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	if (!dvr_dev->work_status) {
		spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
		return;
	}

	line = itu656_readl(ARK1668_ITU656_LINE_NUM_PER_FIELD) & 0x7FE;
	if (dvr_dev->itu601en)
		pixel = itu656_readl(ARK1668_ITU656_PIX_NUM_PER_LINE) & 0xFFF;
	else
		pixel = (itu656_readl(ARK1668_ITU656_PIX_NUM_PER_LINE) & 0xFFE) >> 1;	

	itu656_printk("[ITU] line = %d, pixel=%d\r\n", line, pixel);

	dvr_dev->src_width = activePix = pixel;
	dvr_dev->src_height = activeLine = line;

	if (dvr_dev->interlace) {
		if (line >= 230 && line <= 250)
			dvr_dev->system = NTSC;
		else if (line >= 278 && line < 298)
			dvr_dev->system = PAL;
		else
			dvr_dev->system = NTSC;

		if(dvr_dev->system == NTSC)
		{
			activeLine = 240;
			activePix = 720;
		}
		else
		{
			activeLine = 288;
			activePix = 720;
		}
		cvbs_type_change_detect(dvr_dev);
	}

	itu656_writel(ARK1668_ITU656_OUTLINE_NUM_PER_FIELD, activeLine);
	itu656_writel(ARK1668_ITU656_DATA_OUT_NUM, activeLine*activePix);
	itu656_writel(ARK1668_ITU656_SIZE, activePix<<16);

	width = activePix;
	height = activeLine;
	if (dvr_dev->interlace) {
#ifdef 	ITU656_USE_DEINTERLACE
		height *= 2;
#endif
	}
	if (dvr_dev->itu601en) {
		display_arg.win_x = dvr_dev->itu656in.left_blank + 2;
		display_arg.win_y = dvr_dev->itu656in.top_blank + 2;
		display_arg.win_width = width - dvr_dev->itu656in.left_blank - 
								dvr_dev->itu656in.right_blank-2;
	}else{
		display_arg.win_x = dvr_dev->itu656in.left_blank;
		display_arg.win_y = dvr_dev->itu656in.top_blank + 2;
		display_arg.win_width = width - dvr_dev->itu656in.left_blank - 
			                    dvr_dev->itu656in.right_blank;
		}
	if (dvr_dev->interlace) {
#ifdef 	ITU656_USE_DEINTERLACE	
		display_arg.win_width -= 12;
#endif
	}
	display_arg.win_height  = height - dvr_dev->itu656in.top_blank - 
	                          dvr_dev->itu656in.bottom_blank - 4;
	display_arg.width = width;
	display_arg.height = height;
	display_arg.out_width = dvr_dev->itu656in.width;
	display_arg.out_height = dvr_dev->itu656in.height;
	display_arg.out_x = dvr_dev->itu656in.xpos;
	display_arg.out_y = dvr_dev->itu656in.ypos;
	display_arg.format = ARK_DISP_VIDEO_PIXFMT_YUYV;

	if (!dvr_dev->deinter_indirect_show){
		//ark_fb_update_window_by_layer_id(DISPLAY_LAYER, &display_arg);
                ark_itu656_display_init(width,height,0,0,display_arg.out_width,display_arg.out_height,1);
        }

	if (dvr_dev->itu656in.tvout) {
		ark_disp_set_gui_tvout(0);
		dvr_config_tvout(dvr_dev);
		dvr_tvout_enable(dvr_dev, 1);
	}

	dvr_dev->discard_frame = DISCARD_FRAME_SYS_CHANGE;
	if (dvr_dev->interlace) {
#ifdef 	ITU656_USE_DEINTERLACE
		dvr_dev->discard_frame = DISCARD_FRAME_SYS_CHANGE * 2;		
#endif
	}
	
	dvr_dev->video_reinit = 1;
	dvr_dev->show_video = 0;
	dvr_dev->display_odd_even = 0;
	for (i = 0; i < ITU656_BUFFER_NUM; i++)
		dvr_dev->buf_status[i] = 0;
	dvr_dev->write_buffer = 0;
	dvr_dev->display_buffer = ITU656_BUFFER_NUM - 1;

	for (i = 0; i < ITU656_FRAME_NUM; i++)
		dvr_dev->framebuf_status[i] = 0;
	dvr_dev->write_framebuf = 0;////
	dvr_dev->get_framebuf = 0;
	ark_itu656_enable_write();	
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
}

static irqreturn_t ark_deinterlace_int_handler(int irq, void *dev_id)
{
	u32 raw_int,i;
	unsigned long flags;
	struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	static int last_mirror_type;

	raw_int = itu656_readl_dein(ARK1668_DEINTERLACE_RAW_INT);
	
	spin_lock_irqsave(&dvr_dev->spin_lock, flags);
	
	if(raw_int & (1 << 0 )){
		itu656_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x1);
	}else if(raw_int & (1 << 1)){
		itu656_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x2);   //error
		deinterlace_reset();
		pr_err("deinterlace axi error\n");
	}
	
	if(last_mirror_type != dvr_dev->mirror_type && dvr_dev->interlace){
		if (dvr_dev->mirror_type == MIRROR_TYPE_NONE){
			dvr_dev->write_buffer = 0;
			dvr_dev->display_buffer = 0;
			for (i = 0; i < ITU656_BUFFER_NUM; i++)
				dvr_dev->buf_status[i] = 0;
			itu656_writel_lcd(ARK1668_LCDC_VIDEO2_CTL,  itu656_readl_lcd(ARK1668_LCDC_VIDEO2_CTL)& ~(0x03<<17));
			last_mirror_type = MIRROR_TYPE_NONE;
			pr_debug("mirror_type=0, reset.\n");
			goto end;
		}
	}
	
	last_mirror_type = dvr_dev->mirror_type;
	if(dvr_dev->mirror_type && dvr_dev->interlace){//for mirror
		if (dvr_dev->deinter_odd_even == DEINTERLACE_FIELD_EVEN) {
			dvr_dev->buf_status[dvr_dev->write_buffer] |= ITU656_BUFFER_FULL_MIRROR;
			GetPingPongNextBuf(dvr_dev->write_buffer,ITU656_BUFFER_NUM);
			if(dvr_dev->buf_status[dvr_dev->write_buffer]&ITU656_BUFFER_FULL_MIRROR)
			{
				GetPingPongPreBuf(dvr_dev->write_buffer, ITU656_BUFFER_NUM);
				dvr_dev->buf_status[dvr_dev->write_buffer] &= ~ITU656_BUFFER_FULL_MIRROR;
			}
		}
		
		queue_work(dvr_dev->mirror_queue, &dvr_dev->mirror_work);
			
	}
	else if(dvr_dev->deinter_indirect_show){//for vbox
		
		dvr_dev->framebuf_status[dvr_dev->write_framebuf] |= ITU656_BUFFER_FULL_APP;
		GetPingPongNextBuf(dvr_dev->write_framebuf,ITU656_FRAME_NUM);
		if(dvr_dev->framebuf_status[dvr_dev->write_framebuf])
		{
			GetPingPongPreBuf(dvr_dev->write_framebuf, ITU656_FRAME_NUM);
			dvr_dev->framebuf_status[dvr_dev->write_framebuf] &= ~ITU656_BUFFER_FULL_APP;
		}
		
		dvr_dev->frame_finish_count = 0;
		for(i = 0;i < ITU656_FRAME_NUM;i++){
			if(dvr_dev->framebuf_status[i] == ITU656_BUFFER_FULL_APP)
				dvr_dev->frame_finish_count++;
		}
		
		wake_up_interruptible(&dvr_dev->frame_finish_waitq);		
		if(dvr_dev->fasync_queue != NULL) {
			//printk(KERN_ALERT "kill_fasync camera frame finish.\n");
			kill_fasync(&dvr_dev->fasync_queue, SIGIO, POLL_IN);
		}
	}
	else{//normal
		
		if (dvr_dev->deinter_odd_even == DEINTERLACE_FIELD_EVEN) {
			dvr_dev->buf_status[dvr_dev->write_buffer] |= ITU656_BUFFER_FULL_LCD;
			GetPingPongNextBuf(dvr_dev->write_buffer,ITU656_BUFFER_NUM);
			if(dvr_dev->buf_status[dvr_dev->write_buffer])
			{
				GetPingPongPreBuf(dvr_dev->write_buffer, ITU656_BUFFER_NUM);
				dvr_dev->buf_status[dvr_dev->write_buffer] &= ~ITU656_BUFFER_FULL_LCD;
			}
            dvr_dev->buf_status[dvr_dev->display_buffer] &= ~ITU656_BUFFER_FULL_LCD;
            
		}

	}
	
end:
	dvr_dev->deinter_status = 0;	
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);

	return IRQ_HANDLED;
}

extern unsigned int g_screen_width;
extern unsigned int g_screen_height;

/***********************************************************************************************/
void dvr_set_sys_clk(int level)
{
        int val, width, height;

        width  = itu656_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
        height = itu656_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;
    
	spin_lock(&g_dvr_dev->spin_lock);
	if(level)
	{
		if(g_dvr_dev->enter_carback){//only carback support 1080p
		
                if((width == 800) || (height== 480)) val = 0;
                else val = 2;

                if(((itu656_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f) != val)
                        ark_sys_pad_config(ARK1668_SYS_DEVICE_CLK_CFG0, 0xf, 24, val);
                }
	}else{
		if(g_dvr_dev->enter_carback){//IntScal2Clk 

                if((width == 800) || (height== 480)) val = 2;
                else val = 4;
            
                if(((itu656_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f) != val)
                        ark_sys_pad_config(ARK1668_SYS_DEVICE_CLK_CFG0, 0xf, 24, val);
                }
	}
    
	spin_unlock(&g_dvr_dev->spin_lock);
}
EXPORT_SYMBOL(dvr_set_sys_clk);

int dvr_get_pragressive(void)
{
	int pragressive;

	spin_lock(&g_dvr_dev->spin_lock);
	pragressive = !g_dvr_dev->interlace;
	spin_unlock(&g_dvr_dev->spin_lock);
	
	return pragressive;
}
EXPORT_SYMBOL(dvr_get_pragressive);

void dvr_restart(void)
{
	struct itu656in_para para = {0};
	para.source = DVR_SOURCE_CAMERA;
    
        para.width  = itu656_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
        para.height = itu656_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;

	spin_lock(&g_dvr_dev->spin_lock);
	if(g_dvr_dev->enter_carback == 1)
	{
		g_dvr_dev->work_status = 0;
		del_timer(&g_dvr_dev->timer);
		g_dvr_dev->show_video = 0;
		ark_disp_set_layer_en(DISPLAY_LAYER, 0);
		ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_itu656_disable_write(); /*stop write data back*/
		ark_itu656_disable();
		msleep(100);
		if(g_dvr_dev->enter_carback == 1)
		{
			ark_itu656_reg_uninit();
			g_dvr_dev->carback_break = 1;
			if(g_dvr_dev->priv_data.get_progressive)
				para.progressive = g_dvr_dev->priv_data.get_progressive();
			dvr_init(g_dvr_dev, &para);
			dvr_start(g_dvr_dev);
		}
	}
	spin_unlock(&g_dvr_dev->spin_lock);
}
EXPORT_SYMBOL(dvr_restart);

int ark_get_carback_signal(void)
{
	return g_dvr_dev->carback_signal;
}
EXPORT_SYMBOL(ark_get_carback_signal);


/***********************************************************************************************/

int dvr_enter_carback(void)
{
	struct itu656in_para para = {0};
	para.source = DVR_SOURCE_CAMERA;
	//para.progressive = 1;

        if(!g_itu656in_priv->init){
                if(g_itu656in_priv->dvr_config() == 0){
                        g_itu656in_priv->init = 1;
                }

                g_itu656in_priv->select_channel(g_itu656in_priv->channel);
        }

        para.width  = itu656_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
        para.height = itu656_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;
    
	spin_lock(&g_dvr_dev->spin_lock);
	if(g_dvr_dev->priv_data.enter_carback_cb)
		g_dvr_dev->priv_data.enter_carback_cb();
	g_dvr_dev->clock_scale_store = ((itu656_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f);
    
	memcpy(&g_dvr_dev->itu656in_back, &g_dvr_dev->itu656in, sizeof(struct itu656in_para));
	if (g_dvr_dev->work_status) {
		g_dvr_dev->work_status = 0;
		del_timer(&g_dvr_dev->timer);
		g_dvr_dev->show_video = 0;
		ark_disp_set_layer_en(DISPLAY_LAYER, 0);
		ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_itu656_disable_write(); /*stop write data back*/
		ark_itu656_disable();
		g_dvr_dev->channel = ARK7116_AV0;
		if(g_dvr_dev->priv_data.select_channel)
			g_dvr_dev->priv_data.select_channel(g_dvr_dev->channel);
		msleep(200);
		ark_itu656_reg_uninit();
		g_dvr_dev->carback_break = 1;
	} else g_dvr_dev->carback_break = 0;
	pr_debug("%s carback_break=%d.\n", __FUNCTION__, g_dvr_dev->carback_break);
	if(g_dvr_dev->priv_data.get_progressive)
		para.progressive = g_dvr_dev->priv_data.get_progressive();
	dvr_init(g_dvr_dev, &para);	
	dvr_start(g_dvr_dev);
	g_dvr_dev->enter_carback = 1;
	
	if(g_dvr_dev->priv_data.dvr_start_cb)
		g_dvr_dev->priv_data.dvr_start_cb();
	
	spin_unlock(&g_dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_enter_carback);

int dvr_exit_carback(void)
{
	spin_lock(&g_dvr_dev->spin_lock);
	g_dvr_dev->enter_carback = 0;
	g_dvr_dev->work_status = 0;
	del_timer(&g_dvr_dev->timer);
	g_dvr_dev->show_video = 0;
	g_dvr_dev->carback_signal = 0;
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	msleep(20);
	ark_itu656_disable_write(); /*stop write data back*/
	ark_itu656_disable();
	if(((itu656_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f) != g_dvr_dev->clock_scale_store)
		ark_sys_pad_config(ARK1668_SYS_DEVICE_CLK_CFG0, 0xf, 24, g_dvr_dev->clock_scale_store);	//clock set back
	dvr_init(g_dvr_dev, &g_dvr_dev->itu656in_back);
	if(g_dvr_dev->priv_data.select_channel)
			g_dvr_dev->priv_data.select_channel(g_dvr_dev->channel);	
	msleep(100);
	ark_itu656_reg_uninit();
	pr_debug("%s carback_break=%d.\n", __FUNCTION__, g_dvr_dev->carback_break);
	if (g_dvr_dev->start_carback_exit) {
		dvr_start(g_dvr_dev);
		g_dvr_dev->start_carback_exit = 0;
	} else if (g_dvr_dev->carback_break) {
		if (g_dvr_dev->itu656in.tvout) {
			ark_disp_set_layer_en(TVOUT_LAYER, 0);	
			ark_disp_set_gui_tvout(1);
		}
	}
	
	if(g_dvr_dev->priv_data.dvr_stop_cb)
		g_dvr_dev->priv_data.dvr_stop_cb();
	if(g_dvr_dev->priv_data.exit_carback_cb)
		g_dvr_dev->priv_data.exit_carback_cb();
	
	spin_unlock(&g_dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_exit_carback);

int dvr_exit_wait(void)
{
	spin_lock(&g_dvr_dev->spin_lock);
	if (g_dvr_dev->carback_break) {
		spin_unlock(&g_dvr_dev->spin_lock);
		msleep(300);
		spin_lock(&g_dvr_dev->spin_lock);
		g_dvr_dev->carback_break = 0;
	}
	spin_unlock(&g_dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_exit_wait);

int dvr_detect_carback_signal(void)
{
	int signal = 0;
	
	spin_lock(&g_dvr_dev->spin_lock);
	if (g_dvr_dev->channel != ARK7116_AV0) {
		pr_debug("now is not in carback.\n");
	} else {
		if(g_dvr_dev->priv_data.detect_signal)
			signal = g_dvr_dev->priv_data.detect_signal();
	}
	g_dvr_dev->carback_signal = signal;
	spin_unlock(&g_dvr_dev->spin_lock);
	return signal;
}
EXPORT_SYMBOL(dvr_detect_carback_signal);

static irqreturn_t ark_itu656_int_handler(int irq, void *dev_id)
{
	u8 field;
	u32 intr_stat;
	unsigned long flags;
	struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	int deinter_type;
	unsigned int deintout_phyaddr;
	int timeout = 10000;
	int syschange_mask = TOTAL_LINE_CHANGED_INTERRUPT;
	int i;

	intr_stat = itu656_readl(ARK1668_ITU656_ISR);
	itu656_writel(ARK1668_ITU656_ICR, 0xFF);
	field = (intr_stat >> 8) & 0x1;

	spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	if (!dvr_dev->interlace)
		syschange_mask |= ACTIVE_PIX_CHANGED_INTERRUPT;

	if(intr_stat & syschange_mask)
	{
		dvr_dev->show_video = 0;
		ark_disp_set_layer_en(DISPLAY_LAYER, 0);
		if (dvr_dev->itu656in.tvout)
			ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_itu656_disable_write();
		mod_timer(&dvr_dev->timer, jiffies +  msecs_to_jiffies(50));
	}

	if (!dvr_dev->work_status) goto end;

	if (dvr_dev->system == PAL)
		field = !field;
	
	//printk(KERN_ALERT "ark_itu656_int_handler--intr_stat=0x%0x\n",intr_stat);

	if (intr_stat & FIELD_INTERRUPT) {
		//printk(KERN_ALERT"w 0x%x\n", dvr_dev->fieldbuf_phyaddr[dvr_dev->cur_buffer]);
		if(dvr_dev->interlace) {
			itu656_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->fieldbuf_phyaddr[dvr_dev->cur_buffer]);
			if (dvr_dev->itu656in.tvout) {
				int tvout_addr = dvr_dev->fieldbuf_phyaddr[(dvr_dev->cur_buffer + 3) & 3];
				if (field)
					ark_disp_set_tvout_next_oddfield_bufaddr(tvout_addr);
				else
					ark_disp_set_tvout_next_evenfield_bufaddr(tvout_addr);
			}
			dvr_dev->cur_buffer = (dvr_dev->cur_buffer + 1) & 0x3;

			if (dvr_dev->video_reinit && dvr_dev->discard_frame-- == 0) {
				//for vbox
				if(dvr_dev->deinter_indirect_show == 1)
					dvr_dev->show_video = 0;
				else
					dvr_dev->show_video = 1;
				dvr_dev->video_reinit = 0;		
			}
			
			if (dvr_dev->system == PAL) deinter_type = DEINTERLACE_TYPE_PAL;
			else deinter_type = DEINTERLACE_TYPE_NTSC;

			if (field) {
				dvr_dev->deinter_odd_even = DEINTERLACE_FIELD_ODD;
				deintout_phyaddr = dvr_dev->oddbuf_phyaddr[dvr_dev->write_buffer];
			} else {
				dvr_dev->deinter_odd_even = DEINTERLACE_FIELD_EVEN;
				deintout_phyaddr = dvr_dev->evenbuf_phyaddr[dvr_dev->write_buffer];
			}
			//for vbox
			if(dvr_dev->deinter_indirect_show){
				//deintout_phyaddr = dvr_dev->framebuf_phyaddr[dvr_dev->write_framebuf];
				if(dvr_dev->deinter_odd_even == DEINTERLACE_FIELD_EVEN){
					deintout_phyaddr = dvr_dev->framebuf_phyaddr[dvr_dev->write_framebuf];
				}else{
					goto end;
				}
			}
			
			//for mirror
			if(dvr_dev->mirror_type && dvr_dev->deinter_odd_even != DEINTERLACE_FIELD_EVEN){
				goto end;
			}
			
			while(dvr_dev->deinter_status && timeout--);
			dvr_dev->deinter_status = 1;
			deinterlace_process (
        					DEINTERLACE_LINE_SIZE_720H,
        					DEINTERLACE_DATA_MODE_422,
        					deinter_type,
        					dvr_dev->deinter_odd_even,
        					dvr_dev->fieldbuf_phyaddr[dvr_dev->cur_buffer],
        					dvr_dev->fieldbuf_phyaddr[(dvr_dev->cur_buffer + 1) & 3],
        					dvr_dev->fieldbuf_phyaddr[(dvr_dev->cur_buffer + 2) & 3],
        					deintout_phyaddr,
        					0,	  // for yuv420
        					0);   // for yuv420 
		} else {
			if(dvr_dev->deinter_indirect_show){//for vbox
				dvr_dev->framebuf_status[dvr_dev->write_framebuf] |= ITU656_BUFFER_FULL_APP;
				GetPingPongNextBuf(dvr_dev->write_framebuf,ITU656_FRAME_NUM);
				if(dvr_dev->framebuf_status[dvr_dev->write_framebuf])
				{
					GetPingPongPreBuf(dvr_dev->write_framebuf, ITU656_FRAME_NUM);
					dvr_dev->framebuf_status[dvr_dev->write_framebuf] &= ~ITU656_BUFFER_FULL_APP;
				}

				itu656_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->framebuf_phyaddr[dvr_dev->write_framebuf]);
				
				dvr_dev->frame_finish_count = 0;
				for(i = 0;i < ITU656_FRAME_NUM;i++){
					if(dvr_dev->framebuf_status[i] == ITU656_BUFFER_FULL_APP)
						dvr_dev->frame_finish_count++;
				}
				
				wake_up_interruptible(&dvr_dev->frame_finish_waitq);		
				if(dvr_dev->fasync_queue != NULL) {
					kill_fasync(&dvr_dev->fasync_queue, SIGIO, POLL_IN);
				}
				
			}else{
				dvr_dev->buf_status[dvr_dev->write_buffer] |= ITU656_BUFFER_FULL_LCD;
				GetPingPongNextBuf(dvr_dev->write_buffer,ITU656_BUFFER_NUM);
				if(dvr_dev->buf_status[dvr_dev->write_buffer])
				{
					GetPingPongPreBuf(dvr_dev->write_buffer, ITU656_BUFFER_NUM);
					dvr_dev->buf_status[dvr_dev->write_buffer] &= ~ITU656_BUFFER_FULL_LCD;
				}
				itu656_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->oddbuf_phyaddr[dvr_dev->write_buffer]);
			}
			
			if (dvr_dev->video_reinit && dvr_dev->discard_frame-- == 0) {
				if(dvr_dev->deinter_indirect_show == 1)
					dvr_dev->show_video = 0;
				else 
					dvr_dev->show_video = 1;
				dvr_dev->video_reinit = 0;
			}
		}
	}

end:
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);

    return IRQ_HANDLED;
}

void ark_itu656_display_int_handler(void)
{
	struct dvr_dev* dvr_dev = g_dvr_dev;
	unsigned long flags;
	//struct arkfb_window_addr display_addr = {0};
	
	if(!dvr_dev || !dvr_dev->work_status)
		return;

	spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	if (dvr_dev->show_video) {
		if(dvr_dev->priv_data.detect_signal && dvr_dev->priv_data.detect_signal())
		{
			pr_debug("ark_itu656_display_int_handler-->show_video\n");
			ark_disp_set_layer_en(DISPLAY_LAYER, 1);
			if (dvr_dev->itu656in.tvout)
				ark_disp_set_layer_en(TVOUT_LAYER, 1);
			dvr_dev->show_video = 0;
			dvr_dev->carback_signal = 1;
		}
		else
		{
			dvr_dev->carback_signal = 0;
			pr_debug("No signal detect.\n");
		}
		
	}
	
	//for vbox
	if(dvr_dev->deinter_indirect_show)
		goto end; 

	if (dvr_dev->interlace) {
		if(dvr_dev->display_odd_even) {		
			if(dvr_dev->mirror_type == MIRROR_TYPE_NONE)
				ark_itu656_display_addr(dvr_dev->evenbuf_phyaddr[dvr_dev->display_buffer]);
			dvr_dev->display_odd_even = 0;
		} else {
			dvr_dev->buf_status[dvr_dev->display_buffer] &= ~ITU656_BUFFER_FULL_LCD;
			GetPingPongNextBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
			if(dvr_dev->buf_status[dvr_dev->display_buffer] == 0)
			{
				GetPingPongPreBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
				dvr_dev->buf_status[dvr_dev->display_buffer] |= ITU656_BUFFER_FULL_LCD;
			}	
			
			ark_itu656_display_addr(dvr_dev->oddbuf_phyaddr[dvr_dev->display_buffer]);
			dvr_dev->display_odd_even = 1;
		}
	} else {
		dvr_dev->buf_status[dvr_dev->display_buffer] &= ~ITU656_BUFFER_FULL_LCD;
		GetPingPongNextBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
		if(dvr_dev->buf_status[dvr_dev->display_buffer] == 0)
		{
			GetPingPongPreBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
			dvr_dev->buf_status[dvr_dev->display_buffer] |= ITU656_BUFFER_FULL_LCD;
		}							
		ark_itu656_display_addr(dvr_dev->oddbuf_phyaddr[dvr_dev->display_buffer]);
	}
	
end:	
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);

    //return IRQ_HANDLED;
}
EXPORT_SYMBOL(ark_itu656_display_int_handler);

static ssize_t dvr_get(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int len = 0;
    //struct i2c_client *client = to_i2c_client(dev);
	//struct dvr_dev *dvr_dev = i2c_get_clientdata(client);
	//struct dvr_dev *dvr_dev = g_dvr_dev;
	
	return len;
}

static ssize_t dvr_set(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{	
    //struct i2c_client *client = to_i2c_client(dev);
	//struct dvr_dev *dvr_dev = i2c_get_clientdata(client);
	struct dvr_dev *dvr_dev = g_dvr_dev;
	
	if(dvr_dev)
	{
		if(!strncmp(buf, "start", 5)){
			if(dvr_dev->start)
			{
				dvr_dev->start(dvr_dev);
				if(dvr_dev->priv_data.dvr_start_cb)
					dvr_dev->priv_data.dvr_start_cb();
			}
		}

		if(!strncmp(buf, "stop", 4)){
			if(dvr_dev->stop)
			{
				dvr_dev->stop(dvr_dev);
				
				if(dvr_dev->priv_data.dvr_stop_cb)
					dvr_dev->priv_data.dvr_stop_cb();
			}
		}

		if(!strncmp(buf, "channel", 7)){
			int channel;
			sscanf(buf, "%*s%d", &channel);
			dvr_dev->channel = (u8)channel;
			//printk("set channel:%d\n", dvr_dev->channel);
			if(dvr_dev->priv_data.select_channel)
			{
				if(dvr_dev->priv_data.select_channel(dvr_dev->channel) < 0){
					pr_warn("set channel fail: channel(1 ~ 3):%d\n", channel);
				}
			}
		}

		if(!strncmp(buf, "source", 6)){
			int source;
			sscanf(buf, "%*s%d", &source);
			if (source == DVR_SOURCE_AUX)
			{
				dvr_dev->channel = ARK7116_AV1;
			}
			else if (source == DVR_SOURCE_CAMERA)
			{
				dvr_dev->channel = ARK7116_AV0;
			}
			else if (source == DVR_SOURCE_DVD)
			{
				dvr_dev->channel = ARK7116_AV2;
			}
			else
			{
				return count;
			}
			
			dvr_dev->itu656in.source = source;	
			if(dvr_dev->priv_data.select_channel)
			{
				if(dvr_dev->priv_data.select_channel(dvr_dev->channel) < 0){
					pr_warn("set channel fail: channel(1 ~ 3):%d\n", dvr_dev->channel);
				}
			}
		}
	}
    return count;
}

static DEVICE_ATTR(dvr, S_IWUSR | S_IRUGO,//static DEVICE_ATTR(dvr, S_IWUGO | S_IRUGO,
        dvr_get, dvr_set);

static struct attribute *dvr_sysfs_attrs[] = {
        &dev_attr_dvr.attr,
        NULL
};

static const struct attribute_group dvr_sysfs = {
        .attrs  = dvr_sysfs_attrs,
};

extern int ark_carback_get_status(void);
static long dvr_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
	//struct dvr_dev *dvr_dev =(struct dvr_dev *)filp->private_data;	
	struct dvr_dev *dvr_dev =g_dvr_dev;	
	//struct i2c_client *client;
	unsigned long error = 0;
	unsigned long flags;

	if(!dvr_dev){
		pr_err("%s error null device\n", __func__);
		return -ENXIO;
	}

	pr_debug("[DIAG_ITU656_IOCTL] cmd=0x%08x g_dvr_dev=%px filp->private_data=%px (%s) "
	       "start=%px (want %px, %s) stop=%px (want %px, %s)\n",
	       cmd, dvr_dev, filp->private_data,
	       (dvr_dev == filp->private_data) ? "MATCH" : "*** MISMATCH ***",
	       dvr_dev->start, dvr_start, (dvr_dev->start == dvr_start) ? "OK" : "*** CORRUPT ***",
	       dvr_dev->stop, dvr_stop, (dvr_dev->stop == dvr_stop) ? "OK" : "*** CORRUPT ***");
	pr_debug("[DIAG_ITU656_IOCTL] &priv_data=%px select_channel=%px detect_signal=%px "
	       "get_progressive=%px display_effect=%px%s dvr_start_cb=%px dvr_stop_cb=%px\n",
	       &dvr_dev->priv_data,
	       dvr_dev->priv_data.select_channel, dvr_dev->priv_data.detect_signal,
	       dvr_dev->priv_data.get_progressive, dvr_dev->priv_data.display_effect,
	       (dvr_dev->priv_data.display_effect &&
	        (unsigned long)dvr_dev->priv_data.display_effect == (unsigned long)dvr_dev)
	           ? " *** CORRUPT: equals dvr_dev base addr ***" : "",
	       dvr_dev->priv_data.dvr_start_cb, dvr_dev->priv_data.dvr_stop_cb);

	//client = dvr_dev->client;

#if 0//defined(CONFIG_ARK_CARBACK) || defined (CONFIG_ARK_CARBACK_MODULE)
	if (ark_carback_get_status()) {
		spin_lock_irqsave(&dvr_dev->spin_lock, flags);
		switch (cmd)
		{
			case ARK_DVR_START:	
				pr_debug("ARK_DVR_START when carback.\n");
				dvr_dev->start_carback_exit = 1;
				break;
			case ARK_DVR_STOP:	
				pr_debug("ARK_DVR_STOP when carback.\n");
				dvr_dev->start_carback_exit = 0;
				break;				
			case ARK_DVR_INIT:
				if(copy_from_user(&dvr_dev->itu656in_back, (void  *)arg, sizeof(struct itu656in_para))){
					pr_err("%s: copy from user para error\n", __func__);
					error = -EFAULT;
				}		
				break;
			case ARK_DVR_SWITCH_CHANNEL:
				if(copy_from_user(&dvr_dev->itu656in_back.source, (void  *)arg, sizeof(int))){
					pr_err("%s: copy from user frame error\n", __func__);
					error = -EFAULT;			
				}
				break;			
			case ARK_DVR_DETECT_SIGNAL:
			{
				unsigned int signal = 0;
				if(copy_to_user((void  *)arg, &signal, sizeof(signal))){
					pr_err("%s: copy to signal error\n", __func__);
					error = -EFAULT;			
				}
				break;
			}
			case ARK_DVR_SET_MIRROR_TYPE:
			{
				unsigned int type = 0;
				if(copy_from_user(&type, (void  *)arg, sizeof(int))){
					pr_err("%s: copy from user frame error\n", __func__);
					error = -EFAULT;			
				}
				
				if(dvr_dev->interlace && type >= MIRROR_TYPE_NONE && type < MIRROR_TYPE_END){
					dvr_dev->mirror_type = type;
					pr_debug("%s: mirror_type=%d\n", __func__, type);
				}

				break;
			}
			
	    }
		
		spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
		return error;
	}
		
#endif
	
	spin_lock_irqsave(&dvr_dev->spin_lock, flags);
	
	switch (cmd)
	{
		case ARK_DVR_START:
		{
			if(dvr_dev->start)
			{
				dvr_dev->start(dvr_dev);
				if(dvr_dev->priv_data.dvr_start_cb)
					dvr_dev->priv_data.dvr_start_cb();
			}
			break;
		}
		case ARK_DVR_STOP:
		{
			if(dvr_dev->stop)
			{
				dvr_dev->stop(dvr_dev);	
				if(dvr_dev->priv_data.dvr_stop_cb)
					dvr_dev->priv_data.dvr_stop_cb();
			}
			break;
		}
		case ARK_DVR_INIT:
		{
			struct itu656in_para para = {0};
			if(copy_from_user(&para, (void  *)arg, sizeof(struct itu656in_para))){
				pr_err("%s: copy from user para error\n", __func__);
				error = -EFAULT;
				goto end;
			}
			//para.progressive = 1;
			if(dvr_dev->priv_data.get_progressive)
				para.progressive = dvr_dev->priv_data.get_progressive();
			dvr_init(dvr_dev, &para);
			break;
		}
		case ARK_DVR_GETFRAME:
		{
			break;
		}
		case ARK_DVR_SWITCH_CHANNEL:
		{
			int source = 0;
			if(copy_from_user(&source, (void  *)arg, sizeof(source))){
				pr_err("%s: copy from user frame error\n", __func__);
				error = -EFAULT;
				goto end;				
			}
			if (source == DVR_SOURCE_AUX)
				dvr_dev->channel = ARK7116_AV1;
			else if (source == DVR_SOURCE_CAMERA)
				dvr_dev->channel = ARK7116_AV0;
			else if (source == DVR_SOURCE_DVD)
				dvr_dev->channel = ARK7116_AV2;
			if(dvr_dev->priv_data.select_channel)
			{
				if(dvr_dev->priv_data.select_channel(dvr_dev->channel) < 0){
					pr_warn("set channel fail: channel(1 ~ 3):%d\n", dvr_dev->channel);
				}
			}
			break;
		}
		case ARK_DVR_CHANNEL_STATUS:
		{
			unsigned int channel = dvr_dev->channel;
			if(copy_to_user((void  *)arg, &channel, sizeof(channel))){
				pr_err("%s: copy to user channel error\n", __func__);
				error = -EFAULT;
				goto end;				
			}
			break;
		}
		case ARK_DVR_TVOUT:
		{
			int tvout = 0;
			if(copy_from_user(&tvout, (void  *)arg, sizeof(tvout))){
				pr_err("%s: copy from user tvout error\n", __func__);
				error = -EFAULT;
				goto end;				
			}	
			if (tvout) {
				ark_disp_set_gui_tvout(0);
				dvr_config_tvout(dvr_dev);
				dvr_tvout_enable(dvr_dev, 1);
				dvr_dev->itu656in.tvout = 1;
			} else {
				ark_disp_set_layer_en(TVOUT_LAYER, 0);
				//dvr_tvout_enable(dvr_dev, 0);
				dvr_dev->itu656in.tvout = 0;
				ark_disp_set_gui_tvout(1);
			}
			break;
		}
		case ARK_DVR_DETECT_SIGNAL:
		{
			unsigned int signal = 0;
			
			if(dvr_dev->priv_data.detect_signal)
				signal = dvr_dev->priv_data.detect_signal();
			if(copy_to_user((void  *)arg, &signal, sizeof(signal))){
				pr_err("%s: copy to signal error\n", __func__);
				error = -EFAULT;
				goto end;				
			}	
			break;
		}
		//for vbox
		case ARK_DVR_INDIRECT_LCD:
		{
			int indirect;
			if(copy_from_user(&indirect, (void  *)arg, sizeof(int))){
				pr_err("%s: copy from user indirect error\n", __func__);
				error = -EFAULT;
				goto end;				
			}

			if(indirect == 1)
				dvr_dev->deinter_indirect_show = 1;
			else if(indirect == 2)
				dvr_dev->deinter_indirect_show = 2;
			else 
				dvr_dev->deinter_indirect_show = 0;
			
			break;
		}
		case ARK_DVR_GET_FRAME_INFO:
		{
			struct ark_frame_info  frame_info;
			
			memset(&frame_info,0,sizeof(struct ark_frame_info));
			frame_info.frame_max = ITU656_FRAME_NUM;
			memcpy(&frame_info.framebuf_phyaddr,&dvr_dev->framebuf_phyaddr,sizeof(dvr_dev->framebuf_phyaddr));			
			if(copy_to_user((void  *)arg, &frame_info, sizeof(struct ark_frame_info))){
				pr_err("%s: copy to signal error\n", __func__);
				error = -EFAULT;
				goto end;				
			}	
			break;
		}
		case ARK_DVR_SET_FRAME_COMPLETE:
		{
			int framebuf = 0;
			if(copy_from_user(&framebuf, (void  *)arg, sizeof(char))){
				pr_err("%s: copy from user ARK_DVR_SET_FRAME_COMPLETE error\n", __func__);
				error = -EFAULT;
				goto end;
			}			
			if (framebuf > ITU656_FRAME_NUM || framebuf < 0)
			{
				error = -EINVAL;
				goto end;
			}
			
			dvr_dev->framebuf_status[framebuf] &= (~ITU656_BUFFER_FULL_APP);
			break;
		}
		case ARK_DVR_SET_FRAME_SHOW:
		{
			unsigned int addr = 0;
			if(copy_from_user(&addr, (void  *)arg, sizeof(unsigned int))){
				pr_err("%s: copy from user ARK_DVR_SET_FRAME_SHOW error\n", __func__);
				error = -EFAULT;
				goto end;
			}	
			if (addr == 0)
			{
				error = -EINVAL;
				goto end;
			}
			
			if(dvr_dev->deinter_indirect_show == 1){
				itu656_writel(ARK1668_ITU656_ENABLE_REG, itu656_readl(ARK1668_ITU656_ENABLE_REG) | (CBCR_YUYV));
			}
			else if(dvr_dev->deinter_indirect_show == 2){
				itu656_writel_lcd(ARK1668_LCDC_VIDEO2_CTL, itu656_readl_lcd(ARK1668_LCDC_VIDEO2_CTL) | (1<<17));//yvyu
				if(dvr_dev->interlace){
                                        itu656_writel_lcd(ARK1668_LCDC_VIDEO2_CTL, itu656_readl_lcd(ARK1668_LCDC_VIDEO2_CTL) | (1<<9));
                                        itu656_writel_lcd(ARK1668_LCDC_TV_CONTROL, itu656_readl_lcd(ARK1668_LCDC_TV_CONTROL) | (1<<4) | (1<<8));
				}else{
                                        itu656_writel_lcd(ARK1668_LCDC_CONTROL, itu656_readl_lcd(ARK1668_LCDC_CONTROL) | (1<<7));
				}
			}
			itu656_writel_lcd(ARK1668_LCDC_VIDEO2_ADDR1, addr);
			break;
		}
		case ARK_DVR_SET_MIRROR_TYPE:
		{
			unsigned int type = 0;
			if(copy_from_user(&type, (void  *)arg, sizeof(int))){
				pr_err("%s: copy from user frame error\n", __func__);
				error = -EFAULT;			
			}
			if(dvr_dev->interlace && type >= MIRROR_TYPE_NONE && type < MIRROR_TYPE_END){
				dvr_dev->mirror_type = type;
				pr_debug("%s: mirror_type=%d\n", __func__, type);
			}
			break;
		}
		case ARK_DVR_GET_CVBS_TYPE:
		{
			if(copy_to_user((void  *)arg, &dvr_dev->system, sizeof(int))){
				pr_err("%s: copy to cvbs error\n", __func__);
				error = -EFAULT;
				goto end;				
			}	
			break;
		}
		case ARK_DVR_GET_BRIGHTNESS:
		case ARK_DVR_SET_BRIGHTNESS:
		case ARK_DVR_GET_CONTRAST:
		case ARK_DVR_SET_CONTRAST:
		case ARK_DVR_GET_SATURATION:
		case ARK_DVR_SET_SATURATION:
		case ARK_DVR_GET_HUE:
		case ARK_DVR_SET_HUE:
		case ARK_DVR_GET_SHARPNESS:
		case ARK_DVR_SET_SHARPNESS:
		{
			if(dvr_dev->priv_data.display_effect)
				error = dvr_dev->priv_data.display_effect(cmd, arg);
			if(error)
				goto end;

			break;
		}
		
		default:
			pr_err("%s: error cmd 0x%x\n", __func__, cmd);
			error = -EFAULT;
			goto end;
    }
	
end:
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
	
    return error;
}

static int dvr_open(struct inode *inode, struct file *filp)
{
	struct dvr_dev * dvr_dev =
		container_of(inode->i_cdev, struct dvr_dev, cdev);

	if(dvr_dev)
		filp->private_data = dvr_dev;
  	else
		pr_err("err %s, null cdev\n", __func__);

	pr_debug("[DIAG_ITU656_OPEN] container_of-dvr_dev=%px g_dvr_dev=%px (%s) "
	       "start=%px (want %px) stop=%px (want %px)\n",
	       dvr_dev, g_dvr_dev, (dvr_dev == g_dvr_dev) ? "MATCH" : "*** MISMATCH ***",
	       dvr_dev ? dvr_dev->start : NULL, dvr_start,
	       dvr_dev ? dvr_dev->stop : NULL, dvr_stop);

	return 0;
}

static int dvr_fasync(int fd, struct file * filp, int on) 
{
        struct dvr_dev *dvr_dev = (struct dvr_dev*)filp->private_data;
        int retval;
        
        retval=fasync_helper(fd,filp,on,&dvr_dev->fasync_queue);
        if(retval<0)
                return retval;
        return 0;
}

static int dvr_release(struct inode *inode, struct file *filp)
{
    if(filp->f_flags & FASYNC)
    {
        /* remove this filp from the asynchronusly notified filp's */
        dvr_fasync(-1, filp, 0);
    }
	
    return 0;
}
//for vbox
static ssize_t dvr_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
	struct dvr_dev *dvr_dev = (struct dvr_dev *)filp->private_data;
	int count;
	if (size > ITU656_FRAME_NUM)
		return -EINVAL;

	if(!dvr_dev->deinter_indirect_show)
		return -EINVAL;

	wait_event_interruptible(dvr_dev->frame_finish_waitq, dvr_dev->frame_finish_count > 0);
	spin_lock_irq(&dvr_dev->spin_lock);

	count = min((int)size, dvr_dev->frame_finish_count);
	memset(dvr_dev->frame_finish,0,sizeof(dvr_dev->frame_finish));
	if (count > 0) {
		int i;
		for (i = 0; i < count; i++) {
			//if(dvr_dev->framebuf_status[dvr_dev->get_framebuf] != ITU656_BUFFER_FULL_APP)break;
			dvr_dev->frame_finish[i] = dvr_dev->get_framebuf;
			GetPingPongNextBuf(dvr_dev->get_framebuf, ITU656_FRAME_NUM);
		}
		
		if(copy_to_user(user, dvr_dev->frame_finish, count))
		{
			pr_err("%s: copy from user para error\n", __func__);
		}
		dvr_dev->frame_finish_count -= count;
	}

	spin_unlock_irq(&dvr_dev->spin_lock);

	return count;
}

static unsigned int dvr_poll(struct file *filp, poll_table *wait)
{
	struct dvr_dev *dvr_dev = (struct dvr_dev *)filp->private_data;
	unsigned int mask = 0;

	spin_lock_irq(&dvr_dev->spin_lock);
	poll_wait(filp, &dvr_dev->frame_finish_waitq, wait);
	if (dvr_dev->frame_finish_count > 0)
		mask |= POLLIN | POLLRDNORM;
	spin_unlock_irq(&dvr_dev->spin_lock);

	return mask;
}

static struct file_operations dvr_fops = {
	.open			= dvr_open,
	.release		= dvr_release,
	.unlocked_ioctl	= dvr_ioctl,
	.fasync			= dvr_fasync,
	//for vbox
	.read			= dvr_read,
	.poll			= dvr_poll,
};

static int ark1668_itu656_probe(struct platform_device *pdev)//(struct i2c_client *client, struct ark_private_data *pdata)
{
        struct ark_private_data *pdata = g_itu656in_priv;
        struct i2c_client *client = g_itu656in_client;
        struct dvr_dev *dvr_dev;
        struct resource *res;
        void __iomem *regs;
        int value;
        int ret = -1;	
        dev_t dev;
    
	if(!pdata || !client)
		return 	-EINVAL;
        //printk(KERN_ALERT "+++%s start.\n", __func__);
        dvr_dev = devm_kzalloc(&pdev->dev, sizeof(struct dvr_dev), GFP_KERNEL);
        if (dvr_dev == NULL) {
        	dev_err(&pdev->dev, "%s %d: failed to allocate memory\n",
        	    __FUNCTION__, __LINE__);
        	return -ENOMEM;
        }
    
	{
		static int diag_probe_count = 0;
		diag_probe_count++;
		pr_info("[DIAG_ITU656_PROBE] call #%d: dvr_dev=%px sizeof(struct dvr_dev)=%zu\n",
		       diag_probe_count, dvr_dev, sizeof(struct dvr_dev));
		if (diag_probe_count > 1)
			pr_info("[DIAG_ITU656_PROBE] *** probe() called more than once -- "
			       "g_dvr_dev will be overwritten, any fd opened against an earlier "
			       "instance's cdev will resolve to a STALE dvr_dev via container_of "
			       "in dvr_open() while dvr_ioctl() uses the latest g_dvr_dev ***\n");
	}

	g_dvr_dev = dvr_dev;

	dvr_dev->name = "ark_dvr";
	dvr_dev->dev_major = DVR_MAJOR;
	dvr_dev->dev_minor= 0;
	dvr_dev->work_status = 0;
	dvr_dev->deinter_status = 0;
	dvr_dev->start = dvr_start;
	dvr_dev->stop = dvr_stop;
	pr_info("[DIAG_ITU656_PROBE] dvr_dev=%px &start=%px start=%px (want dvr_start=%px) "
	       "&stop=%px stop=%px (want dvr_stop=%px)\n",
	       dvr_dev, &dvr_dev->start, dvr_dev->start, dvr_start,
	       &dvr_dev->stop, dvr_dev->stop, dvr_stop);
	dvr_dev->client = client;
	dvr_dev->fasync_queue = NULL;
	dvr_dev->system = NTSC;
	dvr_dev->cur_buffer = 0;
	dvr_dev->write_buffer = 0;
	dvr_dev->display_buffer = 0;
	dvr_dev->carback_signal = 0;
	memcpy(&dvr_dev->priv_data, pdata, sizeof(struct ark_private_data));
	pr_info("[DIAG_ITU656_PROBE] priv_data after memcpy: &priv_data=%px "
	       "select_channel=%px (want %px) detect_signal=%px (want %px) "
	       "get_progressive=%px (want %px) display_effect=%px (want %px)\n",
	       &dvr_dev->priv_data,
	       dvr_dev->priv_data.select_channel, pdata->select_channel,
	       dvr_dev->priv_data.detect_signal, pdata->detect_signal,
	       dvr_dev->priv_data.get_progressive, pdata->get_progressive,
	       dvr_dev->priv_data.display_effect, pdata->display_effect);
	pr_info("[DIAG_ITU656_PROBE] priv_data after memcpy (cont): "
	       "dvr_start_cb=%px (want %px) dvr_stop_cb=%px (want %px) "
	       "enter_carback_cb=%px (want %px) exit_carback_cb=%px (want %px) "
	       "dvr_config=%px (want %px)\n",
	       dvr_dev->priv_data.dvr_start_cb, pdata->dvr_start_cb,
	       dvr_dev->priv_data.dvr_stop_cb, pdata->dvr_stop_cb,
	       dvr_dev->priv_data.enter_carback_cb, pdata->enter_carback_cb,
	       dvr_dev->priv_data.exit_carback_cb, pdata->exit_carback_cb,
	       dvr_dev->priv_data.dvr_config, pdata->dvr_config);

	dvr_dev->ic_type = pdata->ic_type;
	dvr_dev->old_cvbs_type = TYPE_UNKNOWN;
	//for vbox
	dvr_dev->write_framebuf = 0;
	dvr_dev->get_framebuf = 0;
	dvr_dev->deinter_indirect_show = 0;
	dvr_dev->frame_finish_count = 0;
	init_waitqueue_head(&dvr_dev->frame_finish_waitq);

	spin_lock_init(&dvr_dev->spin_lock);
	//mutex_init(&dvr_dev->mutex_lock);
	
	dev = MKDEV(dvr_dev->dev_major, dvr_dev->dev_minor);
	if ((ret= register_chrdev_region(dev, 1, "dvr")) != 0) {
		pr_err("unable to get major %d\n", DVR_MAJOR);
		goto err_regchr;
	}

	cdev_init(&dvr_dev->cdev, &dvr_fops);
	if ((ret= cdev_add(&dvr_dev->cdev, dev, 1)) != 0) {
		pr_err("dvr_dev: unable register character device\n");
		goto err_cdev_add;
	}

	ret = sysfs_create_group(&client->dev.kobj, &dvr_sysfs);
	if (ret){
		pr_err("error dvr sysfs_create\n");
		goto err_sysfs_create_group;
	}
    
#if ITU656_DEV_CLASS_CREATE
        dvr_dev->itu656_class = class_create(THIS_MODULE, "itu656_class");
        if(IS_ERR(dvr_dev->itu656_class)) {
                dev_err(&pdev->dev, "Err: failed in creating ark isp scale class.\n");
                dvr_dev->itu656_class = NULL;
                goto err_class_add;
        }

        dvr_dev->itu656_device = device_create(dvr_dev->itu656_class, NULL, dev, NULL, "dvr");
        if (IS_ERR(dvr_dev->itu656_device)) {
                dev_err(&pdev->dev, "Err: failed in creating ark isp scale device.\n");
                dvr_dev->itu656_device = NULL;
                goto err_class_add;
        }
#endif

        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (IS_ERR(res)) {
        	ret = PTR_ERR(res);
        	goto err_mem_res_itu;
        }
        regs = devm_ioremap_resource(&pdev->dev, res);
        if (IS_ERR(regs)) {
        	ret = PTR_ERR(regs);
        	goto err_mem_res_itu;
        }
        dvr_dev->context.itu656_base = regs;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
        if (IS_ERR(res)) {
        	ret = PTR_ERR(res);
        	goto err_mem_res_sys;
        }
        regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
        if (IS_ERR(regs)) {
        	ret = PTR_ERR(regs);
        	goto err_mem_res_sys;
        }
        dvr_dev->context.sys_base = regs;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
        if (IS_ERR(res)) {
        	ret = PTR_ERR(res);
        	goto err_mem_res_deinterlace;
        }
        regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
        if (IS_ERR(regs)) {
        	ret = PTR_ERR(regs);
        	goto err_mem_res_deinterlace;
        }
        dvr_dev->context.deinterlace_base = regs;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
        if (IS_ERR(res)) {
        	ret = PTR_ERR(res);
        	goto err_mem_res_lcd;
        }
        regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
        if (IS_ERR(regs)) {
        	ret = PTR_ERR(regs);
        	goto err_mem_res_lcd;
        }
        dvr_dev->context.lcd_base = regs;
    

        dvr_dev->context.itu656_irq = platform_get_irq(pdev, 0);
        if (dvr_dev->context.itu656_irq < 0) {
        	dev_err(&pdev->dev, "%s %d: can't get itu656_irq resource.\n", __FUNCTION__, __LINE__);
        	goto err_irq;
        }
        ret = devm_request_irq(
				&pdev->dev,
				dvr_dev->context.itu656_irq, 
				ark_itu656_int_handler, 
				IRQF_SHARED, 
				"dvr_itu656", 
				dvr_dev
				);
        if(ret){
        dev_err(&pdev->dev, "%s %d: can't get assigned itu656_irq %d, error %d\n",
            __FUNCTION__, __LINE__, dvr_dev->context.itu656_irq, ret);
        goto err_irq;
        }

        dvr_dev->context.deinterlace_irq = platform_get_irq(pdev, 1);
        if (dvr_dev->context.deinterlace_irq < 0) {
        	dev_err(&pdev->dev, "%s %d: can't get deinterlace_irq resource.\n", __FUNCTION__, __LINE__);
        	goto err_irq;
        }
        ret = devm_request_irq(
        			&pdev->dev,
        			dvr_dev->context.deinterlace_irq, 
        			ark_deinterlace_int_handler, 
        			IRQF_SHARED, 
        			"dvr_deinterlace", 
        			dvr_dev
        			);
        if(ret){
        dev_err(&pdev->dev, "%s %d: can't get assigned deinterlace_irq %d, error %d\n",
            __FUNCTION__, __LINE__, dvr_dev->context.deinterlace_irq, ret);
        goto err_irq;
        }
    
        if(dvr_dev->priv_data.support_max_resolution == TYPE_1080P)
                dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE_1080P*ITU656_BUFFER_NUM;
        else if(dvr_dev->priv_data.support_max_resolution == TYPE_720P)
                dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE*ITU656_FRAME_NUM;
        else
                dvr_dev->buffer_size = ITU656_FIELD_SIZE*16;

        dvr_dev->buffer_virtaddr = (void *)__get_free_pages(GFP_KERNEL, get_order(dvr_dev->buffer_size));
        if (!dvr_dev->buffer_virtaddr) {
                pr_err("%s get buffer fail\n", __func__);
                ret = -ENOMEM;
                goto err_get_buffer;
        }
        dvr_dev->buffer_phyaddr = virt_to_phys(dvr_dev->buffer_virtaddr);
    
	timer_setup(&dvr_dev->timer, dither_timeout_timer, 0);
	//i2c_set_clientdata(client, dvr_dev);
	//for mirror
	dvr_dev->mirror_queue = create_singlethread_workqueue("mirror_queue");
	if(!dvr_dev->mirror_queue) {
    	pr_err("%s %d: create_singlethread_workqueue fail.\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	INIT_WORK(&dvr_dev->mirror_work, mirror_paint_work);

        dvr_dev->channel = ITU656_CH1;
        if(!of_property_read_u32(pdev->dev.of_node, "channel", &value)) {
        //printk(KERN_ALERT "get itu channel=%d\n", value);
                if(value >= ITU656_CH0 && value <= ITU656_CH2)
                        dvr_dev->itu_channel = value;

        }

	dvr_dev->mirror_type = MIRROR_TYPE_NONE;
        if(!of_property_read_u32(pdev->dev.of_node, "mirror", &value)) {
                pr_debug("get mirror type=%d\n", value);
                if(value >= MIRROR_TYPE_NONE && value < MIRROR_TYPE_END)
                        dvr_dev->mirror_type = value;

        }
    
        carback_first_enter();

	pr_info("[DIAG_ITU656_PROBE_END] dvr_dev=%px start=%px (want %px) stop=%px (want %px)\n",
	       dvr_dev, dvr_dev->start, dvr_start, dvr_dev->stop, dvr_stop);

        return 0;

err_get_buffer:
        free_pages((unsigned long)dvr_dev->buffer_virtaddr, get_order(dvr_dev->buffer_size));
err_irq:
        iounmap(dvr_dev->context.lcd_base);
err_mem_res_lcd:
        iounmap(dvr_dev->context.deinterlace_base);    
err_mem_res_deinterlace:
        iounmap(dvr_dev->context.sys_base);
err_mem_res_sys:
err_mem_res_itu:
#if ITU656_DEV_CLASS_CREATE
        if (dvr_dev->itu656_class) {
        if (dvr_dev->itu656_device) {
        device_destroy(dvr_dev->itu656_class, dev);
        }
        class_destroy(dvr_dev->itu656_class);
        }
#endif
err_class_add:
        sysfs_remove_group(&client->dev.kobj, &dvr_sysfs);
err_sysfs_create_group:
        cdev_del(&dvr_dev->cdev);
err_cdev_add:
        unregister_chrdev_region(dev, 1);
err_regchr:
        pr_err("ark668 itu656 probe failed\n");

        return ret;	
}

static int ark1668_itu656_remove(struct platform_device *pdev)
{
	struct dvr_dev *dvr_dev = g_dvr_dev;
	struct i2c_client *client;
        dev_t dev;
	
	if (dvr_dev == NULL)
		return -ENODEV;	
    
	client = dvr_dev->client;
    
	free_pages((unsigned long)dvr_dev->buffer_virtaddr, get_order(dvr_dev->buffer_size));
        iounmap(dvr_dev->context.lcd_base);
        iounmap(dvr_dev->context.deinterlace_base);
        iounmap(dvr_dev->context.sys_base);
        dev = MKDEV(dvr_dev->dev_major, dvr_dev->dev_minor);
#if ITU656_DEV_CLASS_CREATE
        if (dvr_dev->itu656_class) {
        	if (dvr_dev->itu656_device) {
        		device_destroy(dvr_dev->itu656_class, dev);
        	}
        	class_destroy(dvr_dev->itu656_class);
        }
#endif
	sysfs_remove_group(&client->dev.kobj, &dvr_sysfs);
	cdev_del(&dvr_dev->cdev);
	unregister_chrdev_region(MKDEV(dvr_dev->dev_major, dvr_dev->dev_minor), 1);		
	g_dvr_dev = NULL;
	
	return 0;
}


static const struct of_device_id ark1668_itu656_of_match[] = {
	{ .compatible = "arkmicro,ark1668-itu656", },
	{ }
};
MODULE_DEVICE_TABLE(of, ark1668_itu656_of_match);

static struct platform_driver ark1668_itu656_driver = {
        .driver     = {
        .name   = "ark1668-itu656",
        	.of_match_table = of_match_ptr(ark1668_itu656_of_match),
        },
        .probe      = ark1668_itu656_probe,
        .remove     = ark1668_itu656_remove,
};
module_platform_driver(ark1668_itu656_driver);


MODULE_AUTHOR("Leo");
MODULE_DESCRIPTION("ArkMicro ark1668 Itu656 Driver");
MODULE_LICENSE("GPL v2");

