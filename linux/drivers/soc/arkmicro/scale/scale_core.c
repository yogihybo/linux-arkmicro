#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/io.h>

#include "scale.h"

#define SYS_SOFT_RSTNB		0x78


static unsigned char ver_cof[16][7]={
  {4,242, 22, 104, 22, 242, 4},//{4,-14, 22, 104, 22, -14, 4},
  {1, 243, 33, 86, 33, 243, 1},//{1, -13, 33, 86, 33, -13, 1},
  {251, 1, 36, 64, 36, 1, 251},//{-5, 1, 36, 64, 36, 1, -5},
  {252, 9, 34, 50, 34, 9, 252},//{-4, 9, 34, 50, 34, 9, -4},
  {255, 12, 32, 42, 32, 12,255},//{-1, 12, 32, 42, 32, 12, -1},
  {2, 13, 30, 38, 30, 13, 2},
  {5, 14, 28, 34, 28, 14, 5},
  {6, 16, 26, 32, 26, 16, 6},
  {9, 15, 25, 30, 25, 15, 9},
  {9, 15, 25, 30, 25, 15, 9},
  {11, 15, 24, 28, 24, 15, 11},
  {11, 15, 24, 28, 24, 15, 11},
  {12, 16, 23, 26, 23, 16, 12},
  {12, 16, 23, 26, 23, 16, 12},
  {13, 15, 23, 26, 23, 15, 13},
  {13, 16, 22, 26, 22, 16, 13}
};

static int get_scaler_cof(int iwin_height, int oheight)
{
	int index;
#if 0
	int val;
	float scaler_cof[] ={1.25, 1.50, 2.00, 2.50, 3.00, 3.50,
						  4.00, 4.50, 5.00, 5.50, 6.00, 6.50,
						  7.00, 7.50, 8.00, 8.50};
	val = iwin_height /(oheight + 4);
#else
	long long val;
	int scaler_cof[] ={12500, 15000, 20000, 25000, 30000, 35000,
					  40000, 45000, 50000, 55000, 60000, 65000,
					  70000, 75000, 80000, 85000};
	val = iwin_height *10000 /(oheight + 4);
#endif


	if(val >= scaler_cof[0] && val < scaler_cof[1])
		index = 0;
	else if(val >= scaler_cof[1] && val < scaler_cof[2])
		index = 1;
	else if(val >= scaler_cof[2] && val < scaler_cof[3])
		index = 2;
	else if(val >= scaler_cof[3] && val < scaler_cof[4])
		index = 3;
	else if(val >= scaler_cof[4] && val < scaler_cof[5])
		index = 4;
	else if(val >= scaler_cof[5] && val < scaler_cof[6])
		index = 5;
	else if(val >= scaler_cof[6] && val < scaler_cof[7])
		index = 6;
	else if(val >= scaler_cof[7] && val < scaler_cof[8])
		index = 7;
	else if(val >= scaler_cof[8] && val < scaler_cof[9])
		index = 8;
	else if(val >= scaler_cof[9] && val < scaler_cof[10])
		index = 9;
	else if(val >= scaler_cof[10] && val < scaler_cof[11])
		index = 10;
	else if(val >= scaler_cof[11] && val < scaler_cof[12])
		index = 11;
	else if(val >= scaler_cof[12] && val < scaler_cof[13])
		index = 12;
	else if(val >= scaler_cof[13] && val < scaler_cof[14])
		index = 13;
	else if(val >= scaler_cof[14] && val < scaler_cof[15])
		index = 14;
	else if(val >= scaler_cof[15] && val < scaler_cof[16])
		index = 15;
	else
		index = 16;

	return 16;
}

static void scale_rotate(struct ark_scale_context *context, int mode, int rotate, int pic_width, int pic_height, int pic_addr)
{
	unsigned int roty_width =0,roty_src_width=0,rotc_width =0,rotc_src_width =0;
	unsigned int nor_act_width,nor_tot_width;
	int yuv420 = 0;
	unsigned int val;

	if((mode != SCALE_FORMAT_YUV420) && (mode != SCALE_FORMAT_YUV422)) {
		printk("%s, Invalid mode\n", __FUNCTION__);
		return ;
	}

	if(mode == SCALE_FORMAT_YUV420)
		yuv420 = 1;

	switch(rotate) {
		case SCALE_ROTATE_0: {
			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(yuv420<<15)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			writel(0, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			if(pic_width%8 != 0) {
				nor_act_width = (pic_width/8+1)*8;
			} else {
				nor_act_width = pic_width;
			}
			if(pic_width%8 != 0) {
				nor_tot_width = (pic_width/8+1)*8;
			} else {
				nor_tot_width = pic_width;
			}

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val |=(nor_act_width<<12)|(nor_tot_width);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			writel(pic_addr, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
			writel(pic_addr + pic_width*pic_height, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		case SCALE_ROTATE_90: {
			if(pic_width%8 != 0){
				pic_width = (pic_width/8+1)*8;
			}
			if(yuv420) {
				if(pic_height%8 != 0){
					pic_height = (pic_height/8+1)*8;
				}
			}
			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(1<<17)|(yuv420<<15)|(1<<14)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			writel(0, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			if(!yuv420) {	//for yuv422 mode.
				if(pic_width%8 != 0)
					nor_act_width = (pic_width/8+1)*8;
				else
					nor_act_width = pic_width;
				if(pic_width%8 != 0)
					nor_tot_width = (pic_width/8+1)*8;
				else
					nor_tot_width = pic_width;
				val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
				val |=(0x1<<28)|(0x1<<26)|(0x1<<24)|(nor_act_width<<12)|(nor_tot_width);
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			}
			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val |=(0x1<<28)|(0x1<<26)|(0x1<<24);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			if(pic_height%8 != 0) {
				roty_width = (pic_height/8+1)*8;
			} else {
				roty_width = pic_height;
			}
			if(pic_width%8 != 0) {
				roty_src_width = (pic_width/8+1)*8;
			}
			else {
				roty_src_width = pic_width;
			}
			writel((roty_src_width<<12) | (roty_width<<0), context->mmio_base + AXI_SCALE_ROTYX_WIDTH);

			if(pic_height%8 != 0) {
				rotc_width = (pic_height/8+1)*8;
			}
			else {
				rotc_width = pic_height;
			}
			if(yuv420) {
				if(pic_width%16 != 0) {
					rotc_src_width = (pic_width/16+1)*8;
				} else {
					rotc_src_width = pic_width/2;
				}
			} else {
				if(pic_width%8 != 0)
					rotc_src_width = (pic_width/8+1)*8;
				else
					rotc_src_width = pic_width;
			}

			writel((rotc_src_width<<12)|(rotc_width<<0), context->mmio_base + AXI_SCALE_ROTCX_FADR);

			writel(pic_addr + roty_width-16, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
			writel(pic_addr + roty_width*roty_src_width+rotc_width-16, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		case SCALE_ROTATE_180: {
			if(pic_width%8 != 0) {
				pic_width = (pic_width/8+1)*8;
			}
			if(pic_height%8 != 0) {
				pic_height = (pic_height/8+1)*8;
			}
			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(1<<17)|(yuv420<<15)|(1<<13)|(1<<12)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			writel(0, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			if(pic_width%8 != 0) {
				nor_act_width = (pic_width/8+1)*8;
			} else {
				nor_act_width = pic_width;
			}
			if(pic_width%8 != 0) {
				nor_tot_width = (pic_width/8+1)*8;
			} else {
				nor_tot_width = pic_width;
			}

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val |= (0x1<<27)|(nor_act_width<<12)|(nor_tot_width);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			writel((pic_height<<12)|(pic_width<<0), context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			if(yuv420) {
				if(pic_height%8 != 0) {
					rotc_width = (pic_height/8+1)*8;
				} else {
					rotc_width = pic_height;
				}
				if(pic_width%16 != 0)
					rotc_src_width = (pic_width/16+1)*8;
				else
					rotc_src_width = pic_width;
				val = (rotc_src_width<<12)|(rotc_width<<0);
				writel(val, context->mmio_base + AXI_SCALE_ROTCX_FADR);
				val = pic_addr + (pic_height-1)*pic_width;
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
				val = pic_addr + pic_width*pic_height+(pic_height/2-1)*pic_width;
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			} else {
				val = pic_addr + (pic_height-1)*pic_width;
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
				val = pic_addr + pic_width*pic_height+(pic_height-1)*pic_width;
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);

			}
			break;
		}
		case SCALE_ROTATE_270: {
			if(pic_width%8 != 0)
				pic_width = (pic_width/8+1)*8;
			if(pic_height%8 != 0)
				pic_height = (pic_height/8+1)*8;

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(yuv420<<15)|(1<<14)|(1<<13)|(1<<12)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val &=~(0x1f<<24);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			if(pic_height%8 != 0)
				roty_width = (pic_height/8+1)*8;
			else
				roty_width = pic_height;
			if(yuv420) {	//for yuv420 mode.
				if(pic_width%16 != 0)
					roty_src_width = (pic_width/16+1)*16;
				else
					roty_src_width = pic_width;
			} else {	//for yuv422 mode.
				if(pic_width%8 != 0)
					roty_src_width = (pic_width/8+1)*8;
				else
					roty_src_width = pic_width;
			}
			val = (roty_src_width<<12)|(roty_width<<0);
			writel(val, context->mmio_base + AXI_SCALE_ROTYX_WIDTH);

			if(pic_height%8 != 0)
				rotc_width = (pic_height/8+1)*8;
			else
				rotc_width = pic_height;
			if(yuv420) {
				if(pic_width%16 != 0)
					rotc_src_width = (pic_width/16+1)*8;
				else
					rotc_src_width = pic_width/2;
			} else {
				if(pic_width%8 != 0)
						rotc_src_width = (pic_width/8+1)*8;
					else
						rotc_src_width = pic_width;
			}

			val = (rotc_src_width<<12)|(rotc_width<<0);
			writel(val, context->mmio_base + AXI_SCALE_ROTCX_FADR);

			val = pic_addr;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);

			val = pic_addr + roty_width*roty_src_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		case SCALE_ROTATE_0_MIRROR: {
			if(pic_width%8 != 0)
				pic_width = (pic_width/8+1)*8;
			if(pic_height%8 != 0)
				pic_height = (pic_height/8+1)*8;

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(yuv420<<15)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			writel(0, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			if(pic_width%8 != 0)
				nor_act_width = (pic_width/8+1)*8;
			else
				nor_act_width = pic_width;
			if(pic_width%8 != 0)
				nor_tot_width = (pic_width/8+1)*8;
			else
				nor_tot_width = pic_width;

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val |=0x1<<27|(nor_act_width<<12)|(nor_tot_width);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			val = pic_addr + (pic_height-1)*pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
			if(yuv420)
				val = pic_addr + pic_width*pic_height+(pic_height/2-1)*pic_width;
			else
				val = pic_addr + pic_width*pic_height+(pic_height-1)*pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		case SCALE_ROTATE_90_MIRROR: {
			if(pic_width%8 != 0)
			  pic_width = (pic_width/8+1)*8;
			if(pic_height%8 != 0)
			  pic_height = (pic_height/8+1)*8;

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(1<<17)|(yuv420<<15)|(1<<14)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val &=~(0x1f<<24);
			val |= (0x1<<28)|(0x1<<26)|(0x3<<24);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			if(pic_height%8 != 0)
				roty_width = (pic_height/8+1)*8;
			else
				roty_width = pic_height;
			if(yuv420) {
				if(pic_width%16 != 0)
					roty_src_width = (pic_width/16+1)*16;
				else
					roty_src_width = pic_width;
			} else {
				if(pic_width%8 != 0)
					roty_src_width = (pic_width/8+1)*8;
				else
					roty_src_width = pic_width;
			}

			val = (roty_src_width<<12)|(roty_width<<0);
			writel(val, context->mmio_base + AXI_SCALE_ROTYX_WIDTH);

			if(pic_height%8 != 0)
				rotc_width = (pic_height/8+1)*8;
			else
				rotc_width = pic_height;
			if(yuv420) {
				if(pic_width%16 != 0)
					rotc_src_width = (pic_width/16+1)*8;
				else
					rotc_src_width = pic_width/2;
			} else {
				if(pic_width%8 != 0)
					rotc_src_width = (pic_width/8+1)*8;
				else
					rotc_src_width = pic_width;
			}

			val = (rotc_src_width<<12)|(rotc_width<<0);
			writel(val, context->mmio_base + AXI_SCALE_ROTCX_FADR);

			val = pic_addr + roty_width*roty_src_width-16;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);

			val = pic_addr + roty_width*roty_src_width+rotc_width*rotc_src_width-16;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		case SCALE_ROTATE_180_MIRROR: {
			if(pic_width%8 != 0)
				pic_width = (pic_width/8+1)*8;
			if(pic_height%8 != 0)
				pic_height = (pic_height/8+1)*8;

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(1<<17)|(yuv420<<15)|(1<<13)|(1<<12)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			writel(0, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			if(pic_width%8 != 0)
				nor_act_width = (pic_width/8+1)*8;
			else
				nor_act_width = pic_width;
			if(pic_width%8 != 0)
				nor_tot_width = (pic_width/8+1)*8;
			else
				nor_tot_width = pic_width;

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val |=(nor_act_width<<12)|(nor_tot_width);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			val = pic_addr;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);

			val = pic_addr + pic_width*pic_height;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		case SCALE_ROTATE_270_MIRROR: {
			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
			val &=~(0x7f<<11);
			val |=(yuv420<<15)|(1<<14)|(1<<11);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);
			val &=~(0x1f<<24);
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL_REG);

			val = readl(context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);
			val &=~(0xffffff);
			val |=(pic_height<<12)|pic_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_IMG_SIZE);

			if(pic_height%8 != 0)
				roty_width = (pic_height/8+1)*8;
			else
				roty_width = pic_height;
			if(yuv420) {
				if(pic_width%16 != 0)
					roty_src_width = (pic_width/16+1)*8;
				else
					roty_src_width = pic_width;
			} else {
				if(pic_width%8 != 0)
					roty_src_width = (pic_width/8+1)*8;
				else
					roty_src_width = pic_width;
			}

			val = (roty_src_width<<12)|(roty_width<<0);
			writel(val, context->mmio_base + AXI_SCALE_ROTYX_WIDTH);

			if(pic_height%8 != 0)
				rotc_width = (pic_height/8+1)*8;
			else
				rotc_width = pic_height;
			if(yuv420) {
				if(pic_width%16 != 0)
					rotc_src_width = (pic_width/16+1)*8;
				else
					rotc_src_width = pic_width/2;
			} else {
				if(pic_width%8 != 0)
					rotc_src_width = (pic_width/8+1)*8;
				else
					rotc_src_width = pic_width;
			}

			val = (rotc_src_width<<12)|(rotc_width<<0);
			writel(val, context->mmio_base + AXI_SCALE_ROTCX_FADR);

			val = pic_addr;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
			val = pic_addr + roty_width*roty_src_width;
			writel(val, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);
			break;
		}
		default:
			break;
	}
}

static void scale_peaking_denoise(struct ark_scale_context *context, unsigned int dst_width ,unsigned int dst_height)
{
//denoise
#define hand_sel			1 //Hand_sel
#define denoise_Bypass		0//0x2//0//0 //Bypass
#define blank_hand			0xff//0x7ff//16
#define valid_blank_hand	0xff//0x7ff//16
#define K_Y					0x80//0xfff//0xfff//80
#define K_CC				0x80//0xff//0xff//80
//peking
#define Peaking_bypass		0//0x2//0x2//0
#define peaking_hand_sel	1
#define hv_peaking_sel		0
#define lgainh				10
#define mgainh				10
#define hgainh				5
#define lgainv				30//0x1e
#define mgainv				20
#define hgainv				15
#define gaind45				20
#define gaind135			20
#define core_max			50//0x32
#define core_min			10//0xa

	unsigned int Pixel_hand	 = dst_width;
	unsigned int hsync_hand = dst_height;
	unsigned int valid_pixel_hand = dst_width;
	unsigned int val = 0;

	val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
	val &= ~(1<<19);
	writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

	val = (Peaking_bypass<<28) | (hv_peaking_sel<<27) | (peaking_hand_sel<<26) |
			(hand_sel<<25) | (denoise_Bypass<<23) | (blank_hand<<12) | (Pixel_hand<<0);
	writel(val, context->mmio_base + AXI_SCALE_PD_CTL1);

	val = (valid_pixel_hand<<12) | (hsync_hand<<0);
	writel(val, context->mmio_base + AXI_SCALE_PD_CTL2);

	val = (K_CC<<24) | (K_Y<<12) | (valid_blank_hand<<0);
	writel(val, context->mmio_base + AXI_SCALE_DENOISE_CTL3);

	val = (lgainv<<24) | (hgainh<<16) | (mgainh<<8) | (lgainh<<0);
	writel(val, context->mmio_base + AXI_SCALE_PEAKING_CTL4);

	val = (gaind135<<24) | (gaind45<<16) | (hgainv<<8) | (mgainv<<0);
	writel(val, context->mmio_base + AXI_SCALE_PEAKING_CTL5);

	val = (core_min<<8) | (core_max<<0);
	writel(val, context->mmio_base + AXI_SCALE_PEAKING_CTL6);
}

void scale_softreset(struct ark_scale_context *context)
{
	u32 val;
	if(context) {
		if(context->softreset_reg >= 0) {
			int reg = context->softreset_reg;
			int offset = context->softreset_offset;
			val = readl(context->sys_base + reg);
			val &= ~(1 << offset);
			writel(val, context->sys_base + reg);
			udelay(10);
			val |= (1 << offset);
			writel(val, context->sys_base + reg);
		} else {
			//Only arkn141 platform.
			val = readl(context->sys_base + SYS_SOFT_RSTNB);
			val &= ~(1 << 10);
			writel(val, context->sys_base + SYS_SOFT_RSTNB);
			udelay(10);
			val |= (1 << 10);
			writel(val, context->sys_base + SYS_SOFT_RSTNB);
		}
	}
}

static int ark_scale_set_param(struct ark_scale_context *context, struct ark_scale_param *param)
{
	int ret = SCALE_RET_OK;
	int do_horz_down_scalar = 0;		// down scalar
	int yuv_order = (param->iformat >> 8) & 0xFF;
	int format = param->iformat & 0xFF;
	unsigned int vcut = param->up_cut + param->bottom_cut;
	unsigned int hcut = param->left_cut + param->right_cut;
	u32 val;

	if(context->soc_type == SOC_TYPE_ARK1668E) {
		writel(0x0, context->mmio_base + AXI_SCALE_ROTATA_CTL);
		writel((1<<28)|(1<<23), context->mmio_base + AXI_SCALE_PD_CTL1);	//disable demoise peaking
	}

	do {
		//write back middle finish, write back bresp error, write back frame finish)
		writel(0x7, context->mmio_base + AXI_SCALE_CLCD_INT_CLR);

		//write back bresp error, write back frame finish)
		if(context->soc_type == SOC_TYPE_ARK1668E) {
			writel((1 << 0), context->mmio_base + AXI_SCALE_INT_CTL);
		} else {
			writel((1 << 0) | (1 << 1), context->mmio_base + AXI_SCALE_INT_CTL);
		}

		writel(0, context->mmio_base + AXI_SCALE_RESERVED);

		writel(0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3|0x0<<2|format<<0,
				context->mmio_base + AXI_SCALE_CONTROL);

		val = readl(context->mmio_base + AXI_SCALE_CONTROL);
		if (format == SCALE_FORMAT_YUYV)
			val |= yuv_order << 6;
		else if (format == SCALE_FORMAT_YUV422 || format == SCALE_FORMAT_YUV420)
			val |= (yuv_order & 1) << 2;
		writel(val, context->mmio_base + AXI_SCALE_CONTROL);

		if (param->iwidth == param->owidth && param->iheight == param->oheight &&
			param->iwidth == param->iwinwidth && param->iheight == param->iwinheight) {
			val |= (1<<5);
			writel(val, context->mmio_base + AXI_SCALE_CONTROL);
		}
		if(context->soc_type == SOC_TYPE_ARK1668E) {
			//height +100 means add scale margin(\D3\E0\C1\BF).
			writel(((param->iheight+100+vcut) << 12) | (param->iwidth << 0), context->mmio_base + AXI_SCALE_VIDEO_SOURCE_SIZE);
			writel((param->iwinheight+100+vcut) << 12 | param->iwinwidth << 0, context->mmio_base + AXI_SCALE_VIDEO_WINDOW_SIZE);
		} else /* if(context->soc_type == SOC_TYPE_ARKN141)  */ {
			writel((param->iheight << 12) | (param->iwidth << 0), context->mmio_base + AXI_SCALE_VIDEO_SOURCE_SIZE);
			writel(param->iwinheight << 12 | param->iwinwidth << 0, context->mmio_base + AXI_SCALE_VIDEO_WINDOW_SIZE);
		}
		/* y_positong<<12 | x_position */
		writel((param->iy << 12) | (param->ix << 0), context->mmio_base + AXI_SCALE_VIDEO_WINDOW_POINT);
		/* Y (YUV/YUYV) data start address.*/
		writel(param->iyaddr, context->mmio_base + AXI_SCALE_VIDEO_ADDR1);
		/* U or UV data start address. */
		if(!param->iuaddr) {
			if(format == SCALE_FORMAT_Y_UV420 || \
				format == SCALE_FORMAT_YUV420 || \
				format == SCALE_FORMAT_Y_UV422 || \
				format == SCALE_FORMAT_YUV422) {
				param->iuaddr = param->iyaddr + ((param->iwidth+0xF)&(~0xF)) * ((param->iheight+0xF)&(~0xF));
			} else {
				param->iuaddr = 0;
			}
		}
		writel(param->iuaddr, context->mmio_base + AXI_SCALE_VIDEO_ADDR2);
		/* V data start address. */
		if(!param->ivaddr) {
			if(format == SCALE_FORMAT_Y_UV420) {
				param->ivaddr = 0;
			} else if(format == SCALE_FORMAT_YUV420) {
				param->ivaddr = param->iuaddr + ((param->iwidth+0xF)&(~0xF)) * ((param->iheight+0xF)&(~0xF)) / 4;
			} else if(format == SCALE_FORMAT_Y_UV422) {
				param->ivaddr = param->iuaddr;
			} else if(format == SCALE_FORMAT_YUV422) {
				param->ivaddr = param->iuaddr + ((param->iwidth+0xF)&(~0xF)) * ((param->iheight+0xF)&(~0xF)) / 2;
			} else {
				param->ivaddr = 0;
			}
		}
		writel(param->ivaddr, context->mmio_base + AXI_SCALE_VIDEO_ADDR3);

		/*scale output size */
		writel((param->oheight << 12) | (param->owidth << 0), context->mmio_base + AXI_SCALE_VIDEO_SIZE);

		if (param->iwinwidth > (param->owidth + hcut) * 3 / 2)
			do_horz_down_scalar = 1;

		if( do_horz_down_scalar == 0 )
			writel(0<<9| 0<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0,
				context->mmio_base + AXI_SCALE_SCALE_CTL);
		else
			writel(0<<9| 1<<8 | 1<<7| 1<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0,
				context->mmio_base + AXI_SCALE_SCALE_CTL);
		writel(512<<11 | 0<<0, context->mmio_base + AXI_SCALE_SCALE_VXMOD); /*V_xmod_odd_init<<11 | V_xmod_even_init */

		/*left cut number in line scaler<<18 | hfz */
		val = param->left_cut << 18 | (param->iwinwidth * 1024 / (param->owidth + hcut));
		writel(val, context->mmio_base + AXI_SCALE_SCALE_CTL0);
		/* Up cut line number in veritical scaler <<18 | vfz */
		val = param->up_cut << 18 | (param->iwinheight * 1024 / (param->oheight + vcut + 4));
 		writel(val, context->mmio_base + AXI_SCALE_SCALE_CTL1);
		/* right_cut_num<<8 | bottom_cut_num */
		val = param->right_cut << 8  | param->bottom_cut;
		writel(val, context->mmio_base + AXI_SCALE_RIGHT_BOTTOM_CUT_NUM);

		/* check output uaddr */
		if(!param->ouaddr) {
			if((param->oformat == SCALE_OUT_FORMAT_Y_UV420) || (param->oformat == SCALE_OUT_FORMAT_Y_UV422)) {
				param->ouaddr = param->oyaddr + ((param->owidth+0xF)&(~0xF)) * ((param->oheight+0xF)&(~0xF));
			} else if(param->oformat == SCALE_OUT_FORMAT_YUYV) {
				param->ouaddr = 0;
			}
		}
		//param->ovaddr = 0;
		if(context->soc_type == SOC_TYPE_ARKN141) {
			/*The number of the horizontal pix in write back ram*/
			writel(param->owidth, context->mmio_base + AXI_SCALE_WB_DATA_HSIZE_RAM);
			writel(param->oyaddr, context->mmio_base + AXI_SCALE_WB_DEST_YADDR);
			writel(param->ouaddr, context->mmio_base + AXI_SCALE_WB_DEST_UADDR);
			writel(param->ovaddr, context->mmio_base + AXI_SCALE_WB_DEST_VADDR);
		} else if(context->soc_type == SOC_TYPE_ARK1668E) {
			int index;
			//eliminate the filter line H.
			val = readl(context->mmio_base + AXI_SCALE_SCALE_CTL);
			val &= ~((1<<8)|(1<<6)|(1<<11)|(1<<12));
			val |= (1<<12);
			writel(val, context->mmio_base + AXI_SCALE_SCALE_CTL);

			//set ratate initilize value. it will be set again soon.
			writel(0x00988000, context->mmio_base + AXI_SCALE_ROTATA_CTL);

			//set filetr initilize value, it will be set again soon.
			writel(0x64, context->mmio_base + AXI_SCALE_VFILTER_CLR);

			//set default value.
			writel(0xFF, context->mmio_base + AXI_SCALE_WB_DATA_HSIZE_RAM);

#if 1		//filter.
			writel(0x0, context->mmio_base + AXI_SCALE_VFILTER_CLR);
			writel(0x0, context->mmio_base + AXI_SCALE_VFILTER_COEFF);
			index = get_scaler_cof(param->iwinheight, param->oheight);
			if(index < 16) {
				val = (ver_cof[index][1]<<24) | (ver_cof[index][0]<<16) | (1<<14) | param->iwinwidth;
				writel(val, context->mmio_base + AXI_SCALE_VFILTER_CLR);

				val = (ver_cof[index][5]<<24) | (ver_cof[index][4]<<16) | (ver_cof[index][3]<<8) | ver_cof[index][2];
				writel(val, context->mmio_base + AXI_SCALE_VFILTER_COEFF);

				val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
				val &=~(0xFF<<20);
				val |=(ver_cof[index][6]<<20);;
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);

				val = readl(context->mmio_base + AXI_SCALE_SCALE_CTL);
				val &=~(0x1<<24);
				val |=(1<<24);
				writel(val, context->mmio_base + AXI_SCALE_SCALE_CTL);
			}
#endif
			writel(param->oyaddr, context->mmio_base + AXI_SCALE_ROTATA_Y_FADR);
			writel(param->ouaddr, context->mmio_base + AXI_SCALE_ROTATA_C_FADR);

			if(param->oformat == SCALE_OUT_FORMAT_Y_UV420) {
				scale_rotate(context, SCALE_FORMAT_YUV420, param->rotate, param->owidth, param->oheight, param->oyaddr);
				val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
				val &= ~((1<<15) | (1<<28));
				val |=(1<<15);
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);
			} else if(param->oformat == SCALE_OUT_FORMAT_Y_UV422) {
				scale_rotate(context, SCALE_FORMAT_YUV422, param->rotate, param->owidth, param->oheight, param->oyaddr);
				val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
				val &= ~((1<<15) | (1<<28));
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);
			} else if(param->oformat == SCALE_OUT_FORMAT_YUYV) {
				scale_rotate(context, SCALE_FORMAT_YUV422, param->rotate, param->owidth, param->oheight, param->oyaddr);
				val = readl(context->mmio_base + AXI_SCALE_ROTATA_CTL);
				val |=(1<<17) | (1<<28);
				writel(val, context->mmio_base + AXI_SCALE_ROTATA_CTL);
			}
		}
		writel(0x3, context->mmio_base + AXI_SCALE_EN);
	} while (0);

	scale_peaking_denoise(context, param->owidth, param->oheight);

	return ret;
}

static inline void ark_scale_start_writeback(struct ark_scale_context *context)
{
   writel(1, context->mmio_base + AXI_SCALE_WB_START);
   udelay(50);
   writel(0, context->mmio_base + AXI_SCALE_WB_START);
}

static int ark_scale_wait_finish_int(struct ark_scale_context *context)
{
	int ret = wait_event_interruptible_timeout(context->waitq,
				context->busy == 0, msecs_to_jiffies(100));

	if (ret < 0)
		return ret;

	if (ret == 0) {
		printk(KERN_ALERT "wait scale finish timeout.\n");
		return -ETIMEDOUT;
	}

	return 0;
}

int ark_scale_get_busy_status(struct ark_scale_context *context)
{
	return context->busy;
}
EXPORT_SYMBOL(ark_scale_get_busy_status);

int ark_scale_start_nowait(struct ark_scale_context *context, struct ark_scale_param *param)
{
	if (unlikely(down_interruptible(&context->scale_sem))) {
		printk(KERN_ALERT "down_interruptible scale error\n");
		return -1;	
	}

	if (unlikely(context->busy)) {
		printk(KERN_ALERT "%s error! scale is busy now.\n", __FUNCTION__);
		return -1;
	}

	context->busy = 1;

	/* clk_disable_unprepare(context->clk);
	scale_softreset(context);
	clk_prepare_enable(context->clk); */

	ark_scale_set_param(context, param);
	ark_scale_start_writeback(context);

	return 0;
}
EXPORT_SYMBOL(ark_scale_start_nowait);

int ark_scale_wait_idle(struct ark_scale_context *context)
{
	int ret;

	ret = ark_scale_wait_finish_int(context);
	up(&context->scale_sem);

	return ret;
}
EXPORT_SYMBOL(ark_scale_wait_idle);

int ark_scale_start(struct ark_scale_context *context, struct ark_scale_param *param)
{
	int ret;

	ret = ark_scale_start_nowait(context, param);
	if (ret < 0) {
		up(&context->scale_sem);
		return ret;
	}

	ret = ark_scale_wait_finish_int(context);

	up(&context->scale_sem);

	return ret;
}
EXPORT_SYMBOL(ark_scale_start);

int ark_scale_dev_init(struct ark_scale_context *context)
{
	clk_disable_unprepare(context->clk);
	scale_softreset(context);
	clk_prepare_enable(context->clk);

	spin_lock_init(&context->lock);

	init_waitqueue_head(&context->waitq);

	return 0;
}

irqreturn_t ark_scale_intr_handler(int irq, void *dev_id)
{
	struct ark_scale_device *scale = (struct ark_scale_device *)dev_id;
	struct ark_scale_context *context = &scale->context;

	unsigned int status = readl(context->mmio_base + AXI_SCALE_INT_STATUS);

	do {
		if(status & (1 << 2))	// write back bresp error
			break;

		// AXI SCALE write back middle finish interupt
		//if(status & (1 << 5))
			//break;

		if(status & (1 << 0))	// write back frame finish interupt
			break;

		return IRQ_NONE;	//add.
	} while(0);

	writel(0, context->mmio_base + AXI_SCALE_EN);

	writel(0x07, context->mmio_base + AXI_SCALE_CLCD_INT_CLR);

	context->busy = 0;
	wake_up_interruptible(&context->waitq);

    return IRQ_HANDLED;
}
