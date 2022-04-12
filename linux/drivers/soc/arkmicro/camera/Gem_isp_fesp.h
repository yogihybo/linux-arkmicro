// =============================================================================
// File        : Gem_isp_fesp.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef  _GEM_ISP_FESP_H_
#define  _GEM_ISP_FESP_H_

#define GEM_LENS_BASE     (0x0e0)
#define GEM_FIXPATT_BASE  (0x0f0)
#define GEM_BADPIX_BASE   (0x100)
#define GEM_CROSS_BASE    (0x108)
#define GEM_LUT_BASE      (0x1a0)

#define GEM_LENS_LSCOFST_BASE	(0x6c * 4)

#include "Gem_isp_ae.h"

struct Lensshade_t 
{
  unsigned char enable; 		// 1 bit
  
  unsigned char scale; 			// 2 bit
  										// 镜头阴影增益插值间隔尺度
										//	0: 16点 1: 32点 2: 64点 3: 128点		
  
  unsigned short coef[195]; 		// 16 bit  
  										// coef[0]~coef[64]    R分量阴影增益插值系数
  										// coef[65]~coef[129]  G分量阴影增益插值系数
  										//	coef[130]~coef[194] B分量阴影增益插值系数
  unsigned int rcenterRx;		// 16 bit 镜头阴影R分量x中心坐标
  unsigned int rcenterRy;		// 16 bit 镜头阴影R分量y中心坐标
  unsigned int rcenterGx;		// 16 bit 镜头阴影G分量x中心坐标
  unsigned int rcenterGy;		// 16 bit 镜头阴影G分量y中心坐标
  unsigned int rcenterBx;		// 16 bit 镜头阴影B分量x中心坐标
  unsigned int rcenterBy;		// 16 bit 镜头阴影B分量y中心坐标
  
  unsigned int lscofst;			// 16bit
};

struct Fixpatt_t
{
  unsigned short enable;
  unsigned short mode;
  unsigned short rBlacklevel;		// 16 bit, 有效位8bit (0 ~ 255).
  unsigned short grBlacklevel;	// 16 bit, 有效位8bit (0 ~ 255).
  unsigned short gbBlacklevel;	// 16 bit, 有效位8bit (0 ~ 255).
  unsigned short bBlacklevel;		// 16 bit, 有效位8bit (0 ~ 255).
  unsigned char profile[17];		// (0 ~ 255)

};

// BadPixel可用于去除画面4边临近边界上的奇异点（由demosaic算法引入？）
struct Badpix_t
{
  unsigned char enable;				// 0 禁止 1 使能
  unsigned char mode; 				// 0 普通模式 1 查表模式
  unsigned short thresh;			// 普通模式下的阈值
  											//		与周围点亮度偏差大于该阈值的点将进行滤波
  unsigned char profile[16]; 		// 与16级灰度亮度对应的阈值
};

struct Crosstalk_t
{
	unsigned short enable;		// crosstalk enable, 1bit, (1: enable 0:disable)
	
	unsigned short mode; 		// crosstalk mode, 2bit 滤波模式
										//			00: unite filter thres=128 
										//			10: use reg thres
										//				阈值寄存器模式
										//					计算5x5的4个方向差分平均值avg, 与thres0cgf比较; 
										//					根据比较结果, (avg > thres0cgf)选择thres1cgf或者(avg <= thres0cgf)选择thres2cgf配置滤波器的阈值.
										//					当thres1cgf与thres2cgf相同时, 此时阈值总是选择同一个值 
										//			x1: base on lut
										//				与模式10比较, 滤波的强度不仅与阈值有关, 同时与像素点的亮度大小相关.
										//				通过查找表profile[17], 可定义与亮度值相关的滤波强度因子.
										//				例如, 亮度值较低的像素(信噪比小), 滤波强度因子可配置较大, 提高噪声滤除能力; 
										//				亮度值较大的像素(信噪比大), 滤波强度因子可配置较低, 减轻噪声滤除导致的画面模糊.
										//			模式11 与 模式01 都是基于查找表, 但存在差异. 
										//			模式01的滤波强度高于模式11.
	
	// 噪声阈值
	// 阈值越大, 滤波的强度越大. 
	//		参数thresh(thres2cgf)的变化对滤波效果的影响大于thres1cgf的变化的影响.
	//		因此一般调节thresh(thres2cgf)
	unsigned short thresh;			// 噪声阈值2, thres2cgf, 16bit, 有效范围 0 ~ 1023
											//		
	unsigned short thres1cgf;		// 噪声阈值1, 16bit, 有效范围 0 ~ 1023
	
	unsigned short snsCgf;			//	2bit, 0 ~ 3
											//		值越大, 滤除奇异点的能力越大. 
											//		此值的变化基本不影响锐度.
											//		缺省值可以设置为3
	
	unsigned short thres0cgf;		// 方向差分阈值比较寄存器, 16bit
	
	unsigned char profile[17];	// 滤波器强度配置查找表, 8bit, 有效范围 0 ~ 255
											//		根据亮度分量查找"查找表"选择滤波强度
											//		查找表表项值越大, 降噪强度越大
											//			0   	滤波强度最小
											//			255	滤波强度最大
											//
											//	1) 场景光照强度很大时(中午日光直射)，曝光时间超短，阴暗处的细节因曝光时间不够而导致噪点很大。
											//	   通过查找表不同参数的设置，提高低亮度区(0 ~ 15, 16 ~ 31)的滤波强度因子来加强阴暗处的滤波，
											//		减小其他亮度区的滤波强度值来降低非阴暗处的滤波强度, 从而抑制阴暗处的噪声，保留明亮处的细节。
};

typedef struct isp_fesp_
{ 
  struct Lensshade_t Lensshade;
  struct Fixpatt_t Fixpatt;
  struct Badpix_t Badpix;
  struct Crosstalk_t Crosstalk;
} isp_fesp_t;

typedef struct isp_fesp_ *isp_fesp_ptr_t;

void isp_fesp_init (isp_fesp_ptr_t p_fesp);

void isp_fesp_init_io (isp_fesp_ptr_t p_fesp);

void isp_fesp_run (isp_fesp_ptr_t p_fesp, isp_ae_ptr_t p_ae);

#endif