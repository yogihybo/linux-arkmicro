#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "ark_camera.h"
#include "ark_isp_exposure_cmos.h"
#include "Gem_isp_io.h"

static u32_t isp_video_width  = 1920;
static u32_t isp_video_height = 1080;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_10;
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;		// ISP??YUV??????ÃŠÂ½????
static unsigned int isp_sensor_fps = 0;

// Ã¨Å½Â·Ã¥Ââ€“ISPÃ¨Â§â€ Ã©Â¢â€˜Ã§Å¡â€Ã¨Â¾â€œÃ¥â€¡ÂºÃ¥Â°ÂºÃ¥Â¯Â¸Ã¥Â®Å¡Ã¤Â¹â€°(1080PÃ¦Ë†â€“Ã¨â‚¬â€¦720P)
u32_t isp_get_video_width  (void)
{
	return isp_video_width;
}

u32_t isp_get_video_height (void)
{
	return isp_video_height;
}

//0Ã¯Â¼Å¡y_uv420 1:y_uv422 2:yuv420 3:yuv422
unsigned int isp_get_video_format (void)
{
	return isp_yuv_format;
}

unsigned int isp_get_video_image_size (void)
{
	if(isp_yuv_format == ARKN141_ISP_YUV_FORMAT_Y_UV420 || isp_yuv_format == ARKN141_ISP_YUV_FORMAT_YUV420)
		return isp_video_width * isp_video_height * 3 / 2;
	else	// ARKN141_ISP_YUV_FORMAT_Y_UV422 ARKN141_ISP_YUV_FORMAT_Y_UV422
		return isp_video_width * isp_video_height * 2;
}

unsigned int isp_get_sensor_bit (void)
{
	return isp_sensor_bit;
}

unsigned int isp_get_sensor_fps(void)
{
	return isp_sensor_fps;
}

static struct i2c_client *i2c_client;

static int I2C_WRITE (unsigned int addr, unsigned int data)
{
	return i2c_smbus_write_byte_data(i2c_client, addr, data);
}

static unsigned int I2C_READ (unsigned int addr)
{
	return i2c_smbus_read_byte_data(i2c_client, addr);
}

typedef struct stSensor_cfg {
	unsigned int  addr;
	unsigned char data;
} sensor_cfg;

static struct stSensor_cfg stSensor_cfg_1080P25FPS_10bit[] =
{
//window_size=1920*1080
//mclk=24mhz,pclk=74.25mhz
//pixel_line_total=2640,line_frame_total=1125
//row_time=35.55usus,frame_rate=25fps
/*system*/
0xfe,0x80,
0xfe,0x80,
0xfe,0x80,
0xfe,0x00,
0xf2,0x00,//[1]I2C_open_ena [0]pwd_dn
0xf3,0x0f,//00[3]Sdata_pad_io [2:0]Ssync_pad_io
0xf4,0x36,//[6:4]pll_ldo_set
0xf5,0xc0,//[7]soc_mclk_enable [6]pll_ldo_en [5:4]cp_clk_sel [3:0]cp_clk_div
0xf6,0x44,//[7:3]wpllclk_div [2:0]refmp_div
0xf7,0x01,//[7]refdiv2d5_en [6]refdiv1d5_en [5:4]scaler_mode [3]refmp_enb [1]div2en [0]pllmp_en
0xf8,0x63,//38////38//[7:0]pllmp_div
0xf9,0x40,//82//[7:3]rpllclk_div [2:1]pllmp_prediv [0]analog_pwc
0xfc,0x8e,
/*cisctl&analog*/
0xfe,0x00,
0x87,0x18, //[6]aec_delay_mode
0xee,0x30, //[5:4]dwen_sramen
0xd0,0xb7, //ramp_en
0x03,0x04,
0x04,0x60,
0x05,0x05,// 0x528 = 1320
0x06,0x28,//
0x07,0x00,
0x08,0x11, //19
0x09,0x00,
0x0a,0x02, //cisctl row start
0x0b,0x00,
0x0c,0x02, //cisctl col start
0x0d,0x04,
0x0e,0x40, //38
0x12,0xe2, //vsync_ahead_mode
0x13,0x16,
0x19,0x0a, //ad_pipe_num
0x21,0x1c, //eqc1fc_eqc2fc_sw
0x28,0x0a, //16//eqc2_c2clpen_sw
0x29,0x24, //eq_post_width
0x2b,0x04, //c2clpen --eqc2
0x32,0xf8, //[5]txh_en ->avdd28
0x37,0x03, //[3:2]eqc2sel=0
0x39,0x15,//17 //[3:0]rsgl
0x43,0x07,//vclamp
0x44,0x40, //0e//post_tx_width
0x46,0x0b,
0x4b,0x20, //rst_tx_width
0x4e,0x08, //12//ramp_t1_width
0x55,0x20, //read_tx_width_pp
0x66,0x05, //18//stspd_width_r1
0x67,0x05, //40//5//stspd_width_r
0x77,0x01, //dacin offset x31
0x78,0x00, //dacin offset
0x7c,0x93, //[1:0] co1comp
0x8c,0x12, //12 ramp_t1_ref
0x8d,0x92,//90
0x90,0x01,
0x9d,0x10,
0xce,0x7c,//70//78//[4:2]c1isel
0xd2,0x41,//[5:3]c2clamp
0xd3,0xdc,//ec//0x39[7]=0,0xd3[3]=1 rsgh=vref
0xe6,0x50,//ramps offset
/*gain*/
0xb6,0xc0,
0xb0,0x70,
0xb1,0x01,
0xb2,0x00,
0xb3,0x00,
0xb4,0x00,
0xb8,0x01,
0xb9,0x00,
/*blk*/
0x26,0x30,//23 //[4]Ã¥â€ â„¢0Ã¯Â¼Å’Ã¥â€¦Â¨n mode
0xfe,0x01,
0x40,0x23,
0x55,0x07,
0x60,0x40, //[7:0]WB_offset
0xfe,0x04,
0x14,0x78, //g1 ratio
0x15,0x78, //r ratio
0x16,0x78, //b ratio
0x17,0x78, //g2 ratio
/*window*/
0xfe,0x01,
0x92,0x00, //win y1
0x94,0x03, //win x1
0x95,0x04,
0x96,0x38, //[10:0]out_height
0x97,0x07,
0x98,0x80, //[11:0]out_width
/*ISP*/
0xfe,0x01,
0x01,0x05,//03//[3]dpc blending mode [2]noise_mode [1:0]center_choose 2b'11:median 2b'10:avg 2'b00:near
0x02,0x89, //[7:0]BFF_sram_mode
0x04,0x01, //[0]DD_en
0x07,0xa6,
0x08,0xa9,
0x09,0xa8,
0x0a,0xa7,
0x0b,0xff,
0x0c,0xff,
0x0f,0x00,
0x50,0x1c,
0x89,0x03,
0xfe,0x04,
0x28,0x86,//84
0x29,0x86,//84
0x2a,0x86,//84
0x2b,0x68,//84
0x2c,0x68,//84
0x2d,0x68,//84
0x2e,0x68,//83
0x2f,0x68,//82
0x30,0x4f,//82
0x31,0x68,//82
0x32,0x67,//82
0x33,0x66,//82
0x34,0x66,//82
0x35,0x66,//82
0x36,0x66,//64
0x37,0x66,//68
0x38,0x62,
0x39,0x62,
0x3a,0x62,
0x3b,0x62,
0x3c,0x62,
0x3d,0x62,
0x3e,0x62,
0x3f,0x62,
/****DVP & MIPI****/
0xfe,0x01,
0x9a,0x06, //[5]OUT_gate_mode [4]hsync_delay_half_pclk [3]data_delay_half_pclk [2]vsync_polarity [1]hsync_polarity [0]pclk_out_polarity
0xfe,0x00,
0x7b,0x28, //[7:6]updn [5:4]drv_high_data [3:2]drv_low_data [1:0]drv_pclk
0x23,0x2d, //[3]rst_rc [2:1]drv_sync [0]pwd_rc
0xfe,0x03,
0x01,0x20, //27[6:5]clkctr [2]phy-lane1_en [1]phy-lane0_en [0]phy_clk_en
0x02,0x56, //[7:6]data1ctr [5:4]data0ctr [3:0]mipi_diff
0x03,0xb2, //b6[7]clklane_p2s_sel [6:5]data0hs_ph [4]data0_delay1s [3]clkdelay1s [2]mipi_en [1:0]clkhs_ph
0x12,0x80,
0x13,0x07, //LWC
0xfe,0x00,
0x3e,0x40, //91[7]lane_ena [6]DVPBUF_ena [5]ULPEna [4]MIPI_ena [3]mipi_set_auto_disable [2]RAW8_mode [1]ine_sync_mode [0]double_lane_en
};

static struct stSensor_cfg stSensor_cfg_1080P30FPS_10bit[] =
{
//window_size=1920*1080
//mclk=24mhz,pclk=74.25mhz
//pixel_line_total=2200,line_frame_total=1125
//row_time=29.629us,frame_rate=30fps
/*system*/
0xfe,0x80,
0xfe,0x80,
0xfe,0x80,
0xfe,0x00,
0xf2,0x00,//[1]I2C_open_ena [0]pwd_dn
0xf3,0x0f,//00[3]Sdata_pad_io [2:0]Ssync_pad_io
0xf4,0x36,//[6:4]pll_ldo_set
0xf5,0xc0,//[7]soc_mclk_enable [6]pll_ldo_en [5:4]cp_clk_sel [3:0]cp_clk_div
0xf6,0x44,//[7:3]wpllclk_div [2:0]refmp_div
0xf7,0x01,//[7]refdiv2d5_en [6]refdiv1d5_en [5:4]scaler_mode [3]refmp_enb [1]div2en [0]pllmp_en
0xf8,0x63,//38////38//[7:0]pllmp_div
0xf9,0x40,//82//[7:3]rpllclk_div [2:1]pllmp_prediv [0]analog_pwc
0xfc,0x8e,
/*cisctl&analog*/
0xfe,0x00,
0x87,0x18, //[6]aec_delay_mode
0xee,0x30, //[5:4]dwen_sramen
0xd0,0xb7, //ramp_en
0x03,0x04,
0x04,0x60,
0x05,0x04,// 0x44c = 1100
0x06,0x4c,//
0x07,0x00,
0x08,0x11, //19
0x09,0x00,
0x0a,0x02, //cisctl row start
0x0b,0x00,
0x0c,0x02, //cisctl col start
0x0d,0x04,
0x0e,0x40, //38
0x12,0xe2, //vsync_ahead_mode
0x13,0x16,
0x19,0x0a, //ad_pipe_num
0x21,0x1c, //eqc1fc_eqc2fc_sw
0x28,0x0a, //16//eqc2_c2clpen_sw
0x29,0x24, //eq_post_width
0x2b,0x04, //c2clpen --eqc2
0x32,0xf8, //[5]txh_en ->avdd28
0x37,0x03, //[3:2]eqc2sel=0
0x39,0x15,//17 //[3:0]rsgl
0x43,0x07,//vclamp
0x44,0x40, //0e//post_tx_width
0x46,0x0b,
0x4b,0x20, //rst_tx_width
0x4e,0x08, //12//ramp_t1_width
0x55,0x20, //read_tx_width_pp
0x66,0x05, //18//stspd_width_r1
0x67,0x05, //40//5//stspd_width_r
0x77,0x01, //dacin offset x31
0x78,0x00, //dacin offset
0x7c,0x93, //[1:0] co1comp
0x8c,0x12, //12 ramp_t1_ref
0x8d,0x92,//90
0x90,0x01,
0x9d,0x10,
0xce,0x7c,//70//78//[4:2]c1isel
0xd2,0x41,//[5:3]c2clamp
0xd3,0xdc,//ec//0x39[7]=0,0xd3[3]=1 rsgh=vref
0xe6,0x50,//ramps offset
/*gain*/
0xb6,0xc0,
0xb0,0x70,
0xb1,0x01,
0xb2,0x00,
0xb3,0x00,
0xb4,0x00,
0xb8,0x01,
0xb9,0x00,
/*blk*/
0x26,0x30,//23 //[4]Ã¥â€ â„¢0Ã¯Â¼Å’Ã¥â€¦Â¨n mode
0xfe,0x01,
0x40,0x23,
0x55,0x07,
0x60,0x40, //[7:0]WB_offset
0xfe,0x04,
0x14,0x78, //g1 ratio
0x15,0x78, //r ratio
0x16,0x78, //b ratio
0x17,0x78, //g2 ratio
/*window*/
0xfe,0x01,
0x92,0x00, //win y1
0x94,0x03, //win x1
0x95,0x04,
0x96,0x38, //[10:0]out_height
0x97,0x07,
0x98,0x80, //[11:0]out_width
/*ISP*/
0xfe,0x01,
0x01,0x05,//03//[3]dpc blending mode [2]noise_mode [1:0]center_choose 2b'11:median 2b'10:avg 2'b00:near
0x02,0x89, //[7:0]BFF_sram_mode
0x04,0x01, //[0]DD_en
0x07,0xa6,
0x08,0xa9,
0x09,0xa8,
0x0a,0xa7,
0x0b,0xff,
0x0c,0xff,
0x0f,0x00,
0x50,0x1c,
0x89,0x03,
0xfe,0x04,
0x28,0x86,//84
0x29,0x86,//84
0x2a,0x86,//84
0x2b,0x68,//84
0x2c,0x68,//84
0x2d,0x68,//84
0x2e,0x68,//83
0x2f,0x68,//82
0x30,0x4f,//82
0x31,0x68,//82
0x32,0x67,//82
0x33,0x66,//82
0x34,0x66,//82
0x35,0x66,//82
0x36,0x66,//64
0x37,0x66,//68
0x38,0x62,
0x39,0x62,
0x3a,0x62,
0x3b,0x62,
0x3c,0x62,
0x3d,0x62,
0x3e,0x62,
0x3f,0x62,
/****DVP & MIPI****/
0xfe,0x01,
0x9a,0x06, //[5]OUT_gate_mode [4]hsync_delay_half_pclk [3]data_delay_half_pclk [2]vsync_polarity [1]hsync_polarity [0]pclk_out_polarity
0xfe,0x00,
0x7b,0x28, //[7:6]updn [5:4]drv_high_data [3:2]drv_low_data [1:0]drv_pclk
0x23,0x2d, //[3]rst_rc [2:1]drv_sync [0]pwd_rc
0xfe,0x03,
0x01,0x20, //27[6:5]clkctr [2]phy-lane1_en [1]phy-lane0_en [0]phy_clk_en
0x02,0x56, //[7:6]data1ctr [5:4]data0ctr [3:0]mipi_diff
0x03,0xb2, //b6[7]clklane_p2s_sel [6:5]data0hs_ph [4]data0_delay1s [3]clkdelay1s [2]mipi_en [1:0]clkhs_ph
0x12,0x80,
0x13,0x07, //LWC
0xfe,0x00,
0x3e,0x40, //91[7]lane_ena [6]DVPBUF_ena [5]ULPEna [4]MIPI_ena [3]mipi_set_auto_disable [2]RAW8_mode [1]ine_sync_mode [0]double_lane_en
};

// 1080P/30fpsÃ¦Â¨Â¡Ã¥Â¼Â 10bit
int gc2053_init_30fps_10bit  (void)
{
	int i;
	XM_printf ("cmos sensor gc2053 init 1080P 30fps 10bit mode\n");

	for(i = 0; i < sizeof(stSensor_cfg_1080P30FPS_10bit)/sizeof(stSensor_cfg_1080P30FPS_10bit[0]); i ++)
	{
		if(I2C_WRITE (stSensor_cfg_1080P30FPS_10bit[i].addr, stSensor_cfg_1080P30FPS_10bit[i].data) < 0)
			return -1;
	}

	return 0;
}

// 1080P/25fpsÃ¦Â¨Â¡Ã¥Â¼Â 10bit
int gc2053_init_25fps_10bit  (void)
{
	int i;
	XM_printf ("cmos sensor gc2053 init 1080P 25fps 10bit mode\n");

	for(i = 0; i < sizeof(stSensor_cfg_1080P25FPS_10bit)/sizeof(stSensor_cfg_1080P25FPS_10bit[0]); i ++)
	{
		if(I2C_WRITE (stSensor_cfg_1080P25FPS_10bit[i].addr, stSensor_cfg_1080P25FPS_10bit[i].data) < 0)
			return -1;
	}

	return 0;
}

// Ã¥Ë†ÂÃ¥Â§â€¹Ã¥Å’â€“CMOS sensor
// Ã¨Â¿â€Ã¥â€ºÅ¾Ã¥â‚¬Â¼
//  0    success
// -1    failure
int arkn141_isp_cmos_sensor_init (void)
{
	// Ã¥Ë†ÂÃ¥Â§â€¹Ã¥Å’â€“ GC2053
	return 0;
}

// Ã¥Â¤ÂÃ¤Â½ÂCMOS sensor
// Ã¨Â¿â€Ã¥â€ºÅ¾Ã¥â‚¬Â¼
//  0    success
// -1    failure
int arkn141_isp_cmos_sensor_reset (void)
{
	// Ã¥Â¤ÂÃ¤Â½Â GC2053
	return 0;
}

// Ã¨Â¯Â»Ã¥Ââ€“sensorÃ§Å¡â€Ã¥Â¯â€Ã¥Â­ËœÃ¥â„¢Â¨Ã¥â€ â€¦Ã¥Â®Â¹
//		Ã¨Â¯Â»Ã¥â€¡ÂºÃ§Å¡â€Ã¦â€¢Â°Ã¥â‚¬Â¼Ã¤Â¿ÂÃ¥Â­ËœÃ¥Å“Â¨dataÃ¦Å’â€¡Ã©â€™Ë†Ã¦Å’â€¡Ã¥Ââ€˜Ã§Å¡â€Ã¥Å“Â°Ã¥Ââ‚¬.
//		Ã¨â€¹Â¥Ã¦â€¢Â°Ã¥â‚¬Â¼Ã¤Â¸ÂÃ¥Â¤Å¸32Ã¤Â½Â, Ã¥Â°â€ Ã©Â«ËœÃ¤Â½ÂÃ§â€Â¨0Ã¥Â¡Â«Ã¥â€¦â€¦
// Ã¨Â¿â€Ã¥â€ºÅ¾Ã¥â‚¬Â¼
//  0    success
// -1    failure
int arkn141_isp_cmos_sensor_read_register  (u32_t addr, u32_t *data)
{
    u8 val = i2c_smbus_read_byte_data(i2c_client, addr);
	*data = val;

    return 0;
}

// Ã¥â€ â„¢Ã¥â€¦Â¥Ã¦â€¢Â°Ã¦ÂÂ®Ã¥â€ â€¦Ã¥Â®Â¹Ã¥Ë†Â°sensorÃ§Å¡â€Ã¥Â¯â€Ã¥Â­ËœÃ¥â„¢Â¨
// Ã¨Â¿â€Ã¥â€ºÅ¾Ã¥â‚¬Â¼
//  0    success
// -1    failure
int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	return i2c_smbus_write_byte_data(i2c_client, addr, data);
}

#define	EXPOSURE_LINES_ADDR		(0x3012)	// Integration time adjustment (I2C) Designated in line units
#define	AGAIN_ADDR					(0x3060)	// analog_gain
#define	DGAIN_ADDR					(0x305E)	// global_gain

#define	STD_30_LINES	(1125)

#define	CMOS_STD_INTTIME	(STD_30_LINES-2)
static cmos_inttime_t cmos_inttime;
static cmos_gain_t cmos_gain;

static u16_t FRM_LENGTH = STD_30_LINES;

static u32_t regValTable[29][4] = { {0x00, 0x00,0x01,0x00},//0xb4 0xb3 0xb8 0xb9
                                {0x00, 0x10,0x01,0x0c},
                                {0x00, 0x20,0x01,0x1b},
                                {0x00, 0x30,0x01,0x2c},
                                {0x00, 0x40,0x01,0x3f},
                                {0x00, 0x50,0x02,0x16},
                                {0x00, 0x60,0x02,0x35},
                                {0x00, 0x70,0x03,0x16},
                                {0x00, 0x80,0x04,0x02},
                                {0x00, 0x90,0x04,0x31},
                                {0x00, 0xa0,0x05,0x32},
                                {0x00, 0xb0,0x06,0x35},
                                {0x00, 0xc0,0x08,0x04},
                                {0x00, 0x5a,0x09,0x19},
                                {0x00, 0x83,0x0b,0x0f},
                                {0x00, 0x93,0x0d,0x12},
                                {0x00, 0x84,0x10,0x00},
                                {0x00, 0x94,0x12,0x3a},
                                {0x01, 0x2c,0x1a,0x02},
                                {0x01, 0x3c,0x1b,0x20},
                                {0x00, 0x8c,0x20,0x0f},
                                {0x00, 0x9c,0x26,0x07},
                                {0x02, 0x64,0x36,0x21},
                                {0x02, 0x74,0x37,0x3a},
                                {0x00, 0xc6,0x3d,0x02},
                                {0x00, 0xdc,0x3f,0x3f},
                                {0x02, 0x85,0x3f,0x3f},
                                {0x02, 0x95,0x3f,0x3f},
                                {0x00, 0xce,0x3f,0x3f},
   						};

static u32_t analog_gain_table[29] =
{
    /* zephyr@2017-04-18 */
    1024,
	1184,
	1424,
	1632,
	2032,
	2352,
	2832,
	3248,
	4160,
	4800,
	5776,
	6640,
	8064,
	9296,
	11552,
	13312,
	16432,
	18912,
	22528,
	25936,
	31840,
	36656,
	45600,
	52512,
	64768,
	82880,
	88000,
	107904,
	113168,
};

static u32_t digital_gain_table[] =
{
   64,	65,  66,	 67,	68,	69,	70,	71,
   72,	73,  74,	 75,	76,	77,	78,	79,
   80,	81,  82,	 83,	84,	85,	86,	87,
	88,	89,  90,	 91,	92,	93,	94,	95,
   96,	97,  98,	 99,	100,	101,	102,	103,
   104,	105, 106, 107,	108,	109,	110,	111,
   112,	113, 114, 115,	116,	117,	118,	119,
   120,	121, 122, 123,	124,	125,	126,	127
};


// PCLK = 74.25MHz
// Frame Size = 2040 * 1214
// fps = 74.25 * 1000000 / (2040 * 1214) = 29.98 Ö¡/Ãë
static  cmos_inttime_ptr_t cmos_inttime_initialize(void)
{
	cmos_inttime.full_lines = FRM_LENGTH;
	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.max_lines_target = (u16_t)(FRM_LENGTH - 2);
	cmos_inttime.min_lines_target = 2;

	cmos_inttime.exposure_ashort = 0;

	return &cmos_inttime;
}

// The sensor's integration time is obtained by the following formula.
static void cmos_inttime_update (cmos_inttime_ptr_t p_inttime)
{
	u16_t exp_time;

	exp_time = (u16_t)p_inttime->exposure_ashort;

	if(exp_time < 2)
		exp_time = 2;

	//arkn141_isp_cmos_sensor_write_register (0xfe, 0x00 ) ;		// Page 0
	arkn141_isp_cmos_sensor_write_register (0x03, (u8_t)((exp_time >> 8) & 0x3F) );	// P0:0x03 Exposure [13:8]
	arkn141_isp_cmos_sensor_write_register (0x04, (u8_t)(exp_time     ) );	//P0:0x04 Exposure [7:0]
}

// ÉèÖÃCMOS sensorÔÊĞíÊ¹ÓÃµÄ×î´óÔöÒæ
static int gc2053_cmos_max_gain_set (cmos_gain_ptr_t gain, unsigned int max_analog_gain,  unsigned int max_digital_gain);


// The Programmable Gain Control (PGC) of this device consists of the analog block and digital block.
static cmos_gain_ptr_t cmos_gain_initialize(void)
{
	cmos_gain.again_shift = 10;
#if IQ_20190522
	cmos_gain.max_again_target = 113168;	// 111±¶
#else
	// 20190626 ¼õĞ¡×î´óÔöÒæ±¶Êıµ½72±¶£¬¼õÇáµÍÕÕ¶ÈÏÂµÄÔëÉù
	cmos_gain.max_again_target = 72 * 1024;	// 72±¶
#endif	
	cmos_gain.again_count = sizeof(analog_gain_table)/sizeof(analog_gain_table[0]);

	cmos_gain.dgain_shift = 6;
	//cmos_gain.max_dgain_target = 127;// 2±¶
	cmos_gain.max_dgain_target = 64;	
	cmos_gain.dgain_count = sizeof(digital_gain_table)/sizeof(digital_gain_table[0]);

	return &cmos_gain;
}

// ÉèÖÃCMOS sensorÔÊĞíÊ¹ÓÃµÄ×î´óÔöÒæ
int gc2053_cmos_max_gain_set (cmos_gain_ptr_t gain, unsigned int max_analog_gain,  unsigned int max_digital_gain)
{
	int count, index;
	if(gain == NULL)
		return -1;
	if(max_analog_gain == 0)
	{
		max_analog_gain = 1;		// ½ûÖ¹Ä£ÄâÔöÒæ
	}

	if(gain->max_again_target != 1)
	{
		// ÔÚÔöÒæ±íÖĞ²éÕÒ¸Ã×î´óÔöÒæÖµ
		count = sizeof(analog_gain_table) / sizeof(analog_gain_table[0]);
		for (index = 0; index < count; index ++)
		{
			if(analog_gain_table[index] >= max_analog_gain)
			{
				break;
			}
		}
		if(index >= count)
			index = count - 1;	// Ê¹ÓÃ×îºóÒ»¸öÔöÒæÖµ

		// ĞŞ¸Ä×î´ó¿ÉÓÃµÄÔöÒæÖµ
		gain->max_again_target = analog_gain_table[index];
		gain->again_count = index + 1;		// ĞŞ¸Ä¿ÉÓÃµÄÔöÒæÏîÊıÁ¿
	}


	if(max_digital_gain == 0)
	{
		max_digital_gain = 1;		// ½ûÖ¹Êı×ÖÔöÒæ
	}

	if(gain->max_dgain_target != 1)
	{
		// ÔÚÔöÒæ±íÖĞ²éÕÒ¸Ã×î´óÔöÒæÖµ
		count = sizeof(digital_gain_table) / sizeof(digital_gain_table[0]);
		for (index = 0; index < count; index ++)
		{
			if(digital_gain_table[index] >= max_digital_gain)
			{
				break;
			}
		}
		if(index >= count)
			index = count - 1;	// Ê¹ÓÃ×îºóÒ»¸öÔöÒæÖµ

		// ĞŞ¸Ä×î´ó¿ÉÓÃµÄÔöÒæÖµ
		gain->max_dgain_target = digital_gain_table[index];
		gain->dgain_count = index + 1;		// ĞŞ¸Ä¿ÉÓÃµÄÔöÒæÏîÊıÁ¿
	}

	return 0;
}

// ÉèÖÃCMOS sensorÔÊĞíÊ¹ÓÃµÄ×î´óÔöÒæ
int gc2053_cmos_max_gain_get (cmos_gain_ptr_t gain, unsigned int* max_analog_gain,  unsigned int* max_digital_gain)
{
	if(gain == NULL)
		return -1;
	*max_analog_gain = gain->max_again_target;
	*max_digital_gain = gain->max_dgain_target;
	return 0;
}

static void cmos_gain_update (cmos_gain_ptr_t gain)
{
	u8_t u8DgainHigh, u8DgainLow;
	u32_t dgain = digital_gain_table[gain->dindex];

	u8DgainHigh = (dgain >> 6) & 0x0f;
	u8DgainLow = (dgain & 0x3f) << 2;
	arkn141_isp_cmos_sensor_write_register (0xb1, u8DgainHigh);
	arkn141_isp_cmos_sensor_write_register (0xb2, u8DgainLow);

	// analog gain
	arkn141_isp_cmos_sensor_write_register (0xb4, regValTable[gain->aindex][0]);
	arkn141_isp_cmos_sensor_write_register (0xb3, regValTable[gain->aindex][1]);
	arkn141_isp_cmos_sensor_write_register (0xb8, regValTable[gain->aindex][2]);
	arkn141_isp_cmos_sensor_write_register (0xb9, regValTable[gain->aindex][3]);
}

// ¸ù¾İÆØ¹âÁ¿¼ÆËãÄ£ÄâÔöÒæ
static u32_t analog_gain_from_exposure_calculate (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max)
{
	// ¶ş·Ö·¨²éÕÒ×î½Ó½üµÄÄ£ÄâÔöÒæ
	// ¼ÆËã¾«¶È·Ç³£ÖØÒª,»áµ¼ÖÂÆØ¹âµÄ¶¶¶¯
	int l, h, m, match;
	i64_t exp;
	i64_t mid;
	u32_t again = 1 << gain->again_shift;
	match = 0;
	if(exposure <= exposure_max)
	{
		gain->again = analog_gain_table[0];
		gain->aindex = 0;
		return exposure;
	}
	if(gain->again_count == 0)
	{
		gain->again = analog_gain_table[0];
		gain->aindex = 0;
		return exposure;
	}
	l = 0;
	h = gain->again_count - 1;

	if(h < 0)
		h = 0;

	//h = sizeof(analog_gain_table)/sizeof(analog_gain_table[0]) - 1;
	//exp = (i64_t)(1 << gain->again_shift);
	//exp = exp * (i64_t)exposure;
	exp = exposure;
	while(l <= h)
	{
		m = (l + h) >> 1;
		mid = analog_gain_table[m];
		mid = mid * (i64_t)exposure_max;
		mid = div_s64(mid, again);
		// Ñ°ÕÒÂú×ã mid <= expµÄ×î´óm
		if(mid < exp)
		{
			if(m > match)
				match = m;
			// ĞèÔö´óÄ£ÄâÔöÒæ
			l = m + 1;
		}
		else if(mid > exp)
		{
			// Ğè¼õĞ¡Ä£ÄâÔöÒæ
			h = m - 1;
		}
		else
		{
			// mid == exp
			match = m;
			break;
		}
	}
	m = match;
	gain->again = analog_gain_table[m];
	gain->aindex = (u16_t)m;
	//return (u32_t)(exp / analog_gain_table[m]);
	exp = exp * (i64_t)again;
	exp = div_s64(exp, analog_gain_table[m]);
	return (u32_t)exp;
}



// ¸ù¾İÆØ¹âÁ¿¼ÆËãÊı×ÖÔöÒæ
// Êı×ÖÔöÒæ
static u32_t digital_gain_from_exposure_calculate (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max)
{
	int l, h, m, match;
	i64_t exp;
	i64_t mid;
	u32_t dgain = 1 << gain->dgain_shift;
	match = 0;
	if(exposure <= exposure_max)
	{
		gain->dgain = digital_gain_table[0];
		gain->dindex = 0;
		return exposure;
	}
	if(gain->dgain_count == 0)
	{
		gain->dgain = digital_gain_table[0];
		gain->dindex = 0;
		return exposure;
	}
	l = 0;
	h = gain->dgain_count - 1;

	if(h < 0)
		h = 0;

	exp = exposure;
	while(l <= h)
	{
		m = (l + h) >> 1;
		mid = digital_gain_table[m];
		mid = mid * (i64_t)exposure_max;
		mid = div_s64(mid, dgain);
		// Ñ°ÕÒÂú×ã mid <= expµÄ×î´óm
		if(mid < exp)
		{
			if(m > match)
				match = m;
			// ĞèÔö´óÄ£ÄâÔöÒæ
			l = m + 1;
		}
		else if(mid > exp)
		{
			// Ğè¼õĞ¡Ä£ÄâÔöÒæ
			h = m - 1;
		}
		else
		{
			// mid == exp
			match = m;
			break;
		}
	}
	m = match;
	gain->dgain = digital_gain_table[m];
	gain->dindex = (u16_t)m;
	//return (u32_t)(exp / analog_gain_table[m]);
	exp = exp * (i64_t)dgain;
	exp = div_s64(exp, digital_gain_table[m]);
	return (u32_t)exp;
}

static u32_t last_exp_time, last_again, last_dgain;

static void cmos_inttime_gain_reset (void)
{
	last_exp_time = 0xFFFFFFFF;
	last_again = 0xFFFFFFFF;
	last_dgain = 0xFFFFFFFF;
}

// ·µ»ØÖµ±íÊ¾ÑÓ³ÙÖ¸¶¨µÄÊ±¼äÒÔ±ãµÈ´ısensorÆØ¹â²ÎÊı²úÉú×÷ÓÃ
//	1	±íÊ¾ÏÂÒ»Ö¡sensorÆØ¹â²ÎÊı²úÉúÓ°Ïì
//	2	±íÊ¾ÑÓ³Ù1Ö¡sensorÆØ¹â²ÎÊı²úÉúÓ°Ïì
//	3	±íÊ¾ÑÓ³Ù2Ö¡sensorÆØ¹â²ÎÊı²úÉúÓ°Ïì
static int cmos_inttime_gain_update (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain)
{
	u16_t exp_time;

	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
		if(exp_time < 2)
			exp_time = 2;

		//arkn141_isp_cmos_sensor_write_register (0xfe, 0x00 ) ;		// Page 0
		arkn141_isp_cmos_sensor_write_register (0x03, (u8_t)((exp_time >> 8) & 0x3F) );	// P0:0x03 Exposure [13:8]
		arkn141_isp_cmos_sensor_write_register (0x04, (u8_t)(exp_time     ) );	//P0:0x04 Exposure [7:0]
	}

	if(gain)
	{
		cmos_gain_update (gain);
	}

	// ·µ»ØÖµ±íÊ¾ÑÓ³ÙÖ¸¶¨µÄÊ±¼äÒÔ±ãµÈ´ısensorÆØ¹â²ÎÊı²úÉú×÷ÓÃ
	//	1	±íÊ¾ÏÂÒ»Ö¡sensorÆØ¹â²ÎÊı²úÉúÓ°Ïì
	//	2	±íÊ¾ÑÓ³Ù1Ö¡sensorÆØ¹â²ÎÊı²úÉúÓ°Ïì
	//	3	±íÊ¾ÑÓ³Ù2Ö¡sensorÆØ¹â²ÎÊı²úÉúÓ°Ïì
	return 2;
}

static void cmos_inttime_gain_update_manual (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain)
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;
	int i, aindex, dindex;

	// Ã¨Â®Â¡Ã§Â®â€”aindex, dindex
	for (i = 0; i < gain->again_count; i++)
	{
		if(analog_gain_table[i] >= gain->again)
			break;
	}
	if(i == gain->again_count)
		i = gain->again_count - 1;
	aindex = i;
	gain->aindex = aindex;
	gain->again_lookup = i;

	for (i = 0; i < gain->dgain_count; i++)
	{
		if(digital_gain_table[i] >= gain->dgain)
			break;
	}
	if(i == gain->dgain_count)
		i = gain->dgain_count - 1;
	dindex = i;
	gain->dindex = dindex;

	if(p_inttime->exposure_ashort >= (FRM_LENGTH - 2))
		p_inttime->exposure_ashort = (FRM_LENGTH - 2);

	cmos_inttime_gain_update (p_inttime, gain);

}


static u32_t cmos_get_iso (cmos_gain_ptr_t gain)
{
	i64_t iso = gain->again * gain->dgain;
	iso = (iso * 100) >> (gain->again_shift + gain->dgain_shift);

	gain->iso =  (u32_t)iso;

	return gain->iso;
}

static void cmos_fps_set (cmos_inttime_ptr_t p_inttime, u8_t fps)
{
	switch (fps)
	{
		case 30:
		default:
			p_inttime->full_lines = FRM_LENGTH;	//STD_30_LINES;
			p_inttime->lines_per_500ms = FRM_LENGTH * 30 / 2;	//STD_30_LINES * 30 / 2;
			break;
	}
}

	// ÉèÖÃsensor readout drection
	// horz_reverse_direction --> 1  horz reverse direction ´¹Ö±·´Ïò
	//                        --> 0  horz normal direction
	// vert_reverse_direction --> 1  vert reverse direction Ë®Æ½·´Ïò
	//                        --> 0  vert normal direction
static int cmos_sensor_set_readout_direction (u8_t horz_reverse_direction, u8_t vert_reverse_direction)
{
	return 0;
}

static const char *gc2053_cmos_sensor_get_sensor_name (void)
{
	return "GC2053";
}

extern void isp_sensor_set_reset_pin_low (void);
extern void isp_sensor_set_reset_pin_high (void);
extern int gc2053_init_25fps_10bit  (void);
extern int gc2053_init_30fps_10bit  (void);



static int gc2053_isp_sensor_init(isp_sen_ptr_t p_sen)
{
	int ret = 0;
	int loop = 10;
	int video_format = ARKN141_VIDEO_FORMAT_1080P_30;

	if(	video_format == ARKN141_VIDEO_FORMAT_1080P_30
		)
	{

	}
	else
	{
		// un-support video format
		XM_printf ("un-support video format (%d)\n", video_format );
		return -1;
	}

	while(loop > 0)
	{
		// tLOW >= 500ns
		isp_sensor_set_reset_pin_low ();
		mdelay (5);
		isp_sensor_set_reset_pin_high ();
		mdelay (2);
		cmos_inttime_gain_reset ();
		if (isp_sensor_fps == 30)
			ret = gc2053_init_30fps_10bit ();
		else		
			ret = gc2053_init_25fps_10bit ();
		if(ret == 0)
		{
			//cmos_sensor_set_readout_direction (1, 1);
		}
		if(ret == 0)
			break;
		loop --;
	}
	if(loop == 0)
	{
		ret = -1;
		XM_printf ("gc2053 init video format(%d) NG\n", video_format);
	}
	else
	{
		ret = 0;
		XM_printf ("gc2053 init video format(%d) OK\n", video_format);
	}

	return ret;
}

static void gc2053_cmos_isp_awb_init (isp_awb_ptr_t p_awb)		// °×Æ½ºâ³õÊ¼²ÎÊı
{
  //p_awb->enable = 0;
  p_awb->enable = 1;
  p_awb->mode = 1; //0£ºÎŞĞ§;  1£ºËã·¨1£¬Í³Ò»¹À¼Æ;  2£ºËã·¨2£¬»ùÓÚ²Î¿¼¹âÔ´
  p_awb->manual = 0;//0=×Ô¶¯°×Æ½ºâ  1=ÊÖ¶¯°×Æ½ºâ
  p_awb->weight[0][0] = 1;
  p_awb->weight[0][1] = 2;
  p_awb->weight[0][2] = 1;
  p_awb->weight[1][0] = 2;
  p_awb->weight[1][1] = 4;
  p_awb->weight[1][2] = 2;
  p_awb->weight[2][0] = 1;
  p_awb->weight[2][1] = 2;
  p_awb->weight[2][2] = 1;
  //p_awb->black = 4;
  p_awb->black = 16;
  p_awb->white = 210;
  p_awb->jitter = 13;
  p_awb->r2g_min = 256/4;
  p_awb->r2g_max = 256*4;
  p_awb->b2g_min = 256/4;
  p_awb->b2g_max = 256*4;



  // G/R, G/B,
#if 0
  p_awb->r2g_light[0] = 182;   	// DAY
  p_awb->b2g_light[0] = 203;
  p_awb->r2g_light[1] = 194; 		// CWF
  p_awb->b2g_light[1] = 158;
  p_awb->r2g_light[2] = 222; 		// A
  p_awb->b2g_light[2] = 131;
  p_awb->r2g_light[3] = 246;		// TL84
  p_awb->b2g_light[3] = 154;
#else

  p_awb->r2g_light[0] = 120;   	// ×ÔÈ»¹â  						//old:117
  p_awb->b2g_light[0] = 153;										//old:158
  p_awb->r2g_light[1] = 127;   	// DAY							//old:124
  p_awb->b2g_light[1] = 161;										//old:162
  p_awb->r2g_light[2] = 155; 		// CWF							//old:151
  p_awb->b2g_light[2] = 96;										//old:98
  p_awb->r2g_light[3] = 208; 		// A								//old:206
  p_awb->b2g_light[3] = 83;										//old:82
  p_awb->r2g_light[4] = 241;		// TL84							//old:232
  p_awb->b2g_light[4] = 83;										//old:83

#endif
  p_awb->r2g_light[5] = 0;
  p_awb->b2g_light[5] = 0;
  p_awb->r2g_light[6] = 0;
  p_awb->b2g_light[6] = 0;
  p_awb->r2g_light[7] = 0;
  p_awb->b2g_light[7] = 0;

  p_awb->use_light[0] = 1;
  p_awb->use_light[1] = 1;
  p_awb->use_light[2] = 1;
  p_awb->use_light[3] = 1;
  p_awb->use_light[4] = 1;
  p_awb->use_light[5] = 0;
  p_awb->use_light[6] = 0;
  p_awb->use_light[7] = 0;

  p_awb->gain_g2r = 426;//434;
  p_awb->gain_g2b = 280;//348;

  isp_awb_init_io (p_awb);
}

static isp_gamma_polyline_tbl gamma_polyline_tbl[] = {
	// ÌáÉı³¡¾°µÄ¶Ô±È¶È
	{	128,
		{
			0,        	140,        336,        588,        896,       1260,       	1680,       2156,
			2688,       3276,       3920,       4620,       5376,       6188,       7056,       7980,
			8960,       9996,      	11088,      12236,      13440,      14700,      16016,      17388,
			18816,      20300,      21840,      23436,      25088,      26796,      28560,      30380,
			32256,      34644,      36464,      38228,      39936,      41588,      43184,      44724,
			46208,      47636,      49008,      50324,      51584,      52788,      53936,      55028,
			56064,      57044,      57968,      58836,      59648,      60404,      61104,      61748,
			62336,      62868,      63344,      63764,      64128,      64436,      64688,      64884,
			65024
		}
	},
};

static const isp_gamma_polyline_tbl *gc2053_cmos_isp_get_gamma_table (int *tbl_count)
{
	*tbl_count = sizeof(gamma_polyline_tbl)/sizeof(gamma_polyline_tbl[0]);
	return gamma_polyline_tbl;
}



static void gc2053_cmos_isp_colors_init (isp_colors_ptr_t p_colors)	// É«²Ê³õÊ¼²ÎÊı
{
  int i, j;

  p_colors->colorm.enable = 0;// É«¾ØÕó 
  p_colors->colorm.matrixcoeff[0][0] = 256;
  p_colors->colorm.matrixcoeff[0][1] = 0;
  p_colors->colorm.matrixcoeff[0][2] = 0;
  p_colors->colorm.matrixcoeff[1][0] = 0;
  p_colors->colorm.matrixcoeff[1][1] = 256;
  p_colors->colorm.matrixcoeff[1][2] = 0;
  p_colors->colorm.matrixcoeff[2][0] = 0;
  p_colors->colorm.matrixcoeff[2][1] = 0;
  p_colors->colorm.matrixcoeff[2][2] = 256;

  p_colors->gamma.enable =  1;
  for (i = 0; i < 65; i++)
  {
		p_colors->gamma.gamma_lut[i] = gamma_polyline_tbl[0].gamma_lut[i];
  }


 // Ê¹ÓÃ0~255·¶Î§, ¾¡Á¿±£ÁôËùÓĞµÄÏ¸½Ú
  p_colors->rgb2ypbpr_type = HDTV_type_0255;

  isp_create_rgb2ycbcr_matrix (p_colors->rgb2ypbpr_type, &p_colors->rgb2yuv);


  // demosaic ²ÎÊı
	p_colors->demosaic.mode = 0;
	p_colors->demosaic.coff_00_07 = 32;
	p_colors->demosaic.coff_20_27 = 255;	// ÂË²¨
	p_colors->demosaic.horz_thread = 0;
	//p_colors->demosaic.demk = 128;
	p_colors->demosaic.demk = 512;		// 20161230 ¸ÄÉÆ½âÎö¶È(Ê÷Ò¶,ÎÆÀí)
	p_colors->demosaic.demDhv_ofst = 0;

  isp_colors_init_io(p_colors);
}

static const unsigned char noise0_0[17] = {
255, 255, 255, 255,
255, 248, 240, 212,
212, 212, 212, 212,
212, 212, 212, 212,
212
};

static const unsigned char noise1_0[17] = {
255, 255, 255, 255,
255, 248, 240, 212,
192, 160, 144, 128,
128, 128, 128, 128,
128
};

static void gc2053_cmos_isp_denoise_init (isp_denoise_ptr_t p_denoise)// ½µÔë³õÊ¼ÉèÖÃ
{
  int i, x0, y0, x1, y1, x2, y2, x3, y3;
  int a, b, c, d, e, f, delta;

  p_denoise->enable2d = 7;
  if(isp_get_work_mode() == ISP_WORK_MODE_NORMAL)
  {
#if ISP_3D_DENOISE_SUPPORT
  		p_denoise->enable3d = 7;
#else
		p_denoise->enable3d = 0;
#endif
  }
  else
  {
		p_denoise->enable3d = 0;
  }

  //p_denoise->sensitiv0 = 4;
  //p_denoise->sensitiv1 = 4;
  p_denoise->sensitiv0 = 3;    	// ½µµÍ½µÔëÇ¿¶È, ÌáÉıÇåÎú¶È
  p_denoise->sensitiv1 = 3;

  p_denoise->sel_3d_table = 3;		// 3µÄ½µÔëĞ§¹û½ÏºÃ
  p_denoise->sel_3d_matrix = 1;



  p_denoise->y_thres0 = 6;
  p_denoise->u_thres0 = 10;
  p_denoise->v_thres0 = 10;

  p_denoise->y_thres1 = 6;
  p_denoise->u_thres1 = 10;
  p_denoise->v_thres1 = 10;

  p_denoise->y_thres2 = 6;
  p_denoise->u_thres2 = 11;
  p_denoise->v_thres2 = 11;

  for (i = 0; i <= 16; i ++)
  {
	  p_denoise->noise0[i] = noise0_0[i];
  }

  for (i = 0; i <= 16; i ++)
  {
	  p_denoise->noise1[i] = noise1_0[i];
  }


  isp_denoise_init_io (p_denoise);
}



// 20170227 Ôö¼Ó³¡¾°µÄ½âÎö¶ÈÀ­Éì, Ôö¼ÓÍ¨Í¸¶È, Ôö¼ÓµÍÕÕ¶È³¡¾°µÄ½âÎö¶È
static const unsigned char gc2053_default_resolt[33] = {
 200,  210,  215,  220,  225,  227,
 230,  230,  233,  235,  235,  235,
 235,  235,  235,  235,  235,  235,
 235,  235,  235,  235,  235,  235,
 235,  235,  235,  235,  235,  230,
 225,  220,  220,
};




#define	ERIS_COLORT_1 1
#if ERIS_COLORT_1
static const unsigned int gc2053_default_colort_old[33] = {
   64,     128,    192,    256,    320,    384,    511,    511,
   511,    511,    511,    511,    511,    511,    511,    511,
   511,    511,    511,    511,    511,    511,    511,    511,
   511,    511,    511,    511,    448,    384,    256,    192,
   128
};

static const unsigned int gc2053_default_colort[33] = {
   64,     128,    192,    256,    272,    320,    384,    384,
   384,    384,    384,    384,    384,    384,    384,    384,
   384,    384,    384,    384,    384,    384,    384,    384,
   384,    384,    384,    384,    320,    272,    256,    192,
   128
};

#else

/*
static unsigned int gc2053_default_colort[33] = {
   1,    1,    1,    1,    1,    1,    1,    1,
   16,    24,    32,    40,    64,   80,    96,    112,
   128,    160,    212,    256,    256,    256,    256,    256,
   256,    256,    256,    256,    256,    256,    256,    256,
   128
};*/

static const unsigned int gc2053_default_colort_1[33] = {
 32,   38,   44,   50,   56,   62,
  68,   74,   80,   86,   92,   98,
 104,  110,  116,  122,  128,  152,
 176,  200,  224,  248,  272,  296,
 320,  343,  367,  391,  415,  439,
 463,  487,  511,
};

static const unsigned int gc2053_default_colort[33] = {
  8,   8,   8,   8,   8,   8,
  16, 16,  16,  16,  16,  16,
 32,  32,  32,  32,  32,  48,
 48,  48,  48,  64,  64,  96,
 96,  64,  64,  48,  48,  48,
 32,  32,  32,
};
#endif

static void gc2053_cmos_isp_eris_init(isp_eris_ptr_t p_eris)			// ¿í¶¯Ì¬³õÊ¼ÉèÖÃ
{
  int i, j, x0, y0, x1, y1, x2, y2;
  int a, b, c, d;

  p_eris->enable = 1;
#if ERIS_MANUAL
  p_eris->manual = 1;
#else
  p_eris->manual = 0;
#endif
  p_eris->target = 128;
  p_eris->black = 12;
  // 10bitÊ¹ÓÃD2~D11, D0~D1¹Ì¶¨Îª0
  //if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
  //  p_eris->white = 4095;		// ¾¡¿ÉÄÜ±£Áô¶¯Ì¬·¶Î§
  //else
	  p_eris->white = 1023;		// ¾¡¿ÉÄÜ±£Áô¶¯Ì¬·¶Î§
  p_eris->gain_max = 256;	//256;
  //p_eris->gain_max = 128;	//256;
  //p_eris->gain_min = 64;	//256;
  p_eris->gain_min = 16;
  //p_eris->gain_man = 256;
  p_eris->gain_man = 360;
  p_eris->cont_max = 256;	// 2*64;
  p_eris->cont_min = 64;	//	6*64;
  p_eris->cont_man = 16;
  p_eris->dfsEris = 1;
  p_eris->varEris = 0;
  p_eris->resols = 0;
  p_eris->resoli = 0;
  p_eris->spacev = 0;


  for (i = 0; i < 33; i++)
  {
	  p_eris->resolt[i] = gc2053_default_resolt[i];
  }

    for (i = 0; i < 33; i++)
    {
		// p_eris->colort[i] = 512;
    	 p_eris->colort[i] = gc2053_default_colort[i];
    }



	// ERISÖ±·½Í¼³õÊ¼ÉèÖÃ 
	// u8_t hist_thresh[4] = {0x10, 0x40, 0x80, 0xc0};
  	p_eris->eris_hist_thresh[0] = 0x10;
	p_eris->eris_hist_thresh[1] = 0x40;
	p_eris->eris_hist_thresh[2] = 0x80;
	p_eris->eris_hist_thresh[3] = 0xC0;


  isp_eris_init_io(p_eris);
}

#if 1
const unsigned short lenscoeff[]=
{
  // r
  4096,4315,4322,4317,4331,     4339,4355,4361,4406,4443,
  4467,4466,4485,4519,4552,     4591,4648,4702,4771,4847,
  4923,5001,5125,5242,5373,     5518,5694,5885,6047,6230,
  6416,6597,6821,7055,7305,     7481,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,
  // g
  4096,4315,4322,4317,4331,     4339,4355,4361,4406,4443,
  4467,4466,4485,4519,4552,     4591,4648,4702,4771,4847,
  4923,5001,5125,5242,5373,     5518,5694,5885,6047,6230,
  6416,6597,6821,7055,7305,     7481,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,
  // b
   4096,4315,4322,4317,4331,     4339,4355,4361,4406,4443,
  4467,4466,4485,4519,4552,     4591,4648,4702,4771,4847,
  4923,5001,5125,5242,5373,     5518,5694,5885,6047,6230,
  6416,6597,6821,7055,7305,     7481,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,     7799,7799,7799,7799,7799,
  7799,7799,7799,7799,7799,
};
#else
const unsigned short lenscoeff[]=
{
  // r
  4096,4320,4335,4340,4351,     4368,4406,4442,4459,4467,
  4480,4510,4555,4594,4642,     4692,4750,4812,4883,4954,
  5109,5281,5463,5681,5900,     6172,6449,6789,7057,7387,
  7728,8106,8602,9000,9362,     9471,9826,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,
  // g
  4096,4320,4335,4340,4351,     4368,4406,4442,4459,4467,
  4480,4510,4555,4594,4642,     4692,4750,4812,4883,4954,
  5109,5281,5463,5681,5900,     6172,6449,6789,7057,7387,
  7728,8106,8602,9000,9362,     9471,9826,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,
  // b
  4096,4320,4335,4340,4351,     4368,4406,4442,4459,4467,
  4480,4510,4555,4594,4642,     4692,4750,4812,4883,4954,
  5109,5281,5463,5681,5900,     6172,6449,6789,7057,7387,
  7728,8106,8602,9000,9362,     9471,9826,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,10479,10479,10479,10479,10479,
  10479,10479,10479,10479,10479,
};
#endif


#define  Ycenter_x   960
#define  Ycenter_y   540
#define  CenterRx   Ycenter_x
#define  CenterRy   Ycenter_y
#define  CenterGx   Ycenter_x
#define  CenterGy   Ycenter_y
#define  CenterBx   Ycenter_x
#define  CenterBy   Ycenter_y

static void gc2053_cmos_isp_fesp_init(isp_fesp_ptr_t p_fesp)	// ¾µÍ·Ğ£Õı, fix-pattern-correction, »µµãÈ¥³ı³õÊ¼ÉèÖÃ
{
  int i, j, k;
  int x0, y0, x1, y1, x2, y2, x3, y3;
  int a, b, c, d, e, f, delta;
 // unsigned short R_lenslut[65];
//  unsigned short G_lenslut[65];
//  unsigned short B_lenslut[65];
//  unsigned short Y_lenslut[65];

  p_fesp->Lensshade.enable = 0;
  p_fesp->Lensshade.scale = 1;
  p_fesp->Lensshade.lscofst = 50;
  p_fesp->Lensshade.rcenterRx = CenterRx;
  p_fesp->Lensshade.rcenterRy = CenterRy;
  p_fesp->Lensshade.rcenterGx = CenterRx;
  p_fesp->Lensshade.rcenterGy = CenterRy;
  p_fesp->Lensshade.rcenterBx = CenterRx;
  p_fesp->Lensshade.rcenterBy = CenterRy;

  for( i=0 ; i < 195 ;i++ )
  {
    p_fesp->Lensshade.coef[i] = lenscoeff[i];
  }


  // fix pattern correction

  p_fesp->Fixpatt.enable = 1;
  p_fesp->Fixpatt.mode = 0;

	p_fesp->Fixpatt.rBlacklevel = 0x40;
	p_fesp->Fixpatt.grBlacklevel = 0x40;
	p_fesp->Fixpatt.gbBlacklevel = 0x40;
	p_fesp->Fixpatt.bBlacklevel = 0x40;
	p_fesp->Fixpatt.profile[0] = 255;
	p_fesp->Fixpatt.profile[1] = 255;
	p_fesp->Fixpatt.profile[2] = 255;
	p_fesp->Fixpatt.profile[3] = 255;
	p_fesp->Fixpatt.profile[4] = 255;
	p_fesp->Fixpatt.profile[5] = 255;
	p_fesp->Fixpatt.profile[6] = 255;
	p_fesp->Fixpatt.profile[7] = 255;
	p_fesp->Fixpatt.profile[8] = 255;
	p_fesp->Fixpatt.profile[9] = 255;
	p_fesp->Fixpatt.profile[10] = 255;
	p_fesp->Fixpatt.profile[11] = 255;
	p_fesp->Fixpatt.profile[12] = 255;
	p_fesp->Fixpatt.profile[13] = 255;
	p_fesp->Fixpatt.profile[14] = 255;
	p_fesp->Fixpatt.profile[15] = 255;
	p_fesp->Fixpatt.profile[16] = 255;

  // bad pixel correction
  p_fesp->Badpix.enable = 1;
  p_fesp->Badpix.mode = 0;
  p_fesp->Badpix.thresh = 19;		// IMX322 Êµ²â×îµÍ»µµãÅĞ¶ÏãĞÖµ
  p_fesp->Badpix.profile[0] = 255;
  p_fesp->Badpix.profile[1] = 255;
  p_fesp->Badpix.profile[2] = 255;
  p_fesp->Badpix.profile[3] = 255;
  p_fesp->Badpix.profile[4] = 255;
  p_fesp->Badpix.profile[5] = 255;
  p_fesp->Badpix.profile[6] = 255;
  p_fesp->Badpix.profile[7] = 255;
  p_fesp->Badpix.profile[8] = 255;
  p_fesp->Badpix.profile[9] = 255;
  p_fesp->Badpix.profile[10] = 255;
  p_fesp->Badpix.profile[11] = 255;
  p_fesp->Badpix.profile[12] = 255;
  p_fesp->Badpix.profile[13] = 255;
  p_fesp->Badpix.profile[14] = 255;
  p_fesp->Badpix.profile[15] = 255;

  // cross talk correction
  // cross talkµÄãĞÖµÔ½´ó, Í¼ÏñÔ½Ä£ºı. 8ÊÇÒ»¸ö½ÏºÏÊÊµÄÖµ. 
  // Ê¹ÓÃ3D½µÔëÀ´È¥³ıÔëÉù
  p_fesp->Crosstalk.enable = 1;
  p_fesp->Crosstalk.mode = 1;
  p_fesp->Crosstalk.thresh = 10;
  p_fesp->Crosstalk.snsCgf = 3;		//		ÖµÔ½´ó, ÂË³ıÆæÒìµãµÄÄÜÁ¦Ô½´ó.
  p_fesp->Crosstalk.thres0cgf = 10;
  p_fesp->Crosstalk.thres1cgf = 10;
  p_fesp->Crosstalk.profile[0] = 255;
  p_fesp->Crosstalk.profile[1] = 255;
  p_fesp->Crosstalk.profile[2] = 255;
  p_fesp->Crosstalk.profile[3] = 243;
  p_fesp->Crosstalk.profile[4] = 243;
  p_fesp->Crosstalk.profile[5] = 243;
  p_fesp->Crosstalk.profile[6] = 243;
  p_fesp->Crosstalk.profile[7] = 243;
  p_fesp->Crosstalk.profile[8] = 232;
  p_fesp->Crosstalk.profile[9] = 232;
  p_fesp->Crosstalk.profile[10] = 232;
  p_fesp->Crosstalk.profile[11] = 232;
  p_fesp->Crosstalk.profile[12] = 232;
  p_fesp->Crosstalk.profile[13] = 212;
  p_fesp->Crosstalk.profile[14] = 212;
  p_fesp->Crosstalk.profile[15] = 212;
  p_fesp->Crosstalk.profile[16] = 212;

  isp_fesp_init_io (p_fesp);
}

#define	SATUATION_OFFSET		64		// ±¥ºÍ¶È²¹³¥
static void gc2053_cmos_isp_enhance_init (isp_enhance_ptr_t p_enhance)	// Í¼ÏñÔöÇ¿³õÊ¼ÉèÖÃ
{
  p_enhance->sharp.enable = 1;
  p_enhance->sharp.mode = 0;
  p_enhance->sharp.coring = 0;// 0-7
  //p_enhance->sharp.strength = 64;//64;//32;//128;
  //p_enhance->sharp.strength = 32;
  p_enhance->sharp.strength = 255;
  //p_enhance->sharp.strength = 196;
  //p_enhance->sharp.gainmax = 256;
  //p_enhance->sharp.gainmax = 128;		// 20170223 Ã¤Â¿Â®Ã¦â€Â¹Ã¤Â¸ÂºÃ¨Â½Â»Ã¥Â¾Â®Ã§Å¡â€Ã©â€ÂÃ¥Å’â€“
  //p_enhance->sharp.gainmax = 144;		// 20170305 Ã¥Â¾Â®Ã¨Â°Æ’, Ã¥Â¢Å¾Ã¥Å Â Ã¤Â¸â‚¬Ã§â€šÂ¹,
  //p_enhance->sharp.gainmax = 160;		// 20170803Ã§â„¢Â½Ã¥Â¤Â©Ã¨Â·Â¯Ã¦Âµâ€¹Ã§Å¡â€Ã¨Â§â€ Ã©Â¢â€˜Ã¨Â½Â¦Ã§â€°Å’Ã§Å¡â€Ã¨Â¯â€ Ã¥Ë†Â«Ã¦Â¯â€Ã§Â¬Â¬Ã¤Â¸â‚¬Ã§Å½Â°Ã¥Å“Âº0330Ã§Â¨ÂÃ¥Â·Â®, Ã¥Â¢Å¾Ã¥Å Â Ã©â€ÂÃ¥Å’â€“Ã§Â¨â€¹Ã¥ÂºÂ¦Ã¦â€Â¹Ã¥â€“â€Ã¨Â¯â€ Ã¥Ë†Â«Ã¥ÂºÂ¦
  p_enhance->sharp.gainmax = 255;		// 20170805Ã¦Â Â¹Ã¦ÂÂ®20170804Ã¨Â·Â¯Ã¦Âµâ€¹Ã§Â»â€œÃ¦Å¾Å“,Ã¨Â½Â¦Ã§â€°Å’Ã¨Â¾Â¨Ã¨Â¯â€ Ã¥ÂºÂ¦Ã¨Â¾Æ’0330Ã§Â¨ÂÃ¥Â·Â®,
													//   Ã©â‚¬Å¡Ã¨Â¿â€¡Ã¥Ë†â€ Ã¦Å¾ÂÃ¨Â§â€ Ã©Â¢â€˜, Ã§Â¬Â¬Ã¤Â¸â‚¬Ã§Å½Â°Ã¥Å“Âº(0330)Ã§Å¡â€Ã©â€ÂÃ¥Å’â€“Ã¥ÂºÂ¦Ã¨Â¾Æ’Ã©Â«Ëœ, Ã¦ÂÂÃ¥Ââ€¡Ã©â€ÂÃ¥Å’â€“Ã¥Â¢Å¾Ã§â€ºÅ Ã¨â€¡Â³256
  p_enhance->bcst.enable = 1;
  //p_enhance->bcst.bright = -24; // -256~255
  p_enhance->bcst.bright = 0; // -256~255
  p_enhance->bcst.contrast = 1024;//1024; // 0~1.xxx
  p_enhance->bcst.satuation = 1024 + SATUATION_OFFSET; //0~1.xxx
  // p_enhance->bcst.hue = 0; // -128~127
  p_enhance->bcst.hue = 0;
  p_enhance->bcst.offset0 = 0; // 0~255
  p_enhance->bcst.offset1 = 128; // 0~255
  //p_enhance->bcst.offset1 = 116; // 0~255

  isp_enhance_init_io (p_enhance);
}

static void gc2053_cmos_isp_ae_init (isp_ae_ptr_t p_ae)		// ×Ô¶¯ÆØ¹â³õÊ¼ÉèÖÃ
{
	p_ae->histoBand[0] = 0x10;
	p_ae->histoBand[1] = 0x40;
	p_ae->histoBand[2] = 0x80;
	p_ae->histoBand[3] = 0xc0;

	p_ae->winWeight[0][0] = 1;
	p_ae->winWeight[0][1] = 2;
	p_ae->winWeight[0][2] = 1;
	p_ae->winWeight[1][0] = 6;
	p_ae->winWeight[1][1] = 14;
	p_ae->winWeight[1][2] = 6;
	p_ae->winWeight[2][0] = 12;
	p_ae->winWeight[2][1] = 15;
	p_ae->winWeight[2][2] = 12;

	//p_ae->bright_target = 10;
	p_ae->bright_target = AE_BRIGHT_TARGET_DEFAULT;
	//p_ae->bright_target = 26;
	p_ae->black_target = 128;
	p_ae->compensation = AE_COMPENSATION_DEFAULT;
}

static void gc2053_cmos_isp_sys_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp)		// ÏµÍ³³õÊ¼ÉèÖÃ (sensor pixelÎ»Êı, bayer mode)
{
	p_sys->ispenbale = 0;

	p_sys->ckpolar = 0;
	p_sys->vcpolar = 0;
	p_sys->hcpolar = 0;

	p_sys->vmskenable = 0;	// ×Ô¶¯¶ªÖ¡. ±£Áô,±ØĞëÉèÖÃÎª0
	p_sys->frameratei = 0;
	p_sys->framerateo = 0; 	// ±£Áô,±ØĞëÉèÖÃÎª0

#if 0
	p_sys->frameratei = 30;
	p_sys->framerateo = 0; 	// ±£Áô,±ØĞëÉèÖÃÎª0
	p_sys->vifrasel0 = 0xAAAAAAAA;
	p_sys->vifrasel1 = 0;

#else
	p_sys->frameratei = 0;
	p_sys->framerateo = 0;
	p_sys->vifrasel0 = 0;
	p_sys->vifrasel1 = 0;
#endif

	// IN/OUT (60Ö¡/55Ö¡, ), 
#if 0
	p_sys->frameratei = 64;
	p_sys->framerateo = 64;
	p_sys->vifrasel0 = 0xFFFFFFFF;	// 29
	// p_sys->vifrasel1 = 0x07FFEFFE;	// 25
   p_sys->vifrasel1 = 0xFFFFFFFF;	// 22
#endif
	// IN/OUT (1Ö¡/1Ö¡)
	//  p_sys->frameratei = 0;
	//  p_sys->vifrasel0 = 0x00000000;
	// p_sys->vifrasel1 = 0x00000000;


	//(0.±íÊ¾8Î» 1:±íÊ¾10Î» 2:±íÊ¾12Î»  3:±íÊ¾14Î»)
	//XM_printf("sensor bit: 0:8bit 1:10bit 2:12bit 3:14bit  \n");
	p_sys->sensorbit = ARKN141_ISP_SENSOR_BIT_10;

	// 0:RGGB 1:GRBG 2:BGGR 3:GBRG
	//XM_printf("bayer mode: 0:RGGB 1:GRBG 2:BGGR 3:GBRG \n");
	//p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_BGGR;
	p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;

	p_sys->imagewidth = p_isp->image_width;
	p_sys->imageheight = p_isp->image_height;

	// imagehblank, zonestridex, zonestridey ÊÇÒÔISP CoreµÄÊ±ÖÓÎª¼ÆÊı»ù×¼, 
	// ¼ÆËãÊ±Ê×ÏÈ°´ÕÕSensor Pixel ClockÊ±Ğò½øĞĞ¼ÆËã, È»ºó»»Ëãµ½ISP Core Clock,
	p_sys->imagehblank = 0;
	p_sys->zonestridex = 0;
	p_sys->zonestridey = 0;

	p_sys->resizebit	= 2;


	//Ê¹ÄÜ
	p_sys->vmanSftenable = 0;
	p_sys->vchkIntenable = 1;// Ö¡¿ªÊ¼
	p_sys->pabtIntenable = 1; //µãÒì³£
	p_sys->fendIntenable = 1; // Ö¡Íê³É
	p_sys->fabtIntenable = 1; // µØÖ·Òì³£
	p_sys->babtIntenable = 1; //×ÜÏßÒì³£
	p_sys->ffiqIntenable = 0;  //¿ìÖĞ¶Ï
	p_sys->pendIntenable = 1;  //ÖĞÖ¹ £¬Èç¹ûÒ»Ö¡Î´Íê³É£¬»áÍê³É¸ÃÖ¡

	p_sys->infoIntenable = 1;	// Ê¹ÄÜISPµÄÆØ¹âÍ³¼ÆÍê³ÉÖĞ¶Ï

	// ÉèÖÃ ³¡ 
	p_sys->vmanSftset = 0;
	p_sys->vchkIntclr = 1;
	p_sys->pabtIntclr = 1;
	p_sys->fendIntset = 1;
	p_sys->fendIntclr = 1;
	p_sys->fabtIntclr = 1;
	p_sys->babtIntclr = 1;
	p_sys->ffiqIntclr = 1;
	p_sys->pendIntclr = 1;
	p_sys->infoStaclr = 1;

	p_sys->vchkIntraw = 0;
	p_sys->pabtIntraw = 0;
	p_sys->fendIntraw = 0;
	p_sys->fabtIntraw = 0;
	p_sys->babtIntraw = 0;
	p_sys->ffiqIntraw = 0;
	p_sys->pendIntraw = 0;

	p_sys->vchkIntmsk = 0;
	p_sys->pabtIntmsk = 0;
	p_sys->fendIntmsk = 0;
	p_sys->fabtIntmsk = 0;
	p_sys->babtIntmsk = 0;
	p_sys->ffiqIntmsk = 0;
	p_sys->pendIntmsk = 0;

	p_sys->fendIntid[0] = 0;
	p_sys->fendIntid[1] = 1;
	p_sys->fendIntid[2] = 2;
	p_sys->fendIntid[3] = 3;
	p_sys->ffiqIntdelay = 4;// ¿ìÖĞ¶Ï 
	p_sys->fendStaid = 0;
	p_sys->infoStadone = 0;
	if(isp_get_work_mode() == ISP_WORK_MODE_NORMAL)
	{
		p_sys->debugmode  = 0;
		p_sys->testenable = 0; // ¿ªÆôdram²âÊÔÄ£Ê½  
		p_sys->rawmenable = 0; // 1 ÔÊĞíRAWĞ´³ö
		p_sys->yuvenable  = 1; // 0:¹ØµôÊı¾İÊä³ö  1:´ò¿ª
#if ISP_3D_DENOISE_SUPPORT
		p_sys->refenable  = 1; // 1;3D ²Î¿¼Ö¡¿ªÆô 0:¹Ø±Õ 
#else
		p_sys->refenable  = 0; // 1;3D ²Î¿¼Ö¡¿ªÆô 0:¹Ø±Õ 
#endif
	}
	else if(isp_get_work_mode() == ISP_WORK_MODE_RAW)
	{
		// RAWĞ´³ö»áÕ¼ÓÃ3D²Î¿¼Ö¡Í¨µÀ
		p_sys->debugmode  = 1;
		p_sys->testenable = 0; // ¿ªÆôdram²âÊÔÄ£Ê½  
		p_sys->rawmenable = 1; // 1 ÔÊĞíRAWĞ´³ö
		p_sys->yuvenable  = 1; // 0:¹ØµôÊı¾İÊä³ö  1:´ò¿ª
		p_sys->refenable  = 0; // 1;3D ²Î¿¼Ö¡¿ªÆô 0:¹Ø±Õ 
	}
	else if(isp_get_work_mode() == ISP_WORK_MODE_AUTOTEST)
	{
		p_sys->debugmode  = 1;
		p_sys->testenable = 1; // ¿ªÆôdram²âÊÔÄ£Ê½ 
		p_sys->rawmenable = 0; // 1 ÔÊĞíRAWĞ´³ö
		p_sys->yuvenable  = 1; // 0:¹ØµôÊı¾İÊä³ö  1:´ò¿ª
		p_sys->refenable  = 1; // 1;3D ²Î¿¼Ö¡¿ªÆô 0:¹Ø±Õ 
									 // Ê¹ÄÜ²Î¿¼Ö¡ (DRAM²âÊÔÄ£Ê½Ê¹ÓÃREFBUFÖ¸ÏòµÄÊı¾İÎªRAWÊı¾İ)
	}
	else
	{
		p_sys->debugmode  = 0;
		p_sys->testenable = 0; // ¿ªÆôdram²âÊÔÄ£Ê½ 
		p_sys->rawmenable = 0; // 1 ÔÊĞíRAWĞ´³ö
		p_sys->yuvenable  = 1; // 0:¹ØµôÊı¾İÊä³ö  1:´ò¿ª
#if ISP_3D_DENOISE_SUPPORT
		p_sys->refenable  = 1; // 1;3D ²Î¿¼Ö¡¿ªÆô 0:¹Ø±Õ 	
#else
		p_sys->refenable  = 0; // 1;3D ²Î¿¼Ö¡¿ªÆô 0:¹Ø±Õ
#endif
	}

#if ISP_3D_DENOISE_SUPPORT
	XM_printf ("ISP 3D Enable\n");
#else
	XM_printf ("ISP 3D Disable\n");
#endif

/*
#if 1

	p_sys->debugmode  = 0;
	p_sys->testenable = 0; //¿ªÆôdram²âÊÔÄ£Ê½  
	p_sys->rawmenable = 0;
	p_sys->yuvenable  = 1;//0:¹ØµôÊı¾İÊä³ö  1:´ò¿ª
	p_sys->refenable  = 1;//1;
#else // catch raw
	p_sys->debugmode  = 0;
	p_sys->testenable = 0; //¿ªÆôdram²âÊÔÄ£Ê½  
	p_sys->rawmenable = 0;
	p_sys->yuvenable  = 1;//0:¹ØµôÊı¾İÊä³ö 
	p_sys->refenable  = 1;
#endif
	*/
	p_sys->yuvformat = isp_get_video_format ();  //0Ã¯Â¼Å¡y_uv420 1:y_uv422 2:yuv420 3:yuv422
   //XM_printf("0Ã¯Â¼Å¡y_uv420 1:y_uv422 2:yuv420 3:yuv422 \n");
	//p_sys->dmalock = 2;   //×ÜÏßËøÊ¹ÄÜ 2:Ê¹ÄÜ  ÆäËûÊıÖµÎª¹Ø±Õ
   p_sys->dmalock = 0;    	//	×ÜÏßËøÊ¹ÄÜ 2:Ê¹ÄÜ  ÆäËûÊıÖµÎª¹Ø±Õ
	//		×ÜÏßËø½ûÖ¹»á¼õÉÙH264µÄ±àÂëÊ±¼ä
	p_sys->hstride = p_isp->image_stride; //Í¼Ïñ¿ç¶È 16×Ö½Ú±¶Êı
	p_sys->refaddr = p_isp->ref_addr;    //²Î¿¼Ö¡µØÖ·
	p_sys->rawaddr0 = p_isp->raw_addr[0];
	p_sys->rawaddr1 = p_isp->raw_addr[1];
	p_sys->rawaddr2 = p_isp->raw_addr[2];
	p_sys->rawaddr3 = p_isp->raw_addr[3];
	p_sys->yaddr0 = p_isp->y_addr[0];
	p_sys->uaddr0 = p_isp->u_addr[0];
	p_sys->vaddr0 = p_isp->v_addr[0];

	p_sys->yaddr1 = p_isp->y_addr[1];
	p_sys->uaddr1 = p_isp->u_addr[1];
	p_sys->vaddr1 = p_isp->v_addr[1];

	p_sys->yaddr2 = p_isp->y_addr[2];
	p_sys->uaddr2 = p_isp->u_addr[2];
	p_sys->vaddr2 = p_isp->v_addr[2];

	p_sys->yaddr3 = p_isp->y_addr[3];
	p_sys->uaddr3 = p_isp->u_addr[3];
	p_sys->vaddr3 = p_isp->v_addr[3];

}

// ISPÔËĞĞÉèÖÃ
extern cmos_exposure_t isp_exposure;


typedef struct _isp_awb_polyline_tbl {
	int  inttime;

	int  black;
	int  jitter;
} isp_awb_polyline_tbl;

static isp_awb_polyline_tbl awb_polyline_tbl[] = {
	{     1,	  16,  	10		},
	{		5,		8,		13		},
	{	 800,		8,		13		},
	{  CMOS_STD_INTTIME,   16,		13		}
};

static void awb_match_inttime (int inttime, isp_awb_polyline_tbl *awb_tbl)
{
	int i;
	int val;
	isp_awb_polyline_tbl *lo, *hi;
	int count = sizeof(awb_polyline_tbl)/sizeof(awb_polyline_tbl[0]);
	for (i = 0; i < count; i ++)
	{
		if(inttime <= awb_polyline_tbl[i].inttime)
			break;
	}

	//Æ¥Åä
	if(inttime == awb_polyline_tbl[i].inttime)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[i], sizeof(isp_awb_polyline_tbl));
		return;
	}

	// ±ß½ç
	else if(inttime < awb_polyline_tbl[0].inttime)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[0], sizeof(isp_awb_polyline_tbl));
		return;
	}
	else if(i == count)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[count - 1], sizeof(isp_awb_polyline_tbl));
		return;
	}

	lo = &awb_polyline_tbl[i - 1];
	hi = &awb_polyline_tbl[i];
	val = (lo->black + (hi->black - lo->black) * (inttime - lo->inttime) / (hi->inttime - lo->inttime));
	if(val < 4)
		val = 4;
	else if(val > 48)
		val = 48;
	awb_tbl->black = val;

	val = (lo->jitter + (hi->jitter - lo->jitter) * (inttime - lo->inttime) / (hi->inttime - lo->inttime));
	if(val < 4)
		val = 4;
	else if(val > 48)
		val = 48;
	awb_tbl->jitter = val;
}

static void gc2053_cmos_isp_awb_run (isp_awb_ptr_t p_awb)
{

}

#if IQ_20190522
static const isp_demosaic_polyline_tbl demosaic_polyline_tbl[] = {
	//  gain    		demk

	{	  1,           90   },
	{	  8,           104  },
	{	  32,          132  },
	{	  64,          240  },
	{	  100,         240 },
	{	  600,         212},
	{	  1*CMOS_STD_INTTIME,	  	148 },		// Å“Ã¶Â¿ÂªÃ†Ã´Ã„Â£Ã„Ã¢Ã”Ã¶Ã’Ã¦ÃŠÂ±, Å½Ã‹ÃŠÂ±ÃŠÂ¹Ã„ÃœÅ“ÃÅ½Ã³ÂµÃ„demkÃ–Âµ, Â±Â£Â³Ã–Å“ÃÂºÃƒÂµÃ„Å“Ã¢ÃÃ¶Â¶Ãˆ, ÃÂ¬ÃŠÂ±Ã”Ã«Ã‰Ã¹Å“ÃÂµÃ
	{	  8*CMOS_STD_INTTIME,	   48	},		// Å“Ã¶Â¿ÂªÃ†Ã´Ã„Â£Ã„Ã¢Ã”Ã¶Ã’Ã¦ÃŠÂ±, Å½Ã‹ÃŠÂ±ÃŠÂ¹Ã„ÃœÅ“ÃÅ½Ã³ÂµÃ„demkÃ–Âµ, Â±Â£Â³Ã–Å“ÃÂºÃƒÂµÃ„Å“Ã¢ÃÃ¶Â¶Ãˆ, ÃÂ¬ÃŠÂ±Ã”Ã«Ã‰Ã¹Å“ÃÂµÃ
	{    20*CMOS_STD_INTTIME,     16   },		// Â²ÃÂ¿Å’ f:\Ã‚Â·Â²Ã¢ÃŠÃ“Ã†Âµ\20170114ÃÃ­Ã‰Ã\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
								//		Å’Ã¦Â¹Ã‹Å“Ã¢ÃÃ¶Â¶ÃˆÃ“Ã«Ã”Ã«Ã‰Ã¹
	{    35*CMOS_STD_INTTIME,     6    },		// Å“ÃÅ½Ã³ÃŠÃ½Ã—Ã–Ã”Ã¶Ã’Ã¦
	{    64*CMOS_STD_INTTIME,     3    },		// Å“ÃÅ½Ã³ÃŠÃ½Ã—Ã–Ã”Ã¶Ã’Ã¦

};
#else
// 20190626 ½µµÍÀ­ÉìÇ¿¶È£¬½µµÍÔëÉù
static const isp_demosaic_polyline_tbl demosaic_polyline_tbl[] = {
	//  gain    		demk

	{	  1,           90-16  },
	{	  8,           104-16  },
	{	  32,          132-16  },
	{	  64,          240-32  },
	{	  100,         240-32 },
	{	  600,         212-24},
	{	  1*CMOS_STD_INTTIME,	  	148-16 },		// ½ö¿ªÆôÄ£ÄâÔöÒæÊ±, ´ËÊ±Ê¹ÄÜ½Ï´óµÄdemkÖµ, ±£³Ö½ÏºÃµÄ½âÎö¶È, Í¬Ê±ÔëÉù½ÏµÍ
	{	  8*CMOS_STD_INTTIME,	   48-12	},		// ½ö¿ªÆôÄ£ÄâÔöÒæÊ±, ´ËÊ±Ê¹ÄÜ½Ï´óµÄdemkÖµ, ±£³Ö½ÏºÃµÄ½âÎö¶È, Í¬Ê±ÔëÉù½ÏµÍ
	{    20*CMOS_STD_INTTIME,     16-4   },		// ²Î¿¼ f:\Â·²âÊÓÆµ\20170114ÍíÉÏ\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
								//		¼æ¹Ë½âÎö¶ÈÓëÔëÉù
	{    35*CMOS_STD_INTTIME,     6-3    },		// ½Ï´óÊı×ÖÔöÒæ
	{    64*CMOS_STD_INTTIME,     3-1    },		// ½Ï´óÊı×ÖÔöÒæ

};
#endif

static const isp_demosaic_polyline_tbl *gc2053_cmos_isp_get_demosaic_table (int *tbl_count)
{
	*tbl_count = sizeof(demosaic_polyline_tbl)/sizeof(demosaic_polyline_tbl[0]);
	return demosaic_polyline_tbl;
}


#if IQ_20190522
static const isp_sharp_polyline_tbl gc2053_sharp_polyline_tbl[] = {
	//  inttime          	 			strength    gainmax
	{   3,               	 			 64,        200   	},
	{   CMOS_STD_INTTIME,             64,        200		},
	{   CMOS_STD_INTTIME*8,           40,        96      },
	{   CMOS_STD_INTTIME*32,          24,        64      }
};
#elif SENSOR_GC2053
	// 20191022 Ôö¼Ó°×ÌìµÄÈñ¶È
static const isp_sharp_polyline_tbl gc2053_sharp_polyline_tbl[] = {
	//  inttime          	 			strength    gainmax
	{   3,                				 64,        210	   },
	{   CMOS_STD_INTTIME,             56,        180		},
	{   CMOS_STD_INTTIME*8,           24,        64      },
	{   CMOS_STD_INTTIME*32,          10,        24      },
	{   CMOS_STD_INTTIME*48,          6,        16      }
};

#else
// 20190626 Å“ÂµÂµÃÂµÃÃ•Ã•Â¶ÃˆÃÃ‚ÂµÃ„ÃˆÃ±Â¶ÃˆÂ£Â¬Å“ÂµÂµÃÃ”Ã«Ã‰Ã¹
static const isp_sharp_polyline_tbl gc2053_sharp_polyline_tbl[] = {
	//  inttime          	 			strength    gainmax
	{   3,                				 64,        144	   },
	{   CMOS_STD_INTTIME,             56,        144		},
	{   CMOS_STD_INTTIME*8,           24,        64      },
	{   CMOS_STD_INTTIME*32,          10,        24      },
	{   CMOS_STD_INTTIME*48,          6,        16      }
};
#endif

static const isp_sharp_polyline_tbl *gc2053_cmos_isp_get_sharp_table (int *tbl_count)
{
	*tbl_count = sizeof(gc2053_sharp_polyline_tbl)/sizeof(gc2053_sharp_polyline_tbl[0]);
	return gc2053_sharp_polyline_tbl;
}

#if IQ_20190522
static const isp_denoise_inttime_polyline_tbl denoise_inttime_polyline_tbl[] = {
// inttime_gain y_0  u_0  v_0   y_1  u_1  v_1   y_2   u_2  v_2
	{  
		1,         7,  10,   10,     7,   10,   10,     3,   3,   3,
		//1,         5,  3,   3,     5,   3,   3,     4,   3,   3,
	} ,
	
	// 20170305 Å“Â«2DÂµÃ„Å“ÂµÃ”Ã«Ã‡Â¿Â¶ÃˆÅ’Ãµ1(-1), Â±Â£ÃÃ´Å¾Ã¼Â¶Ã ÂµÃ„ÃÅ¾Å“Ãš
	{  
		12,        7,  10,   10,     7,   10,   10,     3,   3,   3,
		//32,        5,  4,   4,     5,   4,   4,     4,   3,   3,
	} ,	
	{  
		128,        7,  12,   12,     7,   12,   12,     3,   3,   3,
		//128,       4,  4,   4,     4,   4,   4,     4,   3,   3,
	} ,	
	
	// e:\proj\ARKN141\test\ISPÂµÃ·ÃŠÃ”\20170202
	{
		1073,      9,  13,   13,    9,  13,  13,    4,   3,   3,
	},


	{
		CMOS_STD_INTTIME*2,    12,   14,   14,    12,   14,   14,    5,   6,  6,
	},
	{
		//CMOS_STD_INTTIME*4,    10,   10,  10,   10,  10,   10,    8,   8,  8,
		CMOS_STD_INTTIME*4,    14,   18,  18,   14,  18,   18,    8,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   22,  28,  28,    22,  28,  28,   12,  12, 12,
		//CMOS_STD_INTTIME*12,   12,  14,  14,    12,  14,  14,   12,  12, 12,
	} ,
	{
		CMOS_STD_INTTIME*14,   26,  32,  32,    26,  32,  32,   13,  13, 13,
		//CMOS_STD_INTTIME*14,   14,  16,  16,    14,  16,  16,   13,  13, 13,
	} ,
	{
		CMOS_STD_INTTIME*20,   33,  40,  40,    33,  40,  40,   13,  13, 13,
		//CMOS_STD_INTTIME*20,   22,  25,  25,    22,  25,  25,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*40,   52,  60,  60,    52,  60,  60,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*64,   84, 94, 94,      84, 94, 94,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*128,  160, 190, 190,  160, 190, 190,   8,  9, 9,
	},
	{
		CMOS_STD_INTTIME*177,  220, 240, 240,   223, 240, 240,   13,  13, 13,
	},

};
#else
// 20190626 Ã”Ã¶Å’Ã“Å“ÂµÃ”Ã«Ã‡Â¿Â¶ÃˆÂ£Â¬Å“ÂµÂµÃÃ”Ã«Ã‰Ã¹
static const isp_denoise_inttime_polyline_tbl denoise_inttime_polyline_tbl[] = {
// inttime_gain y_0  u_0  v_0   y_1  u_1  v_1   y_2   u_2  v_2
	{  
		1,         8,  11,   11,     8,   11,   11,     3,   3,   3,
		//1,         5,  3,   3,     5,   3,   3,     4,   3,   3,
	} ,
	
	// 20170305 Å“Â«2DÂµÃ„Å“ÂµÃ”Ã«Ã‡Â¿Â¶ÃˆÅ’Ãµ1(-1), Â±Â£ÃÃ´Å¾Ã¼Â¶Ã ÂµÃ„ÃÅ¾Å“Ãš
	{  
		12,        8,  11,   11,     8,   11,   11,     3,   3,   3,
		//32,        5,  4,   4,     5,   4,   4,     4,   3,   3,
	} ,	
	{  
		128,        8,  13,   13,    8,   13,   13,     3,   3,   3,
		//128,       4,  4,   4,     4,   4,   4,     4,   3,   3,
	} ,	
	
	// e:\proj\ARKN141\test\ISPÂµÃ·ÃŠÃ”\20170202
	{
		1073,      10,  15,   15,    10,  15,  15,    4,   3,   3,
	},


	{
		CMOS_STD_INTTIME*2,    13,   15,   15,    13,   15,   15,    5,   6,  6,
	},
	{
		//CMOS_STD_INTTIME*4,    10,   10,  10,   10,  10,   10,    8,   8,  8,
		CMOS_STD_INTTIME*4,    15,   20,  20,   15,  20,   20,    8,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   24,  32,  32,    24,  32,  32,   12,  12, 12,
		//CMOS_STD_INTTIME*12,   12,  14,  14,    12,  14,  14,   12,  12, 12,
	} ,
	{
		CMOS_STD_INTTIME*14,   28,  36,  36,    28,  36,  36,   13,  13, 13,
		//CMOS_STD_INTTIME*14,   14,  16,  16,    14,  16,  16,   13,  13, 13,
	} ,
	{
		CMOS_STD_INTTIME*20,   33,  48,  48,    33,  48,  48,   13,  13, 13,
		//CMOS_STD_INTTIME*20,   22,  25,  25,    22,  25,  25,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*40,   48,  60,  60,    48,  60,  60,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*64,   56, 64, 64,      56, 64, 64,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*80,   80, 128, 128,    80, 128, 128,   8,  9, 9,
	}, 
	{
		CMOS_STD_INTTIME*128,  128, 160, 160,  128, 160, 160,   8,  9, 9,
	},
	{
		CMOS_STD_INTTIME*177,  220, 240, 240,   223, 240, 240,   13,  13, 13,
	},

};
#endif

static const isp_denoise_inttime_polyline_tbl *gc2053_cmos_isp_get_denoise_table (int *tbl_count)
{
	*tbl_count = sizeof(denoise_inttime_polyline_tbl)/sizeof(denoise_inttime_polyline_tbl[0]);
	return (isp_denoise_inttime_polyline_tbl *)denoise_inttime_polyline_tbl;
}

#if IQ_20190522
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 Ã”Ã¶Å’Ã“Â¶ÃŒÃ†Ã˜Â¹Ã¢Â³Â¡Å¸Â°ÂµÃ„Ã€Â­Ã‰Ã¬Ã‡Â¿Â¶Ãˆ, Å¾Ã„Ã‰Ã†Å“Ã¢ÃÃ¶Â¶ÃˆÅ’Â°Ã‘Ã•Ã‰Â«
	{	1,		   96, (int)(0.86 * 512),  (int)(0.45 * 512) },

	// 20190325Â°Ã—ÃŒÃ¬Ã‚Â·Â²Ã¢Â£Â¬Ã‰Â«Â²ÃŠÃ†Â«Â»Ã†Â£Â¬Å“ÂµÂµÃÂ°Ã—ÃŒÃ¬ÂµÃ„Ã‰Â«Â²ÃŠÃ€Â­Ã‰Ã¬Ã‡Â¿Â¶Ãˆ
	// 20190426Â°Ã—ÃŒÃ¬Ã‚Â·Â²Ã¢Â£Â¬ÃÃÂ³Â¡Å¸Â°ÃÃ‚Å“Å¡Ã–Ã¾ÃÃ¯Â¶Â¥Â²Â¿Ã‰Ã”ÃÂ¢Â¹Ã½Ã†Ã˜Â£Â¬Å“ÂµÂµÃÃÃÂ¶ÃˆÃ”Ã¶Ã’Ã¦
	{	5,		   96, (int)(0.91 * 512),  (int)(0.45 * 512) },
	{	32,	   112, (int)(0.92 * 512),  (int)(0.45 * 512) },
	{  80,	   212, (int)(0.93 * 512),  (int)(0.45 * 512) },
	{  400,	   212, (int)(0.94 * 512),  (int)(0.45 * 512) },
	{	700,   	240, (int)(0.92 * 512),  (int)(0.45 * 512) },
	{	820,	   240, (int)(0.91 * 512),  (int)(0.45 * 512) },
	{	900,	   256, (int)(0.90 * 512),  (int)(0.45 * 512) },
	{	1023,	   256,  (int)(0.86 * 512),  (int)(0.40 * 512) },


	// Ã‚Â·Â²Ã¢Ã”Ã«Ã‰Ã¹Ã†Â«Å½Ã³Â£Â¬Å“ÂµÂµÃÃÃÂ¶ÃˆÃ”Ã¶Ã’Ã¦Â£Â¬Ã’Ã–Ã–Ã†Ã”Ã«Ã‰Ã¹
	{	CMOS_STD_INTTIME,	   240,  (int)(0.80 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.75 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.50 * 512),  (int)(0.30 * 512) },	
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.40 * 512),  (int)(0.2 * 512) },
	
	{	CMOS_STD_INTTIME*30,		240,   (int)(0.24 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*60,		320,    (int)(0.15 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	320,    (int)(0.10 * 512),  (int)(0.2 * 512) },
};

#elif SENSOR_GC2053
/****
// 20191021 ÔöÇ¿°×ÌìÑÕÉ«µÄ±¥ºÍ¶È
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 Ôö¼Ó¶ÌÆØ¹â³¡¾°µÄÀ­ÉìÇ¿¶È, ¸ÄÉÆ½âÎö¶È¼°ÑÕÉ«
	{	1,		   96, (int)(0.80 * 512),  (int)(0.60 * 512) },

	// 20190325°×ÌìÂ·²â£¬É«²ÊÆ«»Æ£¬½µµÍ°×ÌìµÄÉ«²ÊÀ­ÉìÇ¿¶È
	// 20190426°×ÌìÂ·²â£¬ÁÁ³¡¾°ÏÂ½¨ÖşÎï¶¥²¿ÉÔÎ¢¹ıÆØ£¬½µµÍÁÁ¶ÈÔöÒæ
	{	5,		   96, (int)(0.85 * 512),  (int)(0.60 * 512) },
	{	32,	   112, (int)(0.86 * 512),  (int)(0.60 * 512) },
	{  80,	   212, (int)(0.87 * 512),  (int)(0.60 * 512) },
	{  400,	   212, (int)(0.88 * 512),  (int)(0.60 * 512) },
	{	700,   	240, (int)(0.86 * 512),  (int)(0.60 * 512) },
	{	820,	   240, (int)(0.85 * 512),  (int)(0.60 * 512) },
	{	900,	   256, (int)(0.80 * 512),  (int)(0.60 * 512) },
	{	1023,	   256,  (int)(0.70 * 512),  (int)(0.55 * 512) },


	// Â·²âÔëÉùÆ«´ó£¬½µµÍÁÁ¶ÈÔöÒæ£¬ÒÖÖÆÔëÉù
	{	CMOS_STD_INTTIME,	   240,  (int)(0.60 * 512),  (int)(0.55 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.50 * 512),  (int)(0.55 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.30 * 512),  (int)(0.45 * 512) },	
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.15 * 512),  (int)(0.35 * 512) },
	
	// ½µµÍµÍÕÕ¶ÈÔöÒæ£¬¼õÇáÔëÉù
	{	CMOS_STD_INTTIME*30,		160,   (int)(0.10 * 512),  (int)(0.25 * 512) },
	{	CMOS_STD_INTTIME*60,		144,    (int)(0.10 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	144,    (int)(0.10 * 512),  (int)(0.2 * 512) },
};
*****/

// 20191022 ÔöÇ¿°×ÌìÑÕÉ«µÄ±¥ºÍ¶È
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 Ôö¼Ó¶ÌÆØ¹â³¡¾°µÄÀ­ÉìÇ¿¶È, ¸ÄÉÆ½âÎö¶È¼°ÑÕÉ«
	{	1,		   96, (int)(0.80 * 512),  (int)(0.80 * 512) },

	// 20190325°×ÌìÂ·²â£¬É«²ÊÆ«»Æ£¬½µµÍ°×ÌìµÄÉ«²ÊÀ­ÉìÇ¿¶È
	// 20190426°×ÌìÂ·²â£¬ÁÁ³¡¾°ÏÂ½¨ÖşÎï¶¥²¿ÉÔÎ¢¹ıÆØ£¬½µµÍÁÁ¶ÈÔöÒæ
	{	5,		   96, (int)(0.85 * 512),  (int)(0.80 * 512) },
	{	32,	   112, (int)(0.86 * 512),  (int)(0.80 * 512) },
	{  80,	   212, (int)(0.87 * 512),  (int)(0.80 * 512) },
	{  400,	   212, (int)(0.88 * 512),  (int)(0.80 * 512) },
	{	700,   	240, (int)(0.86 * 512),  (int)(0.80 * 512) },
	{	820,	   240, (int)(0.85 * 512),  (int)(0.80 * 512) },
	{	900,	   256, (int)(0.80 * 512),  (int)(0.80 * 512) },
	{	1023,	   256,  (int)(0.70 * 512),  (int)(0.75 * 512) },


	// Â·²âÔëÉùÆ«´ó£¬½µµÍÁÁ¶ÈÔöÒæ£¬ÒÖÖÆÔëÉù
	{	CMOS_STD_INTTIME,	   240,  (int)(0.60 * 512),  (int)(0.75 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.50 * 512),  (int)(0.70 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.30 * 512),  (int)(0.60 * 512) },	
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.15 * 512),  (int)(0.45 * 512) },
	
	// ½µµÍµÍÕÕ¶ÈÔöÒæ£¬¼õÇáÔëÉù
	{	CMOS_STD_INTTIME*30,		160,   (int)(0.10 * 512),  (int)(0.25 * 512) },
	{	CMOS_STD_INTTIME*60,		144,    (int)(0.10 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	144,    (int)(0.10 * 512),  (int)(0.2 * 512) },
};

#else
// 20191022 ÔöÇ¿°×ÌìÑÕÉ«µÄ±¥ºÍ¶È
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 Ôö¼Ó¶ÌÆØ¹â³¡¾°µÄÀ­ÉìÇ¿¶È, ¸ÄÉÆ½âÎö¶È¼°ÑÕÉ«
	{	1,		   96, (int)(0.80 * 512),  (int)(0.45 * 512) },

	// 20190325°×ÌìÂ·²â£¬É«²ÊÆ«»Æ£¬½µµÍ°×ÌìµÄÉ«²ÊÀ­ÉìÇ¿¶È
	// 20190426°×ÌìÂ·²â£¬ÁÁ³¡¾°ÏÂ½¨ÖşÎï¶¥²¿ÉÔÎ¢¹ıÆØ£¬½µµÍÁÁ¶ÈÔöÒæ
	{	5,		   96, (int)(0.85 * 512),  (int)(0.45 * 512) },
	{	32,	   112, (int)(0.86 * 512),  (int)(0.45 * 512) },
	{  80,	   212, (int)(0.87 * 512),  (int)(0.45 * 512) },
	{  400,	   212, (int)(0.88 * 512),  (int)(0.45 * 512) },
	{	700,   	240, (int)(0.86 * 512),  (int)(0.45 * 512) },
	{	820,	   240, (int)(0.85 * 512),  (int)(0.45 * 512) },
	{	900,	   256, (int)(0.80 * 512),  (int)(0.45 * 512) },
	{	1023,	   256,  (int)(0.70 * 512),  (int)(0.40 * 512) },


	// Â·²âÔëÉùÆ«´ó£¬½µµÍÁÁ¶ÈÔöÒæ£¬ÒÖÖÆÔëÉù
	{	CMOS_STD_INTTIME,	   240,  (int)(0.60 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.50 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.30 * 512),  (int)(0.30 * 512) },	
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.15 * 512),  (int)(0.2 * 512) },
	
	// ½µµÍµÍÕÕ¶ÈÔöÒæ£¬¼õÇáÔëÉù
	{	CMOS_STD_INTTIME*30,		160,   (int)(0.10 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*60,		144,    (int)(0.10 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	144,    (int)(0.10 * 512),  (int)(0.2 * 512) },
};

#endif

static const isp_eris_polyline_tbl* gc2053_cmos_isp_get_eris_auto_table(int *tbl_count)
{
	*tbl_count = sizeof(eris_polyline_tbl)/sizeof(eris_polyline_tbl[0]);
	return eris_polyline_tbl;
}




static const isp_eris_man_polyline_tbl eris_man_polyline_tbl[] = {

	{	1,		   					112,	160,	(int)(0.95 * 512),  (int)(0.75 * 512) },
	{	5,		   					144,	160,	(int)(0.96 * 512),  (int)(0.75 * 512) },
	{	32,	   					196,	160,	(int)(0.97 * 512),  (int)(0.75 * 512) },

	{  80,	   					240,	144,	(int)(0.99 * 512),  (int)(0.8 * 512) },
	{	700,   						240,	80,	(int)(0.99 * 512),  (int)(0.9 * 512) },
	{	1023,	   					320,	64,	(int)(0.98 * 512),  (int)(0.9 * 512) },

	// 20170217 µØÏÂ³µ¿â²âÊÔ·¢ÏÖ, N141Ç½±ÚÉÏÖ¸Òı±êÖ¾µÄÉ«²ÊÆ«°µ, ÉÔÎ¢Ìá¸ß°µ³¡¾°ÏÂÉ«²ÊµÄ±ÈÂÊÒò×Ó(+0.1)
	{	CMOS_STD_INTTIME,	   	320,  48,	(int)(0.98 * 512),  (int)(0.8 * 512) },
	{	CMOS_STD_INTTIME*4,		320,  32,	(int)(0.98 * 512),  (int)(0.6 * 512) },

	{	CMOS_STD_INTTIME*10,		440,  32,	(int)(0.95 * 512),  (int)(0.6 * 512) },

	{	CMOS_STD_INTTIME*12,		440,  32,	 (int)(0.85 * 512),  (int)(0.5 * 512) },
	{	CMOS_STD_INTTIME*15,		440,   32,	(int)(0.80 * 512),  (int)(0.5 * 512) },

	{	CMOS_STD_INTTIME*35,		440,   32,	(int)(0.60 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*60,		440,   32,	(int)(0.50 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*128,	440,   32,	(int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*177,	440,   32,	(int)(0.20 * 512),  (int)(0.2 * 512) },
};

static const isp_eris_man_polyline_tbl *gc2053_cmos_isp_get_eris_man_table(int *tbl_count)
{
	*tbl_count = sizeof(eris_man_polyline_tbl)/sizeof(eris_man_polyline_tbl[0]);
	return eris_man_polyline_tbl;
}

// 3D¹Ø±ÕÊ±µÄ²ÎÊı, 
static const isp_crosstalk_polyline_tbl crosstalk_polyline_tbl[] = {
	//  inttime_gain    	crosstalk
	// 20170217ÉÏÎç¼°Ö®Ç°Ê¹ÓÃµÄ°æ±¾
	{		50,				2 + 2	 },
	{		100,				3 + 2	 },
	{		161,				3 + 2	 },
	{		242,				3 + 2	 },
	{		1024,		  		3 + 3	 },

	// 20190321 ÍíÉÏÂ·²â£¬ÔëÉùÆ«´ó£¬Ôö¼Ó½µÔëÇ¿¶È
	{		CMOS_STD_INTTIME,		  		5 + 4	 },
	{		CMOS_STD_INTTIME * 2,	  	8 + 4  },
	{		CMOS_STD_INTTIME * 4,	  	10 + 7  },
	{		CMOS_STD_INTTIME * 10,	  	12 + 12  },
	{  	CMOS_STD_INTTIME * 30,     24 + 18  },
	{  	CMOS_STD_INTTIME * 64,     48 + 23  },
	{  	CMOS_STD_INTTIME * 128,    64 + 33  },
	{  	CMOS_STD_INTTIME * 177,    128  },
};

static const isp_crosstalk_polyline_tbl *gc2053_cmos_isp_get_crosstalk_table (int *tbl_count)
{
	*tbl_count = sizeof(crosstalk_polyline_tbl)/sizeof(crosstalk_polyline_tbl[0]);
	return crosstalk_polyline_tbl;
}



#pragma data_alignment=32

#if IQ_20190522
static const isp_enhance_polyline_tbl enhance_polyline_tbl[] = {
	//  inttime   bright  contrast
	//{		3,		   -6,	1024   },
	//{		64,	   -8,	1024 	},
	// ±£Áô¶ÌÆØ¹âµÍ¹â´¦µÄÏ¸½Ú
	{		3,		   -2,	1024	   },
	{		64,	   -2,	1024	 	},
	
	{   512,       -4,	1024    },
	{   800,       -4,	1024    },
	{	 1125,		-4,	1024	},		// ³¡¾°¹âÕÕ½ÏÈõÊ±, ÔëÉùÔö¼Ó, ÏàÓ¦Ôö¼ÓºÚµçÆ½
	{	 CMOS_STD_INTTIME * 2,	  	-4,	1024},
	// 20190321 Ò¹ÍíÂ·²â£¬ÔëÉùÆ«´ó£¬Ôö´óºÚµçÆ½
	{	 CMOS_STD_INTTIME * 16,	  	-5,	1024},
	
};
#else
// 20190626 Ôö´óµÍÕÕ¶È³¡¾°ÏÂµÄºÚµçÆ½
static const isp_enhance_polyline_tbl enhance_polyline_tbl[] = {
	//  inttime   bright  contrast
	//{		3,		   -6,	1024   },
	//{		64,	   -8,	1024 	},
	// ±£Áô¶ÌÆØ¹âµÍ¹â´¦µÄÏ¸½Ú
	{		3,		   -2,	1024	   },
	{		64,	   -2,	1024	 	},
	
	{   512,       -4,	1024    },
	{   800,       -4,	1024    },
	{	 1125,		-4,	1024	},		// ³¡¾°¹âÕÕ½ÏÈõÊ±, ÔëÉùÔö¼Ó, ÏàÓ¦Ôö¼ÓºÚµçÆ½
	{	 CMOS_STD_INTTIME * 2,	  	-4,	1024},
	// 20190321 Ò¹ÍíÂ·²â£¬ÔëÉùÆ«´ó£¬Ôö´óºÚµçÆ½
	{	 CMOS_STD_INTTIME * 16,	  	-5,	1024},
	{	 CMOS_STD_INTTIME * 32,	  	-7,	1024},
	{	 CMOS_STD_INTTIME * 64,	  	-9,	1024},
	
};
#endif

static const isp_enhance_polyline_tbl * gc2053_cmos_isp_get_enhance_table (int *tbl_count)
{
	*tbl_count = sizeof(enhance_polyline_tbl)/sizeof(enhance_polyline_tbl[0]);
	return enhance_polyline_tbl;
}

#if 1
static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		  900	},
	{		10 * CMOS_STD_INTTIME,	  800	},
	{  	12 * CMOS_STD_INTTIME,     720   },
	{  	24 * CMOS_STD_INTTIME,     512   },
	{  	32 * CMOS_STD_INTTIME,     384    },
	{  	64 * CMOS_STD_INTTIME,     384    },
};
#else
// 20190626 Ã”Ã¶Å’Ã“Ã‰Â«Â²ÃŠÂ±Â¥ÂºÃÂ¶Ãˆ
static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		1000	},
	{		10 * CMOS_STD_INTTIME,	   900	},
	{  	12 * CMOS_STD_INTTIME,     800   },
	{  	24 * CMOS_STD_INTTIME,     640   },
	{  	32 * CMOS_STD_INTTIME,     512    },
	{  	64 * CMOS_STD_INTTIME,     512    },
};
#endif

static const isp_satuation_polyline_tbl * gc2053_cmos_isp_get_satuation_table (int *tbl_count)
{
	*tbl_count = sizeof(satuation_polyline_tbl)/sizeof(satuation_polyline_tbl[0]);
	return satuation_polyline_tbl;
}

#define	AE_GAIN(x)	((unsigned int)(x*1.0))

// 20190422°×Ìì²âÊÔ£¬ÓëÔ­³§±È½Ï£¬ÆØ¹â¹ıÆØ
static const isp_ae_polyline_tbl ae_polyline_tbl_20190422[] = {
	// 1) Ôö¼Ó¶ÌÆØ¹â³¡¾°ÏÂ(°üÀ¨Äæ¹â)µÄÁÁ¶È
	{	1,								AE_GAIN(64),	128,	{1, 1, 1, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(60),	128,	{1, 1, 1, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(56),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(56),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	512,							AE_GAIN(53),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(47),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},

	// 2 Ôö¼Óº£°¶³ÇÍíÉÏÂ·±ßÁ½²àµÄÁÁ¶È
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(39),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(33),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*4,		AE_GAIN(24),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*8,		AE_GAIN(19),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
};

static const isp_ae_polyline_tbl ae_polyline_tbl_new[] = {
	// 1) Ôö¼Ó¶ÌÆØ¹â³¡¾°ÏÂ(°üÀ¨Äæ¹â)µÄÁÁ¶È
	{	1,								AE_GAIN(58),	128,	{1, 1, 1, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(54),	128,	{1, 1, 1, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(50),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(50),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	512,							AE_GAIN(48),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(48),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},

	// 2 Ôö¼Óº£°¶³ÇÍíÉÏÂ·±ßÁ½²àµÄÁÁ¶È
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(48),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(42),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*4,		AE_GAIN(32),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
};

static const isp_ae_polyline_tbl ae_polyline_tbl_20170322_pm[] = {
	// 1) Ôö¼Ó¶ÌÆØ¹â³¡¾°ÏÂ(°üÀ¨Äæ¹â)µÄÁÁ¶È
	{	1,								AE_GAIN(36),	128,	{1, 1, 1, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(36),	128,	{1, 1, 1, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(36),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(40),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	512,							AE_GAIN(40),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(40),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},

	// 2 Ôö¼Óº£°¶³ÇÍíÉÏÂ·±ßÁ½²àµÄÁÁ¶È
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(39),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(33),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*4,		AE_GAIN(24),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*8,		AE_GAIN(19),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
};

/****
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) Ã¥Â¢Å¾Ã¥Å Â Ã§Å¸Â­Ã¦â€ºÂÃ¥â€¦â€°Ã¥Å“ÂºÃ¦â„¢Â¯Ã¤Â¸â€¹(Ã¥Å’â€¦Ã¦â€¹Â¬Ã©â‚¬â€ Ã¥â€¦â€°)Ã§Å¡â€Ã¤ÂºÂ®Ã¥ÂºÂ¦
	{	1,								AE_GAIN(32),	128,	{1, 2, 1, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(32),	128,	{1, 2, 1, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(32),	128,	{2, 4, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(32),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	512,							AE_GAIN(33),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(35),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},

	// 2 Ã¥Â¢Å¾Ã¥Å Â Ã¦ÂµÂ·Ã¥Â²Â¸Ã¥Å¸Å½Ã¦â„¢Å¡Ã¤Â¸Å Ã¨Â·Â¯Ã¨Â¾Â¹Ã¤Â¸Â¤Ã¤Â¾Â§Ã§Å¡â€Ã¤ÂºÂ®Ã¥ÂºÂ¦
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(35),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(33),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*4,		AE_GAIN(24),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*8,		AE_GAIN(19),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
};
****/
/****
// 20191021 ¸ÄÉÆ°×Ìì/ÍíÉÏ¹ıÆØµÄÎÊÌâ
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) Ôö¼Ó¶ÌÆØ¹â³¡¾°ÏÂ(°üÀ¨Äæ¹â)µÄÁÁ¶È
	{	1,								AE_GAIN(24),	128,	{2, 3, 2, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(24),	128,	{2, 3, 2, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(24),	128,	{3, 5, 3, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(24),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},	
	{	512,							AE_GAIN(24),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(24),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	
	// 2 Ôö¼Óº£°¶³ÇÍíÉÏÂ·±ßÁ½²àµÄÁÁ¶È
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME,			AE_GAIN(24),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*4,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(16),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};
*****/
// 20191030
// ²âÊÔ·¢ÏÖÒõÌì°×ÌìÁÁ¶ÈÉÔÎ¢Æ«°µ, Ôö¼Ó³¡¾°ÁÁ¶È
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) Ôö¼Ó¶ÌÆØ¹â³¡¾°ÏÂ(°üÀ¨Äæ¹â)µÄÁÁ¶È
	{	1,								AE_GAIN(32),	128,	{2, 3, 2, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(32),	128,	{2, 3, 2, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(32),	128,	{3, 5, 3, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(32),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},	
	{	512,							AE_GAIN(32),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(29),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	
	// 2 Ôö¼Óº£°¶³ÇÍíÉÏÂ·±ßÁ½²àµÄÁÁ¶È
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME,			AE_GAIN(25),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*4,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(16),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};

static const isp_ae_polyline_tbl *gc2053_cmos_isp_get_ae_table (int *tbl_count)
{
	*tbl_count = sizeof(ae_polyline_tbl)/sizeof(ae_polyline_tbl[0]);
	return ae_polyline_tbl;
}


static void gc2053_cmos_isp_set_day_night_mode (cmos_gain_ptr_t gain, int day_night)	// day_night = 1, Ò¹ÍíÔöÇ¿Ä£Ê½, day_night = 0, ÆÕÍ¨Ä£Ê½
{
}

u32_t isp_init_cmos_sensor (cmos_sensor_t *cmos_sensor)
{
	memset (cmos_sensor, 0, sizeof(cmos_sensor_t));

	cmos_sensor->cmos_gain_initialize = cmos_gain_initialize;
	cmos_sensor->cmos_max_gain_set = gc2053_cmos_max_gain_set;
	cmos_sensor->cmos_max_gain_get = gc2053_cmos_max_gain_get;

	//cmos_sensor->cmos_gain_update = cmos_gain_update;
	cmos_sensor->cmos_inttime_initialize = cmos_inttime_initialize;
	//cmos_sensor->cmos_inttime_update = cmos_inttime_update;
	cmos_sensor->cmos_inttime_gain_update = cmos_inttime_gain_update;
	cmos_sensor->cmos_inttime_gain_update_manual = cmos_inttime_gain_update_manual;
	cmos_sensor->analog_gain_from_exposure_calculate = analog_gain_from_exposure_calculate;
	cmos_sensor->digital_gain_from_exposure_calculate = digital_gain_from_exposure_calculate;
	cmos_sensor->cmos_get_iso = cmos_get_iso;
	cmos_sensor->cmos_fps_set = cmos_fps_set;
	cmos_sensor->cmos_sensor_set_readout_direction = cmos_sensor_set_readout_direction;

	cmos_sensor->cmos_sensor_get_sensor_name = gc2053_cmos_sensor_get_sensor_name;
	// sensor³õÊ¼»¯
	cmos_sensor->cmos_isp_sensor_init = gc2053_isp_sensor_init;

	cmos_sensor->cmos_isp_awb_init = gc2053_cmos_isp_awb_init;
	cmos_sensor->cmos_isp_colors_init = gc2053_cmos_isp_colors_init;
	cmos_sensor->cmos_isp_denoise_init = gc2053_cmos_isp_denoise_init;
	cmos_sensor->cmos_isp_eris_init = gc2053_cmos_isp_eris_init;
	cmos_sensor->cmos_isp_fesp_init = gc2053_cmos_isp_fesp_init;
	cmos_sensor->cmos_isp_enhance_init = gc2053_cmos_isp_enhance_init;
	cmos_sensor->cmos_isp_ae_init = gc2053_cmos_isp_ae_init;
	cmos_sensor->cmos_isp_sys_init = gc2053_cmos_isp_sys_init;

	cmos_sensor->cmos_isp_awb_run = NULL;
	cmos_sensor->cmos_isp_colors_run = NULL;
	cmos_sensor->cmos_isp_denoise_run = NULL;
	cmos_sensor->cmos_isp_eris_run = NULL;
	cmos_sensor->cmos_isp_fesp_run = NULL;
	cmos_sensor->cmos_isp_enhance_run = NULL;
	cmos_sensor->cmos_isp_ae_run = NULL;
	cmos_sensor->cmos_isp_sharp_run = NULL;

	cmos_sensor->cmos_isp_get_gamma_table = gc2053_cmos_isp_get_gamma_table;
	cmos_sensor->cmos_isp_get_denoise_table = gc2053_cmos_isp_get_denoise_table;
	cmos_sensor->cmos_isp_get_eris_auto_table = gc2053_cmos_isp_get_eris_auto_table;	// ×Ô¶¯Ä£Ê½
	cmos_sensor->cmos_isp_get_eris_man_table = gc2053_cmos_isp_get_eris_man_table;	// ÊÖ¶¯Ä£Ê½
	cmos_sensor->cmos_isp_get_crosstalk_table = gc2053_cmos_isp_get_crosstalk_table;
	cmos_sensor->cmos_isp_get_fpn_table = NULL;
	cmos_sensor->cmos_isp_get_lsc_table = NULL;
	cmos_sensor->cmos_isp_get_satuation_table = gc2053_cmos_isp_get_satuation_table;
	cmos_sensor->cmos_isp_get_enhance_table = gc2053_cmos_isp_get_enhance_table;
	cmos_sensor->cmos_isp_get_sharp_table = gc2053_cmos_isp_get_sharp_table;
	cmos_sensor->cmos_isp_get_ae_table = gc2053_cmos_isp_get_ae_table;
	cmos_sensor->cmos_isp_get_demosaic_table = gc2053_cmos_isp_get_demosaic_table;

	return 0;
}
EXPORT_SYMBOL(isp_init_cmos_sensor);

static int
gc2053_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    i2c_client = client;

	of_property_read_u32(client->dev.of_node, "sensor-fps", &isp_sensor_fps);
	
	return 0;
}

static int
gc2053_remove(struct i2c_client *client)
{
	return 0;
}

static const struct of_device_id gc2053_of_match[] = {
	{ .compatible = "arkmicro,gc2053" },
	{ }
};
MODULE_DEVICE_TABLE(of, gc2053_of_match);

static const struct i2c_device_id gc2053_id[] = {
	{ "gc2053", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, gc2053_id);

static struct i2c_driver gc2053_driver = {
	.driver = {
		.name = "gc2053",
		.of_match_table = of_match_ptr(gc2053_of_match),
	},
	.probe = gc2053_probe,
	.remove = gc2053_remove,
    .id_table = gc2053_id,
};

module_i2c_driver(gc2053_driver);

MODULE_DESCRIPTION("Sensor gc2053 driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
