#include <linux/delay.h>
#include <linux/math64.h>
#include "ark_camera.h"
#include "ark_isp_exposure_cmos.h"
#include "Gem_isp_io.h"


#define	ARKN141_CMOS_SENSOR_PS1210K		0
#define	ARKN141_CMOS_SENSOR_AR0330		1
#define	ARKN141_CMOS_SENSOR_IMX322		2
#define	ARKN141_CMOS_SENSOR_IMX323		3	
#define	ARKN141_CMOS_SENSOR_AR0238		4
#define	ARKN141_CMOS_SENSOR_AR0230		5
#define	ARKN141_CMOS_SENSOR_BG0806		6
#define	ARKN141_CMOS_SENSOR	ARKN141_CMOS_SENSOR_IMX323	

#define	_DISABLE_GAMMA_UNDER_LOW_LIGHT_
#define	EXPOSURE_LINES_ADDR			(0x0202)	// Integration time adjustment (I2C) Designated in line units	
#define	AGAIN_ADDR					(0x301E)	// Gain setting
#define	STD_30_LINES				(1125+20)		
//#define	STD_30_LINES			2200		
#define	CMOS_STD_INTTIME			(1125+20-2)
//#define	ONLY_ANALOG

/* module define: [0:close; 1:open] */
#define IQ_20190321 			0
#define IQ_20190322 			0
#define IQ_20190323 			0
#define SENSOR_IMX323 		1
#define SENSOR_GC2053	        1
#define REVERT_2_BUTTON			0	//镜头旋转180度
#ifndef ERIS_MANUAL
#define ERIS_MANUAL				0
#endif
#ifndef ISP_3D_DENOISE_SUPPORT
#define ISP_3D_DENOISE_SUPPORT	0	//0:3D disable; 1:3D enable
#endif

static cmos_inttime_t cmos_inttime;
static cmos_gain_t cmos_gain;
//static u16_t sensor_frame_rate = 1;
//static u16_t FRM_LENGTH = 1125;	// 1080P
//static u16_t FRM_LENGTH = 2200;		// 720P
static u16_t FRM_LENGTH = STD_30_LINES;
static u32_t last_exp_time = 0xFFFFFFFF;
static u32_t last_again = 0xFFFFFFFF;
//static u32_t last_dgain = 0xFFFFFFFF;
static u32_t isp_video_width  = 1920;
static u32_t isp_video_height = 1080;
static u32_t isp_sensor_bit = ARKN141_ISP_SENSOR_BIT_12;
static unsigned int isp_sensor_fps = 0;
// ISP的YUV输出格式定义( 0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 )
static unsigned int isp_yuv_format = ARKN141_ISP_YUV_FORMAT_Y_UV420;


extern int imx322_init_12bit  (unsigned int frame_lines);
extern int imx322_init_1080p_30fps_10bit  (unsigned int frame_lines);
extern int imx322_init_10bit  (unsigned int frame_lines);
extern int imx322_init_720p_10bit_30fps_mode (unsigned int frame_lines);
extern int imx322_init_720p_12bit_30fps_mode (unsigned int frame_lines);
extern int imx322_init_720p_10bit_60fps_mode (unsigned int frame_lines);
extern void isp_sensor_set_reset_pin_low (void);
extern void isp_sensor_set_reset_pin_high (void);
extern int imx322_i2c_read (u32_t addr);
extern int imx322_i2c_write(u32_t addr, u8_t data);


#if 0
static int arkn141_isp_cmos_sensor_read_register  (u32_t addr, u32_t *data)
{
	*data = imx322_i2c_read(addr);
    return 0;
}
#endif

static int arkn141_isp_cmos_sensor_write_register (u32_t addr, u32_t data)
{
	return imx322_i2c_write(addr, data);
}

static int XMSYS_H264CodecGetVideoFormat(void)
{
	return ARKN141_VIDEO_FORMAT_1080P_30;
}

u32_t isp_get_video_width  (void)
{
	return isp_video_width;
}

u32_t isp_get_video_height (void)
{
	return isp_video_height;
}

//0茂录拧y_uv420 1:y_uv422 2:yuv420 3:yuv422
unsigned int isp_get_video_format (void)
{
	return isp_yuv_format;
}

static unsigned int isp_get_sensor_bit (void)
{
	return isp_sensor_bit;
}

unsigned int isp_get_sensor_fps(void)
{
	return isp_sensor_fps;
}

void isp_set_sensor_fps(unsigned int fps)
{
	isp_sensor_fps = fps;
}

#if 0
static void cmos_inttime_gain_reset (void)
{
	last_exp_time = 0xFFFFFFFF;
	last_again = 0xFFFFFFFF;
	last_dgain = 0xFFFFFFFF;
}
#endif
// PCLK = 74.25MHz
// Frame Size = 2200 * 1125
// fps = 74.25 * 1000000 / (2200 * 1125) = 30 帧/秒
static  cmos_inttime_ptr_t cmos_inttime_initialize(void)
{
	cmos_inttime.full_lines = FRM_LENGTH;
	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.max_lines_target = (u16_t)(FRM_LENGTH - 1);
	//cmos_inttime.min_lines_target = 1;
	cmos_inttime.min_lines_target = 2;	// 改善逆光下太暗的情况
	
	cmos_inttime.exposure_ashort = 0;
	if(last_exp_time != 0xFFFFFFFF)
		cmos_inttime.exposure_ashort = last_exp_time;

	return &cmos_inttime;
}
#if 0
// The sensor's integration time is obtained by the following formula.
// Integration time = 1 frame period × (SVS + 1 - SPL) - (INTEG_TIME) × (1H period) - 0.3 [H] (However, SVS > SPL)
static void cmos_inttime_update (cmos_inttime_ptr_t p_inttime) 
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;

	exp_time = (u16_t)p_inttime->exposure_ashort;

	shutter_sweep_line_count = FRM_LENGTH - exp_time;
	//XM_printf ("INTEG_TIME = %4d\n", shutter_sweep_line_count);
	arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 0, (u8_t)(shutter_sweep_line_count >> 8) );	// INTEG_TIME [15:8]
	arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 1, (u8_t)(shutter_sweep_line_count     ) );	// INTEG_TIME [7:0]
}
#endif
// 不使用数字增益. 使用数字增益在场景光强急剧变化时会带来较大的噪声.
// 使用eris进行拉伸
// 20170106晚上的测试已验证, 开启部分或全部数字增益时需要使用较大的降噪因子, 而这会导致画面模糊(3D运动模糊)

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323

#ifdef ONLY_ANALOG
// ANALOG gain
static const u16_t analog_gain_table[] = 
{
	  256, 	  265, 	  274, 	  284, 	  294, 	  304, 	  315, 	  326, 
	  337, 	  349, 	  362, 	  374, 	  387, 	  401, 	  415, 	  430, 
	  445, 	  461, 	  477, 	  493, 	  511, 	  529, 	  547, 	  567, 
	  586, 	  607, 	  628, 	  650, 	  673, 	  697, 	  722, 	  747, 
	  773, 	  800, 	  828, 	  858, 	  888, 	  919, 	  951, 	  985, 
	 1019, 	 1055, 	 1092, 	 1130, 	 1170, 	 1211, 	 1254, 	 1298, 
	 1344, 	 1391, 	 1440, 	 1490, 	 1543, 	 1597, 	 1653, 	 1711, 
	 1771, 	 1833, 	 1898, 	 1964, 	 2033, 	 2105, 	 2179, 	 2255, 
	 2335, 	 2417, 	 2502, 	 2590, 	 2681, 	 2775, 	 2872, 	 2973, 
	 3078,
	 

};
	 
#else
// ANALOG + digital gain
static const u16_t analog_gain_table[] = 
{
	// analog gain, 信噪比较好
	  256, 	  265, 	  274, 	  284, 	  294, 	  304, 	  315, 	  326, 
	  337, 	  349, 	  362, 	  374, 	  387, 	  401, 	  415, 	  430, 
	  445, 	  461, 	  477, 	  493, 	  511, 	  529, 	  547, 	  567, 
	  586, 	  607, 	  628, 	  650, 	  673, 	  697, 	  722, 	  747, 
	  773, 	  800, 	  828, 	  858, 	  888, 	  919, 	  951, 	  985, 
	 1019, 	 1055, 	 1092, 	 1130, 	 1170, 	 1211, 	 1254, 	 1298, 
	 1344, 	 1391, 	 1440, 	 1490, 	 1543, 	 1597, 	 1653, 	 1711, 
	 1771, 	 1833, 	 1898, 	 1964, 	 2033, 	 2105, 	 2179, 	 2255, 
	 2335, 	 2417, 	 2502, 	 2590, 	 2681, 	 2775, 	 2872, 	 2973, 
	 3078, 	 
	 
	 // digital gain, 信噪比较差
	 3186, 	 3298, 	 3414, 	 3534, 	 3658, 	 3787, 	 3920, 
	 4057, 	 4200, 	 4348, 	 4500, 	 4658, 	 4822, 	 4992, 	 5167, 
	 5349, 	 5537, 	 5731, 	 5933, 	 6141, 	 6357, 	 6580, 	 6811, 
	 7051, 	 7299, 	 7555, 	 7821, 	 8095, 	 8380, 	 8674, 	 8979, 
	 9295, 	 9621, 	 9960, 	10310, 	10672, 	11047, 	11435, 	11837, 
	12253, 	12684, 	13129, 	13591, 	14068, 	14563, 	15074, 	15604, 
	16153, 	16720, 	17308, 	17916, 	18546, 	19197, 	19872, 	20570, 
	21293, 	22041, 	22816, 	23618, 	24448, 	25307, 	26196, 	27117, 
	28070, 	29056, 	30077, 	31134, 	32228, 	33361, 	34533, 	35747, 
	37003, 	38304, 	39650, 	41043, 	42485, 	43978, 	45524
};
#endif

#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322


#ifdef ONLY_ANALOG
// ANALOG gain
static const u16_t analog_gain_table[] =
{
	  512, 	  530, 	  549, 	  568, 	  588, 	  609, 	  630, 	  652, 
	  675, 	  699, 	  723, 	  749, 	  775, 	  802, 	  830, 	  860, 
	  890, 	  921, 	  953, 	  987, 	 1022, 	 1057, 	 1095, 	 1133, 
	 1173, 	 1214, 	 1257, 	 1301, 	 1347, 	 1394, 	 1443, 	 1494, 
	 1546, 	 1601, 	 1657, 	 1715, 	 1775, 	 1838, 	 1902, 	 1969, 
	 2038, 	 2110, 	 2184, 	 2261, 	 2340, 	 2423, 	 2508, 	 2596, 
	 2687, 	 2781, 	 2879, 	 2980, 	 3085, 	 3194, 	 3306, 	 3422, 
	 3542, 	 3667, 	 3796, 	 3929, 	 4067, 	 4210, 	 4358, 	 4511, 
	 4669, 	 4834, 	 5003, 	 5179, 	 5361, 	 5550, 	 5745, 	 5947, 
	 6156, 	 6372, 	 6596, 	 6828, 	 7068, 	 7316, 	 7573, 	 7839, 
	 8115,
};

#else

// ANALOG + digital gain
static const u16_t analog_gain_table[] =
{
	// ANALOG gain
	  512, 	  530, 	  549, 	  568, 	  588, 	  609, 	  630, 	  652, 
	  675, 	  699, 	  723, 	  749, 	  775, 	  802, 	  830, 	  860, 
	  890, 	  921, 	  953, 	  987, 	 1022, 	 1057, 	 1095, 	 1133, 
	 1173, 	 1214, 	 1257, 	 1301, 	 1347, 	 1394, 	 1443, 	 1494, 
	 1546, 	 1601, 	 1657, 	 1715, 	 1775, 	 1838, 	 1902, 	 1969, 
	 2038, 	 2110, 	 2184, 	 2261, 	 2340, 	 2423, 	 2508, 	 2596, 
	 2687, 	 2781, 	 2879, 	 2980, 	 3085, 	 3194, 	 3306, 	 3422, 
	 3542, 	 3667, 	 3796, 	 3929, 	 4067, 	 4210, 	 4358, 	 4511, 
	 4669, 	 4834, 	 5003, 	 5179, 	 5361, 	 5550, 	 5745, 	 5947, 
	 6156, 	 6372, 	 6596, 	 6828, 	 7068, 	 7316, 	 7573, 	 7839, 
	 8115, 	
	
	// digital gain
	 8400, 	 8695, 	 9001, 	 9317, 	 9644, 	 9983, 	10334, 
	10697, 	11073, 	11462, 	11865, 	12282, 	12714, 	13160, 	13623, 
	14102, 	14597, 	15110, 	15641, 	16191, 	16760, 	17349, 	17958, 
	18590, 	19243, 	19919, 	20619, 	21344, 	22094, 	22870, 	23674, 
	24506, 	25367, 	26259, 	27181, 	28136, 	29125, 	30149, 	31208, 
	32305, 	33440, 	34615, 	35832, 	37091, 	38395, 	39744, 	41141, 
	42586, 	44083, 	45632, 	47236, 	48896, 	50614, 	52393, 	54234, 
	56140, 	58113, 	60155, 	62269, 	64457
};
#endif

#endif	// ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323

 // 设置CMOS sensor允许使用的最大增益
static int imx322_cmos_max_gain_set (cmos_gain_ptr_t gain, unsigned int max_analog_gain,  unsigned int max_digital_gain);


// The Programmable Gain Control (PGC) of this device consists of the analog block and digital block.
// The total of analog gain and digital gain can be set up to 42 dB by the GAIN register (address 1Eh [7:0]) setting.
static cmos_gain_ptr_t cmos_gain_initialize(void)
{
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
	
	cmos_gain.again_shift = 8;
#ifdef ONLY_ANALOG
	 cmos_gain.max_again_target = (u16_t)(3078);
#else
	cmos_gain.max_again_target = (u16_t)(45524);		// 178倍增益
	//cmos_gain.max_again_target = (u16_t)(32305);	// 126
	//cmos_gain.max_again_target = (u16_t)(16384); 	// 64倍增益
#endif
	
#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322
	
	cmos_gain.again_shift = 9;
#ifdef ONLY_ANALOG
	cmos_gain.max_again_target = (u16_t)(8115);
#else
	cmos_gain.max_again_target = (u16_t)(64457);
#endif
	
#endif
	cmos_gain.again_count = sizeof(analog_gain_table)/sizeof(analog_gain_table[0]);
	
	cmos_gain.dgain_shift = 0;
	cmos_gain.max_dgain_target = 1;		// 禁止
	cmos_gain.dgain_count = 0;
	
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323

	//imx322_cmos_max_gain_set (&cmos_gain, 	32305, 1);		// 20170119晚上测试, 1 ) 车库效果较佳, 稍微存在一点过曝. 
																			//                   2 ) 马路上路灯及广告灯箱等存在严重过曝
																			//		应该是数字增益(8979)太大
	//imx322_cmos_max_gain_set (&cmos_gain, 	16384, 1);		// 64倍增益
	//imx322_cmos_max_gain_set (&cmos_gain, 	45524, 1);		// 177倍增益
	imx322_cmos_max_gain_set (&cmos_gain, 	24576, 1);			// 96倍增益	24576
	
	//imx322_cmos_max_gain_set (&cmos_gain, 	3920, 1);		// 稍微开启部分数字增益
	//imx322_cmos_max_gain_set (&cmos_gain, 	3078, 1);		// 20170223 仅开启模拟增益, 减小夜晚灯光的光晕现象
	
#endif
	
	if(last_again != 0xFFFFFFFF)
	{
		cmos_gain.aindex = last_again;
	}

	return &cmos_gain;
}

// 设置CMOS sensor允许使用的最大增益
static int imx322_cmos_max_gain_set (cmos_gain_ptr_t gain, unsigned int max_analog_gain,  unsigned int max_digital_gain)
{
	int count, index;
	if(gain == NULL)
		return -1;
	if(max_analog_gain == 0)
	{
		max_analog_gain = 1;		// 禁止模拟增益
	}
		
	if(gain->max_again_target != 1)
	{
		// 在增益表中查找该最大增益值
		count = sizeof(analog_gain_table) / sizeof(analog_gain_table[0]);
		for (index = 0; index < count; index ++)
		{
			if(analog_gain_table[index] >= max_analog_gain)
			{
				break;
			}
		}
		if(index >= count)
			index = count - 1;	// 使用最后一个增益值
		
		// 修改最大可用的增益值
		gain->max_again_target = analog_gain_table[index];
		gain->again_count = index + 1;		// 修改可用的增益项数量
	}
	return 0;
}

// 设置CMOS sensor允许使用的最大增益
static int imx322_cmos_max_gain_get (cmos_gain_ptr_t gain, unsigned int* max_analog_gain,  unsigned int* max_digital_gain)
{
	if(gain == NULL)
		return -1;
	*max_analog_gain = gain->max_again_target;
	*max_digital_gain = gain->max_dgain_target;
	return 0;
}
#if 0
static void cmos_gain_update (cmos_gain_ptr_t gain)
{
//	XM_printf ("AGAIN_REG, 0x%04x\n", gain->aindex);
//	XM_printf ("DGAIN_REG, 0x%04x\n", gain->dindex);

	arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
}
#endif
// 根据曝光量计算模拟增益
static u32_t analog_gain_from_exposure_calculate (cmos_gain_ptr_t gain, u32_t exposure, u32_t exposure_max)
{
	// 二分法查找最接近的模拟增益
	// 计算精度非常重要,会导致曝光的抖动
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
		//mid = mid / again;
		mid = div_s64(mid, again);
		// 寻找满足 mid <= exp的最大m
		if(mid < exp)
		{
			if(m > match)
				match = m;
			// 需增大模拟增益
			l = m + 1;
		}
		else if(mid > exp)
		{
			// 需减小模拟增益
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
	//exp = exp / analog_gain_table[m];
	div_s64(exp, analog_gain_table[m]);
	
	return (u32_t)exp;
}

// 返回值表示延迟指定的时间以便等待sensor曝光参数产生作用
//	1	表示下一帧sensor曝光参数产生影响
//	2	表示延迟1帧sensor曝光参数产生影响
//	3	表示延迟2帧sensor曝光参数产生影响
static int cmos_inttime_gain_update (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain) 
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;

	// Register Hold
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x01);		// register setting hold
	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
		last_exp_time = exp_time;
	
		shutter_sweep_line_count = FRM_LENGTH - exp_time;
		//XM_printf ("sweep_line=%d\n", shutter_sweep_line_count);
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 0, (u8_t)(shutter_sweep_line_count >> 8) );	// INTEG_TIME [15:8]
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 1, (u8_t)(shutter_sweep_line_count     ) );	// INTEG_TIME [7:0]
	}
	
	if(gain)
	{
		//XM_printf ("aindex=%d\n", gain->aindex);
		last_again = gain->aindex;
		arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
	}
	
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x00);		// reflection is applied
	// 返回值表示延迟指定的时间以便等待sensor曝光参数产生作用
	//	1	表示下一帧sensor曝光参数产生影响
	//	2	表示延迟1帧sensor曝光参数产生影响
	//	3	表示延迟2帧sensor曝光参数产生影响
	return 2;
}

static void cmos_inttime_gain_update_manual (cmos_inttime_ptr_t p_inttime, cmos_gain_ptr_t gain) 
{
	u16_t exp_time;
	u16_t shutter_sweep_line_count;
	int i, aindex;
	
	// 计算aindex, dindex
	for (i = 0; i < gain->again_count; i++)
	{
		if(analog_gain_table[i] >= gain->again)
			break;
	}
	if(i == gain->again_count)
		i = gain->again_count - 1;
	aindex = i;
	gain->aindex = aindex;

	// Register Hold
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x01);		// register setting hold
	if(p_inttime)
	{
		exp_time = (u16_t)p_inttime->exposure_ashort;
	
		shutter_sweep_line_count = FRM_LENGTH - exp_time;
		//XM_printf ("sweep_line=%d\n", shutter_sweep_line_count);
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 0, (u8_t)(shutter_sweep_line_count >> 8) );	// INTEG_TIME [15:8]
		arkn141_isp_cmos_sensor_write_register (EXPOSURE_LINES_ADDR + 1, (u8_t)(shutter_sweep_line_count     ) );	// INTEG_TIME [7:0]
	}
	
	if(gain)
	{
		arkn141_isp_cmos_sensor_write_register (AGAIN_ADDR, (u16_t)gain->aindex);
	}
	
	arkn141_isp_cmos_sensor_write_register (0x0104, 0x00);		// reflection is applied
}


static u32_t cmos_get_iso (cmos_gain_ptr_t gain)
{
	i64_t iso = gain->again;
	iso = (iso * 100) >> (gain->again_shift);
	
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

	// 设置sensor readout drection
	// horz_reverse_direction --> 1  horz reverse direction 垂直反向
	//                        --> 0  horz normal direction
	// vert_reverse_direction --> 1  vert reverse direction 水平反向
	//                        --> 0  vert normal direction
static int cmos_sensor_set_readout_direction (u8_t horz_reverse_direction, u8_t vert_reverse_direction)
{
	int ret;
	int val = 0;
	if(vert_reverse_direction)
		val |= 1 << 1;			// revert

#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
	if(horz_reverse_direction)
		val |= 1 << 0;			// revert
#endif	
	
	ret = arkn141_isp_cmos_sensor_write_register (0x0101, val);	
	
	return ret;
}

static const char *imx322_cmos_sensor_get_sensor_name (void)
{
#if ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX322
	return "IMX322";	
#elif ARKN141_CMOS_SENSOR == ARKN141_CMOS_SENSOR_IMX323
	return "IMX323";	
#endif
}

static int imx322_isp_sensor_init(isp_sen_ptr_t p_sen)
{
	int ret = 0;
	int loop = 10;
	int video_format = XMSYS_H264CodecGetVideoFormat();
	if( video_format == ARKN141_VIDEO_FORMAT_1080P_30)
	{	
		while(loop > 0)
		{
			// tLOW >= 500ns
			isp_sensor_set_reset_pin_low ();
			mdelay(1);
			isp_sensor_set_reset_pin_high ();
			mdelay(1);
			// 1920x1080
			//FRM_LENGTH = 1125 + 0x20;		// 0x465 + 0x20
			//FRM_LENGTH = 1125 + 0x120;	// 0x465 + 0x20
			//FRM_LENGTH = 1125 + 90;		// scale 40M输入/30帧输出
			//FRM_LENGTH = 1125 + 900;		// scale 40M输入/30帧输出
			//FRM_LENGTH = 1125 + 20;		// scale不出现pop error的最小值
			//FRM_LENGTH = 1125 + 20;		// scale不出现pop error的最小值
			//FRM_LENGTH = 1125 + 120;
			//FRM_LENGTH = 1125 + 350;
			//FRM_LENGTH = 1125 + 150;
			//FRM_LENGTH = 1125 + 450;
			FRM_LENGTH = 1125 + 30;	
			//if(isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
			{
				ret = imx322_init_12bit (FRM_LENGTH);
#if REVERT_2_BUTTON
				// 镜头旋转180度
				if(ret == 0)
				{
					cmos_sensor_set_readout_direction (1, 1);
				}
#endif
			}
			//else
			{
				//ret = imx322_init_1080p_30fps_10bit  (FRM_LENGTH);
				//ret = imx322_init_10bit (FRM_LENGTH);
			}
			
			if(ret == 0)
			{
				break;
			}
			loop --;
		}
		
		if(loop == 0)
		{
			ret = -1;
			XM_printf ("imx322 init 1080p NG\n");
		}
		else
		{
			ret = 0;
			XM_printf ("imx322 init 1080p OK\n");
		}
	}
	else if(video_format == ARKN141_VIDEO_FORMAT_720P_30 
#ifndef HONGJING_CVBS
			 || video_format == ARKN141_VIDEO_FORMAT_720P_60
#endif
			)
	{
		// 1280x720
		FRM_LENGTH = 0x02EE + 28;		// 0x02EE = 750
		while(loop > 0)
		{
			// tLOW >= 500ns
			isp_sensor_set_reset_pin_low ();
			mdelay(1);
			isp_sensor_set_reset_pin_high ();
			mdelay(1);
			if(video_format == ARKN141_VIDEO_FORMAT_720P_30)
			{
				//if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
				//	ret = imx322_init_720p_12bit_30fps_mode (FRM_LENGTH);
				//else
					ret = imx322_init_720p_10bit_30fps_mode (FRM_LENGTH);
			}
			else
				ret = imx322_init_720p_10bit_60fps_mode (FRM_LENGTH);
			//if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
			//	ret = imx322_init_720p_12bit_30fps_mode (FRM_LENGTH);
			//else
			//	ret = imx322_init_720p_10bit_30fps_mode (FRM_LENGTH);
			//	ret = imx322_init_720p_10bit_60fps_mode (FRM_LENGTH);
				
			if(ret == 0)
				break;
			loop --;
		}
		if(loop == 0)
		{
			ret = -1;
			XM_printf ("imx322 init 720p NG\n");
		}
		else
		{
			ret = 0;
			XM_printf ("imx322 init 720p OK\n");
		}
	}
	else
	{
		XM_printf ("un-support video format (%d)\n", video_format );
		return -1;
	}
	return ret;
}

static void imx322_cmos_isp_awb_init (isp_awb_ptr_t p_awb)		// 白平衡初始参数
{
	//p_awb->enable = 0;
  p_awb->enable = 1;    
  p_awb->mode = 1; //0：无效;  1：算法1，统一估计;  2：算法2，基于参考光源
  p_awb->manual = 0;//0=自动白平衡  1=手动白平衡
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
  
  // A		R_GAIN	0x127
  //			G_GAIN	0x113
  //			B_GAIN	0x386
  //
  // TL84	R_GAIN	0x1A8
  //			G_GAIN	0x113
  //			B_GAIN	0x2C9
  // D50		R_GAIN	0x1F2
  //			G_GAIN	0x113
  //			B_GAIN	0x22D
  // D65		R_GAIN	0x204
  //			G_GAIN	0x113
  //			B_GAIN	0x1CB
  // 10000K	R_GAIN	0x1F5
  //			G_GAIN	0x113
  //			B_GAIN	0x12C
  // G/R, G/B, 
  p_awb->r2g_light[0] = 136;
  p_awb->b2g_light[0] = 153;
  p_awb->r2g_light[1] = 141;
  p_awb->b2g_light[1] = 126;
  p_awb->r2g_light[2] = 141; 
  p_awb->b2g_light[2] = 235;
 // p_awb->r2g_light[3] = 143;   
 // p_awb->b2g_light[3] = 147;
  p_awb->r2g_light[3] = 166;
  p_awb->b2g_light[3] = 99;
  p_awb->r2g_light[4] = 239; 
  p_awb->b2g_light[4] = 78;
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
  
  p_awb->gain_g2r = 500;//434;
  p_awb->gain_g2b = 410;//348;

  isp_awb_init_io (p_awb);	
}



static isp_gamma_polyline_tbl gamma_polyline_tbl[] = {
	// 提升场景的对比度
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

static const isp_gamma_polyline_tbl *imx322_cmos_isp_get_gamma_table (int *tbl_count)
{
	*tbl_count = sizeof(gamma_polyline_tbl)/sizeof(gamma_polyline_tbl[0]);
	return gamma_polyline_tbl;
}


static void imx322_cmos_isp_colors_init (isp_colors_ptr_t p_colors)	// 色彩初始参数
{
  int i;
  
  p_colors->colorm.enable = 0;// 色矩阵 
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
  

  // 使用0~255范围, 尽量保留所有的细节
  p_colors->rgb2ypbpr_type = HDTV_type_0255;
  
  isp_create_rgb2ycbcr_matrix (p_colors->rgb2ypbpr_type, &p_colors->rgb2yuv);


  // demosaic 参数
	p_colors->demosaic.mode = 0;
	p_colors->demosaic.coff_00_07 = 32;
	p_colors->demosaic.coff_20_27 = 255;	// 滤波
	p_colors->demosaic.horz_thread = 0;
	//p_colors->demosaic.demk = 128;
	p_colors->demosaic.demk = 512;		// 20161230 改善解析度(树叶,纹理)
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

static void imx322_cmos_isp_denoise_init (isp_denoise_ptr_t p_denoise)	// 降噪初始设置
{
  //int i, x0, y0, x1, y1, x2, y2, x3, y3;
  //int a, b, c, d, e, f, delta;
  int i;

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
  p_denoise->sensitiv0 = 3;    	// 降低降噪强度, 提升清晰度
  p_denoise->sensitiv1 = 3;
  
  p_denoise->sel_3d_table = 3;		// 3的降噪效果较好
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


// 20170227 增加场景的解析度拉伸, 增加通透度, 增加低照度场景的解析度
static const unsigned char imx322_default_resolt_before_20171122[33] = {
 200,  210,  220,  225,  225,  225, 
 225,  225,  225,  225,  225,  225, 
 225,  225,  225,  225,  225,  230, 
 230,  230,  230,  230,  230,  230, 
 230,  230,  230,  230,  230,  225, 
 220,  215,  210,  	
};

static unsigned int imx322_default_resolt[33] = {
 200,  215,  220,  225,  230,  230, 
 235,  235,  235,  235,  235,  235, 
 235,  235,  235,  235,  235,  235, 
 235,  235,  235,  235,  235,  235, 
 235,  235,  235,  235,  235,  230, 
 230,  225,  220,  	
};


static const unsigned int imx322_default_colort[33] = {
   64,     128,    192,    256,    320,    384,    511,    511, 
   511,    511,    511,    511,    511,    511,    511,    511, 
   511,    511,    511,    511,    511,    511,    511,    511, 
   511,    511,    511,    511,    448,    384,    256,    192, 
   128	
};

static void imx322_cmos_isp_eris_init(isp_eris_ptr_t p_eris)			// 宽动态初始设置
{
  //int i, j, x0, y0, x1, y1, x2, y2;
  //int a, b, c, d;
  int i;

  p_eris->enable = 1;
#if ERIS_MANUAL
  p_eris->manual = 1;
#else
  p_eris->manual = 0;
#endif
  p_eris->target = 128;
  p_eris->black = 0;
  // 10bit使用D2~D11, D0~D1固定为0
  if(isp_get_sensor_bit() == ARKN141_ISP_SENSOR_BIT_12)
  	  p_eris->white = 4095;		// 尽可能保留动态范围
  else
	  p_eris->white = 1023;		// 尽可能保留动态范围
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
	  p_eris->resolt[i] = imx322_default_resolt[i];
  }

  
  for (i = 0; i < 33; i++)
  {
    p_eris->colort[i] = imx322_default_colort[i];   	
  }
  

	// ERIS直方图初始设置 
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

static void imx322_cmos_isp_fesp_init(isp_fesp_ptr_t p_fesp)	// 镜头校正, fix-pattern-correction, 坏点去除初始设置
{
	int i;
	p_fesp->Lensshade.enable = 0;
	
  //int i, j, k;
  //int x0, y0, x1, y1, x2, y2, x3, y3;
  //int a, b, c, d, e, f, delta;
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
  
  if( isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
  {
  		p_fesp->Fixpatt.rBlacklevel = 240;
  		p_fesp->Fixpatt.grBlacklevel = 240;
  		p_fesp->Fixpatt.gbBlacklevel = 240;
  		p_fesp->Fixpatt.bBlacklevel = 240;
  }
  else
  {
  		p_fesp->Fixpatt.rBlacklevel = 60;
  		p_fesp->Fixpatt.grBlacklevel = 60;
  		p_fesp->Fixpatt.gbBlacklevel = 60;
  		p_fesp->Fixpatt.bBlacklevel = 60;	  
  }
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
  p_fesp->Badpix.thresh = 19;		// IMX322 实测最低坏点判断阈值
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
  // cross talk的阈值越大, 图像越模糊. 8是一个较合适的值. 
  // 使用3D降噪来去除噪声
  p_fesp->Crosstalk.enable = 1;
  p_fesp->Crosstalk.mode = 1;
  p_fesp->Crosstalk.thresh = 10;
  p_fesp->Crosstalk.snsCgf = 3;		//		值越大, 滤除奇异点的能力越大.
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

#define	SATUATION_OFFSET		64		// 饱和度补偿

static void imx322_cmos_isp_enhance_init (isp_enhance_ptr_t p_enhance)	// 图像增强初始设置
{
  p_enhance->sharp.enable = 1;
  p_enhance->sharp.mode = 0;
  p_enhance->sharp.coring = 0;// 0-7 
  //p_enhance->sharp.strength = 64;//64;//32;//128; 
  //p_enhance->sharp.strength = 32;
  p_enhance->sharp.strength = 255;
  //p_enhance->sharp.strength = 196;
  //p_enhance->sharp.gainmax = 256;
  //p_enhance->sharp.gainmax = 128;		// 20170223 修改为轻微的锐化
  //p_enhance->sharp.gainmax = 144;		// 20170305 微调, 增加一点, 
  //p_enhance->sharp.gainmax = 160;		// 20170803白天路测的视频车牌的识别比第一现场0330稍差, 增加锐化程度改善识别度
  p_enhance->sharp.gainmax = 255;		// 20170805根据20170804路测结果,车牌辨识度较0330稍差, 
													//   通过分析视频, 第一现场(0330)的锐化度较高, 提升锐化增益至256 
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

static void imx322_cmos_isp_ae_init (isp_ae_ptr_t p_ae)		// 自动曝光初始设置
{
	p_ae->histoBand[0] = 0x10;
	p_ae->histoBand[1] = 0x40;
	p_ae->histoBand[2] = 0x80;
	p_ae->histoBand[3] = 0xc0;
	
	// u8_t win_weight[3][3] = {1, 1, 1, 1,  1, 1, 1, 1, 1};
#if IQ_20190321
	p_ae->winWeight[0][0] = 1;
	p_ae->winWeight[0][1] = 1;
	p_ae->winWeight[0][2] = 1;
	p_ae->winWeight[1][0] = 1;
	p_ae->winWeight[1][1] = 1;
	p_ae->winWeight[1][2] = 1;
	p_ae->winWeight[2][0] = 1;
	p_ae->winWeight[2][1] = 1;
	p_ae->winWeight[2][2] = 1;
#else
	p_ae->winWeight[0][0] = 1;
	p_ae->winWeight[0][1] = 2;
	p_ae->winWeight[0][2] = 1;
	p_ae->winWeight[1][0] = 6;
	p_ae->winWeight[1][1] = 14;
	p_ae->winWeight[1][2] = 6;
	p_ae->winWeight[2][0] = 12;
	p_ae->winWeight[2][1] = 15;
	p_ae->winWeight[2][2] = 12;	
#endif
	
	//p_ae->bright_target = 10;
	p_ae->bright_target = AE_BRIGHT_TARGET_DEFAULT;
	p_ae->black_target = 128;
	p_ae->compensation = AE_COMPENSATION_DEFAULT;
}

static void imx322_cmos_isp_sys_init (isp_sys_ptr_t p_sys, isp_param_ptr_t p_isp)		// 系统初始设置 (sensor pixel位数, bayer mode)
{
	p_sys->ispenbale = 1;  
	p_sys->ckpolar = 0; //0
	p_sys->vcpolar = 1;    
	p_sys->hcpolar = 1;     
	p_sys->vmskenable = 0;	// 自动丢帧. 保留,必须设置为0
	p_sys->frameratei = 0; 
	p_sys->framerateo = 0; 	// 保留,必须设置为0
	
#if 0
	p_sys->frameratei = 30;
	p_sys->framerateo = 0; 	// 保留,必须设置为0
	p_sys->vifrasel0 = 0xAAAAAAAA;
	p_sys->vifrasel1 = 0;
	
#else
	p_sys->frameratei = 0;
	p_sys->framerateo = 0; 
	p_sys->vifrasel0 = 0;
	p_sys->vifrasel1 = 0;
#endif
	
	// IN/OUT (60帧/55帧, ), 
#if 0
	p_sys->frameratei = 64;
	p_sys->framerateo = 64; 
	p_sys->vifrasel0 = 0xFFFFFFFF;	// 29
	// p_sys->vifrasel1 = 0x07FFEFFE;	// 25
   p_sys->vifrasel1 = 0xFFFFFFFF;	// 22
#endif
	// IN/OUT (1帧/1帧)
	//  p_sys->frameratei = 0;
	//  p_sys->vifrasel0 = 0x00000000;
	// p_sys->vifrasel1 = 0x00000000;
	
	
	//(0.表示8位 1:表示10位 2:表示12位  3:表示14位)
	//XM_printf("sensor bit: 0:8bit 1:10bit 2:12bit 3:14bit  \n");
	
  	if(isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
		p_sys->sensorbit = ARKN141_ISP_SENSOR_BIT_12;  
	else
		p_sys->sensorbit = ARKN141_ISP_SENSOR_BIT_10;
	
	// 0:RGGB 1:GRBG 2:BGGR 3:GBRG 
	//XM_printf("bayer mode: 0:RGGB 1:GRBG 2:BGGR 3:GBRG  ov9712=2  pp1210 720P=1 1080P=0 \n");
	
	if(IMAGE_H_SZ == 1920 )	// 1080P
		p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;
	else if(IMAGE_H_SZ == 1280 )	// 720P
	{
		// 10bit
		// 根据 zonestridey 的设置值选择 GBRG or RGGB
		//p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_GBRG;
		p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;
	}
	else
		p_sys->bayermode = ARKN141_ISP_RAW_IMAGE_BAYER_MODE_RGGB;
	
	p_sys->imagewidth = p_isp->image_width;
	p_sys->imageheight = p_isp->image_height;
	
	// imagehblank, zonestridex, zonestridey 是以ISP Core的时钟为计数基准, 
	// 计算时首先按照Sensor Pixel Clock时序进行计算, 然后换算到ISP Core Clock,
	
	// FPGA测试时 ISP Core Clock == Sensor Pixel Clock
	//double ISP_CORE_CLOCK = arkn141_get_clks (ARKN141_CLK_ISP);
	//double SENSOR_PIXEL_CLOCK = arkn141_get_clks (ARKN141_CLK_SENSOR_MCLK) * 2;
	// double ratio = 1;//((double)ISP_CORE_CLOCK) / SENSOR_PIXEL_CLOCK;
	//unsigned long ratio = ISP_CORE_CLOCK/SENSOR_PIXEL_CLOCK;
#if IMX323_DCK_SYNC_MODE_ENABLE
	p_sys->sonyif = 0;
#else
	p_sys->sonyif = 1;
#endif
	if(p_sys->imagewidth == 1920 && p_sys->imageheight == 1080)
	{
		unsigned int blank;
		unsigned long ISP_CORE_CLOCK_1K = isp_get_clk()/1000;	//isp clk: 必须> 84M,但不能太大(建议85M)
		unsigned long SENSOR_PIXEL_CLOCK_1K = (isp_get_sensor_mclk()/1000 * 2); //mclk:37.125M左右(倍频(这里x 2)前)
		p_sys->imagehblank = (unsigned int)(1 * 96);		// ISP Core Clock
		// 48 = 16(OB side ignored area) + 24(Ignored area of effective pixel side) + 8(Effective margin for color processing)
		p_sys->zonestridex = (unsigned int)(1 * 48);			
		// 31 	= 6(Dummy for communication) + 1(Frame information line) + 4(OB side ignored area) 
		//		+ 8(Vertical direction effective OB) + 4(Ignored area of effective pixel side) + 8(Effective margin for color processing)
		// p_sys->zonestridey = (unsigned int)(1 * (95-58));
		p_sys->zonestridey = (unsigned int)(1 * 31);
		//p_sys->zonestridey = (unsigned int)(1 * (95-58));
		//  p_sys->zonestridey = (unsigned int)(ratio * (25));		// 忽略 8(Effective margin for color processing)
		
		// imagehblank = ISP消隐周期 = sensor消隐点个数(sensor行像素点个数 - 行有效点个数) * ISP_CORE_CLOCK / SENSOR_PIXEL_CLOCK - 20(ISP逻辑占用周期)
		// ISP处理时间延长
		//p_sys->c = (unsigned int)(1 * 200);		// ISP Core Clock
		// p_sys->imagehblank = (unsigned int)(1 * 160);		// ISP Core Clock
		//p_sys->imagehblank = (unsigned int)(1 * 240);	
		//blank = (1125 * 2 - 48 - 1920) * ratio - 30;
		blank = (1125 * 2 - 48 - 1920)*ISP_CORE_CLOCK_1K/SENSOR_PIXEL_CLOCK_1K - 30;
		// p_sys->imagehblank = (unsigned int)(1 * 280);	// ISP 110MHz
		//p_sys->imagehblank = (unsigned int)(1 * 240);	// ISP 110MHz
		//p_sys->imagehblank = (unsigned int)(1 * 240);	
		p_sys->imagehblank = (unsigned int)(1 * 96);	
		if(p_sys->imagehblank > (unsigned int)blank)
			p_sys->imagehblank = (unsigned int)blank;
		//XM_printf ("max_imagehblank = %d, imageblank = %d\n", (uint32)blank, p_sys->imagehblank);
		
		p_sys->resizebit	= 0;
	}
	else
	{
		p_sys->imagehblank = 96;
		// 48 = 16(OB side ignored area) + 24(Ignored area of effective pixel side) + 8(Effective margin for color processing)
		p_sys->zonestridex = 48;
		// 23 	= 6(Dummy for communication) + 1(Frame information line) + 4(OB side ignored area) 
		//		+ 6(Vertical direction effective OB) + 2(Ignored area of effective pixel side) + 4(Effective margin for color processing)
		p_sys->zonestridey = 23;
		
		p_sys->resizebit	= 0;	// 裁剪低2位
	}
	
	if(p_sys->imagewidth == 1920 && p_sys->imageheight == 1080 && isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
	{
		p_sys->sonysac1 = 0xfff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x800; 
	}
	else if(p_sys->imagewidth == 1920 && p_sys->imageheight == 1080 && isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_10)
	{
		p_sys->sonysac1 = 0xfff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x800; 
		
		p_sys->resizebit	= 0;	// 10bit, 从D2~D11输出. sensor 左对齐12bit输出, 裁剪D0~D1
	}
	else if(p_sys->imagewidth == 1280 && p_sys->imageheight == 720 && isp_get_sensor_bit () == ARKN141_ISP_SENSOR_BIT_12)
	{
		p_sys->sonysac1 = 0xfff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x800; 	  
		
		p_sys->resizebit	= 0;	// 12bit
	}
	else
	{
		// 720p 10bit 30帧/60帧模式, 选择左对齐输出, D2~D11, 将D0~D1裁剪
		p_sys->sonysac1 = 0x3ff;
		p_sys->sonysac2 = 0x000;
		p_sys->sonysac3 = 0x000;
		p_sys->sonysac4 = 0x200;
		
		//p_sys->resizebit	= 0;	// 不裁剪, sensor 右对齐10bit输出
		p_sys->resizebit	= 2;	// 裁剪低2位, sensor 左对齐12bit输出
	}
	
	// 使能 
	p_sys->vmanSftenable = 0; 
	p_sys->vchkIntenable = 1;// 帧开始
	p_sys->pabtIntenable = 1; //点异常
	p_sys->fendIntenable = 1; // 帧完成
	p_sys->fabtIntenable = 1; // 地址异常
	p_sys->babtIntenable = 1; //总线异常
	p_sys->ffiqIntenable = 0;  //快中断
	p_sys->pendIntenable = 1;  //中止 ，如果一帧未完成，会完成该帧
	
	p_sys->infoIntenable = 1;	// 使能ISP的曝光统计完成中断
	
	// 设置 场 
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
	p_sys->ffiqIntdelay = 4;// 快中断 
	p_sys->fendStaid = 0;
	p_sys->infoStadone = 0;
	if(isp_get_work_mode() == ISP_WORK_MODE_NORMAL)
	{
		p_sys->debugmode  = 0;
		p_sys->testenable = 0; // 开启dram测试模式  
		p_sys->rawmenable = 0; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
#if ISP_3D_DENOISE_SUPPORT
		p_sys->refenable  = 1; // 1;3D 参考帧开启 0:关闭 
#else
		p_sys->refenable  = 0; // 1;3D 参考帧开启 0:关闭 
#endif
	}
	else if(isp_get_work_mode() == ISP_WORK_MODE_RAW)
	{
		// RAW写出会占用3D参考帧通道
		p_sys->debugmode  = 1;
		p_sys->testenable = 0; // 开启dram测试模式  
		p_sys->rawmenable = 1; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
		p_sys->refenable  = 0; // 1;3D 参考帧开启 0:关闭 
	}
	else if(isp_get_work_mode() == ISP_WORK_MODE_AUTOTEST)
	{
		p_sys->debugmode  = 1;
		p_sys->testenable = 1; // 开启dram测试模式  
		p_sys->rawmenable = 0; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
		p_sys->refenable  = 1; // 1;3D 参考帧开启 0:关闭 
									  // 使能参考帧 (DRAM测试模式使用REFBUF指向的数据为RAW数据)
	}
	else
	{
		p_sys->debugmode  = 0;
		p_sys->testenable = 0; // 开启dram测试模式  
		p_sys->rawmenable = 0; // 1 允许RAW写出
		p_sys->yuvenable  = 1; // 0:关掉数据输出  1:打开
#if ISP_3D_DENOISE_SUPPORT
		p_sys->refenable  = 1; // 1;3D 参考帧开启 0:关闭 	
#else
		p_sys->refenable  = 0; // 1;3D 参考帧开启 0:关闭
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
	p_sys->testenable = 0; //开启dram测试模式  
	p_sys->rawmenable = 0; 
	p_sys->yuvenable  = 1;//0:关掉数据输出  1:打开
	p_sys->refenable  = 1;//1;  
#else // catch raw
	p_sys->debugmode  = 0;
	p_sys->testenable = 0; //开启dram测试模式  
	p_sys->rawmenable = 0; 
	p_sys->yuvenable  = 1;//0:关掉数据输出  
	p_sys->refenable  = 1;     
#endif
*/
	p_sys->yuvformat = isp_get_video_format ();  //0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 
   //XM_printf("0：y_uv420 1:y_uv422 2:yuv420 3:yuv422 \n");
	//p_sys->dmalock = 2;   //总线锁使能 2:使能  其他数值为关闭
   p_sys->dmalock = 0;    	//	总线锁使能 2:使能  其他数值为关闭
	//		总线锁禁止会减少H264的编码时间
	p_sys->hstride = p_isp->image_stride; //图像跨度 16字节倍数
	p_sys->refaddr = p_isp->ref_addr;    //参考帧地址
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

#if 0
// ISP运行设置
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
	{  1125,   16,		13		}
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
	
	// 匹配
	if(inttime == awb_polyline_tbl[i].inttime)
	{
		memcpy (awb_tbl, &awb_polyline_tbl[i], sizeof(isp_awb_polyline_tbl));
		return;
	}
	
	// 边界
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
static void imx322_cmos_isp_awb_run (isp_awb_ptr_t p_awb)
{
	return;
#if 0
	//isp_awb_info_read (p_awb);	
	unsigned int data0;
	unsigned int inttime;
	isp_awb_polyline_tbl awb_tbl;
	inttime = isp_exposure.cmos_inttime.exposure_ashort;
	// 
	awb_match_inttime ((int)inttime, &awb_tbl);
	p_awb->black = (unsigned char)awb_tbl.black;
	//p_awb->jitter = (unsigned char)awb_tbl.jitter;
	/*
	data0 	= ((p_awb->enable  & 0x01) <<  0) 
				| ((p_awb->mode    & 0x03) <<  1) 	// bit1-bit2     mode 
				// 0: unite gray white average 
				//	1: unite color temperature average  
				//	2: zone color temperature Weight
				| ((p_awb->manual  & 0x01) <<  3) 	// bit3  manual, 0: auto awb  1: mannual awb
				| ((p_awb->black   & 0xFF) <<  8) 
				| ((p_awb->white   & 0xFF) << 16)
				;
	Gem_write ((GEM_AWB0_BASE+0x00), data0);
	*/
#endif
}
#endif

static const isp_demosaic_polyline_tbl demosaic_polyline_tbl[] = {
	//  gain    		demk

	// 3D 关闭
	{	  1,           96   },
	{	  8,           112  },
	{	  32,          144  },
	{	  64,          256  },
	{	  100,         256 },
	{	  600,         256},
#if IQ_20190321
	// 20181030 根据晚上路测结果，适当增加解析度
	//{	  1*CMOS_STD_INTTIME,	  	112 },		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	//{	  8*CMOS_STD_INTTIME,	   60	},		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	//{    20*CMOS_STD_INTTIME,     20   },		// 参考 f:\路测视频\20170114晚上\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
	{	  1*CMOS_STD_INTTIME,	  	240 },		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{	  8*CMOS_STD_INTTIME,	   128	},		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{    20*CMOS_STD_INTTIME,     64   },		// 参考 f:\路测视频\20170114晚上\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
								//		兼顾解析度与噪声
	{    35*CMOS_STD_INTTIME,     8    },		// 较大数字增益
	{    64*CMOS_STD_INTTIME,     4    },		// 较大数字增益
	//{    128*CMOS_STD_INTTIME,    2    },		// 较大数字增益
#else
	{	  1*CMOS_STD_INTTIME,	  	210 },		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{	  8*CMOS_STD_INTTIME,	   80	},		// 仅开启模拟增益时, 此时使能较大的demk值, 保持较好的解析度, 同时噪声较低
	{    20*CMOS_STD_INTTIME,     32   },		// 参考 f:\路测视频\20170114晚上\RAW\20170114\212128\212128_21212841_ISP_DEMK_064.PNG
								//		兼顾解析度与噪声
	{    35*CMOS_STD_INTTIME,     8    },		// 较大数字增益
	{    64*CMOS_STD_INTTIME,     4    },		// 较大数字增益

#endif
};

static const isp_demosaic_polyline_tbl *imx322_cmos_isp_get_demosaic_table (int *tbl_count)
{
	*tbl_count = sizeof(demosaic_polyline_tbl)/sizeof(demosaic_polyline_tbl[0]);
	return demosaic_polyline_tbl;
}


#if IQ_20190321

static const isp_sharp_polyline_tbl imx322_sharp_polyline_tbl[] = {
	//  inttime           strength    gainmax
	{   3,                64,        256	   },
	{   1125,             64,        256		},
	// 20181030 根据晚上的路测结果，适当增加锐度
	//{   1125*32,          48,        64      }
	{   1125*32,          56,        128      }
};

#elif IQ_20190323
static const isp_sharp_polyline_tbl imx322_sharp_polyline_tbl[] = {
	//  inttime           strength    gainmax
	{   3,                64,        256	   },
	{   1125,             64,        256		},
	// 20181030 根据晚上的路测结果，适当增加锐度
	//{   1125*32,          48,        64      }
	// 20190321 对比联咏的样机，夜晚噪声偏大，降低锐化强度，降低噪声
	{   1125*32,          32,        96      }
};

#elif SENSOR_IMX323
/*
	// 20191022 适当增加白天的锐度
static const isp_sharp_polyline_tbl imx322_sharp_polyline_tbl[] = {
	//  inttime           strength    gainmax
	{   3,                64,        230	   },
	{   1125,             64,        230		},
	// 20181030 根据晚上的路测结果，适当增加锐度
	//{   1125*32,          48,        64      }
	// 20190321 对比联咏的样机，夜晚噪声偏大，降低锐化强度，降低噪声
	{   1125*32,          32,        96      }
};
*/

// 20191024 根据路测结果,地面噪声比海思大, 稍微降低锐化强度,减少噪声
static const isp_sharp_polyline_tbl imx322_sharp_polyline_tbl[] = {
	//  inttime           strength    gainmax
	{   3,                64,        216	   },
	{   1125,             64,        216		},
	// 20181030 根据晚上的路测结果，适当增加锐度
	//{   1125*32,          48,        64      }
	// 20190321 对比联咏的样机，夜晚噪声偏大，降低锐化强度，降低噪声
	{   1125*32,          32,        96      }
};
#else

static const isp_sharp_polyline_tbl imx322_sharp_polyline_tbl[] = {
	//  inttime           strength    gainmax
	{   3,                64,        216	   },
	{   1125,             64,        216		},
	// 20181030 根据晚上的路测结果，适当增加锐度
	//{   1125*32,          48,        64      }
	// 20190321 对比联咏的样机，夜晚噪声偏大，降低锐化强度，降低噪声
	{   1125*32,          32,        96      }
};

#endif

static const isp_sharp_polyline_tbl *imx322_cmos_isp_get_sharp_table (int *tbl_count)
{
	*tbl_count = sizeof(imx322_sharp_polyline_tbl)/sizeof(imx322_sharp_polyline_tbl[0]);
	return imx322_sharp_polyline_tbl;
}


#if SENSOR_IMX323
	// 20191024 根据路测结果,地面噪声比海思大, 稍微增加降噪强度,减少噪声
static const isp_denoise_inttime_polyline_tbl denoise_inttime_polyline_tbl[] = {
// inttime_gain y_0  u_0  v_0   y_1  u_1  v_1   y_2   u_2  v_2
	{
		1,         6,  7,   7,     6,   7,   7,     3,   3,   3,
		//1,         5,  3,   3,     5,   3,   3,     4,   3,   3,
	} ,

	// 20170305 将2D的降噪强度减1(-1), 保留更多的细节
	{
		12,        6,  7,   7,     6,   7,   7,     3,   3,   3,
		//32,        5,  4,   4,     5,   4,   4,     4,   3,   3,
	} ,
	{
		128,       6,  8,   8,     6,   8,   8,     3,   3,   3,
		//128,       4,  4,   4,     4,   4,   4,     4,   3,   3,
	} ,

	// e:\proj\ARKN141\test\ISP调试\20170202
	{
		1073,      7,   9,   9,    7,   9,   9,    4,   3,   3,
	},

	{
		CMOS_STD_INTTIME*2,    9,   10,   10,     9,   10,   10,    5,   6,  6,
	},
	{
		//CMOS_STD_INTTIME*4,    10,   10,  10,   10,  10,   10,    8,   8,  8,
		CMOS_STD_INTTIME*4,    11,   14,  14,   11,  14,   14,    8,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   16,  20,  20,    16,  20,  20,   12,  12, 12,
		//CMOS_STD_INTTIME*12,   12,  14,  14,    12,  14,  14,   12,  12, 12,
	} ,
	{
		CMOS_STD_INTTIME*14,   18,  22,  22,    18,  22,  22,   13,  13, 13,
		//CMOS_STD_INTTIME*14,   14,  16,  16,    14,  16,  16,   13,  13, 13,
	} ,
	{
		CMOS_STD_INTTIME*20,   28,  32,  32,    28,  32,  32,   13,  13, 13,
		//CMOS_STD_INTTIME*20,   22,  25,  25,    22,  25,  25,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*40,   44,  50,  50,    44,  50,  50,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*64,   70, 78, 78,      70, 78, 78,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*128,  144, 160, 160,  144, 160, 160,   8,  9, 9,
	},
	{
		CMOS_STD_INTTIME*177,  220, 230, 230,   223, 230, 230,   13,  13, 13,
	},


};
#else
//  ******* 无3D降噪   ******

// 参考 ISP调试\2D降噪\14503843\, 强光照环境下, 曝光时间短. 为了尽可能保持画面的细节,需要减轻降噪的程度.
// 	inttime_gain = 12 配置3,4,4,3,4,4,可以较好的降噪(地面噪声)及保持树的细节
static const isp_denoise_inttime_polyline_tbl denoise_inttime_polyline_tbl[] = {
// inttime_gain y_0  u_0  v_0   y_1  u_1  v_1   y_2   u_2  v_2
	{  
		1,         5,  6,   6,     5,   6,   6,     3,   3,   3,
		//1,         5,  3,   3,     5,   3,   3,     4,   3,   3,
	} ,
	
	// 20170305 将2D的降噪强度减1(-1), 保留更多的细节
	{  
		12,        5,  6,   6,     5,   6,   6,     3,   3,   3,
		//32,        5,  4,   4,     5,   4,   4,     4,   3,   3,
	} ,	
	{  
		128,        5,  6,   6,     5,   6,   6,     3,   3,   3,
		//128,       4,  4,   4,     4,   4,   4,     4,   3,   3,
	} ,	
	
	// e:\proj\ARKN141\test\ISP调试\20170202
	{
		1073,      6,   7,   7,    6,   7,   7,    4,   3,   3,
	},
	
#if IQ_20190321
	// 20170227
	// 低亮度场景关闭gamma(对比度拉伸), 减小光晕现象.
	// gamma关闭后, 噪声的电平(拉伸)也跟随降低. 此时可降低低亮度场景下的降噪程度
	// 20181030 根据晚上路测结果，适当降低降噪强度
	{
		CMOS_STD_INTTIME*2,    7,   8,   8,     7,   8,   8,    5,   6,  6,
	},
	{
		//CMOS_STD_INTTIME*4,    10,   10,  10,   10,  10,   10,    8,   8,  8,
		CMOS_STD_INTTIME*4,    8,   9,  9,   8,  9,   9,    8,   8,  8,
	} ,
	{
		//CMOS_STD_INTTIME*12,   14,  16,  16,    14,  16,  16,   12,  12, 12,
		CMOS_STD_INTTIME*12,   12,  14,  14,    12,  14,  14,   12,  12, 12,
	} ,
	{
		//CMOS_STD_INTTIME*14,   16,  18,  18,    16,  18,  18,   13,  13, 13,
		CMOS_STD_INTTIME*14,   14,  16,  16,    14,  16,  16,   13,  13, 13,
	} ,
	{
		//CMOS_STD_INTTIME*20,   27,  29,  29,    27,  29,  29,   13,  13, 13,
		CMOS_STD_INTTIME*20,   22,  25,  25,    22,  25,  25,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*40,   44,  50,  50,    44,  50,  50,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*64,   70, 78, 78,      70, 78, 78,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*128,  144, 160, 160,  144, 160, 160,   8,  9, 9,
	},
	{
		CMOS_STD_INTTIME*177,  220, 230, 230,   223, 230, 230,   13,  13, 13,
	},
#else

	{
		CMOS_STD_INTTIME*2,    8,   9,   9,     8,   9,   9,    5,   6,  6,
	},
	{
		//CMOS_STD_INTTIME*4,    10,   10,  10,   10,  10,   10,    8,   8,  8,
		CMOS_STD_INTTIME*4,    10,   12,  12,   10,  12,   12,    8,   8,  8,
	} ,
	{
		CMOS_STD_INTTIME*12,   16,  20,  20,    16,  20,  20,   12,  12, 12,
		//CMOS_STD_INTTIME*12,   12,  14,  14,    12,  14,  14,   12,  12, 12,
	} ,
	{
		CMOS_STD_INTTIME*14,   18,  22,  22,    18,  22,  22,   13,  13, 13,
		//CMOS_STD_INTTIME*14,   14,  16,  16,    14,  16,  16,   13,  13, 13,
	} ,
	{
		CMOS_STD_INTTIME*20,   28,  32,  32,    28,  32,  32,   13,  13, 13,
		//CMOS_STD_INTTIME*20,   22,  25,  25,    22,  25,  25,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*40,   44,  50,  50,    44,  50,  50,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*64,   70, 78, 78,      70, 78, 78,   13,  13, 13,
	},
	{
		CMOS_STD_INTTIME*128,  144, 160, 160,  144, 160, 160,   8,  9, 9,
	},
	{
		CMOS_STD_INTTIME*177,  220, 230, 230,   223, 230, 230,   13,  13, 13,
	},
#endif
#endif

static const isp_denoise_inttime_polyline_tbl *imx322_cmos_isp_get_denoise_table (int *tbl_count)
{
	*tbl_count = sizeof(denoise_inttime_polyline_tbl)/sizeof(denoise_inttime_polyline_tbl[0]);
	return (isp_denoise_inttime_polyline_tbl *)denoise_inttime_polyline_tbl;
}


// 为了减少低照度场景的噪声, 最小化eris的拉伸强度

static const isp_eris_polyline_tbl eris_polyline_tbl_old[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.95 * 512),  (int)(0.75 * 512) },
	{	5,		   144, (int)(0.96 * 512),  (int)(0.75 * 512) },
	{	32,	   196, (int)(0.97 * 512),  (int)(0.75 * 512) },
	
//	{	1,		   112,  (int)(0.8 * 512),  (int)(0.7 * 512) },
//	{	5,		   144, (int)(0.8 * 512),  (int)(0.7 * 512) },
//	{	32,	   196, (int)(0.85 * 512),  (int)(0.7 * 512) },
	// 20170215 短曝光场景(1, 5, 32)应稍微降低增益(128, 192, 256 --> 96, 144, 208), 减轻高层建筑顶端发白的情况
	
	{  80,	   240, (int)(0.99 * 512),  (int)(0.8 * 512) },
	{  400,	   240, (int)(0.99 * 512),  (int)(0.8 * 512) },
	{	700,   	240, (int)(0.99 * 512),  (int)(0.8 * 512) },
	{	820,	   160, (int)(0.98 * 512),  (int)(0.8 * 512) },
	{	900,	   112, (int)(0.98 * 512),  (int)(0.8 * 512) },
	{	1023,	   80,  (int)(0.98 * 512),  (int)(0.75 * 512) },

	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	{	CMOS_STD_INTTIME,	   64,  (int)(0.98 * 512),  (int)(0.75 * 512) },
	{	CMOS_STD_INTTIME*2,	48,  (int)(0.98 * 512),  (int)(0.75 * 512) },
	{	CMOS_STD_INTTIME*4,	40,  (int)(0.98 * 512),  (int)(0.6 * 512) },
	
	// 20170223 降低eris增益值, 减弱光晕现象 (256 --> 176)
	//{	CMOS_STD_INTTIME*10,	256,  (int)(0.4 * 512),  (int)(0.4 * 512) },
//	{	CMOS_STD_INTTIME*10,	176,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	// 20170223 降低eris增益值, 减弱光晕现象 (256 --> 128)
	//{	CMOS_STD_INTTIME*10,	128,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	// 20170226 降低eris增益值为8, 降低光晕现象
	{	CMOS_STD_INTTIME*10,	32,   (int)(0.90 * 512),  (int)(0.5 * 512) },
	
	{	CMOS_STD_INTTIME*12,	32,   (int)(0.80 * 512),  (int)(0.5 * 512) },
	// 20170217 
	// 夜晚的场景基本是最大物理增益开启, 实测时光晕较大, 前车的车牌因为增益过大, 车牌位置发白的情况较多.
	// 适当降低最大物理增益时的eris增益值(256 --> 176), 增加场景的对比度, 减弱光晕现象, 缓解车牌区域发白的情况.
	// 20170223 降低eris增益值, 减弱光晕现象 (176 --> 128)
	//{	CMOS_STD_INTTIME*15,	176,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*15,	128,  (int)(0.4 * 512),  (int)(0.4 * 512) },
	// 20170226 降低eris增益值为8, 降低光晕现象
	{	CMOS_STD_INTTIME*15,	32,   (int)(0.80 * 512),  (int)(0.4 * 512) },
	
	{	CMOS_STD_INTTIME*35,		32,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*60,		32,   (int)(0.50 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*128,	32,   (int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*177,	32,   (int)(0.20 * 512),  (int)(0.2 * 512) },
};

// 20190319之前的版本
static const isp_eris_polyline_tbl eris_polyline_tbl_20190319[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.90 * 512),  (int)(0.75 * 512) },
//	{	5,		   144, (int)(0.95 * 512),  (int)(0.98 * 512) },
//	{	32,	   196, (int)(0.98 * 512),  (int)(1.00 * 512) },
//	{  80,	   240, (int)(1.00 * 512),  (int)(1.20 * 512) },
	{	5,		   112, (int)(0.95 * 512),  (int)(0.98 * 512) },
	{	32,	   128, (int)(0.98 * 512),  (int)(1.00 * 512) },
	{  80,	   240, (int)(1.00 * 512),  (int)(1.00 * 512) },
	{  400,	   240, (int)(1.04 * 512),  (int)(1.0 * 512) },
	{	700,   	240, (int)(0.97 * 512),  (int)(0.85 * 512) },
	{	820,	   240, (int)(0.97 * 512),  (int)(0.8 * 512) },
	{	900,	   256, (int)(0.97 * 512),  (int)(0.75 * 512) },
	{	1023,	   256,  (int)(0.97 * 512),  (int)(0.75 * 512) },


	// 尽量增大车牌正确曝光的机会 


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },
	
	// 20181030 根据晚上路测结果，适当增加解析度
	{	CMOS_STD_INTTIME,	   280,  (int)(0.95 * 512),  (int)(0.75 * 512) },
	{	CMOS_STD_INTTIME*2,	280,  (int)(0.92 * 512),  (int)(0.70 * 512) },
	{	CMOS_STD_INTTIME*10,	280,   (int)(0.90 * 512),  (int)(0.4 * 512) },	
	{	CMOS_STD_INTTIME*12,	300,   (int)(0.88 * 512),  (int)(0.4 * 512) },
	
	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		320,   (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*60,		512,    (int)(0.20 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*128,	512,    (int)(0.15 * 512),  (int)(0.4 * 512) },
};

#if IQ_20190321
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.90 * 512),  (int)(0.65 * 512) },
//	{	5,		   144, (int)(0.95 * 512),  (int)(0.98 * 512) },
//	{	32,	   196, (int)(0.98 * 512),  (int)(1.00 * 512) },
//	{  80,	   240, (int)(1.00 * 512),  (int)(1.20 * 512) },
	{	5,		   112, (int)(0.95 * 512),  (int)(0.80 * 512) },
	{	32,	   128, (int)(0.98 * 512),  (int)(0.85 * 512) },
	{  80,	   240, (int)(1.00 * 512),  (int)(0.85 * 512) },
	{  400,	   240, (int)(1.04 * 512),  (int)(0.85 * 512) },
	{	700,   	240, (int)(0.97 * 512),  (int)(0.55 * 512) },
	{	820,	   240, (int)(0.97 * 512),  (int)(0.5 * 512) },
	{	900,	   256, (int)(0.97 * 512),  (int)(0.45 * 512) },
	{	1023,	   256,  (int)(0.97 * 512),  (int)(0.45 * 512) },


	// 尽量增大车牌正确曝光的机会 


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },
	
	// 20181030 根据晚上路测结果，适当增加解析度
	{	CMOS_STD_INTTIME,	   280,  (int)(0.95 * 512),  (int)(0.45 * 512) },
	{	CMOS_STD_INTTIME*2,	280,  (int)(0.92 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*10,	280,   (int)(0.90 * 512),  (int)(0.2 * 512) },	
	{	CMOS_STD_INTTIME*12,	300,   (int)(0.88 * 512),  (int)(0.2 * 512) },
	
	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		320,   (int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*60,		512,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	512,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};

#elif IQ_20190322	// 20190322 白天测试，色彩偏黄
// 20190321 晚上路测，噪声偏大
// 20190322 白天测试，色彩偏黄
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.90 * 512),  (int)(0.65 * 512) },
//	{	5,		   144, (int)(0.95 * 512),  (int)(0.98 * 512) },
//	{	32,	   196, (int)(0.98 * 512),  (int)(1.00 * 512) },
//	{  80,	   240, (int)(1.00 * 512),  (int)(1.20 * 512) },
	{	5,		   112, (int)(0.95 * 512),  (int)(0.80 * 512) },
	{	32,	   128, (int)(0.98 * 512),  (int)(0.85 * 512) },
	{  80,	   240, (int)(1.00 * 512),  (int)(0.85 * 512) },
	{  400,	   240, (int)(1.04 * 512),  (int)(0.85 * 512) },
	{	700,   	240, (int)(0.97 * 512),  (int)(0.55 * 512) },
	{	820,	   240, (int)(0.97 * 512),  (int)(0.5 * 512) },
	{	900,	   256, (int)(0.97 * 512),  (int)(0.45 * 512) },
	{	1023,	   256,  (int)(0.97 * 512),  (int)(0.45 * 512) },


	// 尽量增大车牌正确曝光的机会 


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },
	
	// 路测噪声偏大，降低亮度增益，抑制噪声
	{	CMOS_STD_INTTIME,	   240,  (int)(0.95 * 512),  (int)(0.45 * 512) },
	{	CMOS_STD_INTTIME*2,	240,  (int)(0.92 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*10,	240,   (int)(0.90 * 512),  (int)(0.2 * 512) },	
	{	CMOS_STD_INTTIME*12,	260,   (int)(0.88 * 512),  (int)(0.2 * 512) },
	
	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		280,   (int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*60,		480,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	480,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};

#elif  IQ_20190323

// 20190323 20190322白天路测，色彩偏黄，降低白天的色彩拉伸强度
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.90 * 512),  (int)(0.65 * 512) },
//	{	5,		   144, (int)(0.95 * 512),  (int)(0.98 * 512) },
//	{	32,	   196, (int)(0.98 * 512),  (int)(1.00 * 512) },
//	{  80,	   240, (int)(1.00 * 512),  (int)(1.20 * 512) },

	// 20190322白天路测，色彩偏黄，降低白天的色彩拉伸强度
//	{	5,		   112, (int)(0.95 * 512),  (int)(0.80 * 512) },
//	{	32,	   128, (int)(0.98 * 512),  (int)(0.85 * 512) },
//	{  80,	   240, (int)(1.00 * 512),  (int)(0.85 * 512) },
//	{  400,	   240, (int)(1.04 * 512),  (int)(0.85 * 512) },
//	{	700,   	240, (int)(0.97 * 512),  (int)(0.55 * 512) },
//	{	820,	   240, (int)(0.97 * 512),  (int)(0.5 * 512) },
	{	5,		   112, (int)(0.95 * 512),  (int)(0.65 * 512) },
	{	32,	   128, (int)(0.98 * 512),  (int)(0.70 * 512) },
	{  80,	   240, (int)(1.00 * 512),  (int)(0.70 * 512) },
	{  400,	   240, (int)(1.04 * 512),  (int)(0.70 * 512) },
	{	700,   	240, (int)(0.97 * 512),  (int)(0.5 * 512) },
	{	820,	   240, (int)(0.97 * 512),  (int)(0.5 * 512) },
	{	900,	   256, (int)(0.97 * 512),  (int)(0.45 * 512) },
	{	1023,	   256,  (int)(0.97 * 512),  (int)(0.45 * 512) },


	// 尽量增大车牌正确曝光的机会 


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },
	
	// 路测噪声偏大，降低亮度增益，抑制噪声
	{	CMOS_STD_INTTIME,	   240,  (int)(0.95 * 512),  (int)(0.45 * 512) },
	{	CMOS_STD_INTTIME*2,	240,  (int)(0.92 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*10,	240,   (int)(0.90 * 512),  (int)(0.2 * 512) },	
	{	CMOS_STD_INTTIME*12,	260,   (int)(0.88 * 512),  (int)(0.2 * 512) },
	
	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		280,   (int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*60,		480,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	480,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};

#elif  SENSOR_IMX323
/*****
	// 20191021 增强白天颜色的饱和度
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   96, (int)(0.90 * 512),  (int)(0.65 * 512) },

	// 20190325白天路测，色彩偏黄，降低白天的色彩拉伸强度
	{	5,		   96, (int)(0.94 * 512),  (int)(0.65 * 512) },
	{	32,	   112, (int)(0.97 * 512),  (int)(0.70 * 512) },
	{  80,	   212, (int)(0.98 * 512),  (int)(0.70 * 512) },
	{  400,	   212, (int)(0.99 * 512),  (int)(0.70 * 512) },
	{	700,   	212, (int)(0.97 * 512),  (int)(0.70 * 512) },
	{	820,	   240, (int)(0.96 * 512),  (int)(0.70 * 512) },
	{	900,	   256, (int)(0.96 * 512),  (int)(0.65 * 512) },
	{	1023,	   256,  (int)(0.96 * 512),  (int)(0.60 * 512) },


	// 尽量增大车牌正确曝光的机会 


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },
	
	// 路测噪声偏大，降低亮度增益，抑制噪声
	{	CMOS_STD_INTTIME,	   240,  (int)(0.94 * 512),  (int)(0.60 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.91 * 512),  (int)(0.60 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.80 * 512),  (int)(0.50 * 512) },	
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.70 * 512),  (int)(0.45 * 512) },
	
	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		160,   (int)(0.35 * 512),  (int)(0.3 * 512) },
	{	CMOS_STD_INTTIME*60,		144,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	144,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};
******/
/*****
// 20191022 增强白天颜色的饱和度
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {

	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   96, (int)(0.90 * 512),  (int)(0.85 * 512) },

	// 20190325白天路测，色彩偏黄，降低白天的色彩拉伸强度
	{	5,		   96, (int)(0.94 * 512),  (int)(0.85 * 512) },
	{	32,	   112, (int)(0.97 * 512),  (int)(0.90 * 512) },
	{  80,	   212, (int)(0.98 * 512),  (int)(0.90 * 512) },
	{  400,	   212, (int)(0.99 * 512),  (int)(0.90 * 512) },
	{	700,   	212, (int)(0.97 * 512),  (int)(0.90 * 512) },
	{	820,	   240, (int)(0.96 * 512),  (int)(0.90 * 512) },
	{	900,	   256, (int)(0.96 * 512),  (int)(0.85 * 512) },
	{	1023,	   256,  (int)(0.96 * 512),  (int)(0.80 * 512) },


	// 尽量增大车牌正确曝光的机会


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },

	// 路测噪声偏大，降低亮度增益，抑制噪声
	{	CMOS_STD_INTTIME,	   240,  (int)(0.94 * 512),  (int)(0.80 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.91 * 512),  (int)(0.80 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.80 * 512),  (int)(0.70 * 512) },
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.70 * 512),  (int)(0.65 * 512) },

	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		160,   (int)(0.35 * 512),  (int)(0.3 * 512) },
	{	CMOS_STD_INTTIME*60,		144,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	144,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};
*****/
// 20191024 根据路测结果
// 1) 地面噪声比海思大, 降低解析度拉伸强度,减少噪声
// 2) 天空存在发白现象, 降低亮度增益, 改善亮度拉伸过曝的现象
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {

	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   90, (int)(0.89 * 512),  (int)(0.85 * 512) },

	// 20190325白天路测，色彩偏黄，降低白天的色彩拉伸强度
	{	5,		   90, (int)(0.93 * 512),  (int)(0.85 * 512) },
	{	32,	   106, (int)(0.96 * 512),  (int)(0.90 * 512) },
	{  80,	   200, (int)(0.97 * 512),  (int)(0.90 * 512) },
	{  400,	   200, (int)(0.98 * 512),  (int)(0.90 * 512) },
	{	700,   	200, (int)(0.96 * 512),  (int)(0.90 * 512) },
	{	820,	   230, (int)(0.95 * 512),  (int)(0.90 * 512) },
	{	900,	   240, (int)(0.95 * 512),  (int)(0.85 * 512) },
	{	1023,	   240,  (int)(0.95 * 512),  (int)(0.80 * 512) },


	// 尽量增大车牌正确曝光的机会


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },

	// 路测噪声偏大，降低亮度增益，抑制噪声
	{	CMOS_STD_INTTIME,	   240,  (int)(0.93 * 512),  (int)(0.80 * 512) },
	{	CMOS_STD_INTTIME*2,	214,  (int)(0.90 * 512),  (int)(0.80 * 512) },
	{	CMOS_STD_INTTIME*10,	210,   (int)(0.80 * 512),  (int)(0.70 * 512) },
	{	CMOS_STD_INTTIME*12,	210,   (int)(0.70 * 512),  (int)(0.65 * 512) },

	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		160,   (int)(0.35 * 512),  (int)(0.3 * 512) },
	{	CMOS_STD_INTTIME*60,		144,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	144,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};
#else

// 20190326 20190325白天路测，色彩偏黄，降低白天的色彩拉伸强度
static const isp_eris_polyline_tbl eris_polyline_tbl[] = {
	
	// 20170305 增加短曝光场景的拉伸强度, 改善解析度及颜色
	{	1,		   112, (int)(0.90 * 512),  (int)(0.5 * 512) },

	// 20190325白天路测，色彩偏黄，降低白天的色彩拉伸强度
	{	5,		   112, (int)(0.94 * 512),  (int)(0.5 * 512) },
	{	32,	   128, (int)(0.97 * 512),  (int)(0.55 * 512) },
	{  80,	   240, (int)(0.98 * 512),  (int)(0.55 * 512) },
	{  400,	   240, (int)(0.99 * 512),  (int)(0.55 * 512) },
	{	700,   	240, (int)(0.97 * 512),  (int)(0.5 * 512) },
	{	820,	   240, (int)(0.96 * 512),  (int)(0.5 * 512) },
	{	900,	   256, (int)(0.96 * 512),  (int)(0.45 * 512) },
	{	1023,	   256,  (int)(0.96 * 512),  (int)(0.40 * 512) },


	// 尽量增大车牌正确曝光的机会 


	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	//{	CMOS_STD_INTTIME,	   280,  (int)(0.85 * 512),  (int)(0.75 * 512) },
	//{	CMOS_STD_INTTIME*2,	280,  (int)(0.80 * 512),  (int)(0.70 * 512) },
	//{	CMOS_STD_INTTIME*10,	280,   (int)(0.70 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*12,	300,   (int)(0.65 * 512),  (int)(0.4 * 512) },
	
	// 路测噪声偏大，降低亮度增益，抑制噪声
	{	CMOS_STD_INTTIME,	   240,  (int)(0.94 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*2,	240,  (int)(0.91 * 512),  (int)(0.40 * 512) },
	{	CMOS_STD_INTTIME*10,	240,   (int)(0.90 * 512),  (int)(0.2 * 512) },	
	{	CMOS_STD_INTTIME*12,	260,   (int)(0.88 * 512),  (int)(0.2 * 512) },
	
	// 20180915 晚上路测，iso=CMOS_STD_INTTIME*24时噪声偏大，降低拉伸系数
	//{	CMOS_STD_INTTIME*30,		360,   (int)(0.60 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*60,		512,    (int)(0.50 * 512),  (int)(0.4 * 512) },
	//{	CMOS_STD_INTTIME*128,	1023,    (int)(0.40 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*30,		280,   (int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*60,		480,    (int)(0.20 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*128,	480,    (int)(0.15 * 512),  (int)(0.2 * 512) },
};
#endif

static const isp_eris_polyline_tbl *imx322_cmos_isp_get_eris_auto_table(int *tbl_count)
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
	{	1023,	   					128,	64,	(int)(0.98 * 512),  (int)(0.9 * 512) },

	// 20170217 地下车库测试发现, N141墙壁上指引标志的色彩偏暗, 稍微提高暗场景下色彩的比率因子(+0.1)
	{	CMOS_STD_INTTIME,	   	128,  48,	(int)(0.98 * 512),  (int)(0.8 * 512) },
	{	CMOS_STD_INTTIME*4,		128,  32,	(int)(0.98 * 512),  (int)(0.6 * 512) },
	
	{	CMOS_STD_INTTIME*10,		128,  32,	(int)(0.95 * 512),  (int)(0.6 * 512) },
	
	{	CMOS_STD_INTTIME*12,		112,  32,	 (int)(0.85 * 512),  (int)(0.5 * 512) },
	{	CMOS_STD_INTTIME*15,		96,   32,	(int)(0.80 * 512),  (int)(0.5 * 512) },
	
	{	CMOS_STD_INTTIME*35,		96,   32,	(int)(0.60 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*60,		96,   32,	(int)(0.50 * 512),  (int)(0.4 * 512) },
	{	CMOS_STD_INTTIME*128,	96,   32,	(int)(0.40 * 512),  (int)(0.2 * 512) },
	{	CMOS_STD_INTTIME*177,	96,   32,	(int)(0.20 * 512),  (int)(0.2 * 512) },
};

static const isp_eris_man_polyline_tbl *imx322_cmos_isp_get_eris_man_table(int *tbl_count)
{
	*tbl_count = sizeof(eris_man_polyline_tbl)/sizeof(eris_man_polyline_tbl[0]);
	return eris_man_polyline_tbl;
}

// 3D关闭时的参数, 
static const isp_crosstalk_polyline_tbl crosstalk_polyline_tbl[] = {
	//  inttime_gain    	crosstalk
	// 20170217上午及之前使用的版本
	{		50,				2 + 1	 },
	{		100,				3 + 1	 },
	{		161,				3 + 1	 },
	{		242,				3 + 1	 },
	{		1024,		  		3 + 2	 },
	
#if IQ_20190321
	{		CMOS_STD_INTTIME,		  		5	 },
	{		CMOS_STD_INTTIME * 2,	  	8  },
	{		CMOS_STD_INTTIME * 4,	  	10  },
	{		CMOS_STD_INTTIME * 10,	  	12  },
	{  	CMOS_STD_INTTIME * 30,     24  },
	{  	CMOS_STD_INTTIME * 64,     48  },
	{  	CMOS_STD_INTTIME * 128,    64  },
	{  	CMOS_STD_INTTIME * 177,    128  },
#else
	// 20190321 晚上路测，噪声偏大，增加降噪强度
	{		CMOS_STD_INTTIME,		  		5 + 2	 },
	{		CMOS_STD_INTTIME * 2,	  	8 + 2  },
	{		CMOS_STD_INTTIME * 4,	  	10 + 4  },
	{		CMOS_STD_INTTIME * 10,	  	12 + 8  },
	{  	CMOS_STD_INTTIME * 30,     24 + 12  },
	{  	CMOS_STD_INTTIME * 64,     48 + 16  },
	{  	CMOS_STD_INTTIME * 128,    64 + 24  },
	{  	CMOS_STD_INTTIME * 177,    128  },
#endif
};


static const isp_crosstalk_polyline_tbl *imx322_cmos_isp_get_crosstalk_table (int *tbl_count)
{
	*tbl_count = sizeof(crosstalk_polyline_tbl)/sizeof(crosstalk_polyline_tbl[0]);
	return crosstalk_polyline_tbl;
}



//#pragma data_alignment=32
static const isp_enhance_polyline_tbl enhance_polyline_tbl[] = {
	//  inttime   bright  contrast
	//{		3,		   -6,	1024   },
	//{		64,	   -8,	1024 	},
	// 保留短曝光低光处的细节
	{		3,		   -2,	1024	   },
	{		64,	   -2,	1024	 	},
	
	{   512,       -4,	1024    },
	{   800,       -4,	1024    },
	{	 1125,		-4,	1024	},		// 场景光照较弱时, 噪声增加, 相应增加黑电平
	{	 CMOS_STD_INTTIME * 2,	  	-4,	1024},
	// 20190321 夜晚路测，噪声偏大，增大黑电平
	{	 CMOS_STD_INTTIME * 16,	  	-5,	1024},
	
};

static const isp_enhance_polyline_tbl * imx322_cmos_isp_get_enhance_table (int *tbl_count)
{
	*tbl_count = sizeof(enhance_polyline_tbl)/sizeof(enhance_polyline_tbl[0]);
	return enhance_polyline_tbl;
}

// 20190319之前的版本
#if IQ_20190321
static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		  1024+SATUATION_OFFSET	},
	{		10 * CMOS_STD_INTTIME,	  1024+SATUATION_OFFSET	},
	{  	12 * CMOS_STD_INTTIME,     1024   },
	{  	24 * CMOS_STD_INTTIME,     720   },
	{  	32 * CMOS_STD_INTTIME,     512    },
	{  	64 * CMOS_STD_INTTIME,     384    },
};
#elif IQ_20190323
static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		  1024+SATUATION_OFFSET	},
	{		10 * CMOS_STD_INTTIME,	  800	},
	{  	12 * CMOS_STD_INTTIME,     720   },
	{  	24 * CMOS_STD_INTTIME,     512   },
	{  	32 * CMOS_STD_INTTIME,     384    },
	{  	64 * CMOS_STD_INTTIME,     384    },
};

#elif SENSOR_IMX323
	// 20191021 改善白天的饱和度
static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		  1024	},
	{		10 * CMOS_STD_INTTIME,	  900	},
	{  	12 * CMOS_STD_INTTIME,     800   },
	{  	24 * CMOS_STD_INTTIME,     640   },
	{  	32 * CMOS_STD_INTTIME,     512    },
	{  	64 * CMOS_STD_INTTIME,     512    },
};
#else
static const isp_satuation_polyline_tbl satuation_polyline_tbl[] = {
	//  inttime    satuation
	{		1 * CMOS_STD_INTTIME,		  900	},
	{		10 * CMOS_STD_INTTIME,	  800	},
	{  	12 * CMOS_STD_INTTIME,     720   },
	{  	24 * CMOS_STD_INTTIME,     512   },
	{  	32 * CMOS_STD_INTTIME,     384    },
	{  	64 * CMOS_STD_INTTIME,     384    },
};

#endif

static const isp_satuation_polyline_tbl * imx322_cmos_isp_get_satuation_table (int *tbl_count)
{
	*tbl_count = sizeof(satuation_polyline_tbl)/sizeof(satuation_polyline_tbl[0]);
	return satuation_polyline_tbl;
}


static const isp_ae_polyline_tbl ae_polyline_tbl_old_version[] = {
	{	16,							64,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							64,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},	
	{	512,							56,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							32,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME,			28,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME*5/2,	24,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		22,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*11,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*64,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	
};

// 增强白天曝光
// 20171122下午测试， 比较MSTAR及善领, 场景偏暗， 增加曝光
static const isp_ae_polyline_tbl ae_polyline_tbl_20171128[] = {
	{	16,							64,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							64,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},	
	{	512,							56,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							32,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME,			24,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME*5/2,	19,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
	//{	CMOS_STD_INTTIME*11,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*64,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	
};

static const isp_ae_polyline_tbl ae_polyline_tbl_20171113_night[] = {
	{	16,							50,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							46,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	512,							35,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME,			24,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME*5/2,	23,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	//{	CMOS_STD_INTTIME*8,		22,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	//{	CMOS_STD_INTTIME*11,		18,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	{	CMOS_STD_INTTIME*16,		20,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*32,		16,	128,	{5, 5, 5, 5, 5,  5,  5, 5,  5}	},		
	//{	CMOS_STD_INTTIME*64,		16,	128,	{5, 5, 5, 5, 5,  5,  5, 5,  5}	},		
	//{	CMOS_STD_INTTIME*128,	16,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	//{	CMOS_STD_INTTIME*256,	16,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	}		
};

static const isp_ae_polyline_tbl ae_polyline_tbl_20171122[] = {
	{	16,							50,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							46,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	512,							38,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							28,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME,			24,	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME*5/2,	19,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
};

static const isp_ae_polyline_tbl ae_polyline_tbl_[] = {
	{	16,							50,	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							46,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME,			38,	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	CMOS_STD_INTTIME*5/2,	28,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*8,		22,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*11,		18,	128,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	{	CMOS_STD_INTTIME*16,		16,	112,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	{	CMOS_STD_INTTIME*48,		16,	104,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*64,		16,	96,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},		
	{	CMOS_STD_INTTIME*128,	16,	80,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	//{	CMOS_STD_INTTIME*178,	16,	64,	{3, 3, 3, 5, 14, 5, 13, 14, 13}	},	
	
	// 最大178

};

#define	AE_GAIN(x)	((unsigned int)(x*1.0))
// 20180915 
// 1) 晚上车牌识别率较高
// 2) 晚上霓虹灯招牌可以较好识别
// 3) 晚上路边行人可以很好识别
// 20181029 上午，阳光较好
// 1) 建筑物顶部偏白，过曝
static const isp_ae_polyline_tbl ae_polyline_tbl_20181029[] = {
	{	16,							AE_GAIN(38),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(38),	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},	
	{	512,							AE_GAIN(36),	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							AE_GAIN(35),	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(23),	128,	{4, 5, 4, 5, 10, 5, 6,  8,  6 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(17),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
//	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
	//{	CMOS_STD_INTTIME*11,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*64,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};

#if IQ_20190321
// 20181029 修改
// 1) 降低亮场景下的亮度
// 2) 增加暗场景下的亮度(不是很暗的场景)
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	{	16,							AE_GAIN(33),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(33),	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},	
	{	512,							AE_GAIN(33),	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							AE_GAIN(32),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(25),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(20),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(17),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
//	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
	//{	CMOS_STD_INTTIME*11,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*64,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};

#elif  IQ_20190322		// 20190322路测，逆光效果较暗

// 20190321晚上路测，噪声偏大
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	{	16,							AE_GAIN(38),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(38),	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},	
	{	512,							AE_GAIN(36),	128,	{4, 5, 4, 5, 10, 5, 6,  8,   6}	},
	{	878,							AE_GAIN(33),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
//	{	CMOS_STD_INTTIME*11,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},	
	//{	CMOS_STD_INTTIME*11,		18,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*64,		17,	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};

#elif IQ_20190323

// 20190323 20190322路测，1)逆光效果较暗, 增强短曝光下的亮度 2)海岸城晚上路边两侧偏暗
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) 增加短曝光场景下(包括逆光)的亮度
	{	1,								AE_GAIN(46),	128,	{1, 1, 1, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(42),	128,	{1, 1, 1, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(38),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(38),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},	
	{	512,							AE_GAIN(36),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(33),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	
	// 2 增加海岸城晚上路边两侧的亮度
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*4,		AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};

#elif SENSOR_IMX323
/****
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) 增加短曝光场景下(包括逆光)的亮度
	{	1,								AE_GAIN(30),	128,	{2, 3, 2, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(30),	128,	{2, 3, 2, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(30),	128,	{3, 5, 3, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(30),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},	
	{	512,							AE_GAIN(30),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(28),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	
	// 2 增加海岸城晚上路边两侧的亮度
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME,			AE_GAIN(24),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*4,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(16),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};
*****/
/****
// 20191024
// 天空存在发白现象, 降低场景亮度, 改善亮度拉伸过曝的现象
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) 增加短曝光场景下(包括逆光)的亮度
	{	1,								AE_GAIN(27),	128,	{2, 3, 2, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(27),	128,	{2, 3, 2, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(27),	128,	{3, 5, 3, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(27),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},	
	{	512,							AE_GAIN(27),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(26),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},

	// 2 增加海岸城晚上路边两侧的亮度
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(24),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*4,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*8,		AE_GAIN(16),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
};
****/

// 20191030
// 测试发现阴天白天亮度稍微偏暗, 增加场景亮度
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) 增加短曝光场景下(包括逆光)的亮度
	{	1,								AE_GAIN(32),	128,	{2, 3, 2, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(32),	128,	{2, 3, 2, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(32),	128,	{3, 5, 3, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(32),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	512,							AE_GAIN(32),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(29),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},

	// 2 增加海岸城晚上路边两侧的亮度
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME,			AE_GAIN(25),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*4,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*8,		AE_GAIN(16),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
};

#else
static const isp_ae_polyline_tbl ae_polyline_tbl[] = {
	// 1) 增加短曝光场景下(包括逆光)的亮度
	{	1,								AE_GAIN(58),	128,	{1, 1, 1, 2, 5,  2, 12, 15, 12}	},
	{	16,							AE_GAIN(54),	128,	{1, 1, 1, 4, 10, 4, 12, 15, 12}	},
	{	32,							AE_GAIN(50),	128,	{2, 3, 2, 6, 15, 6, 12, 15, 12}	},
	{	64,							AE_GAIN(50),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},	
	{	512,							AE_GAIN(48),	128,	{4, 5, 4, 5, 10, 5, 6,  10,   6}	},
	{	878,							AE_GAIN(42),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	
	// 2 增加海岸城晚上路边两侧的亮度
	//{	CMOS_STD_INTTIME,			AE_GAIN(27),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},
	//{	CMOS_STD_INTTIME*5/2,	AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	//{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME,			AE_GAIN(35),	128,	{3, 4, 3, 5, 7,  5, 7,  7,  7 }	},
	{	CMOS_STD_INTTIME*5/2,	AE_GAIN(28),	128,	{4, 4, 4, 5, 7,  5, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*4,		AE_GAIN(22),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
	{	CMOS_STD_INTTIME*8,		AE_GAIN(18),	128,	{7, 7, 7, 7, 7,  7, 7,  7,  7 }	},		
};

#endif

static const isp_ae_polyline_tbl *imx322_cmos_isp_get_ae_table (int *tbl_count)
{
	*tbl_count = sizeof(ae_polyline_tbl)/sizeof(ae_polyline_tbl[0]);
	return ae_polyline_tbl;
}


//static void imx322_cmos_isp_set_day_night_mode (cmos_gain_ptr_t gain, int day_night)	// day_night = 1, 夜晚增强模式, day_night = 0, 普通模式
//{
//}


u32_t isp_init_cmos_sensor (cmos_sensor_t *cmos_sensor)
{
	memset (cmos_sensor, 0, sizeof(cmos_sensor_t));
	cmos_sensor->cmos_gain_initialize = cmos_gain_initialize;
	cmos_sensor->cmos_max_gain_set = imx322_cmos_max_gain_set;
	cmos_sensor->cmos_max_gain_get = imx322_cmos_max_gain_get;
	
	//cmos_sensor->cmos_gain_update = cmos_gain_update;
	cmos_sensor->cmos_inttime_initialize = cmos_inttime_initialize;
	//cmos_sensor->cmos_inttime_update = cmos_inttime_update;
	cmos_sensor->cmos_inttime_gain_update = cmos_inttime_gain_update;
	cmos_sensor->cmos_inttime_gain_update_manual = cmos_inttime_gain_update_manual;
	cmos_sensor->analog_gain_from_exposure_calculate = analog_gain_from_exposure_calculate;
	cmos_sensor->digital_gain_from_exposure_calculate = NULL;
	cmos_sensor->cmos_get_iso = cmos_get_iso;
	cmos_sensor->cmos_fps_set = cmos_fps_set;
	cmos_sensor->cmos_sensor_set_readout_direction = cmos_sensor_set_readout_direction;
	
	cmos_sensor->cmos_sensor_get_sensor_name = imx322_cmos_sensor_get_sensor_name;
	// sensor初始化
	cmos_sensor->cmos_isp_sensor_init = imx322_isp_sensor_init;
	
	cmos_sensor->cmos_isp_awb_init = imx322_cmos_isp_awb_init;
	cmos_sensor->cmos_isp_colors_init = imx322_cmos_isp_colors_init;
	cmos_sensor->cmos_isp_denoise_init = imx322_cmos_isp_denoise_init;
	cmos_sensor->cmos_isp_eris_init = imx322_cmos_isp_eris_init;
	cmos_sensor->cmos_isp_fesp_init = imx322_cmos_isp_fesp_init;
	cmos_sensor->cmos_isp_enhance_init = imx322_cmos_isp_enhance_init;
	cmos_sensor->cmos_isp_ae_init = imx322_cmos_isp_ae_init;
	cmos_sensor->cmos_isp_sys_init = imx322_cmos_isp_sys_init;

	cmos_sensor->cmos_isp_awb_run = NULL;
	cmos_sensor->cmos_isp_colors_run = NULL;
	cmos_sensor->cmos_isp_denoise_run = NULL;
	cmos_sensor->cmos_isp_eris_run = NULL;
	cmos_sensor->cmos_isp_fesp_run = NULL;
	cmos_sensor->cmos_isp_enhance_run = NULL;
	cmos_sensor->cmos_isp_ae_run = NULL;
	cmos_sensor->cmos_isp_sharp_run = NULL;
	
	cmos_sensor->cmos_isp_get_gamma_table = imx322_cmos_isp_get_gamma_table;
	cmos_sensor->cmos_isp_get_denoise_table = imx322_cmos_isp_get_denoise_table;
	cmos_sensor->cmos_isp_get_eris_auto_table = imx322_cmos_isp_get_eris_auto_table;	// 自动模式
	cmos_sensor->cmos_isp_get_eris_man_table = imx322_cmos_isp_get_eris_man_table;	// 手动模式
	cmos_sensor->cmos_isp_get_crosstalk_table = imx322_cmos_isp_get_crosstalk_table;
	cmos_sensor->cmos_isp_get_fpn_table = NULL;
	cmos_sensor->cmos_isp_get_lsc_table = NULL;
	cmos_sensor->cmos_isp_get_satuation_table = imx322_cmos_isp_get_satuation_table;
	cmos_sensor->cmos_isp_get_enhance_table = imx322_cmos_isp_get_enhance_table;
	cmos_sensor->cmos_isp_get_sharp_table = imx322_cmos_isp_get_sharp_table;
	cmos_sensor->cmos_isp_get_ae_table = imx322_cmos_isp_get_ae_table;
	cmos_sensor->cmos_isp_get_demosaic_table = imx322_cmos_isp_get_demosaic_table;
	
	return 0;
}
EXPORT_SYMBOL(isp_init_cmos_sensor);

