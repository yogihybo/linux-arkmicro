#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/errno.h>

#include "arkn141_itu656.h"

#define itu656_readl(context, reg)				    __raw_readl((context)->itu656_base+(reg))
#define itu656_writel(context, reg, val)			__raw_writel((val), (context)->itu656_base+(reg))
#define itu656_readl_sys(context, reg)			    __raw_readl((context)->sys_base+(reg))
#define itu656_writel_sys(context, reg, val)		__raw_writel((val), (context)->sys_base+(reg))
#define itu656_readl_dein(context, reg)			    __raw_readl((context)->deinterlace_base+(reg))
#define itu656_writel_dein(context, reg, val)	    __raw_writel((val), (context)->deinterlace_base+(reg))


inline void ark_itu656_set_global_enable(struct ark_itu656in_context *context,int enable)
{
    // ENABLE_REG 0x930
    if (enable)
		itu656_writel(context,ITU656IN_ENABLE_REG,itu656_readl(context,ITU656IN_ENABLE_REG) | (1<<0));// global enable
    else
        itu656_writel(context,ITU656IN_ENABLE_REG,itu656_readl(context,ITU656IN_ENABLE_REG) & ~(1<<0));// global disable
}

void ark_itu656_stop(struct ark_itu656in_context *context)
{
    ark_itu656_set_global_enable(context,0);
	itu656_writel(context,ITU656IN_IMR,0);// disable all interrupt outputs
}
EXPORT_SYMBOL(ark_itu656_stop);

static void ark_itu656_pad_select(struct ark_itu656in_context *context)
{
    unsigned int val;

    if (context->itu_channel == ITU656_CH0) {
		val = itu656_readl_sys(context,SYS_PAD_CTRL00);
		val &= 0xc01ffe00;
		//	   ituclk| vsync | hsync | din4  | din5  | din6
		val |=(1<<0)|(1<<3)|(1<<6)|(1<<21)|(1<<24)|(1<<27);
		itu656_writel_sys(context,SYS_PAD_CTRL00,val);

		val = itu656_readl_sys(context,SYS_PAD_CTRL01);
		val &= (~0x7fff);
		//	   din7 | din8 | din9 | din10 | din11
		val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12);
		itu656_writel_sys(context,SYS_PAD_CTRL01,val);
    } else  if(context->itu_channel == ITU656_CH1){
        val = itu656_readl_sys(context,SYS_PAD_CTRL01);
        val &=0xc0007fff;
        //     ituclk| vsync | hsync | din0  | din1
        val |=(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27);
		itu656_writel_sys(context,SYS_PAD_CTRL01,val);

        val = itu656_readl_sys(context,SYS_PAD_CTRL02);
        val &= 0xfffc0000;
        //     din2 | din3 | din4 | din5 | din6  | din7
        val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15);
        itu656_writel_sys(context,SYS_PAD_CTRL02,val);
    } else if(context->itu_channel == ITU656_CH0_CH1){	//select ch0 and ch1
		val = itu656_readl_sys(context,SYS_PAD_CTRL00);
		val &= 0xc01ffe00;
		//	   ituclk| vsync | hsync | din4  | din5  | din6
		val |=(1<<0)|(1<<3)|(1<<6)|(1<<21)|(1<<24)|(1<<27);
		itu656_writel_sys(context,SYS_PAD_CTRL00,val);

		val = itu656_readl_sys(context,SYS_PAD_CTRL01);
		val &= 0xc0000000;
		//	   din7 | din8 | din9 | din10 | din11
		val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12);
		//	   ituclk| vsync | hsync | din0  | din1
		val |=(1<<15)|(1<<18)|(1<<21)|(1<<24)|(1<<27);
		itu656_writel_sys(context,SYS_PAD_CTRL01,val);

		val = itu656_readl_sys(context,SYS_PAD_CTRL02);
		val &= 0xfffc0000;
		//	   din2 | din3 | din4 | din5 | din6  | din7
		val |=(1<<0)|(1<<3)|(1<<6)|(1<<9)|(1<<12)|(1<<15);
		itu656_writel_sys(context,SYS_PAD_CTRL02,val);
	}
}

static void ark_itu656_reg_init(struct ark_itu656in_context *context)
{
    unsigned int Itu_clk_b_dly = 0;
    unsigned int val;

    itu656_INFO("in--->");
	
	itu656_writel(context,ITU656IN_ENABLE_REG,itu656_readl(context,ITU656IN_ENABLE_REG) | (3<<1));

    // Clock Off    
	itu656_writel_sys(context,SYS_PER_CLK_EN,itu656_readl_sys(context,SYS_PER_CLK_EN) & ~(1<<24));
	itu656_writel_sys(context,SYS_AHB_CLK_EN,itu656_readl_sys(context,SYS_AHB_CLK_EN) & ~(1<<18));

    //soft reset
	itu656_writel_sys(context,SYS_SOFT_RSTNA,itu656_readl_sys(context,SYS_SOFT_RSTNA) & ~(1<<19));
    msleep(1);
	itu656_writel_sys(context,SYS_SOFT_RSTNA,itu656_readl_sys(context,SYS_SOFT_RSTNA) | (1<<19));

	itu656_writel_sys(context,SYS_PER_CLK_EN,itu656_readl_sys(context,SYS_PER_CLK_EN) | (1<<24));
	itu656_writel_sys(context,SYS_AHB_CLK_EN,itu656_readl_sys(context,SYS_AHB_CLK_EN) | (1<<18));

    // SYS_DEVICE_CLK_CFG0
    // 9	R/W	0	Itu_clk_b_sel
    // 				1:itu_clk_b_inv
    //				0:itu_clk_b
    //rSYS_DEVICE_CLK_CFG0 |= (0x1 << 9);
    itu656_writel_sys(context,SYS_DEVICE_CLK_CFG0,itu656_readl_sys(context,SYS_DEVICE_CLK_CFG0) & ~(0x1<<9));

	val = itu656_readl_sys(context,SYS_DEVICE_CLK_CFG3);
    val &= ~(0x7F << 23);
    val |= (Itu_clk_b_dly << 23);
	itu656_writel_sys(context,SYS_DEVICE_CLK_CFG3,val);

    val = (0 << 5 | 0x0 << 1 | 0x0);// | (1 << 4);
    itu656_writel(context,ITU656IN_MODULE_EN,val);

    // 0 : sel itu601 input
    // 1 : select itu656 input
    if (!context->itu656in.itu601in)
        itu656_writel(context,ITU656IN_INPUT_SEL,0x01);	// itu656 in
    else {
		itu656_writel(context,ITU656IN_MODULE_EN,itu656_readl(context,ITU656IN_MODULE_EN) | 1);
        itu656_writel(context,ITU656IN_INPUT_SEL,0x0);	// itu601 in
    }

     // bit.0 odd int    bit.3  even int
    if (context->itu656in.interlace)
        val = (1<<10) | (0xF<<4) | (1<<3) | (1<<2);
    else
        val = (1<<10) | (0xF<<4) | (1<<3) | (1<<2) | (1<<0);
	itu656_writel(context,ITU656IN_IMR,val);

	itu656_writel(context,ITU656IN_ICR,0x7ff);
	itu656_writel(context,ITU656IN_SLICE_PIXEL_NUM,0);

    // open window setting
    val = ((context->itu656in.left_cut & 0x7ff) << 16) | (context->itu656in.right_cut & 0x7ff);
	itu656_writel(context,ITU656IN_H_CUT,val);
    val = ((context->itu656in.up_cut & 0x7ff) << 16) | (context->itu656in.down_cut & 0x7ff );
	itu656_writel(context,ITU656IN_V_CUT,val);

    if (!context->itu656in.itu601in)
         // bit.5: 0.field mode or 1.frame mode
        // field mode
        val = (0<<13)|(0<<12)|  (1<<11)| (1<<6) |(1<<5) |(1<<4) | (1<<1) | 0;
    else
        // 601 frame mode
        //	field mode
        val = (0<<13)|(1<<12)|  (1<<11) |(0<<5) |(1<<4) | (1<<1) | 0;
	itu656_writel(context,ITU656IN_ENABLE_REG,val);

    val = (5 << 16 | 10 << 8 | 0xFF);
	itu656_writel(context,ITU656IN_DELTA_NUM,val);

	itu656_writel(context,ITU656IN_SIZE,context->itu656in.width << 16);
	itu656_writel(context,ITU656IN_DATA_OUT_LINE_NUM_PER_FIELD,context->itu656in.height);

	val = (context->itu656in.width - context->itu656in.left_cut - context->itu656in.right_cut)
	    * (context->itu656in.height - context->itu656in.up_cut - context->itu656in.down_cut);
	itu656_writel(context,ITU656IN_TOTAL_PIX,val);

	val = itu656_readl(context,ITU656IN_INPUT_CTL);
    if (context->itu656in.interlace) {
        if(context->itu656in.system == CVBS_PAL) {
            val |= (1<<14)|(1<<15);//PAL
        } else if(context->itu656in.system == CVBS_NTSC) {
            val |= (1<<15);//NTSL
        }
    } else {
        // 0	R/W	1'b2	0 : Cb Y Cr Y
        //					1 : Y Cb Y Cr
        //HS normal,VS INV,Y Cb Y Cr
        val |= ((1<<15)|(0<<13)|(0<<2));
    }
	itu656_writel(context,ITU656IN_INPUT_CTL,val);

    val = context->framebuf_phyaddr[0].yaddr;
	itu656_writel(context,ITU656IN_DRAM_Y_ADDR,val);
    val = context->framebuf_phyaddr[0].yaddr + 
		context->itu656in.width * context->itu656in.height;
	itu656_writel(context,ITU656IN_DRAM_Y_ADDR,val);

    itu656_INFO("out<---");
}

static void ark_itu656_reg_uninit(struct ark_itu656in_context *context)
{
    // Clock Off
	itu656_writel_sys(context,SYS_PER_CLK_EN,itu656_readl_sys(context,SYS_PER_CLK_EN) & ~(1<<24));
	itu656_writel_sys(context,SYS_AHB_CLK_EN,itu656_readl_sys(context,SYS_AHB_CLK_EN) & ~(1<<18));
}

static void ark_itu656_enable(struct ark_itu656in_context *context)
{
	itu656_writel(context,ITU656IN_ENABLE_REG,itu656_readl(context,ITU656IN_ENABLE_REG) | 1);
}

static void ark_itu656_disable(struct ark_itu656in_context *context)
{	
	itu656_writel(context,ITU656IN_ENABLE_REG,0);
}

static void deinterlace_reset(struct ark_itu656in_context *context)
{
    itu656_writel_dein(context,DEINTERLACE_CTRL0,1);
    ndelay(100);
    itu656_writel_dein(context,DEINTERLACE_CTRL0,0);

    itu656_writel_sys(context,SYS_AHB_CLK_EN,itu656_readl_sys(context, SYS_AHB_CLK_EN) & ~(1<<17));
    itu656_writel_sys(context,SYS_SOFT_RSTNA,itu656_readl_sys(context, SYS_SOFT_RSTNA) & ~(1<<22));
    msleep(1);
    itu656_writel_sys(context,SYS_SOFT_RSTNA,itu656_readl_sys(context, SYS_SOFT_RSTNA) | (1<<22));
    itu656_writel_sys(context,SYS_AHB_CLK_EN,itu656_readl_sys(context, SYS_AHB_CLK_EN) | (1<<17));
}

static inline void ark_itu656_enable_write(struct ark_itu656in_context *context)
{
	itu656_writel(context,ITU656IN_MODULE_EN,itu656_readl(context, ITU656IN_MODULE_EN) & ~(1<<2));
}

static inline void ark_itu656_disable_write(struct ark_itu656in_context *context)
{
	itu656_writel(context,ITU656IN_MODULE_EN,itu656_readl(context, ITU656IN_MODULE_EN) | (1<<2));
}

static void deinterlace_init(struct ark_itu656in_context *context)
{
    deinterlace_reset(context);
    itu656_writel_dein(context,DEINTERLACE_INT_CLEAR,0x3);
    itu656_writel_dein(context,DEINTERLACE_INT_MASK,0x3);
}

static int deinterlace_process (unsigned int deinterlace_size,  unsigned int data_mode, unsigned int deinterlace_type,
    unsigned int deinterlace_field, unsigned int src_field_addr_0, unsigned int src_field_addr_1,unsigned int src_field_addr_2,
    unsigned int dst_y_addr, unsigned int dst_u_addr, unsigned int dst_v_addr,struct ark_itu656in_context *context)
{
    unsigned int pixel_per_line;
    unsigned int total_line;
    unsigned int pn;
    unsigned int denoise_bypass;
    unsigned int stride;
    unsigned int only_wr_1_field;
    unsigned int field;
    int ret = DEINTERLACE_SUCCESS;
	unsigned int val;

    if(deinterlace_field != DEINTERLACE_FIELD_ODD
        &&	deinterlace_field != DEINTERLACE_FIELD_EVEN){
        itu656_ERROR("invalid deinterlace field (%d)", deinterlace_field);
        return DEINTERLACE_PARA_ERROR;
    }
    field = deinterlace_field;
    //itu656_INFO("field: %x, data_mode: %d", field, data_mode);
    if(deinterlace_size == DEINTERLACE_LINE_SIZE_960H){
        pixel_per_line = 120 * (1 + data_mode);
    }else if(deinterlace_size == DEINTERLACE_LINE_SIZE_720H){
        pixel_per_line = 90 * (1 + data_mode);
    }else{
        itu656_ERROR("invalid deinterlace size (%d)", deinterlace_size);
        return DEINTERLACE_PARA_ERROR;
    }

    if(data_mode == DEINTERLACE_DATA_MODE_420){
        denoise_bypass = 1;
        stride = pixel_per_line;
        only_wr_1_field = 1;
    }else if(data_mode == DEINTERLACE_DATA_MODE_422){
        if(dst_y_addr == 0){
            itu656_ERROR("illegal deinterlace dst Y address");
            return DEINTERLACE_PARA_ERROR;
        }
        denoise_bypass = 1;
        stride = 0;
        only_wr_1_field = 0;
    }else{
        itu656_ERROR("invalid deinterlace data mode (%d)", data_mode);
        return DEINTERLACE_PARA_ERROR;
    }

    if(deinterlace_type == DEINTERLACE_TYPE_PAL){
        pn = 0;
        total_line = 288;
    }else if(deinterlace_type == DEINTERLACE_TYPE_NTSC){
        pn = 1;
        total_line = 240;
    }else{
        itu656_ERROR("invalid deinterlace type (%d)", deinterlace_type);
        return DEINTERLACE_PARA_ERROR;
    }

    val	=  (0x0 << 29)
        |  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field
        |  (pixel_per_line << 20)       // pixel_pl
        |  (total_line << 11)           // total_line
        |  (stride << 3)                // stride
        |  (data_mode << 2)             // data_mode
        |  (field << 1)                 // field
		;
	itu656_writel_dein(context,DEINTERLACE_CTRL0,val);
	itu656_writel_dein(context,DEINTERLACE_CTRL1,0x0000700d);
	itu656_writel_dein(context,DEINTERLACE_CTRL2,0x30004230);

    // denoise bypass  pn: 1:n display_motion line_intra global_cnt display_mv_0
    val = (denoise_bypass << 15)
        | (0x2 << 16)
        | (pn << 13)
        | (0x1 << 11) //
        | (0x1 << 9) //
        | (0x0 << 8)
        | (0x0 << 7)
        | (0x0 << 6)
        | (0x0 << 5)
        | (0x1 << 3)
        | (0x1 << 1)
        ;
	itu656_writel_dein(context,DEINTERLACE_CTRL3,val);

	itu656_writel_dein(context,DEINTERLACE_FILM_MODECTRL,(unsigned int)(0x0 << 31));
	itu656_writel_dein(context,DEINTERLACE_SADDR0,(unsigned int)src_field_addr_0);
	itu656_writel_dein(context,DEINTERLACE_SADDR1,(unsigned int)src_field_addr_1);
	itu656_writel_dein(context,DEINTERLACE_SADDR2,(unsigned int)src_field_addr_2);

    if(data_mode == DEINTERLACE_DATA_MODE_422)
    {
        itu656_writel_dein(context,DEINTERLACE_DADDRY,(unsigned int)dst_y_addr);	// dy_0  when filed = 1 ,data_mode = 0  then dy_0 = s0_0
                                             //      when filed = 0 ,data_mode = 0  then dy_0 = s2_0
        itu656_writel_dein(context,DEINTERLACE_DADDRU,(unsigned int)dst_u_addr);
		itu656_writel_dein(context,DEINTERLACE_DADDRV,(unsigned int)dst_v_addr);						 
    }

    //	pingpong addr fetch
	itu656_writel_dein(context,DEINTERLACE_ADDR_SWITCHMODE,0x00);

    // 
    //	1		W		0			AXI_wr error interrupt clr :
    //								1:clear the interrupt
    //	0		W		0			Field finish interrupt clr:
    //								1: clear the interrupt
	itu656_writel_dein(context,DEINTERLACE_INT_CLEAR,0x03);

    // start de-interlace
	itu656_writel_dein(context,DEINTERLACE_START,0x01);

    return ret;
}

void dvr_start(struct ark_itu656in_context *context)
{
    if(!context->work_status){
        context->work_status = 1;
        context->discard_frame = START_DISCARD_FRAME;
        context->prev_frame = -1;
        context->cur_frame = -1;
        if (context->itu656in.interlace)
            deinterlace_init(context);
        ark_itu656_pad_select(context);
        ark_itu656_reg_init(context);
        ark_itu656_enable(context);
        itu656_INFO("camera start end");
    }
}

void dvr_stop(struct ark_itu656in_context *context)
{
    ark_itu656_disable(context);
    msleep(100);
    ark_itu656_reg_uninit(context);
    context->work_status = 0;

    itu656_INFO("camera stop");
}

int dvr_get_pragressive(void)
{
	return 1;
}
EXPORT_SYMBOL(dvr_get_pragressive);

void dvr_restart(void)
{

}
EXPORT_SYMBOL(dvr_restart);

#if 0
static int get_push_frame_count(struct ark_itu656in_context *context)
{
	int i;
	int count = 0;
	
 	for (i = 0; i < context->framebuf_num; i++) {
		if (context->framebuf_status[i] == FRAMEBUF_STATUS_BUSY)
			count++;
	}
	return count;
}
#endif

static void dvr_push_frame_buffer(struct ark_itu656in_context *context, int frame_id)
{
	unsigned int val;

    //printk("dvr_push_frame_buffer %d.\n", frame_id);
    if (frame_id >= 0 && frame_id < context->framebuf_num) {
		itu656_writel(context,ITU656IN_DRAM_Y_ADDR,context->framebuf_phyaddr[frame_id].yaddr);
        if (context->itu656in.interlace)
            val = context->framebuf_phyaddr[frame_id].yaddr + context->itu656in.width * context->itu656in.height * 2;
        else
            val = context->framebuf_phyaddr[frame_id].yaddr + context->itu656in.width * context->itu656in.height;
		itu656_writel(context,ITU656IN_DRAM_Y_ADDR,val);
    }
}

#if 0
static int set_free_buffer_busy(struct ark_itu656in_context *context)
{
    int i;
    int count = 0;
    
    for (i = 0; i < context->framebuf_num; i++) {
        if (context->framebuf_status[i] == FRAMEBUF_STATUS_FREE)  {
            dvr_push_frame_buffer(context, i);
            list_add_tail(&context->framebuf_id[i].list, &context->framebuf_push_list);
            context->framebuf_status[i] = FRAMEBUF_STATUS_BUSY;
            count++;
        }
    }

    return count;
}

static void set_ready_buffer(struct ark_itu656in_context *context, unsigned int frame_id)
{
	struct itu656_framebuf_id *bufid, *next;

    context->framebuf_status[frame_id] = FRAMEBUF_STATUS_READY;
    if (!list_empty(&context->framebuf_push_list)) {
        list_for_each_entry_safe(bufid, next, &context->framebuf_push_list, list) {
            if (frame_id != bufid->id) {
                printk("isbuf %d lost, repush.\n", bufid->id);
                dvr_push_frame_buffer(context, bufid->id);
                list_del(&bufid->list);
                list_add_tail(&bufid->list, &context->framebuf_push_list);      
                context->framebuf_status[bufid->id] = FRAMEBUF_STATUS_BUSY;     
            } else {
                list_del(&bufid->list);
                break;
            }
        }
    }
}

static void reuse_ready_buffer(struct ark_itu656in_context *context, unsigned int frame_id)
{
	struct itu656_framebuf_id *bufid, *next;
    
    printk(KERN_ALERT "no free framebuf, reuse isbuf %d.%d,%d,%d,%d.\n", frame_id,
        context->framebuf_status[0], context->framebuf_status[1],
        context->framebuf_status[2], context->framebuf_status[3]);
    if (!list_empty(&context->framebuf_push_list)) {
        list_for_each_entry_safe(bufid, next, &context->framebuf_push_list, list) {
            if (bufid->id == frame_id) {
                list_del(&bufid->list);
                break;
            }
        }
    }
    dvr_push_frame_buffer(context, frame_id);
    list_add_tail(&context->framebuf_id[frame_id].list, &context->framebuf_push_list);
    context->framebuf_status[frame_id] = FRAMEBUF_STATUS_BUSY; 
}
#endif


#define GetPingPongNextBuf(index,bufcnt) {index++;if(index == bufcnt) index = 0;}
#define GetPingPongPreBuf(index,bufcnt) {index--; if(index <0) index = bufcnt - 1;}

irqreturn_t ark_deinterlace_int_handler(int irq, void *dev_id)
{
    u32 raw_int;
    unsigned long flags;
    struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	struct ark_itu656in_context *context = &dvr_dev->context;

    raw_int = itu656_readl_dein(context,DEINTERLACE_RAW_INT);

    spin_lock_irqsave(&context->spin_lock, flags);

    if(raw_int & (1 << 0 )){
		itu656_writel_dein(context,DEINTERLACE_INT_CLEAR,0x1);
    }else if(raw_int & (1 << 1)){
        itu656_writel_dein(context,DEINTERLACE_INT_CLEAR,0x2);//error
        deinterlace_reset(context);
        printk("deinterlace axi error\n");
    }

    //poll
    itu656_INFO("frame_id=%d, count=%d.", context->prev_frame, context->frame_finish_count);
    if (context->frame_finish_count >= context->framebuf_num)
        context->frame_finish_count = context->framebuf_num - 1;
    context->frame_finish[context->frame_finish_count++] = context->prev_frame; //set flag to wakeup frame_finish_waitq

    wake_up_interruptible(&dvr_dev->frame_finish_waitq);

    //async
    if(dvr_dev->fasync_queue != NULL) {
        //printk(KERN_ALERT "kill_fasync camera frame finish.\n");
        kill_fasync(&dvr_dev->fasync_queue, SIGIO, POLL_IN);
    }

    context->deinter_status = 0;
    spin_unlock_irqrestore(&context->spin_lock, flags);

    return IRQ_HANDLED;
}

static int arkn141_frame_test_and_push(struct ark_itu656in_context *context, int frame_id)
{
	int i;
	int tmp;

	tmp = frame_id;
	for(i=0; i<context->framebuf_num; i++){
		tmp ++;
		//tmp %= 4;
		tmp %= context->framebuf_num;
		if (context->framebuf_status[tmp] == FRAMEBUF_STATUS_FREE){
			dvr_push_frame_buffer(context, tmp);
			//printk("###1 find free framebuf:%d.\n", tmp);
			break;
		}
	}

	if (i == context->framebuf_num) {
		tmp = frame_id;
		for(i=0; i<context->framebuf_num; i++){
			tmp ++;
			//tmp %= 4;
			tmp %= context->framebuf_num;
			if (context->framebuf_status[tmp] == FRAMEBUF_STATUS_READY){
				context->framebuf_status[tmp] = FRAMEBUF_STATUS_FREE;
				dvr_push_frame_buffer(context, tmp);
				printk(KERN_ALERT "### reuse ready framebuf:%d.\n", tmp);
				break;
			}
		}
		if (i == context->framebuf_num) {
			printk(KERN_ALERT "### err: no free and ready framebuf, reuse default frame_id:%d framebuf.\n", frame_id);
			context->framebuf_status[frame_id] = FRAMEBUF_STATUS_FREE;
			dvr_push_frame_buffer(context, frame_id);
		}
	}

	return 0;
}

irqreturn_t ark_itu656_int_handler(int irq, void *dev_id)
{
    struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	struct ark_itu656in_context *context = &dvr_dev->context;
    u32 intr_stat;
    unsigned long flags;
    int deinter_type;
    int timeout = 10000;
    //int ready_id = 0;
    u32 syschange_mask = 1 << 6; //total line change
    //int i;
	static int frame_id = 0;
	int push_frame_state = 0;
	//struct itu656_framebuf_id *bufid, *next;

    intr_stat = itu656_readl(context,ITU656IN_ISR);
	itu656_writel(context,ITU656IN_ICR,intr_stat);
    spin_lock_irqsave(&context->spin_lock, flags);

//    itu656_INFO("intr_stat=0x%x.", intr_stat);

    if (unlikely((intr_stat & (1 << 10)))){
        itu656_ERROR("pop err.");

		//if (set_free_buffer_busy(context) == 0)
			//reuse_ready_buffer(context, 0);
#if 0	//test
		for(i = 0; i <context->framebuf_num;i++){
			if (context->framebuf_status[i] == FRAMEBUF_STATUS_FREE){
				dvr_push_frame_buffer(context,i);
				break;
			}
		}

		if (i == context->framebuf_num) {
			printk(KERN_ALERT "no free framebuf, reuse framebuf[0](%d,%d,%d,%d).\n",
				context->framebuf_status[0],context->framebuf_status[1],context->framebuf_status[2],context->framebuf_status[3]);
			dvr_push_frame_buffer(context,0);
		}
#else
		arkn141_frame_test_and_push(context, frame_id);
		push_frame_state = 1;
#endif
    }

    if (unlikely(!context->itu656in.interlace))
        syschange_mask |= 1 << 5;    //active pixel change

    if (unlikely(intr_stat & syschange_mask)){
    	int total_width,total_height,width,height;
		
        total_width  = (itu656_readl(context,ITU656IN_PIX_NUM_PER_LINE) >> 16) & 0xFFF;
        total_height = (itu656_readl(context,ITU656IN_LINE_NUM_PER_FIELD) >> 16) & 0xFFF;
        width        =  itu656_readl(context,ITU656IN_PIX_NUM_PER_LINE) & 0xFFF;
        height       =  itu656_readl(context,ITU656IN_LINE_NUM_PER_FIELD) & 0xFFF;
        itu656_INFO("system change, %dx%d-%dx%d, intr_stat=0x%x.",
                    total_width, total_height, width, height, intr_stat);
        ark_itu656_disable_write(context);
        mod_timer(&dvr_dev->timer, jiffies +  msecs_to_jiffies(100));
        goto irq_end;
    }

    if (intr_stat & (FRAME_INTERRUPT_INTERRUPT | FIELD_INTERRUPT)){
        //itu656_INFO("interlace: %d", context->itu656in.interlace);
        //int tmp_id;
        unsigned int yaddr = itu656_readl(context,ITU656IN_DRAM_Y_ADDR);
        for (frame_id = 0; frame_id < context->framebuf_num; frame_id++) {
            if (yaddr == context->framebuf_phyaddr[frame_id].yaddr)
                break;
        }

		if(frame_id >= context->framebuf_num){
			goto irq_end;
		}

#if 0	//test
        if (context->discard_frame > 0) {
            context->discard_frame--;
            goto irq_end;
        }
		context->framebuf_status[frame_id] = FRAMEBUF_STATUS_BUSY;
#if 0
		for(i=0; i<context->framebuf_num; i++){
			if (context->framebuf_status[i] == FRAMEBUF_STATUS_FREE){
				dvr_push_frame_buffer(context,i);
				break;
			}
		}
#else
	tmp_id = frame_id;

	for(i=0; i<context->framebuf_num; i++){
		tmp_id ++;
		tmp_id %= 4;
		if (context->framebuf_status[tmp_id] == FRAMEBUF_STATUS_FREE){
			dvr_push_frame_buffer(context, tmp_id);
			break;
		}
	}
#endif

		if (i == context->framebuf_num) {
			printk(KERN_ALERT "no free framebuf, reuse framebuf(%d,%d,%d,%d).\n",
				context->framebuf_status[0],context->framebuf_status[1],context->framebuf_status[2],context->framebuf_status[3]);
			dvr_push_frame_buffer(context,frame_id);
		}
#else
		if(context->framebuf_status[frame_id] != FRAMEBUF_STATUS_FREE) {
			printk(KERN_ALERT "### frame_id:%d != free, status:%d, all(%d,%d,%d,%d)\n", frame_id, context->framebuf_status[frame_id], context->framebuf_status[0],context->framebuf_status[1],context->framebuf_status[2],context->framebuf_status[3]);
			if(!push_frame_state)
				arkn141_frame_test_and_push(context, frame_id);
			goto irq_end;
		}

        if (context->discard_frame > 0) {
            context->discard_frame--;
            goto irq_end;
        }
		context->framebuf_status[frame_id] = FRAMEBUF_STATUS_READY;
		if(!push_frame_state)
			arkn141_frame_test_and_push(context, frame_id);
#endif
		/*
		if (i < context->framebuf_num) {
			if (context->framebuf_status[i] != FRAMEBUF_STATUS_BUSY) {
				printk(KERN_ALERT "pop no-pushed framebuf %d.\n", i);
			} else {
				ready_id = i;
				set_ready_buffer(context,ready_id);
			}
		}

		set_free_buffer_busy(context);
		if (get_push_frame_count(context) < 2) {
			reuse_ready_buffer(context, ready_id);
			goto irq_end;
		}
		*/

        //when interrupt comes, the rITU656IN_DRAM_Y_ADDR frame data is not ready completely
        //should use previous frame
        context->pprev_frame = context->prev_frame;
        context->prev_frame = context->cur_frame;
        context->cur_frame = frame_id;
        if (/*context->pprev_frame < 0 || */context->prev_frame < 0) {
            goto irq_end;
        }

        if (context->itu656in.interlace) {
            if (intr_stat & FRAME_INTERRUPT_INTERRUPT) {
                deinter_type = (context->itu656in.system == CVBS_PAL) ? DEINTERLACE_TYPE_PAL : DEINTERLACE_TYPE_NTSC;
                //itu656_INFO("deinter_status: %d, timeout: %d", ct->deinter_status, timeout);
                while(context->deinter_status && timeout--);
                context->deinter_status = 1;
                deinterlace_process (
                                DEINTERLACE_LINE_SIZE_720H,
                                DEINTERLACE_DATA_MODE_420,
                                deinter_type,
                                DEINTERLACE_FIELD_EVEN,
                                context->framebuf_phyaddr[context->prev_frame].yaddr,
                                context->framebuf_phyaddr[context->prev_frame].yaddr + CVBS_WIDTH,
                                context->framebuf_phyaddr[context->cur_frame].yaddr,
                                0,
                                0,	  // for yuv420
                                0,
                                context);   // for yuv420
            }
        } else {
            //poll
            //printk("frame_id=%d, count=%d.\n", dvr_dev->prev_frame, dvr_dev->frame_finish_count);
            if (context->frame_finish_count >= context->framebuf_num)
                context->frame_finish_count = context->framebuf_num - 1;
#if 0	//test
            context->frame_finish[context->frame_finish_count++] = context->pre_frame;//set flag to wakeup frame_finish_waitq
#else
            context->frame_finish[context->frame_finish_count++] = context->cur_frame;//set flag to wakeup frame_finish_waitq
#endif
            wake_up_interruptible(&dvr_dev->frame_finish_waitq);

            //async
            if(dvr_dev->fasync_queue != NULL) {
                //printk(KERN_ALERT "kill_fasync camera frame finish.\n");
                kill_fasync(&dvr_dev->fasync_queue, SIGIO, POLL_IN);
            }
        }
    }

irq_end:
    spin_unlock_irqrestore(&context->spin_lock, flags);

    return IRQ_HANDLED;
}

void dither_timeout_timer(struct timer_list *t)
{
    struct dvr_dev *dvr_dev = from_timer(dvr_dev, t, timer);
	struct ark_itu656in_context *context = &dvr_dev->context;
    int i;

    context->discard_frame = START_DISCARD_FRAME;
    context->prev_frame = -1;
    context->cur_frame = -1;
    for (i = 0; i < context->framebuf_num; i++)
        context->framebuf_status[i] = FRAMEBUF_STATUS_FREE;

    ark_itu656_enable_write(context);
}

