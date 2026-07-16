/*
 * Arkmicro v4l2 driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/videodev2.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <asm/setup.h>
#include <linux/dma-mapping.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-image-sizes.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>
#include "ark1668_vin.h"

extern int ark_itu656_display_init(int src_width, int src_height,int out_posx, int out_posy,int out_width, int out_height,int interlace);
extern int ark_itu656_display_addr(unsigned int addr);
extern int ark_itu656_display_uninit(void);
extern int ark168vin_set_display(int layer, unsigned int cmd, void *arg);
extern int ark_disp_set_layer_en(int layer_id, int enable);
extern int ark_carback_get_status(void);
extern int ark168vin_set_scal(int layer,int src_w, int src_h,int out_w,int out_h);
extern void carback_first_enter(void);
int dvr_enter_carback(void);
int dvr_exit_carback(void);
static void vin_start(struct dvr_dev *vin);
static void vin_init(struct ark1668_vin_device *vin, struct vin_para *para);
static void vin_stop(struct dvr_dev *vin);
static void vin_exit(void);
extern unsigned int g_screen_width;
extern unsigned int g_screen_height;

struct ark1668_vin_device* g_ark168_vin = NULL;

static int vin_queue_setup(struct vb2_queue *vq,
			    unsigned int *nbuffers, unsigned int *nplanes,
			    unsigned int sizes[], struct device *alloc_devs[])
{
	struct ark1668_vin_device* ark_vin = vb2_get_drv_priv(vq);
	unsigned int size = ark_vin->fmt.fmt.pix.sizeimage;

	if (*nplanes)
		return sizes[0] < size ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = size;
	return 0;
}

static int vin_buffer_init(struct vb2_buffer *vb)
{
	return 0;
}

static int vin_buffer_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct ark1668_vin_device* ark_vin = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long size = ark_vin->fmt.fmt.pix.sizeimage;

	if (vb2_plane_size(vb, 0) < size) {
		v4l2_err(&ark_vin->v4l2_dev, "buffer too small (%lu < %lu)\n",
			 vb2_plane_size(vb, 0), size);
		return -EINVAL;
	}

	vb2_set_plane_payload(vb, 0, size);
	vbuf->field = ark_vin->fmt.fmt.pix.field;
	return 0;
}

static void vin_config(void)
{
	struct vin_para para = {0};
	struct ark_private_data *arkvin_priv = g_ark168_vin->pdata.g_arkvin_priv;
	para.source = DVR_SOURCE_AUX;

    if(!arkvin_priv->init){
            if(arkvin_priv->dvr_config() == 0){
                    arkvin_priv->init = 1;
            }

            arkvin_priv->select_channel(arkvin_priv->channel);
    }

    para.width  = vin_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
    para.height = vin_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;
    
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	g_ark168_vin->dvr_dev->clock_scale_store = ((vin_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f);
	memcpy(&g_ark168_vin->dvr_dev->itu656in_back, &g_ark168_vin->dvr_dev->itu656in, sizeof(struct vin_para));
	g_ark168_vin->dvr_dev->carback_break = 0;
	if(arkvin_priv->get_progressive)
		para.progressive = arkvin_priv->get_progressive();
	vin_init(g_ark168_vin, &para);	
	vin_start(g_ark168_vin->dvr_dev);
	g_ark168_vin->dvr_dev->enter_carback = 1;
	
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
}
static void vin_setaddr(struct ark1668_vin_device *vin)
{
	struct v4l2_pix_format *pixfmt = &vin->fmt.fmt.pix;
	u32 sizeimage = pixfmt->sizeimage;	
	dma_addr_t addr_dma;
	/* Get the physical address of the assigned BUF  */
	addr_dma = vb2_dma_contig_plane_dma_addr(&vin->cur_frm->vb.vb2_buf, 0);
	ARKVIN_DBGPRTK("vin_setaddr addr_dma  : %p\n",addr_dma);
	vin_writel(ARK1668_ITU656_DRAM_DEST1, addr_dma);
}

static int vin_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct ark1668_vin_device *vin = vb2_get_drv_priv(vq);
	unsigned long flags;
	g_ark168_vin->stream_flag = true;
	vin_config();
	spin_lock_irqsave(&vin->ark_queue_lock, flags);
	vin->sequence = 0;
	vin->stop = false;
	reinit_completion(&vin->comp);
	vin->cur_frm = list_first_entry(&vin->ark_queue,struct vin_buffer, list);
	ARKVIN_DBGPRTK("start_streaming->get out buf: %p\n",vin->cur_frm->vb.vb2_buf.planes[0].mem_priv);
	list_del(&vin->cur_frm->list);
	vin_setaddr(vin);
	spin_unlock_irqrestore(&vin->ark_queue_lock, flags);
	
	return 0;
}

static void vin_stop_streaming(struct vb2_queue *vq)
{
	struct ark1668_vin_device* ark_vin = vb2_get_drv_priv(vq);
	struct vin_buffer *buf;
	unsigned long flags;

	ark_vin->stop = true;
	g_ark168_vin->stream_flag = false;
	vin_exit();
	
	/* Release all active buffers */
	spin_lock_irqsave(&ark_vin->ark_queue_lock, flags);
	if (unlikely(ark_vin->cur_frm)) {
		vb2_buffer_done(&ark_vin->cur_frm->vb.vb2_buf,
				VB2_BUF_STATE_ERROR);
		ark_vin->cur_frm = NULL;
	}
	list_for_each_entry(buf, &ark_vin->ark_queue, list)
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	INIT_LIST_HEAD(&ark_vin->ark_queue);
	spin_unlock_irqrestore(&ark_vin->ark_queue_lock, flags);
}

static void vin_buffer_queue(struct vb2_buffer *vb)
{
	ARKVIN_DBGPRTK("vin_buffer_queue add buf : %p\n",vb->planes[0].mem_priv);
	
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vin_buffer *buf = container_of(vbuf, struct vin_buffer, vb);
	struct ark1668_vin_device *vin = vb2_get_drv_priv(vb->vb2_queue);
	unsigned long flags;
	dma_addr_t addr_dma;
	addr_dma = vb2_dma_contig_plane_dma_addr(vb, 0);
	/*app buf physical address can be obtained directly from this variable */
	vb->planes[0].m.offset = addr_dma;

	ARKVIN_DBGPRTK("!vin->cur_frm = %d,list_empty(&vin->ark_queue)= %d,vb2_is_streaming(vb->vb2_queue) = %d\n",!vin->cur_frm,list_empty(&vin->ark_queue),vb2_is_streaming(vb->vb2_queue));
	spin_lock_irqsave(&vin->ark_queue_lock, flags);
	if (!vin->cur_frm && list_empty(&vin->ark_queue) && vb2_is_streaming(vb->vb2_queue)){
		vin->cur_frm = buf;
		vin_setaddr(vin);
	}	
	else{
		/* add buf to list */
		list_add_tail(&buf->list, &vin->ark_queue);
	}
	spin_unlock_irqrestore(&vin->ark_queue_lock, flags);
	
}

static const struct vb2_ops vin_vb2_ops = {
	.queue_setup		= vin_queue_setup,
	.buf_init			= vin_buffer_init,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.buf_prepare		= vin_buffer_prepare,
	.start_streaming	= vin_start_streaming,
	.stop_streaming		= vin_stop_streaming,
	.buf_queue		= vin_buffer_queue,
};

static int vin_querycap(struct file *file, void *priv,struct v4l2_capability *cap)
{
	strcpy(cap->driver, ARK_VIN_NAME);
	strcpy(cap->card, "Ark videoin driver");
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE;
	
	return 0;
}

static int vin_enum_fmt_vid_cap(struct file *file, void *priv,struct v4l2_fmtdesc *f)
{
	u32 index = f->index;

	if (index >= 1)
		return -EINVAL;

	f->pixelformat = V4L2_PIX_FMT_VYUY;

	return 0;
}

static int vin_g_fmt_vid_cap(struct file *file, void *priv,struct v4l2_format *fmt)
{
	struct ark1668_vin_device *vin = video_drvdata(file);
	*fmt = vin->fmt;
	return 0;
}


static int vin_try_fmt(struct ark1668_vin_device *vin, struct v4l2_format *f)
{
	struct v4l2_pix_format *pixfmt = &f->fmt.pix;
	int ret;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	pixfmt->width = CVBS_WIDTH;
	pixfmt->height = CVBS_HEIGHT;
	pixfmt->field = V4L2_FIELD_ANY;
	pixfmt->bytesperline = (pixfmt->width * 16) >> 3;
	pixfmt->sizeimage = pixfmt->bytesperline * pixfmt->height;
	ARKVIN_DBGPRTK("pixfmt->width = %d.\n",pixfmt->width);
	ARKVIN_DBGPRTK("pixfmt->height = %d.\n",pixfmt->height);
	ARKVIN_DBGPRTK("pixfmt->bytesperline = %d.\n",pixfmt->bytesperline);
	ARKVIN_DBGPRTK("pixfmt->sizeimage = %d.\n",pixfmt->sizeimage);

	return 0;
}


static int vin_set_fmt(struct ark1668_vin_device *vin, struct v4l2_format *f)
{
	int ret;
	ret = vin_try_fmt(vin, f);
	if (ret)
		return ret;
	
	vin->fmt = *f;
	return 0;
}

static int vin_try_fmt_vid_cap(struct file *file, void *priv,struct v4l2_format *f)
{
	struct ark1668_vin_device *ark_vin = video_drvdata(file);
	return vin_try_fmt(ark_vin, f);
}

static int vin_s_fmt_vid_cap(struct file *file, void *priv,struct v4l2_format *f)
{
	struct ark1668_vin_device *ark_vin = video_drvdata(file);

	if (vb2_is_streaming(&ark_vin->vb2_vidq))
		return -EBUSY;

	return vin_set_fmt(ark_vin, f);
}

static int vin_enum_input(struct file *file, void *priv,struct v4l2_input *inp)
{
	if (inp->index != 0)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = 0;
	strcpy(inp->name, "Camera");
	return 0;
}

static int vin_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0;
	return 0;
}

static int vin_s_input(struct file *file, void *priv, unsigned int i)
{
	if (i > 0)
		return -EINVAL;
	return 0;
}

static long vin_ioctl_default(struct file *file, void *priv,
			       bool valid_prio, unsigned int cmd, void *param)
{
	struct dvr_dev *dvr_dev =g_ark168_vin->dvr_dev;
	struct vin_display_outpara *vout_para  = (struct vin_display_outpara *)param;
	unsigned long error = 0;
	unsigned long flags;
	if(!dvr_dev){
		printk(KERN_ERR"%s error null device", __func__);
		return -ENXIO;
	}
	spin_lock_irqsave(&dvr_dev->spin_lock, flags);
	switch (cmd)
	{
		case VIN_SHOW_WINDOW:
		{
			ark168vin_set_display(vout_para->layer,VIN_SHOW_WINDOW,NULL);
			break;
		}
		
		case VIN_HIDE_WINDOW:
		{
			ark168vin_set_display(vout_para->layer,VIN_HIDE_WINDOW,NULL);
			break;
		}
		
		case VIN_SET_WINDOW_POS:
		{
			ark168vin_set_display(vout_para->layer,VIN_SET_WINDOW_POS,&vout_para->pos_data);
			break;
		}
		
		case VIN_SET_SOURCE_SIZE:
		{
			ark168vin_set_display(vout_para->layer,VIN_SET_SOURCE_SIZE,&vout_para->source_size);
			break;
		}

		case VIN_SET_LAYER_SIZE:
		{
			ark168vin_set_display(vout_para->layer,VIN_SET_LAYER_SIZE,&vout_para->layer_size);
			break;
		}
		
		case VIN_SET_WINDOW_FORMAT:
		{
			ark168vin_set_display(vout_para->layer,VIN_SET_WINDOW_FORMAT,&vout_para->format);
			break;
		}
		
		case VIN_SET_WINDOW_ADDR:
		{
			ark168vin_set_display(vout_para->layer,VIN_SET_WINDOW_ADDR,&vout_para->disp_addr);
			break;
		}
		
		case VIN_SET_WINDOW_SCALER:
		{
			ark168vin_set_display(vout_para->layer,VIN_SET_WINDOW_SCALER,&vout_para->scaler);
			break;
		}
		
		default:
			printk("%s: error cmd 0x%x\n", __func__, cmd);
			error = -EFAULT;
			goto end;
    }
	return 0;
	
end:
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
    return error;
}

static int ark_fb_update_window_by_layer_id(int layer_id, struct arkfb_update_window *arg)
{
        return 0;
}

static int ark_disp_set_tvenc_cfg(struct ark_disp_tvenc_cfg_arg *arg)
{
        return 0;
}

static void ark_disp_set_tvout_next_oddfield_bufaddr(unsigned int addr)
{

}
static void ark_disp_set_tvout_next_evenfield_bufaddr(unsigned int addr)
{

}

static const struct v4l2_ioctl_ops vin_ioctl_ops = {
	.vidioc_querycap		= vin_querycap,
	.vidioc_enum_fmt_vid_cap	= vin_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= vin_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= vin_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= vin_try_fmt_vid_cap,

	.vidioc_enum_input		= vin_enum_input,
	.vidioc_g_input			= vin_g_input,
	.vidioc_s_input			= vin_s_input,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,
	.vidioc_default			= vin_ioctl_default,
};

static int vin_open(struct file *file)
{
	struct ark1668_vin_device* ark_vin = video_drvdata(file);
	int ret;
	if (mutex_lock_interruptible(&ark_vin->lock))
		return -ERESTARTSYS;

	ret = v4l2_fh_open(file);
	if (ret < 0)
		goto unlock;

	if (!v4l2_fh_is_singular_file(file))
		goto unlock;
	
unlock:
	mutex_unlock(&ark_vin->lock);
	return ret;
}

static int vin_release(struct file *file)
{
	struct ark1668_vin_device* ark_vin = video_drvdata(file);
	bool fh_singular;
	int ret;

	mutex_lock(&ark_vin->lock);

	fh_singular = v4l2_fh_is_singular_file(file);

	ret = _vb2_fop_release(file, NULL);

	mutex_unlock(&ark_vin->lock);
	return 0;
}


static const struct v4l2_file_operations vin_fops = {
	.owner		= THIS_MODULE,
	.open		= vin_open,
	.release	= vin_release,
	.unlocked_ioctl	= video_ioctl2,
	.read		= vb2_fop_read,
	.mmap		= vb2_fop_mmap,
	.poll		= vb2_fop_poll,
};

static int deinterlace_process (unsigned int deinterlace_size,  unsigned int data_mode, unsigned int deinterlace_type,
	unsigned int deinterlace_field, unsigned int src_field_addr_0, unsigned int src_field_addr_1,unsigned int src_field_addr_2,
	unsigned int dst_y_addr, unsigned int dst_u_addr, unsigned int dst_v_addr)
{
	unsigned int pixel_per_line;
	unsigned int total_line;
	unsigned int pn;
	unsigned int denoise_bypass;
	unsigned int stride;
	unsigned int only_wr_1_field;
	unsigned int field;
	unsigned int out_mode;
	unsigned int vary_level_th;
	int ret = DEINTERLACE_SUCCESS;
        unsigned int val;
	
	if(deinterlace_field != DEINTERLACE_FIELD_ODD
		&&	deinterlace_field != DEINTERLACE_FIELD_EVEN){
		printk("invalid deinterlace field (%d)\r\n", deinterlace_field);
		return DEINTERLACE_PARA_ERROR;				
	}
	field = deinterlace_field;
	
	if(deinterlace_size == DEINTERLACE_LINE_SIZE_960H){
		pixel_per_line = 120 * (1 + data_mode);
	}else if(deinterlace_size == DEINTERLACE_LINE_SIZE_720H){
		pixel_per_line = 90 * (1 + data_mode);
	}else{
		printk("invalid deinterlace size (%d)\r\n", deinterlace_size);
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(data_mode == DEINTERLACE_DATA_MODE_420){
		denoise_bypass = 1;
		stride = pixel_per_line;
		only_wr_1_field = 1;
	}else if(data_mode == DEINTERLACE_DATA_MODE_422){
		if(dst_y_addr == 0){
			printk("illegal deinterlace dst Y address\r\n");
			return DEINTERLACE_PARA_ERROR;
		}
		denoise_bypass = 1;
		stride = 0;
		only_wr_1_field = 0;
	}else{
		printk("invalid deinterlace data mode (%d)\r\n", data_mode);
		return DEINTERLACE_PARA_ERROR;
	}
	
	if(deinterlace_type == DEINTERLACE_TYPE_PAL){
		pn = 0;
		total_line = 288;
		vary_level_th = 0x800;
	}else if(deinterlace_type == DEINTERLACE_TYPE_NTSC){
		pn = 1;
		total_line = 240;
		vary_level_th = 0x8;
	}else{
		printk("invalid deinterlace type (%d)\r\n", deinterlace_type);
		return DEINTERLACE_PARA_ERROR;		
	}		

	out_mode = DEINTERLACE_DATA_MODE_422;
	
	val = (out_mode<<31)
		|  (0x0 << 29)
		|  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field                      	
		|  (pixel_per_line << 20)       // pixel_pl
		|  (total_line << 11)           // total_line
		|  (stride << 3)                // stride
		|  (data_mode << 2)             // data_mode
		|  (field << 1);                 // field
	vin_writel_dein(ARK1668_DEINTERLACE_CTRL0, val);	

 	val = (out_mode<<31)
		|  (0x0 << 29)
		|  (only_wr_1_field << 28)  	  // field_1 only_wr_1_field                      	
		|  (pixel_per_line << 20)       // pixel_pl
		|  (total_line << 11)           // total_line
		|  (stride << 3)                // stride
		|  (data_mode << 2)             // data_mode
		|  (field << 1);                 // field
	vin_writel_dein(ARK1668_DEINTERLACE_CTRL0, val);	
 
	val = (0x8 << 24 ) 
		|(vary_level_th << 8)
		|(0X8 << 0);
       vin_writel_dein(ARK1668_DEINTERLACE_CTRL1, val);		
	val = 0x300A4230; 
       vin_writel_dein(ARK1668_DEINTERLACE_CTRL2, val);	

	// denoise bypass  pn: 1:n display_motion line_intra global_cnt display_mv_0
	val = (0x07 << 16)				//motion_ctrl_reg5\uff1a\u964d\u566a\u95e8\u9650
		| (denoise_bypass << 15)  
		| (pn << 13) 
		| (0x1 << 11)
		| (0x1 << 10)
		| (0x1 << 9)
		| (0x0 << 6)
		| (0x0 << 5)
		| (0x1 << 3)
		| (0x0 << 2)  // max_ycbcr_ena :use for control the max control   
		| (0x1 << 1)
		| (0x0 << 0);
       vin_writel_dein(ARK1668_DEINTERLACE_CTRL3, val);	
     
	vin_writel_dein(ARK1668_DEINTERLACE_FILM_MODECTRL, (0x0 << 31));

	vin_writel_dein(ARK1668_DEINTERLACE_SADDR0, src_field_addr_0);	// s0_0                             
	vin_writel_dein(ARK1668_DEINTERLACE_SADDR1, src_field_addr_1);	// s1_0
	vin_writel_dein(ARK1668_DEINTERLACE_SADDR2, src_field_addr_2);	// s2_0
     
	vin_writel_dein(ARK1668_DEINTERLACE_DADDRY, dst_y_addr);	// dy_0  when filed = 1 ,data_mode = 0  then dy_0 = s0_0 , when filed = 0 ,data_mode = 0  then dy_0 = s2_0 
	vin_writel_dein(ARK1668_DEINTERLACE_DADDRU, dst_u_addr);	// du_0
	vin_writel_dein(ARK1668_DEINTERLACE_DADDRV, dst_v_addr);	// dv_0
     
	//	pingpong addr fetch 
	vin_writel_dein(ARK1668_DEINTERLACE_ADDR_SWITCHMODE, 0x0);
   
	// \u6e05\u9664\u4e2d\u65ad
	vin_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x3); 
	
	// start de-interlace
	vin_writel_dein(ARK1668_DEINTERLACE_START, 0x1); 
 
	return ret;
}

void mirror_paint_work(struct work_struct *work)
{
	struct dvr_dev* dvr_dev = g_ark168_vin->dvr_dev;
	int width,height;
	int i,j;
	unsigned char *addr_src;
	unsigned char *addr_dest;
	unsigned short *psrc;
	unsigned short *pdest;
	unsigned int right_cut;
        unsigned int value;

	if(!dvr_dev->interlace) return;
	if(dvr_dev->mirror_type == MIRROR_TYPE_NONE) return;

	GetPingPongNextBuf(dvr_dev->write_mirror,ITU656_BUFFER_NUM);
	dvr_dev->buf_status[dvr_dev->write_mirror] &= ~ITU656_BUFFER_FULL_MIRROR;

	if (dvr_dev->system == PAL) {
		width = 720;
		height = 576;
		right_cut = 12;//must even ,as :2 ,4, 6..., set point to uy
	} else {
		width = 720;
		height = 480;
		right_cut = 12;//must even ,as :2 ,4, 6...,set point to uy
	}

	addr_src  = (unsigned char *)dvr_dev->evenbuf_virtaddr[dvr_dev->write_mirror];
	addr_dest = (unsigned char *)dvr_dev->oddbuf_virtaddr[dvr_dev->write_mirror];
	dma_map_single(NULL,(void *)addr_src, width*height*2, DMA_FROM_DEVICE);

	//srcdata : vyuy vyuy....vyuy vyuy 
	if(dvr_dev->mirror_type == MIRROR_TYPE_L2R){
		for(i = 0;i < height; i++){
			psrc  = (unsigned short *)(addr_src + width*2*i + (width-1-right_cut)*2);
			pdest = (unsigned short *)(addr_dest + width*2*i);
			for(j = 0;j < width-right_cut; j++){
				*pdest = *psrc;
				pdest++;
				psrc--;
			}
		}
	}
	else if(dvr_dev->mirror_type == MIRROR_TYPE_U2D){
		for(i = 0;i < height; i++){
			psrc  = (unsigned short *)(addr_src + width*2*i);
			pdest = (unsigned short *)(addr_dest + width*2*(height-1-i));
			memcpy(pdest,psrc,width*2);
		}
	}
	else if(dvr_dev->mirror_type == MIRROR_TYPE_L2R_U2D){
		for(i = 0;i < height; i++){
			psrc  = (unsigned short *)(addr_src + width*2*i + (width-1-right_cut)*2);
			pdest = (unsigned short *)(addr_dest + width*2*(height-1-i));
			for(j = 0;j < width-right_cut; j++){
				*pdest = *psrc;
				pdest++;
				psrc--;
			}
		}
	}
	else{
		memcpy(addr_dest,addr_src,width*height*2);
	}
	
	//for mirror
	value = vin_readl_lcd(ARK1668_LCDC_VIDEO2_CTL);
	if(dvr_dev->mirror_type & MIRROR_TYPE_L2R) value |= (0x01<<17);
	else value &= ~(0x03<<17);
        vin_writel_lcd(ARK1668_LCDC_VIDEO2_CTL,value);

	dma_map_single(NULL,(void *)addr_dest, width*height*2, DMA_TO_DEVICE);
	dvr_dev->buf_status[dvr_dev->write_mirror] |= ITU656_BUFFER_FULL_LCD;
}


static void ark_vin_reg_uninit(void)
{
        // Clock Off
        vin_writel_sys(ARK1668_SYS_PER_CLK_EN, vin_readl_sys(ARK1668_SYS_PER_CLK_EN)& ~(1 << 12));
        vin_writel_sys(ARK1668_SYS_AXI_CLK_EN, vin_readl_sys(ARK1668_SYS_AXI_CLK_EN)& ~(1 << 2));
        vin_writel_sys(ARK1668_SYS_AHB_CLK_EN, vin_readl_sys(ARK1668_SYS_AHB_CLK_EN)& ~(1 << 10));
}

static inline void ark_vin_disable_write(void)
{
        vin_writel(ARK1668_ITU656_MODULE_EN,vin_readl(ARK1668_ITU656_MODULE_EN) | (1 << 2));
}

static inline void ark_vin_enable_write(void)
{
        vin_writel(ARK1668_ITU656_MODULE_EN,vin_readl(ARK1668_ITU656_MODULE_EN) & ~(1 << 2));
}

void ark_vin_disable(void)
{
        vin_writel(ARK1668_ITU656_IMR, 0);
        vin_writel(ARK1668_ITU656_ENABLE_REG,vin_readl(ARK1668_ITU656_ENABLE_REG) & ~(1 << 0));
}

static int ark_disp_set_gui_tvout(int enable)
{
        return 0;
}

void ark_sys_pad_config(unsigned int reg_offset, unsigned int mask, 
	unsigned int bit_offset, unsigned int val)
{
	unsigned int reg;

	reg = vin_readl_sys(reg_offset);
	reg &= ~(mask << bit_offset);
	reg |= ((val & mask) << bit_offset);
	vin_writel_sys(reg_offset, reg);
}

static void itu656_pad(void)
{
        unsigned int val;

        val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0B);
        val |= (0x1FF<<16);
        vin_writel_sys(ARK1668_SYS_PAD_CTRL0B,val);

        val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0A);
        val &= ~(0xF<<4);
        val |= 5<<4;
        vin_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);
}


static void ark_itu656_set_direct_data_to_lcd_enable(int enable)
{
        unsigned int val;

	itu656_pad(); 
	vin_writel(ARK1668_ITU656_INPUT_SEL, vin_readl(ARK1668_ITU656_INPUT_SEL)|(1<<0));
	if (enable) {
                val = 1<<2 | // 1=disable itu656 data write back to ddr2 
                      1<<1;  // 1=enable itu656 data and timing by pass to lcd
                vin_writel(ARK1668_ITU656_MODULE_EN, vin_readl(ARK1668_ITU656_MODULE_EN)| val);
                vin_writel(ARK1668_ITU656_PIX_LINE_NUM_DELTA, 30<<8 | 10);
                // rITU656IN_IMR = 1<<7; 
	} else {
                val = 1<<2 | // 0=enable itu656 data write back to ddr2 
                      1<<1;  // 1=disable itu656 data and timing by pass to lcd
                vin_writel(ARK1668_ITU656_MODULE_EN, vin_readl(ARK1668_ITU656_MODULE_EN)& ~val);
                vin_writel(ARK1668_ITU656_IMR, 0); // disable interrupt outputs
	}
}

static inline void ark_itu656_set_global_enable(int enable)
{
        if (enable)
                vin_writel(ARK1668_ITU656_ENABLE_REG, vin_readl(ARK1668_ITU656_ENABLE_REG)|(1<<0));// global enable
        else
                vin_writel(ARK1668_ITU656_ENABLE_REG, vin_readl(ARK1668_ITU656_ENABLE_REG)& ~(1<<0));// global disable
}

static void vin_exit(void)
{
	struct ark_private_data *arkvin_priv = g_ark168_vin->pdata.g_arkvin_priv;
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	g_ark168_vin->dvr_dev->enter_carback = 0;
	g_ark168_vin->dvr_dev->work_status = 0;
	del_timer(&g_ark168_vin->dvr_dev->timer);
	g_ark168_vin->dvr_dev->show_video = 0;
	g_ark168_vin->dvr_dev->carback_signal = 0;
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	msleep(20);
	ark_vin_disable_write(); /*stop write data back*/
	ark_vin_disable();
	ark_vin_reg_uninit();
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
}

/********************************************************************************************/
int ark_itu656_direct_data_to_lcd_start(void)
{
        itu656_pad(); 
        ark_itu656_set_direct_data_to_lcd_enable(1);
        return 0;
}
EXPORT_SYMBOL(ark_itu656_direct_data_to_lcd_start);

void ark_itu656_stop(void)
{
        ark_itu656_set_global_enable(0);
       	 vin_writel(ARK1668_ITU656_IMR ,0); // disable all interrupt outputs    
}
EXPORT_SYMBOL(ark_itu656_stop);

int ark_sys_pad_config_gpio_mode(int gpio)
{
	if((gpio<=GPIO1) && (gpio>=GPIO0))
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x3, (gpio-GPIO0)*2, 0);
	else if(gpio<=GPIO9)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL00, 0xf, (gpio-GPIO2)*4, 0);
	else if(gpio<=GPIO17)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL01, 0xf, (gpio-GPIO10)*4, 0);
	else if(gpio<=GPIO25)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL02, 0xf, (gpio-GPIO18)*4, 0);
	else if(gpio<=GPIO29)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL03, 0xf, (gpio-GPIO26)*4, 0);
	else if(gpio<=GPIO38)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL07, 0x3, (gpio-GPIO30)*2, 0);
	else if(gpio<=GPIO46)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL04, 0xf, (gpio-GPIO39)*4, 0);
	else if(gpio<=GPIO54)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL05, 0xf, (gpio-GPIO47)*4, 0);
	else if(gpio<=GPIO61)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL06, 0xf, (gpio-GPIO55)*4, 0);
	else if(gpio<=GPIO71)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL08, 0x3, (gpio-GPIO62)*2, 0);
	else if(gpio<=GPIO84)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x3, (gpio-GPIO72)*2+4, 0);
	else if(gpio<=GPIO85)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x1, 30, 0);
	else if(gpio<=GPIO86)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL09, 0x1, 31, 0);
	else if(gpio<=GPIO116)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL0B, 0x1, (gpio-GPIO85), 0);
	else if(gpio<=GPIO127)
		ark_sys_pad_config(ARK1668_SYS_PAD_CTRL0C, 0x1, (gpio-GPIO117), 0);
	else
		return  -1;

	return 0;
}
EXPORT_SYMBOL(ark_sys_pad_config_gpio_mode);

void dvr_set_sys_clk(int level)
{
        int val, width, height;

        width  = vin_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
        height = vin_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;
    
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	if(level)
	{
		if(g_ark168_vin->dvr_dev->enter_carback){//only carback support 1080p
		
                if((width == 800) || (height== 480)) val = 0;
                else val = 2;

                if(((vin_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f) != val)
                        ark_sys_pad_config(ARK1668_SYS_DEVICE_CLK_CFG0, 0xf, 24, val);
                }
	}else{
		if(g_ark168_vin->dvr_dev->enter_carback){//IntScal2Clk 

                if((width == 800) || (height== 480)) val = 2;
                else val = 4;
            
                if(((vin_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f) != val)
                        ark_sys_pad_config(ARK1668_SYS_DEVICE_CLK_CFG0, 0xf, 24, val);
                }
	}
    
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
}
EXPORT_SYMBOL(dvr_set_sys_clk);

int dvr_get_pragressive(void)
{
	int pragressive;

	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	pragressive = !g_ark168_vin->dvr_dev->interlace;
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
	
	return pragressive;
}
EXPORT_SYMBOL(dvr_get_pragressive);

void dvr_restart(void)
{
	struct vin_para para = {0};
	para.source = DVR_SOURCE_CAMERA;
    
        para.width  = vin_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
        para.height = vin_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;

	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	if(g_ark168_vin->dvr_dev->enter_carback == 1)
	{
		g_ark168_vin->dvr_dev->work_status = 0;
		del_timer(&g_ark168_vin->dvr_dev->timer);
		g_ark168_vin->dvr_dev->show_video = 0;
		ark_disp_set_layer_en(DISPLAY_LAYER, 0);
		ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_vin_disable_write(); /*stop write data back*/
		ark_vin_disable();
		msleep(100);
		if(g_ark168_vin->dvr_dev->enter_carback == 1)
		{
			ark_vin_reg_uninit();
			g_ark168_vin->dvr_dev->carback_break = 1;
			if(g_ark168_vin->pdata.g_arkvin_priv->get_progressive)
				para.progressive = g_ark168_vin->pdata.g_arkvin_priv->get_progressive();
			vin_init(g_ark168_vin, &para);
			vin_start(g_ark168_vin->dvr_dev);
		}
	}
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
}
EXPORT_SYMBOL(dvr_restart);

int ark_get_carback_signal(void)
{
	return g_ark168_vin->dvr_dev->carback_signal;
}
EXPORT_SYMBOL(ark_get_carback_signal);

int dvr_enter_carback(void)
{
	g_ark168_vin->dvr_dev->buffer_virtaddr = (void *)__get_free_pages(GFP_KERNEL, get_order(g_ark168_vin->dvr_dev->buffer_size));
	if (!g_ark168_vin->dvr_dev->buffer_virtaddr) {
	        printk("%s get. buffer fail\n", __func__);
	}
	g_ark168_vin->dvr_dev->buffer_phyaddr = virt_to_phys(g_ark168_vin->dvr_dev->buffer_virtaddr);
	ARKVIN_DBGPRTK("g_ark168_vin->dvr_dev->buffer_size = %d.\n",g_ark168_vin->dvr_dev->buffer_size);
	struct vin_para para = {0};
	struct ark_private_data *arkvin_priv = g_ark168_vin->pdata.g_arkvin_priv;
	if(g_ark168_vin->stream_flag)
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	para.source = DVR_SOURCE_CAMERA;
	g_ark168_vin->vin_status = g_ark168_vin->stream_flag;
	ARKVIN_DBGPRTK("g_ark168_vin->vin_status = %d.\n",g_ark168_vin->vin_status);
	g_ark168_vin->stream_flag = false;
	//para.progressive = 1;

        if(!arkvin_priv->init){
                if(arkvin_priv->dvr_config() == 0){
                        arkvin_priv->init = 1;
                }

                arkvin_priv->select_channel(arkvin_priv->channel);
        }

        para.width  = vin_readl_lcd(ARK1668_LCDC_TIMING1) & 0xfff;
        para.height = vin_readl_lcd(ARK1668_LCDC_TIMING2) >> 10 & 0x7ff;
    
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	if(arkvin_priv->enter_carback_cb)
		arkvin_priv->enter_carback_cb();
	g_ark168_vin->dvr_dev->clock_scale_store = ((vin_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f);
    
	memcpy(&g_ark168_vin->dvr_dev->itu656in_back, &g_ark168_vin->dvr_dev->itu656in, sizeof(struct vin_para));
	if (g_ark168_vin->dvr_dev->work_status) {
		g_ark168_vin->dvr_dev->work_status = 0;
		del_timer(&g_ark168_vin->dvr_dev->timer);
		g_ark168_vin->dvr_dev->show_video = 0;
		if(!g_ark168_vin->vin_status)
		ark_disp_set_layer_en(DISPLAY_LAYER, 0);
		ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_vin_disable_write(); /*stop write data back*/
		ark_vin_disable();
		g_ark168_vin->dvr_dev->channel = ARK7116_AV0;
		if(arkvin_priv->select_channel)
			arkvin_priv->select_channel(g_ark168_vin->dvr_dev->channel);
		msleep(200);
		ark_vin_reg_uninit();
		g_ark168_vin->dvr_dev->carback_break = 1;
	} else g_ark168_vin->dvr_dev->carback_break = 0;
	printk("%s carback_break=%d.\n", __FUNCTION__, g_ark168_vin->dvr_dev->carback_break);
	if(arkvin_priv->get_progressive)
		para.progressive = arkvin_priv->get_progressive();
	vin_init(g_ark168_vin, &para);	
	vin_start(g_ark168_vin->dvr_dev);
	g_ark168_vin->dvr_dev->enter_carback = 1;
	
	if(arkvin_priv->dvr_start_cb)
		arkvin_priv->dvr_start_cb();
	
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_enter_carback);

int dvr_exit_carback(void)
{
	struct ark_private_data *arkvin_priv = g_ark168_vin->pdata.g_arkvin_priv;
	g_ark168_vin->stream_flag = g_ark168_vin->vin_status;
	if(g_ark168_vin->stream_flag)
		ark168vin_set_scal(DISPLAY_LAYER,NTSC_WIDTH,NTSC_HEIGHT,DISPLAY_WIDTH,DISPLAY_HEIGHT);
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	free_pages((unsigned long)g_ark168_vin->dvr_dev->buffer_virtaddr, get_order(g_ark168_vin->dvr_dev->buffer_size));
	g_ark168_vin->dvr_dev->enter_carback = 0;
	del_timer(&g_ark168_vin->dvr_dev->timer);
	g_ark168_vin->dvr_dev->show_video = 0;
	g_ark168_vin->dvr_dev->carback_signal = 0;
	msleep(20);
	if(!g_ark168_vin->stream_flag){
		g_ark168_vin->dvr_dev->work_status = 0;
		ark_vin_disable_write(); /*stop write data back*/
		ark_vin_disable();
	}

	if(((vin_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG0) >> 24) & 0x0f) != g_ark168_vin->dvr_dev->clock_scale_store)
		ark_sys_pad_config(ARK1668_SYS_DEVICE_CLK_CFG0, 0xf, 24, g_ark168_vin->dvr_dev->clock_scale_store);	//clock set back
	vin_init(g_ark168_vin, &g_ark168_vin->dvr_dev->itu656in_back);
	if(arkvin_priv->select_channel)
			arkvin_priv->select_channel(g_ark168_vin->dvr_dev->channel);	
	msleep(200);
	if(!g_ark168_vin->stream_flag)
	ark_vin_reg_uninit();
	printk("%s carback_break=%d.\n", __FUNCTION__, g_ark168_vin->dvr_dev->carback_break);

	if (g_ark168_vin->dvr_dev->start_carback_exit) {
		vin_start(g_ark168_vin->dvr_dev);
		g_ark168_vin->dvr_dev->start_carback_exit = 0;
	} else if (g_ark168_vin->dvr_dev->carback_break) {
		if (g_ark168_vin->dvr_dev->itu656in.tvout) {
			ark_disp_set_layer_en(TVOUT_LAYER, 0);	
			ark_disp_set_gui_tvout(1);
		}
	}
	
	if(arkvin_priv->dvr_stop_cb)
		arkvin_priv->dvr_stop_cb();
	if(arkvin_priv->exit_carback_cb)
		arkvin_priv->exit_carback_cb();
	if(g_ark168_vin->stream_flag)
		ark_disp_set_layer_en(DISPLAY_LAYER, 1);
	
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_exit_carback);

int dvr_exit_wait(void)
{
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	if (g_ark168_vin->dvr_dev->carback_break) {
		spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
		msleep(300);
		spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
		g_ark168_vin->dvr_dev->carback_break = 0;
	}
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_exit_wait);

int dvr_detect_carback_signal(void)
{
	int signal = 0;
	
	spin_lock(&g_ark168_vin->dvr_dev->spin_lock);
	if (g_ark168_vin->dvr_dev->channel != ARK7116_AV0) {
		printk("now is not in carback.\n");
	} else {
		if(g_ark168_vin->pdata.g_arkvin_priv->detect_signal)
			signal = g_ark168_vin->pdata.g_arkvin_priv->detect_signal();
	}
	g_ark168_vin->dvr_dev->carback_signal = signal;
	spin_unlock(&g_ark168_vin->dvr_dev->spin_lock);
	return signal;
}
EXPORT_SYMBOL(dvr_detect_carback_signal);

void ark_itu656_display_int_handler(void)
{
	if(!g_ark168_vin)
		return;
	struct dvr_dev* dvr_dev = g_ark168_vin->dvr_dev;
	unsigned long flags;
	//struct arkfb_window_addr display_addr = {0};
	
	if(!dvr_dev || !dvr_dev->work_status)
		return;
	if(!g_ark168_vin->stream_flag){
		spin_lock_irqsave(&dvr_dev->spin_lock, flags);
		if (dvr_dev->show_video) {
			if(g_ark168_vin->pdata.g_arkvin_priv->detect_signal && g_ark168_vin->pdata.g_arkvin_priv->detect_signal())
			{
				printk(KERN_ALERT "ark_itu656_display_int_handler-->show_video\n");
				ark_disp_set_layer_en(DISPLAY_LAYER, 1);
				if (dvr_dev->itu656in.tvout)
					ark_disp_set_layer_en(TVOUT_LAYER, 1);
				dvr_dev->show_video = 0;
				dvr_dev->carback_signal = 1;
			}
			else
			{
				dvr_dev->carback_signal = 0;
				printk(" No signal detect.\n");
			}
			
		}
		
		//for vbox
		if(dvr_dev->deinter_indirect_show)
			goto end; 
		
		if (dvr_dev->interlace) {
			if(dvr_dev->display_odd_even) {		
				if(dvr_dev->mirror_type == MIRROR_TYPE_NONE)
					ark_itu656_display_addr(dvr_dev->evenbuf_phyaddr[dvr_dev->display_buffer]);
				dvr_dev->display_odd_even = 0;
			} else {
				dvr_dev->buf_status[dvr_dev->display_buffer] &= ~ITU656_BUFFER_FULL_LCD;
				GetPingPongNextBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
				if(dvr_dev->buf_status[dvr_dev->display_buffer] == 0)
				{
					GetPingPongPreBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
					dvr_dev->buf_status[dvr_dev->display_buffer] |= ITU656_BUFFER_FULL_LCD;
				}	
				
				ark_itu656_display_addr(dvr_dev->oddbuf_phyaddr[dvr_dev->display_buffer]);
				dvr_dev->display_odd_even = 1;
			}
		} else {
			dvr_dev->buf_status[dvr_dev->display_buffer] &= ~ITU656_BUFFER_FULL_LCD;
			GetPingPongNextBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
			if(dvr_dev->buf_status[dvr_dev->display_buffer] == 0)
			{
				GetPingPongPreBuf(dvr_dev->display_buffer, ITU656_BUFFER_NUM);
				dvr_dev->buf_status[dvr_dev->display_buffer] |= ITU656_BUFFER_FULL_LCD;
			}							
			ark_itu656_display_addr(dvr_dev->oddbuf_phyaddr[dvr_dev->display_buffer]);
		}
	
end:	
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
		}
}
EXPORT_SYMBOL(ark_itu656_display_int_handler);

/*******************************************************************************************/

static irqreturn_t ark_vin_int_handler(int irq, void *dev_id)
{
	u8 field;
	u32 intr_stat;
	unsigned long flags;
	struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	int deinter_type;
	unsigned int deintout_phyaddr;
	int timeout = 10000;
	int syschange_mask = TOTAL_LINE_CHANGED_INTERRUPT;
	int i;
	intr_stat = vin_readl(ARK1668_ITU656_ISR);
	vin_writel(ARK1668_ITU656_ICR, 0xFF);
	field = (intr_stat >> 8) & 0x1;
	spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	if (!dvr_dev->interlace)
		syschange_mask |= ACTIVE_PIX_CHANGED_INTERRUPT;
	
	if(intr_stat & syschange_mask)
	{
		dvr_dev->show_video = 0;
		if(!g_ark168_vin->stream_flag)
		ark_disp_set_layer_en(DISPLAY_LAYER, 0);
		if (dvr_dev->itu656in.tvout)
			ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_vin_disable_write();
		mod_timer(&dvr_dev->timer, jiffies +  msecs_to_jiffies(50));
	}	
	
	if (!dvr_dev->work_status) goto end; 
	if (dvr_dev->system == PAL)
		field = !field;
	
	//printk(KERN_ALERT "ark_itu656_int_handler--intr_stat=0x%0x\n",intr_stat);

	if (intr_stat & FIELD_INTERRUPT) {
		if(g_ark168_vin->stream_flag){
			spin_lock(&g_ark168_vin->ark_queue_lock);
			if (g_ark168_vin->cur_frm) {
				ARKVIN_DBGPRTK("v4l2 streaming vb2_buffer_done...\n");
				struct vb2_v4l2_buffer *vbuf = &g_ark168_vin->cur_frm->vb;
				struct vb2_buffer *vb = &vbuf->vb2_buf;

				vb->timestamp = ktime_get_ns();
				vbuf->sequence = g_ark168_vin->sequence++;
				vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
				g_ark168_vin->cur_frm = NULL;
			}
			if (!list_empty(&g_ark168_vin->ark_queue) && !g_ark168_vin->stop) {
				g_ark168_vin->cur_frm = list_first_entry(&g_ark168_vin->ark_queue,
							     struct vin_buffer, list);
				ARKVIN_DBGPRTK("INT->get out buf: %p\n",g_ark168_vin->cur_frm->vb.vb2_buf.planes[0].mem_priv);
				list_del(&g_ark168_vin->cur_frm->list);
				vin_setaddr(g_ark168_vin);
			}
			if (g_ark168_vin->stop)
				complete(&g_ark168_vin->comp);
			spin_unlock(&g_ark168_vin->ark_queue_lock);
		}
		if(!g_ark168_vin->stream_flag){
			//printk(KERN_ALERT"w 0x%x\n", dvr_dev->fieldbuf_phyaddr[dvr_dev->cur_buffer]);
			if(dvr_dev->interlace) {
				vin_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->fieldbuf_phyaddr[dvr_dev->cur_buffer]);
				if (dvr_dev->itu656in.tvout) {
					int tvout_addr = dvr_dev->fieldbuf_phyaddr[(dvr_dev->cur_buffer + 3) & 3];
					if (field)
						ark_disp_set_tvout_next_oddfield_bufaddr(tvout_addr);
					else
						ark_disp_set_tvout_next_evenfield_bufaddr(tvout_addr);
				}
				dvr_dev->cur_buffer = (dvr_dev->cur_buffer + 1) & 0x3;

				if (dvr_dev->video_reinit && dvr_dev->discard_frame-- == 0) {
					//for vbox
					if(dvr_dev->deinter_indirect_show == 1)
						dvr_dev->show_video = 0;
					else
						dvr_dev->show_video = 1;
					dvr_dev->video_reinit = 0;		
				}
				
				if (dvr_dev->system == PAL) deinter_type = DEINTERLACE_TYPE_PAL;
				else deinter_type = DEINTERLACE_TYPE_NTSC;
				if (field) {
					dvr_dev->deinter_odd_even = DEINTERLACE_FIELD_ODD;
					deintout_phyaddr = dvr_dev->oddbuf_phyaddr[dvr_dev->write_buffer];
				} else {
					dvr_dev->deinter_odd_even = DEINTERLACE_FIELD_EVEN;
					deintout_phyaddr = dvr_dev->evenbuf_phyaddr[dvr_dev->write_buffer];
				}
				//for vbox
				if(dvr_dev->deinter_indirect_show){
					//deintout_phyaddr = dvr_dev->framebuf_phyaddr[dvr_dev->write_framebuf];
					if(dvr_dev->deinter_odd_even == DEINTERLACE_FIELD_EVEN){
						deintout_phyaddr = dvr_dev->framebuf_phyaddr[dvr_dev->write_framebuf];
					}else{
						goto end;
					}
				}
				
				//for mirror
				if(dvr_dev->mirror_type && dvr_dev->deinter_odd_even != DEINTERLACE_FIELD_EVEN){
					goto end;
				}
				
				while(dvr_dev->deinter_status && timeout--);
				dvr_dev->deinter_status = 1;
				deinterlace_process (
	        					DEINTERLACE_LINE_SIZE_720H,
	        					DEINTERLACE_DATA_MODE_422,
	        					deinter_type,
	        					dvr_dev->deinter_odd_even,
	        					dvr_dev->fieldbuf_phyaddr[dvr_dev->cur_buffer],
	        					dvr_dev->fieldbuf_phyaddr[(dvr_dev->cur_buffer + 1) & 3],
	        					dvr_dev->fieldbuf_phyaddr[(dvr_dev->cur_buffer + 2) & 3],
	        					deintout_phyaddr,
	        					0,	  // for yuv420
	        					0);   // for yuv420 
			} else {
				if(dvr_dev->deinter_indirect_show){//for vbox
					dvr_dev->framebuf_status[dvr_dev->write_framebuf] |= ITU656_BUFFER_FULL_APP;
					GetPingPongNextBuf(dvr_dev->write_framebuf,ITU656_FRAME_NUM);
					if(dvr_dev->framebuf_status[dvr_dev->write_framebuf])
					{
						GetPingPongPreBuf(dvr_dev->write_framebuf, ITU656_FRAME_NUM);
						dvr_dev->framebuf_status[dvr_dev->write_framebuf] &= ~ITU656_BUFFER_FULL_APP;
					}

					vin_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->framebuf_phyaddr[dvr_dev->write_framebuf]);
					
					dvr_dev->frame_finish_count = 0;
					for(i = 0;i < ITU656_FRAME_NUM;i++){
						if(dvr_dev->framebuf_status[i] == ITU656_BUFFER_FULL_APP)
							dvr_dev->frame_finish_count++;
					}
					
					wake_up_interruptible(&dvr_dev->frame_finish_waitq);		
					if(dvr_dev->fasync_queue != NULL) {
						kill_fasync(&dvr_dev->fasync_queue, SIGIO, POLL_IN);
					}
					
				}else{
					dvr_dev->buf_status[dvr_dev->write_buffer] |= ITU656_BUFFER_FULL_LCD;
					GetPingPongNextBuf(dvr_dev->write_buffer,ITU656_BUFFER_NUM);
					if(dvr_dev->buf_status[dvr_dev->write_buffer])
					{
						GetPingPongPreBuf(dvr_dev->write_buffer, ITU656_BUFFER_NUM);
						dvr_dev->buf_status[dvr_dev->write_buffer] &= ~ITU656_BUFFER_FULL_LCD;
					}
					vin_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->oddbuf_phyaddr[dvr_dev->write_buffer]);
				}
				
				if (dvr_dev->video_reinit && dvr_dev->discard_frame-- == 0) {
					if(dvr_dev->deinter_indirect_show == 1)
						dvr_dev->show_video = 0;
					else 
						dvr_dev->show_video = 1;
					dvr_dev->video_reinit = 0;
				}
			}
		}

end:
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
}
   	return IRQ_HANDLED;
}

int detect_cvbs_mode(void)
{
	unsigned int pnline;
	unsigned int pnpixel;
	int ntsc = 0;
		
	pnline	= vin_readl(ARK1668_ITU656_LINE_NUM_PER_FIELD) & 0x7FE;
	pnpixel = (vin_readl(ARK1668_ITU656_PIX_NUM_PER_LINE) & 0xFFE) >> 1;
		
	if ((pnline > 230) && (pnline < 250)) {
		ntsc = 1;
	} else if((pnline > 278) && (pnline < 298)) {
		ntsc = 0;
	} else {
		ntsc = 1;
	}
		
	return ntsc;
}

static void ark_vin_pad_select(struct dvr_dev *dvr_dev)
{
	unsigned int val;

	if (dvr_dev->itu601en) {
		//hsync, vsync
		val = (1 << 19) | (1 << 18);
        vin_writel_sys(ARK1668_SYS_PAD_CTRL07, vin_readl_sys(ARK1668_SYS_PAD_CTRL07)|val);
	}
	
	if (dvr_dev->itu_channel== ITU656_CH0) {
		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0A);
		val &= ~(0xF<<4);
		vin_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);	            

		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL07);
		val &= ~(0x1FFFF<<0);
		val |= 0x15555;
		vin_writel_sys(ARK1668_SYS_PAD_CTRL07, val);
	} else if (dvr_dev->itu_channel == ITU656_CH1) {
		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0A);
		val &= ~(0xF<<4);
		val |= 5<<4;
		vin_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);	
    
		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0B);
		val |= (0x1FF<<16);
		vin_writel_sys(ARK1668_SYS_PAD_CTRL0B, val);	
	} else if(dvr_dev->itu_channel == ITU656_CH2){
		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0A);
		val &= ~(0xF<<4);
		val |= 0xA<<4;
		vin_writel_sys(ARK1668_SYS_PAD_CTRL0A, val);	
    
		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0B);
		val |= (0x7F<<25);
		vin_writel_sys(ARK1668_SYS_PAD_CTRL0B, val);

		val = vin_readl_sys(ARK1668_SYS_PAD_CTRL0C);
		val |= (0x3<<0);
		vin_writel_sys(ARK1668_SYS_PAD_CTRL0C, val);
	}   	
}


static void ark_vin_reg_init(struct dvr_dev *dvr_dev)
{
        unsigned int val;
	
	// Clock On
	vin_writel_sys(ARK1668_SYS_PER_CLK_EN, vin_readl_sys(ARK1668_SYS_PER_CLK_EN)|1 << 12);
	vin_writel_sys(ARK1668_SYS_AXI_CLK_EN, vin_readl_sys(ARK1668_SYS_AXI_CLK_EN)|1 << 2);
	vin_writel_sys(ARK1668_SYS_AHB_CLK_EN, vin_readl_sys(ARK1668_SYS_AHB_CLK_EN)|1 << 10);

	//soft reset
	vin_writel_sys(ARK1668_SYS_SOFT_RSTNA, vin_readl_sys(ARK1668_SYS_SOFT_RSTNA)& ~(1 << 9));
	msleep(1);
	vin_writel_sys(ARK1668_SYS_SOFT_RSTNA, vin_readl_sys(ARK1668_SYS_SOFT_RSTNA)| (1 << 9));

	vin_writel(ARK1668_ITU656_MODULE_EN, 1<<2);
	if (dvr_dev->itu601en) {
              vin_writel(ARK1668_ITU656_MODULE_EN, vin_readl(ARK1668_ITU656_MODULE_EN)| 1 );
		vin_writel(ARK1668_ITU656_INPUT_SEL, 0);
                //for vbox r601
                if(g_ark168_vin->pdata.g_arkvin_priv->ic_type == IC_TYPE_UB934){
            		val = (0 << 13) | (0 << 12) | (1 << 4)| (1 << 3) | (1 << 2);
                }else{
            		val = (1 << 13) | (1 << 12) | (1 << 4);
                }
                vin_writel(ARK1668_ITU656_SEP_MODE_SEL, val);

	}else{
		vin_writel(ARK1668_ITU656_INPUT_SEL, 0x01);
        }

	vin_writel(ARK1668_ITU656_ICR, 0xff);

	val	= (10<<16) /*debounce*/
		| (DELTA_LINE<<8) 
		| 0xFF/*DELTA_PIX*/;

        vin_writel(ARK1668_ITU656_PIX_LINE_NUM_DELTA, val);

	vin_writel(ARK1668_ITU656_DRAM_DEST1, dvr_dev->buffer_phyaddr);
	vin_writel(ARK1668_ITU656_DRAM_DEST2, dvr_dev->buffer_phyaddr);		

	if (dvr_dev->interlace) {	
#ifdef ITU656_USE_DEINTERLACE
		vin_writel(ARK1668_ITU656_IMR, FIELD_INTERRUPT);
		val = WRITE_MEMORY_TWO_FIELD
        		|CBCR_YUYV
        		|H_FILTER_COEF_AUTO
        		|YCBCR444_422_FILTER_ENABLE
        		|CB_FIRST
        		|GLOBAL_DISABLE;
#else
		vin_writel(ARK1668_ITU656_IMR, FRAME_INTERRUPT_INTERRUPT);
		val = WRITE_MEMORY_TWO_FIELD
			|CBCR_YUYV
			|H_FILTER_COEF_AUTO
			|STORE_DATA_2FIELD_2ADDR
			|YCBCR444_422_FILTER_ENABLE
			|CB_FIRST
			|GLOBAL_DISABLE;
#endif	
                vin_writel(ARK1668_ITU656_ENABLE_REG, val);

	} else {
		vin_writel(ARK1668_ITU656_IMR, FIELD_INTERRUPT);
		val = WRITE_MEMORY_TWO_FIELD
			|CBCR_YUYV
			|H_FILTER_COEF_AUTO
			|YCBCR444_422_FILTER_ENABLE
			|CB_FIRST
			|GLOBAL_DISABLE;
                vin_writel(ARK1668_ITU656_ENABLE_REG, val);

		val	= (10<<16) /*debounce*/
			| (DELTA_LINE<<8) 
			| DELTA_PIX;
                vin_writel(ARK1668_ITU656_PIX_LINE_NUM_DELTA, val);
	}

	//for vbox
	if(dvr_dev->deinter_indirect_show){
		vin_writel(ARK1668_ITU656_ENABLE_REG, vin_readl(ARK1668_ITU656_ENABLE_REG)& ~(CBCR_YUYV));
	}
}


static void ark_vin_enable(struct dvr_dev *dvr_dev)
{
        vin_writel(ARK1668_ITU656_ENABLE_REG, vin_readl(ARK1668_ITU656_ENABLE_REG)| (1 << 0)); 
        vin_writel(ARK1668_ITU656_IMR, vin_readl(ARK1668_ITU656_IMR)| (TOTAL_LINE_CHANGED_INTERRUPT));
        if (!dvr_dev->interlace)
        vin_writel(ARK1668_ITU656_IMR, vin_readl(ARK1668_ITU656_IMR)| (ACTIVE_PIX_CHANGED_INTERRUPT));

        vin_writel_sys(ARK1668_SYS_DEVICE_CLK_CFG1, vin_readl_sys(ARK1668_SYS_DEVICE_CLK_CFG1)| 1);
}

static void deinterlace_reset(void)
{
	vin_writel_dein(ARK1668_DEINTERLACE_CTRL0, (1 << 0));
	ndelay(100);
	vin_writel_dein(ARK1668_DEINTERLACE_CTRL0, (0 << 0));
}

static void deinterlace_init(void)
{
	deinterlace_reset();	
	vin_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x3);		
	vin_writel_dein(ARK1668_DEINTERLACE_INT_MASK, 0x3);
}

static void vin_tvout_enable(struct dvr_dev *dvr_dev, int enable)
{
	struct ark_disp_tvenc_cfg_arg tvenc_cfg = {0};	

	if (enable) {
		tvenc_cfg.enable = enable;
		if (dvr_dev->system == PAL)
			tvenc_cfg.out_mode = ARKDISP_TVENC_OUT_CVBS_PAL;
		else
			tvenc_cfg.out_mode = ARKDISP_TVENC_OUT_CVBS_NTSC;
		tvenc_cfg.backcolor_y = 0x10;
		tvenc_cfg.backcolor_cb = 0x80;
		tvenc_cfg.backcolor_cr = 0x80;
		ark_disp_set_tvenc_cfg(&tvenc_cfg);	
	}
}

static void vin_config_tvout(struct dvr_dev *dvr_dev)
{
	struct arkfb_update_window tvout_arg = {0};
	int width, height;
	int out_width, out_height;

	if(dvr_dev->interlace) {
		if (dvr_dev->system == PAL) {
			width = 720;
			height = 288;
		} else {
			width = 720;
			height = 240;
		}
		out_width = width;
		out_height = height;
	} else {
		width = dvr_dev->src_width;
		height = dvr_dev->src_height;
		out_width = 720;
		out_height = 480;
	}
	
	tvout_arg.win_width = width;
	tvout_arg.win_height  = height; //cut 2 lines on the bottom
	tvout_arg.width = width;
	tvout_arg.height = height;
	tvout_arg.out_width = out_width;
	tvout_arg.out_height = out_height;
	tvout_arg.interlace_out = !dvr_dev->interlace;
	tvout_arg.format = ARK_DISP_VIDEO_PIXFMT_YUYV;
	
	ark_fb_update_window_by_layer_id(TVOUT_LAYER, &tvout_arg);	
}


static void cvbs_type_change_detect(struct dvr_dev *dvr_dev)
{
	if(dvr_dev->ic_type == IC_TYPE_TP2825B)
	{
		if(dvr_dev->system == NTSC)
		{
			if(dvr_dev->old_cvbs_type != NTSC)
			{
                		vin_writel(ARK1668_ITU656_ENABLE_REG,vin_readl(ARK1668_ITU656_ENABLE_REG) & ~(2<<5));
				dvr_dev->old_cvbs_type = NTSC;
			}
		}
		else
		{
			if(dvr_dev->old_cvbs_type != PAL)
			{
                		vin_writel(ARK1668_ITU656_ENABLE_REG,vin_readl(ARK1668_ITU656_ENABLE_REG) | (2<<5));
				dvr_dev->old_cvbs_type = PAL;
			}
		}
	}
}


static void dither_timeout_timer(struct timer_list *t)
{
	struct dvr_dev *dvr_dev = from_timer(dvr_dev, t, timer);
	struct arkfb_update_window display_arg = {0};
	int width, height;
	int line,pixel;
	int activeLine, activePix;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	if (!dvr_dev->work_status) {
		spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
		return;
	}

	line = vin_readl(ARK1668_ITU656_LINE_NUM_PER_FIELD) & 0x7FE;
	if (dvr_dev->itu601en)
		pixel = vin_readl(ARK1668_ITU656_PIX_NUM_PER_LINE) & 0xFFF;
	else
		pixel = (vin_readl(ARK1668_ITU656_PIX_NUM_PER_LINE) & 0xFFE) >> 1;	

	dvr_dev->src_width = activePix = pixel;
	dvr_dev->src_height = activeLine = line;

	if (dvr_dev->interlace) {
		if (line >= 230 && line <= 250)
			dvr_dev->system = NTSC;
		else if (line >= 278 && line < 298)
			dvr_dev->system = PAL;
		else
			dvr_dev->system = NTSC;

		if(dvr_dev->system == NTSC)
		{
			activeLine = 240;
			activePix = 720;
		}
		else
		{
			activeLine = 288;
			activePix = 720;
		}
		cvbs_type_change_detect(dvr_dev);
	}

	vin_writel(ARK1668_ITU656_OUTLINE_NUM_PER_FIELD, activeLine);
	vin_writel(ARK1668_ITU656_DATA_OUT_NUM, activeLine*activePix);
	vin_writel(ARK1668_ITU656_SIZE, activePix<<16);
	

	width = activePix;
	height = activeLine;
	if (dvr_dev->interlace) {
#ifdef 	ITU656_USE_DEINTERLACE
		height *= 2;
#endif
	}

	if(!g_ark168_vin->stream_flag){
		if (dvr_dev->itu601en) {
			display_arg.win_x = dvr_dev->itu656in.left_blank + 2;
			display_arg.win_y = dvr_dev->itu656in.top_blank + 2;
			display_arg.win_width = width - dvr_dev->itu656in.left_blank - 
									dvr_dev->itu656in.right_blank-2;
	}else{
		display_arg.win_x = dvr_dev->itu656in.left_blank;
		display_arg.win_y = dvr_dev->itu656in.top_blank + 2;
		display_arg.win_width = width - dvr_dev->itu656in.left_blank - 
			                    dvr_dev->itu656in.right_blank;
		}
		if (dvr_dev->interlace) {
#ifdef 	ITU656_USE_DEINTERLACE	
		display_arg.win_width -= 12;
#endif
	}
		display_arg.win_height  = height - dvr_dev->itu656in.top_blank - 
		                          dvr_dev->itu656in.bottom_blank - 4;
		display_arg.width = width;
		display_arg.height = height;
		display_arg.out_width = dvr_dev->itu656in.width;
		display_arg.out_height = dvr_dev->itu656in.height;
		display_arg.out_x = dvr_dev->itu656in.xpos;
		display_arg.out_y = dvr_dev->itu656in.ypos;
		display_arg.format = ARK_DISP_VIDEO_PIXFMT_YUYV;

		if (!dvr_dev->deinter_indirect_show){
			//ark_fb_update_window_by_layer_id(DISPLAY_LAYER, &display_arg);
	                ark_itu656_display_init(width,height,0,0,display_arg.out_width,display_arg.out_height,1);
	        }
}
	
	if (dvr_dev->itu656in.tvout) {
		ark_disp_set_gui_tvout(0);
		vin_config_tvout(dvr_dev);
		vin_tvout_enable(dvr_dev, 1);
	}

	dvr_dev->discard_frame = DISCARD_FRAME_SYS_CHANGE;
	if (dvr_dev->interlace) {
#ifdef 	ITU656_USE_DEINTERLACE
		dvr_dev->discard_frame = DISCARD_FRAME_SYS_CHANGE * 2;		
#endif
	}
	
	dvr_dev->video_reinit = 1;
	dvr_dev->show_video = 0;
	dvr_dev->display_odd_even = 0;
	for (i = 0; i < ITU656_BUFFER_NUM; i++)
		dvr_dev->buf_status[i] = 0;
	dvr_dev->write_buffer = 0;
	dvr_dev->display_buffer = ITU656_BUFFER_NUM - 1;

	for (i = 0; i < ITU656_FRAME_NUM; i++)
		dvr_dev->framebuf_status[i] = 0;
	dvr_dev->write_framebuf = 0;////
	dvr_dev->get_framebuf = 0;
	ark_vin_enable_write();	
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
}

static void vin_start(struct dvr_dev *dvr_dev)
{ 
	if(!dvr_dev->work_status){
		deinterlace_init();
		if(g_ark168_vin->pdata.g_arkvin_priv->select_channel)
			g_ark168_vin->pdata.g_arkvin_priv->select_channel(dvr_dev->channel);
		ark_vin_pad_select(dvr_dev);
		ark_vin_reg_init(dvr_dev);
		ark_vin_enable(dvr_dev);
		dvr_dev->work_status = 1;
		dvr_dev->old_cvbs_type = TYPE_UNKNOWN;
	}
}

static void vin_stop(struct dvr_dev *dvr_dev)
{
	dvr_dev->work_status = 0;
	del_timer(&dvr_dev->timer);
	dvr_dev->show_video = 0;
	ark_disp_set_tvout_next_oddfield_bufaddr(0);
	ark_disp_set_tvout_next_evenfield_bufaddr(0);
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	ark_vin_disable_write(); /*stop write data back*/
	ark_vin_disable();
	if (dvr_dev->itu656in.tvout) {
		ark_disp_set_layer_en(TVOUT_LAYER, 0);	
		ark_disp_set_gui_tvout(1);
	}	
	msleep(100);
	ark_vin_reg_uninit();
}

static void vin_init(struct ark1668_vin_device *vin, struct vin_para *para)
{
	struct dvr_dev* dvr_dev = vin->dvr_dev;
	int i;
	memcpy(&dvr_dev->itu656in, para, sizeof(struct vin_para));
	dvr_dev->interlace = !dvr_dev->itu656in.progressive;
	dvr_dev->itu601en = dvr_dev->itu656in.itu601en;	
	if(!g_ark168_vin->stream_flag){	
		if (dvr_dev->interlace) {
			for(i=0; i<ITU656_BUFFER_NUM; i++)
			{
				dvr_dev->oddbuf_phyaddr[i]  = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*2*i;
				dvr_dev->evenbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*(2*i+1);
				//for mirror 
				dvr_dev->oddbuf_virtaddr[i] = (unsigned int)dvr_dev->buffer_virtaddr + ITU656_FRAME_SIZE*2*i;
				dvr_dev->evenbuf_virtaddr[i]= (unsigned int)dvr_dev->buffer_virtaddr + ITU656_FRAME_SIZE*(2*i+1);
			}		
			for(i=0; i<4; i++)
				dvr_dev->fieldbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*6 + ITU656_FIELD_SIZE*i;
			
			//for vbox
			for(i=0; i<ITU656_FRAME_NUM; i++){
				dvr_dev->framebuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*i;
			}
			
		} else {
			for(i=0; i<ITU656_BUFFER_NUM; i++)
			{
				if(vin->pdata.g_arkvin_priv->support_max_resolution == TYPE_1080P)
					dvr_dev->oddbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE_1080P*i;
				else
					dvr_dev->oddbuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE*i;		
			}
			
			//for vbox
			for(i=0; i<ITU656_FRAME_NUM; i++)
				dvr_dev->framebuf_phyaddr[i] = dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE*i;
		}
		}
switch (para->source) {
case DVR_SOURCE_DVD:
	dvr_dev->channel = ARK7116_AV2;
	break;
case DVR_SOURCE_AUX:
	dvr_dev->channel = ARK7116_AV1;
	break;
case DVR_SOURCE_CAMERA:
default:
	dvr_dev->channel = ARK7116_AV0;
	break;
	}
}

static irqreturn_t ark_deinterlace_int_handler(int irq, void *dev_id)
{
	u32 raw_int,i;
	unsigned long flags;
	struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	static int last_mirror_type;

	raw_int = vin_readl_dein(ARK1668_DEINTERLACE_RAW_INT);
	
	spin_lock_irqsave(&dvr_dev->spin_lock, flags);
	
	if(raw_int & (1 << 0 )){
		vin_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x1);
	}else if(raw_int & (1 << 1)){
		vin_writel_dein(ARK1668_DEINTERLACE_INT_CLEAR, 0x2);   //error
		deinterlace_reset();
		printk("deinterlace axi error\n");
	}
	
	if(last_mirror_type != dvr_dev->mirror_type && dvr_dev->interlace){
		if (dvr_dev->mirror_type == MIRROR_TYPE_NONE){
			dvr_dev->write_buffer = 0;
			dvr_dev->display_buffer = 0;
			for (i = 0; i < ITU656_BUFFER_NUM; i++)
				dvr_dev->buf_status[i] = 0;
			vin_writel_lcd(ARK1668_LCDC_VIDEO2_CTL, vin_readl_lcd(ARK1668_LCDC_VIDEO2_CTL)& ~(0x03<<17));
			last_mirror_type = MIRROR_TYPE_NONE;
			printk("mirror_type=0, reset.\n");
			goto end;
		}
	}
	
	last_mirror_type = dvr_dev->mirror_type;
	if(dvr_dev->mirror_type && dvr_dev->interlace){//for mirror
		if (dvr_dev->deinter_odd_even == DEINTERLACE_FIELD_EVEN) {
			dvr_dev->buf_status[dvr_dev->write_buffer] |= ITU656_BUFFER_FULL_MIRROR;
			GetPingPongNextBuf(dvr_dev->write_buffer,ITU656_BUFFER_NUM);
			if(dvr_dev->buf_status[dvr_dev->write_buffer]&ITU656_BUFFER_FULL_MIRROR)
			{
				GetPingPongPreBuf(dvr_dev->write_buffer, ITU656_BUFFER_NUM);
				dvr_dev->buf_status[dvr_dev->write_buffer] &= ~ITU656_BUFFER_FULL_MIRROR;
			}
		}
		
		queue_work(dvr_dev->mirror_queue, &dvr_dev->mirror_work);
			
	}
	else if(dvr_dev->deinter_indirect_show){//for vbox
		
		dvr_dev->framebuf_status[dvr_dev->write_framebuf] |= ITU656_BUFFER_FULL_APP;
		GetPingPongNextBuf(dvr_dev->write_framebuf,ITU656_FRAME_NUM);
		if(dvr_dev->framebuf_status[dvr_dev->write_framebuf])
		{
			GetPingPongPreBuf(dvr_dev->write_framebuf, ITU656_FRAME_NUM);
			dvr_dev->framebuf_status[dvr_dev->write_framebuf] &= ~ITU656_BUFFER_FULL_APP;
		}
		
		dvr_dev->frame_finish_count = 0;
		for(i = 0;i < ITU656_FRAME_NUM;i++){
			if(dvr_dev->framebuf_status[i] == ITU656_BUFFER_FULL_APP)
				dvr_dev->frame_finish_count++;
		}
		
		wake_up_interruptible(&dvr_dev->frame_finish_waitq);		
		if(dvr_dev->fasync_queue != NULL) {
			//printk(KERN_ALERT "kill_fasync vindeo frame finish.\n");
			kill_fasync(&dvr_dev->fasync_queue, SIGIO, POLL_IN);
		}
	}
	else{//normal
		
		if (dvr_dev->deinter_odd_even == DEINTERLACE_FIELD_EVEN) {
			dvr_dev->buf_status[dvr_dev->write_buffer] |= ITU656_BUFFER_FULL_LCD;
			GetPingPongNextBuf(dvr_dev->write_buffer,ITU656_BUFFER_NUM);
			if(dvr_dev->buf_status[dvr_dev->write_buffer])
			{
				GetPingPongPreBuf(dvr_dev->write_buffer, ITU656_BUFFER_NUM);
				dvr_dev->buf_status[dvr_dev->write_buffer] &= ~ITU656_BUFFER_FULL_LCD;
			}
            dvr_dev->buf_status[dvr_dev->display_buffer] &= ~ITU656_BUFFER_FULL_LCD;
            
		}

	}
	
end:
	dvr_dev->deinter_status = 0;	
	spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);

	return IRQ_HANDLED;
}


static int vin_async_bound(struct v4l2_async_notifier *notifier,
			    struct v4l2_subdev *subdev,
			    struct v4l2_async_subdev *asd)
{
	struct ark1668_vin_device *ark_vin = container_of(notifier->v4l2_dev,
					      struct ark1668_vin_device, v4l2_dev);
	struct vin_subdev_entity *subdev_entity =
		container_of(notifier, struct vin_subdev_entity, notifier);

	if (video_is_registered(&ark_vin->video_dev)) {
		v4l2_err(&ark_vin->v4l2_dev, "only supports one sub-device.\n");
		return -EBUSY;
	}

	subdev_entity->sd = subdev;
	return 0;
}

static void vin_async_unbind(struct v4l2_async_notifier *notifier,
			      struct v4l2_subdev *subdev,
			      struct v4l2_async_subdev *asd)
{
	ARKVIN_DBGPRTK("vin_async_unbind ......\n" );
	struct ark1668_vin_device *ark_vin = container_of(notifier->v4l2_dev,
					      struct ark1668_vin_device, v4l2_dev);
	cancel_work_sync(&ark_vin->awb_work);
	video_unregister_device(&ark_vin->video_dev);
}

static int vin_set_default_fmt(struct ark1668_vin_device *vin)
{
	struct v4l2_format f = {
		.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
		.fmt.pix = {
			.width		= DISPLAY_WIDTH,
			.height		= DISPLAY_HEIGHT,
			.field		= V4L2_FIELD_ANY,
			.pixelformat	= V4L2_PIX_FMT_YUYV,
		},
	};

	vin->fmt = f;
	return 0;
}

static int vin_async_complete(struct v4l2_async_notifier *notifier)
{
	struct ark1668_vin_device *ark_vin = container_of(notifier->v4l2_dev,struct ark1668_vin_device, v4l2_dev);
	ark_vin->current_subdev = container_of(notifier,struct vin_subdev_entity, notifier);
	struct video_device *vdev = &ark_vin->video_dev;
	struct v4l2_subdev	 *sd = ark_vin->current_subdev->sd;
	struct vb2_queue *q = &ark_vin->vb2_vidq;
	struct ark_subdev_data *ark_data = v4l2_get_subdevdata(sd);
	int ret;
	
	/* Register subdev device node */
	ret = v4l2_device_register_subdev_nodes(&ark_vin->v4l2_dev);
	if (ret < 0) {
		printk(KERN_ALERT "v4l2_device_register_subdev_nodes  error\n");
		return ret;
	}

	if(!ark_data->g_arkvin_priv || !ark_data->g_arkvin_client){
		printk(KERN_ALERT "get ark_data error .\n");
		return 	-EINVAL;
	}
	/* Get a method from a child device's private data */
	g_ark168_vin->pdata = *ark_data;
	ARKVIN_DBGPRTK("subdev addr is %p \n",sd);

	mutex_init(&ark_vin->lock);
	init_completion(&ark_vin->comp);

	/* Initialize videobuf2 queue */
	q->type			= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes		= VB2_MMAP | VB2_DMABUF | VB2_READ;
	q->drv_priv		= ark_vin;
	q->buf_struct_size	= sizeof(struct vin_buffer);
	q->ops			= &vin_vb2_ops;
	q->mem_ops		= &vb2_dma_contig_memops;
	q->timestamp_flags	= V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock			= &ark_vin->lock;
	q->min_buffers_needed	= 1;
	q->dev			= ark_vin->dev;

	ret = vb2_queue_init(q);
	if (ret < 0) {
		v4l2_err(&ark_vin->v4l2_dev,
			 "vb2_queue_init() failed: %d\n", ret);
		return ret;
	}

	/* Init video dma queues */
	INIT_LIST_HEAD(&ark_vin->ark_queue);
	spin_lock_init(&ark_vin->ark_queue_lock);

	ret = vin_set_default_fmt(ark_vin);
	if (ret) {
		v4l2_err(&ark_vin->v4l2_dev, "Could not set default format\n");
		return ret;
	}

	/* Register video device */
	strlcpy(vdev->name, ARK_VIN_NAME, sizeof(vdev->name));
	vdev->release		= video_device_release_empty;
	vdev->fops		= &vin_fops;
	vdev->ioctl_ops		= &vin_ioctl_ops;
	vdev->v4l2_dev		= &ark_vin->v4l2_dev;
	vdev->vfl_dir		= VFL_DIR_RX;
	vdev->queue		= q;
	vdev->lock		= &ark_vin->lock;
	vdev->device_caps	= V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE;
	video_set_drvdata(vdev, ark_vin);

	ret = video_register_device(vdev, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		v4l2_err(&ark_vin->v4l2_dev,
			 "video_register_device failed: %d\n", ret);
		return ret;
	}
	return 0;
}

static void vin_driver_init(struct device *dev)
{
	g_ark168_vin->dvr_dev = devm_kzalloc(dev, sizeof(struct dvr_dev), GFP_KERNEL);
       if (g_ark168_vin->dvr_dev == NULL) {
        	dev_err(dev, "%s %d: failed to allocate memory\n",
        	    __FUNCTION__, __LINE__);
        	return;
       }

	g_ark168_vin->dvr_dev->work_status = 0;
	g_ark168_vin->dvr_dev->deinter_status = 0;
	g_ark168_vin->dvr_dev->start = vin_start;
	g_ark168_vin->dvr_dev->stop = vin_stop;
	g_ark168_vin->dvr_dev->fasync_queue = NULL;
	g_ark168_vin->dvr_dev->system = NTSC;
	g_ark168_vin->dvr_dev->cur_buffer = 0;
	g_ark168_vin->dvr_dev->write_buffer = 0;
	g_ark168_vin->dvr_dev->display_buffer = 0;
	g_ark168_vin->dvr_dev->carback_signal = 0;
	g_ark168_vin->dvr_dev->ic_type = g_ark168_vin->pdata.g_arkvin_priv->ic_type;
	g_ark168_vin->dvr_dev->old_cvbs_type = TYPE_UNKNOWN;
	//for vbox
	g_ark168_vin->dvr_dev->write_framebuf = 0;
	g_ark168_vin->dvr_dev->get_framebuf = 0;
	g_ark168_vin->dvr_dev->deinter_indirect_show = 0;
	g_ark168_vin->dvr_dev->frame_finish_count = 0;
	g_ark168_vin->stream_flag = false;
	init_waitqueue_head(&g_ark168_vin->dvr_dev->frame_finish_waitq);

	spin_lock_init(&g_ark168_vin->dvr_dev->spin_lock);
}


static int vin_parse_dt(struct device *dev, struct ark1668_vin_device *ark_vin)
{
	struct device_node *np = dev->of_node;
	struct device_node *epn = NULL, *rem;
	struct v4l2_fwnode_endpoint v4l2_epn;
	struct vin_subdev_entity *subdev_entity;
	unsigned int flags;
	int ret;

	INIT_LIST_HEAD(&ark_vin->subdev_entities);
	while (1) {
		epn = of_graph_get_next_endpoint(np, epn);
		if (!epn)
			return 0;

		rem = of_graph_get_remote_port_parent(epn);
		if (!rem) {
			dev_notice(dev, "Remote device at %pOF not found\n",
				   epn);
			continue;
		}
		ret = v4l2_fwnode_endpoint_parse(of_fwnode_handle(epn),
						 &v4l2_epn);
		if (ret) {
			of_node_put(rem);
			ret = -EINVAL;
			dev_err(dev, "Could not parse the endpoint\n");
			break;
		}

		subdev_entity = devm_kzalloc(dev,
					  sizeof(*subdev_entity), GFP_KERNEL);
		if (!subdev_entity) {
			of_node_put(rem);
			ret = -ENOMEM;
			break;
		}

		subdev_entity->asd = devm_kzalloc(dev,
				     sizeof(*subdev_entity->asd), GFP_KERNEL);
		if (!subdev_entity->asd) {
			of_node_put(rem);
			ret = -ENOMEM;
			break;
		}

		flags = v4l2_epn.bus.parallel.flags;

		subdev_entity->asd->match_type = V4L2_ASYNC_MATCH_FWNODE;
		subdev_entity->asd->match.fwnode = of_fwnode_handle(rem);
		/* Adds an instance to the linked list*/
		list_add_tail(&subdev_entity->list, &ark_vin->subdev_entities);
	}

	of_node_put(epn);
	return ret;
}

static void vin_subdev_cleanup(struct ark1668_vin_device *vin)
{
	struct vin_subdev_entity *subdev_entity;

	list_for_each_entry(subdev_entity, &vin->subdev_entities, list)
		v4l2_async_notifier_unregister(&subdev_entity->notifier);

	INIT_LIST_HEAD(&vin->subdev_entities);
}


static int ark1668_vin_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct vin_subdev_entity *subdev_entity;
	struct ark1668_vin_device* ark_vin;
	struct resource *res;
    void __iomem *regs;
	int ret,value;

	ark_vin = devm_kzalloc(dev, sizeof(*ark_vin), GFP_KERNEL);
	if (!ark_vin)
		return -ENOMEM;

	g_ark168_vin = ark_vin;

	platform_set_drvdata(pdev, ark_vin);
	ark_vin->dev = dev;

	ret = v4l2_device_register(dev,&ark_vin->v4l2_dev);
	if (ret) {
		printk(KERN_ALERT "unable to register v4l2 device.\n");
	}

	ret = vin_parse_dt(dev, ark_vin);
	if (ret)
		printk(KERN_ALERT "fail to parse device tree\n");

	if (list_empty(&ark_vin->subdev_entities)) {
		printk(KERN_ALERT "no subdev found \n");
		ret = -ENODEV;
	}
	/*find and prepare the async subdev notifier and register it */
	{
		static const struct v4l2_async_notifier_operations vin_async_ops = {
			.bound    = vin_async_bound,
			.unbind   = vin_async_unbind,
			.complete = vin_async_complete,
		};
	list_for_each_entry(subdev_entity, &ark_vin->subdev_entities, list) {
		subdev_entity->notifier.subdevs = &subdev_entity->asd;
		subdev_entity->notifier.num_subdevs = 1;
		subdev_entity->notifier.ops = &vin_async_ops;

		ret = v4l2_async_notifier_register(&ark_vin->v4l2_dev,
						   &subdev_entity->notifier);
		if (ret) {
			printk(KERN_ALERT "fail to register async notifier\n");
		}

		if (video_is_registered(&ark_vin->video_dev))
			break;
	}
	}
	vin_driver_init(dev);
	ARKVIN_DBGPRTK("g_ark168_vin->pdata.g_arkvin_client->name = %s.\n",g_ark168_vin->pdata.g_arkvin_client->name);
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
	}
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
	}
	ark_vin->dvr_dev->context.itu656_base = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
	}
	regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
	}
	ark_vin->dvr_dev->context.sys_base = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
	}
	regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
	}
	ark_vin->dvr_dev->context.deinterlace_base = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	if (IS_ERR(res)) {
		ret = PTR_ERR(res);
	}
	regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
	if (IS_ERR(regs)) {
		ret = PTR_ERR(regs);
	}
	ark_vin->dvr_dev->context.lcd_base = regs;


	ark_vin->dvr_dev->context.itu656_irq = platform_get_irq(pdev, 0);
	if (ark_vin->dvr_dev->context.itu656_irq < 0) {
		dev_err(&pdev->dev, "%s %d: can't get itu656_irq resource.\n", __FUNCTION__, __LINE__);
	}
	ret = devm_request_irq(
			&pdev->dev,
			ark_vin->dvr_dev->context.itu656_irq, 
			ark_vin_int_handler, 
			IRQF_SHARED, 
			"ark168_vin", 
			ark_vin->dvr_dev
			);
	if(ret){
	dev_err(&pdev->dev, "%s %d: can't get assigned vin %d, error %d\n",
	    __FUNCTION__, __LINE__, ark_vin->dvr_dev->context.itu656_irq, ret);
	}

	ark_vin->dvr_dev->context.deinterlace_irq = platform_get_irq(pdev, 1);
	if (ark_vin->dvr_dev->context.deinterlace_irq < 0) {
		dev_err(&pdev->dev, "%s %d: can't get deinterlace_irq resource.\n", __FUNCTION__, __LINE__);
	}
	ret = devm_request_irq(
				&pdev->dev,
				ark_vin->dvr_dev->context.deinterlace_irq, 
				ark_deinterlace_int_handler, 
				IRQF_SHARED, 
				"dvr_deinterlace", 
				ark_vin->dvr_dev
				);
	if(ret){
	dev_err(&pdev->dev, "%s %d: can't get assigned deinterlace_irq %d, error %d\n",
	    __FUNCTION__, __LINE__, ark_vin->dvr_dev->context.deinterlace_irq, ret);
	}
	
	if(ark_vin->pdata.g_arkvin_priv->support_max_resolution == TYPE_1080P)
	        ark_vin->dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE_1080P*ITU656_BUFFER_NUM;
	else if(ark_vin->pdata.g_arkvin_priv->support_max_resolution == TYPE_720P)
	        ark_vin->dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE*ITU656_FRAME_NUM;
	else
	        ark_vin->dvr_dev->buffer_size = ITU656_FIELD_SIZE*16;

	timer_setup(&ark_vin->dvr_dev->timer, dither_timeout_timer, 0);	
	ark_vin->dvr_dev->mirror_queue = create_singlethread_workqueue("mirror_queue");
	if(!ark_vin->dvr_dev->mirror_queue) {
		printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);	
		return -1;
	}

	INIT_WORK(&ark_vin->dvr_dev->mirror_work, mirror_paint_work);

	ark_vin->dvr_dev->channel = ITU656_CH1;
	if(!of_property_read_u32(pdev->dev.of_node, "channel", &value)) {
	//printk(KERN_ALERT "get itu channel=%d\n", value);
	        if(value >= ITU656_CH0 && value <= ITU656_CH2)
	                ark_vin->dvr_dev->itu_channel = value;

	}

	ark_vin->dvr_dev->mirror_type = MIRROR_TYPE_NONE;
	if(!of_property_read_u32(pdev->dev.of_node, "mirror", &value)) {
	        printk("get mirror type=%d\n", value);
	        if(value >= MIRROR_TYPE_NONE && value < MIRROR_TYPE_END)
	                ark_vin->dvr_dev->mirror_type = value;

	}

	carback_first_enter();
	return 0;
}

static int ark1668_vin_remove(struct platform_device *pdev)
{
	struct ark1668_vin_device *ark_vin = platform_get_drvdata(pdev);
	struct dvr_dev *dvr_dev = ark_vin->dvr_dev;
    iounmap(dvr_dev->context.lcd_base);
    iounmap(dvr_dev->context.deinterlace_base);
    iounmap(dvr_dev->context.sys_base);
	unregister_chrdev_region(MKDEV(dvr_dev->dev_major, dvr_dev->dev_minor), 1);		
	g_ark168_vin = NULL;
	vin_subdev_cleanup(ark_vin);
	v4l2_device_unregister(&ark_vin->v4l2_dev);
	return 0;
}

static const struct of_device_id ark1668_vin_of_match[] = {
	{ .compatible = "arkmicro,ark1668-vin", },
	{ }
};
MODULE_DEVICE_TABLE(of, ark1668_itu656_of_match);

static struct platform_driver ark1668_vin_driver = {
        .driver     = {
        .name   = "ark1668-vin",
        	.of_match_table = of_match_ptr(ark1668_vin_of_match),
        },
        .probe      = ark1668_vin_probe,
        .remove     = ark1668_vin_remove,
};

module_platform_driver(ark1668_vin_driver);

MODULE_AUTHOR("arkmicro");
MODULE_DESCRIPTION("The V4L2 driver for arkmicro");
MODULE_LICENSE("GPL v2");

