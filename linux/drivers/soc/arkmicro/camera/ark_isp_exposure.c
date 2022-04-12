#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/math64.h>
#include "ark_camera.h"
#include "ark_isp_exposure.h"
#include "ark_isp_exposure_cmos.h"

#define ENABLE_ERROR_RATIO	1

static void histogram_bands_initialize (histogram_band_t bands[4])
{
	bands[0].error   = 0;
	bands[1].error   = 0;
	bands[2].error   = 0;
	bands[3].error   = 0;
	bands[0].balance = 0;
	bands[1].balance = 0;
	bands[2].balance = 0;
	bands[3].balance = 0;
}

static void histogram_initialize (histogram_ptr_t p_hist)
{
	p_hist->hist_balance		= 0;
	p_hist->hist_dark			= 0;
	p_hist->hist_dark_shift	= 0x100;
	p_hist->hist_error   	= 0;

	histogram_bands_initialize (p_hist->bands);
}


void isp_system_ae_black_target_write (auto_exposure_ptr_t ae, u8_t data)
{
	ae->ae_black_target = data;
}

u8_t isp_system_ae_black_target_read (auto_exposure_ptr_t ae)
{
	return ae->ae_black_target;
}

void isp_system_ae_bright_target_write (auto_exposure_ptr_t ae, u8_t data)
{
	ae->ae_bright_target = data;
}

u8_t isp_system_ae_bright_target_read (auto_exposure_ptr_t ae)
{
	return ae->ae_bright_target;
}


void isp_histogram_thresh_write (auto_exposure_ptr_t ae, u8_t histoBand[4])
{
	ae->metering_hist_thresh_0_1 = histoBand[0];
	ae->metering_hist_thresh_1_2 = histoBand[1];
	ae->metering_hist_thresh_3_4 = histoBand[2];
	ae->metering_hist_thresh_4_5 = histoBand[3];

	isp_histogram_bands_write (histoBand);
}

void isp_system_ae_compensation_write (auto_exposure_ptr_t ae, u8_t data) 
{
   ae->ae_compensation = data;
}

u8_t isp_system_ae_compensation_read (auto_exposure_ptr_t ae) 
{
	return ae->ae_compensation;
}

void isp_system_ae_ev_write( auto_exposure_ptr_t ae, i8_t data)
{
	if(data < -6)
		data = -6;
	else if(data > 6)
		data = 6;
	ae->ae_ev = data;
}

i8_t isp_system_ae_ev_read  (auto_exposure_ptr_t ae)
{
	return ae->ae_ev;
}


void isp_histogram_thread_update (cmos_exposure_ptr_t p_exp)
{
	histogram_band_t *bands = p_exp->cmos_ae.histogram.bands;
	auto_exposure_ptr_t ae = &p_exp->cmos_ae;
	bands[0].cent    = (ae->metering_hist_thresh_0_1 + 0) / 2;
	bands[1].cent    = (ae->metering_hist_thresh_0_1 + ae->metering_hist_thresh_1_2) / 2;
	bands[2].cent    = (ae->metering_hist_thresh_1_2 + ae->metering_hist_thresh_3_4) / 2;
	bands[3].cent    = (ae->metering_hist_thresh_3_4 + ae->metering_hist_thresh_4_5) / 2;
	bands[4].cent    = (ae->metering_hist_thresh_4_5 + 255) / 2;

}

void isp_auto_exposure_compensation (auto_exposure_ptr_t ae, histogram_band_t bands[5])
{
	u32_t ae_comp;
	//u8_t ev_value[7] = {32, 39, 50, 64, 83, 107, 128};
	u8_t ev_value[13] = {32, 35, 39, 44, 50, 57, 64, 74, 83, 95, 107, 117, 128};
	u8_t ev;
	if(ae->ae_ev < -6)
		ae->ae_ev = -6;
	else if(ae->ae_ev > 6)
		ae->ae_ev = 6;
	ev = ev_value[ae->ae_ev + 6];
	ae_comp = ae->ae_compensation;
	ae_comp *= ev;
	ae_comp /= ISP_SYSTEM_EV_DEFAULT;
	if(ae_comp < 1)
		ae_comp = 1;
	
	bands[0].target  = (ae->ae_black_target << 8) / 2;
	bands[1].target  = (ae->ae_black_target << 8);
	bands[2].target  = 0x2000;
	bands[3].target  = (ae->ae_bright_target << 8) * 2;
	bands[4].target  = (ae->ae_bright_target << 8);
	
	if(ae_comp > 64)
	{
		bands[3].target  = bands[3].target * ae_comp / 64;
		bands[4].target  = bands[4].target * ae_comp / 64;
	}
	else
	{
		bands[0].target  = bands[0].target * 64 / ae_comp;
		bands[1].target  = bands[1].target * 64 / ae_comp;
	}
	
	bands[0].cent    = (ae->metering_hist_thresh_0_1 + 0) / 2;
	bands[1].cent    = (ae->metering_hist_thresh_0_1 + ae->metering_hist_thresh_1_2) / 2;
	bands[2].cent    = (ae->metering_hist_thresh_1_2 + ae->metering_hist_thresh_3_4) / 2;
	bands[3].cent    = (ae->metering_hist_thresh_3_4 + ae->metering_hist_thresh_4_5) / 2;
	bands[4].cent    = (ae->metering_hist_thresh_4_5 + 255) / 2;
}

static void histogram_data_read (histogram_ptr_t p_hist, isp_ae_ptr_t  isp_ae)
{
	int _i;
	u32_t hacc = 0;

	isp_histogram_bands_read(p_hist->bands, isp_ae);

	for(_i = 0; _i < HISTOGRAM_BANDS; ++_i)
	{
		hacc += p_hist->bands[_i].value;
	}
	if(hacc >= 0xFFFF)
		p_hist->bands[2].value = 0;
	else
		p_hist->bands[2].value = (u16_t)(0xFFFF - hacc);

}

static void histogram_error_calculate (auto_exposure_ptr_t ae, histogram_ptr_t p_hist, isp_ae_ptr_t  isp_ae)
{
	u8_t ae_comp;
	//u8_t lum = isp_ae_lum_read ();
	u8_t lum = isp_ae->lumCurr;

	ae_comp = ae->ae_compensation;

	p_hist->hist_error = ae_comp - p_hist->hist_balance;
		
	p_hist->lum_hist[0] = p_hist->lum_hist[1];
	p_hist->lum_hist[1] = p_hist->lum_hist[2];
	p_hist->lum_hist[2] = p_hist->lum_hist[3];
	p_hist->lum_hist[3] = lum;
		
#if 1
	if(p_hist->hist_error > 0)
	{
		// ÆØ¹â²»¹»³¡¾°
		int total_lum = 0;
		int i;
		for (i = 0; i < 4; i ++)
		{
			if(p_hist->lum_hist[i] >= 3)
				break;
			total_lum += p_hist->lum_hist[i];
		}
		if(i == 4)
		{
			// ÁÁ¶ÈÆ«°µ³¡¾°ÏÂ, Ôö¼ÓÆØ¹â
			if(total_lum == 0)
				p_hist->hist_error *= 3;
			else
				p_hist->hist_error = p_hist->hist_error * 3 * 4 / total_lum;	
		}
	}
#else
	// ÁÁ¶ÈÆ«°µ³¡¾°ÏÂ, Ôö¼ÓÆØ¹â
	if(lum < 3 && p_hist->hist_error > 0)
	{
		if(lum == 0)
			p_hist->hist_error *= 3;
		else
			p_hist->hist_error = p_hist->hist_error * 3 / lum;
	}
#endif
}

static void histogram_balance_calculate (histogram_ptr_t p_hist)
{
	int _i;
	int64_t hbal;
	int center;
	int64_t temp;

	hbal = p_hist->bands[2].cent;

	for(_i = 0; _i < HISTOGRAM_BANDS; ++_i)
	{
		if(_i == 0)                    
			center = 0x00;
		else if(_i == HISTOGRAM_BANDS) 
			center = 0xff;
		else                         
			center = p_hist->bands[_i].cent;

		temp = (p_hist->bands[_i].value + p_hist->bands[_i].target/4);
		temp *= (center - p_hist->bands[2].cent);
		temp = div_s64(temp, p_hist->bands[_i].target);	// targetÔ½´ó,Æä¶ÔÎó²îµÄ¼ÆËãÓ°ÏìÔ½Ğ¡
		hbal += temp;
	}
	p_hist->hist_balance =  (int)hbal;
}

static void histogram_dark_calculate (histogram_ptr_t p_hist)
{
	i32_t h_dark = 0;

	h_dark = (p_hist->hist_dark_shift*(p_hist->bands[0].value*2 + p_hist->bands[1].value) + 0x2000) /
				(p_hist->bands[2].value + 0x2000);

	p_hist->hist_dark = h_dark;
}



static void histogram_update (auto_exposure_ptr_t ae, histogram_ptr_t p_hist, isp_ae_ptr_t  isp_ae)
{
	histogram_data_read (p_hist, isp_ae);

	histogram_balance_calculate (p_hist);

	histogram_error_calculate (ae, p_hist, isp_ae);

	//histogram_dark_calculate (p_hist);
}

// ¼ÆËãÄæ¹â³¡¾°µÄ±ÈÀıÒò×Ó(>=256), 
static int base_ratio = 36;
static void calc_bright_error_ratio (isp_ae_ptr_t isp_ae, histogram_ptr_t p_hist)
{
	unsigned int y_max = 0;
	unsigned int y_total = 0;
	unsigned short *yavg_s = (unsigned short *)isp_ae->yavg_s;
	int i;

	for (i = 0; i < 9; i ++)
	{
		if(yavg_s[i] > y_max)
			y_max = yavg_s[i];
		y_total += yavg_s[i];
	}
	//XM_unlock();
	y_total -= y_max;
	y_total *= base_ratio;
	y_total /= 80;
	if(y_total == 0)
		y_total = 1;
		
	if(y_max > 10 * y_total)
		y_max = 10 * y_total;
	
	if(y_max <= y_total)
	{
		p_hist->hist_error_ratio = 256;
	}
	else
	{
		// 20181019 ÆØ¹â·¢ÉúÍ»±ä£¬¸ÄÎªÖğ²½±Æ½ü£¬Ö±µ½ÎÈÌ¬
		p_hist->hist_error_ratio = y_max * 256 / y_total;
		// p_hist->hist_error_ratio = (unsigned int)(11*256/10);	// ÌáÉı10%
	}
}

#if ENABLE_ERROR_RATIO
#define	MAX_ERROR_RATIO		16
static const int histogram_error_ratio[3][3] = {
	0,	0,	0,
	3,	5, 3,
	5,	7, 5
};

static const int histogram_error_ratio_2[3][3] = {
	1,	1,	 1,
	1,	1, 1,
	1,	1, 1
};

#endif

static void histogram_3x3_update (auto_exposure_ptr_t ae, histogram_ptr_t p_hist, isp_ae_ptr_t  isp_ae)
{
	int i, j;
	static histogram_t hist[3][3]; 
	unsigned short histogram_s[3][3][5];
	int hist_error_min = 65535;
	int hist_error_max = -65536;

	int last_error = p_hist->hist_error;

	memcpy (histogram_s, isp_ae->histogram_s, sizeof(histogram_s));
	
	for (i = 0; i < 3; i ++)
	{
		for (j = 0; j < 3; j ++)
		{
			memcpy (&hist[i][j], p_hist, sizeof(histogram_t));
			hist[i][j].bands[0].value = histogram_s[i][j][0] << 4;
			hist[i][j].bands[1].value = histogram_s[i][j][1] << 4;
			hist[i][j].bands[2].value = histogram_s[i][j][2] << 4;
			hist[i][j].bands[3].value = histogram_s[i][j][3] << 4;
			hist[i][j].bands[4].value = histogram_s[i][j][4] << 4;
			histogram_balance_calculate (&hist[i][j]);
			histogram_error_calculate (ae, &hist[i][j], isp_ae);
#if ENABLE_ERROR_RATIO
			hist[i][j].hist_error = hist[i][j].hist_error * histogram_error_ratio[i][j] / MAX_ERROR_RATIO;
#endif			
			if(hist[i][j].hist_error < hist_error_min)
				hist_error_min = hist[i][j].hist_error;
			if(hist[i][j].hist_error > hist_error_max)
				hist_error_max = hist[i][j].hist_error;
		}
	}
	
	// ¼ÆËã×îĞ¡Öµ
	histogram_update (ae, p_hist, isp_ae);
	
	p_hist->hist_error_one = p_hist->hist_error;
	p_hist->hist_error_min = hist_error_min;
	p_hist->hist_error_max = hist_error_max;
	p_hist->hist_error_last = last_error;
	
	if(ae->ae_mode == AE_MODE_BRIGHT)
	{
		calc_bright_error_ratio (isp_ae, p_hist);
		if(p_hist->hist_error > 0)
		{
			p_hist->hist_error = p_hist->hist_error * p_hist->hist_error_ratio / 256;
		}
			
	}
	else if(ae->ae_mode == AE_MODE_DARK)
	{
		int curr_error;
		if(hist_error_min < -4)
		{
			curr_error = hist_error_min;
			if(hist_error_min < -4)
			{
				//printf ("he %d --> %d\n", p_hist->hist_error, hist_error_min);
				//int curr_error = hist_error_min + p_hist->hist_error * 4 / abs(hist_error_min);
				p_hist->hist_error = hist_error_min + p_hist->hist_error * 4 / abs(hist_error_min);
			}
		}
		else
		{
			curr_error = p_hist->hist_error;
		
			// 20181113 Ôö¼ÓÒ»ÌõÕıÆØ¹âÔ¼ÊøÌõ¼ş
			//  µ±hist_errorÓëhist_error_min¾ùÎªÕıÖµÊ±(ÆØ¹â²»¹»£¬Ôö¼ÓÆØ¹â)£¬Èôhist_error_minÓëhist_error²îÒì½Ï´ó£¬
			// Îª±ÜÃâhist_error_min·­×ªµ¼ÖÂÆØ¹âÕñµ´(Ã÷°µ½»Ìæ)£¬´ËÊ±Ó¦Ô¼Êøhist_errorµÄÖµ£¬
			// hist_error_minÖµÔ½Ğ¡£¬hist_errorµÄ±ä»¯Ô¼Êø³Ì¶ÈÔ½´ó¡£
			// hist_error_minÖµÔ½´ó£¬hist_errorµÄ±ä»¯Ô¼Êø³Ì¶ÈÔ½Ğ¡¡£
			if(hist_error_min > 0 && curr_error > 0)
			{
				if(hist_error_min < 8)
				{
					curr_error /= 2;			// curr_error / 2
				}
				else if( (curr_error / hist_error_min) > 3)
				{
					curr_error = curr_error * 2 / 3;		// curr_error / 1.5
				}
					
				if(curr_error < hist_error_min)
					curr_error = hist_error_min;
				
				p_hist->hist_error = curr_error;
			}
			
			if(last_error < 0 && curr_error > 0 || last_error > 0 && curr_error < 0)
			{
				//if(abs(last_error - curr_error) > 128)
				{
					// ±ÜÃâÁÁ¶È±ä»¯¹ı´ó
					curr_error = (last_error + curr_error) / 4;
					p_hist->hist_error = curr_error;
				}
			}
			
		}
	}
}

extern isp_ae_t p_ae;

// °µ³¡¾°ÅĞ¶¨¹æÔò
static int check_dark_mode (unsigned int iso, isp_ae_t *ptr_ae)
{
	int is_dark_mode = 0;
	unsigned int y_max = 0;
	unsigned int y_max_i = 0;
	unsigned int y_max_j = 0;
	int i,j;
	
	if(ptr_ae)
	{
		unsigned short *yavg = (unsigned short *)ptr_ae->yavg_s;
		
		int count = 0;
		for (i = 0; i < 3; i++)
		{
			for (j = 0; j < 3; j ++)
			{
				unsigned int yavg = ptr_ae->yavg_s[i][j];
				if(yavg == 0)
					count ++;
				if(y_max < yavg)
				{
					y_max = yavg;
					y_max_i = i;
					y_max_j = j;
				}
			}
		}
		
		if(count >= 2)
		{
			// Î¢¹âÇøÖÁÉÙ2¸ö
			// ¼ì²é×î´óÁÁ¶ÈµÄÇøÓò£¬Æä¸ßÁÁÇøÊÇ·ñĞ¡ÓÚ256¡£ÍíÉÏÒ»°ãÎªµã¹âÔ´£¬²»Í¬ÓÚ°×Ìì¸ß¹âÇø
			unsigned int histo = ptr_ae->histogram_s[y_max_i][y_max_j][3] + ptr_ae->histogram_s[y_max_i][y_max_j][4];
			if(histo < 256)
				is_dark_mode = 1;			
			
		}
	}
		
	return is_dark_mode;
}

// Äæ¹â
static int check_bl_mode (unsigned int iso, isp_ae_t *ptr_ae)
{
	int is_bl_mode = 0;
	int i;
	
	if(ptr_ae && iso < 16)
	{
		unsigned short *yavg = (unsigned short *)ptr_ae->yavg_s;
		int count = 0;
		for (i = 0; i < 9; i++)
		{
			if(yavg[i] == 0)
				return 0;
			if(yavg[i] == 1)
				count ++;
		}
		
		if(count >= 2)
			is_bl_mode = 1;
	}
		
	return is_bl_mode;
}

// ÅĞ¶ÏÊÇ·ñÊÇ°×Ìì(ÁÁ³¡¾°)Ä£Ê½
static int is_bright_mode (unsigned int iso, isp_ae_t *ptr_ae)
{
	unsigned int y_max = 0;
	unsigned int y_max_i = 0;
	unsigned int y_max_j = 0;
	unsigned int zero_count = 0;
	int i,j;
	
	// ³¬¶ÌÆØ¹â
	if(iso <= 4)
		return 1;
	// ¶ÌÆØ¹â
	if(iso < 16 && ptr_ae->lumCurr >= 8)
		return 1;
		
	// 20190416 ä¸‹é¢çš„æ¡ä»¶ä¼šå®¹æ˜“å¯¼è‡´å‘¨æœŸæ€§æ˜æš—æ›å…‰ï¼Œç§»è‡³æœ€ä¸‹é¢
	// ä¸­æ›å…‰
	//if(iso < 128 && ptr_ae->lumCurr >= 16)
	//	return 1;
		
	// ÅĞ¶ÏÃ¿¸ö¿éµÄÁÁ¶ÈÖµ¾ù´óÓÚÄ³¸öÖµ
	int count = 0;
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j ++)
		{
			unsigned int yavg = ptr_ae->yavg_s[i][j];
			if(yavg == 0)
				zero_count ++;
			if(yavg > y_max)
			{
				y_max = yavg;
				y_max_i = i;
				y_max_j = j;
			}
			if(yavg <= 1)
				count ++;
		}
	}
	
	// 20181023 ¸ù¾İÂ·²â½á¹û£¬Ôö¼ÓÒ»ÌõÁÁ³¡¾°(°×Ìì)µÄÅĞ¶Ï¹æÔò
	// 1) ISO < 48, ´æÔÚÖÁÉÙÒ»¸öÁÁÇø(ÁÁ¶ÈÖµ´óÓÚ64£¬ÇÒ¸ß¹âÇø4+5´óÓÚ1000) 
	//    ae_sensor_inttime =         35
	//    ae_sensor_again = 1.000000
	//    ae_sensor_dgain = 1.000000	
	//    [0][1]:yavg_s =   72;[0] = 2292; [1] =   25; [2] =  401; [3] =  848; [4] =  529;
	if(iso < 48)
	{
		unsigned int histo = ptr_ae->histogram_s[y_max_i][y_max_j][3] + ptr_ae->histogram_s[y_max_i][y_max_j][4];
		if(y_max >= 64 && histo >= 1000 )
			return 1;
	}
	
	
	// ÎŞÎ¢¹âÇø
	if(zero_count >= 1)
		return 0;
	
	// ×î´óÁÁ¶ÈÖµ´óÓÚ100
	if(y_max > 100)
		return 1;
	
	// Èõ¹âÇø¸öÊı
	if(count >= 2)
		return 0;
	
	// 20190416 ä¸‹é¢çš„æ¡ä»¶ä¼šå®¹æ˜“å¯¼è‡´å‘¨æœŸæ€§æ˜æš—æ›å…‰ï¼Œç§»è‡³æ­¤å¤„
	// ä¸­æ›å…‰
	if(iso < 128 && ptr_ae->lumCurr >= 16)
		return 1;
	
	return 0;
}

static int get_y_max (isp_ae_t *ptr_ae)
{
	unsigned int y_max = 0;
	int i, j;
		
	// åˆ¤æ–­æ¯ä¸ªå—çš„äº®åº¦å€¼å‡å¤§äºæŸä¸ªå€¼
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j ++)
		{
			unsigned int yavg = ptr_ae->yavg_s[i][j];
			if(yavg > y_max)
			{
				y_max = yavg;
			}
		}
	}
	
	return y_max;
}

void isp_auto_exposure_set_iso (cmos_exposure_t *p_isp_exposure, unsigned int iso)
{
	if(p_isp_exposure->cmos_ae.ae_mode == AE_MODE_BRIGHT)
	{
		//if(iso > p_isp_exposure->cmos_inttime.full_lines * 4/5)
		// 20190407 æµ‹è¯•å‘ç°å­˜åœ¨æŸç§æƒ…å†µï¼Œæ˜æš—åå¤ï¼Œæ­¤æ—¶çš„æœ€å¤§äº®åº¦å€¼150ã€‚
		// åœ¨äº®åœºæ™¯ä¸‹ï¼Œå¢åŠ ä¸€ä¸ªæ¡ä»¶ï¼Œå½“æœ€å¤§äº®åº¦å€¼>=95, ç¦æ­¢åˆ‡æ¢åˆ°æš—åœºæ™¯æ¨¡å¼
		//if(iso > p_isp_exposure->cmos_inttime.full_lines * 1/2)
		if(   (iso > p_isp_exposure->cmos_inttime.full_lines) 
			|| ((iso > p_isp_exposure->cmos_inttime.full_lines * 1/2) && (get_y_max(&p_ae) < 95)) )
		{
			p_isp_exposure->cmos_ae.n_cycle ++;
			p_isp_exposure->cmos_ae.d_cycle = 0;
			if(p_isp_exposure->cmos_ae.n_cycle > 5)
			{
				p_isp_exposure->cmos_ae.ae_mode = AE_MODE_DARK;
				p_isp_exposure->cmos_ae.n_cycle = 0;
				XM_printf ("dark Mode\n");
			}
		}
		else
		{
			p_isp_exposure->cmos_ae.n_cycle = 0;
			p_isp_exposure->cmos_ae.d_cycle = 0;
		}		
	}
	else if(p_isp_exposure->cmos_ae.ae_mode == AE_MODE_DARK)
	{
		//if(iso < 16 && p_ae.lumCurr >= 5 || iso <= 3)
		if(is_bright_mode (iso, &p_ae))
		{
			p_isp_exposure->cmos_ae.d_cycle ++;
			p_isp_exposure->cmos_ae.n_cycle = 0;
			if(p_isp_exposure->cmos_ae.d_cycle >= 4)
			{
				p_isp_exposure->cmos_ae.ae_mode = AE_MODE_BRIGHT;
				p_isp_exposure->cmos_ae.d_cycle = 0;
				XM_printf ("short bright Mode\n");
			}
		}
		/*else if(iso < 128 && p_ae.lumCurr >= 8)
		{
			p_isp_exposure->cmos_ae.d_cycle ++;
			p_isp_exposure->cmos_ae.n_cycle = 0;
			if(p_isp_exposure->cmos_ae.d_cycle > 5)
			{
				p_isp_exposure->cmos_ae.ae_mode = AE_MODE_BRIGHT;
				p_isp_exposure->cmos_ae.d_cycle = 0;
				XM_printf ("bright Mode\n");
			}
		}*/
		else
		{
			p_isp_exposure->cmos_ae.n_cycle = 0;
			p_isp_exposure->cmos_ae.d_cycle = 0;
		}
		
	}
	
	// ¸ù¾İÁÁ¶ÈĞ£ÕıÌØÊâ³¡¾°ÏÂµÄÄ£Ê½
	if(p_isp_exposure->cmos_ae.ae_mode == AE_MODE_BRIGHT)
	{
		// ¼ì²é±ØĞëÎª°µ³¡Ä£Ê½µÄÄ³Ğ©¼«¶Ë³¡¾°
		if(check_dark_mode(iso, &p_ae))
		{
			p_isp_exposure->cmos_ae.ae_mode = AE_MODE_DARK;
			p_isp_exposure->cmos_ae.n_cycle = 0;
			p_isp_exposure->cmos_ae.d_cycle = 0;
			XM_printf ("dark correct Mode\n");
		}
	}
	else if(p_isp_exposure->cmos_ae.ae_mode == AE_MODE_DARK)
	{
		// ¼ì²éÄæ¹âÄ£Ê½
		if(check_bl_mode(iso, &p_ae))
		{
			p_isp_exposure->cmos_ae.ae_mode = AE_MODE_BRIGHT;
			p_isp_exposure->cmos_ae.n_cycle = 0;
			p_isp_exposure->cmos_ae.d_cycle = 0;
			XM_printf ("bright correct Mode\n");
		}
	}
}

void isp_auto_exposure_initialize (auto_exposure_ptr_t p_ae_block)
{
	p_ae_block->exposure_quant = 0x0080;
	p_ae_block->increment_offset = (1 << 8) * 16;
	p_ae_block->increment = p_ae_block->increment_offset;
	p_ae_block->increment_damping = 0;		// ·´Ïà×èÄá¹Ø±Õ
	p_ae_block->exposure_target = 0;

	p_ae_block->exposure_steps = 0;
	p_ae_block->exposure_factor = 1;

	p_ae_block->steady_control.enable = 0;
	p_ae_block->steady_control.count = 0;

	histogram_initialize (&(p_ae_block->histogram));
}

// ¸ù¾İµ±Ç°µÄÖ±·½Í¼Îó²î¾ø¶ÔÖµ¼ÆËãÆØ¹âÔöÁ¿µÄ²½³¤
// Îó²î¾ø¶ÔÖµÔ½´ó,²½³¤Ô½´ó
static u32_t auto_exposure_increment_get (auto_exposure_ptr_t p_ae_block)
{
	u32_t _step = p_ae_block->exposure_quant;

	if(abs(p_ae_block->histogram.hist_error) <= 0x4)
	{
		_step = p_ae_block->exposure_quant / 64;
	}
	else if(abs(p_ae_block->histogram.hist_error) <= 0x8)
	{
		_step = p_ae_block->exposure_quant / 32;
		//_step = p_ae_block->exposure_quant / 20;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x10)
	{
		//_step = p_ae_block->exposure_quant / 16;
		_step = p_ae_block->exposure_quant / 12;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x18)
	{
		//_step = p_ae_block->exposure_quant / 10;
		//_step = p_ae_block->exposure_quant / 5;
		_step = p_ae_block->exposure_quant / 3;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x20)
	{
		//_step = p_ae_block->exposure_quant / 5;
		//_step = p_ae_block->exposure_quant * 2 / 5;		// 0.4
		_step = p_ae_block->exposure_quant * 1 / 2;	
		//_step = p_ae_block->exposure_quant * 3 / 4;	// 0.75
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x30)	// 48
	{
		//_step = p_ae_block->exposure_quant / 3;
		_step = p_ae_block->exposure_quant * 3 / 4;	// 0.75
		//_step = p_ae_block->exposure_quant * 1;	
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x40)	// 64
	{
		_step = p_ae_block->exposure_quant * 4 / 5 ;
		//_step = p_ae_block->exposure_quant * 3/2;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x50)	// 80
	{
		_step = p_ae_block->exposure_quant * 1;
		//_step = p_ae_block->exposure_quant * 2;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x60)	// 96
	{
		//_step = p_ae_block->exposure_quant * 3 / 2;
		_step = p_ae_block->exposure_quant * 5 / 4;
		//_step = p_ae_block->exposure_quant * 5/2;
	}

	// ¶¨ÒåÌ«´óµÄstep¿ÉÄÜµ¼ÖÂÆØ¹âµ÷Õû¹ı´ó, µ¼ÖÂÖ±·½Í¼´íÎóÎ»·­×ª(´ÓÕıµ½¸º »ò ´Ó¸ºµ½Õı), µ¼ÖÂÆØ¹âÉÁË¸ÏÖÏó·¢Éú
	// ±ÈÈç ÏÂÃæÊµÀıÔÊĞí´óµÄ²½·ù¿ØÖÆ (> 0x200)
	// hist_error --> -2357,  step = 128(²½·ù¹ı´ó), ¼õÉÙÆØ¹â, ²½·ùµ÷Õû¹ı´ó, ĞÂµÄÆØ¹âÖµ inttime=   61, again=1.000000, dgain=1.000000
	// hist_error -->   115,  step = 16, ÒòÎª²½·ùµ÷Õû¹ı´ó,ĞèÒªÖğ²½Ôö¼ÓÆØ¹âÖµ, ĞÂµÄÆØ¹âÖµ inttime=   64, again=1.000000, dgain=1.007813
	// ...
	// hist_error -->      5, step = 1, ÆØ¹âÎÈ¶¨, inttime=   91, again=1.000000, dgain=1.007813
	/*
	else if(abs(p_ae_block->histogram.hist_error) > 0x200)
	{
		_step = p_ae_block->exposure_quant * 8;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 0x100)
	{
		_step = p_ae_block->exposure_quant * 4;
	}*/
	
	else if(abs(p_ae_block->histogram.hist_error) < 0x70)	// 112
	{
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 3;
		_step = p_ae_block->exposure_quant * 3/2;
		//_step = p_ae_block->exposure_quant * 4/3;
	}
	// ¿¼ÂÇÏÂÃæµÄ²½·ù¹ı´ó,µ¼ÖÂ»­ÃæÁÁ°µÃ÷ÏÔ
	else if(abs(p_ae_block->histogram.hist_error) < 0x80)	
	{
		//_step = p_ae_block->exposure_quant * 6/4;
		//_step = p_ae_block->exposure_quant * 7/2;
		_step = p_ae_block->exposure_quant * 2;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0x90)	// 144
	{
		// _step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 5/2;
		//_step = p_ae_block->exposure_quant * 3/2;
		//_step = p_ae_block->exposure_quant * 7/4;
		//_step = p_ae_block->exposure_quant * 4;
	}
	
	else if(abs(p_ae_block->histogram.hist_error) < 0xA0)	// 160
	{
		//_step = p_ae_block->exposure_quant * 3/2;
		_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 5 / 2;
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 9/2;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0xB0)	// 176
	{
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 5;
		_step = p_ae_block->exposure_quant * 7/2;
		//_step = p_ae_block->exposure_quant * 5;
	}	
	else if(abs(p_ae_block->histogram.hist_error) < 0xC0)	// 192
	{
		//_step = p_ae_block->exposure_quant * 3/2;
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 8;
		//_step = p_ae_block->exposure_quant * 5/2;
		//_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 6;
		_step = p_ae_block->exposure_quant * 4;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0xD0)	// 192
	{
		//_step = p_ae_block->exposure_quant * 3/2;
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 8;
		//_step = p_ae_block->exposure_quant * 5/2;
		//_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 13/2;
		_step = p_ae_block->exposure_quant * 9/2;
	}
	else if(abs(p_ae_block->histogram.hist_error) < 0xE0)	// 192
	{
		//_step = p_ae_block->exposure_quant * 3/2;
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 8;
		//_step = p_ae_block->exposure_quant * 5/2;
		//_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 7;
		_step = p_ae_block->exposure_quant * 5;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 3000)
	{
		// _step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 20;
		_step = p_ae_block->exposure_quant * 30;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 2000)
	{
		// _step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 15;
		//_step = p_ae_block->exposure_quant * 25;
		_step = p_ae_block->exposure_quant * 27;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 1000)
	{
		// _step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 11;
		//_step = p_ae_block->exposure_quant * 18;
		_step = p_ae_block->exposure_quant * 21;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 900)
	{
		// _step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 9;
		//_step = p_ae_block->exposure_quant * 16;
		_step = p_ae_block->exposure_quant * 18;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 700)
	{
		// _step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 8;
		//_step = p_ae_block->exposure_quant * 10;
		//_step = p_ae_block->exposure_quant * 16;
		//_step = p_ae_block->exposure_quant * 12;
		_step = p_ae_block->exposure_quant * 15;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 600)
	{
		//_step = p_ae_block->exposure_quant * 6;
		//_step = p_ae_block->exposure_quant * 8;
		//_step = p_ae_block->exposure_quant * 14;
		//_step = p_ae_block->exposure_quant * 10;
		_step = p_ae_block->exposure_quant * 13;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 500)
	{
		//_step = p_ae_block->exposure_quant * 5;
		//_step = p_ae_block->exposure_quant * 7;
		//_step = p_ae_block->exposure_quant * 12;
		//_step = p_ae_block->exposure_quant * 8;
		_step = p_ae_block->exposure_quant * 12;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 400)
	{
		//_step = p_ae_block->exposure_quant * 4;
		//_step = p_ae_block->exposure_quant * 6;
		_step = p_ae_block->exposure_quant * 10;
		//_step = p_ae_block->exposure_quant * 7;
		//_step = p_ae_block->exposure_quant * 11;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 300)
	{
		//_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 5;
		//_step = p_ae_block->exposure_quant * 10;
		//_step = p_ae_block->exposure_quant * 6;
		_step = p_ae_block->exposure_quant * 8;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 0x100)
	{
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 9;
		_step = p_ae_block->exposure_quant * 7;
		//_step = p_ae_block->exposure_quant * 5;
	}
	/*
	else if(abs(p_ae_block->histogram.hist_error) > 240)
	{
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 3;
		_step = p_ae_block->exposure_quant * 5;
	}
	else if(abs(p_ae_block->histogram.hist_error) > 220)
	{
		//_step = p_ae_block->exposure_quant * 2;
		_step = p_ae_block->exposure_quant * 3;
		//_step = p_ae_block->exposure_quant * 4;
	}*/
	else
	{
		// 0xA0 <= x >= 0x100
		// 2 ~ 2.5
		//XM_printf ("error=%d, _step=%d\n", p_ae_block->histogram.hist_error, _step);
		//_step = p_ae_block->exposure_quant * 9;
		//_step = p_ae_block->exposure_quant * 2;
		//_step = p_ae_block->exposure_quant * 4;
		//_step = p_ae_block->exposure_quant * 8;
		_step = p_ae_block->exposure_quant * 6;
	}
	
	// ÒÔÏÂ²½·ù½µµÍÒ»°ë»áµ¼ÖÂËíµÀ½ø³öÊÊÓ¦»ºÂı,ÖØĞÂ»Ö¸´ÎªÔ­À´µÄ²½·ù.
	//_step /= 2;
	//_step *= 2;
	
	return _step;
}

#undef XM_printf
#define	XM_printf(...)
void auto_exposure_step_calculate (auto_exposure_ptr_t p_ae_block, cmos_exposure_ptr_t exp_cmos_block)
{
	u32_t _step;
	u32_t _step_1st;
	u32_t ratio = 1;

	u8_t	old_steady_enable = p_ae_block->steady_control.enable;

	_step = auto_exposure_increment_get (p_ae_block);
	
	_step_1st = _step;

	// ¼«¶ËÆØ¹âÌõ¼şĞŞÕı
	// 1) ¸ÃĞŞÕıÔÚÄ³Ğ©¹âÕÕÌõ¼şÏÂÈİÒ×´¥·¢Õñµ´, ĞèÒª½Ï³¤Ê±¼ä²ÅÄÜÆ½ÎÈ.
	//		Ôö¼ÓĞŞÕıÌõ¼şµÄãĞÖµ 16 --> 32
	//	2) ¼õĞ¡½×ÌİÖµ	
	if(p_ae_block->histogram.hist_error < (-32))
	{
		// ÆØ¹âÑÏÖØ¹ı¶È, ÄÜÁ¿Ö÷Òª¼¯ÖĞÔÚ×îÓÒ²à¸ß¹âÇø (0xC0 ~ 0xFF)
		if(p_ae_block->histogram.bands[4].value >= 0xF800)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ75%, 32*0.75 = 24
			XM_printf ("SC-OV-0 0.75\r\n");
			_step = p_ae_block->exposure_quant * 8 * 3;
		}
		// ÄÜÁ¿Ö÷Òª¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xF800)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ50%, 16/32 = 0.5
			XM_printf ("SC-OV-1 0.5\r\n");
			_step = p_ae_block->exposure_quant * 8 * 2 ;
		}
		// ÄÜÁ¿Ö÷Ìå¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xE000)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ43.75%, 14/32 = 0.4375
			XM_printf ("SC-OV-2 0.4375\r\n");
			_step = p_ae_block->exposure_quant * 7 * 2 ;
		}
		// ÄÜÁ¿Ö÷Ìå¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xD000)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ31.25%, 10/32 = 0.3125
			XM_printf ("SC-OV-2 0.3125\r\n");
			_step = p_ae_block->exposure_quant * 5 * 2 ;
		}
		// ÄÜÁ¿Ö÷Ìå¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xC000)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ25%, 8/32 = 0.25
			XM_printf ("SC-OV-2 0.25\r\n");
			_step = p_ae_block->exposure_quant * 4 * 2 ;
		}
		// ÄÜÁ¿Ö÷Ìå¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xB000)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ12.5%, 4/32 = 0.125
			XM_printf ("SC-OV-2 0.125\r\n");
			_step = p_ae_block->exposure_quant * 2 * 2 ;
		}
		// ÄÜÁ¿Ö÷Ìå¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0xA000)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ9.375%, 3/32 = 0.09375
			XM_printf ("SC-OV-2 0.09375\r\n");
			_step = p_ae_block->exposure_quant * 3 ;
		}
		// ÄÜÁ¿Ö÷Ìå¼¯ÖĞÔÚ¸ß¹âÇø (0x80 ~ 0xFF)
		else if( (p_ae_block->histogram.bands[3].value + p_ae_block->histogram.bands[4].value) >= 0x8000)
		{
			// ½«ÆØ¹âÁ¿Ë¥¼õ0.0625, 2/32 = 0.0625
			XM_printf ("SC-OV-2 0.0625\r\n");
			_step = p_ae_block->exposure_quant * 2 * 1 ;
		}
		// µÍ¹âÇø(0x00 ~ 0x3F)ÄÜÁ¿»ù±¾²»´æÔÚ, µÍÓÚ2/256
		else if( (p_ae_block->histogram.bands[0].value + p_ae_block->histogram.bands[1].value) <= 0x0200 )
		{
			// µÍ¹âÇø(0x00 ~ 0x3F)ÄÜÁ¿»ù±¾²»´æÔÚ
			// ½«ÕûÌåÆØ¹âÁ¿Ë¥¼õ25%, 8 / 32 = 0.25
			XM_printf ("SC-OV-2 0.25\r\n");
			_step = p_ae_block->exposure_quant * 8 ;
		}
		// µÍ¹âÇøÄÜÁ¿(0x00 ~ 0x0F)¶ÔÆØ¹âË¥¼õËÙ¶ÈµÄÓ°ÏìºÜµÍ
		// µÍ¹âÇø(0x00 ~ 0x0F)ÄÜÁ¿»ù±¾²»´æÔÚ, µÍÓÚ2/256
		// ÕâÒ»Ìõ¹æÔòÃ»ÓĞ¿¼ÂÇµÍ¹âÇø²»´æÔÚµÄ³¡¾°
		else if( (p_ae_block->histogram.bands[0].value ) <= 0x0200 )
		{
			// µÍ¹âÇø(0x00 ~ 0x0F)ÄÜÁ¿»ù±¾²»´æÔÚ
			// ½«ÕûÌåÆØ¹âÁ¿Ë¥¼õ1/32, 
			XM_printf ("SC-OV-3 0.03125\r\n");
			_step = p_ae_block->exposure_quant * 1 ;
		}
	}
	else if(p_ae_block->histogram.hist_error > 32)
	{

		// ÒÔÏÂ¶ÌÆØ¹â£¬¸ß¹â´¦Îª0, Ö´ĞĞÏÂÃæµÄ¸ß¹âÆØ¹âÕı²¹³¥¹ı³Ì£¬¿ÉÄÜµ¼ÖÂÉÁË¸ÎÊÌâ¡£
		// ¼õĞ¡ÆØ¹â½×ÌİÎªexposure_quant / 2£¬±ÜÃâÉÁË¸
		// ae_hist_error =     29
		// ae_sensor_inttime =         22
		// ae_sensor_again = 1.000000
		// ae_sensor_dgain = 1.000000
		// ae info histoGram :
		// [0] = 2789;
		// [1] =  923;
		// [2] =  383;
		// [3] =    0;
		// [4] =    0;
		
		// ÆØ¹âÑÏÖØ²»×ã, ÄÜÁ¿Ö÷Òª¼¯ÖĞÔÚµÍ¹âÇø(0x00 ~ 0x3F)
		if( (p_ae_block->histogram.bands[0].value + p_ae_block->histogram.bands[1].value) >= 0xFF80 )
		{
			// ÆØ¹âÁ¿Ôö¼Ó2±¶ (*3), ¼´Ê¹Òç³ö, Òç³öµÄ±ÈÀı½ÏµÍ
			if(p_ae_block->histogram.bands[1].value <= 0x4000)
			{
				// 0x10 ~ 0x3FÇøÓòµÄÄÜÁ¿½ÏĞ¡
				//ratio = 8;		// 1024 * 8
				_step = p_ae_block->exposure_quant * 4;
				//_step = p_ae_block->exposure_quant / 2;
				XM_printf ("SC-UE-0 2.00\r\n");
			}
			else
			{
				//ratio = 4;		// 1024 * 4
				_step = p_ae_block->exposure_quant * 2;
				//_step = p_ae_block->exposure_quant / 2;
				XM_printf ("SC-UE-0 1.00\r\n");
			}
		}
		// ¸ß¹âÇø (0x80 ~ 0xFF)µÄ±ÈÀıºÜµÍ
		// ½« ÖĞÉ«µ÷µÄÇøÓò(0x40~0x7F)Ó³Éäµ½(0x80~0xC0)
		else if( (p_ae_block->histogram.bands[4].value + p_ae_block->histogram.bands[3].value) < 0x00C0 )
		{
			// ÆØ¹âÁ¿Ôö¼Ó0.5±¶. (*1.5)
			// ¸ù¾İ ÖĞÉ«µ÷/µÍ¹â µÄ±ÈÀı¾ö¶¨·Å´ó±¶Êı
			if(p_ae_block->histogram.bands[2].value <= 0x1000)
			{
				// 0.375
				//ratio = 3;		// 2048
				_step = p_ae_block->exposure_quant * 4;		// 1024
				//_step = p_ae_block->exposure_quant / 2;
				XM_printf ("SC-UE-1 0.375\r\n");
			}
			else if(p_ae_block->histogram.bands[2].value <= 0x2000)
			{
				// 0.25
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant * 4;		// 1024
				//_step = p_ae_block->exposure_quant / 2;
				XM_printf ("SC-UE-1 0.250\r\n");
			}
			else if(p_ae_block->histogram.bands[2].value <= 0x4000)
			{
				// 0.125
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant * 2;		// 512
				//_step = p_ae_block->exposure_quant / 2;
				XM_printf ("SC-UE-1 0.125\r\n");
			}
			else if(p_ae_block->histogram.bands[2].value <= 0x8000)
			{
				// 0.0625
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant * 1;		// 256
				//_step = p_ae_block->exposure_quant / 2;
				XM_printf ("SC-UE-1 0.0625\r\n");
			}
			else
			{
			}
		}
		
		// ×îÓÒ²à¸ß¹âÇø (0xC0 ~ 0xFF)µÄ±ÈÀıºÜµÍ
		// ¸ß¹âÇø (0xC0 ~ 0xFF)¶ÔÆØ¹âÔöÇ¿ËÙ¶ÈµÄÓ°Ïì·Ç³£ÓĞÏŞ
		else if( p_ae_block->histogram.bands[4].value < 0x00C0)
		{
			if(p_ae_block->increment < p_ae_block->increment_offset)
			{
				// 
			}
			else
			{
				// ÆØ¹âÁ¿Ôö¼Ó1/64
				//ratio = 1;		// 2048
				_step = p_ae_block->exposure_quant/2;		// 64
				XM_printf ("SC-UE-2 0.016\r\n");
			}
		}
		
	}
	
	if(_step < _step_1st)
		_step = _step_1st;

	/*
	// ÆØ¹âÄæ×ª¹ı³ÌÔöÁ¿Ë¥¼õ¿ØÖÆ
	if(p_ae_block->increment > p_ae_block->increment_offset && p_ae_block->histogram.hist_error < 0)
	{
		// ÆØ¹âÔö¼Ó¹ı³ÌÖĞ,¼ì²âµ½ÆØ¹â¹ı¶È. (¼´ÆØ¹â¹ı³ÌÄæ×ª, ÆØ¹â·´ÏàË¥¼õÖĞ)
		// ½«²½³¤ËõÖÁ¼ÆËãÔöÁ¿µÄ1/4
		_step = _step/4;
		// ½øÒ»²½¼ì²éÉÏÃæĞŞÕıµÄ²½³¤ÊÇ·ñ´óÓÚ×î½üµÄ²½³¤, ÒòÎª×î½üµÄ²½³¤µ¼ÖÂÆØ¹â¹ı³ÌÄæ×ª, 
		// ÄÇÃ´ĞÂµÄÆØ¹â·´ÏàË¥¼õ¹ı³ÌÖĞ, ²½³¤Ó¦²»´óÓÚ×î½üµÄ²½³¤
		if(_step > (u32_t)p_ae_block->increment_step/2)
			_step = p_ae_block->increment_step/2;

		// Ê¹ÄÜÔöÁ¿×èÄá
		p_ae_block->increment_damping = 1;

		if(p_ae_block->steady_control.enable == 0)
		{
			if(p_ae_block->steady_control.count == 0)
			{
				// ±£Áôµ±Ç°µÄÆØ¹â²Î¿¼
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			else if(abs(p_ae_block->steady_control.steady_error) > abs(p_ae_block->histogram.hist_error))
			{
				// ±£Áô¾ø¶ÔÖµÏÂµÄÆØ¹â²Î¿¼
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			p_ae_block->steady_control.count ++;

			if(p_ae_block->steady_control.count >= 3)
			{
				p_ae_block->steady_control.count = 0;
				p_ae_block->steady_control.enable = 1;

				XM_printf ("\t\t Steady Enabel 0, error=%d, exposure=%d\r\n", 
					p_ae_block->steady_control.steady_error,
					p_ae_block->steady_control.exposure);
			}
		}
	}
	else if(p_ae_block->increment < p_ae_block->increment_offset && p_ae_block->histogram.hist_error > 0)
	{
		// ÆØ¹â¼õÉÙ¹ı³ÌÖĞ,¼ì²âµ½ÆØ¹â²»¹». (¼´ÆØ¹â¹ı³ÌÄæ×ª, ÆØ¹â·´ÏàÔö¼ÓÖĞ)
		// ½«²½³¤ËõÖÁ¼ÆËãÔöÁ¿µÄ1/4
		_step = _step/4;
		// ½øÒ»²½¼ì²éÉÏÃæĞŞÕıµÄ²½³¤ÊÇ·ñ´óÓÚ×î½üµÄ²½³¤, ÒòÎª×î½üµÄ²½³¤µ¼ÖÂÆØ¹â¹ı³ÌÄæ×ª, 
		// ÄÇÃ´ĞÂµÄÆØ¹â·´ÏàÔö³¤¹ı³ÌÖĞ, ²½³¤Ó¦²»´óÓÚ×î½üµÄ²½³¤
		if(_step > (u32_t)p_ae_block->increment_step/2)
			_step = p_ae_block->increment_step/2;

		// Ê¹ÄÜÔöÁ¿×èÄá
		p_ae_block->increment_damping = 1;

		if(p_ae_block->steady_control.enable == 0)
		{
			if(p_ae_block->steady_control.count == 0)
			{
				// ±£Áôµ±Ç°µÄÆØ¹â²Î¿¼
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			else if(abs(p_ae_block->steady_control.steady_error) > abs(p_ae_block->histogram.hist_error))
			{
				// ±£Áô¾ø¶ÔÖµÏÂµÄÆØ¹â²Î¿¼
				p_ae_block->steady_control.steady_error = p_ae_block->histogram.hist_error;
				p_ae_block->steady_control.exposure = exp_cmos_block->last_exposure;
			}
			p_ae_block->steady_control.count ++;
			if(p_ae_block->steady_control.count >= 3)
			{
				p_ae_block->steady_control.count = 0;
				p_ae_block->steady_control.enable = 1;
			
				XM_printf ("\t\t Steady Enabel 1, error=%d, exposure=%d\r\n", 
					p_ae_block->steady_control.steady_error,
					p_ae_block->steady_control.exposure);
			}
		}
	}
	else if(p_ae_block->increment_damping)
	{
		// ¹Ø±ÕÔöÁ¿×èÄá
		_step = _step/2;
		p_ae_block->increment_damping = 0;

		if(p_ae_block->steady_control.enable == 0)
			p_ae_block->steady_control.count = 0;
	}
	else
	{
		if(p_ae_block->steady_control.enable == 0)
			p_ae_block->steady_control.count = 0;

	}
	*/

#if 0
	_step = _step / 16;

	if(_step < 1) 
		_step = 1;
#else
	//_step = (_step + 8) / 16;
//	_step = (_step + 4) / 8;

	if(_step < 1) 
		_step = 1;
#endif

	p_ae_block->increment_step = (u16_t)_step;
	p_ae_block->increment_ratio = (u16_t)ratio;

	if(p_ae_block->increment_offset < _step)
		p_ae_block->increment_min = 1;
	else
		p_ae_block->increment_min = p_ae_block->increment_offset - _step;
	p_ae_block->increment_max = p_ae_block->increment_offset + _step * ratio;


	if(p_ae_block->histogram.hist_error > 8)
	//if(p_ae_block->histogram.hist_error > 4)
	//if(p_ae_block->histogram.hist_error > 2)
	{
		// Ö±·½Í¼ÕûÌåÆ«×ó, Ôö¼ÓÆØ¹â
		p_ae_block->increment = p_ae_block->increment_max;
	}
	else if(p_ae_block->histogram.hist_error < -8)
	//else if(p_ae_block->histogram.hist_error < -4)
	//else if(p_ae_block->histogram.hist_error < -2)
	{
		// Ö±·½Í¼ÕûÌåÆ«ÓÒ, ¼õÉÙÆØ¹â
		p_ae_block->increment = p_ae_block->increment_min;
	}
	else
	{
		// Ö±·½Í¼ÕûÌåÆ½ºâ, ±£³Öµ±Ç°ÆØ¹âÖµ
		p_ae_block->increment = p_ae_block->increment_offset;
	}


	/*
	// ÎÈÌ¬¿ØÖÆ¹ı³ÌÏÂµÄ½â³ıÎÈÌ¬¿ØÖÆ¼ì²é
	if(p_ae_block->steady_control.enable)
	{
		// ¼ì²éÊÇ·ñÊÇ½øÈëÎÈÌ¬¿ØÖÆ¹ı³ÌºóµÄµÚÒ»´ÎÆØ¹â
		if(old_steady_enable == 1)
		{
			// ½øÈëÎÈÌ¬¿ØÖÆ¹ı³ÌºóµÄ·ÇµÚÒ»´ÎÆØ¹â

			// ¼ì²éÎÈÌ¬¿ØÖÆÆØ¹â´íÎóµÄ±ß½çãĞÖµÊÇ·ñ³¬³ö
			if(	abs(p_ae_block->steady_control.steady_error - p_ae_block->histogram.hist_error) >= STEADY_CONTROL_AE_ERR_THREAD )
			{
				// ÆØ¹â´íÎóÎó²îÒÑ³¬³öÎÈÌ¬¿ØÖÆµÄ·¶Î§, ½â³ıÎÈÌ¬¿ØÖÆ
				// ¹Ø±ÕÎÈÌ¬¿ØÖÆ
				p_ae_block->steady_control.enable = 0;
				p_ae_block->steady_control.count = 0;

				XM_printf ("\t\t Steady disable\r\n");
			}
		}
	}

	// ÎÈÌ¬¿ØÖÆ¹ı³ÌÏÂµÄÆØ¹âÖµ¼°ÔöÁ¿Éè¶¨
	if(p_ae_block->steady_control.enable)
	{
		// ½øÈëÎÈÌ¬×´Ì¬¹ı³Ìºó, Ëø¶¨ÎÈÌ¬¹ı³ÌµÄÃ¿´ÎÆØ¹âµÄÆØ¹âÖµÎªÍ¬Ò»¸öÆØ¹âÖµÉè¶¨, ÕâÑù±ÜÃâÆØ¹âÖµ·ÇÎÈÌ¬µ¼ÖÂµÄÉÁË¸
		exp_cmos_block->exposure = p_ae_block->steady_control.exposure;
		// Ëø¶¨ÆØ¹âÔöÁ¿Îª1
		p_ae_block->increment = p_ae_block->increment_offset;

	//	XM_printf ("\t\t Steady Control, exposure=%d, inc=%d\r\n", exp_cmos_block->exposure, 	p_ae_block->increment);
	}
	*/

	/*
	XM_printf ("\t\tstep = %4d, offset = %10d, max = %10d, min = %10d\n", 
		_step, 
		p_ae_block->increment_offset, 
		p_ae_block->increment_max,
		p_ae_block->increment_min);
		*/
}

static int cmos_exposure_get_error (cmos_exposure_ptr_t exp_cmos_block, int index)
{
	if(exp_cmos_block->stat_count < AE_STAT_COUNT)
		return 0;
	index = exp_cmos_block->stat_index - 1 - index;
	if(index < 0)
		index += AE_STAT_COUNT;
	return exp_cmos_block->stat_error[index];
}

#define min(a,b) ((a < b) ? a : b)
#define max(a,b) ((a > b) ? a : b)

static int steady_ae_enable = 1;		// AEÎÈÌ¬¿ØÖÆÊ¹ÄÜ, Ïû³ıÆØ¹âÕñµ´ÏÖÏó

void cmos_exposure_set_steady_ae (int enable)
{
	steady_ae_enable = enable;
}

extern isp_ae_t p_ae;

static int auto_exposure_increment_calculate (auto_exposure_ptr_t p_ae_block, cmos_exposure_ptr_t exp_cmos_block)
{
	//histogram_update(p_ae_block, &(p_ae_block->histogram), &p_ae);
	histogram_3x3_update (p_ae_block, &(p_ae_block->histogram), &p_ae);
	
	//isp_auto_exposure_compensation(p_ae_block, p_ae_block->histogram.bands);
	
	if(steady_ae_enable)
	{
		if(exp_cmos_block->stat_count < AE_STAT_COUNT)
		{
			exp_cmos_block->stat_error[exp_cmos_block->stat_count] = p_ae_block->histogram.hist_error;
			exp_cmos_block->stat_lum[exp_cmos_block->stat_count] = p_ae.lumCurr;	// isp_ae_lum_read ();
			exp_cmos_block->stat_count ++;
		}
		else
		{
			unsigned int index = exp_cmos_block->stat_index;
			int error_0, error_1, error_2;
			int diff, min_error, max_error;
			exp_cmos_block->stat_error[index] = p_ae_block->histogram.hist_error;
			exp_cmos_block->stat_lum[index] = p_ae.lumCurr;	//isp_ae_lum_read ();
			index ++;
			if(index >= AE_STAT_COUNT)
				index = 0;
			exp_cmos_block->stat_index = index;
			
			// ¶ÁÈ¡×î½üµÄAE´íÎóÖµ
			error_0 = cmos_exposure_get_error(exp_cmos_block, 0);
			error_1 = cmos_exposure_get_error(exp_cmos_block, 1);
			error_2 = cmos_exposure_get_error(exp_cmos_block, 2);
			
			// 20181023 ¼ì²éÒì³£Çé¿ö
			// 1) ÈôÕûÌåhist_error_oneÓë¼«Ğ¡Öµhist_error_min¼«ĞÔÒ»ÖÂ£¬»­ÃæÓ¦´æÔÚ½ÏÃ÷ÏÔµÄ¹ıÆØ»òÕßÆØ¹â²»¹»£¬´ËÊ±Ó¦Á¢¿Ì½â³ıÎÈÌ¬¿ØÖÆ,
			//		·ñÔò»­Ãæ»á³öÏÖÃ÷ÏÔÆ«°µµÄ³¡¾°
			if(p_ae_block->histogram.hist_error_min < -6 && p_ae_block->histogram.hist_error_one < -6)
			{
				// ½â³ıÎÈÌ¬¿ØÖÆ
				exp_cmos_block->locked = 0;
				exp_cmos_block->locked_threshhold = 0;
				goto step_calc;
			}
			else if(p_ae_block->histogram.hist_error_min > 6 && p_ae_block->histogram.hist_error_one > 6)
			{
				// ½â³ıÎÈÌ¬¿ØÖÆ
				exp_cmos_block->locked = 0;
				exp_cmos_block->locked_threshhold = 0;
				goto step_calc;
			}
			
			
			// ¼ì²éÎÈÌ¬¿ØÖÆÊÇ·ñÒÑÓ¦ÓÃ
			if(exp_cmos_block->locked)
			{
				// Õñµ´±£»¤ÖĞ
				if(exp_cmos_block->locked_threshhold > 0)
				{
					if(error_0 > 0 && error_0 <= exp_cmos_block->locked_threshhold)
					{
						// Ğ¡ÓÚãĞÖµ, ±»×èÖ¹
						XM_printf ("error = %d, locked_threshhold = %d, blocking\n", error_0, exp_cmos_block->locked_threshhold);
						return 1;
					}
					else
					{
						// ×èÖ¹½â³ı
						XM_printf ("error = %d, locked_threshhold = %d, blocking leave\n", error_0, exp_cmos_block->locked_threshhold);
						exp_cmos_block->locked = 0;
						exp_cmos_block->locked_threshhold = 0;
					}
				}
				else 
				{
					if(error_0 < 0 && error_0 >= exp_cmos_block->locked_threshhold)
					{
						// ´óÓÚãĞÖµ, ±»×èÖ¹
						XM_printf ("error = %d, locked_threshhold = %d, blocking\n", error_0, exp_cmos_block->locked_threshhold);
						return 1;
					}
					else
					{
						// ×èÖ¹½â³ı
						XM_printf ("error = %d, locked_threshhold = %d, blocking leave\n", error_0, exp_cmos_block->locked_threshhold);
						exp_cmos_block->locked = 0;
						exp_cmos_block->locked_threshhold = 0;
					}
				}
			}
			else
			{
				// ÅĞ¶ÏÊÇ·ñ´æÔÚÕñµ´
				if(error_0 > 0 && error_1 < 0  && error_2 > 0 )
				{
					// Õñµ´, 11, -29, 10
					exp_cmos_block->locked = 1;
					diff = abs(error_0 - error_2);
					max_error = max(error_0, error_2);
					exp_cmos_block->locked_threshhold = max_error + max(3, diff);
					if(exp_cmos_block->locked_threshhold > 255)
					{
						// ¸´Î»
						exp_cmos_block->stat_count = 0;
						exp_cmos_block->locked = 0;
						exp_cmos_block->locked_threshhold = 0;
						goto  step_calc;
					}
					XM_printf ("error = %d, locked_threshhold = %d, blocking enter\n", error_0, exp_cmos_block->locked_threshhold);
					return 1;
				}
				else if(error_0 < 0 && error_1 > 0  && error_2 < 0 )
				{
					// Õñµ´, -9, 11, -11
					exp_cmos_block->locked = 1;
					diff = abs(error_0 - error_2);
					min_error = min(error_0, error_2);
					exp_cmos_block->locked_threshhold = min_error - max(3, diff);
					if(exp_cmos_block->locked_threshhold < -255)
					{
						// ¸´Î»
						exp_cmos_block->stat_count = 0;
						exp_cmos_block->locked = 0;
						exp_cmos_block->locked_threshhold = 0;
						goto  step_calc;
					}
					XM_printf ("error = %d, locked_threshhold = %d, blocking enter\n", error_0, exp_cmos_block->locked_threshhold);
					return 1;
				}
			}
				
		}
	}

step_calc:
	auto_exposure_step_calculate(p_ae_block, exp_cmos_block);
	
	return 0;
}

int isp_auto_exposure_run (auto_exposure_ptr_t ae_block, cmos_exposure_ptr_t exp_cmos_block, int man_exp)
{
	i64_t i64_temp;
	u32_t new_physical_exposure;		// ĞÂµÄ¾ø¶Ô(ÎïÀí)ÆØ¹âÁ¿
	int ae_stable = 1;
	if(man_exp == 1)
	{
		exp_cmos_block->exposure = ae_block->exposure_target;
	}
	
	exp_cmos_block->last_exposure = exp_cmos_block->exposure;


	// ¹À¼ÆÆØ¹âÁ¿
	int blocking = auto_exposure_increment_calculate(ae_block, exp_cmos_block);
	if(blocking)
	{
		return ae_stable;
	}
	
	if(man_exp == 1)
	{
		ae_block->increment = ae_block->increment_offset;
	}

	// ¸ù¾İÆØ¹âÁ¿, ÖØĞÂ¼ÆËãsensorµÄÆØ¹â²ÎÊı
	isp_exposure_cmos_calculate (
				exp_cmos_block,
				ae_block->increment,
				ae_block->increment_max,
				ae_block->increment_min,
				ae_block->increment_offset
				);

	i64_temp = exp_cmos_block->cmos_inttime.exposure_ashort * exp_cmos_block->sys_factor;
	i64_temp *= exp_cmos_block->cmos_gain.again * exp_cmos_block->cmos_gain.dgain;
	i64_temp >>= (exp_cmos_block->cmos_gain.again_shift + exp_cmos_block->cmos_gain.dgain_shift);
	new_physical_exposure = (u32_t)i64_temp;

	// 20170102 zhuoyonghong
	// Îª±ÜÃâ³öÏÖÆØ¹â²»ÎÈµÄÏÖÏó, ²»ÔÙÖ´ĞĞÂß¼­ÆØ¹â¼ÓËÙ¹ı³Ì(Ñ­»·Ö´ĞĞÂß¼­ÆØ¹â¹ı³ÌÖ±ÖÁÎïÀíÆØ¹â³öÏÖ²îÒì).
	// Âß¼­ÆØ¹â¹ı³ÌµÄÀíÂÛÆØ¹âÖµ²»µÈÓÚÊµ¼ÊµÄÎïÀíÆØ¹âÁ¿, »áÒıÈëÎó²î(ÎïÀíÆØ¹âÁ¿-ÀíÂÛÆØ¹âÁ¿). 
	// Á¬Ğø¶à´ÎÂß¼­ÆØ¹â¹ı³Ì²úÉúµÄÎïÀíÆØ¹âÁ¿Îó²î(ÎïÀíÆØ¹âÁ¿-ÀíÂÛÆØ¹âÁ¿)»á¸ü´ó.
	// Òò´ËÈ¡Ïû¸ÃÂß¼­ÆØ¹â¼ÓËÙ¹ı³Ì, Ã¿´ÎÖğ²½Ö´ĞĞ 
	// 	1)Âß¼­ÆØ¹âÁ¿¼ÆËã-->2)ÎïÀíÆØ¹âÁ¿¼ÆËã-->3)ÎïÀíÆØ¹â -->4ÆØ¹â·´À¡
	// µÄ¹ı³Ì, ±ÜÃâ½Ï´óÆØ¹âÁ¿Îó²îµÄÒıÈë. ½Ï´óÆØ¹âÁ¿Îó²î»áµ¼ÖÂ»­ÃæÁÁ¶È¶¶¶¯
#if 0
	if(man_exp == 0)
	{
		// ×Ô¶¯ÆØ¹âÄ£Ê½ÏÂ
		//int loop = 16;
		int loop = 1;
		int same_exp = 0;
		// Ñ­»·Ö±µ½ÎïÀíÆØ¹âÁ¿´æÔÚ±ä»¯
		while (loop > 0 && new_physical_exposure == exp_cmos_block->physical_exposure)
		{
			if(new_physical_exposure == exp_cmos_block->exp_llimit)		// ×îĞ¡ÆØ¹âÁ¿
				break;
			// Ê¹ÓÃÓë½øÈëÊ±Í¬ÑùµÄÆØ¹âÁ¿ÔöÁ¿²ÎÊı, ¼ÆËãÏÂÒ»¸öÀíÂÛµÄÆØ¹âÁ¿
			// auto_exposure_step_calculate (ae_block, exp_cmos_block);
			// ¸ù¾İ¼ÆËãµÄÀíÂÛÆØ¹âÁ¿, ÖØĞÂ¼ÆËãsensorµÄÆØ¹â²ÎÊı
			same_exp = isp_exposure_cmos_calculate (
						exp_cmos_block,
						ae_block->increment,
						ae_block->increment_max,
						ae_block->increment_min,
						ae_block->increment_offset
						);
			if(same_exp)
				break;
			// 
			i64_temp = exp_cmos_block->cmos_inttime.exposure_ashort * exp_cmos_block->sys_factor;
			i64_temp *= exp_cmos_block->cmos_gain.again * exp_cmos_block->cmos_gain.dgain;
			i64_temp >>= (exp_cmos_block->cmos_gain.again_shift + exp_cmos_block->cmos_gain.dgain_shift);
			new_physical_exposure = (u32_t)i64_temp;
			loop --;
		}

		if(new_physical_exposure == exp_cmos_block->physical_exposure)
		{
			// Á¬ĞøµÄ¾ø¶ÔÆØ¹âÁ¿ÏàÍ¬
			if(	(ae_block->increment == ae_block->increment_offset)
				|| (new_physical_exposure == exp_cmos_block->exp_llimit)
				|| (exp_cmos_block->exposure == exp_cmos_block->exp_tlimit)		
				|| (same_exp == 1)
				)
				ae_stable = 1;
			else
				ae_stable = 0;
		}
		else
			ae_stable = 0;
	}
#else
	// ±È½ÏĞÂµÄÎïÀíÆØ¹âÖµÓë×î½üµÄÖµ±È½Ï. ÏàÍ¬ÔòÈÏÎªÆØ¹âÎÈ¶¨
	if(exp_cmos_block->physical_exposure == new_physical_exposure)
		ae_stable = 1;
	else
		ae_stable = 0;
#endif

	exp_cmos_block->physical_exposure = new_physical_exposure;
	
	/*
	XM_printf ("\t\tlog_exposure=%10d, inttime=%5d, again=%f, dgain=%f\n", 
		(u32_t)new_physical_exposure,
		exp_cmos_block->cmos_inttime.exposure_ashort,
		((float)exp_cmos_block->cmos_gain.again) / (1 << exp_cmos_block->cmos_gain.again_shift),
		((float)exp_cmos_block->cmos_gain.dgain) / (1 << exp_cmos_block->cmos_gain.dgain_shift));
		*/

	// µ÷ÕûsensorµÄÆØ¹â²ÎÊı
	exp_cmos_block->exposure_delay_cycle = isp_cmos_inttime_update (exp_cmos_block);

	return ae_stable;
}

#define MAX_EXPOSURE_LUTS 3
static u16_t exposure_luts_defaults[MAX_EXPOSURE_LUTS][4] =
{ 
	{0xA000, 0x1400, 0x0180, 0x0110 },
	{0x8000, 0x4000, 0x0800, 0x0100 },
	{0x2000, 0x3000, 0x0700, 0x0220 } 
};

void auto_exposure_lut_load (auto_exposure_ptr_t p_ae_block, u8_t lut)
{
	int i;
	if(MAX_EXPOSURE_LUTS <= lut)
	{
		return;
	}

	for(i=0;i<HISTOGRAM_BANDS;++i)
	{
		p_ae_block->histogram.bands[i].target  = exposure_luts_defaults[lut][i];
	}
}