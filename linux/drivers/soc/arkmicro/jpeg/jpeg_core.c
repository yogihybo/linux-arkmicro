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
 *      ark_jpeg_core.c
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
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/matroxfb.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>

#include "ark_jpeg_io.h"
#include "jpeg.h"
#include "jpeg_priv.h"

static struct ark_jpeg_context *core_jpeg_context;
//static struct ark_jpeg_context *arkjpeg_tasklet_data = NULL;
static int head_size;
static int jpeg_left_len;
static int JpegLeftLine = 0;
static int JpegBlkCount = 0;
static int JpegFrameStart = 0;

static unsigned int JpegSrcWidth = 0;
static unsigned int JpegSrcHeight = 0;
static unsigned int JpegBlkHeight = 0;
static unsigned int JpegPutWidth = 0;
static unsigned int JpegPutHeight = 0;

//static unsigned char *JpegDecPutAddr=NULL;
static void *JpegDecPutAddr = NULL;

static bool JpegScalerEn = false;

unsigned char *JpegScaBuf1 = NULL;
unsigned char *JpegScaBuf2 = NULL;
unsigned char *JpegScaBuf3 = NULL;

unsigned char *JpegBlkBuf1 = NULL;
unsigned char *JpegBlkBuf2 = NULL;
unsigned char *JpegBlkBuf3 = NULL;

unsigned int JpegBlkBuf1PhyAddr;
unsigned int JpegBlkBuf2PhyAddr;
unsigned int JpegBlkBuf3PhyAddr;

static bool hard_head_parser;
static bool head_parser_finished;

//static JpegStartAddr *decode_addr;

int ark_prescale_wait_block_int(void)
{
	struct ark_jpeg_context *context = core_jpeg_context;
	int ret;

	ret = wait_for_completion_timeout(&context->psblockint_completion, msecs_to_jiffies(1000));
	if(ret == 0)
	{
		printk(KERN_ALERT "ark_prescale_wait_block_int timeout.\n");
		return -1;	
	}	

	return 0;
}

int ark_prescale_wait_frame_int(void)
{
	struct ark_jpeg_context *context = core_jpeg_context;
	int ret;

	ret = wait_for_completion_timeout(&context->psframeint_completion, msecs_to_jiffies(1000));
	if(ret == 0)
	{
		printk(KERN_ALERT "ark_prescale_wait_frame_int timeout.\n");
		return -1;	
	}	

	return 0;
}

int ark_prescale_set_parameters(struct ark_prescale_cfg_arg *para)
{
	unsigned int hblank,vblank;
	struct ark_jpeg_context *context = core_jpeg_context;

	if(para->dst_width & 1) {
		printk(KERN_ALERT "PreScalerInit() : the dest width is not even.\n");
		return -1;
	}

	if(para->dst_width == 0 || para->dst_height == 0 || para->block_height == 0) {
		printk(KERN_ALERT "PreScalerInit() : illegal paremeter.\r\n");
		return -1;
	}

	hblank = para->left_blank + para->right_blank;
	vblank = para->up_blank + para->down_blank;
	writel(para->src_addr, context->ps_mmio_base + PRESCALE_SRC_ADDR);
	writel(para->dst_addr, context->ps_mmio_base + PRESCALE_DES_ADDR);
	writel(para->dst_addr, context->ps_mmio_base + PRESCALE_DES_ADDR2);
	writel(para->hv_addr1, context->ps_mmio_base + PRESCALE_HV_ADDR1);
	writel(para->hv_addr2, context->ps_mmio_base + PRESCALE_HV_ADDR2);
	writel(para->hv_addr3, context->ps_mmio_base + PRESCALE_HV_ADDR3);
	writel((para->src_width/(para->dst_width+hblank)<<10) + 
		(para->src_width%(para->dst_width+hblank))*1024/(para->dst_width+hblank),
		context->ps_mmio_base + PRESCALE_COEF_H);
	writel((para->src_height/(para->dst_height+vblank)<<10) + 
		(para->src_height%(para->dst_height+vblank))*1024/(para->dst_height+vblank), 
		context->ps_mmio_base + PRESCALE_COEF_V);
	writel(0xf, context->ps_mmio_base + PRESCALE_FILTER_CTL);
	writel((para->dst_width<<16) | para->src_width, context->ps_mmio_base + PRESCALE_LINE_CTL);
	writel((para->dst_height<<16) | para->src_height, context->ps_mmio_base + PRESCALE_HEIGHT_CTL);
	writel(para->block_height, context->ps_mmio_base + PRESCALE_BLK_HEIGHT);
	
	if(para->src_height%para->block_height) {
		writel(para->src_height / para->block_height + 1, context->ps_mmio_base + PRESCALE_BLK_NUM);
		writel(para->src_height % para->block_height, context->ps_mmio_base + PRESCALE_ROW_NUM_LAST_BLK);
	} else {
		writel(para->src_height / para->block_height, context->ps_mmio_base + PRESCALE_BLK_NUM);
		writel(para->block_height, context->ps_mmio_base + PRESCALE_ROW_NUM_LAST_BLK);
	}
	writel(ARKPRESCAL_WORKMODE_MIX, context->ps_mmio_base + PRESCALE_WORK_MODE);
	writel(ARKPRESCAL_VFORMAT_YUV422, context->ps_mmio_base + PRESCALE_SRC_MODE);
	writel(para->hori_format, context->ps_mmio_base + PRESCALE_CTL);	
	//if(pPrsCtx->ppCtx.dwHformat==PRESCALER_HFORMAT_RGB || pPrsCtx->ppCtx.dwHformat==PRESCALER_HFORMAT_YUV)
		//pPrsCtx->pPreScaReg->Pre_SCAL_CTL |= (1<<3);	
	writel((0<<11) | para->left_blank, context->ps_mmio_base + PRESCALE_HDATA_VALID);
	writel((0<<11) | para->up_blank, context->ps_mmio_base + PRESCALE_VDATA_VALID_VIDEO);

	return 0;	
}

void ark_prescale_set_frame_start(void)
{
	struct ark_jpeg_context *context = core_jpeg_context;

	writel(1, context->ps_mmio_base + PRESCALE_FRAME_START);
}

void ark_prescale_set_block_start(void)
{
	struct ark_jpeg_context *context = core_jpeg_context;

	writel(1, context->ps_mmio_base + PRESCALE_BLK_START);
}

void ark_prescale_set_src_addr(unsigned int addr)
{
	struct ark_jpeg_context *context = core_jpeg_context;

	writel(addr, context->ps_mmio_base + PRESCALE_SRC_ADDR);
}

irqreturn_t ark_prescale_intr_handler(int irq, void *dev_id)
{
	unsigned int intr_sts;
	struct ark_jpeg_context *context = core_jpeg_context;

    intr_sts = readl(context->ps_mmio_base + PRESCALE_INT_STATUS);
	writel(intr_sts, context->ps_mmio_base + PRESCALE_INT_CLR);
	
    switch (intr_sts & 0x3) {
    case 1: /* block finish  */
		complete(&context->psblockint_completion);
        break;
    case 2: /* frame finish  */
    case 3:
		complete(&context->psframeint_completion);
        break;
    }

    return IRQ_HANDLED;
}

int ark_jpeg_reg_check(struct ark_jpeg_context *context)
{
	int ret = 0;

	return ret;
}

/* This function programs hardware registers according to the settings
 * stored in the context
 */
int ark_jpeg_init(void)
{
	return 0;
}

int ark_jpeg_do_intr_check(struct ark_jpeg_context *context)
{
	return 0;
}

/* This is a tasklet function. It calls the driver functions to delver data
 * between the video post processing driver and output ring pairs.
 *
 * Arguments:
 *   un_used : un-used input
 *
 * Return:
 *   none
 */
/*void ark_jpeg_do_tasklet(unsigned long un_used)
{
    if (!arkjpeg_tasklet_data)
        return;

    ARKJPEG_DBGPRTK("%s %d: enter\n",
        __FUNCTION__, __LINE__);

}
DECLARE_TASKLET(arkjpeg_tasklet, ark_jpeg_do_tasklet, 0);*/

//#ifndef ARK_JPEG_USE_HW_EMULATION
/* This function is an interrupt service routine
 *
 * Arguments:
 *
 * Return:
 *
 */
irqreturn_t ark_jpeg_intr_handler(int irq, void *dev_id)
{
	unsigned int intr_sts;
	struct ark_jpeg_context *context = core_jpeg_context;

	intr_sts = readl(context->mmio_base + JPEG_INTCTRL);

	ARKJPEG_DBGPRTK("%s %d: irq status 0x%x\n", __FUNCTION__, __LINE__, intr_sts);

	context->intr_status = intr_sts;
	writel(intr_sts, context->mmio_base + JPEG_INTCLR);
	queue_work(context->wrok_queue, &context->jpeg_work);

	return IRQ_HANDLED;
}

int ark_jpeg_dev_init(struct ark_jpeg_context *context)
{
	ARKJPEG_DBGPRTK("%s %d: enter\n", __FUNCTION__, __LINE__);

	core_jpeg_context = context;

	/* initialize lock */
	spin_lock_init(&context->lock);

	/* initialize wait queue */
	init_waitqueue_head(&context->waitq);

	writel(0x3f, context->mmio_base + JPEG_INTCLR);

	return 0;
}

void JpegReset(void)
{
	unsigned int val;

	val = readl(core_jpeg_context->mmio_base + JPEG_CTRL);
	val |= (3 << 0);
	writel(val, core_jpeg_context->mmio_base + JPEG_CTRL);
	val &= ~(3 << 0);
	writel(val, core_jpeg_context->mmio_base + JPEG_CTRL);
}

static int SoftHeaderConfigReg(JPEG_SOF_PARA * SOF_PARA, JPEG_SOS_PARA * SOS_PARA, JPEG_DRI_PARA * DRI_PARA)
{
	unsigned int nBLK0, nBLK1, nBLK2;
	unsigned int format;

	JpegReset();
	format = (SOF_PARA->SOF_NS_PARA[0].H << 4) + SOF_PARA->SOF_NS_PARA[0].V;
	switch (format) {
	case 0x22:		//411
		nBLK0 = 3;
		nBLK1 = 0;
		nBLK2 = 0;
		break;
	case 0x12:		//422a
	case 0x21:		//422b
		nBLK0 = 1;
		nBLK1 = 0;
		nBLK2 = 0;
		break;
	case 0x11:		//444,100
		nBLK0 = 0;
		nBLK1 = 0;
		nBLK2 = 0;
		break;
	default:
		nBLK0 = 0;
		nBLK1 = 0;
		nBLK2 = 0;
		printk(KERN_ALERT "\nunknown jpeg format !!!!!!!!!!!\r\n");
		return 0;
	}

	switch (SOF_PARA->Nf) {
	case 4:
		//colspa = 4;
		break;
	case 3:
		//colspa = 2;
		break;
	case 1:
		//colspa = 1;
		break;
	default:
		return 1;
	}

	writel(((SOF_PARA->Y) << 16) | (0 << 8) | ((SOS_PARA->Ns -
						    1) << 6) |
	       (SOF_PARA->Colspctype << 4) | (1 << 3) | (DRI_PARA->restart_flag << 2) | ((SOF_PARA->Nf - 1) << 0),
	       core_jpeg_context->mmio_base + JPEG_1);
	writel(((SOF_PARA->X) << 16) | ((DRI_PARA->restart_interval - 1) << 0), core_jpeg_context->mmio_base + JPEG_3);
	writel((SOF_PARA->SOF_NS_PARA[0].H << 12) | (SOF_PARA->SOF_NS_PARA[0].V << 8) | (nBLK0 << 4) | (SOF_PARA->
													SOF_NS_PARA[0].
													Tq << 2) |
	       (SOS_PARA->SOS_NS_PARA[0].Td << 1)
	       | (SOS_PARA->SOS_NS_PARA[0].Ta << 0), core_jpeg_context->mmio_base + JPEG_4);
	writel((SOF_PARA->SOF_NS_PARA[1].H << 12) | (SOF_PARA->SOF_NS_PARA[1].V << 8) | (nBLK1 << 4) | (SOF_PARA->
													SOF_NS_PARA[1].
													Tq << 2) |
	       (SOS_PARA->SOS_NS_PARA[1].Td << 1)
	       | (SOS_PARA->SOS_NS_PARA[1].Ta << 0), core_jpeg_context->mmio_base + JPEG_5);
	writel((SOF_PARA->SOF_NS_PARA[2].H << 12) | (SOF_PARA->SOF_NS_PARA[2].V << 8) | (nBLK2 << 4) | (SOF_PARA->
													SOF_NS_PARA[2].
													Tq << 2) |
	       (SOS_PARA->SOS_NS_PARA[2].Td << 1)
	       | (SOS_PARA->SOS_NS_PARA[2].Ta << 0), core_jpeg_context->mmio_base + JPEG_6);
	writel((SOF_PARA->SOF_NS_PARA[3].H << 12) | (SOF_PARA->SOF_NS_PARA[3].V << 8) | (nBLK2 << 4) | (SOF_PARA->
													SOF_NS_PARA[3].
													Tq << 2) |
	       (SOS_PARA->SOS_NS_PARA[3].Td << 1)
	       | (SOS_PARA->SOS_NS_PARA[3].Ta << 0), core_jpeg_context->mmio_base + JPEG_7);

	return 1;
}

#define GET_STREAM_BYTE(a)		(*a++)

static int quan_table[4][64];
static int hea[64], heb[336];
static JPEG_SOF_PARA SOF_PARA = { 0 };

static int SoftJpegHead(unsigned int fileAddr, unsigned int fileSize)
{
	JPEG_SOS_PARA SOS_PARA = { 0 };
	JPEG_DRI_PARA DRI_PARA = { 0 };
	unsigned char HiByte, LoByte;
	unsigned char tmpBuf[10];
	int table_num;
	int len = 0;
	int i, j;
	signed char Tq;		//Pq  removed waring
	int sof_find = 0, huff_find = 0, quan_find = 0, sos_find = 0;
	int v, code, hid;
	int dc, ht, abase, bbase;	//,hbase,hb; removed waring
	int min[64];
	int l[16];
	unsigned char min0_tb, min1_tb, min2_tb, min3_tb, min4_tb, min5_tb, min6_tb, min7_tb;
	unsigned char min8_tb, min9_tb, min10_tb, min11_tb, min12_tb, min13_tb, min14_tb, min15_tb;
	unsigned int colspa;
	unsigned char *jpegFile = (unsigned char *)fileAddr;

	colspa = 0;
	DRI_PARA.restart_flag = 0;
	DRI_PARA.restart_interval = 1;
	for (i = 0; i < 336; i++) {
		heb[i] = 0;
	}

	if ((GET_STREAM_BYTE(jpegFile) != 0xff)
	    || (GET_STREAM_BYTE(jpegFile) != 0xd8)) {
		printk(KERN_ALERT "JpegHeadSoftDec begin error!\n");
		return -1;
	}

	while (1) {
		if (((unsigned int)jpegFile - fileAddr) > fileSize)
			return -1;
		HiByte = GET_STREAM_BYTE(jpegFile);
		if (HiByte == 0xff) {
			LoByte = GET_STREAM_BYTE(jpegFile);
			if (LoByte == 0xc2)	//not baseline
			{
				printk(KERN_ALERT "JpegHeadSoftDec format error!\n");
				return -1;
			}
			if ((LoByte != 0xff) && (LoByte != 0xc0)
			    && (LoByte != 0xd9) && (LoByte != 0xd8)
			    && (LoByte != 0xc4) && (LoByte != 0xdb)
			    && (LoByte != 0xda) && ((LoByte != 0xdd))) {
				HiByte = GET_STREAM_BYTE(jpegFile);
				LoByte = GET_STREAM_BYTE(jpegFile);
				len = (HiByte << 8) + LoByte - 2;
				jpegFile += len;
			}
//jpeg frame start                      
			else if (LoByte == 0xc0) {
				sof_find = 1;
				for (i = 0; i < 8; i++) {
					tmpBuf[i] = GET_STREAM_BYTE(jpegFile);
				}
				SOF_PARA.Lf = (tmpBuf[0] << 8) + tmpBuf[1];
				SOF_PARA.P = tmpBuf[2];
				SOF_PARA.Y = (tmpBuf[3] << 8) + tmpBuf[4];
				SOF_PARA.X = (tmpBuf[5] << 8) + tmpBuf[6];
				SOF_PARA.Nf = tmpBuf[7];
				if ((SOF_PARA.Y == 0) || (SOF_PARA.X == 0)) {
					printk(KERN_ALERT "abnormal jpeg size !\n");
					return -1;
				}
				if ((SOF_PARA.Nf != 3) && (SOF_PARA.Nf != 1)) {
					printk(KERN_ALERT "unknown colspctype !\n");
					return -1;
				}
				if (SOF_PARA.Nf > 255)
					return -1;
				for (i = 0; i < (SOF_PARA.Nf * 3); i += 3) {
					SOF_PARA.SOF_NS_PARA[i / 3].C = GET_STREAM_BYTE(jpegFile);
					tmpBuf[0] = GET_STREAM_BYTE(jpegFile);
					SOF_PARA.SOF_NS_PARA[i / 3].H = ((tmpBuf[0] & 0xf0) >> 4);
					SOF_PARA.SOF_NS_PARA[i / 3].V = tmpBuf[0] & 0x0f;
					SOF_PARA.SOF_NS_PARA[i / 3].Tq = GET_STREAM_BYTE(jpegFile);
				}
			}
//jpeg huffman table            
			else if (LoByte == 0xc4) {
				huff_find = 1;
				for (i = 0; i < 2; i++) {

					tmpBuf[i] = GET_STREAM_BYTE(jpegFile);
				}
				len = (tmpBuf[0] << 8) + tmpBuf[1];

				while ((len - 2) > 0) {

					v = GET_STREAM_BYTE(jpegFile);
					len -= 1;
					hid = ((v >> 4) != 0) ? 2 : 0;
					hid |= ((v & 15) != 0) ? 1 : 0;
					switch (hid) {
					case 1:
						//      hbase=368;
						break;
					case 2:
						//     hbase=0;
						break;
					case 3:
						//    hbase=176;
						break;
					default:
						//    hbase=352;
						break;
					}
					if ((v >> 4) != 0)
						abase = 0;
					else
						abase = 1;
					dc = abase;
					ht = v & 15;
					abase |= (ht << 1);
					switch (abase) {
					case 1:
					case 3:
						bbase = 162;
						break;
					case 2:
						bbase = 174;
						break;
					default:
						bbase = 0;
						break;
					}
					abase <<= 4;
					for (i = abase; i < abase + 16; i++) {
						if (i >= 64)
							return -1;
						hea[i] = 255;
					}
					for (i = 0; i < 16; i++) {

						l[i] = GET_STREAM_BYTE(jpegFile);
						len -= 1;
					}
					code = 0;
					for (i = 0; i < 16; i++, abase++) {
						if (abase >= 64)
							return -1;
						min[abase] = code;
						hea[abase] = bbase - code;
						if (l[i] != 0) {
							for (j = 0; j < l[i]; j++, bbase++) {

								v = GET_STREAM_BYTE(jpegFile);
								len -= 1;
								if (dc) {
									v &= 15;
									if (ht)
										v <<= 4;
									if (bbase >= 336)
										return -1;
									heb[bbase] |= v;
								} else {
									/*removed waring
									   if(v==0)
									   hb=160;
									   else if(v==0xf0)
									   hb=161;
									   else
									   hb=(v>>4)*10+(v&0xf)-1;
									 */
									if (bbase >= 336)
										return -1;
									heb[bbase] = v;
								}
								code++;
							}
						}
						code <<= 1;
					}

				}	//while len != 0
			}
//jpeg quantification table                     
			else if (LoByte == 0xdb) {
				quan_find = 1;
				for (i = 0; i < 2; i++) {
					tmpBuf[i] = GET_STREAM_BYTE(jpegFile);
				}
				len = (tmpBuf[0] << 8) + tmpBuf[1];
				switch (len) {
				case 67:
					table_num = 1;
					break;
				case 132:
					table_num = 2;
					break;
				case 197:
					table_num = 3;
					break;
				case 262:
					table_num = 4;
					break;
				default:
					table_num = 0;
					break;
				}
				if (table_num == 0) {
					printk(KERN_ALERT "unknown quant table !\n");
					return -1;
				}

				for (j = 0; j < table_num; j++) {

					tmpBuf[0] = GET_STREAM_BYTE(jpegFile);
					//      Pq = ((tmpBuf[0] & 0xf0) >> 4);
					Tq = tmpBuf[0] & 0xf;
					if (Tq >= 4)
						return -1;
					if (Tq > colspa) {
						colspa = Tq;
					}
					if (Tq >= 4)
						return -1;
					for (i = 0; i < 64; i++) {
						quan_table[Tq][i] = GET_STREAM_BYTE(jpegFile);
					}
				}
			}
//jpeg start of scan                    
			else if (LoByte == 0xda) {
				sos_find = 1;
				for (i = 0; i < 3; i++) {

					tmpBuf[i] = GET_STREAM_BYTE(jpegFile);
				}
				SOS_PARA.Ls = (tmpBuf[0] << 8) + tmpBuf[1];
				SOS_PARA.Ns = tmpBuf[2];
				//printk("\n jpeg sos Ns = %d\n",SOS_PARA.Ns);
				if (SOS_PARA.Ns > 4)
					return -1;
				for (i = 0; i < (SOS_PARA.Ns * 2); i += 2) {
					SOS_PARA.SOS_NS_PARA[i / 2].Cs = GET_STREAM_BYTE(jpegFile);

					tmpBuf[0] = GET_STREAM_BYTE(jpegFile);
					SOS_PARA.SOS_NS_PARA[i / 2].Td = ((tmpBuf[0] & 0xf0) >> 4);
					SOS_PARA.SOS_NS_PARA[i / 2].Ta = tmpBuf[0] & 0x0f;
				}

				SOS_PARA.Ss = GET_STREAM_BYTE(jpegFile);

				SOS_PARA.Se = GET_STREAM_BYTE(jpegFile);

				tmpBuf[0] = GET_STREAM_BYTE(jpegFile);
				SOS_PARA.Ah = ((tmpBuf[0] & 0xf0) >> 4);
				SOS_PARA.Ai = tmpBuf[0] & 0x0f;
			}
//jpeg restart  
			else if (LoByte == 0xdd) {
				for (i = 0; i < 4; i++) {

					tmpBuf[i] = GET_STREAM_BYTE(jpegFile);
				}
				i = (tmpBuf[0] << 8) + tmpBuf[1];

				if (((tmpBuf[0] << 8) + tmpBuf[1]) != 4) {
					printk(KERN_ALERT "wrong number of DRI marker!\n");
					return -1;
				}
				DRI_PARA.restart_interval = (tmpBuf[2] << 8) + tmpBuf[3];
				DRI_PARA.restart_flag = 1;
				if (DRI_PARA.restart_interval == 0) {
					DRI_PARA.restart_flag = 0;
					DRI_PARA.restart_interval = 1;
				}
			}
		}

		SOF_PARA.Colspctype = colspa;
//config register               
		if ((sof_find == 1) && (sos_find == 1) && (huff_find == 1)
		    && (quan_find == 1)) {
			ARKJPEG_DBGPRTK("soft jpeg find all marker!\n");
			SoftHeaderConfigReg(&SOF_PARA, &SOS_PARA, &DRI_PARA);
			writel(readl(core_jpeg_context->mmio_base + JPEG_CTRL) | (1 << 15),
			       core_jpeg_context->mmio_base + JPEG_CTRL);
//write quant table                     
			for (i = 0; i < (colspa + 1); i++) {
				for (j = 0; j < 64; j++) {
					*((volatile unsigned int *)JPEG_QT + i * 64 + j) = quan_table[i][j];
				}
			}
//write base
			for (i = 0; i < 64; i++) {
				*((volatile unsigned int *)(JPEG_BASE) + i) = (hea[i] & 0x1ff);
			}
//write symb
			for (i = 0; i < 336; i++) {
				*((volatile unsigned int *)(JPEG_SYMB) + i) = (heb[i] & 0xff);
			}
//write min
			j = 0;

			for (i = 0; i < 64;) {
				min0_tb = min[i++] & 1;
				min1_tb = min[i++] & 3;
				min2_tb = min[i++] & 7;
				min3_tb = min[i++] & 15;
				min4_tb = min[i++] & 31;
				min5_tb = min[i++] & 63;
				min6_tb = min[i++] & 127;
				min7_tb = min[i++] & 255;
				min8_tb = min[i++] & 255;
				min9_tb = min[i++] & 255;
				min10_tb = min[i++] & 255;
				min11_tb = min[i++] & 255;
				min12_tb = min[i++] & 255;
				min13_tb = min[i++] & 255;
				min14_tb = min[i++] & 255;
				min15_tb = min[i++] & 255;
				*((volatile unsigned int *)(JPEG_MIN) + j++) =
				    (min15_tb + (min14_tb << 8) + (min13_tb << 16) + (min12_tb << 24));
				*((volatile unsigned int *)(JPEG_MIN) + j++) =
				    (min11_tb + (min10_tb << 8) + (min9_tb << 16) + (min8_tb << 24));
				*((volatile unsigned int *)(JPEG_MIN) + j++) =
				    (min7_tb + (min6_tb << 8) + (min5_tb << 15) + (min4_tb << 21) + (min3_tb << 26) +
				     ((min2_tb & 0x3) << 30));
				*((volatile unsigned int *)(JPEG_MIN) + j++) =
				    ((min2_tb >> 2) + (min1_tb << 1) + (min0_tb << 3));
			}
			writel(readl(core_jpeg_context->mmio_base + JPEG_COUNT)
			       & ~(1 << 15), core_jpeg_context->mmio_base + JPEG_COUNT);
			break;
		}
	}
	ARKJPEG_DBGPRTK("exit soft jpeg header\n");
	return ((int)jpegFile - fileAddr);
}

void JpegLoadStartAddr(JpegStartAddr * wAddr)
{
	writel(wAddr->wAddr1, core_jpeg_context->mmio_base + JPEG_WRSTA);
	writel(wAddr->wAddr1 + wAddr->wSize1, core_jpeg_context->mmio_base + JPEG_WREND);
	writel(wAddr->wAddr2, core_jpeg_context->mmio_base + JPEG_WRSTA1);
	writel(wAddr->wAddr2 + wAddr->wSize2, core_jpeg_context->mmio_base + JPEG_WREND1);
	writel(wAddr->wAddr3, core_jpeg_context->mmio_base + JPEG_WRSTA2);
	writel(wAddr->wAddr3 + wAddr->wSize3, core_jpeg_context->mmio_base + JPEG_WREND2);

	writel(readl(core_jpeg_context->mmio_base + JPEG_CTRL) | (1 << 13), core_jpeg_context->mmio_base + JPEG_CTRL);
}

void JpegContinueWork()
{
	writel(readl(core_jpeg_context->mmio_base + JPEG_CTRL) | (1 << 6), core_jpeg_context->mmio_base + JPEG_CTRL);
}

unsigned int JpegReadFile(unsigned int size)
{
	long ret;
	struct ark_jpeg_context *context = core_jpeg_context;

	context->api_info.EventType = FREAD;
	context->api_info.dwReadLen = size;
	complete(&context->api_completion);
	ret = wait_for_completion_timeout(&context->apidone_completion, msecs_to_jiffies(JPEG_API_TIMEOUT));
	if (ret == 0) {
		printk(KERN_ALERT "JpegReadFile timeout.\n");
		return 0;
	} else {
		/*{
		   int i;
		   for(i = 0; i < 8; i++)
		   printk(KERN_ALERT "0x%.8x, ", *(unsigned int*)(context->buf_base_virt+i*4));
		   printk(KERN_ALERT "\n");
		   for(i = 0; i < 8; i++)
		   printk(KERN_ALERT "0x%.8x, ", *(unsigned int*)(context->buf_base_virt+200*1024-8*4+i*4));
		   printk(KERN_ALERT "\n");
		   } */
		return context->api_retinfo.dwReadedLen;
	}
}

int JpegFileSeek(int offset, int origin)
{
	long ret;
	struct ark_jpeg_context *context = core_jpeg_context;

	context->api_info.EventType = FSEEK;
	context->api_info.lOffset = offset;
	context->api_info.nOrigin = origin;
	complete(&context->api_completion);
	ret = wait_for_completion_timeout(&context->apidone_completion, msecs_to_jiffies(JPEG_API_TIMEOUT));
	if (ret == 0) {
		printk(KERN_ALERT "JpegFileSeek timeout.\n");
		return -1;
	} else
		return context->api_retinfo.nSeekRet;
}

bool dest_size(unsigned src_width, unsigned src_height, unsigned input_width, unsigned input_height,
	       unsigned *dest_width, unsigned *dest_height)
{
	unsigned int width, height;
	struct ark_jpeg_context *context = core_jpeg_context;

	if (src_width == 0 || src_height == 0) {
		printk(KERN_ALERT "Picture type is not supported !\n");
		context->decode_result = DEC_ERROR;
		return false;
	}

	if ((src_width > MAXIMAGEWIDTH) || (src_height > MAXIMAGEHEIGHT)) {
		printk(KERN_ALERT "Error : Picture too large  !\n");
		context->decode_result = DEC_ERROR;
		return false;
	}

	if (context->rotate_angle == CLOCKWISE_90 || context->rotate_angle == CLOCKWISE_270) {
		width = input_height;
		height = input_width;
	} else {
		width = input_width;
		height = input_height;
	}

	if (context->scaler_mode == NO_SCALER) {
		*dest_width = src_width;
		*dest_height = src_height;
	} else if (context->scaler_mode == NOMAL_SCALER) {
		if (context->zoom_mode == ZOOM_IN_ONLY) {
			*dest_width = (src_width > width) ? width : src_width;
			*dest_height = (src_height > height) ? height : src_height;
		} else if (context->zoom_mode == ZOOM_OUT_ONLY) {
			*dest_width = (src_width < width) ? width : src_width;
			*dest_height = (src_height < height) ? height : src_height;
		} else {
			*dest_width = width;
			*dest_height = height;
		}
	} else if (context->scaler_mode == UNIFORM_SCALER) {
		unsigned int hcoff = width * 1024 / src_width;
		unsigned int vcoff = height * 1024 / src_height;

		if (context->zoom_mode == ZOOM_IN_ONLY) {
			if (hcoff >= 1024 && vcoff >= 1024) {
				*dest_width = src_width;
				*dest_height = src_height;
			} else {
				if (hcoff < vcoff) {
					*dest_width = width;
					*dest_height = hcoff * src_height / 1024;
				} else {
					*dest_height = height;
					*dest_width = vcoff * src_width / 1024;
				}
			}
		} else if (context->zoom_mode == ZOOM_OUT_ONLY) {
			if (hcoff > 1024 && vcoff > 1024) {
				if (hcoff < vcoff) {
					*dest_width = width;
					*dest_height = hcoff * src_height / 1024;
				} else {
					*dest_height = height;
					*dest_width = vcoff * src_width / 1024;
				}
			} else {
				*dest_width = src_width;
				*dest_height = src_height;
			}
		} else {
			if (hcoff < vcoff) {
				*dest_width = width;
				*dest_height = hcoff * src_height / 1024;
			} else {
				*dest_height = height;
				*dest_width = vcoff * src_width / 1024;
			}
		}
	}
	//for 2d process align
	*dest_width &= ~7;

	return true;
}

int is_scaler(unsigned src_width, unsigned src_height, unsigned dest_width, unsigned dest_height)
{
	return src_width != dest_width ? 1 : src_height != dest_height ? 1 : 0;
}

int do_scaler(JpegStartAddr * wAddr, unsigned src_width, unsigned src_height, unsigned dest_width, unsigned dest_height,
	      unsigned format)
{
	unsigned dwBlkSize;
	struct ark_jpeg_context *context = core_jpeg_context;
	struct ark_prescale_cfg_arg scapara = { 0 };

	scapara.src_addr = wAddr->wAddr1;
	scapara.dst_addr = context->decode_buf_base_phys;
	scapara.src_width = src_width;
	scapara.src_height = src_height;
	scapara.dst_width = dest_width;
	scapara.dst_height = dest_height;
	scapara.hori_format = format;

	if (src_height <= JPEG_BLOCK_HEIGHT)
		JpegBlkHeight = src_height;
	else
		JpegBlkHeight = JPEG_BLOCK_HEIGHT;

	dwBlkSize = dest_width * JpegBlkHeight * 2;
	scapara.block_height = JpegBlkHeight;
	scapara.hv_addr1 = (wAddr->wAddr3 + wAddr->wSize3 + 31) & ~31;
	scapara.hv_addr2 = (scapara.hv_addr1 + dwBlkSize + 31) & ~31;
	scapara.hv_addr3 = (scapara.hv_addr2 + dwBlkSize + 31) & ~31;
	if (dest_height < src_height)
		scapara.up_blank = 8;
	if (dest_width < src_width)
		scapara.left_blank = 2;

	return ark_prescale_set_parameters(&scapara);
}

bool JpegHeaderIntHandler()
{
	int ret;
	JpegStartAddr wAddr;
	unsigned int dwBlkSize;
	unsigned int input_width, input_height;
	struct ark_jpeg_context *context = core_jpeg_context;

	head_parser_finished = 1;
	JpegSrcWidth = (readl(context->mmio_base + JPEG_1) >> 16) & 0x0000ffff;
	JpegLeftLine = JpegSrcHeight = (readl(context->mmio_base + JPEG_3) >> 16) & 0x0000ffff;

	input_width = context->dst_width;
	input_height = context->dst_height;

	ret = dest_size(JpegSrcWidth, JpegSrcHeight, input_width, input_height, &JpegPutWidth, &JpegPutHeight);
	if (ret == false) {
		printk("%s : jpeg dest_size error\n", __func__);
		return false;
	}

	ARKJPEG_DBGPRTK("jpg scaler %d,%d,%d,%d\n", JpegSrcWidth, JpegSrcHeight, JpegPutWidth, JpegPutHeight);

	JpegDecPutAddr = context->decode_buf_base_virt;

	JpegScalerEn = is_scaler(JpegSrcWidth, JpegSrcHeight, JpegPutWidth, JpegPutHeight);

	if (JpegScalerEn) {
		if (JpegSrcHeight > JPEG_BLOCK_HEIGHT && JpegSrcHeight <= JPEG_BLOCK_HEIGHT * 2) {
			JpegBlkHeight = JPEG_BLOCK_HEIGHT / 2;
		} else
			JpegBlkHeight = JPEG_BLOCK_HEIGHT;
	} else
		JpegBlkHeight = JPEG_BLOCK_HEIGHT;

	context->src_width = JpegSrcWidth;
	context->src_height = JpegSrcHeight;
	context->out_width = JpegPutWidth;
	context->out_height = JpegPutHeight;
	context->decode_size = JpegPutWidth * JpegPutHeight * 2;

	dwBlkSize = JpegBlkHeight * JpegSrcWidth * 2;

	if (JpegPutWidth * JpegPutHeight * 2 > context->decode_buf_size
	    || context->buf_size <
	    JPEG_INPUT_BUFFERSIZE + dwBlkSize * 3 + (JpegScalerEn ? JpegPutWidth * JpegBlkHeight * 2 * 3 : 0)) {
		printk(KERN_ALERT "No enough memory to decode picture.\n");
		context->decode_result = DEC_ERROR;
		return false;
	}

	JpegBlkBuf1 = (unsigned char *)(context->buf_base_virt + JPEG_INPUT_BUFFERSIZE);
	JpegBlkBuf2 = JpegBlkBuf1 + dwBlkSize;
	JpegBlkBuf3 = JpegBlkBuf2 + dwBlkSize;
	JpegBlkBuf1PhyAddr = context->buf_base_phys + JPEG_INPUT_BUFFERSIZE;
	JpegBlkBuf2PhyAddr = JpegBlkBuf1PhyAddr + dwBlkSize;
	JpegBlkBuf3PhyAddr = JpegBlkBuf2PhyAddr + dwBlkSize;

	wAddr.wAddr1 = JpegBlkBuf1PhyAddr;
	wAddr.wSize1 = dwBlkSize;
	wAddr.wAddr2 = wAddr.wAddr1 + wAddr.wSize1;
	wAddr.wSize2 = dwBlkSize;
	wAddr.wAddr3 = wAddr.wAddr2 + wAddr.wSize2;
	wAddr.wSize3 = dwBlkSize;

	//decode_addr = &wAddr;

	JpegLoadStartAddr(&wAddr);

	if (JpegScalerEn) {
		if (do_scaler(&wAddr, JpegSrcWidth, JpegSrcHeight, JpegPutWidth, JpegPutHeight, ARKPRESCAL_HFORMAT_YUV422) < 0)
			return false;
	}

	if (context->break_decode) {
		context->decode_result = DEC_BREAKED;
		return false;
	}

	return true;
}

bool JpegBufferIntHandler()
{
	struct ark_jpeg_context *context = core_jpeg_context;
	//printk("[%s][%d]\n", __func__, __LINE__);
	if (jpeg_left_len > JPEG_FILE_BUFFERSIZE * 1024) {
		if (JpegReadFile(JPEG_FILE_BUFFERSIZE * 1024) != JPEG_FILE_BUFFERSIZE * 1024) {
			printk(KERN_ALERT "read file err 1!\n");
			context->decode_result = DEC_ERROR;
			return false;
		}
		jpeg_left_len -= JPEG_FILE_BUFFERSIZE * 1024;
	} else if (jpeg_left_len > 0) {
		if (JpegReadFile(jpeg_left_len) != jpeg_left_len) {
			printk(KERN_ALERT "read file err 2!\n");
			context->decode_result = DEC_ERROR;
			return false;
		}
		jpeg_left_len = 0;
	} else {
		if (jpeg_left_len < 0) {
			printk(KERN_ALERT "No JPEG frame int when file is all readed!\n");
			context->decode_result = DEC_ERROR;
			return false;
		}
		jpeg_left_len--;
	}

	return true;
}

bool JpegBlockIntHandler()
{
	unsigned int blksize;
	struct ark_jpeg_context *context = core_jpeg_context;
	//printk("[%s][%d] JpegBlkCount:%d\n", __func__, __LINE__, JpegBlkCount);
	JpegLeftLine -= JpegBlkHeight;
	blksize = JpegBlkHeight * JpegSrcWidth * 2;

	if (JpegBlkCount == 0) {
		if (JpegScalerEn) {
			if (JpegFrameStart == 0) {
				if (context->break_decode) {
					context->decode_result = DEC_BREAKED;
					return false;
				}
				ark_prescale_set_frame_start();
				JpegFrameStart = 1;
			} else {
				if (ark_prescale_wait_block_int() != 0) {
					context->decode_result = DEC_ERROR;
					return false;
				}
				if (context->break_decode) {
					context->decode_result = DEC_BREAKED;
					return false;
				}
				ark_prescale_set_src_addr(JpegBlkBuf1PhyAddr);
				ark_prescale_set_block_start();
			}
		} else
			memcpy(JpegDecPutAddr, JpegBlkBuf1, blksize);
	} else if (JpegBlkCount == 1) {
		if (JpegScalerEn) {
			if (ark_prescale_wait_block_int() != 0) {
				context->decode_result = DEC_ERROR;
				return false;
			}
			if (context->break_decode) {
				context->decode_result = DEC_BREAKED;
				return false;
			}
			ark_prescale_set_src_addr(JpegBlkBuf2PhyAddr);
			ark_prescale_set_block_start();
		} else
			memcpy(JpegDecPutAddr, JpegBlkBuf2, blksize);
	} else {
		if (JpegScalerEn) {
			if (ark_prescale_wait_block_int() != 0) {
				context->decode_result = DEC_ERROR;
				return false;
			}
			if (context->break_decode) {
				context->decode_result = DEC_BREAKED;
				return false;
			}
			ark_prescale_set_src_addr(JpegBlkBuf3PhyAddr);
			ark_prescale_set_block_start();
		} else
			memcpy(JpegDecPutAddr, JpegBlkBuf3, blksize);
	}

	if (context->break_decode) {
		context->decode_result = DEC_BREAKED;
		return false;
	}

	JpegBlkCount++;
	if (JpegBlkCount == 3)
		JpegBlkCount = 0;
	JpegDecPutAddr += blksize;

	return true;
}

bool JpegFrameIntHandler()
{
	unsigned int blksize;
	struct ark_jpeg_context *context = core_jpeg_context;

	//printk("[%s][%d] JpegBlkCount:%d\n", __func__, __LINE__, JpegBlkCount);
	if (JpegLeftLine) {
		blksize = JpegLeftLine * JpegSrcWidth * 2;

		if (JpegBlkCount == 0) {
			if (JpegScalerEn) {
				if (JpegFrameStart == 0) {
					ark_prescale_set_frame_start();
					JpegFrameStart = 1;
				} else {
					if (ark_prescale_wait_block_int() != 0) {
						context->decode_result = DEC_ERROR;
						return false;
					}
					if (context->break_decode) {
						context->decode_result = DEC_BREAKED;
						return false;
					}
					ark_prescale_set_src_addr(JpegBlkBuf1PhyAddr);
					ark_prescale_set_block_start();
				}
			} else
				memcpy(JpegDecPutAddr, JpegBlkBuf1, blksize);

		} else if (JpegBlkCount == 1) {
			if (JpegScalerEn) {
				if (ark_prescale_wait_block_int() != 0) {
					context->decode_result = DEC_ERROR;
					return false;
				}
				if (context->break_decode) {
					context->decode_result = DEC_BREAKED;
					return false;
				}
				ark_prescale_set_src_addr(JpegBlkBuf2PhyAddr);
				ark_prescale_set_block_start();
			} else
				memcpy(JpegDecPutAddr, JpegBlkBuf2, blksize);
		} else {
			if (JpegScalerEn) {
				if (ark_prescale_wait_block_int() != 0) {
					context->decode_result = DEC_ERROR;
					return false;
				}

				if (context->break_decode) {
					context->decode_result = DEC_BREAKED;
					return false;
				}
				ark_prescale_set_src_addr(JpegBlkBuf3PhyAddr);
				ark_prescale_set_block_start();
			} else
				memcpy(JpegDecPutAddr, JpegBlkBuf3, blksize);
		}
	}

	if (JpegScalerEn) {
		if (ark_prescale_wait_frame_int() != 0) {
			context->decode_result = DEC_ERROR;
			return false;
		}
	}
	return true;
}

void JpegInit(void)
{
	struct ark_jpeg_context *context = core_jpeg_context;

	//enable hardware header parser;the CODEC acts as a decoder.
	if (context->hard_head_parser)
		writel((1 << 8) | (1 << 3), context->mmio_base + JPEG_1);
	writel(0x28000738, context->mmio_base + JPEG_CTRL);	//Disable read buffer mode
	//when error appears, you disable reset the module
	//jp_clk_divnum:1 (2?)
	//RFTH indicates the FIFO level:2'b01 25-50% full
	//bypass_filter
	//jp_hp_stall_en
	//jp_hpbuf_stall_clr: Head parser and buffer stall clear.
	//jp_hp_stall_en: JPEG head parser stall enable

	//? The counter is used for judging the status of the JPEG. If the counter count the up limit value, indicating an error generation.
	writel(0xff, context->mmio_base + JPEG_COUNT);
	writel(0xff, context->mmio_base + JPEG_INTCLR);
	writel(0x27, context->mmio_base + JPEG_INTMASK);	//Jpeg input buffer interrupt enable:
	//FIFO empty interrupt             disable:
	//Jpeg decode error interrupt   disable:
	//Frame Interrupt enable:
	//Jpeg output buffer interrupt enable:
	//Head decode interrupt enable:
	//JPEG_FILE_BUFFERSIZE:  4*1024
	if (context->read_buf_mode) {
		//Enable read buffer mode; 
		writel(readl(context->mmio_base + JPEG_CTRL) | (1 << 30) | (JPEG_FILE_BUFFERSIZE << 16),
		       context->mmio_base + JPEG_CTRL);
		writel(context->buf_base_phys, context->mmio_base + JPEG_DEC_RD_BASE_ADDR);
	} else {
		writel(context->buf_base_phys, context->mmio_base + JPEG_DEC_RD_BASE_ADDR);
	}
}

void JepgStartDec(void)
{
	writel(1, core_jpeg_context->mmio_base + JPEG_0);
	writel(readl(core_jpeg_context->mmio_base + JPEG_START) | (1 << 31), core_jpeg_context->mmio_base + JPEG_START);
}

void JpegDecode(void)
{
	long ret;
	struct ark_jpeg_context *context = core_jpeg_context;

	jpeg_left_len = context->file_size;

	ARKJPEG_DBGPRTK("jpeg_left_len=%d\n", jpeg_left_len);

	if (hard_head_parser) {
		context->hard_head_parser = true;
		head_parser_finished = 0;
		context->scaler_enable = 0;
		JpegFileSeek(0, SEEK_SET);
	} else {
		hard_head_parser = true;
		context->hard_head_parser = false;
		JpegFileSeek(head_size, SEEK_SET);
		jpeg_left_len -= head_size;
	}

	if (jpeg_left_len > JPEG_FILE_BUFFERSIZE * 1024) {
		if (JpegReadFile(JPEG_FILE_BUFFERSIZE * 1024) != JPEG_FILE_BUFFERSIZE * 1024) {
			printk(KERN_ALERT "read file err 0\n");
			context->decode_result = DEC_ERROR;
			return;
		}
		jpeg_left_len -= JPEG_FILE_BUFFERSIZE * 1024;
	} else if (jpeg_left_len > 0) {
		if (JpegReadFile(jpeg_left_len) != jpeg_left_len) {
			printk(KERN_ALERT "read file err 1\n");
			context->decode_result = DEC_ERROR;
			return;
		}
		jpeg_left_len = 0;
	}

	JpegBlkCount = 0;
	JpegFrameStart = 0;
	context->read_buf_mode = 1;
	JpegInit();
	JepgStartDec();

	ret = wait_for_completion_timeout(&context->decdone_completion, msecs_to_jiffies(JPEG_TIMEOUT));
	if (ret == 0) {
		printk(KERN_ALERT "Wait jpeg int timeout.\n");
		context->decode_result = DEC_ERROR;
	}

	return;
}

void JpegPicDec(void)
{
	unsigned int filelen;
	struct ark_jpeg_context *context = core_jpeg_context;

	JpegReset();

	context->break_decode = false;
	hard_head_parser = true;

JpegDec_Start:

	JpegDecode();

	if (context->decode_result == DEC_ERROR) {
		if (head_parser_finished == 0)	//Harder header parser fail
		{
			filelen = context->file_size;
			JpegFileSeek(0, SEEK_SET);
			filelen = filelen > JPEG_MAX_HEADERSIZE ? JPEG_MAX_HEADERSIZE : filelen;
			if (JpegReadFile(filelen) == filelen) {
				head_size = SoftJpegHead((unsigned int)
							 context->buf_base_virt, filelen);
				if (head_size < 0) {
					printk(KERN_ALERT "soft header parser error!\n");
				} else {
					hard_head_parser = false;
					if (JpegHeaderIntHandler())
						goto JpegDec_Start;
				}
			} else
				printk(KERN_ALERT "read file err 3!\n");
		} else {
			printk(KERN_ALERT "jpeg decode error!\n");
		}
	}
}
