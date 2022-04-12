#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include "ark_camera.h"
#include "ark_isp_exposure_cmos.h"
#include "Gem_isp_io.h"


//#define	SEN_I2C_ADDR				(0x34 >> 1)
#define	FPS_25	0

// XHS行同步信号低电平宽度, 单位为DCK
#define	XHS_low_level_width_6_clk		0x0
#define	XHS_low_level_width_12_clk		0x1
#define	XHS_low_level_width_22_clk		0x2
#define	XHS_low_level_width_128_clk		0x3

// XVS列同步信号低电平宽度, 单位为行同步信号个数
#define	XVS_low_level_width_1_line		0x0
#define	XVS_low_level_width_2_line		0x1
#define	XVS_low_level_width_4_line		0x2
#define	XVS_low_level_width_8_line		0x3

#define	VERTICAL_SCANNING_DIRECTION_NORMAL		0
#define	VERTICAL_SCANNING_DIRECTION_INVERTED	1
#define	VERTICAL_SCANNING_DIRECTION	VERTICAL_SCANNING_DIRECTION_NORMAL


enum {
	IMX322_MODE_HD720P_10BIT_30FPS = 0,
	IMX322_MODE_HD720P_10BIT_60FPS,
	IMX322_MODE_HD720P_12BIT_30FPS,
	IMX322_MODE_HD1080P_12BIT_30FPS,
	IMX322_MODE_HD1080P_10BIT_30FPS
};


static unsigned int FRM_LENGTH = 0x465 + 0x20;		// 0x465 --> 1125
static struct i2c_client *i2c_client = NULL;


static unsigned int I2C_READ (unsigned int addr)
{
#if 1
	struct i2c_msg msgs[2];
	int retries = 0;
	int ret = -1;
	char buf[2];

	buf[0] = (addr>>8)&0xFF;
	buf[1] = addr&0xFF;
	
	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = i2c_client->addr;
	msgs[0].len   = 2;
	msgs[0].buf   = buf;
	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = i2c_client->addr;
	msgs[1].len   = 1;
	msgs[1].buf   = buf;
	
	while(retries < 5)
	{
		ret = i2c_transfer(i2c_client->adapter, msgs, 2);
		if(ret == 2)
			break;
		retries++;
	}
	
	if (retries >= 5)
	{
		printk(KERN_ERR "%s timeout\n", __FUNCTION__);
		return 0;
	}
	//printk(KERN_ALERT "### read addr:0x%x,val:0x%x\n",addr,buf[0]);

	return buf[0];
#else
	return i2c_smbus_read_byte_data(i2c_client, addr);
#endif
}

static int I2C_WRITE (unsigned int addr, unsigned char data)
{
#if 1
	struct i2c_msg msg;
	int ret = -1;
	u8 retries = 0;
	u8 buf[3];

	buf[0] = ((addr>>8)&0xFF);
	buf[1] = (addr&0xFF);
	buf[2] = (data&0xFF);

	msg.flags = !I2C_M_RD;
	msg.addr  = i2c_client->addr;
	msg.len   = sizeof(buf);
	msg.buf   = buf;
	
	while(retries < 5)
	{
		ret = i2c_transfer(i2c_client->adapter, &msg, 1);
		if (ret == 1)
			break;
		retries++;
	}
		
	if (retries >= 5)
	{
		printk(KERN_ERR "%s timeout\n", __FUNCTION__);
		return -1;
	}
	//printk(KERN_ALERT "### write addr:0x%x,val:0x%x\n",addr,data);

	return ret;
#else
	return i2c_smbus_write_byte_data(i2c_client, addr, data);
#endif
}

int imx322_i2c_read (u32_t addr)
{
    return I2C_READ(addr);
}
EXPORT_SYMBOL(imx322_i2c_read);

int imx322_i2c_write(u32_t addr, u8_t data)
{
	return I2C_WRITE(addr, data);
}
EXPORT_SYMBOL(imx322_i2c_write);

#if 0
// FRM_LENGTH配置成0x465会导致ISP无法在一帧间隔内完成处理及数据DDR写入, 因此会出现只有帧同步中断, 无帧完成中断,
// 但是帧数据一直在更新的情况. (这种情况下的帧应该丢弃)
// 应增加消隐行的个数来满足ISP整个流程所需的时序.
int imx322_get_sensor_frame_length (void)
{
	return FRM_LENGTH;
}

// 设置增益, 最大42dB (Analog + Digital Gain)
int imx322_adjust_gain (float gain)
{
	unsigned int gain_setting;
	// 最大42dB (Analog + Digital Gain)
	if(gain > 42.0)
		gain = 42.0;
	gain_setting = (unsigned int)(gain / 0.3 + 0.5);
	I2C_WRITE (0x301E, (unsigned char)gain_setting);
	return 0;
}

// The sensor's integration time is obtained by the following formula.
// Integration time = 1 frame period × (SVS + 1 - SPL) - (INTEG_TIME) × (1H period) - 0.3 [H] (However, SVS > SPL)
int imx322_adjust_integration_time (unsigned int Integration_time_line_count)
{
	// 计算快门开启的时间
	unsigned int shutter_sweep_line_count = 1124 - (Integration_time_line_count>>8);
	I2C_WRITE (0x0202, (unsigned char)(shutter_sweep_line_count >> 8));			// INTEG_TIME [15:8]
	I2C_WRITE (0x0203, (unsigned char)(shutter_sweep_line_count&0xff));			// INTEG_TIME [7:0]
	return 0;
}

unsigned  short imx322_get_integration_time ( void )
{
   ////           INTEG_TIME [15:8]  INTEG_TIME [7:0]
	return (FRM_LENGTH-(I2C_READ (0x0202)<<8)-I2C_READ (0x0203) );
}

// 超长时间曝光操作设置(曝光时间超过一帧)
// Integration time = 1 frame period × (SVS + 1 - SPL) - (INTEG_TIME) × (1H period) - 0.3 [H] (However, SVS > SPL)
// (Controlling the Integration Time in Frame Units)
// integration_time_frame_count  曝光总时间(以帧为单位)
// Integration_time_line_count   曝光积分时间(以行为单位)
int imx322_set_long_exposure (unsigned int integration_time_frame_count, 
										unsigned int Integration_time_line_count
										)
{
	unsigned int SVS, SPL;
	unsigned int INTEG_TIME;
	SVS = integration_time_frame_count - 1;
	if(integration_time_frame_count <= 1)
	{
		XM_printf ("illegal integration_time_frame_count(%d)\n", integration_time_frame_count);
		return -1;
	}
	if( integration_time_frame_count * FRM_LENGTH <= Integration_time_line_count )
	{
		XM_printf ("illegal long_exposure setting(%d, %d) \n", integration_time_frame_count, Integration_time_line_count);
		return -1;
	}

	SPL = SVS - (Integration_time_line_count / FRM_LENGTH); 
	INTEG_TIME = FRM_LENGTH - Integration_time_line_count % FRM_LENGTH;
	if(SVS > 1 && INTEG_TIME >= (FRM_LENGTH - 7))
		INTEG_TIME = FRM_LENGTH - 8;
	
	I2C_WRITE (0x0202, (unsigned char)(INTEG_TIME >> 8));
	I2C_WRITE (0x0203, (unsigned char)(INTEG_TIME));
	I2C_WRITE (0x300F, (unsigned char)SVS);
	I2C_WRITE (0x3010, (unsigned char)(SVS >> 8));
	I2C_WRITE (0x300D, (unsigned char)SPL);
	I2C_WRITE (0x300E, (unsigned char)(SPL >> 8));

	return 0;
}
#endif

// 1080P模式 12bit
int imx322_init_12bit  (unsigned int frame_lines)
{
	int ret;
   unsigned int etime_LN;
   unsigned int Inte_time;
	XM_printf ("imx322 1080P 12bit mode\n");
	
	// 1) FRM_LENGTH配置成0x465会导致ISP无法在一帧间隔内完成处理及数据DDR写入, 因此会出现只有帧同步中断, 无帧完成中断,
	// 但是帧数据一直在更新的情况. (这种情况下的帧应该丢弃)
	// 应增加消隐行的个数来满足ISP整个流程所需的时序.
	// FRM_LENGTH = 0x465 + 0x20;
	// 2) 消隐行较小时，ISP Scalar会出现Y/UV Pop Error, 此时应增加消隐行的行数
	FRM_LENGTH = frame_lines;

	//FRM_LENGTH = 0x465;// 1125
//I2C_WRITE (0x0104, 0x01);

	ret = I2C_WRITE (0x302C, 0x00);			// Trigger standby 
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x0100, 0x00);			// Standby control (I2C)
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x0112, 0x0C);			// I2C ADRES1 AD gradation setting (I2C)
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0113, 0x0C);			// I2C ADRES2 AD gradation setting (I2C)
	if(ret < 0)
		return ret;

	// FRM_LENGTH 0x465 = 1125
	ret = I2C_WRITE (0x0340, (unsigned char)(FRM_LENGTH>>8 ));			// Vertical (V) direction line number designation
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0341, (unsigned char)(FRM_LENGTH&0xff));
	if(ret < 0)
		return ret;
	
	if (isp_get_sensor_fps() == 25) {
		// FPS_25
		// 1320 = 0x0528
		ret = I2C_WRITE (0x0342, 0x05);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
		ret = I2C_WRITE (0x0343, 0x28);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
	} else {
		// LINE_LENGTH 0x44C = 1100
		//I2C_WRITE (0x0342, 0x08);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		//I2C_WRITE (0x0343, 0x98);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		ret = I2C_WRITE (0x0342, 0x04);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
		ret = I2C_WRITE (0x0343, 0x4c);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
	}

	// MODE
	ret = I2C_WRITE (0x3002, 0x0F);			// HD1080 p mode
	if(ret < 0)
		return ret;
	
	// FRSEL Output data rate designation
	// FRSEL [2:0]
	//	Output data rate designation
	//		0: 2 times INCK
	//		1: Equal to INCK
	//		Others: Invalid
	ret = I2C_WRITE (0x3011, 0x00);
	if(ret < 0)
		return ret;

	// WINPV (Adjustments register for each operation mode)
	ret = I2C_WRITE (0x3016, 0x3C); // HD1080p: 3Ch
	if(ret < 0)
		return ret;

	// 10BITA (Adjustments register for each operation mode.)
	ret = I2C_WRITE (0x3021, (0x0 << 7) | (XHS_low_level_width_6_clk << 4));
	if(ret < 0)
		return ret;

	// 720PMODE (Sets in 720 p mode only.)
	ret = I2C_WRITE (0x3022, (0x0 << 7) | (XVS_low_level_width_1_line << 0));
	if(ret < 0)
		return ret;

#if IMX323_DCK_SYNC_MODE_ENABLE
	// sync mode selection
	// 07h Normal sync mode
	// 47h DCK sync mode
	ret = I2C_WRITE (0x304F, 0x47);	
	if(ret < 0)
		return ret;
	
	// [4] SYNCSEL
	// sync mode selection
	// 0 Normal sync mode
	// 1 DCK sync mode
	ret = I2C_WRITE (0x3054, (0x1 << 4) );	
	if(ret < 0)
		return ret;
#endif	
	
	// 10BITB
	ret = I2C_WRITE (0x307A, 0x00);
	if(ret < 0)
		return ret;
	// 10BITC
	ret = I2C_WRITE (0x307B, 0x00);
	if(ret < 0)
		return ret;
	// 10B1080 P (226h)
	ret = I2C_WRITE (0x3098, 0x26);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3099, 0x02);
	if(ret < 0)
		return ret;
	// 12B1080 P (226h)
	ret = I2C_WRITE (0x309A, 0x26);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x309B, 0x02);
	if(ret < 0)
		return ret;
	//I2C_WRITE (0x309A, 0x4c);
	//I2C_WRITE (0x309B, 0x04);   
	// PRES
	ret = I2C_WRITE (0x30CE, 0x16);
	if(ret < 0)
		return ret;
	// DRES
	ret = I2C_WRITE (0x30CF, 0x82);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x30D0, 0x00);
	if(ret < 0)
		return ret;

	// Black level offset value setting
	// 03Ch(min) 1FFh(max)
	ret = I2C_WRITE (0x0008, 0x00);
	if(ret < 0)
		return ret;
	//I2C_WRITE (0x0009, 0x3C);
	//I2C_WRITE (0x0009, 0x3c);
	ret = I2C_WRITE (0x0009, 0xF0);		// 12-bit output: F0h (240d)
	if(ret < 0)
		return ret;
	
	// Vertical (V) scanning direction control (I2C)
	ret = I2C_WRITE (0x0101, VERTICAL_SCANNING_DIRECTION << 1);			// Normal
	//ret = I2C_WRITE (0x0101, 0x03);			// Normal
	if(ret < 0)
		return ret;

	// Integration time adjustment (I2C) Designated in line units
   etime_LN = 1124;
   Inte_time = (1125*10-etime_LN*10-3)/10;
   
	ret = I2C_WRITE (0x0202, Inte_time>>8);			// INTEG_TIME [15:8]
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0203, Inte_time&0xff);			// INTEG_TIME [7:0]
	if(ret < 0)
		return ret;

	// SPL[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	ret = I2C_WRITE (0x300D, 0x00);			// SPL[7:0]
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x300E, 0x00);			// SPL[9:8]
	if(ret < 0)
		return ret;
	// SVS[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	ret = I2C_WRITE (0x300F, 0x00);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3010, 0x00);
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x3012, 0x82);
	if(ret < 0)
		return ret;

	// Gain setting
	ret = I2C_WRITE (0x301E, 0x00);
	if(ret < 0)
		return ret;

	// The values must be changed from the default values, so initial setting after reset is required after power-on.
	ret = I2C_WRITE (0x301F, 0x73);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3027, 0x20);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3117, 0x0D);
	if(ret < 0)
		return ret;


	// Trigger for master mode operation start
	ret = I2C_WRITE (0x302C, 0x00);
	if(ret < 0)
		return ret;

 //  I2C_WRITE (0x0104, 0x00);
	// Normal Operation
	ret = I2C_WRITE (0x0100, 0x01);
	if(ret < 0)
		return ret;
	
	return 0;
}
EXPORT_SYMBOL(imx322_init_12bit);


// 1080P模式 10bit
int imx322_init_10bit  (unsigned int frame_lines)
{
	int ret;
	unsigned int etime_LN;
	unsigned int Inte_time;
	XM_printf ("imx322 1080P 10bit mode\n");
	
	// 1) FRM_LENGTH配置成0x465会导致ISP无法在一帧间隔内完成处理及数据DDR写入, 因此会出现只有帧同步中断, 无帧完成中断,
	// 但是帧数据一直在更新的情况. (这种情况下的帧应该丢弃)
	// 应增加消隐行的个数来满足ISP整个流程所需的时序.
	// FRM_LENGTH = 0x465 + 0x20;
	// 2) 消隐行较小时，ISP Scalar会出现Y/UV Pop Error, 此时应增加消隐行的行数
	FRM_LENGTH = frame_lines;

	//FRM_LENGTH = 0x465;// 1125
//I2C_WRITE (0x0104, 0x01);

	ret = I2C_WRITE (0x302C, 0x00);			// Trigger standby 
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x0100, 0x00);			// Standby control (I2C)
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x0112, 0x0A);			// I2C ADRES1 AD gradation setting (I2C)
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0113, 0x0A);			// I2C ADRES2 AD gradation setting (I2C)
	if(ret < 0)
		return ret;

	// FRM_LENGTH 0x465 = 1125
	ret = I2C_WRITE (0x0340, (unsigned char)(FRM_LENGTH>>8 ));			// Vertical (V) direction line number designation
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0341, (unsigned char)(FRM_LENGTH&0xff));
	if(ret < 0)
		return ret;

	if (isp_get_sensor_fps() == 25) {
		ret = I2C_WRITE (0x0342, 0x05);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
		ret = I2C_WRITE (0x0343, 0x28);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
	} else {
		// LINE_LENGTH 0x44C = 1100
		//I2C_WRITE (0x0342, 0x08);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		//I2C_WRITE (0x0343, 0x98);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		ret = I2C_WRITE (0x0342, 0x04);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
		ret = I2C_WRITE (0x0343, 0x4c);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			return ret;
	}

	// MODE
	ret = I2C_WRITE (0x3002, 0x0F);			// HD1080 p mode
	if(ret < 0)
		return ret;
	
	// FRSEL Output data rate designation
	// FRSEL [2:0]
	//	Output data rate designation
	//		0: 2 times INCK
	//		1: Equal to INCK
	//		Others: Invalid
	ret = I2C_WRITE (0x3011, 0x00);
	if(ret < 0)
		return ret;

	// WINPV (Adjustments register for each operation mode)
	ret = I2C_WRITE (0x3016, 0x3C); // HD1080p: 3Ch
	if(ret < 0)
		return ret;

	// 10BITA (Adjustments register for each operation mode.)
	ret = I2C_WRITE (0x3021, (0x1 << 7) | (XHS_low_level_width_6_clk << 4));
	if(ret < 0)
		return ret;

	// 720PMODE (Sets in 720 p mode only.)
	ret = I2C_WRITE (0x3022, (0x0 << 7) | (XVS_low_level_width_1_line << 0));
	if(ret < 0)
		return ret;

#if IMX323_DCK_SYNC_MODE_ENABLE
	// sync mode selection
	// 07h Normal sync mode
	// 47h DCK sync mode
	ret = I2C_WRITE (0x304F, 0x47);	
	if(ret < 0)
		return ret;
	
	// [4] SYNCSEL
	// sync mode selection
	// 0 Normal sync mode
	// 1 DCK sync mode
	ret = I2C_WRITE (0x3054, (0x1 << 4) );	
	if(ret < 0)
		return ret;
#endif	
	
	// 10BITB
	ret = I2C_WRITE (0x307A, 0x40);
	if(ret < 0)
		return ret;
	// 10BITC
	ret = I2C_WRITE (0x307B, 0x02);
	if(ret < 0)
		return ret;
	// 10B1080 P (226h)
	ret = I2C_WRITE (0x3098, 0x26);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3099, 0x02);
	if(ret < 0)
		return ret;
	// 12B1080 P (226h)
	ret = I2C_WRITE (0x309A, 0x4c);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x309B, 0x04);
	if(ret < 0)
		return ret;
	//I2C_WRITE (0x309A, 0x4c);
	//I2C_WRITE (0x309B, 0x04);   
	// PRES
	ret = I2C_WRITE (0x30CE, 0x16);
	if(ret < 0)
		return ret;
	// DRES
	ret = I2C_WRITE (0x30CF, 0x82);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x30D0, 0x00);
	if(ret < 0)
		return ret;

	// Black level offset value setting
	// 03Ch(min) 1FFh(max)
	ret = I2C_WRITE (0x0008, 0x00);
	if(ret < 0)
		return ret;
	//I2C_WRITE (0x0009, 0x3C);
	//I2C_WRITE (0x0009, 0x3c);
	ret = I2C_WRITE (0x0009, 0xF0);		// 12-bit output: F0h (240d)
	if(ret < 0)
		return ret;
	
	// Vertical (V) scanning direction control (I2C)
	ret = I2C_WRITE (0x0101, VERTICAL_SCANNING_DIRECTION << 1);			// Normal
	//ret = I2C_WRITE (0x0101, 0x03);			// Normal
	if(ret < 0)
		return ret;

	// Integration time adjustment (I2C) Designated in line units
   etime_LN = 1124;
   Inte_time = (1125*10-etime_LN*10-3)/10;
   
	ret = I2C_WRITE (0x0202, Inte_time>>8);			// INTEG_TIME [15:8]
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0203, Inte_time&0xff);			// INTEG_TIME [7:0]
	if(ret < 0)
		return ret;

	// SPL[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	ret = I2C_WRITE (0x300D, 0x00);			// SPL[7:0]
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x300E, 0x00);			// SPL[9:8]
	if(ret < 0)
		return ret;
	// SVS[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	ret = I2C_WRITE (0x300F, 0x00);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3010, 0x00);
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x3012, 0x80);
	if(ret < 0)
		return ret;

	// Gain setting
	ret = I2C_WRITE (0x301E, 0x00);
	if(ret < 0)
		return ret;
	
	// [3] BITSEL
	//	10-bit output 2-bit shift
	//	0: Left justified, 1: Right justified
	I2C_WRITE (0x302D, 0x40 | (1 << 3));
	//I2C_WRITE (0x302D, 0x40 | (0 << 3));
	

	// The values must be changed from the default values, so initial setting after reset is required after power-on.
	ret = I2C_WRITE (0x301F, 0x73);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3027, 0x20);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3117, 0x0D);
	if(ret < 0)
		return ret;


	// Trigger for master mode operation start
	ret = I2C_WRITE (0x302C, 0x00);
	if(ret < 0)
		return ret;

 //  I2C_WRITE (0x0104, 0x00);
	// Normal Operation
	ret = I2C_WRITE (0x0100, 0x01);
	if(ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(imx322_init_10bit);

// 1080P模式 10bit
int imx322_init_1080p_30fps_10bit  (unsigned int frame_lines)
{
	int ret;
   unsigned int etime_LN;
   unsigned int Inte_time;
	XM_printf ("imx322 1080P 30fps 10bit mode\n");
	
	// 1) FRM_LENGTH配置成0x465会导致ISP无法在一帧间隔内完成处理及数据DDR写入, 因此会出现只有帧同步中断, 无帧完成中断,
	// 但是帧数据一直在更新的情况. (这种情况下的帧应该丢弃)
	// 应增加消隐行的个数来满足ISP整个流程所需的时序.
	// FRM_LENGTH = 0x465 + 0x20;
	// 2) 消隐行较小时，ISP Scalar会出现Y/UV Pop Error, 此时应增加消隐行的行数
	FRM_LENGTH = frame_lines;

	//FRM_LENGTH = 0x465;// 1125
//I2C_WRITE (0x0104, 0x01);

	ret = I2C_WRITE (0x302C, 0x00);			// Trigger standby 
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x0100, 0x00);			// Standby control (I2C)
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x0112, 0x0A);			// I2C ADRES1 AD gradation setting (I2C)
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0113, 0x0A);			// I2C ADRES2 AD gradation setting (I2C)
	if(ret < 0)
		return ret;

	// FRM_LENGTH 0x465 = 1125
	ret = I2C_WRITE (0x0340, (unsigned char)(FRM_LENGTH>>8 ));			// Vertical (V) direction line number designation
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0341, (unsigned char)(FRM_LENGTH&0xff));
	if(ret < 0)
		return ret;
	// LINE_LENGTH 0x44C = 1100
	//I2C_WRITE (0x0342, 0x08);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
	//I2C_WRITE (0x0343, 0x98);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
	ret = I2C_WRITE (0x0342, 0x04);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0343, 0x4c);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
	if(ret < 0)
		return ret;

	// MODE
	ret = I2C_WRITE (0x3002, 0x0F);			// HD1080 p mode
	if(ret < 0)
		return ret;
	
	// FRSEL Output data rate designation
	// FRSEL [2:0]
	//	Output data rate designation
	//		0: 2 times INCK
	//		1: Equal to INCK
	//		Others: Invalid
	ret = I2C_WRITE (0x3011, 0x00);
	if(ret < 0)
		return ret;

	// WINPV (Adjustments register for each operation mode)
	ret = I2C_WRITE (0x3016, 0x3C); // HD1080p: 3Ch
	if(ret < 0)
		return ret;

	// 10BITA (Adjustments register for each operation mode.)
	ret = I2C_WRITE (0x3021, (0x1 << 7) | (XHS_low_level_width_6_clk << 4));
	if(ret < 0)
		return ret;

	// 720PMODE (Sets in 720 p mode only.)
	ret = I2C_WRITE (0x3022, (0x0 << 7) | (XVS_low_level_width_1_line << 0));
	if(ret < 0)
		return ret;

#if IMX323_DCK_SYNC_MODE_ENABLE
	// sync mode selection
	// 07h Normal sync mode
	// 47h DCK sync mode
	ret = I2C_WRITE (0x304F, 0x47);	
	if(ret < 0)
		return ret;
	
	// [4] SYNCSEL
	// sync mode selection
	// 0 Normal sync mode
	// 1 DCK sync mode
	ret = I2C_WRITE (0x3054, (0x1 << 4) );	
	if(ret < 0)
		return ret;
#endif	
	
	// 10BITB
	ret = I2C_WRITE (0x307A, 0x40);
	if(ret < 0)
		return ret;
	// 10BITC
	ret = I2C_WRITE (0x307B, 0x02);
	if(ret < 0)
		return ret;
	// 10B1080 P (226h)
	ret = I2C_WRITE (0x3098, 0x26);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3099, 0x02);
	if(ret < 0)
		return ret;
	// 12B1080 P (44ch)
	ret = I2C_WRITE (0x309A, 0x4c);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x309B, 0x04);
	if(ret < 0)
		return ret;
	//I2C_WRITE (0x309A, 0x4c);
	//I2C_WRITE (0x309B, 0x04);   
	// PRES
	ret = I2C_WRITE (0x30CE, 0x16);
	if(ret < 0)
		return ret;
	// DRES
	ret = I2C_WRITE (0x30CF, 0x82);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x30D0, 0x00);
	if(ret < 0)
		return ret;

	// Black level offset value setting
	// 03Ch(min) 1FFh(max)
	ret = I2C_WRITE (0x0008, 0x00);
	if(ret < 0)
		return ret;
	//I2C_WRITE (0x0009, 0x3C);
	ret = I2C_WRITE (0x0009, 0x3c);			// 10-bit output: 3ch (60d)
	//ret = I2C_WRITE (0x0009, 0xF0);		// 12-bit output: F0h (240d)
	if(ret < 0)
		return ret;
	
	// Vertical (V) scanning direction control (I2C)
	ret = I2C_WRITE (0x0101, VERTICAL_SCANNING_DIRECTION << 1);			// Normal
	//ret = I2C_WRITE (0x0101, 0x03);			// Normal
	if(ret < 0)
		return ret;

	// Integration time adjustment (I2C) Designated in line units
   etime_LN = 1124;
   Inte_time = (1125*10-etime_LN*10-3)/10;
   
	ret = I2C_WRITE (0x0202, Inte_time>>8);			// INTEG_TIME [15:8]
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x0203, Inte_time&0xff);			// INTEG_TIME [7:0]
	if(ret < 0)
		return ret;

	// SPL[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	ret = I2C_WRITE (0x300D, 0x00);			// SPL[7:0]
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x300E, 0x00);			// SPL[9:8]
	if(ret < 0)
		return ret;
	// SVS[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	ret = I2C_WRITE (0x300F, 0x00);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3010, 0x00);
	if(ret < 0)
		return ret;

	ret = I2C_WRITE (0x3012, 0x82);
	if(ret < 0)
		return ret;

	// Gain setting
	ret = I2C_WRITE (0x301E, 0x00);
	if(ret < 0)
		return ret;
	
	// [3] BITSEL
	// 选择10-bit 左对齐输出, 使用数据线D2~D11, D0~D1不使用
	//	10-bit output 2-bit shift
	//	0: Left justified, 1: Right justified
	ret = I2C_WRITE (0x302D, 0x40 | (1 << 3));//add luo
	if(ret < 0)
		return ret;

	// The values must be changed from the default values, so initial setting after reset is required after power-on.
	ret = I2C_WRITE (0x301F, 0x73);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3027, 0x20);
	if(ret < 0)
		return ret;
	ret = I2C_WRITE (0x3117, 0x0D);
	if(ret < 0)
		return ret;


	// Trigger for master mode operation start
	ret = I2C_WRITE (0x302C, 0x00);
	if(ret < 0)
		return ret;

 //  I2C_WRITE (0x0104, 0x00);
	// Normal Operation
	ret = I2C_WRITE (0x0100, 0x01);
	if(ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL(imx322_init_1080p_30fps_10bit);


// 1080P模式 10bit, 15fps
//参考 spec 63 页 
int imx322_init_10bit_old (unsigned int frame_lines)
{
   unsigned int etime_LN;
   unsigned int Inte_time;
	XM_printf ("imx322 init 1080P 10bit\n");

	FRM_LENGTH = frame_lines;
//	FRM_LENGTH = 0x465;// 1125
//I2C_WRITE (0x0104, 0x01);

	I2C_WRITE (0x302C, 0x00);			// Trigger standby 

	I2C_WRITE (0x0100, 0x00);			// Standby control (I2C)

	I2C_WRITE (0x0112, 0x0A);			// I2C ADRES1 AD gradation setting (I2C)
	I2C_WRITE (0x0113, 0x0A);			// I2C ADRES2 AD gradation setting (I2C)

	// FRM_LENGTH 0x465 = 1125
	I2C_WRITE (0x0340, (unsigned char)(FRM_LENGTH>>8 ));			// Vertical (V) direction line number designation
	I2C_WRITE (0x0341, (unsigned char)(FRM_LENGTH&0x0ff));
	// LINE_LENGTH 0x44C = 1100
	I2C_WRITE (0x0342, 0x08);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
	I2C_WRITE (0x0343, 0x98);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)

	// MODE
	I2C_WRITE (0x3002, 0x0F);			// HD1080 p mode

	// FRSEL Output data rate designation
	// FRSEL [2:0]
	//	Output data rate designation
	//		0: 2 times INCK
	//		1: Equal to INCK
	//		Others: Invalid
	I2C_WRITE (0x3011, 0x01);

	// WINPV (Adjustments register for each operation mode)
	I2C_WRITE (0x3016, 0x3C); // HD1080p: 3Ch

	// 10BITA (Adjustments register for each operation mode.)
	I2C_WRITE (0x3021, (1 << 7) | (XHS_low_level_width_6_clk << 4));

	// 720PMODE (Sets in 720 p mode only.)
	I2C_WRITE (0x3022, (0x0 << 7) | (XVS_low_level_width_1_line << 0));

	// 10BITB
	I2C_WRITE (0x307A, 0x40);
	// 10BITC
	I2C_WRITE (0x307B, 0x02);
	// 10B1080 P (44ch)
	I2C_WRITE (0x3098, 0x4c);
	I2C_WRITE (0x3099, 0x04);
	// 12B1080 P (226h)
	//I2C_WRITE (0x309A, 0x26);
	//I2C_WRITE (0x309B, 0x02);
	I2C_WRITE (0x309A, 0x4c);
	I2C_WRITE (0x309B, 0x04);   
	// PRES
	I2C_WRITE (0x30CE, 0x16);
	// DRES
	I2C_WRITE (0x30CF, 0x82);
	I2C_WRITE (0x30D0, 0x00);

	// Black level offset value setting
	I2C_WRITE (0x0008, 0x00);
	I2C_WRITE (0x0009, 0x40);
	
	// Vertical (V) scanning direction control (I2C)
	I2C_WRITE (0x0101, VERTICAL_SCANNING_DIRECTION << 1);			// Normal

	// Integration time adjustment (I2C) Designated in line units
   etime_LN = 1124;
   Inte_time = (1125*10-etime_LN*10-3)/10;

	I2C_WRITE (0x0202, Inte_time>>8);			// INTEG_TIME [15:8]
	I2C_WRITE (0x0203, Inte_time&0xff);			// INTEG_TIME [7:0]

	// SPL[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	I2C_WRITE (0x300D, 0x00);			// SPL[7:0]
	I2C_WRITE (0x300E, 0x00);			// SPL[9:8]
	// SVS[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	I2C_WRITE (0x300F, 0x00);
	I2C_WRITE (0x3010, 0x00);

	I2C_WRITE (0x3012, 0x82);
	
	// [3] BITSEL
	//	10-bit output 2-bit shift
	//	0: Left justified, 1: Right justified
	I2C_WRITE (0x302D, 0x40 | (1 << 3));
	//I2C_WRITE (0x302D, 0x40 | (0 << 3));


	// Gain setting
	I2C_WRITE (0x301E, 0x00);

	// The values must be changed from the default values, so initial setting after reset is required after power-on.
	I2C_WRITE (0x301F, 0x73);
	I2C_WRITE (0x3027, 0x20);
	I2C_WRITE (0x3117, 0x0D);


	// Trigger for master mode operation start
	I2C_WRITE (0x302C, 0x00);

   //I2C_WRITE (0x0104, 0x00);
	// Normal Operation
	I2C_WRITE (0x0100, 0x01);

	return 0;
}

// 1080P模式 10bit, 15fps
//参考 spec 63 页 
int imx322_init_1080p_10bit_15fps (void)
{
   unsigned int Inte_time;

	XM_printf ("cmos sensor imx322 init 10bit 15fps mode \n");

	FRM_LENGTH = 0x465;// 1125
//I2C_WRITE (0x0104, 0x01);

	I2C_WRITE (0x302C, 0x00);			// Trigger standby 

	I2C_WRITE (0x0100, 0x00);			// Standby control (I2C)

	I2C_WRITE (0x0112, 0x0A);			// I2C ADRES1 AD gradation setting (I2C)
	I2C_WRITE (0x0113, 0x0A);			// I2C ADRES2 AD gradation setting (I2C)

	// FRM_LENGTH 0x465 = 1125
	I2C_WRITE (0x0340, (unsigned char)(FRM_LENGTH>>8 ));			// Vertical (V) direction line number designation
	I2C_WRITE (0x0341, (unsigned char)(FRM_LENGTH&0x0ff));
	// LINE_LENGTH 0x44C = 1100
	I2C_WRITE (0x0342, 0x08);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)
	I2C_WRITE (0x0343, 0x98);			// Horizontal (H) direction clock number designation (I20343h [7:0] C)

	// MODE
	I2C_WRITE (0x3002, 0x0F);			// HD1080 p mode

	// FRSEL Output data rate designation
	// FRSEL [2:0]
	//	Output data rate designation
	//		0: 2 times INCK
	//		1: Equal to INCK
	//		Others: Invalid
	I2C_WRITE (0x3011, 0x01);

	// WINPV (Adjustments register for each operation mode)
	I2C_WRITE (0x3016, 0x3C); // HD1080p: 3Ch

	// 10BITA (Adjustments register for each operation mode.)
	I2C_WRITE (0x3021, 1);

	// 720PMODE (Sets in 720 p mode only.)
	I2C_WRITE (0x3022, (0x0 << 7) | (XVS_low_level_width_1_line << 0));

	// 10BITB
	I2C_WRITE (0x307A, 0x40);
	// 10BITC
	I2C_WRITE (0x307B, 0x02);
	// 10B1080 P (226h)
	I2C_WRITE (0x3098, 0x26);
	I2C_WRITE (0x3099, 0x02);
	// 12B1080 P (226h)
	//I2C_WRITE (0x309A, 0x26);
	//I2C_WRITE (0x309B, 0x02);
	I2C_WRITE (0x309A, 0x4c);
	I2C_WRITE (0x309B, 0x04);   
	// PRES
	I2C_WRITE (0x30CE, 0x16);
	// DRES
	I2C_WRITE (0x30CF, 0x82);
	I2C_WRITE (0x30D0, 0x00);

	// Black level offset value setting
	// 03Ch(min) 1FFh(max)
	I2C_WRITE (0x0008, 0x00);
	I2C_WRITE (0x0009, 0x3C);
	
	// Vertical (V) scanning direction control (I2C)
	I2C_WRITE (0x0101, VERTICAL_SCANNING_DIRECTION << 1);			// Normal

	// Integration time adjustment (I2C) Designated in line units
   Inte_time = FRM_LENGTH-1;
	I2C_WRITE (0x0202, Inte_time>>8);			// INTEG_TIME [15:8]
	I2C_WRITE (0x0203, Inte_time&0xff);			// INTEG_TIME [7:0]

	// SPL[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	I2C_WRITE (0x300D, 0x00);			// SPL[7:0]
	I2C_WRITE (0x300E, 0x00);			// SPL[9:8]
	// SVS[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
	I2C_WRITE (0x300F, 0x00);
	I2C_WRITE (0x3010, 0x00);

	I2C_WRITE (0x3012, 0x82);

	// Gain setting
	I2C_WRITE (0x301E, 0x00);

	// The values must be changed from the default values, so initial setting after reset is required after power-on.
	I2C_WRITE (0x301F, 0x73);
	I2C_WRITE (0x3027, 0x20);
	I2C_WRITE (0x3117, 0x0D);


	// Trigger for master mode operation start
	I2C_WRITE (0x302C, 0x00);

   I2C_WRITE (0x0104, 0x00);
	// Normal Operation
	I2C_WRITE (0x0100, 0x01);

	return 0;
}
EXPORT_SYMBOL(imx322_init_1080p_10bit_15fps);


// 720P模式 
// 20170411 720P 30帧 10bit模式(按照IMX323规格书参数配置), 白平衡出现问题(亮白色变成粉红色).
//          改为使用720P 60帧 10bit模式的参数, 将其频率及行数修改为30帧模式的参数, 白平衡问题不在出现.
//          1080P与720P共用一套白平衡参数
// 不支持12bit 720p模式
int imx322_init_720p_mode (int mode, unsigned int frame_lines)
{
	int ret = -1;
	if(mode == IMX322_MODE_HD720P_10BIT_30FPS)
	{
		XM_printf ("cmos sensor imx322 init 720P 10bit 30fps mode\n");
	}
	else if(mode == IMX322_MODE_HD720P_10BIT_60FPS)
	{
		XM_printf ("cmos sensor imx322 init 720P 10bit 60fps mode\n");
	}
	else if(mode == IMX322_MODE_HD720P_12BIT_30FPS)
	{
		XM_printf ("720P 12bit 30fps mode not support\n");
		return -1;
	}
	else
	{
		XM_printf ("illegal imx322 720P mode\n");
		return -1;
	}
	
	do
	{
		int LINE_LENGTH;
		unsigned int XHSLNG2;
		
		ret = I2C_WRITE (0x302C, 0x01);			// Trigger standby
		if(ret < 0)
			break;
	
		ret = I2C_WRITE (0x0100, 0x00);			// Standby control (I2C)
		if(ret < 0)
			break;
	
		if(mode == IMX322_MODE_HD720P_12BIT_30FPS)
		{
			// 12bit
			ret = I2C_WRITE (0x0112, 0x0C);			// I2C ADRES1 AD gradation setting (I2C)
			if(ret < 0)
				break;
			ret = I2C_WRITE (0x0113, 0x0C);			// I2C ADRES2 AD gradation setting (I2C)
			if(ret < 0)
				break;
		}
		else
		{
			// 10bit 
			ret = I2C_WRITE (0x0112, 0x0A);			// I2C ADRES1 AD gradation setting (I2C)
			if(ret < 0)
				break;
			ret = I2C_WRITE (0x0113, 0x0A);			// I2C ADRES2 AD gradation setting (I2C)
			if(ret < 0)
				break;
		}
	
		// FRM_LENGTH 
		// 0x02ee (750)
		FRM_LENGTH = frame_lines;
		
		// Vertical (V) direction line number designation
		ret = I2C_WRITE (0x0340, (unsigned char)(FRM_LENGTH >> 8));
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x0341, (unsigned char)(FRM_LENGTH & 0xFF));
		if(ret < 0)
			break;
	
		if(mode == IMX322_MODE_HD720P_10BIT_60FPS)
		{
			// 0339h (825)
			LINE_LENGTH = 0x0339;
		}
		else
		{
			// 0672h (1650)
			LINE_LENGTH = 0x0672;
		}
		ret = I2C_WRITE (0x0342, (unsigned char)(LINE_LENGTH >> 8));		// Horizontal (H) direction clock number designation (I20343h [7:0] C)
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x0343, (unsigned char)(LINE_LENGTH & 0xFF));
		if(ret < 0)
			break;
		
		// MODE
		ret = I2C_WRITE (0x3002, 0x01);			// HD720 p mode
		if(ret < 0)
			break;
	
		// FRSEL Output data rate designation
		// FRSEL [2:0]
		//	Output data rate designation
		//		0: 2 times INCK
		//		1: Equal to INCK
		//		Others: Invalid
		if(mode == IMX322_MODE_HD720P_10BIT_60FPS)
			ret = I2C_WRITE (0x3011, 0x00);
		else
			ret = I2C_WRITE (0x3011, 0x01);
		if(ret < 0)
			break;
	
		// WINPV (Adjustments register for each operation mode)
		ret = I2C_WRITE (0x3016, 0xF0);
		if(ret < 0)
			break;
	
		// 10BITA (Adjustments register for each operation mode.)
		if(mode == IMX322_MODE_HD720P_10BIT_30FPS)
			//ret = I2C_WRITE (0x3021, (0x1 << 7) | (XHS_low_level_width_12_clk << 4));
			ret = I2C_WRITE (0x3021, (0x0 << 7) | (XHS_low_level_width_12_clk << 4));
		else if(mode == IMX322_MODE_HD720P_10BIT_60FPS)
			ret = I2C_WRITE (0x3021, (0x0 << 7) | (XHS_low_level_width_12_clk << 4));
			
		else
			ret = I2C_WRITE (0x3021, (0x0 << 7) | (XHS_low_level_width_12_clk << 4));
		if(ret < 0)
			break;
	
		// 720PMODE (Sets in 720 p mode only.)
		ret = I2C_WRITE (0x3022, (0x1 << 7) | (XVS_low_level_width_1_line << 0));
		if(ret < 0)
			break;
	

#if IMX323_DCK_SYNC_MODE_ENABLE
		// sync mode selection
		// 07h Normal sync mode
		// 47h DCK sync mode
		ret = I2C_WRITE (0x304F, 0x47);	
		if(ret < 0)
			break;
		
		// [4] SYNCSEL
		// sync mode selection
		// 0 Normal sync mode
		// 1 DCK sync mode 
		XHSLNG2 = 1; // (See Driving Timing Chart in HD720p Mode)
		ret = I2C_WRITE (0x3054, ((0x1 << 4) | (XHSLNG2 << 0)) );	
		if(ret < 0)
			break;
#endif			
		
		/*if(mode == IMX322_MODE_HD720P_10BIT_30FPS)
		{
			// 10BITB
			ret = I2C_WRITE (0x307A, 0x40);
			if(ret < 0)
				break;
			// 10BITC
			ret = I2C_WRITE (0x307B, 0x02);
			if(ret < 0)
				break;
		}
		else*/
		{
			// 10bit 60fps / 12bit 30fps
			// 10BITB
			ret = I2C_WRITE (0x307A, 0x00);
			if(ret < 0)
				break;
			// 10BITC
			ret = I2C_WRITE (0x307B, 0x00);
			if(ret < 0)
				break;
		}
		// 10B1080 P (226h)
		ret = I2C_WRITE (0x3098, 0x26);
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x3099, 0x02);
		if(ret < 0)
			break;
		// 12B1080 P (44ch)
		ret = I2C_WRITE (0x309A, 0x4c);
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x309B, 0x04);
		if(ret < 0)
			break;
		// PRES
		if(mode == IMX322_MODE_HD720P_12BIT_30FPS)
			ret = I2C_WRITE (0x30CE, 0x40);
		else
			ret = I2C_WRITE (0x30CE, 0x00);
		if(ret < 0)
			break;
		
		// DRES
		if(mode == IMX322_MODE_HD720P_12BIT_30FPS)
		{
			ret = I2C_WRITE (0x30CF, 0x81);
			if(ret < 0)
				break;
			ret = I2C_WRITE (0x30D0, 0x01);
			if(ret < 0)
				break;
		}
		else
		{
			ret = I2C_WRITE (0x30CF, 0x00);
			if(ret < 0)
				break;
			ret = I2C_WRITE (0x30D0, 0x00);
			if(ret < 0)
				break;
		}
	
		// Black level offset value setting
		// Use with values shown below is recommended
		if(mode == IMX322_MODE_HD720P_12BIT_30FPS)
		{
			// 12-bit output: F0h (240d)
			ret = I2C_WRITE (0x0008, 0x00);
			if(ret < 0)
				break;
			ret = I2C_WRITE (0x0009, 0xF0);
			if(ret < 0)
				break;			
		}
		else
		{
			// 10-bit output: 3Ch (60d)
			ret = I2C_WRITE (0x0008, 0x00);
			if(ret < 0)
				break;
			ret = I2C_WRITE (0x0009, 0x3C);
			if(ret < 0)
				break;
		}
		
		// Vertical (V) scanning direction control (I2C)
		ret = I2C_WRITE (0x0101, VERTICAL_SCANNING_DIRECTION << 1);			// Normal
		if(ret < 0)
			break;
	
		// Integration time adjustment (I2C) Designated in line units
		ret = I2C_WRITE (0x0202, 0x00);			// INTEG_TIME [15:8]
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x0203, 0x40);			// INTEG_TIME [7:0]
		if(ret < 0)
			break;
	
		// SPL[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
		ret = I2C_WRITE (0x300D, 0x00);			// SPL[7:0]
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x300E, 0x00);			// SPL[9:8]
		if(ret < 0)
			break;
		// SVS[9:0] Integration time adjustment (Low-speed shutter) Designated in frame units
		ret = I2C_WRITE (0x300F, 0x00);
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x3010, 0x00);
		if(ret < 0)
			break;
	
		ret = I2C_WRITE (0x3012, 0x80);
		if(ret < 0)
			break;
	
		// Gain setting
		ret = I2C_WRITE (0x301E, 0x00);
		if(ret < 0)
			break;
		
		// [3] BITSEL
		if(mode == IMX322_MODE_HD720P_12BIT_30FPS)
		{
			I2C_WRITE (0x302D, 0x40 | (0 << 3));
		}
		else
		{
			// 选择10-bit 左对齐输出, 使用数据线D2~D11, D0~D1不使用
			//	10-bit output 2-bit shift
			//	0: Left justified, 1: Right justified
			//ret = I2C_WRITE (0x302D, 0x40 | (1 << 3));
			ret = I2C_WRITE (0x302D, 0x40 | (0 << 3));
			if(ret < 0)
				break;
		}
		
	
		// The values must be changed from the default values, so initial setting after reset is required after power-on.
		ret = I2C_WRITE (0x301F, 0x73);
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x3027, 0x20);
		if(ret < 0)
			break;
		ret = I2C_WRITE (0x3117, 0x0D);
		if(ret < 0)
			break;
	
	
		// Trigger for master mode operation start
		ret = I2C_WRITE (0x302C, 0x00);
		if(ret < 0)
			break;
	
		// Normal Operation
		ret = I2C_WRITE (0x0100, 0x01);
		if(ret < 0)
			break;
		
		ret = 0;
	} while (0);

	return ret;
}
EXPORT_SYMBOL(imx322_init_720p_mode);


// 720P模式 
int imx322_init_720p_10bit_30fps_mode (unsigned int frame_lines)
{
	return imx322_init_720p_mode (IMX322_MODE_HD720P_10BIT_30FPS, frame_lines);
	//return imx322_init_720p_mode (IMX322_MODE_HD720P_10BIT_60FPS, frame_lines);
	//return imx322_init_720p_mode (IMX322_MODE_HD720P_12BIT_30FPS, frame_lines);
}
EXPORT_SYMBOL(imx322_init_720p_10bit_30fps_mode);

int imx322_init_720p_12bit_30fps_mode (unsigned int frame_lines)
{
	//return imx322_init_720p_mode (IMX322_MODE_HD720P_10BIT_30FPS, frame_lines);
	//return imx322_init_720p_mode (IMX322_MODE_HD720P_10BIT_60FPS, frame_lines);
	return imx322_init_720p_mode (IMX322_MODE_HD720P_12BIT_30FPS, frame_lines);
}
EXPORT_SYMBOL(imx322_init_720p_12bit_30fps_mode);

int imx322_init_720p_10bit_60fps_mode (unsigned int frame_lines)
{
	return imx322_init_720p_mode (IMX322_MODE_HD720P_10BIT_60FPS, frame_lines);
	//return imx322_init_720p_mode (IMX322_MODE_HD720P_10BIT_60FPS, frame_lines);
	//return imx322_init_720p_mode (IMX322_MODE_HD720P_12BIT_30FPS, frame_lines);
}
EXPORT_SYMBOL(imx322_init_720p_10bit_60fps_mode);


static int imx322_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	unsigned int fps;

    i2c_client = client;
	printk("%s success\n", __FUNCTION__);

	of_property_read_u32(client->dev.of_node, "sensor-fps", &fps);
	isp_set_sensor_fps(fps);
	
	return 0;
}

static int imx322_remove(struct i2c_client *client)
{
	i2c_client = NULL;
	
	return 0;
}

static const struct of_device_id imx322_of_match[] = {
	{ .compatible = "arkmicro,imx322" },
	{ }
};
MODULE_DEVICE_TABLE(of, imx322_of_match);

static const struct i2c_device_id imx322_id[] = {
	{ "imx322", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx322_id);

static struct i2c_driver imx322_driver = {
	.driver = {
		.name = "imx322",
		.of_match_table = of_match_ptr(imx322_of_match),
	},
	.probe = imx322_probe,
	.remove = imx322_remove,
    .id_table = imx322_id,
};

module_i2c_driver(imx322_driver);

MODULE_DESCRIPTION("Sensor imx322 driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");

