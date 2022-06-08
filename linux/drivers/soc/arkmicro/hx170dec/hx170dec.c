/*
 * On2/Hantro G1 decoder/pp driver. Single core version.
 *
 * Copyright (C) 2009  Hantro Products Oy.
 *
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#include <linux/dma-mapping.h>
#include <linux/delay.h>


#include "hx170dec.h"
#include "vdec.h"
#include "jpegdecapi.h"
#include "jpegdeccontainer.h"


struct vdec_device *vdec6731_global;

unsigned int animation_jpegPhyaddr;

unsigned int dis_decWidth;
unsigned int dis_decHeight;
unsigned int dis_format;

static struct mfc_jpeg_context *vde_jpeg_context = NULL;

#define MAX_ANIMFRAME_WIDTH		1920
#define MAX_ANIMFRAME_HEIGHT	1088
#define MAX_ANIMFRAME_SIZE		(MAX_ANIMFRAME_WIDTH * MAX_ANIMFRAME_HEIGHT * 4)

extern int ark_bootanimation_display_init(int width, int height, unsigned int yaddr,unsigned int uaddr,unsigned int vadd,unsigned int format);
extern int ark_bootanimation_display_uninit(void);
extern int ark_bootanimation_set_display_addr(unsigned int yaddr,unsigned int uaddr,unsigned int vaddr,unsigned int format);

int get_bootanimation_status(void)
{
    return vde_jpeg_context->anmation_stats;
}

EXPORT_SYMBOL(get_bootanimation_status);


static inline void vdec_writel(const struct vdec_device *p, unsigned offset, u32 val)
{
	writel(val, p->mmio_base + offset);
}

static inline u32 vdec_readl(const struct vdec_device *p, unsigned offset)
{
	return readl(p->mmio_base + offset);
}

/**
 * Write a range of registers. First register is assumed to be
 * "Interrupt Register" and will be written last.
 */
static int vdec_regs_write(struct vdec_device *p, int begin, int end,
		const struct core_desc *core)
{
	int i;

	if (copy_from_user(&p->regs[begin], core->regs, (end - begin + 1) * 4))
	{
		dev_err(p->dev, "%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	for (i = end; i >= begin; i--)
		vdec_writel(p, 4 * i, p->regs[i]);

	return 0;
}

/**
 * Read a range of registers [begin..end]
 */
static int vdec_regs_read(struct vdec_device *p, int begin, int end,
		const struct core_desc *core)
{
	int i;

	for (i = end; i >= begin; i--)
		p->regs[i] = vdec_readl(p, 4 * i);

	if (copy_to_user(core->regs, &p->regs[begin], (end - begin + 1) * 4))
	{
		dev_err(p->dev, "%s: copy_to_user failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

//mfc jpeg decode
#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

typedef struct {
	unsigned int magic;
	int hasBootlogo;
	int bootlogoDisplayTime;
	int aniCount;
	int aniWidth;
	int aniHeight;
	int aniFps;
	int aniDelayHideTime;
	int reserved[4];
} BANIHEADER;


static int mfc_jpeg_decode(unsigned int src_addr,unsigned int pvSrc_addr,
						   unsigned int dest_addr, unsigned int pvDest_addr,
						   unsigned int size)
{
	int ret = 0;

	JpegDecInst  decoder;
	JpegDecRet   infoRet;
	JpegDecInput DecIn;
	JpegDecImageInfo DecImgInf;
	JpegDecOutput DecOut;
	infoRet = JpegDecInit(&decoder);
	if(infoRet !=JPEGDEC_OK) {
		printk("JpegDecInit failure %d.\n", infoRet);
		return -1;
	}

	DecIn.decImageType = JPEGDEC_IMAGE;
	DecIn.sliceMbSet = 0;

	DecIn.bufferSize = 0;
	DecIn.streamLength = size;
	DecIn.streamBuffer.busAddress= src_addr;
	DecIn.streamBuffer.pVirtualAddress = (u32*)pvSrc_addr;

	/* Get image information of the JFIF */
	infoRet = JpegDecGetImageInfo(decoder,
		                   &DecIn,
		                   &DecImgInf);
	if(infoRet !=JPEGDEC_OK)
	{
		printk("JpegDecGetImageInfo failure.\n");
		ret = -1;
		goto dec_end;
	}

//	printk("mfc_jpeg_decode outputWidth %d,outputHeight %d,outputFormat 0x%x\n",DecImgInf.outputWidth,DecImgInf.outputHeight,DecImgInf.outputFormat);

	dis_decWidth = DecImgInf.outputWidth;
	dis_decHeight=DecImgInf.outputHeight;
	dis_format  = DecImgInf.outputFormat;

	DecIn.pictureBufferY.busAddress = dest_addr;
	DecIn.pictureBufferY.pVirtualAddress = (u32*)pvDest_addr;
	DecIn.pictureBufferCbCr.busAddress = dest_addr + DecImgInf.outputWidth * DecImgInf.outputHeight;
	DecIn.pictureBufferCbCr.pVirtualAddress = (u32*)(pvDest_addr + DecImgInf.outputWidth * DecImgInf.outputHeight);
	DecIn.pictureBufferCr.busAddress = 0;
	DecIn.pictureBufferCr.pVirtualAddress = 0;

	/* Decode JFIF */
    infoRet = JpegDecDecode(decoder, &DecIn, &DecOut);
	if(infoRet != JPEGDEC_FRAME_READY)
	{
		printk("JpegDecDecode failure, ret=%d\n", infoRet);
		ret = -1;
		goto dec_end;
	}

dec_end:
	JpegDecRelease(decoder);
    return 0;
}

//extern int ark_carback_get_status(void);
static void animation_dec_work(struct work_struct *work)
{
	static unsigned int frame = 0;
	static int timeout_count = 0;
	struct mfc_jpeg_context *context = container_of(work, struct mfc_jpeg_context, animation_work);
	BANIHEADER *header = (BANIHEADER *) context->animation_data_virtaddr;
	unsigned int size =
	    *(unsigned int *)(context->animation_data_virtaddr + context->animation_file_phyaddr -
			      context->animation_data_phyaddr);
	struct vdec_device *p = vdec6731_global;

	if (timeout_count > 50) {
		printk(KERN_ALERT "%s Error! Dec timeout.\n", __FUNCTION__);
		context->animation_end = true;
	} else if (context->animation_file_phyaddr + size >= context->animation_data_phyaddr + context->animation_data_size
		&& frame < header->aniCount + header->hasBootlogo ? 1 : 0) {
		printk(KERN_ALERT "%s Error! Animation data is beyond the mark.\n", __FUNCTION__);
		context->animation_end = true;
	}

	if (context->animation_end) {
		ark_bootanimation_display_uninit();
		if (context->animation_display_phyaddr){
			dma_free_coherent(context->dev, context->animation_display_size,
					  (void *)context->animation_display_virtaddr,
					  context->animation_display_phyaddr);
		}
		iounmap((void*)context->animation_data_virtaddr);
		p->context.anmation_stats = 0;
		destroy_workqueue(context->animation_queue);
		return;
	}

	if (frame > 0) {
		if (!context->animation_dec_finish) {
			timeout_count++;
			mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(10));
			return;
		}
		timeout_count = 0;
	}

	if (frame == header->aniCount + header->hasBootlogo ? 1 : 0) {
		if (!context->animation_dec_finish) {
			timeout_count++;
			mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(10));
			return;
		}
		timeout_count = 0;
		context->animation_end = true;
		if (header->aniCount > 0) {
			mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(header->aniDelayHideTime));
		} else if (header->hasBootlogo) {
			mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(header->bootlogoDisplayTime));
		}
		return;
	}

	frame++;
	context->animation_dec_finish = false;

	context->intr_status = mfc_jpeg_decode(context->animation_file_phyaddr + 4, context->animation_file_virtaddr + 4,
					context->animation_display_phyaddr + MAX_ANIMFRAME_SIZE * context->animation_display_index,
					context->animation_display_virtaddr + MAX_ANIMFRAME_SIZE * context->animation_display_index,
					size);


	context->animation_file_phyaddr += size;
    context->animation_file_virtaddr += size;


#if 1
	if (!context->animation_end) {
		if (!context->intr_status) {
			unsigned int format;
			if (!context->animation_initdisplay) {
				if(dis_format == JPEGDEC_YCbCr420_SEMIPLANAR)
					format =  0x11;//ARK1668E_LCDC_FORMAT_Y_UV420;
				if(dis_format == JPEGDEC_YCbCr422_SEMIPLANAR)
					format = 0x10;//ARK1668E_LCDC_FORMAT_YUV;
				ark_bootanimation_display_init(header->aniWidth, header->aniHeight,
										context->animation_display_phyaddr + MAX_ANIMFRAME_SIZE * context->animation_display_index,
										context->animation_display_phyaddr + MAX_ANIMFRAME_SIZE * context->animation_display_index +
										dis_decWidth * dis_decHeight, 0, format);

				context->animation_initdisplay = true;
			} else {
				if(dis_format == JPEGDEC_YCbCr420_SEMIPLANAR)
					format =  0x11;//ARK1668E_LCDC_FORMAT_Y_UV420;
				if(dis_format == JPEGDEC_YCbCr422_SEMIPLANAR)
					format = 0x10;//ARK1668E_LCDC_FORMAT_Y_UV422;
				ark_bootanimation_set_display_addr(context->animation_display_phyaddr +	MAX_ANIMFRAME_SIZE * context->animation_display_index,
											context->animation_display_phyaddr + MAX_ANIMFRAME_SIZE * context->animation_display_index +
											dis_decWidth * dis_decHeight, 0, format);

				context->animation_display_index = !context->animation_display_index;
			}
			context->animation_dec_finish = true;
		} else {
				printk(KERN_ALERT "decode boot animation jpeg error.\n");
				context->animation_dec_finish = true;

		}
	}
#endif
	if (header->hasBootlogo && frame == 1)
		mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(header->bootlogoDisplayTime));
	else
		mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(1000 / header->aniFps));
}

static void animation_timer_handler(struct timer_list *t)
{

	struct mfc_jpeg_context *context = from_timer(context, t, animation_timer);

	queue_work(context->animation_queue, &context->animation_work);
}

/**
 * Misc driver related
 */

static int vdec_misc_open(struct inode *inode, struct file *filp)
{
	struct vdec_device *p = vdec6731_global;
	filp->private_data = p;

	dev_dbg(p->dev, "open\n");
	//clk_prepare_enable(p->clk);
	return 0;
}

static int vdec_misc_release(struct inode *inode, struct file *filp)
{
	struct vdec_device *p = filp->private_data;

	if (p->dec_owner == filp) {
		p->dec_irq_done = false;
		init_waitqueue_head(&p->dec_wq);
		sema_init(&p->dec_sem, VDEC_MAX_CORES);
		p->dec_owner = NULL;
	}

	if (p->pp_owner == filp) {
		p->pp_irq_done = false;
		init_waitqueue_head(&p->pp_wq);
		sema_init(&p->pp_sem, 1);
		p->pp_owner = NULL;
	}

	//clk_disable_unprepare(p->clk);
	dev_dbg(p->dev, "release\n");
	return 0;
}

static long vdec_misc_ioctl(struct file *filp, unsigned int cmd,
    unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	struct vdec_device *p = vdec6731_global;
	struct core_desc core;
	u32 reg;

	switch (cmd) {
		case HX170DEC_IOX_ASIC_ID:
			reg = vdec_readl(p, VDEC_IDR);
			if (copy_to_user(argp, &reg, sizeof(u32)))
				ret = -EFAULT;
			break;

		case HX170DEC_IOC_MC_OFFSETS:
		case HX170DEC_IOCGHWOFFSET:
			if (copy_to_user(argp, &p->iobaseaddr, sizeof(p->iobaseaddr)))
				ret = -EFAULT;
			break;
		case HX170DEC_IOCGHWIOSIZE: /* in bytes */
			if (copy_to_user(argp, &p->iosize, sizeof(p->iosize)))
				ret = -EFAULT;
			break;
		case HX170DEC_IOC_MC_CORES:
			if (copy_to_user(argp, &p->num_cores, sizeof(p->num_cores)))
				ret = -EFAULT;
			break;

		case HX170DEC_IOCS_DEC_PUSH_REG:
			if (copy_from_user(&core, (void *)arg, sizeof(struct core_desc))) {
				dev_err(p->dev, "copy_from_user (dec push reg) failed\n");
				ret = -EFAULT;
			} else {
				/* Skip VDEC_IDR (ID Register, ro) */
				core.regs++; // core.size -= 4;
				ret = vdec_regs_write(p, VDEC_DEC_FIRST_REG + 1, VDEC_DEC_LAST_REG, &core);
			}
			break;
		case HX170DEC_IOCS_PP_PUSH_REG:
			if (copy_from_user(&core, (void *)arg, sizeof(struct core_desc))) {
				dev_err(p->dev, "copy_from_user (pp push reg) failed\n");
				ret = -EFAULT;
			} else {
				/* Don't consider the 5 lastest registers (ro or unused) */
				ret = vdec_regs_write(p, VDEC_PP_FIRST_REG, VDEC_PP_LAST_REG - 5, &core);
			}
			break;

		case HX170DEC_IOCS_DEC_PULL_REG:
			if (copy_from_user(&core, (void *)arg, sizeof(struct core_desc))) {
				dev_err(p->dev, "copy_from_user (dec pull reg) failed\n");
				ret = -EFAULT;
			} else {
				ret = vdec_regs_read(p, VDEC_DEC_FIRST_REG, VDEC_DEC_LAST_REG, &core);
			}
			break;

		case HX170DEC_IOCS_PP_PULL_REG:
			if (copy_from_user(&core, (void*)arg, sizeof(struct core_desc))) {
				dev_err(p->dev, "copy_from_user (pp pull reg) failed\n");
				ret = -EFAULT;
			} else {
				ret = vdec_regs_read(p, VDEC_PP_FIRST_REG, VDEC_PP_LAST_REG, &core);
			}
			break;

		case HX170DEC_IOCX_DEC_WAIT:
			if (copy_from_user(&core, (void *)arg, sizeof(struct core_desc))) {
				dev_err(p->dev, "copy_from_user (dec wait) failed\n");
				ret = -EFAULT;
			} else {
				ret = wait_event_interruptible(p->dec_wq, p->dec_irq_done);
				p->dec_irq_done = false;
				if (unlikely(ret != 0)) {
					dev_err(p->dev, "wait_event_interruptible dec error %d\n", ret);
				} else {
					/* Update dec registers */
					ret = vdec_regs_read(p, VDEC_DEC_FIRST_REG, VDEC_DEC_LAST_REG, &core);
				}
			}
			break;
		case HX170DEC_IOCX_PP_WAIT:
			if (copy_from_user(&core, (void *)arg, sizeof(struct core_desc))) {
				dev_err(p->dev, "copy_from_user (pp wait) failed\n");
				ret = -EFAULT;
			} else {
				ret = wait_event_interruptible(p->pp_wq, p->pp_irq_done);
				p->pp_irq_done = false;
				if (unlikely(ret != 0)) {
					dev_err(p->dev, "wait_event_interruptible pp error %d\n", ret);
				} else {
					/* Update pp registers */
					ret = vdec_regs_read(p, VDEC_PP_FIRST_REG, VDEC_PP_LAST_REG, &core);
				}
			}
			break;

		case HX170DEC_IOCH_DEC_RESERVE:
			if (likely(down_interruptible(&p->dec_sem) == 0)) {
				p->dec_owner = filp;
				ret = 0; /* core id */
				dev_dbg(p->dev, "down dec_sem (core id %d)\n", ret);
			} else {
				dev_err(p->dev, "down_interruptible dec error\n");
				ret = -ERESTARTSYS;
			}
			break;
		case HX170DEC_IOCT_DEC_RELEASE:
			dev_dbg(p->dev, "up dec_sem\n");
			p->dec_owner = NULL;
			up(&p->dec_sem);
			break;

		case HX170DEC_IOCQ_PP_RESERVE:
			if (likely(down_interruptible(&p->pp_sem) == 0)) {
				p->pp_owner = filp;
				ret = 0; /* core id */
				dev_dbg(p->dev, "down pp_sem (core id %d)\n", ret);
			} else {
				dev_err(p->dev, "down_interruptible pp error\n");
				ret = -ERESTARTSYS;
			}
			break;
		case HX170DEC_IOCT_PP_RELEASE:
			dev_dbg(p->dev, "up pp_sem\n");
			p->pp_owner = NULL;
			up(&p->pp_sem);
			break;

		default:
			dev_warn(p->dev, "unknown ioctl %x\n", cmd);
			ret = -EINVAL;
	}
	return ret;
}

const struct file_operations vdec_misc_fops = {
	.owner          =	THIS_MODULE,
	.llseek         =	no_llseek,
	.open           =	vdec_misc_open,
	.release        =	vdec_misc_release,
	.unlocked_ioctl =	vdec_misc_ioctl,
};

static struct miscdevice vdec_misc_device = {
	MISC_DYNAMIC_MINOR,
	"vdec",
	&vdec_misc_fops
};

/*
 * Platform driver related
 */

/* Should we use spin_lock_irqsave here? */
static irqreturn_t vdec_isr(int irq, void *dev_id)
{
	struct vdec_device *p = dev_id;
	u32 irq_status_dec, irq_status_pp;
	int handled = 0;
//	struct  mfc_jpeg_context *context = vde_jpeg_context;
	/* interrupt status register read */
	irq_status_dec = vdec_readl(p, VDEC_DIR);
	//printk(KERN_ALERT "irq_status=0x%x.\n", irq_status_dec);

	if (irq_status_dec & VDEC_DIR_ISET) {
		/* Clear IRQ */
		vdec_writel(p, VDEC_DIR, irq_status_dec & ~VDEC_DIR_ISET);

		p->dec_irq_done = true;
		wake_up_interruptible(&p->dec_wq);
		handled++;
	}

	irq_status_pp = vdec_readl(p, VDEC_PPIR);
	if (irq_status_pp & VDEC_PPIR_ISET) {
		/* Clear IRQ */
		vdec_writel(p, VDEC_PPIR, irq_status_pp & ~VDEC_PPIR_ISET);

		p->pp_irq_done = true;
		wake_up_interruptible(&p->pp_wq);
		handled++;
	}

	if (handled == 0) {
		dev_warn(p->dev, "Spurious IRQ (DIR=%08x PPIR=%08x)\n", \
				irq_status_dec, irq_status_pp);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int vdec_probe(struct platform_device *pdev)
{
	struct vdec_device *p;
	struct resource *res, *animres;
	int ret;
	u32 hwid;

	/* Allocate private data */
	p = devm_kzalloc(&pdev->dev, sizeof(struct vdec_device), GFP_KERNEL);
	if (!p) {
		dev_dbg(&pdev->dev, "out of memory\n");
		return -ENOMEM;
	}

	p->dev = &pdev->dev;
	platform_set_drvdata(pdev, p);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	p->mmio_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(p->mmio_base))
		return PTR_ERR(p->mmio_base);

	p->clk = devm_clk_get(&pdev->dev, "vdec_clk");
	if (IS_ERR(p->clk)) {
		dev_err(&pdev->dev, "no vdec_clk clock defined\n");
		return -ENXIO;
	}

	p->irq = platform_get_irq(pdev, 0);
	if (!p->irq) {
		dev_err(&pdev->dev, "could not get irq\n");
		return -ENXIO;
	}

	ret = devm_request_irq(&pdev->dev, p->irq, vdec_isr,
			0, pdev->name, p);
	if (ret) {
		dev_err(&pdev->dev, "unable to request VDEC irq\n");
		return ret;
	}

	/* Register the miscdevice */
	ret = misc_register(&vdec_misc_device);
	if (ret) {
		dev_err(&pdev->dev, "unable to register miscdevice\n");
		return ret;
	}

	p->num_cores = VDEC_MAX_CORES;
	p->iosize = resource_size(res);
	p->iobaseaddr = res->start;
	vdec6731_global = p;

	p->dec_irq_done = false;
	p->pp_irq_done = false;
	p->dec_owner = NULL;
	p->pp_owner = NULL;
	init_waitqueue_head(&p->dec_wq);
	init_waitqueue_head(&p->pp_wq);
	sema_init(&p->dec_sem, VDEC_MAX_CORES);
	sema_init(&p->pp_sem, 1);

	ret = clk_prepare_enable(p->clk);
	if (ret) {
		dev_err(&pdev->dev, "unable to prepare and enable clock\n");
		misc_deregister(&vdec_misc_device);
		return ret;
	}

	dev_info(&pdev->dev, "VDEC controller at 0x%p, irq = %d, misc_minor = %d\n",
			p->mmio_base, p->irq, vdec_misc_device.minor);

//MFC jpeg decode
	animres = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (IS_ERR(animres)) {
		return PTR_ERR(animres);
	}

	p->context.dev = p->dev;
	p->context.anmation_stats = 0;
	p->context.animation_data_phyaddr = animres->start;
	p->context.animation_data_size = resource_size(animres);
	p->context.animation_data_virtaddr =
	    (unsigned int)ioremap(p->context.animation_data_phyaddr, resource_size(animres));
	if (p->context.animation_data_virtaddr) {
		BANIHEADER *header = (BANIHEADER *) p->context.animation_data_virtaddr;
		if (header->magic == MKTAG('B', 'A', 'N', 'I')) {
			vde_jpeg_context = &p->context;
			p->context.animation_file_phyaddr =
			    p->context.animation_data_phyaddr + sizeof(BANIHEADER);
			p->context.animation_file_virtaddr =
			    p->context.animation_data_virtaddr + sizeof(BANIHEADER);

			p->context.animation_display_size = MAX_ANIMFRAME_SIZE * 2;
			p->context.animation_display_virtaddr =
			    (unsigned int)dma_alloc_coherent(&pdev->dev, p->context.animation_display_size,
							     &p->context.animation_display_phyaddr, GFP_KERNEL);

			if (!p->context.animation_display_virtaddr) {
				dev_err(&pdev->dev, "alloc animation display buffer failed.\n");
				p->context.animation_end = true;
			} else {
				p->context.anmation_stats = 1;
				p->context.animation_end = false;
				p->context.animation_dec_finish = false;
				p->context.animation_initdisplay = false;
				p->context.animation_queue = create_singlethread_workqueue("animation_queue");
				if(!p->context.animation_queue) {
				    printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);
				    return -1;
				}
				INIT_WORK(&p->context.animation_work, animation_dec_work);
				timer_setup(&p->context.animation_timer, animation_timer_handler, 0);
				p->context.animation_timer.expires = jiffies + 10;
				add_timer(&p->context.animation_timer);
			}
		} else {
			vde_jpeg_context = &p->context;
			p->context.animation_end = true;
		}
	} else
		p->context.animation_end = true;

	/* Reset Asic (just in case..) */
	vdec_writel(p, VDEC_DIR, VDEC_DIR_ID | VDEC_DIR_ABORT);
	vdec_writel(p, VDEC_PPIR, VDEC_PPIR_ID);

	hwid = vdec_readl(p, VDEC_IDR);

	dev_warn(&pdev->dev, "Product ID: %#x (revision %d.%d.%d)\n", \
			(hwid & VDEC_IDR_PROD_ID) >> 16,
			(hwid & VDEC_IDR_MAJOR_VER) >> 12,
			(hwid & VDEC_IDR_MINOR_VER) >> 4,
			(hwid & VDEC_IDR_BUILD_VER));
	return 0;
}

static int vdec_remove(struct platform_device *pdev)
{
	platform_set_drvdata(pdev, NULL);
	misc_deregister(&vdec_misc_device);
	return 0;
}

static const struct of_device_id vdec_of_match[] = {
	{ .compatible = "on2,ark-vdec", .data = NULL },
	{},
};
MODULE_DEVICE_TABLE(of, vdec_of_match);

static struct platform_driver vdec_of_driver = {
	.driver		= {
		.name	= "ark-vdec",
		.owner	= THIS_MODULE,
		.of_match_table	= vdec_of_match,
	},
	.probe 		= vdec_probe,
	.remove		= vdec_remove,
};
//module_platform_driver(vdec_of_driver);

static int __init ark_vdec_init(void)
{
    int ret;

    ret = platform_driver_register(&vdec_of_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s %d: failed to register vdec_of_driver\n",
            __FUNCTION__, __LINE__);
    }

    return ret;
}
fs_initcall(ark_vdec_init);

MODULE_AUTHOR("Hantro Products Oy");
MODULE_DESCRIPTION("G1 decoder/pp driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.4");
MODULE_ALIAS("platform:vdec");
