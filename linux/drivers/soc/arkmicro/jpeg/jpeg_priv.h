/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * Name:
 *      ark_jpeg_priv.h
 *
 * Description:
 *
 * 
 * Author:
 *      Sim
 *
 * Remarks:
 *
 */

#ifndef __ARK_JPEG_PRIV_H__
#define __ARK_JPEG_PRIV_H__

/*************************************************************************
 * Definitions and macros
 *************************************************************************/
typedef struct {
	unsigned int width;
	unsigned int height;
	unsigned int *buf;
	unsigned int data_format;
} PIC_INFO;

typedef struct {
	int width;
	int height;
	int format;
} SoftJpegHeaderInfo;

typedef struct {
	unsigned char Cs;
	unsigned char Td;
	unsigned char Ta;

} JPEG_SOS_NS_PARA;

typedef struct {
	unsigned short Ls;
	unsigned char Ns;
	unsigned char Ss;
	unsigned char Se;
	unsigned char Ah;
	unsigned char Ai;
	JPEG_SOS_NS_PARA SOS_NS_PARA[4];
} JPEG_SOS_PARA;

typedef struct {
	unsigned char C;
	unsigned char H;
	unsigned char V;
	unsigned char Tq;
} JPEG_SOF_NS_PARA;

typedef struct {
	unsigned short Lf;
	unsigned char P;
	unsigned short Y;
	unsigned short X;
	unsigned char Nf;
	unsigned char Colspctype;
	JPEG_SOF_NS_PARA SOF_NS_PARA[255];
} JPEG_SOF_PARA;

typedef struct {
	unsigned int restart_interval;
	unsigned int restart_flag;
} JPEG_DRI_PARA;

typedef struct {
	unsigned int wAddr1;
	unsigned int wSize1;
	unsigned int wAddr2;
	unsigned int wSize2;
	unsigned int wAddr3;
	unsigned int wSize3;
} JpegStartAddr;

#define ARKPRESCAL_WORKMODE_VIDEO    0
#define ARKPRESCAL_WORKMODE_HONLY    1
#define ARKPRESCAL_WORKMODE_VONLY    2
#define ARKPRESCAL_WORKMODE_MIX      3

#define ARKPRESCAL_FORMAT_RGB        0
#define ARKPRESCAL_FORMAT_YUV        1
#define ARKPRESCAL_FORMAT_YUV422     2
#define ARKPRESCAL_FORMAT_Y_U_V420   3
#define ARKPRESCAL_FORMAT_Y_UV420    4

#define ARKPRESCAL_HFORMAT_RGB			0
#define ARKPRESCAL_HFORMAT_YUV			1
#define ARKPRESCAL_HFORMAT_YUV422		3
#define ARKPRESCAL_VFORMAT_YUV422		0
#define ARKPRESCAL_VFORMAT_Y_U_V420		2
#define ARKPRESCAL_VFORMAT_Y_UV420		3

struct ark_prescale_cfg_arg {
    unsigned int src_addr;
    unsigned int dst_addr;
	unsigned int hv_addr1;
	unsigned int hv_addr2;
	unsigned int hv_addr3;
    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_width;
    unsigned int dst_height;
	unsigned int block_height;
	unsigned int left_blank;
	unsigned int right_blank;
	unsigned int up_blank;
	unsigned int down_blank;
	unsigned int hori_format;
	unsigned int vert_format;
};

/*************************************************************************
 * Functions
 *************************************************************************/

/* core functions */
int ark_jpeg_reg_check(struct ark_jpeg_context *context);
int ark_jpeg_dev_init(struct ark_jpeg_context *context);
irqreturn_t ark_jpeg_intr_handler(int irq, void *dev_id);
irqreturn_t ark_prescale_intr_handler(int irq, void *dev_id);
void re_scaler(JPEG_DECODE_OPT * dec);
int get_part_pic(struct part_pic *pic);
void JpegPicDec(void);
bool JpegHeaderIntHandler(void);
bool JpegFrameIntHandler(void);
bool JpegBufferIntHandler(void);
bool JpegBlockIntHandler(void);
void JpegContinueWork(void);

#endif //#ifndef __ARK_JPEG_PRIV_H__
