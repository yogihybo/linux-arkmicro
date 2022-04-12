#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/io.h>

#include "isp_scale.h"

static xm_isp_scalar_configuration_parameters isp_scalar_parameters;

int xm_isp_scalar_config (struct ark_isp_scale_context *context,
	xm_isp_scalar_configuration_parameters  *scalar_parameters)
{
	int val;
	int do_horz_down_scalar = 0;		// down scalar��Ҫʹ��"�з\u0153���˲\u0161"

   if( !scalar_parameters )
	{
		XM_printf("xm_isp_scalar_config scalar_parameters == NULL ! \r\n");
		return XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
	}
	if( scalar_parameters->mid_line >= scalar_parameters->dst_height )
	{
		XM_printf("xm_isp_scalar_config mid_line > dst_height! \r\n");
		return XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
	}

	// �ж��Ƿ��\u017d��С����LCD(С��VGA). ����,����4֡���\u0178
#if HYBRID_H264_MJPG
	writel(0xFEFEFEFE, context->mmio_base + ISP_SCALE_FRAME_MASK);
#elif CZS_USB_01
	if(scalar_parameters->dst_width >= 1024 && scalar_parameters->dst_height >= 600)
		writel(0x77777777, context->mmio_base + ISP_SCALE_FRAME_MASK);
	else
		writel(0xFEFEFEFE, context->mmio_base + ISP_SCALE_FRAME_MASK);
#elif TULV
	writel(0xFFFFFFFF, context->mmio_base + ISP_SCALE_FRAME_MASK);
#else
	if(scalar_parameters->dst_width < 640 && scalar_parameters->dst_height < 480)
		writel(0xFEFEFEFE, context->mmio_base + ISP_SCALE_FRAME_MASK);
	else
		writel(0xFEFEFEFE, context->mmio_base + ISP_SCALE_FRAME_MASK);
#endif

	writel(0x77777777, context->mmio_base + ISP_SCALE_FRAME_MASK);//25fps * 24/32 =  18.75fps

	val = 0x0<<29|0x0<<25|0x0<<24|0x8<<20|0x2<<16|0x0<<12|0x0<<8|0x0<<6|0x0<<4|0x0<<3;

	if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_Y_UV422)
		val |= (1 << 2) | (0 << 0);		// 0: yuv422, plane format
	else if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_Y_UV420)
		val |= (1 << 2) | (1 << 0);		// 1: yuv420, plane format
	else if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_YUV422)
		val |= (0 << 2) |(0 << 0);		// [1:0] 0:yuv422, plane format
	else if(scalar_parameters->src_format == XM_ISP_SCALAR_FORMAT_YUV420)
		val |= (0 << 2) | (1 << 0);		// 0: yuv422, plane format
	else
	{
		XM_printf("xm_isp_scalar_config, unsupported format(%d)\n", scalar_parameters->src_format);
		return XM_ISP_SCALAR_ERRCODE_ILLEGAL_PARA;
	}

	writel(scalar_parameters->mid_line & 0xfff, context->mmio_base + ISP_SCALE_RESERVED);

	// \u0152���\u017d\u017d�����Ŀ��\u017d���\u017d�С�Ƿ���ͬ, ��ͬ��\u0153�ֹFIR�˲\u0161��
	if( 	scalar_parameters->src_window_width == scalar_parameters->dst_window_width
		&&	scalar_parameters->src_window_height == scalar_parameters->dst_window_height )
		val |= (1 << 5);

	writel(val, context->mmio_base + ISP_SCALE_CONTROL);

	// �\u017d�\u0152��\u017d�С
	writel(((scalar_parameters->src_width & 0xFFF) << 0)
			| ((scalar_parameters->src_height & 0xFFF) << 12),
			context->mmio_base + ISP_SCALE_VIDEO_SOURCE_SIZE);

	// �\u017d\u017d��ڶ\u0161��
	writel(((scalar_parameters->src_window_x & 0xFFF) << 0)
			|	((scalar_parameters->src_window_y & 0xFFF) << 12),
			context->mmio_base + ISP_SCALE_VIDEO_WINDOW_POINT);
	writel(((scalar_parameters->src_window_width & 0xFFF) << 0)
			|	((scalar_parameters->src_window_height & 0xFFF) << 12),
			context->mmio_base + ISP_SCALE_VIDEO_WINDOW_SIZE);

	// Ŀ���\u0152�� (Ŀ��\u017d���)\u017d�С
	writel(((scalar_parameters->dst_window_width & 0xFFF) << 0)
			| ((scalar_parameters->dst_window_height & 0xFFF) << 12),
			context->mmio_base + ISP_SCALE_VIDEO_SIZE);

   // �\u017d\u017d��ڿ��\u017d���Ŀ��\u017d��ڿ��, ʹ���з\u0153���˲\u0161
	if( scalar_parameters->src_window_width > scalar_parameters->dst_window_width )
		do_horz_down_scalar = 1;
	if(do_horz_down_scalar)
		writel(0<<9| 1<<8 | 1<<7| 1<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0,
			context->mmio_base + ISP_SCALE_SCALE_CTL);
	else
		writel(0<<9| 0<<8 | 1<<7| 0<<6 | 1<<5 | 0<<4 | 0<<3 | 0<<2 | 0<<1 | 0,
			context->mmio_base + ISP_SCALE_SCALE_CTL);

	/*V_xmod_odd_init<<11 | V_xmod_even_init */
	writel(512<<11 | 0<<0, context->mmio_base + ISP_SCALE_SCALE_VXMOD);

	/*left cut number in line scaler<<18 | hfz */
	if(do_horz_down_scalar)
		// ��\u0152������\u017e\u017d��5\u017e����ص�
		writel(5<<18 | (( scalar_parameters->src_window_width * 1024 /
				(scalar_parameters->dst_window_width + 5)) << 0),
				context->mmio_base + ISP_SCALE_SCALE_CTL0);
	else
		writel(0<<18 | (( scalar_parameters->src_window_width * 1024 /
				scalar_parameters->dst_window_width) << 0),
				context->mmio_base + ISP_SCALE_SCALE_CTL0);

	/* Up cut line number in veritical scaler <<18 | vfz */
	writel(0<<18 | (( scalar_parameters->src_window_height * 1024 /
				scalar_parameters->dst_window_height) << 0),
				context->mmio_base + ISP_SCALE_SCALE_CTL1);

	/* right_cut_num<<8 | bottom_cut_num */
	writel(0<<8  | 0<<0, context->mmio_base + ISP_SCALE_RIGHT_BOTTOM_CUT_NUM);

	/* �\u017d��ram��\u0178ݵ���\u0178�ÿ�е��� */
	/*The number of the horizontal pix in write back ram, it is equal or bigger than the wide of scale, x16 */
	writel(scalar_parameters->dst_stride, context->mmio_base + ISP_SCALE_WB_DATA_HSIZE_RAM);

	// Source :YUV FORMAT Address set
   // bypass to isp_scale ����Ҫ�\u017d�\u017d��ַ
	// ISPscale_arkn141_set_source_addr(yuv ,inwidth, inheight );
   // rISP_SCALE_VIDEO_ADDR1 =  (int)yuv;

	if(scalar_parameters->mid_line)//rISP_SCALE_INT_CTL
		val = (1 << 0) | (1 << 2);
	else
		val = (1 << 0) | (1 << 1) ;

   // mask interrupt                                  /*Pix_abort mask*/
   // bit.0: frame finish interupt
   // bit.1: bresp error interrupt
   // bit.2: middle interrupt
   // bit.3: Y addr buffer push error
   // bit.4: Y addr buffer pop error
   // bit.5: UV addr buffer push error
   // bit.6: UV addr buffer pop error
   // bit.7: Finish  addr buffer push error
   // bit.8: Finish  addr buffer pop error
   // bit.9: Pix_abort mask
   /*������֡�쳣����ʹ��һ����\u017d��ַ�������ᱣ\u017d�\u0153finish��FIFO��*/
	if(scalar_parameters->mid_line)
		writel((1<<1) | (1<<2) | (0x3f<<3), context->mmio_base + ISP_SCALE_INT_CTL);	// ��ֵ�ж�
	else
		writel((1<<0) | (1<<1) | (0x3f<<3), context->mmio_base + ISP_SCALE_INT_CTL);	// ֡�ж�

   //offset :0x74

   //bit.0: ISP_scale_sel_601
   //bit.0: 0:isp    1:itu601

   //bit.1: Valid_polarity: the valid polarity of the hsync and vsync
   //bit.1: 0: high level valid   1: low level valid

   //bit.2: 0:uyvy  1:yuyv
   //rISP_SCALE_WB_CTL |= 0|(0<<3)|(1<<2);

	val = ((scalar_parameters->src_ycbcr_sequence & 0x01) << 2)		// 1:yuyv
		|	(0 << 3);	// 0:uv
	if(scalar_parameters->src_hsync_polarity == XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL)
		val |= (0 << 1);	// HSYNC Valid_polarity high level
	else
		val |= (1 << 1);	// HSYNC Valid_polarity low level
	if(scalar_parameters->src_vsync_polarity == XM_ISP_SCALAR_SRC_SYNC_PLOARITY_HIGH_LEVEL)
		val |= (0 << 4);	// VSYNC Valid_polarity high level
	else
		val |= (1 << 4);	// VSYNC Valid_polarity low level


	if(scalar_parameters->src_channel == XM_ISP_SCALAR_SRC_CHANNEL_ISP)
		val |= (0 << 0);
	else	// ISP_SCALAR_SRC_CHANNEL_ITU601
		val |= (1 << 0);

	writel(val, context->mmio_base + ISP_SCALE_WB_CTL);

	// ��\u017d����
	memcpy (&isp_scalar_parameters, scalar_parameters, sizeof(isp_scalar_parameters));

	return  XM_ISP_SCALAR_ERRCODE_OK ;
}

#define SYS_SOFT_RSTNB		0x78
void isp_scale_softreset(struct ark_isp_scale_context *context)
{
	u32 val = readl(context->sys_base + SYS_SOFT_RSTNB);
	val &= ~(1 << 12);
	writel(val, context->sys_base + SYS_SOFT_RSTNB);
	udelay(10);
	val |= (1 << 12);
	writel(val, context->sys_base + SYS_SOFT_RSTNB);
}

/* clear all pushed buffer */
static inline void isp_scale_buffer_clear(struct ark_isp_scale_context *context)
{
	/* 31	R/W	0	Write 1 to clean all the addr fifo */
	writel(readl(context->mmio_base + ISP_SCALE_WB_STATUS) | (1 << 31),
		context->mmio_base + ISP_SCALE_WB_STATUS);
}

int ark_isp_scale_dev_init(struct ark_isp_scale_context *context)
{
	writel(0, context->mmio_base + ISP_SCALE_EN);

	clk_disable_unprepare(context->clk);
	isp_scale_softreset(context);
	clk_prepare_enable(context->clk);

	writel(0, context->mmio_base + ISP_SCALE_WB_CTL);

	spin_lock_init(&context->lock);
	INIT_LIST_HEAD(&context->isbuf_push_list);

	return 0;
}

void isp_scale_buffer_init(struct ark_isp_scale_context *context)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&context->lock, flags);

	while(!list_empty(&context->isbuf_push_list)) {
		list_del(context->isbuf_push_list.next);
	}

	for(i = 0; i < context->isbuf_num; i++) {
		isp_scale_push_buffer(context, i);
		list_add_tail(&context->isbuf_id[i].list, &context->isbuf_push_list);
		context->isbuf_status[i] = ISBUF_STATUS_BUSY;
	}

	spin_unlock_irqrestore(&context->lock, flags);
}

static int get_push_frame_count(struct ark_isp_scale_context *context)
{
	int i;
	int count = 0;
	
 	for (i = 0; i < context->isbuf_num; i++) {
		if (context->isbuf_status[i] == ISBUF_STATUS_BUSY)
			count++;
	}
	return count;
}

irqreturn_t ark_isp_scale_intr_handler(int irq, void *dev_id)
{
	struct ark_isp_scale_device *isp_scale = (struct ark_isp_scale_device *)dev_id;
	struct ark_isp_scale_context *context = &isp_scale->context;
	unsigned int Y_finish, UV_finish;
	unsigned int status    = readl(context->mmio_base + ISP_SCALE_INT_STATUS);
	unsigned int wb_status = readl(context->mmio_base + ISP_SCALE_WB_STATUS);
	unsigned int clr_wb_status =0;
	int y_uv_pop_error = 0;
	int ready_id = 0;
	struct ispbuf_id *isbufid, *next;
	int i;

	//printk("%s status=0x%x.\n", __FUNCTION__, status);

	if( status & FRAME_FINISH )
	{
		//ISP_SCALAR_STOP;
		/* clear frame int */
		writel(CLEAR_FRAME_FINISH, context->mmio_base + ISP_SCALE_CLCD_INT_CLR);
		if(isp_scalar_parameters.mid_line == 0)
		{
			u32_t finish_count = (wb_status >> 12) & 0x7;

			// ÿ\u017d������Ϊ2, �����\u0152���й�?
			//XM_printf ("finish_count=%d\n", finish_count);
	  		if( finish_count >= 2 )
			{
				// ���� FIFO ��֡�ж� ���\u017dû�� ���һ\u017e�����֡������
				Y_finish  = readl(context->mmio_base + ISP_SCALE_WB_FINISH_YADDR);
				UV_finish = readl(context->mmio_base + ISP_SCALE_WB_FINISH_UADDR);

				for (i = 0; i < context->isbuf_num; i++) {
					if (Y_finish == context->isbuf[i].yaddr && UV_finish == context->isbuf[i].uvaddr)
						break;
				}

				if (i < context->isbuf_num) {
					if (context->isbuf_status[i] != ISBUF_STATUS_BUSY) {
						printk(KERN_ALERT "pop no-pushed isbuf %d.\n", i);
					} else {
						ready_id = i;
						context->isbuf_status[i] = ISBUF_STATUS_READY;
						if (!list_empty(&context->isbuf_push_list)) {
							list_for_each_entry_safe(isbufid, next, &context->isbuf_push_list, list) {
								if (i != isbufid->id) {
									printk("isbuf %d lost, repush.\n", isbufid->id);
									isp_scale_push_buffer(context, isbufid->id);
									list_del(&isbufid->list);
									list_add_tail(&isbufid->list, &context->isbuf_push_list);		
									context->isbuf_status[isbufid->id] = ISBUF_STATUS_BUSY;		
								} else {
									list_del(&isbufid->list);
									break;
								}
							}
						}
					}
				}

				for (i = 0; i < context->isbuf_num; i++) {
					if (context->isbuf_status[i] == ISBUF_STATUS_FREE)  {
						isp_scale_push_buffer(context, i);
						list_add_tail(&context->isbuf_id[i].list, &context->isbuf_push_list);
						context->isbuf_status[i] = ISBUF_STATUS_BUSY;
					}
				}

				if (get_push_frame_count(context) < 2) {
					printk(KERN_ALERT "no free isbuf, reuse isbuf %d.%d,%d,%d,%d.\n", ready_id,
						context->isbuf_status[0], context->isbuf_status[1],
						context->isbuf_status[2], context->isbuf_status[3]);
					if (!list_empty(&context->isbuf_push_list)) {
						list_for_each_entry_safe(isbufid, next, &context->isbuf_push_list, list) {
							if (isbufid->id == ready_id) {
								list_del(&isbufid->list);
								break;
							}
						}
					}
					isp_scale_push_buffer(context, ready_id);
					list_add_tail(&context->isbuf_id[ready_id].list, &context->isbuf_push_list);
					context->isbuf_status[ready_id] = ISBUF_STATUS_BUSY;
				} else {
					//poll
					wake_up_interruptible(&isp_scale->frame_finish_waitq);

					if (isp_scale->async_queue_wb != NULL) {
						kill_fasync(&isp_scale->async_queue_wb, SIGIO, POLL_IN);
					}
				}
			}
		}
	}
	if( status & write_back_bresp_error_interrupt)
	{
		writel(write_back_bresp_error_interrupt_clear, context->mmio_base + ISP_SCALE_CLCD_INT_CLR);
		//XM_printf ("isp scalar write_back_bresp_error\n");
	}
	if( status & write_back_middle_finish_interupt)
	{
		writel(write_back_middle_finish_interupt_clear, context->mmio_base + ISP_SCALE_CLCD_INT_CLR);
		if(isp_scalar_parameters.mid_line_user_callback)
		{
			(*isp_scalar_parameters.mid_line_user_callback)(isp_scalar_parameters.mid_line_user_data);
		}
		else
		{
			// ȱʡ\u017d\u0160��
			unsigned int current_used_y_addess = readl(context->mmio_base + ISP_SCALE_WB_FINISH_YADDR);
			unsigned int current_used_uv_addess = readl(context->mmio_base + ISP_SCALE_WB_FINISH_UADDR);
			(void)(current_used_uv_addess);
			if(current_used_y_addess) {
				//isp_scalar_frame_notify (current_used_y_addess);
			}
		}
	}

	// Y/UV Pop Error����ʱ���\u0160\u0152��sensor������������.
	// �\u0160�ʵ���\u0152������������\u017dʹY/UV Pop Error���ٳ���
	if( wb_status & (1 << 1) )
	{
		// bit.1 Y_dest_addr_fifo pop error
		clr_wb_status |= (1 << 1);
		y_uv_pop_error = 1;
		printk (KERN_ALERT "Y pop error\n");
	}
	if( wb_status & (1 << 6) )
	{
		// bit.6 UV_dest_addr_fifo pop error
		clr_wb_status |= (1 << 6);
		y_uv_pop_error = 1;
		printk (KERN_ALERT "UV pop error\n");
	}

	if(y_uv_pop_error)
	{
	    for (i = 0; i < context->isbuf_num; i++) {
            if (context->isbuf_status[i] == ISBUF_STATUS_FREE)  {
				isp_scale_push_buffer(context, i);
				list_add_tail(&context->isbuf_id[i].list, &context->isbuf_push_list);
				context->isbuf_status[i] = ISBUF_STATUS_BUSY;
				break;
			}
    	}

		if (i == context->isbuf_num) {
			if (!list_empty(&context->isbuf_push_list)) {
				list_for_each_entry_safe(isbufid, next, &context->isbuf_push_list, list) {
					if (isbufid->id == 0) {
						list_del(&isbufid->list);
						break;
					}
				}
			}
			isp_scale_push_buffer(context, 0);
			list_add_tail(&context->isbuf_id[0].list, &context->isbuf_push_list);
			context->isbuf_status[0] = ISBUF_STATUS_BUSY;
		}
	}

	/*
	if(wb_status & ((1<<0)|(1<<5)) )
	{
	  // bit.3 = push
	  clr_wb_status |=  wb_status & ((1<<0)|(1<<5))  ;
	  XM_printf("push error!\n");
	}
	*/

	if(wb_status & (1 << 0))
	{
	  // bit.0 = Y_dest_addr_fifo push error
	  clr_wb_status |=  (1 << 0);
	  //XM_printf("Y push error!\n");
	}
	if(wb_status & (1 << 5))
	{
	  // bit.5 = UV_dest_addr_fifo push error
	  clr_wb_status |=  (1 << 5);
	  //XM_printf("UV push error!\n");
	}

	// ����һ\u017e���������֡ ������һ\u017e���ַ���\u017d������finish �� ѹջ
	if(wb_status & (1 << 16))
	{
		// bit.16 Pix_abort
	  clr_wb_status |= wb_status & (1 << 16) ;
	  //XM_printf("pix abort!\n");
	}

	if(wb_status & (1 << 11))
	{
	  // bit.11 finish pop error
	  clr_wb_status |= wb_status & (1 << 11) ;
	  //XM_printf("finish pop error!!!\n");
	}
	if(wb_status & (1 << 10))
	{
	  // bit.10 finish push error
	  clr_wb_status |= wb_status & (1 << 10) ;
	  //XM_printf("finish push error!!!\n");
	}

	writel(clr_wb_status, context->mmio_base + ISP_SCALE_WB_STATUS);

	//rISP_SCALE_INT_STATUS = status;
    return IRQ_HANDLED;
}
