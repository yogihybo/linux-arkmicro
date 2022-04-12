/*
 * Arkmicro mcu serial driver
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/sizes.h>
#include <linux/io.h>
#include <linux/acpi.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>


#define LCD_BASE                                        0xE0500000
#define LCD_CONTROL                                     0x004
#define LCD_TIMING0                                     0x008
#define LCD_TIMING1                                     0x00c
#define LCD_TIMING2                                     0x010
#define LCD_TIMING3                                     0x014
#define LCD_VIDEO2_VP_REG_1                             0x134
#define LCD_OSD1_VP_REG_1                               0x144 
#define LCD_OSD2_VP_REG_1                               0x14c
#define LCD_OSD3_VP_REG_1                               0x154
#define LCD_VIDEO_VP_REG_1                              0x1d8
#define LCD_GAMMA_REG_0                                 0x1ec

#define TV_CONTROL                                      0x2b0
#define TV_TIMING0                                      0x2b4
#define TV_TIMING1                                      0x2b8
#define TV_TIMING2                                      0x2bc
#define TV_TIMING3                                      0x2c0
#define LCD_TIMING_FRAME_START_CNT_TV                   0x2c4

#define LCD_YPBPR_CTRL0                                 0x304
#define LCD_VIDEO_CTL                                   0x3c
#define LCD_OSD1_CTL                                    0x74
#define LCD_OSD2_CTL                                    0x88
#define LCD_OSD3_CTL                                    0x98
					
#define LCD_TV_PARAM_REG0                               0x800
#define LCD_TV_PARAM_REG1                               0x804
#define LCD_TV_PARAM_REG2                               0x808
#define LCD_TV_PARAM_REG3                               0x80C
#define LCD_TV_PARAM_REG4                               0x810
#define LCD_TV_PARAM_REG5                               0x814
#define LCD_TV_PARAM_REG6                               0x818
#define LCD_TV_PARAM_REG7                               0x81c
#define LCD_TV_PARAM_REG8                               0x820
#define LCD_TV_PARAM_REG9                               0x824
#define LCD_TV_PARAM_REG10                              0x828
#define LCD_TV_PARAM_REG11                              0x82c
#define LCD_TV_PARAM_REG12                              0x830
#define LCD_TV_PARAM_REG13                              0x834
#define LCD_TV_PARAM_REG14                              0x838
#define LCD_TV_PARAM_REG15                              0x83c
#define LCD_TV_PARAM_REG16                              0x840
#define LCD_TV_PARAM_REG17                              0x844
#define LCD_TV_PARAM_REG18                              0x848
#define LCD_TV_PARAM_REG19                              0x84c
#define LCD_TV_PARAM_REG20                              0x850
#define LCD_TV_PARAM_REG21                              0x854

#define ITU656_BYP_MODE_CONTROL                         0x3d0
#define ITU656_BYP_MODE_NTSC_REG0                       0x3d4
#define ITU656_BYP_MODE_NTSC_REG1                       0x3d8
#define ITU656_BYP_MODE_NTSC_REG2                       0x3dc
#define ITU656_BYP_MODE_NTSC_REG3                       0x3e0
#define ITU656_BYP_MODE_NTSC_REG4                       0x3e4
#define ITU656_BYP_MODE_PAL_REG0                        0x3e8
#define ITU656_BYP_MODE_PAL_REG1                        0x3ec
#define ITU656_BYP_MODE_PAL_REG2                        0x3f0
#define ITU656_BYP_MODE_PAL_REG3                        0x3f4
#define ITU656_BYP_MODE_PAL_REG4                        0x3f8
#define ITU656_BYP_MODE_REG0                            0x3fc

#define SYS_BASE                                        0xE4900000
#define SYS_LCD_CLOCK_CFG                               0x054
#define SYS_DEVICE_CLK_CFG2                             0x068
#define SYS_CLK_DLY_CFG                                 0x070
#define SYS_ANALOG_REG1                                 0x144
#define SYS_PLL_RFCK_CTL                                0x14c
#define SYS_CPUPLL_CFG                                  0x150
#define SYS_SYSPLL_CFG                                  0x154
#define SYS_LVDS_CTRL_CFG                               0x190
#define SYS_DDS_CLK_CFG                                 0x198


#define DMA_BASE				0xE0000000
#define GPU_BASE				0xE0F00000
#define I2S_BASE				0xE4000000
#define UART4_BASE				0xE4F00000
#define UART2_BASE				0xE8000000
#define UART3_BASE				0xE8100000
#define I2S2_BASE                               0xE8200000


//command id
#define READ_COMMAND_ID_BASE                            0x80
#define READ_REGISTER_ACCESS                            0x81
#define READ_TIMING_DEBUG                               0x82
#define READ_LVDS_SPECIAL                               0x83
#define READ_DISPLAY_SHOW                               0x84
#define READ_GAMMA_DEBUG                                0x85
#define READ_ITU656_DIRECT                              0x86
#define READ_CLOCK_DEBUG                                0x87
#define READ_SYSPLL_DDS                                 0x88
#define READ_COMMAND_ID_END                             0x9F

#define WRITE_COMMAND_ID_BASE                           0xB0
#define WRITE_REGISTER_ACCESS                           0xB1 
#define WRITE_TIMING_DEBUG                              0xB2
#define WRITE_LVDS_SPECIAL                              0xB3
#define WRITE_DISPLAY_SHOW                              0xB4
#define WRITE_GAMMA_DEBUG                               0xB5
#define WRITE_ITU656_DIRECT                             0xB6
#define WRITE_CLOCK_DEBUG                               0xB7
//#define WRITE_SYSPLL_COMMAND_ID                     0xB8
#define WRITE_DISPLAY_INTERFACE                         0xCE
#define WRITE_COMMAND_ID_END                            0xCF

#define REQUEST_READ_COMMAND_ID_BASE                    0xD0
#define REQUEST_READ_REGISTER_ACCESS                    0xD1
#define REQUEST_READ_TIMING_DEBUG                       0xD2
#define REQUEST_READ_LVDS_SPECIAL                       0xD3
#define REQUEST_READ_DISPLAY_SHOW                       0xD4
#define REQUEST_READ_GAMMA_DEBUG                        0xD5
#define REQUEST_READ_ITU656_DIRECT                      0xD6
#define REQUEST_READ_CLOCK_DEBUG                        0xD7
#define REQUEST_READ_SYSPLL_DDS                         0xD8

#define REQUEST_READ_COMMAND_ID_END                     0xEF

#define  CUSTOMER_ARK_FLAG                              0xAC

//显示层
#define DISP_VIDEO1                     (0x01)
#define DISP_VIDEO2                     (0x02)
#define DISP_OSD1                       (0x04)
#define DISP_OSD2                       (0x08)
#define DISP_OSD3                       (0x10)

#define BUF_SIZE_MAX                    255
#define GAMMA_REG_MAX                   48

#define PRINT_FLAG                      1

#define PAL                             0
#define NTSC                            1

//ScreenType
#define SCREEN_TYPE_RGB565		(1<<0)
#define SCREEN_TYPE_RGB888		(1<<1)	
#define SCREEN_TYPE_LVDS		(1<<2)
#define SCREEN_TYPE_VGA			(1<<3)
#define SCREEN_TYPE_CVBS		(1<<4)
#define SCREEN_TYPE_YPBPR		(1<<5)
#define SCREEN_TYPE_ITU656		(1<<6)
#define SCREEN_TYPE_ITU601		(1<<7)

//YPBPR
#define i480hz60			0  
#define i576hz50			1
#define p480hz60			2
#define p576hz50	 		3
#define p720hz60			4
#define p720hz50			5
#define i1080hz60			6
#define i1080hz50			7
#define i1080hz50_1250                  8
#define p1080hz60                       9
#define p1080hz50                       10


//TV encoder setting
#define 	 chroma_freq_palbg				0x2a098acb	//pal
#define 	 chroma_freq_palm				0x21e6efa4	//palm
#define 	 chroma_freq_palnc				0x21f69446	//palnc
#define 	 chroma_freq_ntsc				0x21f07c1f	//ntsc
#define 	 chroma_phase					0x2a
#define 	 clrbar_sel 					0
#define 	 clrbar_mode					0
#define 	 bypass_yclamp					0
#define 	 yc_delay                                       4
#define 	 cvbs_enable					1
#define 	 chroma_bw_1					0 // bw_1,bw_0 : 00: narrow band; 01: wide band; 10: extra wide; 11: ultra wide.
#define 	 chroma_bw_0					1
#define 	 comp_yuv                                       0
#define 	 compchgain 					0
#define 	 hsync_width					0x3f //0x7e*2
#define 	 burst_width					0x44   //pal 0x3e	  ntsc 0x44
#define 	 back_porch 					0x3b   //pal 0x45	  ntsc 0x3b
#define 	 cb_burst_amp					0x20
#define 	 cr_burst_amp					0x00   //pal 0x20	  ntsc 0x00
#define 	 slave_mode 					0x1
#define 	 black_level					0xf2
#define 	 blank_level					0xf0
#define 	 n1                                             0x17
#define 	 n3                                             0x21
#define 	 n8                                             0x1b
#define 	 n9                                             0x1b
#define 	 n10	                                        0x24
#define 	 num_lines                                      525   // pal: 625;	 ntsc: 525.
#define 	 n0                                             0x3e
#define 	 n13                                            0x0f
#define 	 n14                                            0x0f
#define 	 n15                                            0x60
#define 	 n5                                             0x05
#define 	 white_level					0x320
#define 	 cb_gain                                        0x89
#define 	 n20		                                0x04
#define 	 cr_gain		                        0x89
#define 	 n16                                            0x1
#define 	 n7                                             0x2
#define 	 tint                                           0
#define 	 n17                                            0x0a
#define 	 n19                                            0x05
#define 	 n18                                            0x00
#define 	 breeze_way 					0x16
#define 	 n21                                            0x3ff
#define 	 front_porch					0x10	//pal 0x0c	  ntsc 0x10   ??
#define 	 n11                                            0x7ce
#define 	 n12	                                        0x000
#define 	 activeline 					1440
#define 	 firstvideoline 				0xe
#define 	 uv_order                                       0
#define 	 pal_mode                                       0		//pal 0x1	 ntsc 0x0 
#define 	 invert_top 					0
#define 	 sys625_50                                      0
#define 	 cphase_rst 					3
#define 	 vsync5                                         1
#define 	 sync_level 					0x48
#define 	 n22                                            0
#define 	 agc_pulse_level				0xa3
#define 	 bp_pulse_level 				0xc8
#define 	 n4                                             0x15
#define 	 n6 	                                        0x05
#define 	 n2                                             0x15
#define 	 vbi_blank_level				0x128
#define 	 soft_rst                                       0
#define 	 row63	                                        0
#define 	 row64	                                        0x07
#define 	 wss_clock                                      0x2f7
#define 	 wss_dataf1 					0
#define 	 wss_dataf0 					0
#define 	 wss_linef1 					0
#define 	 wss_linef0 					0
#define 	 wss_level                                      0x3ff
#define 	 venc_en                                        1
#define 	 uv_first                                       0
#define 	 uv_flter_en					1
#define 	 notch_en                                       0
#define 	 notch_wide 					0
#define 	 notch_freq 					0
#define 	 row78	                                        0
#define 	 row79	                                        0
#define 	 row80	                                        0

//TV encoder setting pal
#define 	 burst_width_pal				0x3e   //pal 0x3e	  ntsc 0x44
#define 	 back_porch_pal 				0x45   //pal 0x45	  ntsc 0x3b
#define 	 cr_burst_amp_pal				0x20   //pal 0x20	  ntsc 0x00
#define 	 num_lines_pal					625   // pal: 625;	 ntsc: 525.
#define 	 front_porch_pal				0x0c	//pal 0x0c	  ntsc 0x10 
#define 	 pal_mode_pal					1		//pal 0x1	 ntsc 0x0 

#define ARKTOOL_DATA_NONE               0
#define ARKTOOL_DATA_START              1

typedef enum
{
        TV_PAL,
        TV_NTSC,
}TV_SYSTEM;

typedef enum
{
        REG_WRITE_READ = 1,
        LCD_TIMING_DEBUG,
        LVDS_SPECIAL,
        DISPLAY_SHOW,
        GAMMA_DEBUG,
        ITU656_DIRECT,
} FUNC_TYPE;

typedef enum
{
        CLK_24M=0,
        SYSPLL =1,
        DDS ,
        CPUPLL=4 ,

} CLOCK_TYPE;

struct tool_context
{
	//register access
	unsigned int reg_value;
	unsigned int reg_addr;

	//lcd timming
	unsigned int timing0_val;           
	unsigned int timing1_val;          
	unsigned int timing2_val;  
	
	//lcd clock debug
	unsigned int screen_type;	
	unsigned int subscreen_type;
	unsigned int interlace;
	unsigned int lcd_clock_config_val;
	unsigned int lcd_clk_dly_cfg_val;
	unsigned int lcd_clk_freq_val;

	//lcd  vp
	unsigned int lcd_con_val;
	unsigned int vp_val;     

	//lvds setting 
	unsigned int lvds_spec_val;

	//gamma
	unsigned int gamma_reg0_val;
	unsigned int gamma_val[GAMMA_REG_MAX];

	//itu656 bypress
	unsigned int tv_type;
	unsigned int itu656byp_conval;
	unsigned int itu656byp_npreg0val;   
	unsigned int itu656byp_npreg1val;    
	unsigned int itu656byp_npreg2val;   
	unsigned int itu656byp_npreg3val;  
	unsigned int itu656byp_npreg4val;  
	unsigned int itu656byp_reg0val; 

};

#define ARK_TOOL_DBG

#ifdef ARK_TOOL_DBG
#define TOOL_DBG(...) printk(KERN_ALERT __VA_ARGS__)
#else
#define TOOL_DBG(...)
#endif

#define RX_BUF_SIZE		PAGE_SIZE
#define ARK1668_LINUX_MIRCO

struct tool_serial_info {
        void __iomem *mmio;
        void __iomem *sysreg;
        struct tool_context context; 
        struct work_struct rx_task;
        spinlock_t lock;
        
        bool arktool_enable;

        unsigned int  recv_flag;
        unsigned int  recv;
        unsigned int  recv_len;
        unsigned char recv_buf[BUF_SIZE_MAX];
        
        unsigned int  request_read_id;
        unsigned char read_buf[BUF_SIZE_MAX];

        unsigned int  write_len;
        unsigned char write_buf[BUF_SIZE_MAX];
};


extern int tool_serial_send(const unsigned char *buf, int len);
extern void tool_serial_register_rev_handler(bool (*handler)(char ch), bool (*enable_check)(char ch));
extern void tool_serial_unregister_rev_handler(void);

static struct tool_serial_info *tsinfo;

#define tool_readl(reg)                 __raw_readl(tsinfo->mmio+(reg))
#define tool_writel(val, reg)           __raw_writel(val, tsinfo->mmio+(reg))
#define tool_readl_sys(reg)             __raw_readl(tsinfo->sysreg+(reg))
#define tool_writel_sys(val, reg)       __raw_writel(val, tsinfo->sysreg+(reg))

static void cvbs_ntsc_init(void)
{
        unsigned int val;

        val = chroma_freq_ntsc ;//此处定义 N制 P制
        tool_writel(val, LCD_TV_PARAM_REG0);

        val = chroma_bw_1<<27 | comp_yuv<<26|compchgain<<24|yc_delay<<17|cvbs_enable<<16|
                        clrbar_sel<<10|clrbar_mode<<9|bypass_yclamp<<8 | chroma_phase ;
        tool_writel(val, LCD_TV_PARAM_REG1);

        val = cb_burst_amp<<24 | back_porch<<16 | burst_width<<8 | hsync_width;
        tool_writel(val, LCD_TV_PARAM_REG2);

        val = black_level<< 16 | slave_mode<<8 | cr_burst_amp ;
        tool_writel(val, LCD_TV_PARAM_REG3);

        val = n3<<24| n1<<16 | blank_level ;
        tool_writel(val, LCD_TV_PARAM_REG4);

        val = n10<<24| n9<<16 | n8 ;
        tool_writel(val, LCD_TV_PARAM_REG5);

        val = num_lines ;
        tool_writel(val, LCD_TV_PARAM_REG6);

        val = n15<<24 | n14<<16| n13<<8 | n0 ;
        tool_writel(val, LCD_TV_PARAM_REG7);

        val = cb_gain<<24 | white_level<<8 | n5 ;
        tool_writel(val, LCD_TV_PARAM_REG8);

        val = n7<<24| n16 <<16 | cr_gain<<8 | n20 ;
        tool_writel(val, LCD_TV_PARAM_REG9);

        val = n18<<24| n19 <<16 | n17<<8| tint ;
        tool_writel(val, LCD_TV_PARAM_REG10);

        val = front_porch<<24| n21<<8| breeze_way ;
        tool_writel(val, LCD_TV_PARAM_REG11);

        val = n12 <<16     | n11 ;
        tool_writel(val, LCD_TV_PARAM_REG12);

        val = activeline ;
        tool_writel(val, LCD_TV_PARAM_REG13);

        val = n22<<24 | sync_level <<16 | uv_order<<15|pal_mode<<14|chroma_bw_0<<13|
                invert_top<<12|sys625_50<<11|cphase_rst<<9|vsync5<<8 | firstvideoline ;
        tool_writel(val, LCD_TV_PARAM_REG14);

        val = n6<<24 | n4 <<16 | bp_pulse_level<<8 | agc_pulse_level ;
        tool_writel(val, LCD_TV_PARAM_REG15);

        val = soft_rst<<24| vbi_blank_level<<8 | n2 ;
        tool_writel(val, LCD_TV_PARAM_REG16);

        val = row64 <<16| wss_clock ;
        tool_writel(val, LCD_TV_PARAM_REG17);

        val = wss_dataf1 ;
        tool_writel(val, LCD_TV_PARAM_REG18);

        val = wss_dataf0 ;
        tool_writel(val, LCD_TV_PARAM_REG19);

        val = wss_level <<16 | wss_linef0<<8| wss_linef1 ;
        tool_writel(val, LCD_TV_PARAM_REG20);

        val = row80<<24 | row79<<16 | row78<<8 | venc_en<<7 | uv_first <<6 |
                uv_flter_en<<5  |notch_en<<4 |  notch_wide<<3 | notch_freq ;
        tool_writel(val, LCD_TV_PARAM_REG21);

        TOOL_DBG("%s\n", __FUNCTION__);
}

static void cvbs_pal_init(void)
{
        unsigned int val;

        val = chroma_freq_palbg;
        tool_writel(val, LCD_TV_PARAM_REG0);

        val = chroma_bw_1<<27 | comp_yuv<<26|compchgain<<24|yc_delay<<17|cvbs_enable<<16|
                        clrbar_sel<<10|clrbar_mode<<9|bypass_yclamp<<8 | chroma_phase ;
        tool_writel(val, LCD_TV_PARAM_REG1);

        val = cb_burst_amp<<24 | back_porch_pal<<16 | burst_width_pal<<8 | hsync_width;
        tool_writel(val, LCD_TV_PARAM_REG2);

        val = black_level<< 16 | slave_mode<<8 | cr_burst_amp_pal;
        tool_writel(val, LCD_TV_PARAM_REG3);

        val = n3<<24| n1<<16 | blank_level ;
        tool_writel(val, LCD_TV_PARAM_REG4);

        val = n10<<24 | n9<<16 | n8 ;
        tool_writel(val, LCD_TV_PARAM_REG5);

        val = num_lines_pal ;
        tool_writel(val, LCD_TV_PARAM_REG6);

        val = n15<<24 | n14<<16| n13<<8 | n0 ;
        tool_writel(val, LCD_TV_PARAM_REG7);

        val = cb_gain<<24 | white_level<<8 | n5 ;
        tool_writel(val, LCD_TV_PARAM_REG8);

        val = n7<<24| n16 <<16 | cr_gain<<8 | n20 ;
        tool_writel(val, LCD_TV_PARAM_REG9);

        val = n18<<24| n19 <<16 | n17<<8| tint ;
        tool_writel(val, LCD_TV_PARAM_REG10);

        val = front_porch_pal<<24| n21<<8| breeze_way ;
        tool_writel(val, LCD_TV_PARAM_REG11);

        val = n12 <<16| n11 ;
        tool_writel(val, LCD_TV_PARAM_REG12);

        val = activeline ;
        tool_writel(val, LCD_TV_PARAM_REG13);

        val = n22<<24| sync_level <<16 | uv_order<<15|pal_mode_pal<<14|chroma_bw_0<<13|
                        invert_top<<12|sys625_50<<11|cphase_rst<<9|vsync5<<8| firstvideoline ;
        tool_writel(val, LCD_TV_PARAM_REG14);

        val = n6<<24| n4 <<16| bp_pulse_level<<8| agc_pulse_level ;
        tool_writel(val, LCD_TV_PARAM_REG15);

        val = soft_rst<<24| vbi_blank_level<<8| n2 ;
        tool_writel(val, LCD_TV_PARAM_REG16);

        val = row64 <<16  | wss_clock ;
        tool_writel(val, LCD_TV_PARAM_REG17);

        val = wss_dataf1 ;
        tool_writel(val, LCD_TV_PARAM_REG18);

        val = wss_dataf0 ;
        tool_writel(val, LCD_TV_PARAM_REG19);

        val = wss_level <<16 | wss_linef0<<8| wss_linef1 ;
        tool_writel(val, LCD_TV_PARAM_REG20);

        val = row80<<24 | row79<<16 | row78<<8|venc_en<<7| uv_first <<6 |uv_flter_en<<5|
                                                notch_en<<4 |notch_wide<<3 | notch_freq ;
        tool_writel(val, LCD_TV_PARAM_REG21);


        TOOL_DBG("%s\n", __FUNCTION__);

}

static void  ypbpr_config_interlace_mode(unsigned int mode)
{
	unsigned int scan_mode, val; 
        
	if(mode==i480hz60||mode==i576hz50||mode==i1080hz60||
                        mode==i1080hz50||mode==i1080hz50_1250){
                        
		scan_mode = 1;
	}else{
		scan_mode = 0;
	}

	//interlace, encoder_en
	val = tool_readl(TV_CONTROL);
        
	if(scan_mode == 1) {
		val |= (1<<8);
	}else{
		val &= ~(1<<8);
	}
        val |= (1<<0);
        tool_writel(val, TV_CONTROL);

        //Ypbpr
        val = 0;
        tool_writel(0, LCD_YPBPR_CTRL0);
	if(mode<=3){
		val = (1<<7)|(scan_mode<<6)|(mode<<2)|(1<<1)|(1<<0);
	}else{
		val = (0<<7)|(scan_mode<<6)|(mode<<2)|(1<<1)|(1<<0);
	}
        tool_writel(val, LCD_YPBPR_CTRL0);

	if(scan_mode == 1){
		tool_writel(tool_readl(LCD_VIDEO_CTL)| (1<<7),  LCD_VIDEO_CTL);
                tool_writel(tool_readl(LCD_OSD1_CTL) | (1<<26), LCD_OSD1_CTL);
                tool_writel(tool_readl(LCD_OSD2_CTL) | (1<<26), LCD_OSD2_CTL);
                tool_writel(tool_readl(LCD_OSD3_CTL) | (1<<26), LCD_OSD3_CTL);
	}else{                
		tool_writel(tool_readl(LCD_VIDEO_CTL)& ~(1<<7),  LCD_VIDEO_CTL);
                tool_writel(tool_readl(LCD_OSD1_CTL) & ~(1<<26), LCD_OSD1_CTL);
                tool_writel(tool_readl(LCD_OSD2_CTL) & ~(1<<26), LCD_OSD2_CTL);
                tool_writel(tool_readl(LCD_OSD3_CTL) & ~(1<<26), LCD_OSD3_CTL);
	}

        TOOL_DBG("%s mode=%d, scan_mode=%d\n", __FUNCTION__, mode, scan_mode);
		
}

static unsigned int  get_cpupll_frequency(void)
{
	unsigned int val,ref_clk,no,nf,enable,speed;
	
	speed = 0;
	val = tool_readl_sys(SYS_PLL_RFCK_CTL);
	ref_clk = 6000000 * ((val & 0x1) + 1);
	
	val = tool_readl_sys(SYS_CPUPLL_CFG);
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = (val & 0xFF)+1;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
		
	return speed;
}

static unsigned int  get_dds_frequency(void)
{
	unsigned int val, dto_inc, dds_cofe, dds_clk, div, scaler_factor;

	val = tool_readl_sys(SYS_DDS_CLK_CFG);
	TOOL_DBG("SYS_DDS_CLK_CFG = 0x%08x \n",val);

	dto_inc = val&0x3FFFFF;

	val = tool_readl_sys(SYS_ANALOG_REG1);
	TOOL_DBG("SYS_ANALOG_REG1 = 0x%08x \n",val);

	if(val & 0x4)
		scaler_factor = 64;
	else
		scaler_factor = 32;

	div = ((val >> 10) & 0x07);
	if(scaler_factor == 64 && div > 6)
		return 0;
	if(scaler_factor == 32 && div > 5)
		return 0;

	scaler_factor = scaler_factor >> div;

	dds_cofe = 1<<22;
	//dds_clk = (unsigned int)(24000000.0f*dto_inc*scaler_factor/(double)dds_cofe);// 24M*dto_inc*4.0f/dds_cofe
	dds_clk = (24*dto_inc*scaler_factor/dds_cofe)*1000000;
	
	TOOL_DBG("dds_clk = 24000000*dto_inc*scaler_factor/dds_cofe \n");
	TOOL_DBG("dds_clk=%d  dto_inc= %d  scaler_factor= %d dds_cofe= %d \n",
                                        dds_clk,dto_inc,scaler_factor,dds_cofe);

	return dds_clk;

}


static unsigned int get_syspll_frequency(void)
{
	unsigned int val, ref_clk, no, nf, enable, speed;

	speed = 0;
	val = tool_readl_sys(SYS_PLL_RFCK_CTL);
	TOOL_DBG("SYS_PLL_RFCK_CTL = 0x%08x \n",val);

	ref_clk = 6000000 * (((val>>3) & 0x1) + 1);

	val = tool_readl_sys(SYS_SYSPLL_CFG);
	TOOL_DBG("SYS_SYSPLL_CFG = 0x%08x \n",val);

	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;

	nf = (val & 0xFF)+1;

	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	TOOL_DBG("speed = ref_clk * nf /no \n");
	TOOL_DBG("speed=%d  ref_clk=%d  nf=%d  no=%d  \n",speed,ref_clk,nf,no);

	return speed;
}

static void set_dds_frequency(unsigned int dds_clk)
{
	unsigned int dto_inc, dds_cofe, val;

	val = tool_readl_sys(SYS_ANALOG_REG1);
	val &= ~(0x1<<2);// scaler_factor = 32
	val &= ~(0x07<<10); 
	val |= (1<<18); // enable	
	tool_writel_sys(val, SYS_ANALOG_REG1);
	TOOL_DBG("set SYS_ANALOG_REG1=0x%08x\n",val);

	dds_cofe = 1<<22;
	dds_clk /= 1000000;//lwang
	dto_inc = dds_cofe*dds_clk/(24*32);
	TOOL_DBG("dto_inc = dds_cofe*dds_clk/(24*32)\n");
	TOOL_DBG("dto_inc=%d  dds_cofe=%d dds_clk=%d\n",dto_inc,dds_cofe,dds_clk);

	val = tool_readl_sys(SYS_DDS_CLK_CFG);
	val &= ~(0x3fffff);
	val |= dto_inc;
        tool_writel_sys(val, SYS_DDS_CLK_CFG);
	TOOL_DBG("set SYS_DDS_CLK_CFG = 0x%08x\n",val);

}

static bool fill_trans_data(unsigned int cmd_id)
{
        struct tool_context *ptc = &tsinfo->context;
        unsigned char *pw = tsinfo->write_buf;
        unsigned char check_sum = 0;
        unsigned int  data_len = 0;
        bool ret = true;
        int i,j;

        //TOOL_DBG("%s: cmd_id = %0x.\n",__FUNCTION__, cmd_id);
        i = 0;
        memset(pw,0,BUF_SIZE_MAX);
        pw[i++] = CUSTOMER_ARK_FLAG;

	switch(cmd_id)
	{
	case REQUEST_READ_REGISTER_ACCESS:
		pw[i++] = 0x09;
		pw[i++] = READ_REGISTER_ACCESS;
		pw[i++] = (ptc->reg_addr >> 0)  & 0xFF;
		pw[i++] = (ptc->reg_addr >> 8)  & 0xFF;
		pw[i++] = (ptc->reg_addr >> 16) & 0xFF;
		pw[i++] = (ptc->reg_addr >> 24) & 0xFF;
		pw[i++] = (ptc->reg_value >> 0) & 0xFF;
		pw[i++] = (ptc->reg_value >> 8) & 0xFF;
		pw[i++] = (ptc->reg_value >> 16)& 0xFF;
		pw[i++] = (ptc->reg_value >> 24)& 0xFF;
		break;
	case REQUEST_READ_TIMING_DEBUG:
		pw[i++] = 0x0d;
		pw[i++] = READ_TIMING_DEBUG;
		pw[i++] = (ptc->timing0_val >> 0) & 0xFF;
		pw[i++] = (ptc->timing0_val >> 8) & 0xFF;
		pw[i++] = (ptc->timing0_val >> 16)& 0xFF;
		pw[i++] = (ptc->timing0_val >> 24)& 0xFF;
		pw[i++] = (ptc->timing1_val >> 0) & 0xFF;
		pw[i++] = (ptc->timing1_val >> 8) & 0xFF;
		pw[i++] = (ptc->timing1_val >> 16)& 0xFF;
		pw[i++] = (ptc->timing1_val >> 24)& 0xFF;
		pw[i++] = (ptc->timing2_val >> 0) & 0xFF;
		pw[i++] = (ptc->timing2_val >> 8) & 0xFF;
		pw[i++] = (ptc->timing2_val >> 16)& 0xFF;
		pw[i++] = (ptc->timing2_val >> 24)& 0xFF;
		break;
	case REQUEST_READ_LVDS_SPECIAL:
		pw[i++] = 0x05;
		pw[i++] = READ_LVDS_SPECIAL;
		pw[i++] = (ptc->lvds_spec_val >> 0) & 0xFF;
		pw[i++] = (ptc->lvds_spec_val >> 8) & 0xFF;
		pw[i++] = (ptc->lvds_spec_val >> 16)& 0xFF;
		pw[i++] = (ptc->lvds_spec_val >> 24)& 0xFF;
		break;
	case REQUEST_READ_DISPLAY_SHOW:
		pw[i++] = 0x05;
		pw[i++] = READ_DISPLAY_SHOW;
		pw[i++] = (ptc->vp_val >> 0) & 0xFF;
		pw[i++] = (ptc->vp_val >> 8) & 0xFF;
		pw[i++] = (ptc->vp_val >> 16)& 0xFF;
		pw[i++] = (ptc->vp_val >> 24)& 0xFF;
		break;
	case REQUEST_READ_GAMMA_DEBUG:
		pw[i++] = 48*4 +2;
		pw[i++] = READ_GAMMA_DEBUG;
		pw[i++] = ptc->gamma_reg0_val & 0x03;
		for (j = 0;j < GAMMA_REG_MAX ;j++)
		{
			pw[i++] = (ptc->gamma_val[j] >> 0) & 0xFF;
			pw[i++] = (ptc->gamma_val[j] >> 8) & 0xFF;
			pw[i++] = (ptc->gamma_val[j] >> 16)& 0xFF;
			pw[i++] = (ptc->gamma_val[j] >> 24)& 0xFF;

		}
		break;
	case REQUEST_READ_ITU656_DIRECT:
		pw[i++] = 0x1E;
		pw[i++] = READ_ITU656_DIRECT;
		pw[i++] = ptc->tv_type & 0xFF;
		pw[i++] = (ptc->itu656byp_conval >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_conval >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_conval >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_conval >> 24)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg0val >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg0val >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg0val >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg0val >> 24)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg1val >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg1val >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg1val >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg1val >> 24)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg2val >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg2val >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg2val >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg2val >> 24)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg3val >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg3val >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg3val >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg3val >> 24)& 0xFF;		
		pw[i++] = (ptc->itu656byp_npreg4val >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg4val >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_npreg4val >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_npreg4val >> 24)& 0xFF;
		pw[i++] = (ptc->itu656byp_reg0val >> 0) & 0xFF;
		pw[i++] = (ptc->itu656byp_reg0val >> 8) & 0xFF;
		pw[i++] = (ptc->itu656byp_reg0val >> 16)& 0xFF;
		pw[i++] = (ptc->itu656byp_reg0val >> 24)& 0xFF;
		break;
	case REQUEST_READ_CLOCK_DEBUG:
                pw[i++] = 0x0d;
                pw[i++] = READ_CLOCK_DEBUG;
                pw[i++] = (ptc->lcd_clock_config_val >> 0) & 0xFF;
                pw[i++] = (ptc->lcd_clock_config_val >> 8) & 0xFF;
                pw[i++] = (ptc->lcd_clock_config_val >> 16)& 0xFF;
                pw[i++] = (ptc->lcd_clock_config_val >> 24)& 0xFF;
                pw[i++] = (ptc->lcd_clk_dly_cfg_val >> 0)  & 0xFF;
                pw[i++] = (ptc->lcd_clk_dly_cfg_val >> 8)  & 0xFF;
                pw[i++] = (ptc->lcd_clk_dly_cfg_val >> 16) & 0xFF;
                pw[i++] = (ptc->lcd_clk_dly_cfg_val >> 24) & 0xFF;
                pw[i++] = (ptc->lcd_clk_freq_val >> 0) & 0xFF;
                pw[i++] = (ptc->lcd_clk_freq_val >> 8) & 0xFF;
                pw[i++] = (ptc->lcd_clk_freq_val >> 16)& 0xFF;
                pw[i++] = (ptc->lcd_clk_freq_val >> 24)& 0xFF;
		break;
	case REQUEST_READ_SYSPLL_DDS:
		pw[i++] = 0x05;
		pw[i++] = READ_SYSPLL_DDS;
		pw[i++] = (ptc->lcd_clk_freq_val >> 0)  & 0xFF;
		pw[i++] = (ptc->lcd_clk_freq_val >> 8)  & 0xFF;
		pw[i++] = (ptc->lcd_clk_freq_val >> 16) & 0xFF;
		pw[i++] = (ptc->lcd_clk_freq_val >> 24) & 0xFF;
		break;
	default:
	    ret = false;
		break;	

	}

	if ((REQUEST_READ_COMMAND_ID_BASE <= cmd_id) && (cmd_id <=REQUEST_READ_COMMAND_ID_END)){
                data_len = tsinfo->write_buf[1];
                tsinfo->write_len = data_len + 3;
                for (i = 0;i < data_len + 2;i++ )
                        check_sum ^= tsinfo->write_buf[i];

                tsinfo->write_buf[data_len+2] = check_sum;
                tsinfo->request_read_id = cmd_id;
                //TOOL_DBG("fill trans data: set request_read_id = %0x\n",tsinfo->request_read_id);
	}

	return ret;
}

static bool data_check_sum(unsigned char *buf)
{
        unsigned int data_len,i;
        unsigned char check_sum = 0;
        
	if (buf[0] != CUSTOMER_ARK_FLAG){
		TOOL_DBG("%s --> CUSTOMER_ARK_FLAG error.\n", __FUNCTION__);
		memset(buf,0,BUF_SIZE_MAX);
		return false;
	}

	data_len = buf[1];
	for (i = 0;i < data_len + 2;i++ ){
		check_sum ^= buf[i];	
		//TOOL_DBG("%02x, ",rbuf[i]);
		//if((i+1)%10 == 0 || (i >= data_len + 1))
			//TOOL_DBG("\r\n");
	}
	if (buf[data_len+2] != check_sum){
		TOOL_DBG("%s --> error, cmd_id=%0x length=%d. \n", __FUNCTION__ ,buf[2],buf[1]);
		memset(buf,0,BUF_SIZE_MAX);
		return false;
	}
        
	TOOL_DBG("\n ------------------------------------------- \n");
        
        return true;

}

static bool tool_data_process(void)
{
        unsigned int base_offset, val, i;
        unsigned int screen_type, subscreen_type;
        struct tool_context *ptc = &tsinfo->context;
        unsigned char *rbuf = tsinfo->read_buf;
        int lcd_clk_sel, command_id;
        bool ret = true;

        if(!data_check_sum(rbuf)) 
                return false;

	command_id  = rbuf[2];
	switch(command_id)
	{
	
	case WRITE_REGISTER_ACCESS:
        {
                unsigned int* pvirt_addr = NULL;
                
		ptc->reg_addr = (rbuf[3] << 0) | (rbuf[4] << 8) | (rbuf[5] << 16) | (rbuf[6] << 24);
		ptc->reg_value= (rbuf[7] << 0) | (rbuf[8] << 8) | (rbuf[9] << 16) | (rbuf[10] << 24);

		if (ptc->reg_addr >= LCD_BASE && ptc->reg_addr < LCD_BASE+0x2000){
			base_offset = ptc->reg_addr - LCD_BASE;
                        tool_writel(ptc->reg_value, base_offset);
		}else if(ptc->reg_addr >= SYS_BASE && ptc->reg_addr < SYS_BASE+0x1000){
			base_offset = ptc->reg_addr - SYS_BASE;
                        tool_writel_sys(ptc->reg_value, base_offset);
		}else if((ptc->reg_addr >= DMA_BASE && ptc->reg_addr < DMA_BASE+0x1000000)
			    ||(ptc->reg_addr >= I2S_BASE && ptc->reg_addr < I2S_BASE+0x1000000)
			    ||(ptc->reg_addr >= UART2_BASE && ptc->reg_addr < UART2_BASE+0x300000)){
			    
			pvirt_addr = (unsigned int*)ioremap(ptc->reg_addr, 0x04);
                        if(!pvirt_addr){
                                TOOL_DBG("WRITE_REGISTER --> ioremap reg_addr:0x%08x fail.\n",ptc->reg_addr);
                                break;
                        }
                        writel(ptc->reg_value, pvirt_addr);
			iounmap(pvirt_addr);
		}else{
			TOOL_DBG("WRITE_REGISTER -->not support\n");
			break;
		}

		TOOL_DBG("WRITE_REGISTER --> reg_addr:0x%08x=0x%08x\n",ptc->reg_addr,ptc->reg_value);
		break;
        }
                
	case WRITE_TIMING_DEBUG:
        {
                unsigned int hsw, hfp, hbp, vsw;
                
		ptc->timing0_val = (rbuf[3] << 0) | (rbuf[4] << 8) | (rbuf[5] << 16) | (rbuf[6] << 24);
		ptc->timing1_val = (rbuf[7] << 0) | (rbuf[8] << 8) | (rbuf[9] << 16) | (rbuf[10] << 24);
		ptc->timing2_val = (rbuf[11] << 0)| (rbuf[12] << 8)| (rbuf[13]<< 16) | (rbuf[14] << 24);

		screen_type = ptc->screen_type;
		subscreen_type = ptc->subscreen_type;
		TOOL_DBG("WRITE_TIMING -->screen_type=%d, subscreen_type=%d\n",screen_type,subscreen_type);
		if(screen_type==SCREEN_TYPE_RGB565||screen_type==SCREEN_TYPE_RGB888||
                        screen_type==SCREEN_TYPE_LVDS||screen_type==SCREEN_TYPE_ITU601){
#ifdef ARK1668_LINUX_MIRCO
                        hfp = ((ptc->timing0_val >> 0)  & 0x3ff) - 1;
                        hbp = ((ptc->timing0_val >> 10) & 0x3ff) - 1;
                        hsw = ((ptc->timing0_val >> 20) & 0x3ff) - 1;
                        ptc->timing0_val &= ~(0x3fffffff);
                        ptc->timing0_val |= hsw << 20 | hbp << 10 | hfp;

                        vsw =  ((ptc->timing1_val >> 13) & 0x3f) - 1;
                        ptc->timing1_val &= ~(0x0007E000);
                        ptc->timing1_val |= vsw << 13;
#endif
                        tool_writel(ptc->timing0_val, LCD_TIMING0); 
                        tool_writel(ptc->timing1_val, LCD_TIMING1); 
                        tool_writel(ptc->timing2_val, LCD_TIMING2); 
		}else if(screen_type==SCREEN_TYPE_VGA||screen_type==SCREEN_TYPE_CVBS||
		        screen_type==SCREEN_TYPE_YPBPR||screen_type==SCREEN_TYPE_ITU656){
#ifdef ARK1668_LINUX_MIRCO
                        if(screen_type!=SCREEN_TYPE_VGA){
                                hfp = ((ptc->timing0_val >> 0)  & 0x3ff) - 1;
                                hbp = ((ptc->timing0_val >> 10) & 0x3ff) - 1;
                                ptc->timing0_val &= ~(0xfffff);
                                ptc->timing0_val |=  hbp << 10 | hfp;

                        }        
#endif
                        tool_writel(ptc->timing0_val, TV_TIMING0); 
                        tool_writel(ptc->timing1_val, TV_TIMING1); 
                        tool_writel(ptc->timing2_val, TV_TIMING2); 
                        tool_writel(((ptc->timing1_val >> 13) & 0x3F)/2, LCD_TIMING_FRAME_START_CNT_TV);
		}
		
		if(screen_type == SCREEN_TYPE_ITU656){
			val = tool_readl(TV_CONTROL);
			val &= ~(0x01 << 1);
			if(ptc->subscreen_type == NTSC){
				val |= (0x01 << 1);
				TOOL_DBG("SCREEN_TYPE_ITU656 NTSC\n");
			}else{
				TOOL_DBG("SCREEN_TYPE_ITU656 PAL\n");
			}
                        tool_writel(val, TV_CONTROL); 
		}else if (screen_type == SCREEN_TYPE_YPBPR){
			ypbpr_config_interlace_mode(subscreen_type);
		}else if(screen_type == SCREEN_TYPE_CVBS){
			val = tool_readl(TV_CONTROL);
			if(ptc->subscreen_type == NTSC){
				val |= (0x01 << 1);
				cvbs_ntsc_init();
			}else{
				val &= ~(0x01 << 1);
				cvbs_pal_init();
			}
			tool_writel(val, TV_CONTROL); 
		}
		TOOL_DBG("0x%08x 0x%08x 0x%08x\n",ptc->timing0_val,ptc->timing1_val,ptc->timing2_val);
		break;
        }
        case WRITE_LVDS_SPECIAL:
		ptc->lvds_spec_val  = (rbuf[3] << 0) | (rbuf[4] << 8) | (rbuf[5] << 16) | (rbuf[6] << 24);
                tool_writel_sys(ptc->lvds_spec_val, SYS_LVDS_CTRL_CFG); 

		TOOL_DBG("WRITE_LVDS --> lvds_spec_val=0x%08x\n",ptc->lvds_spec_val);
		break;
                
	case WRITE_DISPLAY_SHOW:
		val = rbuf[3] & 0x1F;;	
		ptc->vp_val = (rbuf[4] << 0) | (rbuf[5] << 8) | (rbuf[6] << 16) | (rbuf[7] << 24);
	
		if (val == DISP_VIDEO1){
                        base_offset = LCD_VIDEO_VP_REG_1;
		}else if (val == DISP_VIDEO2){
		        base_offset = LCD_VIDEO2_VP_REG_1;
		}else if (val == DISP_OSD1){
		        base_offset = LCD_OSD1_VP_REG_1;
		}else if (val == DISP_OSD2){
		        base_offset = LCD_OSD2_VP_REG_1;
		}else if (val == DISP_OSD3){
		        base_offset = LCD_OSD3_VP_REG_1;
		}else{
			TOOL_DBG("WRITE_DISPLAY_SHOW: error. \n");
			break;
		}
		tool_writel(ptc->vp_val, base_offset); 
		TOOL_DBG("WRITE_DISPLAY_SHOW --> reg base_offset=0x%08x, vp_val=0x%08x\n",
                                                                base_offset,ptc->vp_val);
		break;
	case WRITE_GAMMA_DEBUG:
        {
                unsigned int cur_read;
                
		base_offset = LCD_GAMMA_REG_0; 
		ptc->gamma_reg0_val = rbuf[3];
		tool_writel(ptc->gamma_reg0_val, base_offset);
		base_offset += 4;
		TOOL_DBG("WRITE_GAMMA --> gamma_reg0_val=0x%0x\n",ptc->gamma_reg0_val);

		if(rbuf[3] == 0x03){
			for (i = 0;i < GAMMA_REG_MAX;i++){
				cur_read = 4*i;
				ptc->gamma_val[i] = (rbuf[4+cur_read] << 0) | (rbuf[5+cur_read] << 8) |
						    (rbuf[6+cur_read] << 16)| (rbuf[7+cur_read] << 24);
			}

			for (i = 0;i < GAMMA_REG_MAX;i++){
                                tool_writel(ptc->gamma_val[i], base_offset);
				base_offset += 4;	
			}
			
			//for (i = 0;i < GAMMA_REG_MAX;i++)
				//TOOL_DBG("WRITE_GAMMA --> gamma_val[%d]=0x%08x\n ",i,ptc->gamma_val[i]);
		}
		break;
        }
	case WRITE_ITU656_DIRECT:
		ptc->tv_type = rbuf[3] & 0xFF;
		ptc->itu656byp_conval    = (rbuf[4] << 0) | (rbuf[5] << 8) | (rbuf[6] << 16) | (rbuf[7] << 24);
		ptc->itu656byp_npreg0val = (rbuf[8] << 0) | (rbuf[9] << 8) | (rbuf[10] << 16)| (rbuf[11] << 24);
		ptc->itu656byp_npreg1val = (rbuf[12] << 0)| (rbuf[13] << 8)| (rbuf[14] << 16)| (rbuf[15] << 24);
		ptc->itu656byp_npreg2val = (rbuf[16] << 0)| (rbuf[17] << 8)| (rbuf[18] << 16)| (rbuf[19] << 24);
		ptc->itu656byp_npreg3val = (rbuf[20] << 0)| (rbuf[21] << 8)| (rbuf[22] << 16)| (rbuf[23] << 24);
		ptc->itu656byp_npreg4val = (rbuf[24] << 0)| (rbuf[25] << 8)| (rbuf[26] << 16)| (rbuf[27] << 24);
		ptc->itu656byp_reg0val   = (rbuf[28] << 0)| (rbuf[29] << 8)| (rbuf[30] << 16)| (rbuf[31] << 24);

		TOOL_DBG("WRITE_ITU656 --> tv_type:%d 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",
                                ptc->tv_type,ptc->itu656byp_conval,ptc->itu656byp_npreg0val, 
                                ptc->itu656byp_npreg1val,ptc->itu656byp_npreg2val,ptc->itu656byp_npreg3val, 
                                ptc->itu656byp_npreg4val,ptc->itu656byp_reg0val );

                tool_writel(ptc->itu656byp_conval, ITU656_BYP_MODE_CONTROL); 
                tool_writel(ptc->itu656byp_reg0val, ITU656_BYP_MODE_REG0); 
		if(ptc->tv_type == TV_NTSC){	
                        tool_writel(ptc->itu656byp_npreg0val, ITU656_BYP_MODE_NTSC_REG0); 
                        tool_writel(ptc->itu656byp_npreg1val, ITU656_BYP_MODE_NTSC_REG1); 
                        tool_writel(ptc->itu656byp_npreg2val, ITU656_BYP_MODE_NTSC_REG2); 
                        tool_writel(ptc->itu656byp_npreg3val, ITU656_BYP_MODE_NTSC_REG3); 
                        tool_writel(ptc->itu656byp_npreg4val, ITU656_BYP_MODE_NTSC_REG4); 
		}else if(ptc->tv_type == TV_PAL){
		
                        tool_writel(ptc->itu656byp_npreg0val, ITU656_BYP_MODE_PAL_REG0); 
                        tool_writel(ptc->itu656byp_npreg1val, ITU656_BYP_MODE_PAL_REG1); 
                        tool_writel(ptc->itu656byp_npreg2val, ITU656_BYP_MODE_PAL_REG2); 
                        tool_writel(ptc->itu656byp_npreg3val, ITU656_BYP_MODE_PAL_REG3); 
                        tool_writel(ptc->itu656byp_npreg4val, ITU656_BYP_MODE_PAL_REG4); 
		}
		break;
	case WRITE_CLOCK_DEBUG:
                lcd_clk_sel = -1;
		ptc->lcd_clock_config_val = (rbuf[3] << 0) | (rbuf[4] << 8) | (rbuf[5] << 16) | (rbuf[6]  << 24);
		ptc->lcd_clk_dly_cfg_val  = (rbuf[7] << 0) | (rbuf[8] << 8) | (rbuf[9] << 16) | (rbuf[10] << 24);
		ptc->lcd_clk_freq_val     = (rbuf[11]<< 0) |(rbuf[12] << 8) | (rbuf[13]<< 16) | (rbuf[14] << 24);

                tool_writel_sys(ptc->lcd_clock_config_val, SYS_LCD_CLOCK_CFG);
                tool_writel_sys(ptc->lcd_clk_dly_cfg_val, SYS_CLK_DLY_CFG);
		TOOL_DBG("WRITE_CLOCK --> lcd_clock_config_val=0x%08x  lcd_clk_dly_cfg_val=0x%08x\n",\
			                                ptc->lcd_clock_config_val,ptc->lcd_clk_dly_cfg_val);

		screen_type = ptc->screen_type;
		if (screen_type == SCREEN_TYPE_LVDS||screen_type==SCREEN_TYPE_RGB565||
                        screen_type==SCREEN_TYPE_RGB888||screen_type==SCREEN_TYPE_ITU601){
			lcd_clk_sel = (ptc->lcd_clock_config_val >> 7) & 0x0F;
		}else if(screen_type==SCREEN_TYPE_YPBPR||screen_type==SCREEN_TYPE_VGA||
	                screen_type==SCREEN_TYPE_CVBS||screen_type==SCREEN_TYPE_ITU656){
	                
			val = tool_readl_sys(SYS_DEVICE_CLK_CFG2);
			val &= ~(0xF << 20);
			val |= (DDS << 20);
			tool_writel_sys(val, SYS_DEVICE_CLK_CFG2);
			lcd_clk_sel = DDS;
		}
	
		if (lcd_clk_sel == DDS){
			TOOL_DBG("WRITE_CLOCK -->  DDS set lcd_clk_freq_val=%d\n",ptc->lcd_clk_freq_val);
			set_dds_frequency(ptc->lcd_clk_freq_val);
		} 
		break;
                
	case WRITE_DISPLAY_INTERFACE:
		ptc->screen_type = rbuf[3];
		ptc->subscreen_type = rbuf[4];
		ptc->interlace = rbuf[5];
		TOOL_DBG("WRITE_DISPLAY_INTERFACE --> screen_type = 0x%08x,subscreen_type=0x%08x,interlace=%d\n",
                                                        ptc->screen_type,ptc->subscreen_type,ptc->interlace);
		break;
                
	case REQUEST_READ_REGISTER_ACCESS:
        {
                unsigned int* pvirt_addr = NULL;
                
		ptc->reg_addr  = (rbuf[3] << 0) | (rbuf[4] << 8) | (rbuf[5] << 16) | (rbuf[6] << 24);
		if (ptc->reg_addr >= LCD_BASE && ptc->reg_addr < LCD_BASE+0x2000){
			base_offset = ptc->reg_addr - LCD_BASE;
			ptc->reg_value = tool_readl(base_offset);

		}else if(ptc->reg_addr >= SYS_BASE && ptc->reg_addr < SYS_BASE+0x1000){
			base_offset = ptc->reg_addr - SYS_BASE;
			ptc->reg_value = tool_readl_sys(base_offset);

		}else if((ptc->reg_addr >= DMA_BASE && ptc->reg_addr < DMA_BASE+0x1000000)
			    ||(ptc->reg_addr >= I2S_BASE && ptc->reg_addr < I2S_BASE+0x1000000)
			    ||(ptc->reg_addr >= UART2_BASE && ptc->reg_addr < UART2_BASE+0x300000)){
			    
			pvirt_addr = (unsigned int*)ioremap(ptc->reg_addr, 0x04);
                        if(!pvirt_addr){
                                TOOL_DBG("WRITE_REGISTER --> ioremap reg_addr:0x%08x fail.\n",ptc->reg_addr);
                                break;
                        }
			ptc->reg_value = readl(pvirt_addr);
			iounmap(pvirt_addr);

		}else{
			TOOL_DBG("REQUEST_READ_REGISTER <-- reg_addr:0x%08x not support\n",ptc->reg_addr);
			break;
		}

		TOOL_DBG("REQUEST_READ_REGISTER <-- reg_addr:0x%08x=0x%08x\n",ptc->reg_addr,ptc->reg_value);
		fill_trans_data(REQUEST_READ_REGISTER_ACCESS);
		break;
        }
	case REQUEST_READ_TIMING_DEBUG:
        {
                unsigned int hsw, hfp, hbp, vsw;   
                
		screen_type = ptc->screen_type;
		if(screen_type==SCREEN_TYPE_VGA||screen_type==SCREEN_TYPE_CVBS||
                        screen_type==SCREEN_TYPE_YPBPR||screen_type==SCREEN_TYPE_ITU656){
			ptc->timing0_val = tool_readl(TV_TIMING0);
			ptc->timing1_val = tool_readl(TV_TIMING1);
			ptc->timing2_val = tool_readl(TV_TIMING2);
#ifdef ARK1668_LINUX_MIRCO
                        if(screen_type!=SCREEN_TYPE_VGA){
                                hfp = ((ptc->timing0_val >> 0)  & 0x3ff) + 1;
                                hbp = ((ptc->timing0_val >> 10) & 0x3ff) + 1;
                                ptc->timing0_val &= ~(0xfffff);
                                ptc->timing0_val |=  hbp << 10 | hfp;
                        }        
#endif
			TOOL_DBG("REQUEST_READ_TIMING <-- TVtiming \n");
		}else{
			ptc->timing0_val = tool_readl(LCD_TIMING0);
			ptc->timing1_val = tool_readl(LCD_TIMING1);
			ptc->timing2_val = tool_readl(LCD_TIMING2);
			TOOL_DBG("REQUEST_READ_TIMING <-- LCDtiming \n");
                        
#ifdef ARK1668_LINUX_MIRCO
                        hfp = ((ptc->timing0_val >> 0)  & 0x3ff) + 1;
                        hbp = ((ptc->timing0_val >> 10) & 0x3ff) + 1;
                        hsw = ((ptc->timing0_val >> 20) & 0x3ff) + 1;
                        ptc->timing0_val &= ~(0x3fffffff);
                        ptc->timing0_val |= hsw << 20 | hbp << 10 | hfp;
        
                        vsw = ((ptc->timing1_val >> 13) & 0x3f) + 1;
                        ptc->timing1_val &= ~(0x0007E000);
                        ptc->timing1_val |= vsw << 13;
#endif                
		}

		TOOL_DBG("0x%08x 0x%08x 0x%08x\n",ptc->timing0_val,ptc->timing1_val,ptc->timing2_val);
		fill_trans_data(REQUEST_READ_TIMING_DEBUG);
       }
		break;
                
	case REQUEST_READ_LVDS_SPECIAL:
		ptc->lvds_spec_val = tool_readl_sys(SYS_LVDS_CTRL_CFG);
		TOOL_DBG("REQUEST_READ_LVDS <-- 0x%08x\n",ptc->lvds_spec_val);
                
		fill_trans_data(REQUEST_READ_LVDS_SPECIAL);
		break;
                
	case REQUEST_READ_DISPLAY_SHOW:
		val  = rbuf[3] & 0x1F;
		if (val == DISP_VIDEO1){
                        base_offset = LCD_VIDEO_VP_REG_1;
		}else if (val == DISP_VIDEO2){
		        base_offset = LCD_VIDEO2_VP_REG_1;
		}else if (val == DISP_OSD1){
		        base_offset = LCD_OSD1_VP_REG_1;
		}else if (val == DISP_OSD2){
		        base_offset = LCD_OSD2_VP_REG_1;
		}else if (val == DISP_OSD3){
		        base_offset = LCD_OSD3_VP_REG_1;
		}else{
			TOOL_DBG("REQUEST_READ_DISPLAY: type error!\n");
			break;
		}
                
                ptc->vp_val = tool_readl(base_offset);
		TOOL_DBG("REQUEST_READ_DISPLAY <-- base_offset=0x%08x  vp_val=0x%08x\n",base_offset, ptc->vp_val);
		fill_trans_data(REQUEST_READ_DISPLAY_SHOW);
		break;
                
	case REQUEST_READ_GAMMA_DEBUG:
		base_offset = LCD_GAMMA_REG_0;
		ptc->gamma_reg0_val = tool_readl(base_offset);
		base_offset += 4;
		for (i = 0;i < GAMMA_REG_MAX ;i++){
			ptc->gamma_val[i] = tool_readl(base_offset);
			base_offset += 4;
		}
		for (i = 0;i < GAMMA_REG_MAX;i++)
			TOOL_DBG("REQUEST_READ_GAMMA <-- gamma_val[%d]=0x%08x\n",i,ptc->gamma_val[i]);

		fill_trans_data(REQUEST_READ_GAMMA_DEBUG);
		break;
                
	case REQUEST_READ_ITU656_DIRECT:
		ptc->tv_type = rbuf[3] & 0xFF;
		ptc->itu656byp_conval  = tool_readl(ITU656_BYP_MODE_CONTROL);	
		ptc->itu656byp_reg0val = tool_readl(ITU656_BYP_MODE_REG0);
		if(ptc->tv_type == TV_NTSC){	
			ptc->itu656byp_npreg0val = tool_readl(ITU656_BYP_MODE_NTSC_REG0);  
			ptc->itu656byp_npreg1val = tool_readl(ITU656_BYP_MODE_NTSC_REG1);  
			ptc->itu656byp_npreg2val = tool_readl(ITU656_BYP_MODE_NTSC_REG2);  
			ptc->itu656byp_npreg3val = tool_readl(ITU656_BYP_MODE_NTSC_REG3);  
			ptc->itu656byp_npreg4val = tool_readl(ITU656_BYP_MODE_NTSC_REG4);  
		}else if(ptc->tv_type == TV_PAL){
			ptc->itu656byp_npreg0val = tool_readl(ITU656_BYP_MODE_PAL_REG0);  
			ptc->itu656byp_npreg1val = tool_readl(ITU656_BYP_MODE_PAL_REG1);  
			ptc->itu656byp_npreg2val = tool_readl(ITU656_BYP_MODE_PAL_REG2);  
			ptc->itu656byp_npreg3val = tool_readl(ITU656_BYP_MODE_PAL_REG3);  
			ptc->itu656byp_npreg4val = tool_readl(ITU656_BYP_MODE_PAL_REG4);  
		}
		
                TOOL_DBG("REQUEST_READ_ITU656 <-- tv_type:%d 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n",\
                        ptc->tv_type,ptc->itu656byp_conval,ptc->itu656byp_npreg0val,
                        ptc->itu656byp_npreg1val,ptc->itu656byp_npreg2val,ptc->itu656byp_npreg3val,
                        ptc->itu656byp_npreg4val,ptc->itu656byp_reg0val );

		fill_trans_data(REQUEST_READ_ITU656_DIRECT);
		break;
                
	case REQUEST_READ_CLOCK_DEBUG:
                lcd_clk_sel = -1;
		screen_type = ptc->screen_type;
		ptc->lcd_clock_config_val = tool_readl_sys(SYS_LCD_CLOCK_CFG);
		ptc->lcd_clk_dly_cfg_val  = tool_readl_sys(SYS_CLK_DLY_CFG);
		TOOL_DBG("REQUEST_READ_CLOCK <-- lcd_clock_config_val=0x%08x,lcd_clk_dly_cfg_val=0x%08x\n",
                                                ptc->lcd_clock_config_val,ptc->lcd_clk_dly_cfg_val);
                if((ptc->lcd_clock_config_val & 0x70) == 0)
                        ptc->lcd_clock_config_val |= (0x01 << 4);

		if (screen_type== SCREEN_TYPE_LVDS||screen_type==SCREEN_TYPE_RGB565||
                        screen_type==SCREEN_TYPE_RGB888||screen_type== SCREEN_TYPE_ITU601){
                        
			lcd_clk_sel = (ptc->lcd_clock_config_val >> 7) & 0x0F;
		}else if(screen_type== SCREEN_TYPE_YPBPR||screen_type== SCREEN_TYPE_VGA||
		        screen_type==SCREEN_TYPE_CVBS||screen_type==SCREEN_TYPE_ITU656){
		        
			val = tool_readl_sys(SYS_DEVICE_CLK_CFG2);
			lcd_clk_sel = (val >> 20) & 0x0F;
			lcd_clk_sel = DDS;
		}

		if (lcd_clk_sel == SYSPLL){
			ptc->lcd_clk_freq_val = get_syspll_frequency();
			TOOL_DBG("SYSPLL lcd_clk_freq_val=%d\n",ptc->lcd_clk_freq_val);
		}else if (lcd_clk_sel == DDS){
			ptc->lcd_clk_freq_val =  get_dds_frequency();
			TOOL_DBG("DDS lcd_clk_freq_val=%d\n",ptc->lcd_clk_freq_val);
		}else if (lcd_clk_sel == CPUPLL){
			ptc->lcd_clk_freq_val = get_cpupll_frequency();
			TOOL_DBG("CPU lcd_clk_freq_val=%d\n",ptc->lcd_clk_freq_val);
		}else{
			TOOL_DBG("error! \n");
			break;
		}
		
		fill_trans_data(REQUEST_READ_CLOCK_DEBUG);
		break;
                
	case REQUEST_READ_SYSPLL_DDS:
		lcd_clk_sel  = rbuf[3];
		TOOL_DBG("REQUEST_READ_SYSPLL_DDS <-- lcd_clk_sel=%0x \n",lcd_clk_sel);

		if (lcd_clk_sel == SYSPLL){
			ptc->lcd_clk_freq_val = get_syspll_frequency();
			TOOL_DBG("SYSPLL lcd_clk_freq_val=%d \n",ptc->lcd_clk_freq_val);
		}else if (lcd_clk_sel == DDS){
			ptc->lcd_clk_freq_val =  get_dds_frequency();
			TOOL_DBG("DDS lcd_clk_freq_val=%d \n",ptc->lcd_clk_freq_val);
		}else{
			TOOL_DBG("error! \n");
			break;
		}

		fill_trans_data(REQUEST_READ_SYSPLL_DDS);
		break;
	default:
		ret = false;
		break;	

	}
	
	memset(rbuf,0,BUF_SIZE_MAX);
        
	return ret;
}

static bool arktool_mem_init(void)
{
        struct tool_serial_info *p = tsinfo;

	if(!p) return false;
        
        p->mmio = ioremap(LCD_BASE, 0x1000);
        if(!p->mmio)
                return false;

        p->sysreg = ioremap(SYS_BASE, 0x200);
        if(!p->sysreg){
                iounmap(p->mmio);
                return false;
        }

        return true;
}
static void arktool_mem_deinit(void)
{
        struct tool_serial_info *p = tsinfo;

        if(p->mmio) 
                iounmap(tsinfo->mmio);
        if(p->sysreg) 
                iounmap(tsinfo->sysreg);        
}

static void send_uart_data(unsigned char *buf, int len)
{
        tool_serial_send(buf, len);
}

static void reset_uart_recv(void)
{
        struct tool_serial_info *p = tsinfo;

        memset(&p->recv_buf,0,BUF_SIZE_MAX);
        p->recv_flag = ARKTOOL_DATA_NONE;
        p->recv = 0;
}

static bool is_command_id(unsigned char id)
{
        struct tool_serial_info *p = tsinfo;
	bool ret = true;
	
	if(id == WRITE_GAMMA_DEBUG){
		if(p->recv_buf[1] != 194) 
                        ret = false;
	}
	else if((id > READ_COMMAND_ID_BASE && id <= READ_SYSPLL_DDS) || \
		(id > WRITE_COMMAND_ID_BASE && id <= WRITE_CLOCK_DEBUG) || \
		(id > REQUEST_READ_COMMAND_ID_BASE && id <= REQUEST_READ_SYSPLL_DDS) || \
		                                        id == WRITE_DISPLAY_INTERFACE){

		if(p->recv_buf[1] > 50) 
                        ret = false;
	}else{
		 ret = false;
	}	

	return ret;
}

static bool store_uart_data(char ch)
{
        struct tool_serial_info *p = tsinfo;
        unsigned int  lenth = 0;
        
	p->recv_buf[p->recv] = ch;
	lenth = p->recv_buf[1]+3;
	if((p->recv == 1 && p->recv_buf[1] > 240) || 
           (p->recv == 2 && !is_command_id(p->recv_buf[2]))){
		reset_uart_recv();
                return false;
	}
		
	if (p->recv_buf[0] == CUSTOMER_ARK_FLAG){
		p->recv++;
		if(p->recv >= lenth){
			if(p->read_buf[0] != CUSTOMER_ARK_FLAG){
				memcpy(p->read_buf,p->recv_buf,lenth);
                                if(data_check_sum(p->read_buf)){
        				reset_uart_recv();
        				return true;
                                }
			}
			reset_uart_recv();
		}
	}
        
	return false;
}
static void arktool_func(void)
{
        struct tool_serial_info *p = tsinfo;
        unsigned char write_flag[3] = {'A','r','K'};

        tool_data_process();
        if ((p->request_read_id >= REQUEST_READ_COMMAND_ID_BASE) && 
                (p->request_read_id <=REQUEST_READ_COMMAND_ID_END)){

                //TOOL_DBG("send_uart_data.\n");
        	p->request_read_id = 0;
        	send_uart_data(write_flag, 3);
        	send_uart_data(p->write_buf, p->write_len);
        }

}

static bool check_arktool_enable(char ch)
{
        struct tool_serial_info *p = tsinfo;
        char arktool_flag[20] = "arktoolenablenow";
        static int i = 0;
                
        if(!p || !p->mmio || !p->sysreg)
                return false;

	if(p->arktool_enable) 
                return true;
        
	if(arktool_flag[i] == ch)
                i++;
	else 
                i = 0;
        
	if(i >= 16){
		p->arktool_enable = 1;
                TOOL_DBG("arktool enable.\n");
        }

        return false;
}

static bool tool_serial_get_ch(char ch)
{
        struct tool_serial_info *p = tsinfo;
	bool ret = false;
        
	if(!p || !p->arktool_enable) 
                return ret;
		
	if((ch & 0xff) == CUSTOMER_ARK_FLAG && p->recv_flag != ARKTOOL_DATA_START)
		p->recv_flag = ARKTOOL_DATA_START;

	if (p->recv_flag == ARKTOOL_DATA_START){
		//TOOL_DBG("receive: %02x\r\n",ch&0xff);
		if (store_uart_data(ch)){
                        schedule_work(&p->rx_task);
			p->recv_flag = ARKTOOL_DATA_NONE; 
		}
                
		ret = true;
	}
        
	return ret;
}


static void tool_serial_rx_task(struct work_struct *work)
{
        arktool_func();
        
        return;
}

static int ark_tool_serial_probe(struct platform_device *pdev)
{
	struct tool_serial_info *info = NULL;
        
        info = devm_kzalloc(&pdev->dev, sizeof(struct tool_serial_info),GFP_KERNEL);
        if (!info) {
                TOOL_DBG("kzalloc tsinfo failed .\n");
                return -ENOMEM;
        }
        
        memset(info, 0 ,sizeof(struct tool_serial_info));
        tsinfo = info;
        arktool_mem_init();
        INIT_WORK(&info->rx_task, tool_serial_rx_task);
        tool_serial_register_rev_handler(tool_serial_get_ch, check_arktool_enable);

        platform_set_drvdata(pdev, info);

	return 0;	
}

static int ark_tool_serial_remove(struct platform_device *pdev)
{
	struct tool_serial_info *info = platform_get_drvdata(pdev);

	cancel_work_sync(&info->rx_task);
	tool_serial_unregister_rev_handler();
        arktool_mem_deinit();

	return 0;
}

static const struct of_device_id ark_tool_serial_of_match[] = {
	{ .compatible = "arkmicro,ark-tool-serial", },
	{},
};

static struct platform_driver ark_tool_serial_platform_driver = {
	.probe		= ark_tool_serial_probe,
	.remove		= ark_tool_serial_remove,
	.driver	= {
		.name	= "ark-tool-serial",
		.of_match_table = of_match_ptr(ark_tool_serial_of_match),
	},
};

static int __init ark_tool_serial_init(void)
{
	return platform_driver_register(&ark_tool_serial_platform_driver);
}

static void __exit ark_tool_serial_exit(void)
{
	platform_driver_unregister(&ark_tool_serial_platform_driver);
}

/*
 * While this can be a module, if builtin it's most likely the console
 * So let's leave module_exit but move module_init to an earlier place
 */
arch_initcall(ark_tool_serial_init);
module_exit(ark_tool_serial_exit);

MODULE_AUTHOR("Leo");
MODULE_DESCRIPTION("Arkmicro arktool serial driver");
MODULE_LICENSE("GPL v2");

