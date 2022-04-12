#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/math64.h>
#include "ark_camera.h"
#include "ark_isp_exposure_cmos.h"
#include "ark_isp_exposure.h"

extern cmos_exposure_t isp_exposure;

static void boundary_min_check_u16 (u16_t * param, u32_t min)
{
	if((*param) < min)
	{
		*param = (u16_t)min;
	}
}

static void boundary_max_check_u16 (u16_t * param, u32_t max)
{
	if((*param) > max)
	{
		*param = (u16_t)max;
	}
}

static void boundary_min_check (u32_t * param, u32_t min)
{
	if((*param) < min)
	{
		*param = min;
	}
}

static void boundary_max_check (u32_t * param, u32_t max)
{
	if((*param) > max)
	{
		*param = max;
	}
}

static void boundaries_check (u32_t * param, u32_t min, u32_t max)
{
	boundary_min_check(param, min);
	boundary_max_check(param, max);
}

u32_t exposure_limit_calculate(
	u16_t exp_lines,
	u32_t max_again,
	u32_t max_dgain,
	u16_t  shift,
	u16_t	gain_shift
	)
{
	i64_t exp = exp_lines * shift;
	exp *= max_again * max_dgain;
	exp = exp >> gain_shift;
	return (u32_t)exp;
}

// function limits the exposure to the nearest multiple of flicker half-period.
static u32_t exposure_antiflicker_correct (
		u32_t exposure_lines,
		u32_t flicker_freq,
		u32_t lines_per_500ms,
		u16_t	sys_factor
		)
{
	int N;
	if (flicker_freq == 0)
	{
		return exposure_lines;
	}
	N = (exposure_lines * flicker_freq) / (lines_per_500ms);
	N = N < 1 ? 1 : N;
	return (N * lines_per_500ms) / flicker_freq;
}

// 计算下一次的曝光量(积分时间/增益)
int isp_exposure_cmos_calculate (
		cmos_exposure_ptr_t p_exp,
		u32_t exp_increment,
		u32_t exp_increment_max,
		u32_t exp_increment_min,
		u32_t exp_increment_offset
		)
{
	unsigned int max_lines_short;
//	unsigned int max_lines_long;
	u32_t exposure_1st, exposure_2nd;
	u32_t new_exposure;
	u32_t old_exposure_ashort;
	i64_t temp64;
	int same_exp = 0;		// 检查理论曝光值是否相同
	
	int exp_dir = 0;		// 1 曝光量增加 0 曝光量不变 -1 曝光量减小
	
	static int flicker_state = 0;
	int new_flicker_state;
	
	old_exposure_ashort = p_exp->cmos_inttime.exposure_ashort;
//	max_lines_long  = p_exp->cmos_inttime.max_lines_target;
	max_lines_short = p_exp->cmos_inttime.max_lines_target;
		
	// 根据直方图误差调整理论曝光量 exposure
	// new_exposure的计算会出现32位溢出
	// new_exposure = (p_exp->exposure * exp_increment + (exp_increment_offset >> 1)) / exp_increment_offset;
	temp64 = p_exp->exposure;
	temp64 *= exp_increment;
	//temp64 += (exp_increment_offset >> 1);		// 不做四舍五入处理, 避免曝光量误差过大导致画面亮度轻微抖动
	new_exposure = div_s64(temp64, exp_increment_offset);
//	p_exp->last_exposure = p_exp->exposure;
	
	/*
	XM_printf ("\t\texposure = %10d, new_exposure = %10d, increment = %10d\n", 
			p_exp->exposure,
			new_exposure,
			exp_increment);
	*/		
	if(p_exp->exposure == new_exposure)
	{
		same_exp = 1;
		exp_dir = 0;
	}
	else if(p_exp->exposure < new_exposure)
	{
		// 曝光量增加
		exp_dir = 1;
	}
	else
	{
		// 曝光量减小
		exp_dir = -1;
	}
	
	p_exp->exposure = new_exposure;
	
	// 检查曝光量的范围
	boundaries_check (
		&p_exp->exposure,
		p_exp->exp_llimit,
		p_exp->exp_tlimit
		);
		
	exposure_1st = p_exp->exposure;
	exposure_2nd = p_exp->exposure;
	
	// the exposure/gain is allocated as following:
	// 1) analog gain
	// 2) digital gain
	// 3) exposure time
	// 4) antiflicker correction (if enabled) -> setting exposure times to finite s
	// 5) adjust again/dgain if necessary
	
	// 检查sensor是否具有模拟增益.
	// 若有, 计算sensor的模拟增益
	if(1 != p_exp->cmos_gain.max_again_target)
	{
		// 根据曝光量exposure_1st, 计算模拟增益again, 然后修正exposure_1st
		exposure_1st = (*p_exp->cmos_sensor.analog_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_1st,
			p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor
			);
	}
	
	// 检查sensor是否具有数字增益.
	// 若有, 计算sensor的数字增益
	if(1 != p_exp->cmos_gain.max_dgain_target)
	{
		// 根据曝光量exposure_1st, 计算数字增益again, 然后修正exposure_1st
		exposure_1st = (*p_exp->cmos_sensor.digital_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_1st,		// 曝光量
			p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor	// 最大积分行数量
			);
	}
	
	// 根据估计的增益值(模拟+数字), 计算所需的曝光积分行数
	//p_exp->cmos_inttime.exposure_ashort = ((exposure_1st + p_exp->sys_factor - 1) / p_exp->sys_factor);
	if(p_exp->cmos_inttime.flicker_freq)		// 频闪时, 计算顺序为 1) 曝光行, 2) 模拟增益 3) 数字增益
		p_exp->cmos_inttime.exposure_ashort = ((exposure_2nd ) / p_exp->sys_factor);
	else
		p_exp->cmos_inttime.exposure_ashort = ((exposure_1st ) / p_exp->sys_factor);		// 禁止四舍五入, 会导致exposure_ashort * sys_factor大于exposure_2nd,
																												//		这样第二次校正时不会进行模拟增益校正,从而导致曝光不稳,出现闪烁.
	//if(p_exp->cmos_inttime.exposure_ashort >= p_exp->cmos_inttime.max_flicker_lines)
	//	p_exp->cmos_inttime.exposure_ashort = p_exp->cmos_inttime.max_flicker_lines;
	if(p_exp->cmos_inttime.exposure_ashort >= p_exp->cmos_inttime.max_lines)
		p_exp->cmos_inttime.exposure_ashort = p_exp->cmos_inttime.max_lines;
	
	if(p_exp->cmos_inttime.exposure_ashort > max_lines_short) 
		p_exp->cmos_inttime.exposure_ashort = max_lines_short;
	
	// anti-flicker 校正
	if(p_exp->cmos_inttime.flicker_freq && (new_exposure >= p_exp->cmos_inttime.max_flicker_lines * p_exp->sys_factor))
	{
		// 消除50/60hz频闪会导致室外白天场景(不需要消除频闪)下的曝光行计算不准确.
		// 由于无法判断50/60hz频闪场景, 又需要保证白天曝光的准确性, 
		// 通过判断总曝光量为最大行曝光量的2倍时, 开启50/60hz频闪消除算法
		
		// 20170806 由于无法准确判断频闪(缺乏硬件支持), 且频闪使能判定算法不可靠, 因此频闪使能时不再考虑曝光是否过曝等问题,
		//		由使用者决定开启(50/60Hz)或关闭频闪功能
		u32_t new_exposure_ashort = exposure_antiflicker_correct(
				p_exp->cmos_inttime.exposure_ashort,
				p_exp->cmos_inttime.flicker_freq,
				p_exp->cmos_inttime.lines_per_500ms,
				p_exp->sys_factor
				);
		// 以下算法为防止反复导致频闪
		// new_exposure    inttime    again
		// 184434	        364	      1.979245364	
		// 190197	        728	      1.020545373		曝光剧烈变化点
		// 181495	        364	      1.947705615		曝光反复点, 维持
		// 140651	        364	      1.509390024		曝光反复点
		if(new_exposure_ashort == old_exposure_ashort)
		{
			p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
		}
		else if(exp_dir == (-1))
		{
			// 曝光量减小
			//double gain = ((double)exposure_2nd) / p_exp->sys_factor;
			//gain = gain / new_exposure_ashort;
			//if(gain >= 1.75 && gain < 2.0) 
			u32_t igain = exposure_2nd / new_exposure_ashort;
			if( igain >= (p_exp->sys_factor * 6/5) && igain < (p_exp->sys_factor * 2) )
			{
				// 1.75 ~ 2.00 维持,使用老的inttime
				p_exp->cmos_inttime.exposure_ashort = old_exposure_ashort;
			}
			else
			{
				// < 1.75 或者 >= 2.00, 使用新的inttime
				p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
				XM_printf ("dec old=%d, new=%d\n", old_exposure_ashort, new_exposure_ashort);
			}
		}
		else
		{
			// 曝光量增加
			// 新的曝光行计数必须大于老的曝光行计数
			if(new_exposure_ashort > old_exposure_ashort)
			{
				p_exp->cmos_inttime.exposure_ashort = new_exposure_ashort;
				XM_printf ("inc old=%d, new=%d\n", old_exposure_ashort, new_exposure_ashort);
			}
			else
			{
				p_exp->cmos_inttime.exposure_ashort = old_exposure_ashort;
			}
		}
				  
		/*		  
		p_exp->cmos_inttime.exposure_ashort = exposure_antiflicker_correct(
				p_exp->cmos_inttime.exposure_ashort,
				p_exp->cmos_inttime.flicker_freq,
				p_exp->cmos_inttime.lines_per_500ms,
				p_exp->sys_factor
				);
		*/
		new_flicker_state = 1;
	}
	else if(p_exp->cmos_inttime.flicker_freq)
	{
		new_flicker_state = 0;
	}
	else
	{
		new_flicker_state = 0;
	}
	
	if(new_flicker_state != flicker_state)
	{
		flicker_state = new_flicker_state;
		XM_printf ("%s\n", flicker_state ? "exp --> flick" : "exp --> antif");
	}
	
	
	// 根据确定的曝光行数第二遍校正模拟及数字增益
	if(1 != p_exp->cmos_gain.max_again_target)
	{
		exposure_2nd = (*p_exp->cmos_sensor.analog_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_2nd,
			(p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor)
			);
	}
	
	if(1 != p_exp->cmos_gain.max_dgain_target)
	{
		exposure_2nd = (*p_exp->cmos_sensor.digital_gain_from_exposure_calculate) (
			&p_exp->cmos_gain,
			exposure_2nd,
			(p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor)
			);
	}
	
	return same_exp;
}

// 根据计算的曝光设置更新sensor设置
int isp_cmos_inttime_update (cmos_exposure_ptr_t p_exp)
{
	// 修改曝光行积分数量/模拟/数字增益
	return p_exp->cmos_sensor.cmos_inttime_gain_update (&p_exp->cmos_inttime, &p_exp->cmos_gain);
}

void isp_cmos_inttime_update_manual (cmos_exposure_ptr_t p_exp)
{
	// 修改曝光行积分数量/模拟/数字增益
	p_exp->cmos_sensor.cmos_inttime_gain_update_manual (&p_exp->cmos_inttime, &p_exp->cmos_gain);
}


// 设置帧率
int  isp_cmos_set_framerate (cmos_exposure_ptr_t p_exp, u8_t fps)
{
	cmos_inttime_ptr_t inttime;
	if(!p_exp->cmos_sensor.cmos_fps_set)
	{
		XM_printf ("cmos_fps_set can't be empty\n");
		return -1;
	}
	(*p_exp->cmos_sensor.cmos_fps_set) (&p_exp->cmos_inttime, fps);
	p_exp->fps = fps;

	inttime = &p_exp->cmos_inttime;
	boundary_min_check_u16(&inttime->min_lines_target, 1);
	inttime->min_lines = inttime->min_lines_target;
	inttime->max_lines_target = (u16_t)(inttime->full_lines - 2);
	inttime->max_lines = inttime->max_lines_target;
	if(inttime->max_lines > inttime->full_lines_limit)
		inttime->max_lines = inttime->full_lines_limit;

	// 更新flicker设置
	inttime->max_flicker_lines = (u16_t)exposure_antiflicker_correct(
		inttime->max_lines,
		inttime->flicker_freq,
		inttime->lines_per_500ms,
		p_exp->sys_factor
		);
	// 最小曝光量不考虑周期性flicker现象
	inttime->min_flicker_lines = inttime->min_lines;

	return 0;
}

// 使能/禁止flicker
int isp_cmos_set_flicker_freq (cmos_exposure_ptr_t p_exp, u8_t flicker_freq)
{
#if ISP_RAW_ENABLE
	flicker_freq = 0;
#endif
	if(	flicker_freq != 0 
		&& flicker_freq != 50
		&& flicker_freq != 60)
	{
		XM_printf ("illegal flicker freqency (%d)\n", flicker_freq);
		return -1;
	}
	if(flicker_freq == 0)
		XM_printf ("exposure anti-flicker disable\n");
	else
		XM_printf ("exposure anti-flicker %d Hz\n");
	
	//OS_EnterRegion ();

	p_exp->cmos_inttime.flicker_freq = flicker_freq;

	// 更新flicker设置
	// max_flicker_lines 小于或等于 p_exp->cmos_inttime.max_lines
	p_exp->cmos_inttime.max_flicker_lines = (u16_t)exposure_antiflicker_correct(
		p_exp->cmos_inttime.max_lines,
		p_exp->cmos_inttime.flicker_freq,
		p_exp->cmos_inttime.lines_per_500ms,
		p_exp->sys_factor
		);
	// 最小曝光量不考虑周期性flicker现象
	p_exp->cmos_inttime.min_flicker_lines = p_exp->cmos_inttime.min_lines;
	
	// 最大曝光量估算
	p_exp->exp_tlimit = exposure_limit_calculate(
				p_exp->cmos_inttime.max_flicker_lines,
		   	p_exp->cmos_gain.max_again,
				p_exp->cmos_gain.max_dgain,
				(u16_t)p_exp->sys_factor,
				(u16_t)(p_exp->cmos_gain.again_shift + p_exp->cmos_gain.dgain_shift)
				);
		
	// 最小曝光量估算
	p_exp->exp_llimit = exposure_limit_calculate(
				p_exp->cmos_inttime.min_flicker_lines,
		   	1,
				1,
				(u16_t)p_exp->sys_factor,
				0
				);
	
	//OS_LeaveRegion();
	
	return 0;
}

int isp_exposure_cmos_initialize (cmos_exposure_ptr_t p_exp)
{
	cmos_gain_ptr_t gain;
	cmos_inttime_ptr_t inttime;
	unsigned int light_freq;

	isp_init_cmos_sensor (&p_exp->cmos_sensor);
	if(!p_exp->cmos_sensor.cmos_gain_initialize || !p_exp->cmos_sensor.cmos_inttime_initialize)
	{
		XM_printf ("the sensor's cmos_gain_initialize or cmos_inttime_initialize can't be empty\n");
		return -1;
	}
	
	// gain 初始化
	gain = (*p_exp->cmos_sensor.cmos_gain_initialize) ();
	if(!gain)
	{
		XM_printf ("the gain instance can't be empty\n");
		return -1;
	}
	memcpy (&p_exp->cmos_gain, gain, sizeof(p_exp->cmos_gain));
	gain = &p_exp->cmos_gain;
	// 缺省增益设置为1
	gain->again = 1 << gain->again_shift;
	gain->dgain = 1 << gain->dgain_shift;
	gain->iso = 100;
	gain->max_again = gain->max_again_target;
	gain->max_dgain = gain->max_dgain_target;

	// 曝光积分行数初始化
	inttime = (*p_exp->cmos_sensor.cmos_inttime_initialize)();
	if(!inttime)
	{
		XM_printf ("the inttime instance can't be empty\n");
		return -1;
	}
	memcpy (&p_exp->cmos_inttime, inttime, sizeof(p_exp->cmos_inttime));
	p_exp->cmos_inttime.flicker_freq = 0;		// 缺省禁止flicker
	
	p_exp->cmos_inttime.exposure_ashort = inttime->min_lines_target;

	// 检查inttime参数

	// 设置sensor的初始参数(积分时间及增益设置)
	isp_cmos_inttime_update (p_exp);

	// 缺省帧率 30帧
	isp_cmos_set_framerate (p_exp, 30);
	
#if 0
	light_freq = AP_GetMenuItem (APPMENUITEM_LIGHT_FREQ);
	if(light_freq == AP_SETTING_LIGHT_FREQ_50HZ)
		isp_cmos_set_flicker_freq (p_exp, 50);
	else if(light_freq == AP_SETTING_LIGHT_FREQ_60HZ)
		isp_cmos_set_flicker_freq (p_exp, 60);
	else //if(light_freq == AP_SETTING_LIGHT_FREQ_OFF)
		isp_cmos_set_flicker_freq (p_exp, 0);
#endif
	
	// 估计一个初始曝光值
	p_exp->sys_factor = 256;//128;	//64;
	p_exp->exposure = p_exp->cmos_inttime.exposure_ashort * p_exp->sys_factor;
	
	// 缺省禁止flicker
	isp_cmos_set_flicker_freq (p_exp, 0);

#if 0
	// 最大曝光量估算
	p_exp->exp_tlimit = exposure_limit_calculate(
				p_exp->cmos_inttime.max_flicker_lines,
		   	p_exp->cmos_gain.max_again,
				p_exp->cmos_gain.max_dgain,
				(u16_t)p_exp->sys_factor,
				(u16_t)(p_exp->cmos_gain.again_shift + p_exp->cmos_gain.dgain_shift)
				);
		
	// 最小曝光量估算
	p_exp->exp_llimit = exposure_limit_calculate(
				p_exp->cmos_inttime.min_flicker_lines,
		   	1,
				1,
				(u16_t)p_exp->sys_factor,
				0
				);
#endif

	p_exp->physical_exposure = 0;

	return 0;
}


void isp_min_integration_time_set (cmos_exposure_ptr_t p_exp, u16_t value)
{
	p_exp->cmos_inttime.min_lines_target = value;
}

void isp_max_integration_time_set (cmos_exposure_ptr_t p_exp, u16_t value)
{
	p_exp->cmos_inttime.max_lines_target = value;
}


extern isp_ae_t p_ae;
// 初始化cmos sensor的实例
int arkn141_isp_ae_initialize (cmos_exposure_ptr_t p_exp)
{
	int i, j;
	u8_t hist_thresh[4] = {0x10, 0x40, 0x80, 0xc0};
	
	u8_t win_weight[3][3] = {1, 1, 1, 1,  1, 1, 1, 1, 1};
	
	memset (p_exp, 0, sizeof(*p_exp));

	isp_auto_exposure_initialize (&(p_exp->cmos_ae));

	isp_exposure_cmos_initialize (p_exp);

	isp_min_integration_time_set (p_exp, ISP_SYSTEM_MIN_INTEGRATION_TIME_DEFAULT);
	isp_max_integration_time_set (p_exp, ISP_SYSTEM_MAX_INTEGRATION_TIME_DEFAULT);
	
	for (i = 0; i < 4; i ++)
	{
		p_ae.histoBand[i] = hist_thresh[i];
	}
	
	for (i = 0; i < 3; i ++)
	{
		for (j = 0; j < 3; j ++)
		{
			p_ae.winWeight[i][j] = win_weight[i][i];
		}
	}
	
	p_ae.bright_target = AE_BRIGHT_TARGET_DEFAULT;
	p_ae.black_target = 128;
	p_ae.compensation = AE_COMPENSATION_DEFAULT;
	
	if(p_exp->cmos_sensor.cmos_isp_ae_init)
	{
		(*p_exp->cmos_sensor.cmos_isp_ae_init)(&p_ae);
	}

	isp_histogram_thresh_write (&p_exp->cmos_ae, p_ae.histoBand);
	isp_ae_window_weight_write (&p_exp->cmos_ae, p_ae.winWeight);
	//isp_histogram_thresh_write (&p_exp->cmos_ae, hist_thresh);
	//isp_ae_window_weight_write (&p_exp->cmos_ae, win_weight);

	isp_histogram_thread_update (p_exp);

	isp_system_ae_bright_target_write (&p_exp->cmos_ae, p_ae.bright_target);
	isp_system_ae_black_target_write  (&p_exp->cmos_ae, p_ae.black_target);	
	isp_system_ae_compensation_write (&p_exp->cmos_ae, p_ae.compensation);
	isp_system_ae_ev_write  (&p_exp->cmos_ae, 0);
	isp_auto_exposure_compensation (&p_exp->cmos_ae, p_exp->cmos_ae.histogram.bands);

	return 0;
}

int arkn141_isp_ae_run (cmos_exposure_ptr_t p_exp)
{
	int ret = isp_auto_exposure_run (&p_exp->cmos_ae, p_exp, 0);
	return ret;
}

int arkn141_isp_set_sensor_readout_direction (cmos_exposure_ptr_t p_exp, 
															 unsigned int horz_reverse_direction, unsigned int vert_reverse_direction)
{
	if(p_exp && p_exp->cmos_sensor.cmos_sensor_set_readout_direction)
	{
		return (*p_exp->cmos_sensor.cmos_sensor_set_readout_direction) (horz_reverse_direction, vert_reverse_direction);
	}
	else
		return (-1);
}

unsigned int cmos_calc_inttime_gain (cmos_exposure_t *p_isp_exposure)
{
	i64_t inttime_gain;
	inttime_gain = p_isp_exposure->cmos_inttime.exposure_ashort;
	inttime_gain *= p_isp_exposure->cmos_gain.again;
	inttime_gain >>= p_isp_exposure->cmos_gain.again_shift;
	inttime_gain *= p_isp_exposure->cmos_gain.dgain;
	inttime_gain >>= p_isp_exposure->cmos_gain.dgain_shift;
	return (unsigned int)inttime_gain;
}
