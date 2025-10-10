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
#include "ark1668e_vin.h"

extern int ark_vin_display_init(int layer,int src_width, int src_height,int out_posx, int out_posy);
extern int ark_vin_display_addr(unsigned int addr);
extern int ark_vin_get_display_addr(void);
extern int ark1668e_lcdc_wait_for_vsync(void);
extern int ark_vin_get_screen_info(int* width,int* height);
extern int ark_disp_set_layer_en(int layer_id, int enable);
extern int ark_scale_start(struct ark_scale_context *context, struct ark_scale_param *param);
int dvr_enter_carback(void);
int dvr_exit_carback(void);
static int vin_start(struct dvr_dev *dvr_dev);
static void vin_init(struct ark1668e_vin_device *vin, struct vin_para *para);
static int vin_exit(void);
static inline void vin_reset(void);
static int vin_aux_config(struct vin_para* para);
static int vin_aux_start(struct dvr_dev *dvr_dev);
static void vin_aux_exit(void);
static inline void ark_vin_disable_write(void);
static inline void ark_vin_enable_write(void);
extern unsigned int g_screen_width;
extern unsigned int g_screen_height;

struct ark1668e_vin_device* g_ark1668e_vin = NULL;
struct ark_scale_param scale_param;
extern struct ark_scale_context *scale_context;

static int vin_queue_setup(struct vb2_queue *vq,
			    unsigned int *nbuffers, unsigned int *nplanes,
			    unsigned int sizes[], struct device *alloc_devs[])
{
	struct ark1668e_vin_device* ark_vin = vb2_get_drv_priv(vq);
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
	struct ark1668e_vin_device* ark_vin = vb2_get_drv_priv(vb->vb2_queue);
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

static int vin_config(void)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret;
	vin = g_ark1668e_vin;
	memset(&vin->dvr_dev->itu656in,0,sizeof(struct vin_para));
	
	vin->dvr_dev->itu656in.source = DVR_SOURCE_CAMERA;

	if(!vin->sd_init){
	    ret = v4l2_subdev_call(vin->current_subdev->sd,core,init,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
		vin->sd_init = 1;
	}
	if(vin->dvr_dev->chip_info == TYPE_RN6752){
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_ENTER_CARBACK,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
	}
	spin_lock(&vin->dvr_dev->spin_lock);

	ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_ITU601_ENABLE,&vin->dvr_dev->itu656in.itu601en);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	
	ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_PROGRESSIVE,&vin->dvr_dev->itu656in.progressive);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	memcpy(&vin->dvr_dev->itu656in_back, &vin->dvr_dev->itu656in, sizeof(struct vin_para));
	vin_reset();
	vin_init(vin, &vin->dvr_dev->itu656in);	
	vin->dvr_dev->channel = g_ark1668e_vin->app_channel_set;
	vin_start(vin->dvr_dev);
	spin_unlock(&vin->dvr_dev->spin_lock);
	return 0;
}

static int vin_aux_config(struct vin_para* para)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret = 0;
	vin = g_ark1668e_vin;
	memset(&vin->dvr_dev->itu656in,0,sizeof(struct vin_para));
	vin->vin_status = vin->stream_flag;
	vin->stream_flag = false;
	vin->aux_flag = 1;

    if(!vin->sd_init){
	    ret = v4l2_subdev_call(vin->current_subdev->sd,core,init,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
		vin->sd_init = 1;
	}
	
	if(vin->dvr_dev->chip_info == TYPE_RN6752){
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_ENTER_CARBACK,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
	}
	spin_lock(&vin->dvr_dev->spin_lock);

	ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_ITU601_ENABLE,&vin->dvr_dev->itu656in.itu601en);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	
	ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_PROGRESSIVE,&vin->dvr_dev->itu656in.progressive);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}

	para->itu601en = vin->dvr_dev->itu656in.itu601en;
	para->progressive = vin->dvr_dev->itu656in.progressive;
	vin->dvr_dev->itu601en = para->itu601en;
	vin->dvr_dev->interlace = !para->progressive;
	memcpy(&vin->dvr_dev->itu656in_back, para, sizeof(struct vin_para));
	vin_reset();
	vin->dvr_dev->scale_framebuf_index = 0;
	vin->dvr_dev->carback_exit_status = 0;
	spin_unlock(&vin->dvr_dev->spin_lock);
	return 0;
}

static void vin_setaddr(struct ark1668e_vin_device *vin)
{
	dma_addr_t addr_dma;
	/* Get the physical address of the assigned BUF  */
	addr_dma = vb2_dma_contig_plane_dma_addr(&vin->cur_frm->vb.vb2_buf, 0);
	//printk("vin_setaddr addr_dma  : 0x%x\n",addr_dma);
	vin_writel(ARK1668E_ITU656_DRAM_DEST1, addr_dma);
	vin_writel(ARK1668E_ITU656_DRAM_DEST1,addr_dma + vin->dvr_dev->src_width*vin->dvr_dev->src_height);
}

static inline void vin_reset(void)
{
	//soft reset
	vin_writel_sys(ARK1668E_SYS_SOFT_RSTNA, vin_readl_sys(ARK1668E_SYS_SOFT_RSTNA)& ~(1 << 9));
	msleep(1);
	vin_writel_sys(ARK1668E_SYS_SOFT_RSTNA, vin_readl_sys(ARK1668E_SYS_SOFT_RSTNA)| (1 << 9));
	//addr reset
	vin_writel(ARK1668E_ITU656_RESET,vin_readl(ARK1668E_ITU656_RESET) | (1 << 1));
	msleep(1);
	vin_writel(ARK1668E_ITU656_RESET,vin_readl(ARK1668E_ITU656_RESET) & ~(1 << 1));
}

static int vin_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct ark1668e_vin_device *vin = vb2_get_drv_priv(vq);
	vin->stream_flag = true;
	vin_config();
	//spin_lock_irqsave(&vin->ark_queue_lock, flags);
	vin->sequence = 0;
	vin->stop = false;
	reinit_completion(&vin->comp);
	vin->cur_frm = list_first_entry(&vin->ark_queue,struct vin_buffer, list);
	//printk(KERN_DEBUG "start_streaming->get out buf: %p\n",vin->cur_frm->vb.vb2_buf.planes[0].mem_priv);
	list_del(&vin->cur_frm->list);
	vin_setaddr(vin);
	//spin_unlock_irqrestore(&vin->ark_queue_lock, flags);
	
	return 0;
}

static void vin_stop_streaming(struct vb2_queue *vq)
{
	struct ark1668e_vin_device* ark_vin = vb2_get_drv_priv(vq);
	struct vin_buffer *buf;

	ark_vin->stop = true;
	ark_vin->stream_flag = false;
	vin_exit();
	
	/* Release all active buffers */
	//spin_lock_irqsave(&ark_vin->ark_queue_lock, flags);
	if (unlikely(ark_vin->cur_frm)) {
		vb2_buffer_done(&ark_vin->cur_frm->vb.vb2_buf,
				VB2_BUF_STATE_ERROR);
		ark_vin->cur_frm = NULL;
	}
	list_for_each_entry(buf, &ark_vin->ark_queue, list)
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	INIT_LIST_HEAD(&ark_vin->ark_queue);
	//spin_unlock_irqrestore(&ark_vin->ark_queue_lock, flags);
}

static void vin_buffer_queue(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vin_buffer *buf = container_of(vbuf, struct vin_buffer, vb);
	struct ark1668e_vin_device *vin = vb2_get_drv_priv(vb->vb2_queue);
	dma_addr_t addr_dma;
	addr_dma = vb2_dma_contig_plane_dma_addr(vb, 0);
	/*app buf physical address can be obtained directly from this variable */
	vb->planes[0].m.offset = addr_dma;

	//printk(KERN_DEBUG "!vin->cur_frm = %d,list_empty(&vin->ark_queue)= %d,vb2_is_streaming(vb->vb2_queue) = %d\n",!vin->cur_frm,list_empty(&vin->ark_queue),vb2_is_streaming(vb->vb2_queue));
	
	if (!vin->cur_frm && list_empty(&vin->ark_queue) && vb2_is_streaming(vb->vb2_queue)){
		vin->cur_frm = buf;
		vin_setaddr(vin);
	}	
	else{
		/* add buf to list */
		list_add_tail(&buf->list, &vin->ark_queue);
		if(!vb2_is_streaming(vb->vb2_queue)){
			printk(KERN_DEBUG "init buff.\n");
			vin->cur_frm = buf;
			vin_setaddr(vin);
		}
	}
	
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
	struct ark1668e_vin_device *vin = video_drvdata(file);
	*fmt = vin->fmt;
	return 0;
}


static int vin_try_fmt(struct ark1668e_vin_device *vin, struct v4l2_format *f)
{
	struct v4l2_pix_format *pixfmt = &f->fmt.pix;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if(pixfmt->width == 0 || pixfmt->height == 0){
		printk(KERN_ALERT "pixfmt width is null\n");
		return -EINVAL;
	}
	pixfmt->field = V4L2_FIELD_ANY;
	pixfmt->bytesperline = (pixfmt->width * 16) >> 3;
	pixfmt->sizeimage = pixfmt->width * pixfmt->height * 3 / 2;
	printk(KERN_DEBUG "pixfmt->width = %d.\n",pixfmt->width);
	printk(KERN_DEBUG "pixfmt->height = %d.\n",pixfmt->height);
	printk(KERN_DEBUG "pixfmt->bytesperline = %d.\n",pixfmt->bytesperline);
	printk(KERN_DEBUG "pixfmt->sizeimage = %d.\n",pixfmt->sizeimage);

	return 0;
}


static int vin_set_fmt(struct ark1668e_vin_device *vin, struct v4l2_format *f)
{
	int ret;
	ret = vin_try_fmt(vin, f);
	if (ret)
		return ret;
	vin_reset();
	vin->fmt = *f;
	return 0;
}

static int vin_try_fmt_vid_cap(struct file *file, void *priv,struct v4l2_format *f)
{
	struct ark1668e_vin_device *ark_vin = video_drvdata(file);
	return vin_try_fmt(ark_vin, f);
}

static int vin_s_fmt_vid_cap(struct file *file, void *priv,struct v4l2_format *f)
{
	struct ark1668e_vin_device *ark_vin = video_drvdata(file);

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
	struct ark1668e_vin_device* vin = NULL;
	vin = g_ark1668e_vin;
	*i = vin->dvr_dev->carback_signal;
	return vin->dvr_dev->carback_signal;
}

static int vin_s_input(struct file *file, void *priv, unsigned int i)
{
	g_ark1668e_vin->app_channel_set = i;
	if (i > 0)
		return -EINVAL;
	return 0;
}

static long vin_ioctl_default(struct file *file, void *priv,
			       bool valid_prio, unsigned int cmd, void *param)
{
	struct dvr_dev *dvr_dev =g_ark1668e_vin->dvr_dev;
	unsigned long error = 0;
	if(!dvr_dev){
		printk(KERN_ERR"%s error null device", __func__);
		return -ENXIO;
	}
	//spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	switch (cmd)
	{
		case VIN_UPDATE_WINDOW:
		{
			struct vin_screen *vout_para  = (struct vin_screen *)param;
			if(!dvr_dev->carback_exit_status){
				dvr_dev->work_status = 0;
				ark_vin_disable_write();
				//flush_workqueue(dvr_dev->scale_queue);
				mdelay(20);
				dvr_dev->screen_xpos = vout_para->disp_x;
				dvr_dev->screen_ypos = vout_para->disp_y;
				dvr_dev->screen_width = vout_para->disp_width;
				dvr_dev->screen_height = vout_para->disp_height;
				printk(KERN_INFO "set carback win-->w:%d,h%d,x:%d,y%d\n",dvr_dev->screen_width,dvr_dev->screen_height,vout_para->disp_x,vout_para->disp_y);
				error = ark_vin_display_init(DISPLAY_LAYER,dvr_dev->screen_width,dvr_dev->screen_height,dvr_dev->screen_xpos,dvr_dev->screen_ypos);
				dvr_dev->work_status = 1;
				ark_vin_enable_write();	
				if( error < 0 )
					return error;
			}
			else{
				dvr_dev->screen_xpos = vout_para->disp_x;
				dvr_dev->screen_ypos = vout_para->disp_y;
				dvr_dev->screen_width = vout_para->disp_width;
				dvr_dev->screen_height = vout_para->disp_height;
				printk(KERN_INFO "set carback win-->w:%d,h%d,x:%d,y%d\n",dvr_dev->screen_width,dvr_dev->screen_height,vout_para->disp_x,vout_para->disp_y);
				error = ark_vin_display_init(DISPLAY_LAYER,dvr_dev->screen_width,dvr_dev->screen_height,dvr_dev->screen_xpos,dvr_dev->screen_ypos);
				if( error < 0 )
					return error;
			}
			break;
		}

		case VIN_START:
		{
			g_ark1668e_vin->aux_status = 1;
			vin_aux_start(dvr_dev);
			break;
		}

		case VIN_STOP:
		{
			vin_aux_exit();
			g_ark1668e_vin->aux_flag = 0;
			g_ark1668e_vin->aux_status = 0;
			break;
		}

		case VIN_SWITCH_CHANNEL:
		{
			int source = *((int *)param);
 
			if (source == DVR_SOURCE_AUX)
				dvr_dev->channel = ARK7116_AV1;
			else if (source == DVR_SOURCE_CAMERA)
				dvr_dev->channel = ARK7116_AV0;
			else if (source == DVR_SOURCE_DVD)
				dvr_dev->channel = ARK7116_AV2;
			else {
				dvr_dev->channel = source;
			}
			g_ark1668e_vin->app_channel_set = source;
			break;
		}

		case VIN_CONFIG:
		{
			struct vin_para *para  = (struct vin_para *)param;
			vin_aux_config(para);
			break;
		}

#ifdef CONFIG_VIDEO_USE_LOCK
		case VIN_IOCTL_DOWN_IDLE:
		{
			//printk(KERN_ALERT "++++++VIN_IOCTL_DOWN_IDLE++++++\n");
			down_interruptible(&dvr_dev->vin_sem);
			break;
		}

		case VIN_IOCTL_DOWN_TIMEOUT:
		{
			//printk(KERN_ALERT "++++++VIN_IOCTL_DOWN_TIMEOUT++++++\n");
			if (down_timeout(&dvr_dev->vin_sem,  msecs_to_jiffies(2000)) != 0) {
				error = -EINVAL;
				printk(KERN_ALERT "+++down vin_sem timeout\n");
				goto end;
			}
			break;
		}

		case VIN_IOCTL_UP_IDLE:
		{
			//printk(KERN_ALERT "++++++VIN_IOCTL_DOWN_IDLE++++++\n");
			up(&dvr_dev->vin_sem);
			break;
		}
#endif

		default:
			printk("%s: error cmd 0x%x\n", __func__, cmd);
			error = -EFAULT;
			goto end;
    }
	return 0;
	
end:
	//spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
    return error;
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
	struct ark1668e_vin_device* ark_vin = video_drvdata(file);
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
	struct ark1668e_vin_device* ark_vin = video_drvdata(file);
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

#if 0
static int deinterlace_process (unsigned int deinterlace_size,  unsigned int data_mode, unsigned int deinterlace_type,
	unsigned int deinterlace_field, unsigned int src_field_addr_0, unsigned int src_field_addr_1,unsigned int src_field_addr_2,
	unsigned int dst_y_addr, unsigned int dst_u_addr, unsigned int dst_v_addr)
{

}
#endif

static inline void ark_vin_disable_write(void)
{
        vin_writel(ARK1668E_ITU656_MODULE_EN,vin_readl(ARK1668E_ITU656_MODULE_EN) | (1 << 2));
}

static inline void ark_vin_enable_write(void)
{
        vin_writel(ARK1668E_ITU656_MODULE_EN,vin_readl(ARK1668E_ITU656_MODULE_EN) & ~(1 << 2));
}

void ark_vin_disable(void)
{
        vin_writel(ARK1668E_ITU656_IMR, 0);
        vin_writel(ARK1668E_ITU656_ENABLE_REG,vin_readl(ARK1668E_ITU656_ENABLE_REG) & ~(1 << 0));
}

void vin_alloc_buf(int type)
{
	struct ark1668e_vin_device* vin = NULL;
	int i;
	vin = g_ark1668e_vin;
	printk("vin_alloc_buf buffer_size = %d\n",vin->dvr_dev->buffer_size);
#if 0
	vin->dvr_dev->buffer_virtaddr = (void *)__get_free_pages(GFP_KERNEL, get_order(vin->dvr_dev->buffer_size));
	if (!vin->dvr_dev->buffer_virtaddr) {
	        printk(KERN_ALERT "%s get. buffer_virtaddr fail\n", __func__);
	}
	vin->dvr_dev->buffer_phyaddr = virt_to_phys(vin->dvr_dev->buffer_virtaddr);
	
	for(i = 0; i < vin->dvr_dev->framebuf_num; i++){
		if(type == TYPE_1080P){
			vin->dvr_dev->framebuf_phyaddr[i] = vin->dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE_1080P*i;
		}
		else if(type == TYPE_720P){
			vin->dvr_dev->framebuf_phyaddr[i] = vin->dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE*i;
		}
		else{
			vin->dvr_dev->framebuf_phyaddr[i] = vin->dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*i;
		}
	}
	vin->dvr_dev->scale_out_yaddr = (void *)__get_free_pages(GFP_KERNEL, get_order(vin->dvr_dev->scale_alloc_width*vin->dvr_dev->scale_alloc_height*2));
	if (!vin->dvr_dev->scale_out_yaddr) {
	        printk(KERN_ALERT "%s get. scale_out_yaddr fail\n", __func__);
	}
	vin->dvr_dev->scale_out_yphyaddr = virt_to_phys(vin->dvr_dev->scale_out_yaddr);
#else
	vin->dvr_dev->buffer_virtaddr = dma_alloc_wc(vin->dev, vin->dvr_dev->buffer_size,
					 (dma_addr_t *)&vin->dvr_dev->buffer_phyaddr,
					 GFP_KERNEL);
	if (!vin->dvr_dev->buffer_virtaddr){
		 printk(KERN_ALERT "%s dma_alloc_wc fail\n", __func__);
	}

	for(i = 0; i < vin->dvr_dev->framebuf_num; i++){
		if(type == TYPE_1080P){
			vin->dvr_dev->framebuf_phyaddr[i] = vin->dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE_1080P*i;
		}
		else if(type == TYPE_720P){
			vin->dvr_dev->framebuf_phyaddr[i] = vin->dvr_dev->buffer_phyaddr + ITU656_PROGRESSIVE_FRAME_SIZE*i;
		}
		else{
			vin->dvr_dev->framebuf_phyaddr[i] = vin->dvr_dev->buffer_phyaddr + ITU656_FRAME_SIZE*i;
		}
	}

	vin->dvr_dev->scale_out_yaddr = dma_alloc_wc(vin->dev, vin->dvr_dev->scale_buffer_size,
					 (dma_addr_t *)&vin->dvr_dev->scale_out_yphyaddr,
					 GFP_KERNEL);
	if (!vin->dvr_dev->scale_out_yaddr){
		 printk(KERN_ALERT "%s dma_alloc_wc fail\n", __func__);
	}

	for(i = 0; i < vin->dvr_dev->scale_framebuf_num; i++){
		vin->dvr_dev->scalebuf_phyaddr[i] = vin->dvr_dev->scale_out_yphyaddr + vin->dvr_dev->scale_alloc_width*vin->dvr_dev->scale_alloc_height*2*i;
	}
#endif
}

void dvr_restart(void)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret;
	vin = g_ark1668e_vin;

	if(!vin->dvr_dev->carback_exit_status)
	{
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_PROGRESSIVE,&vin->dvr_dev->itu656in.progressive);
		if(ret < 0)
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);

		vin->dvr_dev->interlace = !vin->dvr_dev->itu656in.progressive;
		ark_vin_disable_write();
		mod_timer(&vin->dvr_dev->timer, jiffies +  msecs_to_jiffies(50));
	}
}
EXPORT_SYMBOL(dvr_restart);


int dvr_enter_carback(void)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret;
	vin = g_ark1668e_vin;
	memset(&vin->dvr_dev->itu656in,0,sizeof(struct vin_para));
	if(vin->aux_flag){
		vin->aux_status = vin->aux_flag;
		vin_aux_exit();
	}
	
	vin->dvr_dev->itu656in.source = DVR_SOURCE_CAMERA;
	vin->vin_status = vin->stream_flag;
	//vin->dvr_dev->itu656in.itu601en = 1;
	vin->stream_flag = false;

	if(!vin->sd_init){
	    ret = v4l2_subdev_call(vin->current_subdev->sd,core,init,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
		vin->sd_init = 1;
	}
	if(vin->dvr_dev->chip_info == TYPE_RN6752){
		if(vin->aux_status){
			ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_SET_AVIN_MODE,0);
			if(ret < 0){
				printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
				return ret;
			}
		}
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_ENTER_CARBACK,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
	}
	spin_lock(&vin->dvr_dev->spin_lock);

	ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_ITU601_ENABLE,&vin->dvr_dev->itu656in.itu601en);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	
	ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_GET_PROGRESSIVE,&vin->dvr_dev->itu656in.progressive);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	//memcpy(&vin->dvr_dev->itu656in_back, &vin->dvr_dev->itu656in, sizeof(struct vin_para));
	vin_reset();
	vin_init(vin, &vin->dvr_dev->itu656in);	
	vin_start(vin->dvr_dev);
	vin->dvr_dev->carback_exit_status = 0;
	vin->dvr_dev->scale_framebuf_index = 0;
	spin_unlock(&vin->dvr_dev->spin_lock);
	return 0;
}
EXPORT_SYMBOL(dvr_enter_carback);

int dvr_exit_carback(void)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret;
	vin = g_ark1668e_vin;

	if(!vin->aux_status){
		vin->dvr_dev->carback_exit_status = 1;
	}
	if(vin->aux_status){
		//The track layer is closed here to solve the abnormal display problem of only one track layer when carback and AVIN switch  
		ark_disp_set_layer_en(2, 0);
	}
	vin->stream_flag = vin->vin_status;
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	spin_lock(&vin->dvr_dev->spin_lock);
	vin->dvr_dev->vin_mirror_config = MIRROR_NO;
	vin->dvr_dev->work_status = 0;
	vin->dvr_dev->scale_framebuf_index = 0;
	vin->dvr_dev->show_video = 0;
	vin->dvr_dev->carback_signal = 0;
	vin->dvr_dev->layer_status = 0;
	msleep(20);
	ark_vin_disable_write(); /*stop write data back*/
	ark_vin_disable();
	spin_unlock(&vin->dvr_dev->spin_lock);

#ifdef CONFIG_VIDEO_USE_LOCK
	up(&vin->dvr_dev->vin_sem);
#endif

	if(vin->dvr_dev->chip_info == TYPE_RN6752){
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_EXIT_CARBACK,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
		if(vin->aux_status){
			ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_SET_AVIN_MODE,0);
			if(ret < 0){
				printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
				return ret;
			}
		}
	}

	if(vin->aux_status){
		vin_aux_config(&vin->dvr_dev->itu656in_back);
		vin_aux_start(vin->dvr_dev);
	}

	return 0;
}
EXPORT_SYMBOL(dvr_exit_carback);

int dvr_set_mirror(int value)
{
	g_ark1668e_vin->dvr_dev->vin_mirror_config = value;
	return 0;
}
EXPORT_SYMBOL(dvr_set_mirror);

int dvr_get_pragressive(void)
{
	int pragressive;

	//spin_lock(&g_ark1668e_vin->dvr_dev->spin_lock);
	pragressive = !g_ark1668e_vin->dvr_dev->interlace;
	//spin_unlock(&g_ark1668e_vin->dvr_dev->spin_lock);
	
	return pragressive;
}
EXPORT_SYMBOL(dvr_get_pragressive);

static void vin_get_signal_work(struct work_struct *work)
{
	struct ark1668e_vin_device* vin = NULL;
	int signal = 0,ret;
	vin = g_ark1668e_vin;
	if(!vin->dvr_dev->carback_exit_status){
		ret = v4l2_subdev_call(vin->current_subdev->sd,video,g_input_status,&signal);
		if(ret < 0)
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);

		if(!signal){
			vin->dvr_dev->carback_signal = 1;       
		}else{
			vin->dvr_dev->carback_signal = 0;        //no signal
		}
		if (vin->dvr_dev->show_video && vin->dvr_dev->vin_buffer_status) {
			if(vin->dvr_dev->carback_signal)
			{
				if(vin->dvr_dev->first_show_flag){
					if(vin->dvr_dev->signal_flag == 20){
					printk(KERN_ALERT "ark_vin_display_int_handler-->show_video\n");
					ark_vin_display_init(DISPLAY_LAYER,vin->dvr_dev->screen_width,vin->dvr_dev->screen_height,vin->dvr_dev->screen_xpos,vin->dvr_dev->screen_ypos);
					ark_disp_set_layer_en(DISPLAY_LAYER, 1);
					vin->dvr_dev->layer_status = 1;
					vin->dvr_dev->show_video = 0;
					vin->dvr_dev->signal_flag = 0;
					vin->dvr_dev->first_show_flag = 0;
					}else{
						vin->dvr_dev->layer_status = 0;
						vin->dvr_dev->signal_flag++;
					}
				}else{
					if(vin->dvr_dev->signal_flag == 10){
						printk(KERN_ALERT "ark_vin_display_int_handler-->show_video\n");
						ark_vin_display_init(DISPLAY_LAYER,vin->dvr_dev->screen_width,vin->dvr_dev->screen_height,vin->dvr_dev->screen_xpos,vin->dvr_dev->screen_ypos);
						ark_disp_set_layer_en(DISPLAY_LAYER, 1);
						vin->dvr_dev->layer_status = 1;
						vin->dvr_dev->show_video = 0;
						vin->dvr_dev->signal_flag = 0;
					}else{
						vin->dvr_dev->layer_status = 0;
						vin->dvr_dev->signal_flag++;
					}
				}
			}
			else
			{
				vin->dvr_dev->layer_status = 0;
				printk(" No signal detect.\n");
			}
		}
	}
}

static void vin_get_signal_time(struct timer_list *t)
{
	struct ark1668e_vin_device* vin = NULL;
	vin = g_ark1668e_vin;
	queue_work(vin->dvr_dev->detect_queue, &vin->dvr_dev->detect_work);
	mod_timer(&vin->dvr_dev->signal_timer, jiffies +  msecs_to_jiffies(30));
}


int dvr_detect_carback_signal(void)
{
	struct ark1668e_vin_device* vin = NULL;
	vin = g_ark1668e_vin;
	return vin->dvr_dev->carback_signal;
}
EXPORT_SYMBOL(dvr_detect_carback_signal);

int dvr_get_layer_status(void)
{
	struct ark1668e_vin_device* vin = NULL;
	vin = g_ark1668e_vin;
	return vin->dvr_dev->layer_status;
}
EXPORT_SYMBOL(dvr_get_layer_status);

void ark_vin_scale_work(struct work_struct *work)
{
	struct ark1668e_vin_device* vin = NULL;
	int format,oformat,dstaddr;
	vin = g_ark1668e_vin;
	if (vin->dvr_dev->work_status){
		memset(&scale_param,0,sizeof(struct ark_scale_param));
		format = ARK_SCALE_FORMAT_Y_UV420;
		oformat = ARK_SCALE_OUT_FORMAT_YUYV;

		dstaddr = vin->dvr_dev->scalebuf_phyaddr[vin->dvr_dev->scale_framebuf_index];
		vin->dvr_dev->scale_framebuf_index = (vin->dvr_dev->scale_framebuf_index + 1) % ITU656_SCALE_FRAME_NUM;
		//int readaddr;
		//readaddr = ark_vin_get_display_addr();
		//if(readaddr == dstaddr){
		//	ark1668e_lcdc_wait_for_vsync();
		//}

		scale_param.iyaddr = vin->dvr_dev->scale_in_yphyaddr;
		scale_param.iuaddr = vin->dvr_dev->scale_in_yphyaddr + vin->dvr_dev->src_width * vin->dvr_dev->src_height;
		scale_param.ivaddr = 0;
		scale_param.ix = 0;
		scale_param.iy = 0;
		scale_param.iwinwidth = vin->dvr_dev->src_width;
		scale_param.iwinheight = vin->dvr_dev->src_height;
		scale_param.iwidth = vin->dvr_dev->src_width;
		scale_param.iheight = vin->dvr_dev->src_height;
		scale_param.iformat = format;
		scale_param.left_cut = 0;
		scale_param.right_cut = 25;
		scale_param.up_cut = 0;
		scale_param.bottom_cut = 15;
		scale_param.owidth = vin->dvr_dev->screen_width;
		scale_param.oheight = vin->dvr_dev->screen_height;
		scale_param.oyaddr = dstaddr;
		scale_param.ouaddr = 0;
		scale_param.ovaddr = 0;
		scale_param.oformat = oformat;
		scale_param.rotate = 0;
		ark_scale_start(scale_context,&scale_param);

		ark_vin_display_addr(dstaddr);
		vin->dvr_dev->vin_buffer_status = 1;
	}
}

int ark_vin_display_int_handler(void)
{
	struct ark1668e_vin_device* vin = NULL;
	unsigned int count = 0;
	int i,frame_id;
	vin = g_ark1668e_vin;
	if(!vin || !vin->dvr_dev || !vin->dvr_dev->work_status)
		return -1;
	if(!vin->stream_flag){
		wait_event_interruptible(vin->dvr_dev->frame_finish_waitq, vin->dvr_dev->frame_finish_count > 0);
		if(!vin->dvr_dev->carback_exit_status){
			count = min((size_t)1, (size_t)vin->dvr_dev->frame_finish_count);
			if(vin->dvr_dev->framebuf_status[vin->dvr_dev->frame_finish[0]] == FRAMEBUF_STATUS_READY)
			{
				vin->dvr_dev->framebuf_status[vin->dvr_dev->frame_finish[0]] = FRAMEBUF_STATUS_BUSY;
			}
			else
			{
				printk(KERN_ALERT "### ark1668e vin read error state:%hhd\n", vin->dvr_dev->framebuf_status[vin->dvr_dev->frame_finish[0]]);
				goto exit;
			} 
			for(i=0; i<count; i++)
			{
				frame_id = vin->dvr_dev->frame_finish[i];
				vin->dvr_dev->scale_in_yphyaddr = vin->dvr_dev->framebuf_phyaddr[frame_id];
				queue_work(vin->dvr_dev->scale_queue, &vin->dvr_dev->scale_work);
				if (vin->dvr_dev->framebuf_status[frame_id] == FRAMEBUF_STATUS_BUSY) {
					vin->dvr_dev->framebuf_status[frame_id] = FRAMEBUF_STATUS_FREE;
				} else {
					printk(KERN_ALERT "ark1668e vin free no-ready buf %d. status:%d\n", i, vin->dvr_dev->framebuf_status[i]);
				}
				break;
			}  
exit:
			vin->dvr_dev->frame_finish_count -= count;
			if (vin->dvr_dev->frame_finish_count > 0) {
			    int i;
			    char tmp;
			    for (i = 0; i < vin->dvr_dev->frame_finish_count; i++) {
			        tmp = vin->dvr_dev->frame_finish[count + i];
			        vin->dvr_dev->frame_finish[i] = tmp;
			    }
			}
		}
	}
	return 0;
}
EXPORT_SYMBOL(ark_vin_display_int_handler);

static void vin_push_frame_buffer(struct dvr_dev *dvr_dev, int frame_id)
{
	unsigned int val;

    //printk("dvr_push_frame_buffer %d.\n", frame_id);
	if (frame_id >= 0 && frame_id < dvr_dev->framebuf_num) {
		vin_writel(ARK1668E_ITU656_DRAM_DEST1,dvr_dev->framebuf_phyaddr[frame_id]);
        val = dvr_dev->framebuf_phyaddr[frame_id] + dvr_dev->src_width*dvr_dev->src_height;
		vin_writel(ARK1668E_ITU656_DRAM_DEST1,val);
    }
}

static int arkvin_frame_test_and_push(struct dvr_dev *dvr_dev, int frame_id)
{
	int i;
	int tmp;

	tmp = frame_id;
	for(i=0; i<dvr_dev->framebuf_num; i++){
		tmp ++;
		tmp %= 4;
		if (dvr_dev->framebuf_status[tmp] == FRAMEBUF_STATUS_FREE){
			vin_push_frame_buffer(dvr_dev, tmp);
			break;
		}
	}

	if (i == dvr_dev->framebuf_num) {
		tmp = frame_id;
		for(i=0; i<dvr_dev->framebuf_num; i++){
			tmp ++;
			tmp %= 4;
			if (dvr_dev->framebuf_status[tmp] == FRAMEBUF_STATUS_READY){
				dvr_dev->framebuf_status[tmp] = FRAMEBUF_STATUS_FREE;
				vin_push_frame_buffer(dvr_dev, tmp);
				printk(KERN_ALERT "### reuse ready framebuf:%d.\n", tmp);
				break;
			}
		}
		if (i == dvr_dev->framebuf_num) {
			printk(KERN_ALERT "### err: no free and ready framebuf, reuse default frame_id:%d framebuf.\n", frame_id);
			dvr_dev->framebuf_status[frame_id] = FRAMEBUF_STATUS_FREE;
			vin_push_frame_buffer(dvr_dev, frame_id);
		}
	}

	return 0;
}


static irqreturn_t ark_vin_int_handler(int irq, void *dev_id)
{
	u32 intr_stat;
	struct dvr_dev* dvr_dev = (struct dvr_dev *)dev_id;
	struct ark1668e_vin_device* vin = NULL;
	struct vb2_buffer *vb = NULL;
	int push_frame_state = 0;
	static int frame_id = 0;
	vin = g_ark1668e_vin;
	intr_stat = vin_readl(ARK1668E_ITU656_ISR);
    	vin_writel(ARK1668E_ITU656_ICR, 0x1fff);
	//spin_lock_irqsave(&dvr_dev->spin_lock, flags);
	if(intr_stat & (1<<10))
	{
		printk(KERN_ALERT "pop error.\n");
		arkvin_frame_test_and_push(dvr_dev, frame_id);
		push_frame_state = 1;
	}	
	if(intr_stat & TOTAL_LINE_CHANGED_INTERRUPT)
	{
		dvr_dev->show_video = 0;
		if(!vin->stream_flag)
			ark_vin_display_init(DISPLAY_LAYER,0,0,0,0);
		if (dvr_dev->itu656in.tvout)
			ark_disp_set_layer_en(TVOUT_LAYER, 0);
		ark_vin_disable_write();
		if(vin->dvr_dev->chip_info == TYPE_RN6752)
			mod_timer(&dvr_dev->timer, jiffies +  msecs_to_jiffies(150));
		else
			mod_timer(&dvr_dev->timer, jiffies +  msecs_to_jiffies(50));
	}	
	
	if (!dvr_dev->work_status) goto end; 

	//printk(KERN_ALERT "ark_vin_int_handler--intr_stat=0x%0x\n",intr_stat);
	
	if (intr_stat & (FIELD_INTERRUPT | FRAME_INTERRUPT_INTERRUPT)) {
		if(vin->stream_flag){
			spin_lock(&vin->ark_queue_lock);
			if (vin->cur_frm) {
				//printk(KERN_DEBUG "v4l2 streaming vb2_buffer_done...\n");
				vb = &vin->cur_frm->vb.vb2_buf;
				vb->timestamp = ktime_get_ns();
				vin->cur_frm->vb.sequence = vin->sequence++;
				vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
				vin->cur_frm = NULL;
			}
			if (!list_empty(&vin->ark_queue) && !vin->stop) {
				vin->cur_frm = list_first_entry(&vin->ark_queue,
							     struct vin_buffer, list);
				//printk(KERN_DEBUG "INT->get out buf: %p\n",vin->cur_frm->vb.vb2_buf.planes[0].mem_priv);
				list_del(&vin->cur_frm->list);
				vin_setaddr(vin);
			}
			if (vin->stop)
				complete(&vin->comp);
			spin_unlock(&vin->ark_queue_lock);
		}
		if(!vin->stream_flag){
			unsigned int yaddr = vin_readl(ARK1668E_ITU656_DRAM_DEST1);
			 for (frame_id = 0; frame_id < vin->dvr_dev->framebuf_num; frame_id++) {
			 	if (yaddr == vin->dvr_dev->framebuf_phyaddr[frame_id])
			 	break;
        		}
			if(frame_id >= vin->dvr_dev->framebuf_num){
				goto end;
			}
			if(vin->dvr_dev->framebuf_status[frame_id] != FRAMEBUF_STATUS_FREE) {
				//printk(KERN_INFO "### frame_id:%d != free, status:%d, all(%d,%d,%d,%d)\n", frame_id, g_ark1668e_vin->dvr_dev->framebuf_status[frame_id], g_ark1668e_vin->dvr_dev->framebuf_status[0],g_ark1668e_vin->dvr_dev->framebuf_status[1],g_ark1668e_vin->dvr_dev->framebuf_status[2],g_ark1668e_vin->dvr_dev->framebuf_status[3]);
				if(!push_frame_state){
				arkvin_frame_test_and_push(dvr_dev, frame_id);
				goto end;
				}
			}

			if (dvr_dev->discard_frame > 0) {
	            dvr_dev->discard_frame--;
	            goto end;
    		} 

			vin->dvr_dev->framebuf_status[frame_id] = FRAMEBUF_STATUS_READY;
			if(!push_frame_state)
			arkvin_frame_test_and_push(dvr_dev, frame_id);
			dvr_dev->cur_frame = frame_id;
			
			if (dvr_dev->frame_finish_count >= dvr_dev->framebuf_num)
                dvr_dev->frame_finish_count = dvr_dev->framebuf_num - 1;
            dvr_dev->frame_finish[dvr_dev->frame_finish_count++] = dvr_dev->cur_frame;
			wake_up_interruptible(&dvr_dev->frame_finish_waitq);
		}

	//spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
}
end:
   	return IRQ_HANDLED;
}

static void ark_vin_pad_select(struct dvr_dev *dvr_dev)
{
	unsigned int val;
	if (dvr_dev->itu601en) {
		//hsync, vsync
		val = vin_readl_sys(ARK1668E_SYS_PAD_CTRL04);
		val &= ~(0x3f<<24);
		val |=(1<<24)|(1<<27);
        vin_writel_sys(ARK1668E_SYS_PAD_CTRL04, vin_readl_sys(ARK1668E_SYS_PAD_CTRL04)|val);
	}
	
	if (dvr_dev->itu_channel== ITU656_CH0) {
		val = vin_readl_sys(ARK1668E_SYS_PAD_CTRL02);
		val &= ~(0x7ffffff<<3);
		val |= (0x1<<3)|(0x1<<6)|(0x1<<9)|(0x1<<12)|(0x1<<15)|(0x1<<18)|(0x1<<21)|(0x1<<24)|(0x1<<27);          //choose one pad of the channel 0	

		vin_writel_sys(ARK1668E_SYS_PAD_CTRL02, val);
	} else if (dvr_dev->itu_channel == ITU656_CH1) {
		val = vin_readl_sys(ARK1668E_SYS_PAD_CTRL03);
		val |= 0x01249249;         																				//choose one pad of the channel 1	
		vin_writel_sys(ARK1668E_SYS_PAD_CTRL03, val);
    
		val = vin_readl_sys(ARK1668E_SYS_PAD_CTRL0F);
		val |= (1<<24)|(1<<26);
	vin_writel_sys(ARK1668E_SYS_PAD_CTRL0F, val);	
	} else if(dvr_dev->itu_channel == ITU656_CH2){
		val = vin_readl_sys(ARK1668E_SYS_PAD_CTRL03);
		val |= 0x08000000;         																				//choose one pad of the channel 2	
		vin_writel_sys(ARK1668E_SYS_PAD_CTRL03, val);
	
		val = vin_readl_sys(ARK1668E_SYS_PAD_CTRL04);
		val |= 0x249249;
		vin_writel_sys(ARK1668E_SYS_PAD_CTRL04, val);
	}   	
}


static void ark_vin_reg_init(struct dvr_dev *dvr_dev)
{
	unsigned int val;

	val = vin_readl(ARK1668E_ITU656_MODULE_EN);
    val |= (1 << 2);
    vin_writel(ARK1668E_ITU656_MODULE_EN, val);

	if (dvr_dev->itu601en) {
		vin_writel(ARK1668E_ITU656_MODULE_EN, vin_readl(ARK1668E_ITU656_MODULE_EN) | 1);
		
		val = vin_readl(ARK1668E_ITU656_INPUT_SEL);
	    val = 0;
	    vin_writel(ARK1668E_ITU656_INPUT_SEL, val);
		//polarity
		//val = (1 << 13) | (1 << 12);  
		//vin_writel(ARK1668E_ITU656_SEP_MODE_SEL, val);
	}else{
		val = vin_readl(ARK1668E_ITU656_INPUT_SEL);
	    val = 0x00000001;
	    vin_writel(ARK1668E_ITU656_INPUT_SEL, val);
	}

	val = vin_readl(ARK1668E_ITU656_IMR);
    val = 0;
    vin_writel(ARK1668E_ITU656_IMR, val);

	val = vin_readl(ARK1668E_ITU656_YUV_TYPESEL);
    val = 0;
    vin_writel(ARK1668E_ITU656_YUV_TYPESEL, val);
	
	val = vin_readl(ARK1668E_ITU656_SEP_MODE_SEL);
    val |= 1<<3;
    vin_writel(ARK1668E_ITU656_SEP_MODE_SEL, val);

	val = vin_readl(ARK1668E_ITU656_ENABLE_REG);
    if(g_ark1668e_vin->dvr_dev->chip_info == TYPE_ARK7116H)
        val = 0x0000000a;//0x0000600a;
    else
        val = 0x0000200a;//0x0000600a;
    vin_writel(ARK1668E_ITU656_ENABLE_REG, val);

	val = vin_readl(ARK1668E_ITU656_MIRR_SET);
    val = g_ark1668e_vin->dvr_dev->vin_mirror_config;
    vin_writel(ARK1668E_ITU656_MIRR_SET, val);

	val = vin_readl(ARK1668E_ITU656_OUTPUT_TYPE);
    val = 0;
    vin_writel(ARK1668E_ITU656_OUTPUT_TYPE, val);

/********************************************************/
#if 0
    val = 0x4c01;
	vin_writel(ARK1668E_ITU656_YUV_TYPESEL11, val);
	val = vin_readl(ARK1668E_ITU656_MODULE_EN);
    val |= 1;
    vin_writel(ARK1668E_ITU656_MODULE_EN, val);
#endif	
/********************************************************/

	val = vin_readl(ARK1668E_ITU656_IMR);
    val |= (FIFO_POP_ERROR) | (TOTAL_LINE_CHANGED_INTERRUPT) | (ACTIVE_LINE_CHANGED_INTERRUPT) | (1<<3) | (1<<0);
    vin_writel(ARK1668E_ITU656_IMR, val);
}


static void ark_vin_enable(struct dvr_dev *dvr_dev)
{
	vin_writel(ARK1668E_ITU656_ENABLE_REG, vin_readl(ARK1668E_ITU656_ENABLE_REG)| (1 << 0)); 
}

static void deinterlace_reset(void)
{
}

static void deinterlace_init(void)
{
	deinterlace_reset();
}

static void dither_timeout_timer(struct timer_list *t)
{
	struct dvr_dev *dvr_dev = from_timer(dvr_dev, t, timer);
	struct ark1668e_vin_device* vin = NULL;
	int line,pixel,activeLine, activePix,i;
	vin = g_ark1668e_vin;

	//spin_lock_irqsave(&dvr_dev->spin_lock, flags);

	if (!dvr_dev->work_status) {
		//spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
		return;
	}

	line = vin_readl(ARK1668E_ITU656_LINE_NUM_PER_FIELD) & 0x7FF;
	if (dvr_dev->itu601en)
		pixel = vin_readl(ARK1668E_ITU656_PIX_NUM_PER_LINE) & 0xFFF;
	else
		pixel = (vin_readl(ARK1668E_ITU656_PIX_NUM_PER_LINE) & 0xFFE) >> 1;	

	printk(KERN_ALERT "[ITU] line = %d, pixel=%d\r\n", line, pixel);

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
	}

	vin_writel(ARK1668E_ITU656_OUTLINE_NUM_PER_FIELD, activeLine);
	vin_writel(ARK1668E_ITU656_TOTAL_PIX_OUT, activeLine*activePix);
	vin_writel(ARK1668E_ITU656_TOTAL_PIX, activeLine*activePix);
	vin_writel(ARK1668E_ITU656_SIZE, activePix<<16);

	dvr_dev->src_width = activePix;
	dvr_dev->src_height = activeLine;
	dvr_dev->discard_frame = START_DISCARD_FRAME;
	dvr_dev->cur_frame = 0;
	dvr_dev->show_video = 1;

	for (i = 0; i < vin->dvr_dev->framebuf_num; i++){
		vin_push_frame_buffer(dvr_dev,i);
        vin->dvr_dev->framebuf_status[i] = FRAMEBUF_STATUS_FREE;
	}
	 
	ark_vin_enable_write();	
	//spin_unlock_irqrestore(&dvr_dev->spin_lock, flags);
}

static int vin_start(struct dvr_dev *dvr_dev)
{ 
	struct ark1668e_vin_device* vin = NULL;
	int ret = 0;
	vin = g_ark1668e_vin;
	if(!dvr_dev->work_status){
#ifdef CONFIG_VIDEO_USE_LOCK
		down_interruptible(&dvr_dev->vin_sem);
#endif
		dvr_dev->work_status = 1;
		dvr_dev->discard_frame = START_DISCARD_FRAME;
		dvr_dev->cur_frame = 0;
		dvr_dev->vin_buffer_status = 0;
		deinterlace_init();
		ret = v4l2_subdev_call(vin->current_subdev->sd,video,s_routing,dvr_dev->channel,0,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
		ark_vin_pad_select(dvr_dev);
		ark_vin_reg_init(dvr_dev);
		ark_vin_enable(dvr_dev);
	}
	return ret;
}

static int vin_exit(void)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret;

	vin = g_ark1668e_vin;
	vin->dvr_dev->carback_exit_status = 1;
	vin->stream_flag = vin->vin_status;
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	spin_lock(&vin->dvr_dev->spin_lock);
	vin->dvr_dev->vin_mirror_config = MIRROR_NO;
	vin->dvr_dev->work_status = 0;
	vin->dvr_dev->scale_framebuf_index = 0;
	vin->dvr_dev->show_video = 0;
	vin->dvr_dev->carback_signal = 0;
	vin->dvr_dev->layer_status = 0;
	msleep(20);
	ark_vin_disable_write(); /*stop write data back*/
	ark_vin_disable();
	spin_unlock(&vin->dvr_dev->spin_lock);

	if(vin->dvr_dev->chip_info == TYPE_RN6752){
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_EXIT_CARBACK,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
	}

	return 0;
}


static void vin_init(struct ark1668e_vin_device *vin, struct vin_para *para)
{
	struct dvr_dev* dvr_dev = vin->dvr_dev;
	memcpy(&dvr_dev->itu656in, para, sizeof(struct vin_para));
	dvr_dev->interlace = !dvr_dev->itu656in.progressive;
	dvr_dev->itu601en = dvr_dev->itu656in.itu601en;	
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

static int vin_aux_start(struct dvr_dev *dvr_dev)
{ 
	struct ark1668e_vin_device* vin = NULL;
	int ret = 0;
	vin = g_ark1668e_vin;
	if(!dvr_dev->work_status){
		dvr_dev->work_status = 1;
		dvr_dev->discard_frame = START_DISCARD_FRAME;
		dvr_dev->cur_frame = 0;
		dvr_dev->vin_buffer_status = 0;
		ret = v4l2_subdev_call(vin->current_subdev->sd,video,s_routing,vin->app_channel_set,0,0);
		if(ret < 0){
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
			return ret;
		}
		deinterlace_init();
		ark_vin_pad_select(dvr_dev);
		ark_vin_reg_init(dvr_dev);
		ark_vin_enable(dvr_dev);
	}
	return ret;
}

static void vin_aux_exit(void)
{
	struct ark1668e_vin_device* vin = NULL;
	int ret;
	vin = g_ark1668e_vin;
	spin_lock(&vin->dvr_dev->spin_lock);
	vin->dvr_dev->carback_exit_status = 1;
	vin->stream_flag = vin->vin_status;
	vin->dvr_dev->show_video = 0;
	vin->dvr_dev->carback_signal = 0;
	vin->dvr_dev->work_status = 0;
	vin->dvr_dev->layer_status = 0;
	ark_disp_set_layer_en(DISPLAY_LAYER, 0);
	vin->dvr_dev->scale_framebuf_index = 0;
	spin_unlock(&vin->dvr_dev->spin_lock);
	msleep(20);
	ark_vin_disable_write(); /*stop write data back*/
	ark_vin_disable();
	if(vin->dvr_dev->chip_info == TYPE_RN6752){
		ret = v4l2_subdev_call(vin->current_subdev->sd,core,ioctl,VIDIOC_EXIT_CARBACK,0);
		if(ret < 0)
			printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
	}
}

static irqreturn_t ark_deinterlace_int_handler(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}


static int vin_async_bound(struct v4l2_async_notifier *notifier,
			    struct v4l2_subdev *subdev,
			    struct v4l2_async_subdev *asd)
{
	struct vin_subdev_entity *subdev_entity = NULL;
	struct ark1668e_vin_device *ark_vin = NULL;
	subdev_entity = container_of(notifier, struct vin_subdev_entity, notifier);
	ark_vin = container_of(notifier->v4l2_dev,struct ark1668e_vin_device, v4l2_dev);

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
	struct ark1668e_vin_device *ark_vin = NULL;
	ark_vin = container_of(notifier->v4l2_dev,struct ark1668e_vin_device, v4l2_dev);
	cancel_work_sync(&ark_vin->awb_work);
	video_unregister_device(&ark_vin->video_dev);
}

static int vin_set_default_fmt(struct ark1668e_vin_device *vin)
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
	int ret,support_max_resolution,chipinfo;
	struct i2c_client *sd_client = NULL;
	struct ark1668e_vin_device *ark_vin = container_of(notifier->v4l2_dev,struct ark1668e_vin_device, v4l2_dev);
	ark_vin->current_subdev = container_of(notifier,struct vin_subdev_entity, notifier);
	sd_client = v4l2_get_subdevdata(ark_vin->current_subdev->sd);
	if(!sd_client){
		printk(KERN_ALERT "get subdev data error .\n");
		return 	-EINVAL;
	}
	printk("subdev addr is %p, name is %s.\n",ark_vin->current_subdev->sd,sd_client->name);
	
	/* Register subdev device node */
	ret = v4l2_device_register_subdev_nodes(&ark_vin->v4l2_dev);
	if (ret < 0) {
		printk(KERN_ALERT "v4l2_device_register_subdev_nodes  error\n");
		return ret;
	}

	ret = v4l2_subdev_call(ark_vin->current_subdev->sd,core,ioctl,VIDIOC_GET_CHIPINFO,&chipinfo);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	if(chipinfo == TYPE_RN6752){
		printk(KERN_ALERT "decoder chip is rn6752\n");
		ark_vin->dvr_dev->chip_info = TYPE_RN6752;
		ark_vin->dvr_dev->framebuf_num = 8;
        ret = v4l2_subdev_call(ark_vin->current_subdev->sd,core,ioctl,VIDIOC_ENABLE_TIME,0);
        if(ret < 0){
        printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
        return ret;
        }
	}else if(chipinfo == TYPE_ARK7116){
		printk(KERN_ALERT "decoder chip is ark7116\n");
		ark_vin->dvr_dev->chip_info = TYPE_ARK7116;
	}else if(chipinfo == TYPE_ARK7116H){
		printk(KERN_ALERT "decoder chip is ark7116h\n");
		ark_vin->dvr_dev->chip_info = TYPE_ARK7116H;
	}else if(chipinfo == TYPE_PR2000){
		printk(KERN_ALERT "decoder chip is pr2000\n");
		ark_vin->dvr_dev->chip_info = TYPE_PR2000;
	}else{
		printk(KERN_ALERT "no find decoder chip info\n");
	}
	ret = v4l2_subdev_call(ark_vin->current_subdev->sd,core,ioctl,VIDIOC_GET_RESOLUTION,&support_max_resolution);
	if(ret < 0){
		printk(KERN_ALERT "%s %d: v4l2_subdev_call error \n",__FUNCTION__, __LINE__);
		return ret;
	}
	
	if(support_max_resolution == TYPE_1080P){
		if(ark_vin->dvr_dev->chip_info == TYPE_RN6752)
			ark_vin->dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE_1080P*ark_vin->dvr_dev->framebuf_num;
		else
			ark_vin->dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE_1080P*ITU656_FRAME_NUM;
	}
	else if(support_max_resolution == TYPE_720P){
		if(ark_vin->dvr_dev->chip_info == TYPE_RN6752)
			ark_vin->dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE*ark_vin->dvr_dev->framebuf_num;
		else
			ark_vin->dvr_dev->buffer_size = ITU656_PROGRESSIVE_FRAME_SIZE*ITU656_FRAME_NUM;
	}
	else{
		if(ark_vin->dvr_dev->chip_info == TYPE_RN6752)
			ark_vin->dvr_dev->buffer_size = ITU656_FRAME_SIZE*ark_vin->dvr_dev->framebuf_num;
		else
			ark_vin->dvr_dev->buffer_size = ITU656_FRAME_SIZE*ITU656_FRAME_NUM;
	}
	
	ark_vin->dvr_dev->scale_buffer_size = ark_vin->dvr_dev->scale_alloc_width*ark_vin->dvr_dev->scale_alloc_height*2*ark_vin->dvr_dev->scale_framebuf_num;

	vin_alloc_buf(support_max_resolution);

	mutex_init(&ark_vin->lock);
	init_completion(&ark_vin->comp);

	/* Initialize videobuf2 queue */
	ark_vin->vb2_vidq.type			= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ark_vin->vb2_vidq.io_modes		= VB2_MMAP | VB2_DMABUF | VB2_READ;
	ark_vin->vb2_vidq.drv_priv		= ark_vin;
	ark_vin->vb2_vidq.buf_struct_size	= sizeof(struct vin_buffer);
	ark_vin->vb2_vidq.ops			= &vin_vb2_ops;
	ark_vin->vb2_vidq.mem_ops		= &vb2_dma_contig_memops;
	ark_vin->vb2_vidq.timestamp_flags	= V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	ark_vin->vb2_vidq.lock			= &ark_vin->lock;
	ark_vin->vb2_vidq.min_buffers_needed	= 1;
	ark_vin->vb2_vidq.dev			= ark_vin->dev;

	ret = vb2_queue_init(&ark_vin->vb2_vidq);
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
	strlcpy(ark_vin->video_dev.name, ARK_VIN_NAME, sizeof(ark_vin->video_dev.name));
	ark_vin->video_dev.release		= video_device_release_empty;
	ark_vin->video_dev.fops		= &vin_fops;
	ark_vin->video_dev.ioctl_ops		= &vin_ioctl_ops;
	ark_vin->video_dev.v4l2_dev		= &ark_vin->v4l2_dev;
	ark_vin->video_dev.vfl_dir		= VFL_DIR_RX;
	ark_vin->video_dev.queue		= &ark_vin->vb2_vidq;
	ark_vin->video_dev.lock		= &ark_vin->lock;
	ark_vin->video_dev.device_caps	= V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_CAPTURE;
	video_set_drvdata(&ark_vin->video_dev, ark_vin);

	ret = video_register_device(&ark_vin->video_dev, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		v4l2_err(&ark_vin->v4l2_dev,
			 "video_register_device failed: %d\n", ret);
		return ret;
	}
	
	return 0;
}

static const struct v4l2_async_notifier_operations vin_graph_notify_ops = {
	.bound = vin_async_bound,
	.unbind = vin_async_unbind,
	.complete = vin_async_complete,
};

static int vin_driver_init(struct device *dev)
{
	int value;
	struct ark1668e_vin_device* vin = NULL;
	vin = g_ark1668e_vin;
	vin->dvr_dev = devm_kzalloc(dev, sizeof(struct dvr_dev), GFP_KERNEL);
       if (vin->dvr_dev == NULL) {
        	dev_err(dev, "%s %d: failed to allocate memory\n",
        	    __FUNCTION__, __LINE__);
        	return -ENOMEM;
       }

	vin->dvr_dev->work_status = 0;
	vin->dvr_dev->layer_status = 0;
	vin->dvr_dev->signal_flag = 0;
	vin->dvr_dev->first_show_flag = 1;
	vin->dvr_dev->system = NTSC;
	vin->dvr_dev->cur_buffer = 0;
	vin->dvr_dev->carback_signal = 0;
	vin->dvr_dev->frame_finish_count = 0;
	vin->dvr_dev->scale_framebuf_index = 0;
	vin->dvr_dev->vin_buffer_status = 0;
	vin->dvr_dev->vin_mirror_config = MIRROR_NO;
	vin->dvr_dev->framebuf_num = ITU656_FRAME_NUM;
	vin->dvr_dev->scale_framebuf_num = ITU656_SCALE_FRAME_NUM;
	vin->dvr_dev->carback_exit_status = 1;
	vin->aux_flag = 0;
	vin->aux_status = 0;
	vin->stream_flag = false;
	vin->sd_init = 0;

	if(!of_property_read_u32(dev->of_node, "width", &value)) {
	    if(value > 0){
			of_property_read_u32(dev->of_node, "x_pos", &value);
			vin->dvr_dev->screen_xpos = value;
			of_property_read_u32(dev->of_node, "y_pos", &value);
			vin->dvr_dev->screen_ypos = value;
			of_property_read_u32(dev->of_node, "width", &value);
			vin->dvr_dev->screen_width = value;
			of_property_read_u32(dev->of_node, "height", &value);
			vin->dvr_dev->screen_height = value;
			printk("++++++ x_pos = %d,y_pos = %d,width = %d,height = %d",vin->dvr_dev->screen_xpos,vin->dvr_dev->screen_ypos,vin->dvr_dev->screen_width,vin->dvr_dev->screen_height);
	    }
	}else{
		vin->dvr_dev->screen_xpos = 0;
		vin->dvr_dev->screen_ypos = 0;
	    ark_vin_get_screen_info(&vin->dvr_dev->screen_width,&vin->dvr_dev->screen_height);
	}
	
	vin->dvr_dev->scale_alloc_width = vin->dvr_dev->screen_width;
	vin->dvr_dev->scale_alloc_height = vin->dvr_dev->screen_height;
	init_waitqueue_head(&vin->dvr_dev->frame_finish_waitq);
	spin_lock_init(&vin->dvr_dev->spin_lock);
	
	return 0;
}


static int vin_parse_dt(struct device *dev, struct ark1668e_vin_device *ark_vin)
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

static void vin_subdev_cleanup(struct ark1668e_vin_device *vin)
{
	struct vin_subdev_entity *subdev_entity;

	list_for_each_entry(subdev_entity, &vin->subdev_entities, list)
		v4l2_async_notifier_unregister(&subdev_entity->notifier);

	INIT_LIST_HEAD(&vin->subdev_entities);
}


static int ark1668e_vin_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct vin_subdev_entity *subdev_entity;
	struct ark1668e_vin_device* ark_vin;
	struct resource *res;
    void __iomem *regs;
	int ret,value;

	ark_vin = devm_kzalloc(dev, sizeof(*ark_vin), GFP_KERNEL);
	if (!ark_vin)
		return -ENOMEM;

	g_ark1668e_vin = ark_vin;

	platform_set_drvdata(pdev, ark_vin);
	ark_vin->dev = dev;

	ret = v4l2_device_register(dev,&ark_vin->v4l2_dev);
	if (ret) {
		printk(KERN_ALERT "unable to register v4l2 device.\n");
	}

	ret = vin_parse_dt(dev, ark_vin);
	if (ret)
		printk(KERN_ALERT "fail to parse device tree\n");

	vin_driver_init(dev);

	if (list_empty(&ark_vin->subdev_entities)) {
		printk(KERN_ALERT "no subdev found \n");
		ret = -ENODEV;
	}
	/*find and prepare the async subdev notifier and register it */
	list_for_each_entry(subdev_entity, &ark_vin->subdev_entities, list) {
		subdev_entity->notifier.subdevs = &subdev_entity->asd;
		subdev_entity->notifier.num_subdevs = 1;
		subdev_entity->notifier.ops = &vin_graph_notify_ops;

		ret = v4l2_async_notifier_register(&ark_vin->v4l2_dev,
						   &subdev_entity->notifier);
		if (ret) {
			printk(KERN_ALERT "fail to register async notifier\n");
		}

		if (video_is_registered(&ark_vin->video_dev))
			break;
	}
	
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
			"ark1668e_vin", 
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
				"vin_deinterlace", 
				ark_vin->dvr_dev
				);
	if(ret){
	dev_err(&pdev->dev, "%s %d: can't get assigned deinterlace_irq %d, error %d\n",
	    __FUNCTION__, __LINE__, ark_vin->dvr_dev->context.deinterlace_irq, ret);
	}

#ifdef CONFIG_VIDEO_USE_LOCK
	sema_init(&ark_vin->dvr_dev->vin_sem, 1);
#endif

	timer_setup(&ark_vin->dvr_dev->timer, dither_timeout_timer, 0);
	timer_setup(&ark_vin->dvr_dev->signal_timer, vin_get_signal_time, 0);

	ark_vin->dvr_dev->detect_queue = create_singlethread_workqueue("detect_queue");
	if(!ark_vin->dvr_dev->detect_queue) {
		printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);	
		return -1;
	}

	INIT_WORK(&ark_vin->dvr_dev->detect_work, vin_get_signal_work);

	mod_timer(&ark_vin->dvr_dev->signal_timer, jiffies +  msecs_to_jiffies(10));

	ark_vin->dvr_dev->scale_queue = create_singlethread_workqueue("scale_queue");
	if(!ark_vin->dvr_dev->scale_queue) {
		printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);	
		return -1;
	}

	INIT_WORK(&ark_vin->dvr_dev->scale_work, ark_vin_scale_work);

	ark_vin->dvr_dev->itu_channel = ITU656_CH1;
	if(!of_property_read_u32(pdev->dev.of_node, "channel", &value)) {
	    if(value >= ITU656_CH0 && value <= ITU656_CH2)
	            ark_vin->dvr_dev->itu_channel = value;
	}

	return 0;
}

static int ark1668e_vin_remove(struct platform_device *pdev)
{
	struct ark1668e_vin_device *vin = platform_get_drvdata(pdev);
	struct dvr_dev *dvr_dev = vin->dvr_dev;
#if 0
	free_pages((unsigned long)dvr_dev->buffer_virtaddr, get_order(dvr_dev->buffer_size));
	free_pages((unsigned long)dvr_dev->scale_out_yaddr, get_order(dvr_dev->scale_alloc_width*dvr_dev->scale_alloc_height*2));
#else
	if(vin->dvr_dev->buffer_virtaddr)
		dma_free_wc(vin->dev, vin->dvr_dev->buffer_size, vin->dvr_dev->buffer_virtaddr,vin->dvr_dev->buffer_phyaddr);
	if(vin->dvr_dev->scale_out_yaddr)
		dma_free_wc(vin->dev, vin->dvr_dev->scale_buffer_size,vin->dvr_dev->scale_out_yaddr,vin->dvr_dev->scale_out_yphyaddr);
#endif
    iounmap(dvr_dev->context.lcd_base);
    iounmap(dvr_dev->context.deinterlace_base);
    iounmap(dvr_dev->context.sys_base);
	del_timer(&vin->dvr_dev->timer);
	del_timer(&vin->dvr_dev->signal_timer);
	unregister_chrdev_region(MKDEV(dvr_dev->dev_major, dvr_dev->dev_minor), 1);		
	g_ark1668e_vin = NULL;
	vin_subdev_cleanup(vin);
	v4l2_device_unregister(&vin->v4l2_dev);
	return 0;
}

static const struct of_device_id ark1668e_vin_of_match[] = {
	{ .compatible = "arkmicro,ark1668e-vin", },
	{ }
};
MODULE_DEVICE_TABLE(of, ark1668e_vin_of_match);

static struct platform_driver ark1668e_vin_driver = {
        .driver     = {
        .name   = "ark1668e-vin",
        	.of_match_table = of_match_ptr(ark1668e_vin_of_match),
        },
        .probe      = ark1668e_vin_probe,
        .remove     = ark1668e_vin_remove,
};

static int __init ark_vin_init(void)
{
	return platform_driver_register(&ark1668e_vin_driver);
}
device_initcall(ark_vin_init);

MODULE_AUTHOR("arkmicro");
MODULE_DESCRIPTION("The V4L2 driver for arkmicro");
MODULE_LICENSE("GPL v2");

