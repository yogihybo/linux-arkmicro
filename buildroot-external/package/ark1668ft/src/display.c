#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"
#include "display.h"

uint32_t *lcdbase = NULL;
uint32_t *sysbase = NULL;
uint32_t *itubase = NULL;
int fd_videodata = -1;
int fd_osddata = -1;
struct cma_mem *display_mem = NULL;
uint32_t itu_phyaddr;
uint8_t *itu_viraddr;

#define rSYS_AHB_CLK_EN         *(volatile u32*)(sysbase+0x44/4)
#define rSYS_APB_CLK_EN         *(volatile u32*)(sysbase+0x48/4)
#define rSYS_AXI_CLK_EN         *(volatile u32*)(sysbase+0x4c/4)
#define rSYS_PER_CLK_EN         *(volatile u32*)(sysbase+0x50/4)
#define rSYS_LCD_CLK_CFG		*(volatile u32*)(sysbase+0x54/4)
#define rSYS_DEVICE_CLK_CFG0	*(volatile u32*)(sysbase+0x60/4)
#define rSYS_DEVICE_CLK_CFG1	*(volatile u32*)(sysbase+0x64/4)
#define rSYS_DEVICE_CLK_CFG2	*(volatile u32*)(sysbase+0x68/4)
#define rSYS_CLK_DLY_REG        *(volatile u32*)(sysbase+0x70/4)
#define rSYS_SOFT_RSTNA		    *(volatile u32*)(sysbase+0x74/4)
#define rSYS_ANALOG_REG0		*(volatile u32*)(sysbase+0x140/4)
#define rSYS_ANALOG_REG1		*(volatile u32*)(sysbase+0x144/4)
#define rSYS_DDS_CLK_CFG		*(volatile u32*)(sysbase+0x198/4)
#define rSYS_PAD_CTRL00         *(volatile u32*)(sysbase+0x1c0/4)
#define rSYS_PAD_CTRL01         *(volatile u32*)(sysbase+0x1c4/4)
#define rSYS_PAD_CTRL02         *(volatile u32*)(sysbase+0x1c8/4)
#define rSYS_PAD_CTRL03         *(volatile u32*)(sysbase+0x1cc/4)
#define rSYS_PAD_CTRL07         *(volatile u32*)(sysbase+0x1dc/4)
#define rSYS_PAD_CTRL0A		    *(volatile u32*)(sysbase+0x1e8/4)
#define rSYS_PAD_CTRL0B		    *(volatile u32*)(sysbase+0x1ec/4)

#define rLCD_ENABLE		        *(volatile u32*)(lcdbase+0x0/4)
#define rLCD_CONTROL		    *(volatile u32*)(lcdbase+0x004/4)
#define rLCD_TIMING0            *(volatile u32*)(lcdbase+0x008/4)
#define rLCD_TIMING1            *(volatile u32*)(lcdbase+0x00c/4)
#define rLCD_TIMING2            *(volatile u32*)(lcdbase+0x010/4)
#define rLCD_TIMING3            *(volatile u32*)(lcdbase+0x014/4)
#define rLCD_VIDEO_CTL		    *(volatile u32*)(lcdbase+0x03c/4)
#define rLCD_VIDEO_WIN_SIZE	    *(volatile u32*)(lcdbase+0x040/4)
#define rLCD_VIDEO_SIZE		    *(volatile u32*)(lcdbase+0x044/4)
#define rLCD_VIDEO_POSITION	    *(volatile u32*)(lcdbase+0x04c/4)
#define rLCD_BACK_COLOR         *(volatile u32*)(lcdbase+0x050/4)
#define rLCD_VIDEO_ADDR1	    *(volatile u32*)(lcdbase+0x054/4)
#define rLCD_VIDEO_ADDR2	    *(volatile u32*)(lcdbase+0x058/4)
#define rLCD_BLD_MODE_LCD_REG0  *(volatile u32*)(lcdbase+0x060/4)
#define rLCD_BLD_MODE_LCD_REG1  *(volatile u32*)(lcdbase+0x064/4)
#define rLCD_OSD1_CTL           *(volatile u32*)(lcdbase+0x074/4)
#define rLCD_OSD1_SIZE          *(volatile u32*)(lcdbase+0x078/4)
#define rLCD_OSD1_POSITION      *(volatile u32*)(lcdbase+0x07c/4)
#define rLCD_OSD1_ADDR          *(volatile u32*)(lcdbase+0x080/4)
#define rLCD_OSD2_BURST_CTL     *(volatile u32*)(lcdbase+0x084/4)
#define rLCD_OSD2_CTL           *(volatile u32*)(lcdbase+0x088/4)
#define rLCD_OSD2_SIZE          *(volatile u32*)(lcdbase+0x08c/4)
#define rLCD_OSD2_POSITION      *(volatile u32*)(lcdbase+0x090/4)
#define rLCD_OSD2_ADDR          *(volatile u32*)(lcdbase+0x094/4)
#define rLCD_OSD3_CTL           *(volatile u32*)(lcdbase+0x098/4)
#define rLCD_OSD3_SIZE          *(volatile u32*)(lcdbase+0x09c/4)
#define rLCD_OSD3_POSITION      *(volatile u32*)(lcdbase+0x0a0/4)
#define rLCD_OSD3_ADDR          *(volatile u32*)(lcdbase+0x0a4/4)
#define rLCD_VIDEO_SCALE_CTL	*(volatile u32*)(lcdbase+0x0b0/4)
#define rLCD_VIDEO_SCAL_CTL0	*(volatile u32*)(lcdbase+0x0b4/4)
#define rLCD_VIDEO_SCAL_CTL1	*(volatile u32*)(lcdbase+0x0b8/4)
#define rLCD_VIDEO_RIGHT_BOTTOM_CUT_NUM	*(volatile u32*)(lcdbase+0x0bc/4)
#define rLCD_Y2R_COEF321        *(volatile u32*)(lcdbase+0x11c/4)
#define rLCD_Y2R_COEF654        *(volatile u32*)(lcdbase+0x120/4)
#define rLCD_Y2R_COEF7          *(volatile u32*)(lcdbase+0x124/4)
#define rLCD_VIDEO_WIN_POINT	*(volatile u32*)(lcdbase+0x15c/4)
#define rLCD_VIDEO_SOURCE_SIZE	*(volatile u32*)(lcdbase+0x16c/4)
#define rLCD_OSD1_SOURCE_SIZE   *(volatile u32*)(lcdbase+0x170/4)
#define rLCD_OSD2_SOURCE_SIZE   *(volatile u32*)(lcdbase+0x174/4)
#define rLCD_OSD3_SOURCE_SIZE   *(volatile u32*)(lcdbase+0x178/4)
#define rLCD_TV_CONTROL			*(volatile u32*)(lcdbase+0x2b0/4)
#define rLCD_TIMING0_TV			*(volatile u32*)(lcdbase+0x2b4/4)
#define rLCD_TIMING1_TV			*(volatile u32*)(lcdbase+0x2b8/4)
#define rLCD_TIMING2_TV			*(volatile u32*)(lcdbase+0x2bc/4)
#define rLCD_TIMING3_TV			*(volatile u32*)(lcdbase+0x2c0/4)
#define rLCD_TIMING_FRAME_START_CNT_TV  *(volatile u32*)(lcdbase+0x2c4/4)
#define rLCD_EXCTRL2			*(volatile u32*)(lcdbase+0x300/4)
#define rLCD_YPBPR_CTRL0		*(volatile u32*)(lcdbase+0x304/4)
#define rLCD_VIDEO2_CTL			*(volatile u32*)(lcdbase+0x320/4)
#define rLCD_VIDEO2_SOURCE_SIZE	*(volatile u32*)(lcdbase+0x324/4)
#define rLCD_VIDEO2_WIN_POINT	*(volatile u32*)(lcdbase+0x328/4)
#define rLCD_VIDEO2_WIN_SIZE	*(volatile u32*)(lcdbase+0x32c/4)
#define rLCD_VIDEO2_SIZE		*(volatile u32*)(lcdbase+0x330/4)
#define rLCD_VIDEO2_POSITION	*(volatile u32*)(lcdbase+0x334/4)
#define rLCD_VIDEO2_ADDR1		*(volatile u32*)(lcdbase+0x338/4)
#define rLCD_VIDEO2_ADDR2		*(volatile u32*)(lcdbase+0x33c/4)
#define rLCD_VIDEO2_SCALE_CTL   *(volatile u32*)(lcdbase+0x354/4)
#define rLCD_VIDEO2_SCAL_CTL0   *(volatile u32*)(lcdbase+0x358/4)
#define rLCD_VIDEO2_SCAL_CTL1   *(volatile u32*)(lcdbase+0x35c/4)
#define rLCD_VIDEO2_RIGHT_BOTTOM_CUT_NUM    *(volatile u32*)(lcdbase+0x360/4)
#define rLCD_TV_HV_DELAY		*(volatile u32*)(lcdbase+0x3ac/4)

#define rLCD_TV_PARAM_REG0			*(volatile u32*)(lcdbase+0x800/4)
#define rLCD_TV_PARAM_REG1			*(volatile u32*)(lcdbase+0x804/4)
#define rLCD_TV_PARAM_REG2			*(volatile u32*)(lcdbase+0x808/4)
#define rLCD_TV_PARAM_REG3			*(volatile u32*)(lcdbase+0x80C/4)
#define rLCD_TV_PARAM_REG4			*(volatile u32*)(lcdbase+0x810/4)
#define rLCD_TV_PARAM_REG5			*(volatile u32*)(lcdbase+0x814/4)
#define rLCD_TV_PARAM_REG6			*(volatile u32*)(lcdbase+0x818/4)
#define rLCD_TV_PARAM_REG7			*(volatile u32*)(lcdbase+0x81c/4)
#define rLCD_TV_PARAM_REG8			*(volatile u32*)(lcdbase+0x820/4)
#define rLCD_TV_PARAM_REG9			*(volatile u32*)(lcdbase+0x824/4)
#define rLCD_TV_PARAM_REG10			*(volatile u32*)(lcdbase+0x828/4)
#define rLCD_TV_PARAM_REG11			*(volatile u32*)(lcdbase+0x82C/4)
#define rLCD_TV_PARAM_REG12			*(volatile u32*)(lcdbase+0x830/4)
#define rLCD_TV_PARAM_REG13			*(volatile u32*)(lcdbase+0x834/4)
#define rLCD_TV_PARAM_REG14			*(volatile u32*)(lcdbase+0x838/4)
#define rLCD_TV_PARAM_REG15			*(volatile u32*)(lcdbase+0x83C/4)
#define rLCD_TV_PARAM_REG16			*(volatile u32*)(lcdbase+0x840/4)
#define rLCD_TV_PARAM_REG17			*(volatile u32*)(lcdbase+0x844/4)
#define rLCD_TV_PARAM_REG18			*(volatile u32*)(lcdbase+0x848/4)
#define rLCD_TV_PARAM_REG19			*(volatile u32*)(lcdbase+0x84C/4)
#define rLCD_TV_PARAM_REG20			*(volatile u32*)(lcdbase+0x850/4)
#define rLCD_TV_PARAM_REG21			*(volatile u32*)(lcdbase+0x854/4)

#define rITU656IN_MODULE_EN			*(volatile u32*)(itubase+0x000/4)
#define rITU656IN_INPUT_SEL		    *(volatile u32*)(itubase+0x900/4)
#define rITU656IN_SEP_MODE_SEL      *(volatile u32*)(itubase+0x904/4)
#define rITU656IN_ENABLE_REG		*(volatile u32*)(itubase+0x930/4)
#define rITU656IN_SIZE			    *(volatile u32*)(itubase+0x938/4)
#define rITU656IN_DRAM_DEST1        *(volatile u32*)(itubase+0x950/4)
#define rITU656IN_DRAM_DEST2        *(volatile u32*)(itubase+0x954/4)
#define rITU656IN_DATA_OUT_NUM      *(volatile u32*)(itubase+0x958/4)
#define rITU656IN_OUTLINE_NUM_PER_FIELD *(volatile u32*)(itubase+0x95c/4)

typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned int hsw;
	unsigned int vsw;
	unsigned int mode;
} cvbs_format_struct;

cvbs_format_struct cvbs_info[2] =
{
	{720, 576, 142, 24, 0}, //PAL
	{720, 480, 136, 22, 1}  //NTSC
};

void ark_disp_tvenc_reset(void)
{
	rSYS_ANALOG_REG0 &= ~(3<<19);
	rSYS_ANALOG_REG1 &= ~((3<<22) | (1<<17) | (1<<15) | (7<<3) | 1);
	rLCD_YPBPR_CTRL0 &= ~1;
	rLCD_TV_CONTROL = 0;
}

static void ark_disp_config_tvenc_cvbs_timing(unsigned int mode)
{
	rLCD_TIMING0_TV = ((cvbs_info[mode].hsw-1)<<20)|(0<10)|(0<<0);
	rLCD_TIMING1_TV = (0<<19)|((cvbs_info[mode].vsw - 1)<<13)|((cvbs_info[mode].width - 1)<<0);
	rLCD_TIMING2_TV = (0<<23)|(0<<22)|(0<<21)|((cvbs_info[mode].height/2- 1)<<10)|(0<<0);
	rLCD_TIMING_FRAME_START_CNT_TV = (cvbs_info[mode].vsw - 1)/2;
	rLCD_TV_HV_DELAY = 39;
}

static void ark_disp_tvenc_cvbsss_init_ntsc(void)
{
	//TV encoder setting
	#define      chroma_freq_palbg              0x2a098acb  //pal
	#define      chroma_freq_palm               0x21e6efa4  //palm
	#define      chroma_freq_palnc              0x21f69446  //palnc
	#define      chroma_freq_ntsc               0x21f07c1f  //ntsc
	#define      chroma_phase                   0x2a
	#define      clrbar_sel                     0
	#define      clrbar_mode                    0
	#define      bypass_yclamp                  0
	#define      yc_delay                       4
	#define      cvbs_enable                    1
	#define      chroma_bw_1                    0 // bw_1,bw_0 : 00: narrow band; 01: wide band; 10: extra wide; 11: ultra wide.
	#define      chroma_bw_0                    1
	#define      comp_yuv                       0
	#define      compchgain                     0
	#define      hsync_width                    0x3f //0x7e*2
	#define      burst_width                    0x44   //pal 0x3e     ntsc 0x44
	#define      back_porch                     0x3b   //pal 0x45     ntsc 0x3b
	#define      cb_burst_amp                   0x40//0x20
	#define      cr_burst_amp                   0x04//0x00   //pal 0x20     ntsc 0x00
	#define      slave_mode                     0x1
	#define      black_level                    0xf2
	#define      blank_level                    0xf0
	#define      n1                             0x17
	#define      n3                             0x21
	#define      n8                             0x1b
	#define      n9                             0x1b
	#define      n10                            0x24
	#define      num_lines                      525   // pal: 625;   ntsc: 525.
	#define      n0                             0x3e
	#define      n13                            0x0f
	#define      n14                            0x0f
	#define      n15                            0x60
	#define      n5                             0x05
	#define      white_level                    0x320
	#define      cb_gain                        0x89
	#define      n20                            0x04
	#define      cr_gain                        0x89
	#define      n16                            0x1
	#define      n7                             0x2
	#define      tint                           0
	#define      n17                            0x0a
	#define      n19                            0x05
	#define      n18                            0x00
	#define      breeze_way                     0x16
	#define      n21                            0x3ff
	#define      front_porch                    0x10    //pal 0x0c    ntsc 0x10   ??
	#define      n11                            0x7ce
	#define      n12                            0x000
	#define      activeline                     1440
	#define      firstvideoline                 0x14
	#define      uv_order                       0
	#define      pal_mode                       0       //pal 0x1    ntsc 0x0
	#define      invert_top                     0
	#define      sys625_50                      0
	#define      cphase_rst                     3
	#define      vsync5                         1
	#define      sync_level                     0x48
	#define      n22                            0
	#define      agc_pulse_level                0xa3
	#define      bp_pulse_level                 0xc8
	#define      n4                             0x15
	#define      n6                             0x05
	#define      n2                             0x15
	#define      vbi_blank_level                0x128
	#define      soft_rst                       0
	#define      row63                          0
	#define      row64                          0x07
	#define      wss_clock                      0x2f7
	#define      wss_dataf1                     0
	#define      wss_dataf0                     0
	#define      wss_linef1                     0
	#define      wss_linef0                     0
	#define      wss_level                      0x3ff
	#define      venc_en                        1
	#define      uv_first                       0
	#define      uv_flter_en                    1
	#define      notch_en                       0
	#define      notch_wide                     0
	#define      notch_freq                     0
	#define      row78                          0
	#define      row79                          0
	#define      row80                          0

	rLCD_TV_PARAM_REG0 = chroma_freq_ntsc ;//\u017dË\u017d\u0160¶\u0161Òå NÖÆ PÖÆ
	rLCD_TV_PARAM_REG1 = chroma_bw_1<<27 | comp_yuv<<26|compchgain<<24|yc_delay<<17|cvbs_enable<<16|clrbar_sel<<10|clrbar_mode<<9|
	        						  bypass_yclamp<<8 | chroma_phase ;
	rLCD_TV_PARAM_REG2 = cb_burst_amp<<24 | back_porch<<16 | burst_width<<8 | hsync_width;
	rLCD_TV_PARAM_REG3 = 0x4>>26|black_level<< 16 | slave_mode<<8 | cr_burst_amp ;
	rLCD_TV_PARAM_REG4 = n3<<24  | n1<<16 | blank_level ;
	rLCD_TV_PARAM_REG5 = n10<<24 | n9<<16 | n8 ;
	rLCD_TV_PARAM_REG6 = num_lines ;
	rLCD_TV_PARAM_REG7 = n15<<24 | n14<<16| n13<<8 | n0 ;
	rLCD_TV_PARAM_REG8 = cb_gain<<24 | white_level<<8 | n5 ;
	rLCD_TV_PARAM_REG9 = n7<<24      | n16 <<16 | cr_gain<<8 | n20 ;
	rLCD_TV_PARAM_REG10 = n18<<24     | n19 <<16 | n17<<8     | tint ;
	rLCD_TV_PARAM_REG11 = front_porch<<24     | n21<<8       | breeze_way ;
	rLCD_TV_PARAM_REG12 = n12 <<16     | n11 ;
	rLCD_TV_PARAM_REG13 = activeline ;
	rLCD_TV_PARAM_REG14 = n22<<24     | sync_level <<16 | uv_order<<15|pal_mode<<14|chroma_bw_0<<13|invert_top<<12|sys625_50<<11|
	  					  cphase_rst<<9|vsync5<<8     | firstvideoline ;
	rLCD_TV_PARAM_REG15 = n6<<24      | n4 <<16  | bp_pulse_level<<8     | agc_pulse_level ;
	rLCD_TV_PARAM_REG16 = soft_rst<<24| vbi_blank_level<<8    | n2 ;
	rLCD_TV_PARAM_REG17 = row64 <<16  | wss_clock ;
	rLCD_TV_PARAM_REG18 = wss_dataf1 ;
	rLCD_TV_PARAM_REG19 = wss_dataf0 ;
	rLCD_TV_PARAM_REG20 = wss_level <<16 | wss_linef0<<8     | wss_linef1 ;
	rLCD_TV_PARAM_REG21 = row80<<24 | row79<<16 | row78<<8 | venc_en<<7     | uv_first <<6 | uv_flter_en<<5  |notch_en<<4 |  notch_wide<<3 | notch_freq ;
}

static void ark_disp_config_tvenc_cvbs_mode(int mode)
{
	if(mode == CVBS_NTSC) {
		rLCD_TV_CONTROL |= (1<<1);
		ark_disp_tvenc_cvbsss_init_ntsc();
	}
	rLCD_TV_CONTROL |= (1 << 8) | 1;
}

void ark_disp_config_tvenc_interlace_out(int interlace)
{
	if (interlace) rLCD_TV_CONTROL |= (1 << 8);
	else rLCD_TV_CONTROL &= ~(1 << 8);
}

static int SetDDSFrequency(int freq)
{
	unsigned int  dto_inc;
	unsigned int  dds_cofe;
	unsigned int  val;

	val = rSYS_ANALOG_REG1;
	val &= ~(0x1<<2);// scaler_factor = 32
	val &= ~(0x07<<10);
	val |= (1<<18); // enable
	rSYS_ANALOG_REG1 = val;

	dds_cofe = 1<<22;
	dto_inc = dds_cofe*freq/(24*32);

	val = rSYS_DDS_CLK_CFG;
	val &= ~(0x3fffff);
	val |= dto_inc;
	rSYS_DDS_CLK_CFG = val;

	return 0;
}

static void ark_disp_config_tvenc_cvbs_clk()
{
	unsigned int val;

	//config clk
	SetDDSFrequency(216);

	//set div to get 13.5M clk
	val = rSYS_LCD_CLK_CFG;
	val &= ~(0x7f<<25);
	rSYS_LCD_CLK_CFG = val;

	//select dds clk src
	val = rSYS_DEVICE_CLK_CFG2;
	val &= ~(0x1FF<<20);
	val |= (2<<20) | (4<<24) | (1<<28);
	rSYS_DEVICE_CLK_CFG2 = val;
}

static void ark_disp_enable_tvenc_cvbs_dac(void)
{
	rSYS_ANALOG_REG1 |= (3<<22) | (1<<17) | (1<<15);
}

void ark_disp_set_tvenc_en(int enable)
{
	if (enable)
        rSYS_ANALOG_REG1 |= 7 << 3;
	else
        rSYS_ANALOG_REG1 &= ~(7 << 3);
}

void ark_disp_enable(void)
{
    rLCD_ENABLE = 1;
}

void display_video_image_scaler_tvout(void)
{
	rLCD_VIDEO_CTL = (1<<22) | (1<<21) | (1<<8)  | (1<<5)  | (1<<4)  | (1<<0);
	rLCD_VIDEO_WIN_POINT = 0;
	rLCD_VIDEO_WIN_SIZE = (LCD_VIDEO_HEIGHT<<12) | (LCD_VIDEO_WIDTH<<0);
	rLCD_VIDEO_POSITION = 0;
	rLCD_VIDEO_SIZE = (NTSC_HEIGHT<<12) | (NTSC_WIDTH<<0);
	rLCD_VIDEO_SOURCE_SIZE = (LCD_VIDEO_HEIGHT<< 12) | LCD_VIDEO_WIDTH;
	rLCD_VIDEO_ADDR1 = display_mem->phyaddr;
	rLCD_VIDEO_ADDR2 = display_mem->phyaddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT;
	rLCD_VIDEO_SCALE_CTL = (1<<7) | (1<<5);
	/*When need cut after scaler,the dest size of the scaler must be the size
	you wanted plus the cut number you wanted*/
	rLCD_VIDEO_RIGHT_BOTTOM_CUT_NUM = 0;
	rLCD_VIDEO_SCAL_CTL0 = LCD_VIDEO_WIDTH*1024/NTSC_WIDTH;
	rLCD_VIDEO_SCAL_CTL1 = LCD_VIDEO_HEIGHT*1024/NTSC_HEIGHT;
	if(1/*para->Interlace4TV*/)
	{
		rLCD_TV_CONTROL &= ~(1<<8);
		//when v scaler cof is 0x400,v scaler  bypass, now we  should change the cof to
		//force v scaler, otherwise there was sawtooth on picture
		if((rLCD_VIDEO_SCAL_CTL1&0x3FFFF) == 0x400)
			rLCD_VIDEO_SCAL_CTL1 = rLCD_VIDEO_SCAL_CTL1 - 1;
		rLCD_VIDEO_SCALE_CTL &= ~(7<<9);
		rLCD_VIDEO_SCALE_CTL |= (1<<9) | (1<<11);
	}
	rSYS_LCD_CLK_CFG |=(1<<14);
	rLCD_TV_CONTROL |= 1<<2;
	rLCD_CONTROL &= ~(1<<5);
}

void display_video2_image_scaler(void)
{
	rLCD_VIDEO2_CTL = (1<<22) | (1<<21) | (1<<8)  | (1<<5)  | (1<<4)  | (1<<0);
	rLCD_VIDEO2_WIN_POINT = 0;
	rLCD_VIDEO2_WIN_SIZE = (LCD_VIDEO_HEIGHT<<12) | (LCD_VIDEO_WIDTH<<0);
	rLCD_VIDEO2_POSITION = 0;
	rLCD_VIDEO2_SIZE = (ITU601_HEIGHT<<12) | (ITU601_WIDTH<<0);
	rLCD_VIDEO2_SOURCE_SIZE = (LCD_VIDEO_HEIGHT<< 12) | LCD_VIDEO_WIDTH;
	rLCD_VIDEO2_ADDR1 = display_mem->phyaddr;
	rLCD_VIDEO2_ADDR2 = display_mem->phyaddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT;
	rLCD_VIDEO2_SCALE_CTL = (1<<7) | (1<<5);
	/*When need cut after scaler,the dest size of the scaler must be the size
	you wanted plus the cut number you wanted*/
	rLCD_VIDEO2_RIGHT_BOTTOM_CUT_NUM = 0;
	rLCD_VIDEO2_SCAL_CTL0 = LCD_VIDEO_WIDTH*1024/ITU601_WIDTH;
	rLCD_VIDEO2_SCAL_CTL1 = LCD_VIDEO_HEIGHT*1024/ITU601_HEIGHT;
    rLCD_CONTROL |= 1 << 6;
}

void display_osd1_image(void)
{
    rLCD_OSD1_SIZE = (LCD_OSD_HEIGHT << 12) | LCD_OSD_WIDTH;
    rLCD_OSD1_SOURCE_SIZE = (LCD_OSD_HEIGHT << 12) | LCD_OSD_WIDTH;
    rLCD_OSD1_POSITION = (0 << 13) | 0;
    rLCD_OSD1_ADDR = display_mem->phyaddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT*3/2;
    rLCD_OSD1_CTL &= ~(0x7FF<<12);
    rLCD_OSD1_CTL |= (0 << 21) | (0 << 18) | (1 << 17) | (0 << 16) | (6 << 12);
    rLCD_CONTROL |= 1 << 7;
}

void display_osd2_image(void)
{
    rLCD_OSD2_SIZE = (LCD_OSD_HEIGHT << 12) | LCD_OSD_WIDTH;
    rLCD_OSD2_SOURCE_SIZE = (LCD_OSD_HEIGHT << 12) | LCD_OSD_WIDTH;
    rLCD_OSD2_POSITION = (0 << 13) | 400;
    rLCD_OSD2_ADDR = display_mem->phyaddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT*3/2;
    rLCD_OSD2_CTL &= ~(0x7FF<<12);
    rLCD_OSD2_CTL |= (0 << 21) | (0 << 18) | (1 << 17) | (0 << 16) | (6 << 12);
    rLCD_CONTROL |= 1 << 8;
}

void display_osd3_image(void)
{
    rLCD_OSD3_SIZE = (LCD_OSD_HEIGHT << 12) | LCD_OSD_WIDTH;
    rLCD_OSD3_SOURCE_SIZE = (LCD_OSD_HEIGHT << 12) | LCD_OSD_WIDTH;
    rLCD_OSD3_POSITION = (120 << 13) | 240;
    rLCD_OSD3_ADDR = display_mem->phyaddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT*3/2;
    rLCD_OSD3_CTL &= ~(0x7FF<<12);
    rLCD_OSD3_CTL |= (0 << 21) | (0 << 18) | (1 << 17) | (0 << 16) | (6 << 12);
    rLCD_CONTROL |= 1 << 9;
}

void ark_disp_init_tvenc_cvbs(int mode)
{
	ark_disp_tvenc_reset();
	ark_disp_config_tvenc_cvbs_timing(mode);
	ark_disp_config_tvenc_cvbs_mode(mode);
	ark_disp_config_tvenc_interlace_out(0);
	ark_disp_config_tvenc_cvbs_clk();
	ark_disp_enable_tvenc_cvbs_dac();
    ark_disp_set_tvenc_en(1);
    ark_disp_enable();
    display_video_image_scaler_tvout();
}

void ark_disp_config_itu601(void)
{
    u32 val;

    //pad select
	rSYS_PAD_CTRL00 = (1<<28) |(1<<24) |(1<<20) |(1<<16) |(1<<12) |(1<<8) | (1<<4) |(1<<0);
	rSYS_PAD_CTRL03 = (1<<12) |(1<<8) | (1<<4) |(1<<0);


    rLCD_CONTROL = (rLCD_CONTROL & (0xB << 6)) | (1<<0);

    rLCD_TIMING0 = (50<<20) | (0<<10) | (0<<0);
    rLCD_TIMING1 = (0<<19) | (10<<13) | ((ITU601_WIDTH-2-1)<<0);
    rLCD_TIMING2 = (0<<23) | (1<<22) | (1<<21) | ((ITU601_HEIGHT-1)<<10) | (0<<0);

    rLCD_Y2R_COEF321 = 298<<0
              |91<<10
              |425<<20;
    rLCD_Y2R_COEF654 = 96<<0
             |184<<10
             |465<<20;
    rLCD_Y2R_COEF7 = (rLCD_Y2R_COEF7 & 0xffffcc00)
             |41<<0
             |1<<12
             |0<<13;

    rLCD_BACK_COLOR = 0x505aef;		//set lcd red color

    rLCD_BLD_MODE_LCD_REG0 = 0x03020104;
    rLCD_BLD_MODE_LCD_REG1 = 0x3f000;

    //Screen type
    //0: parallel screen 24 bit
    // 1: parallel screen 18 bit
    // 2: parallel screen 16 bit
    // 3: srgb
	rLCD_CONTROL &= ~(0x7<<2);
	rLCD_CONTROL |= (0x3<<2);

    //Srgb_yuv_rgb CLCD_CONTROL[1]
    // 0:rgb
    // 1:yuv
	rLCD_CONTROL |= 0x1<<1;

    //SRGB mode  rLCD_EXCTRL2[17:16]
    //00: through mode��sRGB��;
    //01: sRGB dummy;
    //10: sYUV422;
    //11: normal RGB
	rLCD_EXCTRL2 &=~((0x3<<16)|(0x7<<18)|(0x7<<21));
	rLCD_EXCTRL2 |=(0x2<<16)|(0x5<<18)|(0x5<<21);
	rLCD_ENABLE = 1;
	rLCD_CONTROL |=(0x1<<18);
}

void ark_disp_config_rgb(void)
{
    u32 val;

    //pad select
    rSYS_PAD_CTRL00 = (1<<28) |(1<<24) |(1<<20) |(1<<16) |(1<<12) |(1<<8) | (1<<4) |(1<<0);
    rSYS_PAD_CTRL01 = (1<<28) |(1<<24) |(1<<20) |(1<<16) |(1<<12) |(1<<8) | (1<<4) |(1<<0);
    rSYS_PAD_CTRL02 = (1<<28) |(1<<24) |(1<<20) |(1<<16) |(1<<12) |(1<<8) | (1<<4) |(1<<0);
    rSYS_PAD_CTRL03 = (1<<12) |(1<<8) | (1<<4) |(1<<0);

    rLCD_CONTROL = (rLCD_CONTROL & (0xB << 6)) | (1<<0);

    rLCD_TIMING0 = (50<<20) | (0<<10) | (0<<0);
    rLCD_TIMING1 = (0<<19) | (10<<13) | ((ITU601_WIDTH-2-1)<<0);
    rLCD_TIMING2 = (0<<23) | (1<<22) | (1<<21) | ((ITU601_HEIGHT-1)<<10) | (0<<0);

    rLCD_Y2R_COEF321 = 298<<0
              |91<<10
              |425<<20;
    rLCD_Y2R_COEF654 = 96<<0
             |184<<10
             |465<<20;
    rLCD_Y2R_COEF7 = (rLCD_Y2R_COEF7 & 0xffffcc00)
             |41<<0
             |1<<12
             |0<<13;

    rLCD_BACK_COLOR = 0xFF8080;		//set lcd white color

    rLCD_BLD_MODE_LCD_REG0 = 0x03020104;
    rLCD_BLD_MODE_LCD_REG1 = 0x3f000;

	rLCD_ENABLE = 1;
	rLCD_CONTROL |=(0x1<<18);
}

void ark_disp_init_itu601_out(void)
{
    ark_disp_config_itu601();
    display_video2_image_scaler();
    display_osd1_image();
    display_osd2_image();
    display_osd3_image();
}

void itu656_init(void)
{
    u32 val;

    //config itu ch1 pad
    val = rSYS_PAD_CTRL0A;
    val &= ~(0xF<<4);
    val |= 5<<4;
    rSYS_PAD_CTRL0A = val;

    val = rSYS_PAD_CTRL0B;
    val |= (0x1FF<<16);
    rSYS_PAD_CTRL0B = val;

    //soft reset
	rSYS_SOFT_RSTNA &= ~(1 << 9);
	usleep(100);
	rSYS_SOFT_RSTNA |= (1 << 9);

    //clk inv
    rSYS_DEVICE_CLK_CFG1 |= 1;

    rITU656IN_MODULE_EN = 1 << 2;
	rITU656IN_INPUT_SEL = 0x01;
	rITU656IN_DRAM_DEST1 = itu_phyaddr;
	rITU656IN_DRAM_DEST2 = itu_phyaddr + NTSC_WIDTH * NTSC_HEIGHT;
    rITU656IN_OUTLINE_NUM_PER_FIELD = NTSC_HEIGHT / 2;
	rITU656IN_DATA_OUT_NUM = NTSC_WIDTH * NTSC_HEIGHT / 2;
	rITU656IN_SIZE  = NTSC_WIDTH<<16;
    rITU656IN_MODULE_EN = 0;
    rITU656IN_ENABLE_REG = (1<<13) | (1<<11) | (1<<5) | (1<<3) | (1<<1) | 1;
}

void itu_uninit(void)
{
    rITU656IN_MODULE_EN = 1 << 2;
    rITU656IN_ENABLE_REG = 0;
}

void itu601_init(void)
{
    u32 val;

    //config itu ch0 pad
    //hsync, vsync
	rSYS_PAD_CTRL07 |= (1 << 19) | (1 << 18);

    val = rSYS_PAD_CTRL0A;
    val &= ~(0xF<<4);
    rSYS_PAD_CTRL0A = val;

    val = rSYS_PAD_CTRL07;
    val &= ~(0x1FFFF<<0);
    val |= 0x15555;
    rSYS_PAD_CTRL07 = val;

    // Clock On
	rSYS_PER_CLK_EN |= 1 << 12;
	rSYS_AXI_CLK_EN |= 1 << 2;
	rSYS_AHB_CLK_EN |= 1 << 10;

    //soft reset
	rSYS_SOFT_RSTNA &= ~(1 << 9);
	usleep(100);
	rSYS_SOFT_RSTNA |= (1 << 9);

    //clk inv
    rSYS_CLK_DLY_REG |= 1 << 16;

    rITU656IN_MODULE_EN = 1 << 2;
	rITU656IN_INPUT_SEL = 0;
	rITU656IN_SEP_MODE_SEL = (1<< 13) | (1 << 12) | (1 << 4) | (1 << 2);

	rITU656IN_DRAM_DEST1 = itu_phyaddr;
	rITU656IN_DRAM_DEST2 = itu_phyaddr + ITU601_WIDTH * ITU601_HEIGHT * 2;
    rITU656IN_OUTLINE_NUM_PER_FIELD = ITU601_HEIGHT;
	rITU656IN_DATA_OUT_NUM = ITU601_WIDTH * ITU601_HEIGHT;
	rITU656IN_SIZE = ITU601_WIDTH<<16;
    rITU656IN_MODULE_EN = 1;
    rITU656IN_ENABLE_REG = (1<<13) | (1<<11) | (1<<5) | (1<<3) | (1<<1) | 1;
}

int itu656_compare_data(void)
{
    /* int fd_itu656 = open("itu656.yuv", O_WRONLY | O_CREAT | O_TRUNC);
    write(fd_itu656, itu_viraddr, NTSC_WIDTH * NTSC_HEIGHT);
    close(fd_itu656); */
    int w, h;
    int ret = -1;
    unsigned char *itubuf = (unsigned char*)itu_viraddr;
    unsigned char *databuf = NULL;
    int fddata = -1;
    unsigned long datasize;
    int count = 0;

    fddata = open(ITU656_DATA_PATH, O_RDONLY);
    if (fddata < 0) {
        printf("open data file %s fail.\n", ITU656_DATA_PATH);
        goto end;
    }
    datasize = get_file_size(ITU656_DATA_PATH);
    databuf = malloc(datasize);
    if (!databuf) {
        printf("mallco databuf fail.\n");
        goto end;
    }
    if (read(fddata, databuf, datasize) != datasize) {
        printf("read data file err.\n");
        goto end;
    }

    for (h = 0; h< 240; h++) {
        for(w = 0; w < 320; w++) {
            if (abs(databuf[h*NTSC_WIDTH*2+w] - itubuf[h*NTSC_WIDTH*2+w]) > 16) {
                /* printf("itu656 compare data fail 0x%x, 0x%x.\n",
                    databuf[h*NTSC_WIDTH*2+w], itubuf[h*NTSC_WIDTH*2+w]); */
                if (count++ > ITU656_DIFF_MAX)
                    break;
            }
        }
    }

    if (count < ITU656_DIFF_MAX)
        ret = 0;
    else
        printf("itu656 compare %d diff.\n", count);

end:
    if (fddata > 0)
        close(fddata);
    if (databuf)
        free(databuf);
    return ret;
}

int itu601_compare_data(void)
{
    /* int fd_itu601 = open("itu601.yuv", O_WRONLY | O_CREAT | O_TRUNC);
    write(fd_itu601, itu_viraddr, ITU601_WIDTH * ITU601_HEIGHT * 2);
    close(fd_itu601); */

    int w, h;
    int ret = -1;
    unsigned char *itubuf = (unsigned char*)itu_viraddr;
    unsigned char *databuf = NULL;
    int fddata = -1;
    unsigned long datasize;
    int count = 0;
    char datafilename[32] = ITU601_DATA_PATH;
    int retrycount = 0;

retry:
    fddata = open(datafilename, O_RDONLY);
    if (fddata < 0) {
        printf("open data file %s fail.\n", datafilename);
        goto end;
    }
    datasize = get_file_size(datafilename);
    databuf = malloc(datasize);
    if (!databuf) {
        printf("mallco databuf fail.\n");
        goto end;
    }
    if (read(fddata, databuf, datasize) != datasize) {
        printf("read data file err.\n");
        goto end;
    }

    for (h = 0; h < ITU601_HEIGHT; h++) {
        for(w = 0; w < ITU601_WIDTH * 2; w++) {
            if (abs(databuf[h*ITU601_WIDTH*2+w] - itubuf[h*ITU601_WIDTH*2+w]) > 3) {
                /* printf("itu601 compare data fail (%d,%d) 0x%x, 0x%x.\n",
                    w, h, databuf[h*ITU601_WIDTH*2+w], itubuf[h*ITU601_WIDTH*2+w]); */
                if (count++ > ITU601_DIFF_MAX)
                    goto compare;
            }
        }
    }

compare:
    if (count < ITU601_DIFF_MAX) {
        ret = 0;
        goto end;
    } else {
        if (retrycount) {
            ret = -1;
        } else {
            retrycount = 1;
            count = 0;
            close(fddata);
            free(databuf);
            strcpy(datafilename, ITU601_DATA2_PATH);
            goto retry;
        }
    }

    printf("itu601 compare %d diff.\n", count);

end:
    if (fddata > 0)
        close(fddata);
    if (databuf)
        free(databuf);
    return ret;
}

struct rgbpad_info {
    int gpio;
    char *padname;
};

struct rgbpad_info rgbpads1[] = {
    {6, "G0"},
    {7, "G1"},
    {74, "G2"},
    {75, "G3"},
    {72, "G4"},
    {73, "G5"},
    {17, "G6"},
    {19, "B0"},
    {21, "B2"},
    {23, "B4"},
    {25, "B6"},
};

struct rgbpad_info rgbpads2[] = {
    {16, "G7"},
    {18, "B1"},
    {20, "B3"},
    {22, "B5"},
    {24, "B7"},
};

int lcd_rgb_pad_test(void)
{
    int i;
    unsigned int val;
    int retry;

    ark_disp_config_rgb();
    rLCD_BACK_COLOR = 0xFF8080;		//set lcd white color
    usleep(30000);

    for (i = 0; i < sizeof(rgbpads1) / sizeof(rgbpads1[0]); i++) {
        gpio_export(rgbpads1[i].gpio);
        gpio_set_dir(rgbpads1[i].gpio, "in");
    }
    for (i = 0; i < sizeof(rgbpads1) / sizeof(rgbpads1[0]); i++) {
        retry = 100;
        while (retry--) {
            gpio_get_value(rgbpads1[i].gpio, &val);
            if (val == 1)
                break;
            usleep(5);
        }
        if (val != 1) {
            printf("LCD %s pad output high error.\n", rgbpads1[i].padname);
            return -1;
        }
    }

    rLCD_BACK_COLOR = 0x108080;		//set lcd black color
    usleep(30000);
    for (i = 0; i < sizeof(rgbpads1) / sizeof(rgbpads1[0]); i++) {
        gpio_get_value(rgbpads1[i].gpio, &val);
        if (val != 0) {
            printf("GPIO %s pad output low error.\n", rgbpads1[i].padname);
            return -1;
        }
    }

    for (i = 0; i < sizeof(rgbpads1) / sizeof(rgbpads1[0]); i++)
        gpio_unexport(rgbpads1[i].gpio);

    ark_disp_config_rgb();
    rLCD_BACK_COLOR = 0xFF8080;		//set lcd white color
    usleep(30000);

    for (i = 0; i < sizeof(rgbpads2) / sizeof(rgbpads2[0]); i++) {
        gpio_export(rgbpads2[i].gpio);
        gpio_set_dir(rgbpads2[i].gpio, "in");
    }
    for (i = 0; i < sizeof(rgbpads2) / sizeof(rgbpads2[0]); i++) {
        retry = 100;
        while (retry--) {
            gpio_get_value(rgbpads2[i].gpio, &val);
            if (val == 1)
                break;
            usleep(5);
        }
        if (val != 1) {
            printf("LCD %s pad output high error.\n", rgbpads2[i].padname);
            return -1;
        }
    }

    rLCD_BACK_COLOR = 0x108080;		//set lcd black color
    usleep(30000);
    for (i = 0; i < sizeof(rgbpads2) / sizeof(rgbpads2[0]); i++) {
        gpio_get_value(rgbpads2[i].gpio, &val);
        if (val != 0) {
            printf("GPIO %s pad output low error.\n", rgbpads2[i].padname);
            return -1;
        }
    }

    for (i = 0; i < sizeof(rgbpads2) / sizeof(rgbpads2[0]); i++)
        gpio_unexport(rgbpads2[i].gpio);

    return 0;
}

int display_init(void)
{
    uint32_t val;

    lcdbase = map_phy_memory(0xe0500000, 0x1000, 1);
    if (!lcdbase)
        return -1;

    sysbase = map_phy_memory(0xe4900000, 0x1000, 1);
    if (!sysbase) {
        unmap_phy_memory(lcdbase, 0x1000);
        return -1;
    }

    //config display clk
	rSYS_LCD_CLK_CFG = (0x36<<25) | (1<<23) | (10 << 19) | (1<<7) | (2<<4) | 4;
	//config video2 scaler clk
	val = rSYS_DEVICE_CLK_CFG0;
	val &= ~(0xf << 24);
	val |= 4 << 24;
	rSYS_DEVICE_CLK_CFG0 = val;
    //lcd clk enable
    rSYS_PER_CLK_EN |= (1 << 4);

    itubase = map_phy_memory(0xe0800000, 0x1000, 1);
    if (!itubase) {
        unmap_phy_memory(itubase, 0x1000);
        return -1;
    }

    display_mem = alloc_cma_mem(0x400000);
    if (!display_mem) {
        printf("alloc_cma_mem fail.\n");
        return -1;
    }
    itu_phyaddr = display_mem->phyaddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT*11/2;
    itu_viraddr = (uint8_t*)display_mem->viraddr + LCD_VIDEO_WIDTH*LCD_VIDEO_HEIGHT*11/2;

    fd_videodata = open(LCD_VIDEO_DATA_PATH, O_RDONLY);
    if (fd_videodata < 0) {
        printf("open %s fail.\n", LCD_VIDEO_DATA_PATH);
        return -1;
    }

    if (read(fd_videodata, display_mem->viraddr, LCD_VIDEO_WIDTH
            * LCD_VIDEO_HEIGHT * 3 / 2) != LCD_VIDEO_WIDTH * LCD_VIDEO_HEIGHT * 3 / 2) {
        printf("read video data err.\n");
        return -1;
    }

    fd_osddata = open(LCD_OSD_DATA_PATH, O_RDONLY);
    if (fd_osddata < 0) {
        printf("open %s fail.\n", LCD_OSD_DATA_PATH);
        return -1;
    }

    if (read(fd_osddata, display_mem->viraddr + LCD_VIDEO_WIDTH
            * LCD_VIDEO_HEIGHT * 3 / 2, LCD_VIDEO_WIDTH
            * LCD_VIDEO_HEIGHT * 4) != LCD_VIDEO_WIDTH * LCD_VIDEO_HEIGHT * 4) {
        printf("read osd data err.\n");
        return -1;
    }

    return 0;
}

void display_uninit(void)
{
    if (!lcdbase)
        unmap_phy_memory(lcdbase, 0x1000);

    if (!sysbase)
        unmap_phy_memory(sysbase, 0x1000);

    if (!itubase)
        unmap_phy_memory(itubase, 0x1000);

    if (display_mem)
        free_cma_mem(display_mem);

    if (fd_videodata)
        close(fd_videodata);

    if (fd_osddata)
        close(fd_osddata);
}



