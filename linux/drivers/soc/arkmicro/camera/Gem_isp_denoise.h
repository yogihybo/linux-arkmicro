// =============================================================================
// File        : Gem_isp_denoise.h
// Version     : v1.0
// Author      : Honglei Zhu
// Date        : 2014.12.5
// -----------------------------------------------------------------------------
// Description :
//
// -----------------------------------------------------------------------------
#ifndef __GEM_ISP_DENOISE_HEADFILE__
#define __GEM_ISP_DENOISE_HEADFILE__

#include "Gem_isp_ae.h"

#define GEM_DENOISE_BASE  (0x164)
#define GEM_LUT_BASE  (0x1a0)

typedef struct isp_denoise_
{
  unsigned short enable2d;		// 3 bit 	space 2d denoise 
  										//		bit 0 	1 enable space 2d denoise, 0 disable 
  										// 	bit 1		定义亮度分量(Y)滤波强度系数的选择方法
  										//					0	所有亮度分量的元素使用相同的滤波强度系数y_thres(噪声阈值), 滤波强度仅受噪声阈值影响.
  										//					1	使用查找表(根据亮度值)与噪声阈值(y_thres)结合的方法定义滤波强度系数,
  										//						每个亮度分量的元素的滤波既受噪声阈值影响, 也受该分量值大小的影响
  										//							(使用分量值大小查找17级线性查找表得到对应的幅度影响因子, 幅度强度因子越大, 滤波强度越大;
  										//							 当幅度影响因子均为最大值255时, 此时滤波等同于仅采用滤波阈值的方法).
  										//	
  										//					1 使能 filter0/1的 y_thre0/1* noise0_lutdata
  										//						当所有noise0_lutdata的表项值均为255时, 此时等同于不使用表的效果
  										//					0 仅使用filter0/1的 y_thre0/1	
  										//		bit 2		定义色度分量(CbCr)滤波强度系数的选择方法	
  										//					1 使能 filter0/1的 u_thre0/1* noise0_lutdata 及 v_thre0/1* noise0_lutdata
  										//					0 仅使用filter0/1的 u_thre0/1 及 v_thre0/1 	
  										//
  										//		通过比较, 基于亮度信息的滤波效果非常明显. 通过色度信息的滤波效果非常轻微.
  
  unsigned short enable3d;		// 3 bit 	temporty 3d denoise
  
  unsigned short sensitiv;		// 0724 版本 3 bit  
  
  unsigned short sensitiv0;	// 新版本ISP的滤波器0(差分)的灵敏度设置, 3bit
  										//		值越大, 表示相邻参与滤波的点的数量越多, 邻近点对当前点的影响越大, 滤波趋于平滑. 	
  										//		0		滤波器关闭
  										//		1		3个相邻点参与滤波
  										//		5		表示11个点全部参与滤波.
  										//		6,7	与5相同
  										//				0, 3, 5, 7, 9, 11共6种相邻点选择方法
  
  unsigned short sensitiv1;	// 新版本ISP的滤波器1(边沿)的灵敏度设置, 3bit
  										//		值越大, 表示相邻参与滤波的点数量越多, 滤波的强度越大. 	
  										//		0		滤波器关闭
  										//		5		表示11个点全部参与滤波.
  										//		6,7	与5相同
   
  
  unsigned short sel_3d_table;	// 3D 高斯表选择(temp0-temp3)
  
  unsigned short sel_3d_matrix;	// 3D 选择相关性区域
  											//		0		邻近点矩阵区域
  											//		1		中心点矩阵区域
  
  // 滤波器0的噪声阈值参数 (2D差分)
  unsigned short y_thres0;		// 亮度分量Y阈值,  10 bit ，有效值 0 ~ 1023
  										//		噪声增加时, 信噪比(snr)减小.  信噪比较差(信号强度不变, 噪声强度增加, snr下降)的像素点增多; 
  										//		增加滤波的噪声阈值, 将低于该噪声阈值的更多的低信噪比像素点进行滤波处理, 提升最终图像的信噪比.	
  										//		阈值越大, 参与滤波的像素点越多, 滤波越平滑, 图像解析度趋于降低(图像趋于模糊)
  										
  unsigned short u_thres0;		// 色度分量Cb阈值, 10 bit ，有效值 0 ~ 1023
  unsigned short v_thres0;		// 色度分量Cr阈值, 10 bit ，有效值 0 ~ 1023

  // 滤波器1的噪声阈值参数 (2D边沿), 边沿滤波会保护边缘的细节。
  unsigned short y_thres1;		// 10 bit ，有效值 0 ~ 1023
  unsigned short u_thres1;		// 10 bit ，有效值 0 ~ 1023
  unsigned short v_thres1;		// 10 bit ，有效值 0 ~ 1023

  // 滤波器2的噪声阈值参数 (3D)
  unsigned short y_thres2;		// 10 bit ，
  unsigned short u_thres2;		// 10 bit ，
  unsigned short v_thres2;		// 10 bit ，

  // 强度(已废弃)
  unsigned short y_strength0;//10bit
  unsigned short y_strength1;//10bit
  unsigned short y_strength2;//10bit

  unsigned short u_strength0;//10bit
  unsigned short u_strength1;//10bit
  unsigned short u_strength2;//10bit

  unsigned short v_strength0;//10bit
  unsigned short v_strength1;//10bit
  unsigned short v_strength2;//10bit
  
  
  // 2D查找表
  unsigned char noise0[17];  	// 8bit, 0 ~ 255
  										//		2d space noise0_lutdata (按照Y/CbCr的强度值从0~255的16等分插值查找表)	
  										//			可以根据亮度分量Y(或者CbCr分量)的强度值设计不同的滤波强度.
  										//			如下的强度表
  										//			索引(分量强度)  滤波强度系数   说明
  										//			0   (0  ),        255          (最小分量或者低光处的snr信噪比最小, 安排最大的滤波强度)
  										//			1   (16 ),        255
  										//       ...
  										//			16  (255),        223          (最大分量或者最亮点的snr信噪比最大, 安排最小的滤波强度) 
  										// 	值越大, 滤波强度越大
  
  
  // 3D查找表
  unsigned char noise1[17];  	// 8bit, 0 ~ 255
  										//		3d temporty lutdata
} isp_denoise_t; 

typedef struct isp_denoise_  *isp_denoise_ptr_t;

void isp_denoise_init (isp_denoise_ptr_t p_denoise);

void isp_denoise_init_io (isp_denoise_ptr_t p_denoise);

void isp_denoise_run (isp_denoise_ptr_t p_denoise, isp_ae_ptr_t p_ae);

#endif	// #ifndef __GEM_ISP_DENOISE_HEADFILE__
