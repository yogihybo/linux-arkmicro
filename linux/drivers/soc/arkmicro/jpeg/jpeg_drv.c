/*
 * Arkmicro 1668 jpeg driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/poll.h>

#include "ark_jpeg_io.h"
#include "jpeg.h"
#include "jpeg_priv.h"

extern int ark_bootanimation_display_init(int width, int height, unsigned int addr);
extern int ark_bootanimation_display_uninit(void);
extern int ark_bootanimation_set_display_addr(unsigned int addr);

struct platform_device *ark_jpeg_platform_device = NULL;

struct decode_buffer {
	unsigned decode_kernel_phys_base;
	unsigned decode_size;
};

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

static void animation_jpeg_decode(struct ark_jpeg_context *context, unsigned int width, unsigned int height,
				  unsigned int src_addr, unsigned int dest_addr)
{
	unsigned int val;

	val = readl(context->mmio_base + JPEG_CTRL);
	writel(val | (3 << 0), context->mmio_base + JPEG_CTRL);
	writel(val & ~(3 << 0), context->mmio_base + JPEG_CTRL);

	writel((1 << 8) | (1 << 3), context->mmio_base + JPEG_1);
	writel(0x28000738, context->mmio_base + JPEG_CTRL);
	writel(0xff, context->mmio_base + JPEG_COUNT);
	writel(0x2f, context->mmio_base + JPEG_INTMASK);
	writel(src_addr, context->mmio_base + JPEG_DEC_RD_BASE_ADDR);
	writel(dest_addr, context->mmio_base + JPEG_WRSTA);
	writel(dest_addr + width * height * 2, context->mmio_base + JPEG_WREND);
	writel(0, context->mmio_base + JPEG_WRSTA1);
	writel(0, context->mmio_base + JPEG_WREND1);
	writel(0, context->mmio_base + JPEG_WRSTA2);
	writel(0, context->mmio_base + JPEG_WREND2);
	writel(readl(context->mmio_base + JPEG_CTRL) | (1 << 13), context->mmio_base + JPEG_CTRL);
	writel(1, context->mmio_base + JPEG_0);
	writel(readl(context->mmio_base + JPEG_START) | (1 << 31), context->mmio_base + JPEG_START);
}

//extern int ark_carback_get_status(void);
static void animation_timer_handler(struct timer_list *t)
{
	static unsigned int frame = 0;
	static int timeout_count = 0;
	struct ark_jpeg_context *context = from_timer(context, t, animation_timer);
	BANIHEADER *header = (BANIHEADER *) context->animation_data_virtaddr;
	unsigned int size =
	    *(unsigned int *)(context->animation_data_virtaddr + context->animation_file_phyaddr -
			      context->animation_data_phyaddr);

	if (timeout_count > 50) {
		context->animation_end = true;
	}

	if (context->animation_end) {
		if (context->animation_display_phyaddr)
			dma_free_coherent(context->dev, context->animation_display_size,
					  (void *)context->animation_display_virtaddr,
					  context->animation_display_phyaddr);
		ark_bootanimation_display_uninit();
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
		}
		return;
	}

	frame++;
	context->animation_dec_finish = false;
	animation_jpeg_decode(context, header->aniWidth, header->aniHeight, context->animation_file_phyaddr + 4,
					context->animation_display_phyaddr + header->aniWidth * header->aniHeight * 2 *
					context->animation_display_index);
	context->animation_file_phyaddr += size;

	if (header->hasBootlogo && frame == 1)
		mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(header->bootlogoDisplayTime));
	else
		mod_timer(&context->animation_timer, jiffies + msecs_to_jiffies(1000 / header->aniFps));
}

/* This is the first function being called when opening the device
 * driver. It identifies the driver-specific data structure of the device
 * being opened and stores the start pointer of the data structure in the 
 * private_data field of the file structure, i.e. filp->private_data, 
 * for easier access in the future.
 *
 * Arguments:
 *   inode : kernel data structure that represents the device file of the
 *           driver on disk
 *   filp  : a pointer to the file structure that is created by the kernel
 *           to represent the file opened for the device driver
 *
 * Return:
 *   0
 */
static int ark_jpeg_open(struct inode *inode, struct file *filp)
{
	struct ark_jpeg_device *dev;
	struct ark_jpeg_context *context;

	dev = container_of(inode->i_cdev, struct ark_jpeg_device, cdev);
	context = &dev->context;
	filp->private_data = dev;

	return 0;
}

/* This function is invoked when the associated file structure is being
 * released.
 *
 * Arguments:
 *   inode : kernel data structure that represents the device file of the
 *           driver on disk
 *   filp  : a pointer to the file structure that is created by the kernel
 *           to represent the file opened for the device driver
 *
 * Return:
 *   0
 */
static int ark_jpeg_release(struct inode *inode, struct file *filp)
{
	struct ark_jpeg_device *dev;

	dev = container_of(inode->i_cdev, struct ark_jpeg_device, cdev);

	/* NOTE: intend not to do anything here */

	return 0;
}

/* This function is invoked to handle ioctl system calls and implement
 * the requested ioctl command. It validates the ioctl command and performs
 * the requested device control function. If the command number does not
 * match a valid operation, it returns -ENOTTY. Otherwise, it returns 0
 *
 * Arguments:
 *   inode : kernel data structure that represents the device file of the
 *           driver on disk
 *   filp  : a pointer to the file structure that is created by the kernel
 *           to represent the file opened for the device driver
 *
 * Return:
 *   0
 */
static long ark_jpeg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ark_jpeg_device *jpeg =
        (struct ark_jpeg_device *)file->private_data;
    struct ark_jpeg_context *context = &jpeg->context;

    switch (cmd) {
	case ARKJPEG_SET_DECODE_OPT:
		{
			JPEG_DECODE_OPT input_arg;	
			//printk("ARKJPEG_SET_DECODE_OPT [%s][%d]\n", __func__, __LINE__);
	        if (arg == (unsigned long)NULL) {
	            printk("%s %d: null arg error\n",
	                __FUNCTION__, __LINE__);
	            return -EINVAL;
	        }

	        if (copy_from_user(&input_arg, (void*)arg, sizeof(input_arg))) {
	            printk("%s %d: copy_from_user error\n",
	                __FUNCTION__, __LINE__);
	            return -EFAULT;
	        }

			context->scaler_mode = input_arg.ScalerMode;
			context->zoom_mode = input_arg.ZoomMode;
			context->rotate_angle = input_arg.RotateAngle;
			context->dst_width = input_arg.dwDestWidth;
			context->dst_height = input_arg.dwDestHeight;
			context->format = input_arg.format;
			context->repeat_src_height = input_arg.RepeatdwSrcHeight;
			context->repeat_src_width= input_arg.RepeatdwSrcWidth;
		}
		break;
	case ARKJPEG_SET_JPG_SIZE:
		{
			unsigned int input_arg;
			//printk("ARKJPEG_SET_JPG_SIZE [%s][%d]\n", __func__, __LINE__);
			if (arg == (unsigned long)NULL) {
	            ARKJPEG_DBGPRTK("%s %d: null arg error\n",
	                __FUNCTION__, __LINE__);
	            return -EINVAL;
	        }

	        if (copy_from_user(&input_arg, (void*)arg, sizeof(input_arg))) {
	            ARKJPEG_DBGPRTK("%s %d: copy_from_user error\n",
	                __FUNCTION__, __LINE__);
	            return -EFAULT;
	        }	

			context->file_size = input_arg;
		}
		break;
	case ARKJPEG_DECODE:
		//printk("ARKJPEG_DECODE [%s][%d]\n", __func__, __LINE__);
		complete(&context->decstart_completion);
		break;
	case ARKJPEG_BREAK_DECODE:
		//printk("ARKJPEG_BREAK_DECODE [%s][%d]\n", __func__, __LINE__);
		context->break_decode = true;
		break;
	case ARKJPEG_GET_DECSTATUS:
		{
			JPEG_DEC_STATUS output_arg;
			//printk("ARKJPEG_GET_DECSTATUS [%s][%d]\n", __func__, __LINE__);
	        if (arg == (unsigned long)NULL) {
	            ARKJPEG_DBGPRTK("%s %d: null arg error\n",
	                __FUNCTION__, __LINE__);
	            return -EINVAL;
	        }

			output_arg = context->decode_status;
			
            if (copy_to_user((void*)arg, &output_arg, sizeof(output_arg))) {
                ARKJPEG_DBGPRTK("%s %d: copy_to_user error\n",
                    __FUNCTION__, __LINE__);
                return -EFAULT;
            }
		}	
		break;
	case ARKJPEG_SET_BUFFER:
		{
			struct jpeg_buffer buffer;

			if (arg == (unsigned long)NULL) {
				ARKJPEG_DBGPRTK("%s %d: null arg error\n",
				__FUNCTION__, __LINE__);
			  	return -EINVAL;
			}
			if (copy_from_user(&buffer, (void*)arg, sizeof(buffer))) {
			  ARKJPEG_DBGPRTK("%s %d: copy_to_user error\n",
				__FUNCTION__, __LINE__);
				return -EFAULT;
			}				

			context->buf_size = buffer.file_size;
			context->buf_base_virt = phys_to_virt(buffer.file_base_phys);
			context->buf_base_phys = buffer.file_base_phys;

			context->decode_buf_size = buffer.decode_size;
			context->decode_buf_base_phys = buffer.decode_base_phys;
			context->decode_buf_base_virt  = phys_to_virt(buffer.decode_base_phys);
		}
		break;
	case ARKJPEG_GET_BUFFER:
		break;

	case ARKJPEG_GET_DECODEINFO:
		{
			JPEG_DECODE_INFO output_arg;
			//printk("ARKJPEG_GET_DECODEINFO [%s][%d]", __func__, __LINE__);
	        if (arg == (unsigned long)NULL) {
	            ARKJPEG_DBGPRTK("%s %d: null arg error\n",
	                __FUNCTION__, __LINE__);
	            return -EINVAL;
	        }

			output_arg.DecResult = context->decode_result;
			output_arg.dwDecSize = context->decode_size;
			output_arg.dwSrcWidth = context->src_width;
			output_arg.dwSrcHeight = context->src_height;
			output_arg.dwOutWidth = context->out_width;
			output_arg.dwOutHeight = context->out_height;
			output_arg.RepeatdwSrcHeight = context->repeat_src_height;
			output_arg.RepeatdwSrcWidth = context->repeat_src_width;
            if (copy_to_user((void*)arg, &output_arg, sizeof(output_arg))) {
                ARKJPEG_DBGPRTK("%s %d: copy_to_user error\n",
                    __FUNCTION__, __LINE__);
                return -EFAULT;
            }			
		}
		break;

	case ARKJPEG_GET_APIEVENT:
		{
			int ret;
			//printk("ARKJPEG_GET_APIEVENT  [%s][%d]\n", __func__, __LINE__);
	        if (arg == (unsigned long)NULL) {
	            ARKJPEG_DBGPRTK("%s %d: null arg error\n",
	                __FUNCTION__, __LINE__);
	            return -EINVAL;
	        }
			
			ret = wait_for_completion_timeout(&context->api_completion, msecs_to_jiffies(JPEG_API_TIMEOUT));

			if(ret == 0)
			{
                ARKJPEG_DBGPRTK("%s %d: wait api_completion timeout\n",
                    __FUNCTION__, __LINE__);
				return -EFAULT;
			}
			
            if (copy_to_user((void*)arg, &context->api_info, sizeof(context->api_info))) {
                ARKJPEG_DBGPRTK("%s %d: copy_to_user error\n",
                    __FUNCTION__, __LINE__);
                return -EFAULT;
            }				
		}
		break;
	case ARKJPEG_SET_APIDONEEVENT:
		{	
			//printk("ARKJPEG_SET_APIDONEEVENT [%s][%d]\n", __func__, __LINE__);
			if (arg == (unsigned long)NULL) {
	            ARKJPEG_DBGPRTK("%s %d: null arg error\n",
	                __FUNCTION__, __LINE__);
	            return -EINVAL;
	        }

	        if (copy_from_user(&context->api_retinfo, (void*)arg, sizeof(context->api_retinfo))) {
	            ARKJPEG_DBGPRTK("%s %d: copy_from_user error\n",
	                __FUNCTION__, __LINE__);
	            return -EFAULT;
	        }
			
			complete(&context->apidone_completion);
		}
		break;
    default:
        ARKJPEG_DBGPRTK("%s %d: undefined cmd (0x%2X)\n",
            __FUNCTION__, __LINE__, cmd);
        return -EINVAL;
    }

	return 0;
}

static struct file_operations ark_jpeg_fops = {
	.owner = THIS_MODULE,
	.open = ark_jpeg_open,
	.unlocked_ioctl = ark_jpeg_ioctl,
	.release = ark_jpeg_release,
};

static int jpeg_decode_thread(void *data)
{
	struct ark_jpeg_context *context = (struct ark_jpeg_context *)data;
	while (1) {
		wait_for_completion(&context->decstart_completion);
		ARKJPEG_DBGPRTK("jpeg_decode_thread\n");
		JpegPicDec();

		context->api_info.EventType = DEC_OVER;
		context->api_info.DecResult = context->decode_result;
		complete(&context->api_completion);
	}
	return 0;
}

static void jpeg_int_work(struct work_struct *work)
{
	struct ark_jpeg_context *context = container_of(work, struct ark_jpeg_context, jpeg_work);
	BANIHEADER *header = (BANIHEADER *) context->animation_data_virtaddr;

	//printk(KERN_ALERT "jpeg_int_work 0x%x\n", context->intr_status);

	if (!context->animation_end) {
		if (context->intr_status & 0x4) {
			if (!context->animation_initdisplay) { 
				ark_bootanimation_display_init(header->aniWidth, header->aniHeight, 
										context->animation_display_phyaddr);
				context->animation_initdisplay = true;
			} else {
				ark_bootanimation_set_display_addr(context->animation_display_phyaddr +
					header->aniWidth * header->aniHeight * 2 * context->animation_display_index);
				context->animation_display_index = !context->animation_display_index;
			}
			context->animation_dec_finish = true;
		} else if (context->intr_status & 0x1) {
			int dec_width = (readl(context->mmio_base + JPEG_1) >> 16) & 0x0000ffff;
			int dec_height = (readl(context->mmio_base + JPEG_3) >> 16) & 0x0000ffff;
			if (dec_width == header->aniWidth && dec_height == header->aniHeight) {
				JpegContinueWork();
			} else {
				printk(KERN_ALERT "decode boot animation jpeg error.\n");
				context->animation_dec_finish = true;
			}
		}
		return;
	}

	if (context->intr_status & 0x4) {
		if (JpegFrameIntHandler())
			context->decode_result = DEC_SUCCESS;
		complete(&context->decdone_completion);
	} else if (context->intr_status & 0x20) {
		if (!JpegBufferIntHandler())
			complete(&context->decdone_completion);
		else
			JpegContinueWork();
	} else if (context->intr_status & 0x1) {
		if (!JpegHeaderIntHandler())
			complete(&context->decdone_completion);
		else
			JpegContinueWork();
	} else if (context->intr_status & 0x2) {
		if (!JpegBlockIntHandler())
			complete(&context->decdone_completion);
		else
			JpegContinueWork();
	} else {
		printk(KERN_ALERT "JPG:: Jpeg error int!\n");
	}
}

/* This function is invoked when the device module is loaded into the
 * kernel. It allocates system resources for constructing driver control
 * data structures and initializes them accordingly.
 */
static int ark_jpeg_probe(struct platform_device *pdev)
{
	struct ark_jpeg_device *jpeg;
	struct resource *res;
	void __iomem *regs;
	dev_t dev;
	int error = 0;

	jpeg = devm_kzalloc(&pdev->dev, sizeof(struct ark_jpeg_device), GFP_KERNEL);
	if (jpeg == NULL) {
		dev_err(&pdev->dev, "%s %d: failed to allocate memory\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	jpeg->driver_name = "ark_jpeg_drv";
	jpeg->name = "ark_jpeg";
	jpeg->major = 0;	/* if 0, let system choose */
	jpeg->minor_start = 0;
	jpeg->minor_num = 1;	/* one dev only */
	jpeg->num = 1;

	/* register char device */
	if (!jpeg->major) {
		error = alloc_chrdev_region(&dev, jpeg->minor_start, jpeg->num, jpeg->name);
		if (!error) {
			jpeg->major = MAJOR(dev);
			jpeg->minor_start = MINOR(dev);
		}
	} else {
		dev = MKDEV(jpeg->major, jpeg->minor_start);
		error = register_chrdev_region(dev, jpeg->num, (char *)jpeg->name);
	}

	if (error < 0) {
		dev_err(&pdev->dev, "%s %d: register driver error\n", __FUNCTION__, __LINE__);
		goto err_driver_register;
	}

	/* associate the file operations */
	cdev_init(&jpeg->cdev, &ark_jpeg_fops);
	jpeg->cdev.owner = THIS_MODULE;	//driver->owner;
	jpeg->cdev.ops = &ark_jpeg_fops;
	error = cdev_add(&jpeg->cdev, dev, jpeg->num);
	if (error) {
		dev_err(&pdev->dev, "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
		goto err_cdev_add;
	}

	jpeg->jpeg_class = class_create(THIS_MODULE, "ark_jpeg_class");
	if (IS_ERR(jpeg->jpeg_class)) {
		dev_err(&pdev->dev, "Err: failed in creating ark jpeg class.\n");
		jpeg->jpeg_class = NULL;
		goto err_cdev_add;
	}

	jpeg->jpeg_device = device_create(jpeg->jpeg_class, NULL, dev, NULL, "ark_jpeg");
	if (IS_ERR(jpeg->jpeg_device)) {
		dev_err(&pdev->dev, "Err: failed in creating ark jpeg device.\n");
		jpeg->jpeg_device = NULL;
		goto err_cdev_add;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(res)) {
		error = PTR_ERR(res);
		goto err_mem_res_req;
	}
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs)) {
		error = PTR_ERR(regs);
		goto err_mem_res_req;
	}
	jpeg->context.mmio_base = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (IS_ERR(res)) {
		error = PTR_ERR(res);
		goto err_mem_res_req;
	}
	jpeg->context.animation_data_phyaddr = res->start;
	jpeg->context.animation_data_virtaddr =
	    (unsigned int)ioremap(jpeg->context.animation_data_phyaddr, resource_size(res)); 
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!IS_ERR(res)) {
		regs = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(regs)) {
			error = PTR_ERR(regs);
			goto err_mem_res_req;
		}
		jpeg->context.ps_mmio_base = regs;
	}
	
	/* initialize hardware and data structure */
	error = ark_jpeg_dev_init(&jpeg->context);
	if (error != 0) {
		printk(KERN_ERR "%s %d: dev init err\n", __FUNCTION__, __LINE__);
		goto err_init;
	} 

	jpeg->context.irq = platform_get_irq(pdev, 0);
	if (jpeg->context.irq < 0) {
		dev_err(&pdev->dev, "%s %d: can't get irq resource.\n", __FUNCTION__, __LINE__);
		goto err_irq;
	}
	error = devm_request_irq(&pdev->dev, jpeg->context.irq, ark_jpeg_intr_handler, IRQF_SHARED,	//SA_SHIRQ,
				 "jpeg", jpeg);
	if (error) {
		dev_err(&pdev->dev, "%s %d: can't get assigned irq %d, error %d\n", __FUNCTION__, __LINE__,
			jpeg->context.irq, error);
		goto err_irq;
	}

	jpeg->context.psirq = platform_get_irq(pdev, 1);
	if (jpeg->context.psirq > 0) {
		error = devm_request_irq(&pdev->dev, jpeg->context.psirq, ark_prescale_intr_handler, IRQF_SHARED,	//SA_SHIRQ,
					 "prescale", jpeg);
		if (error) {
			dev_err(&pdev->dev, "%s %d: can't get assigned prescale irq %d, error %d\n", __FUNCTION__, __LINE__,
				jpeg->context.psirq, error);
			goto err_irq;
		}
	}

	if (jpeg->context.ps_mmio_base && jpeg->context.psirq > 0) {
		writel(3, jpeg->context.ps_mmio_base + PRESCALE_INT_CLR);
		writel(3, jpeg->context.ps_mmio_base + PRESCALE_INT_MASK);
		init_completion(&jpeg->context.psblockint_completion);
		init_completion(&jpeg->context.psframeint_completion);		
	}

	jpeg->context.dev = &pdev->dev;

	init_completion(&jpeg->context.decstart_completion);
	init_completion(&jpeg->context.api_completion);
	init_completion(&jpeg->context.apidone_completion);
	init_completion(&jpeg->context.decdone_completion);
	jpeg->context.wrok_queue = create_singlethread_workqueue("jpeg_queue");
	if (!jpeg->context.wrok_queue) {
		printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n", __FUNCTION__, __LINE__);
	}
	INIT_WORK(&jpeg->context.jpeg_work, jpeg_int_work);

	kthread_run(jpeg_decode_thread, &jpeg->context, "jpeg_decode");

	if (jpeg->context.animation_data_virtaddr) {
		BANIHEADER *header = (BANIHEADER *) jpeg->context.animation_data_virtaddr;
		if (header->magic == MKTAG('B', 'A', 'N', 'I')) {
			jpeg->context.animation_file_phyaddr =
			    jpeg->context.animation_data_phyaddr + sizeof(BANIHEADER); 
			jpeg->context.animation_display_size = header->aniWidth * header->aniHeight * 2 * 2; 
			jpeg->context.animation_display_virtaddr =
			    (unsigned int)dma_alloc_coherent(&pdev->dev, jpeg->context.animation_display_size,
							     &jpeg->context.animation_display_phyaddr, GFP_KERNEL); 
			if (!jpeg->context.animation_display_virtaddr) {
				dev_err(&pdev->dev, "alloc animation display buffer failed.\n");
				jpeg->context.animation_end = true;
			} else {
				jpeg->context.animation_end = false;
				jpeg->context.animation_dec_finish = false;
				jpeg->context.animation_initdisplay = false;
				timer_setup(&jpeg->context.animation_timer, animation_timer_handler, 0);
				jpeg->context.animation_timer.expires = jiffies + 1;
				add_timer(&jpeg->context.animation_timer);
			}
		} else {
			jpeg->context.animation_end = true;
		}
	} else
		jpeg->context.animation_end = true;

	platform_set_drvdata(pdev, jpeg);

	return 0;

err_irq:
err_init:
err_mem_res_req:
	cdev_del(&jpeg->cdev);
err_cdev_add:
	if (jpeg->context.animation_data_virtaddr)
		iounmap((void *)jpeg->context.animation_data_virtaddr);
	if (jpeg->jpeg_class) {
		if (jpeg->jpeg_device) {
			device_destroy(jpeg->jpeg_class, dev);
		}
		class_destroy(jpeg->jpeg_class);
	}
	unregister_chrdev_region(dev, jpeg->num);
err_driver_register:
	return error;
}

/* This function is invoked when the device module is removed from the
 * kernel. It releases all resources to the system.
 */
static int ark_jpeg_remove(struct platform_device *pdev)
{
	struct ark_jpeg_device *jpeg;
	dev_t dev;

	jpeg = platform_get_drvdata(pdev);
	if (jpeg == NULL)
		return -ENODEV;

	dev = MKDEV(jpeg->major, jpeg->minor_start);

	if (jpeg->context.wrok_queue)
		destroy_workqueue(jpeg->context.wrok_queue);

	cdev_del(&jpeg->cdev);

	if (jpeg->jpeg_class) {
		if (jpeg->jpeg_device) {
			device_destroy(jpeg->jpeg_class, dev);
		}
		class_destroy(jpeg->jpeg_class);
	}
	unregister_chrdev_region(dev, jpeg->num);

	return 0;
}

static const struct of_device_id jpeg_of_match[] = {
	{.compatible = "arkmicro,ark-jpeg",},
	{}
};

MODULE_DEVICE_TABLE(of, jpeg_of_match);

static struct platform_driver ark_jpeg_driver = {
	.driver = {
		   .name = "ark-jpeg",
		   .of_match_table = of_match_ptr(jpeg_of_match),
		   },
	.probe = ark_jpeg_probe,
	.remove = ark_jpeg_remove,
};

//module_platform_driver(ark_jpeg_driver);
static int __init ark1668_jpeg_init(void)
{
    int ret;

    ret = platform_driver_register(&ark_jpeg_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s %d: failed to register ark1668_lcdfb_driver\n",
            __FUNCTION__, __LINE__);
    }

    return ret;
}
device_initcall(ark1668_jpeg_init);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("ArkMicro 1668 Jpeg Driver");
MODULE_LICENSE("GPL v2");
