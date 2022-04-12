// =============================================================================
// File        : Gem_isp_awb.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef __GEM_ISP_AWB_HEADFILE__
#define __GEM_ISP_AWB_HEADFILE__

#define GEM_AWB0_BASE (0x090) 
#define GEM_AWB1_BASE (0x1bc + 0x0c + 0x0c)
#define GEM_AWB2_BASE (0x20c + 0x0c + 0x0c)



typedef struct isp_auto_awb_ 
{
  unsigned char enable;    // 1 bit
  
  unsigned char mode;     // 2 bit
  
  unsigned char manual;      	// 1 bit, 1 手动模式, 0 自动模式
  
  unsigned char weight[3][3]; // 4 bit
  unsigned char black;     // 8 bit
  unsigned char white;     // 8 bit
  unsigned char jitter;
  unsigned short r2g_min;
  unsigned short r2g_max;
  unsigned short b2g_min;     
  unsigned short b2g_max;
  
  unsigned short r2g_light[8];	// R色温表, 最大数值 4095
  											//		仅自动模式下使用
  unsigned short b2g_light[8];	// B色温表, 最大数值 4095
  											//		仅自动模式下使用
  
  unsigned char use_light[8];	// 表示对应的光源是否使能, 1 使能 0 未使用
  											//		仅自动模式下使用
  
  unsigned short gain_g2r;    // 16 bit, 有效范围 0 ~ 4095
  										//		手动模式下设置红色的增益	
  										//		值越大, 红色分量的颜色越浓
  unsigned short gain_g2b; 	// 16 bit, 有效范围 0 ~ 4095
  										//		手动模式下设置蓝色的增益
  										//		值越大, 蓝色分量的颜色越浓
  
  
  unsigned short gain_r2g;		// 16 bit, 只读项, 保存R分量色温估算值
  unsigned short gain_b2g;		// 16 bit, 只读项, 保存B分量色温估算值
  
  unsigned int gray_num;
} isp_awb_t;  

typedef struct isp_auto_awb_ *isp_awb_ptr_t;

void isp_awb_init (isp_awb_ptr_t p_awb);

void isp_awb_init_io (isp_awb_ptr_t p_awb);

void isp_awb_info_read (isp_awb_ptr_t p_awb);

void isp_awb_run (isp_awb_ptr_t p_awb);

#endif