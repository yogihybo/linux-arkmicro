// =============================================================================
// File        : Gem_isp_colors.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------

#ifndef  GEM_ISP_COLORS_H
#define  GEM_ISP_COLORS_H
#define GEM_COLORS_BASE (0x12c)
#define GEM_LUT_BASE (0x1a0)
#define GEM_DEMOSAIC_BASE (0x1a8)
#include "Gem_isp_ae.h"
#include "Gem_isp_awb.h"

struct isp_color_matrix_t
{
  unsigned short enable;// 1 bit
  short matrixcoeff[3][3]; // 16 bit，
};

struct isp_gamma_lut_t
{
  unsigned int enable;// 1 bit
  unsigned short gamma_lut[65];// 16 bit 只写
};

struct isp_rgb2yuv_matrix_t
{
  signed short rgbcoeff[4][3];// 10 bit 
};

// demosaic参数
// 初步评估(主观/客观), 参考20160628的测试报告
// demosaic的缺省参数定义如下, 
// 	1) 具有较好的解析度, 
//		2) 非常轻微的颜色方块边沿棋盘格现象(imatest实景),  
//		3) iso12233解析度评测, 较好的伪彩及棋盘格抑制
//		4) 1844FLAG禁止标志, 较好的抑制十字边沿黑块	
// mode = 0
// coff_00_07 = 32(40也可以)
// demk = 128
// coff_20_27 = 255
// horz_thread = 0
// demDhv_ofst = 0
struct isp_demosaic_t {
	unsigned char mode;						// bit31  demmethod
													//	2种demosaic算法, 
													//	0 old  1 new
	
	unsigned char coff_00_07;				// bit7-0  demk1
													//	0 ~  7  清晰度调整主要修改低8位的值
													//		黑暗场景下，demk1的影响非常微弱。
													//		demk1可固定为一个合适的值（任何场景下）
	
	unsigned short demk;						// bit19-8 demk, 12bit
													//		demk影响解析度,
													//			demk等于0时, 参数coff_00_07的变化不再影响解析度
													//			demk不为0时, 增加参数coff_00_07的值会提高解析度
													//
													//		当场景光照较暗时，应减小demk，减弱demk解析度拉伸时引起的噪声(此噪音无法使用2D、crosstalk等降噪技术滤除)
													//		黑暗背景下，此噪声容易出现在灯光的光晕边界上。(将该值从128减小到16)
	
	unsigned char coff_20_27;				// 20 ~ 27  一般固定为16
	
	unsigned short horz_thread;			// bit11-0  demDofst
													// 水平方向的阈值 (12bit, 0 ~ 4095), 
													//	平时一般设置为固定值, 主要用于分辨率测试时微调, 达到需要的分辨率 
	
	unsigned short demDhv_ofst;			// bit23-12 demDhv_ofst, (12bit, 0 ~ 4095)
													//		(32~ 256)
													//		一般设置为0. 值增加时易出现棋盘格现象.
};

typedef enum{
   HDTV_type_0255=0,
   HDTV_type_16235,
   SDTV_type_0255,
   SDTV_type_16235,
}rgb_ypbpr_enum;

typedef struct isp_colors_
{
  unsigned int rgb2ypbpr_type;
  struct isp_color_matrix_t colorm;
  struct isp_gamma_lut_t gamma;
  struct isp_rgb2yuv_matrix_t rgb2yuv;
  
  struct isp_demosaic_t	demosaic;
} isp_colors_t;

typedef struct isp_colors_ *isp_colors_ptr_t;

void isp_colors_init (isp_colors_ptr_t p_colors);

void isp_colors_init_io (isp_colors_ptr_t p_colors);

void isp_colors_run (isp_colors_ptr_t p_colors, isp_awb_ptr_t p_awb, isp_ae_ptr_t p_ae);

void isp_create_rgb2ycbcr_matrix (unsigned int rgb2ycbcr_type, struct isp_rgb2yuv_matrix_t *matrix);


#endif
