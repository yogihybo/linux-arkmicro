// =============================================================================
// File        : Gem_isp_ae.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef _GEM_ISP_AE_HEADFILE_
#define _GEM_ISP_AE_HEADFILE_

#define GEM_AE0_BASE (0x070)


#define GEM_AE1_BASE (0x1b0 + 0x0c + 0x0c)	// 新增3个寄存器
#define GEM_AE2_BASE (0x1c4 + 0x0c + 0x0c)


typedef struct isp_auto_exposure_ 
{
  unsigned char histoBand[4];
  unsigned char winWeight[3][3];
  unsigned short histoGram[5];
  
  unsigned short yavg_s[3][3]; 
  unsigned short histogram_s[3][3][5];
  
  unsigned char lumTarget;
  unsigned char lumCurr;
  
  unsigned int eris_hist_thresh;				// ERIS亮度直方图的4个阈值(每个阈值的取值范围为0~255)
  														//	bit0~7对应01区间, bit8~15对应12区间, bit16~23对应23区间, bit24~31对应34区间			
  
  unsigned int  eris_hist_statics[5];		// (只读信息) 5段ERIS亮度直方图的统计点像素个数,
  unsigned int  eris_yavg;						// (只读信息) ERIS亮度统计值, 需除以总像素点数, 得到平均亮度值
  
  unsigned char bright_target;
  unsigned char black_target;
  unsigned char compensation;

} isp_ae_t; 

typedef struct isp_auto_exposure_ *isp_ae_ptr_t;

void isp_ae_init (isp_ae_ptr_t p_ae); 

void isp_ae_init_io (isp_ae_ptr_t ae); 

void isp_ae_info_read (isp_ae_ptr_t p_ae);

void isp_ae_sts2_read (isp_ae_ptr_t p_ae);

void isp_ae_yavg_s_read (isp_ae_ptr_t p_ae);

void isp_histogram_bands_data (unsigned int data1, unsigned int data2);

void isp_ae_run (isp_ae_ptr_t p_ae);

#endif
