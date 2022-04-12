// =============================================================================
// File        : Gem_isp_ae.c
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#include "Gem_isp_ae.h"
#include "Gem_isp_io.h"
#include "Gem_isp_sensor.h"
#include "ark_isp_exposure_cmos.h"
#include "ark_isp_exposure.h"
#include "Gem_isp_eris.h"

// ae:etime_H8:0 etime_M8:20 etime_L8:48 again8:0 dgain8:64

extern cmos_exposure_t isp_exposure;
//static OS_EVENT ae_event;


void isp_ae_event_init (void)
{
	//OS_EVENT_Create (&ae_event);
}

void isp_ae_event_exit (void)
{
	//OS_EVENT_Delete (&ae_event);
}

void isp_ae_event_set (void)
{
	//OS_EVENT_Set (&ae_event);
}

void isp_ae_event_reset (void)
{
	//OS_EVENT_Reset (&ae_event);
}

void isp_ae_event_wait (void)
{
	//OS_EVENT_Wait (&ae_event);
	//if(OS_EVENT_WaitTimed (&ae_event, 60*1000))		// 60s
	//{
	//	XM_printf ("timeout, AE can't stable within 60s\n");
	//}
}


void isp_ae_window_weight_write (auto_exposure_ptr_t ae, u8_t win_weight[3][3])
{
	unsigned int data;
	ae->window_weight[0][0] = win_weight[0][0];
	ae->window_weight[0][1] = win_weight[0][1];
	ae->window_weight[0][2] = win_weight[0][2];
	ae->window_weight[1][0] = win_weight[1][0];
	ae->window_weight[1][1] = win_weight[1][1];
	ae->window_weight[1][2] = win_weight[1][2];
	ae->window_weight[2][0] = win_weight[2][0];
	ae->window_weight[2][1] = win_weight[2][1];
	ae->window_weight[2][2] = win_weight[2][2];
	data = (ae->window_weight[0][0]<<0) | (ae->window_weight[0][1]<<4) |
         (ae->window_weight[0][2]<<8) | (ae->window_weight[1][0]<<12) |
         (ae->window_weight[1][1]<<16) | (ae->window_weight[1][2]<<20) |
         (ae->window_weight[2][0]<<24) | (ae->window_weight[2][1]<<28);
	Gem_write ((GEM_AE0_BASE+0x04), data);

	data = (ae->window_weight[2][2]<<0);
	Gem_write ((GEM_AE0_BASE+0x08), data);
}

// �ֶ���ֵд��
void isp_histogram_bands_write (u8_t histoBand[4])
{
	unsigned int data;
	data = (histoBand[0]<<0) | (histoBand[1]<<8) |
         (histoBand[2]<<16) | (histoBand[3]<<24);
	Gem_write ((GEM_AE0_BASE+0x00), data);
}

static volatile unsigned int isp_hist_data1 = 0;
static volatile unsigned int isp_hist_data2 = 0;
void isp_histogram_bands_data (unsigned int data1, unsigned int data2)
{
	isp_hist_data1 = data1;
	isp_hist_data2 = data2;
}

// 5����Ϣ����
void isp_histogram_bands_read (histogram_band_t bands[5], isp_ae_ptr_t  isp_ae)
{
#if 0
	unsigned int data1, data2;
	//XM_lock();
	data1 = isp_hist_data1; //Gem_read (GEM_AE1_BASE+0x04);
	data2 = isp_hist_data2;	//Gem_read (GEM_AE1_BASE+0x08);
	//XM_unlock ();

	bands[0].value = ((data1 >>  0) & 0xfff) << 4;
	bands[1].value = ((data1 >> 16) & 0xfff) << 4;
	bands[2].value = 0;
	bands[3].value = ((data2 >>  0) & 0xfff) << 4;
	bands[4].value = ((data2 >> 16) & 0xfff) << 4;
#else
	//XM_lock();
	bands[0].value = isp_ae->histoGram[0] << 4;
	bands[1].value = isp_ae->histoGram[1] << 4;
	bands[2].value = 0;
	bands[3].value = isp_ae->histoGram[3] << 4;
	bands[4].value = isp_ae->histoGram[4] << 4;
	//XM_unlock ();
#endif
}

u32_t isp_ae_lum_read (void)
{
	return Gem_read (GEM_AE1_BASE+0x00) & 0xFF;
}


void isp_ae_init (isp_ae_ptr_t p_ae)
{
	arkn141_isp_ae_initialize (&isp_exposure);
  //isp_ae_init_io (p_ae);

  //XM_printf ("\r\np_ae->opt_jitterxx no.1= ", p_ae->opt_jitter);
}

void isp_ae_init_io (isp_ae_ptr_t p_ae)
{
  unsigned int data;

  //data = 256 & 0x0000ffff;
  //Gem_write ((GEM_AE0_BASE+0x00), data);

  data = (p_ae->histoBand[0]<<0) | (p_ae->histoBand[1]<<8) |
         (p_ae->histoBand[2]<<16) | (p_ae->histoBand[3]<<24);
  Gem_write ((GEM_AE0_BASE+0x00), data);

  data = (p_ae->winWeight[0][0]<<0) | (p_ae->winWeight[0][1]<<4) |
         (p_ae->winWeight[0][2]<<8) | (p_ae->winWeight[1][0]<<12) |
         (p_ae->winWeight[1][1]<<16) | (p_ae->winWeight[1][2]<<20) |
         (p_ae->winWeight[2][0]<<24) | (p_ae->winWeight[2][1]<<28);
  Gem_write ((GEM_AE0_BASE+0x04), data);

  data = (p_ae->winWeight[2][2]<<0);
  Gem_write ((GEM_AE0_BASE+0x08), data);

}

static int ae_info_output_enable = 0;
isp_ae_frame_record_t ae_rcd;

void isp_ae_info_output_enable (int enable)
{
	ae_info_output_enable = enable;
}

void isp_ae_run (isp_ae_ptr_t p_ae)
{
	int ret;
	isp_ae_frame_record_t* ae_record = &ae_rcd;

	// ����Զ�AE�Ƿ���
	if(!isp_get_auto_run_state(ISP_AUTO_RUN_AE))
		goto ae_info_output;

	ret = arkn141_isp_ae_run (&isp_exposure);
	if(ret)	// �ع����ȶ�
	{
		isp_ae_event_set ();
	}

		ae_record->hist[0] = isp_exposure.cmos_ae.histogram.bands[0].value;
		ae_record->hist[1] = isp_exposure.cmos_ae.histogram.bands[1].value;
		ae_record->hist[2] = isp_exposure.cmos_ae.histogram.bands[2].value;
		ae_record->hist[3] = isp_exposure.cmos_ae.histogram.bands[3].value;
		ae_record->hist[4] = isp_exposure.cmos_ae.histogram.bands[4].value;

		ae_record->balance = isp_exposure.cmos_ae.histogram.hist_balance;
		ae_record->error = isp_exposure.cmos_ae.histogram.hist_error;
		//ae_record->lum = (u16_t)isp_ae_lum_read();
		ae_record->lum = p_ae->lumCurr;	// (u16_t)isp_ae_lum_read();

		ae_record->ae_compensation = isp_exposure.cmos_ae.ae_compensation;
		ae_record->ae_black_target = isp_exposure.cmos_ae.ae_black_target;
		ae_record->ae_bright_target = isp_exposure.cmos_ae.ae_bright_target;

		ae_record->increment_step = isp_exposure.cmos_ae.increment_step;
		ae_record->increment_offset = isp_exposure.cmos_ae.increment_offset;
		ae_record->increment_max = isp_exposure.cmos_ae.increment_max;
		ae_record->increment_min = isp_exposure.cmos_ae.increment_min;

		ae_record->exposure = isp_exposure.exposure;
		ae_record->last_exposure = isp_exposure.last_exposure;
		ae_record->increment = isp_exposure.cmos_ae.increment;

		ae_record->physical_exposure = isp_exposure.physical_exposure;	// ���һ�ε������ع�ֵ
		ae_record->sensor_inttime = isp_exposure.cmos_inttime.exposure_ashort;
		ae_record->sensor_again = isp_exposure.cmos_gain.again;
		ae_record->sensor_dgain = isp_exposure.cmos_gain.dgain;
		ae_record->sensor_aindex = isp_exposure.cmos_gain.aindex;
		ae_record->sensor_dindex = isp_exposure.cmos_gain.dindex;
		ae_record->sensor_again_shift = isp_exposure.cmos_gain.again_shift;
		ae_record->sensor_dgain_shift = isp_exposure.cmos_gain.dgain_shift;

		ae_record->hist_error_min = isp_exposure.cmos_ae.histogram.hist_error_min;
		ae_record->hist_error_max = isp_exposure.cmos_ae.histogram.hist_error_max;
		ae_record->hist_error_one = isp_exposure.cmos_ae.histogram.hist_error_one;
		ae_record->hist_error_last = isp_exposure.cmos_ae.histogram.hist_error_last;


ae_info_output:
	// �����ع���Ϣ���
	if(ae_info_output_enable )
	{
		//XM_printf ("\nexp_count = %d\n", ae_record->index);
		XM_printf ("\nbalance = %6d, error = %6d, lum=%d, min=%d, max=%d, one=%d, last=%d\n",
					  ae_record->balance, ae_record->error, ae_record->lum,
					  ae_record->hist_error_min, ae_record->hist_error_max, ae_record->hist_error_one, ae_record->hist_error_last);
		XM_printf ("compensation = %3d, black = %d, bright = %d\n",
					  ae_record->ae_compensation,
					  ae_record->ae_black_target,
					  ae_record->ae_bright_target
						  );

		XM_printf ("\thist[5]= 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
					  ae_record->hist[0],
					  ae_record->hist[1],
					  ae_record->hist[2],
					  ae_record->hist[3],
					  ae_record->hist[4]);

		XM_printf ("\tstep = %4d, offset = %10d, max = %10d, min = %10d\n",
				ae_record->increment_step,
				ae_record->increment_offset,
				ae_record->increment_max,
				ae_record->increment_min);

		XM_printf ("\texposure = %10d, new_exposure = %10d, increment = %10d\n",
				ae_record->last_exposure,
				ae_record->exposure,
				ae_record->increment);

		XM_printf ("\tlog_exposure=%10d, inttime=%5d, again*1024=%d, dgain*1024=%d\n",
				(u32_t)ae_record->physical_exposure,
				ae_record->sensor_inttime,
				ae_record->sensor_again * 1024 / (1 << ae_record->sensor_again_shift),
				ae_record->sensor_dgain * 1024 / (1 << ae_record->sensor_dgain_shift)
				);
	}
}

#if 0
void isp_ae_get_current_exp (unsigned int *inttime,
									  float *again,
									  float *dgain
									)
{
	//OS_EnterRegion ();		// ��ֹ�����л�, ��������һ����
	*inttime = isp_exposure.cmos_inttime.exposure_ashort;
	*again = (float)isp_exposure.cmos_gain.again / (1 << isp_exposure.cmos_gain.again_shift);
	*dgain = (float)isp_exposure.cmos_gain.dgain / (1 << isp_exposure.cmos_gain.dgain_shift);
	//OS_LeaveRegion ();
}

// �����ֶ��ع����, ����һ���Բ���
void isp_ae_set_current_exp (unsigned int inttime,
									  float again,
									  float dgain
									)
{
	//OS_EnterRegion ();		// ��ֹ�����л�, ��������һ����
	isp_exposure.cmos_inttime.exposure_ashort = inttime;
	isp_exposure.cmos_gain.again = (unsigned int)(again * (1 << isp_exposure.cmos_gain.again_shift));
	isp_exposure.cmos_gain.dgain = (unsigned int)(dgain * (1 << isp_exposure.cmos_gain.dgain_shift));
	isp_cmos_inttime_update_manual (&isp_exposure);
	//OS_LeaveRegion ();
}
#endif

void isp_ae_get_max_gain (unsigned int *again, unsigned int *dgain)
{
	//OS_EnterRegion ();
	if(isp_exposure.cmos_sensor.cmos_max_gain_get)
		isp_exposure.cmos_sensor.cmos_max_gain_get (&isp_exposure.cmos_gain, again, dgain);
	//OS_LeaveRegion ();
}

void isp_ae_set_max_gain (unsigned int again, unsigned int dgain)
{
	//OS_EnterRegion ();
	if(isp_exposure.cmos_sensor.cmos_max_gain_set)
		isp_exposure.cmos_sensor.cmos_max_gain_set (&isp_exposure.cmos_gain, again, dgain);
	//OS_LeaveRegion ();
}


void isp_ae_info_read (isp_ae_ptr_t p_ae)
{
  unsigned char i, j;
  unsigned int data0, data1, data2;

  // histoBand
  data0 = Gem_read (GEM_AE0_BASE+0x00);
  p_ae->histoBand[0] = (unsigned char)((data0 >>  0));
  p_ae->histoBand[1] = (unsigned char)((data0 >>  8));
  p_ae->histoBand[2] = (unsigned char)((data0 >> 16));
  p_ae->histoBand[3] = (unsigned char)((data0 >> 24));

  // winWeight
  data0 = Gem_read (GEM_AE0_BASE+0x04);
  p_ae->winWeight[0][0] = (unsigned char)((data0 >>  0) & 0x0F);
  p_ae->winWeight[0][1] = (unsigned char)((data0 >>  4) & 0x0F);
  p_ae->winWeight[0][2] = (unsigned char)((data0 >>  8) & 0x0F);
  p_ae->winWeight[1][0] = (unsigned char)((data0 >> 12) & 0x0F);
  p_ae->winWeight[1][1] = (unsigned char)((data0 >> 16) & 0x0F);
  p_ae->winWeight[1][2] = (unsigned char)((data0 >> 20) & 0x0F);
  p_ae->winWeight[2][0] = (unsigned char)((data0 >> 24) & 0x0F);
  p_ae->winWeight[2][1] = (unsigned char)((data0 >> 28) & 0x0F);
  data0 = Gem_read (GEM_AE0_BASE+0x08);
  p_ae->winWeight[2][2] = (unsigned char)((data0 >>  0) & 0x0F);

  // lumCurr
  data0 = Gem_read (GEM_AE1_BASE+0x00);
  data1 = Gem_read (GEM_AE1_BASE+0x04);
  data2 = Gem_read (GEM_AE1_BASE+0x08);
  p_ae->lumCurr = data0 & 0xff;

  // histoGram
  p_ae->histoGram[0] = (data1 >> 0)  & 0xfff;
  p_ae->histoGram[1] = (data1 >> 16) & 0xfff;
  p_ae->histoGram[3] = (data2 >> 0)  & 0xfff;
  p_ae->histoGram[4] = (data2 >> 16) & 0xfff;
  p_ae->histoGram[2] = p_ae->histoGram[0]+p_ae->histoGram[1]+
                       p_ae->histoGram[3]+p_ae->histoGram[4];
  if(p_ae->histoGram[2] >= 4095)
	  p_ae->histoGram[2] = 0;
  else
  	  p_ae->histoGram[2] = 4095 - p_ae->histoGram[2];

  /*
  XM_printf ("p_ae->lum_avg = %d", p_ae->lumCurr);
  XM_printf ("\r\np_ae->histo_gram[0] = %d", p_ae->histoGram[0]);
  XM_printf ("\r\np_ae->histo_gram[1] = %d", p_ae->histoGram[1]);
  XM_printf ("\r\np_ae->histo_gram[2] = %d", p_ae->histoGram[2]);
  XM_printf ("\r\np_ae->histo_gram[3] = %d", p_ae->histoGram[3]);
  XM_printf ("\r\np_ae->histo_gram[4] = %d\r\n", p_ae->histoGram[4]);
  */

}

void isp_ae_sts2_read (isp_ae_ptr_t p_ae)
{
  unsigned int data;
  unsigned int i, j, data3[3][3], data4[3][3];

  data3[0][0] = Gem_read (GEM_AE2_BASE+0x00);
  data4[0][0] = Gem_read (GEM_AE2_BASE+0x04);
  data3[0][1] = Gem_read (GEM_AE2_BASE+0x08);
  data4[0][1] = Gem_read (GEM_AE2_BASE+0x0c);
  data3[0][2] = Gem_read (GEM_AE2_BASE+0x10);
  data4[0][2] = Gem_read (GEM_AE2_BASE+0x14);
  data3[1][0] = Gem_read (GEM_AE2_BASE+0x18);
  data4[1][0] = Gem_read (GEM_AE2_BASE+0x1c);
  data3[1][1] = Gem_read (GEM_AE2_BASE+0x20);
  data4[1][1] = Gem_read (GEM_AE2_BASE+0x24);
  data3[1][2] = Gem_read (GEM_AE2_BASE+0x28);
  data4[1][2] = Gem_read (GEM_AE2_BASE+0x2c);
  data3[2][0] = Gem_read (GEM_AE2_BASE+0x30);
  data4[2][0] = Gem_read (GEM_AE2_BASE+0x34);
  data3[2][1] = Gem_read (GEM_AE2_BASE+0x38);
  data4[2][1] = Gem_read (GEM_AE2_BASE+0x3c);
  data3[2][2] = Gem_read (GEM_AE2_BASE+0x40);
  data4[2][2] = Gem_read (GEM_AE2_BASE+0x44);

  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
		 unsigned int tmp;
        p_ae->yavg_s[i][j] = (data3[i][j] >> 24);
        p_ae->histogram_s[i][j][0] = (data3[i][j] >> 0) & 0xfff;
        p_ae->histogram_s[i][j][1] = (data3[i][j] >> 12) & 0xfff;
        p_ae->histogram_s[i][j][3] = (data4[i][j] >> 0) & 0xfff;
        p_ae->histogram_s[i][j][4] = (data4[i][j] >> 12) & 0xfff;
		  tmp = p_ae->histogram_s[i][j][0] + p_ae->histogram_s[i][j][1] + p_ae->histogram_s[i][j][3] + p_ae->histogram_s[i][j][4];
		  if(tmp > 4095)
		  {
			  //printf ("%d > 4095\n", tmp);
			  tmp = 4095;
		  }
        //p_ae->histogram_s[i][j][2] = 4095 - p_ae->histogram_s[i][j][0] - p_ae->histogram_s[i][j][1]
        //                                  - p_ae->histogram_s[i][j][3] - p_ae->histogram_s[i][j][4];
		  p_ae->histogram_s[i][j][2] = 4095 - tmp;
    }
  }

  // ��ȡerisֱ��ͼ��Ϣ
  p_ae->eris_hist_thresh = Gem_read (GEM_ERIS_HIST_BASE+0x00);

  p_ae->eris_hist_statics[0] = Gem_read (GEM_ERIS_INFO_BASE+0x00);
  p_ae->eris_hist_statics[1] = Gem_read (GEM_ERIS_INFO_BASE+0x04);
  p_ae->eris_hist_statics[2] = Gem_read (GEM_ERIS_INFO_BASE+0x08);
  p_ae->eris_hist_statics[3] = Gem_read (GEM_ERIS_INFO_BASE+0x0c);
  p_ae->eris_hist_statics[4] = Gem_read (GEM_ERIS_INFO_BASE+0x10);

  p_ae->eris_yavg = Gem_read (GEM_ERIS_INFO_BASE+0x14);
}

void isp_ae_yavg_s_read (isp_ae_ptr_t p_ae)
{
  p_ae->yavg_s[0][0] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x00) >> 24);
  p_ae->yavg_s[0][1] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x08) >> 24);
  p_ae->yavg_s[0][2] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x10) >> 24);
  p_ae->yavg_s[1][0] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x18) >> 24);
  p_ae->yavg_s[1][1] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x20) >> 24);
  p_ae->yavg_s[1][2] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x28) >> 24);
  p_ae->yavg_s[2][0] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x30) >> 24);
  p_ae->yavg_s[2][1] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x38) >> 24);
  p_ae->yavg_s[2][2] = (unsigned short)(Gem_read (GEM_AE2_BASE+0x40) >> 24);


  p_ae->eris_yavg = Gem_read (GEM_ERIS_INFO_BASE+0x14);
}