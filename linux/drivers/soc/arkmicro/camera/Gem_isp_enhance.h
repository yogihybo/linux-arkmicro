// =============================================================================
// File        : Gem_isp_enhance.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef _GEM_ISP_ENHANCE_H_
#define _GEM_ISP_ENHANCE_H_

#define GEM_ENHANCE_BASE  (0x190)

struct sharp_t
{
	unsigned char enable;	// 1 bit, 锐化功能使能
	
	unsigned char mode;		// 1 bit, 锐化模式， 0: 强烈 1: 柔和
									//		"强烈"与"柔和"的效果差异较小
	
	unsigned char coring;	// 3 bit, 有效值 0 ~ 7, 锐化配置选择
									// 	值越大, 图像越模糊, 图像锐化程度降低.
	
	unsigned char strength;	// 8 bit, 有效值 0 ~ 255
									//		值越大, 图像越模糊, 图像锐化程度降低.
	
	unsigned short gainmax;	// 10 bit, 有效值 0 ~ 255, 大于255等于255的效果
									//		0的效果等于255的效果
									//		从1到255, 值越大, 锐化效果越强
};

struct bcst_t
{
	unsigned short enable;		// 1 bit
	
	short bright;					// 10 bit, 有效值 -255 ~ 255 
										//		增加或减小每个像素点的亮度值
	
	unsigned short contrast;	// 11 bit, 有效值 0 ~ 2047
										//		通过调整动态范围来改变对比度,
										//		值越大, 直方图越向右移动, 左侧的低光处细节逐渐丢失, 右边的高光处逐渐趋于饱和.
										//		值越小, 直方图越向左移动, 右侧的高光处细节逐渐丢失, 左侧的低光处逐渐趋于饱和.
	
	unsigned short satuation;	// 11 bit, 有效值 0 ~ 2047 
										//		色彩饱和度调节
										//		值越大, 色彩越鲜艳. 值越小, 越趋向于灰色.
										//		等于0时, 为黑白灰度效果
	
	short hue;						// 8 bit,  有效值 -128 ~ 127
										//		色相角调整
	
	unsigned short offset0;		// 10 bit, 有效值 0 ~ 255.  
										//		对比度为1024时,该设置值将无效
	
	unsigned short offset1;		// 10 bit, 有效值 0 ~ 255.   
										//		saturation offset (0,255) (default 128)
										//		hue为0时, 该设置值将无效
};

typedef struct isp_enhance_
{
  struct sharp_t sharp;
  struct bcst_t bcst;
} isp_enhance_t;

typedef struct isp_enhance_ *isp_enhance_ptr_t;

void isp_enhance_init (isp_enhance_ptr_t p_enhance);

void isp_enhance_init_io (isp_enhance_ptr_t p_enhance);





#endif





