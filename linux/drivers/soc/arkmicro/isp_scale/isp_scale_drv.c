/*
 * Arkmicro ISP Scale driver
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
#include <linux/poll.h>
#include <linux/spinlock.h>

#include "isp_scale.h"

static int get_finish_frame_count(struct ark_isp_scale_context *context)
{
	int i;
	int count = 0;
	unsigned long flags = 0;
	
	spin_lock_irqsave(&context->lock, flags);
 	for (i = 0; i < context->isbuf_num; i++) {
		if (context->isbuf_status[i] == ISBUF_STATUS_READY)
			count++;
	}
	spin_unlock_irqrestore(&context->lock, flags);

	return count;
}

static void xm_isp_scalar_start(struct ark_isp_scale_context *context)
{
	writel(3, context->mmio_base + ISP_SCALE_EN);
}

static void xm_isp_scalar_stop(struct ark_isp_scale_context *context)
{
	writel(0, context->mmio_base + ISP_SCALE_EN);
}

static int ark_isp_scale_open(struct inode *inode, struct file *filp)
{
    struct ark_isp_scale_device *dev;
	struct ark_isp_scale_context *context;

    dev = container_of(inode->i_cdev, struct ark_isp_scale_device, cdev);
	context = &dev->context;
    filp->private_data = dev;

    return 0;
}

static int ark_isp_scale_fasync(int fd, struct file *filp, int mode)
{
	int ret;
    struct ark_isp_scale_device *isp_scale =
        (struct ark_isp_scale_device *)filp->private_data;

    ret = fasync_helper(fd, filp, mode, &isp_scale->async_queue_wb);

	return ret;
}

static int ark_isp_scale_release(struct inode *inode, struct file *filp)
{
    struct ark_isp_scale_device *dev;

    dev = container_of(inode->i_cdev, struct ark_isp_scale_device, cdev);

    if(filp->f_flags & FASYNC)
    {
        /* remove this filp from the asynchronusly notified filp's */
        ark_isp_scale_fasync(-1, filp, 0);
    }


    return 0;
}

static long ark_isp_scale_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
    struct ark_isp_scale_device *isp_scale =
        (struct ark_isp_scale_device *)filp->private_data;
    struct ark_isp_scale_context *context = &isp_scale->context;
	int i;
	unsigned long flags;

    switch (cmd)
    {
    case ISP_SCALE_IOCTL_INIT:
		{
			struct isp_scale_init_para para;

			if(copy_from_user(&para, (void  *)arg, sizeof(para))){
				printk("%s: copy from user init error\n", __func__);
				return -EFAULT;
			}

			xm_isp_scalar_stop(context);
			udelay(100);
			isp_scale_softreset(context);

			memset(&context->config, 0, sizeof(context->config));

			context->config.src_channel = para.src_channel;
			context->config.src_hsync_polarity = para.src_hsync_polarity;
			context->config.src_vsync_polarity = para.src_vsync_polarity;

			context->config.src_format = para.src_format;
			context->config.src_ycbcr_sequence = para.src_ycbcr_sequence;

			context->config.src_width = para.src_width;
			context->config.src_height = para.src_height;
			context->config.src_stride = context->config.src_width;
			context->config.src_window_x = para.src_window_x;
			context->config.src_window_y = para.src_window_y;
			context->config.src_window_width = para.src_window_width;
			context->config.src_window_height = para.src_window_height;

			context->config.dst_format = XM_ISP_SCALAR_FORMAT_Y_UV420;
			context->config.dst_width = para.dst_width;
			context->config.dst_height = para.dst_height;
			context->config.dst_stride = context->config.dst_width;
			context->config.dst_window_x = 0;
			context->config.dst_window_y = 0;
			context->config.dst_window_width = para.dst_width;
			context->config.dst_window_height = para.dst_height;

			context->config.mid_line = para.mid_line_counter;

			xm_isp_scalar_config (context, &context->config);
    	}
		break;
	case ISP_SCALE_IOCTL_SET_BUFFER:
		{
			struct isp_scale_sbuf_para sbuf;
			int num;

			if(copy_from_user(&sbuf, (void  *)arg, sizeof(sbuf))){
				printk("%s: copy from user sbuf error\n", __func__);
				return -EFAULT;
			}

			num = sbuf.num;
			if (num > ISBUF_FIFO_DEPTH)
				num = ISBUF_FIFO_DEPTH;

			spin_lock_irqsave(&context->lock, flags);
			context->isbuf_num = num;
			for (i = 0; i < num; i++) {
				context->isbuf[i] = sbuf.buf[i];
				context->isbuf_id[i].id = i;
			}

			spin_unlock_irqrestore(&context->lock, flags);
		}
		break;
	case ISP_SCALE_IOCTL_START:
		isp_scale_buffer_init(context);
		xm_isp_scalar_start(context);
		break;
	case ISP_SCALE_IOCTL_STOP:
		xm_isp_scalar_stop(context);
		break;
	case ISP_SCALE_IOCTL_GET_READY:
		{
			struct isp_scale_buf buf = {0};

			spin_lock_irqsave(&context->lock, flags);
            for (i = 0; i < context->isbuf_num; i++) {
                if (context->isbuf_status[i] == ISBUF_STATUS_READY)  {
					buf = context->isbuf[i];
					break;
				}
            }
			spin_unlock_irqrestore(&context->lock, flags);

			if (copy_to_user((void*)arg, &buf, sizeof(buf))) {
                printk("%s %d: copy_to_user error\n",
                    __FUNCTION__, __LINE__);
                return -EFAULT;
            }
		}
		break;
	case ISP_SCALE_IOCTL_SET_FREE:
		{
			struct isp_scale_buf buf;

			if(copy_from_user(&buf, (void  *)arg, sizeof(buf))){
                printk("%s %d: copy_from_user error\n",
                    __FUNCTION__, __LINE__);
				return -EFAULT;
			}

			spin_lock_irqsave(&context->lock, flags);
			for (i = 0; i < context->isbuf_num; i++) {
				if (buf.yaddr == context->isbuf[i].yaddr && buf.uvaddr == context->isbuf[i].uvaddr) {
					if (context->isbuf_status[i] == ISBUF_STATUS_READY)					
						context->isbuf_status[i] = ISBUF_STATUS_FREE;
					else
						printk(KERN_ALERT "app free no-ready buf %d.\n", i);
					break;
				}
			}
			spin_unlock_irqrestore(&context->lock, flags);
		}
		break;
    default:
        printk("%s %d: undefined cmd (0x%2X)\n",
            __FUNCTION__, __LINE__, cmd);
        return -EINVAL;
    }

    return 0;
}

static int ark_isp_scale_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
    struct ark_isp_scale_device *isp_scale = (struct ark_isp_scale_device *)filp->private_data;
    struct ark_isp_scale_context *context = &isp_scale->context;
	int i;
	char frames[ISBUF_FIFO_DEPTH];
	int count = 0;
	unsigned long flags;

	if (size > context->isbuf_num)
		return -EINVAL;

	wait_event_interruptible(isp_scale->frame_finish_waitq, get_finish_frame_count(context) > 0);

	spin_lock_irqsave(&context->lock, flags);
	for (i = 0; i < context->isbuf_num; i++) {
		if (context->isbuf_status[i] == ISBUF_STATUS_READY) {
			frames[count++] = i;
			if (count >= size)
				break;				
		}	
	}
	spin_unlock_irqrestore(&context->lock, flags);

	if (copy_to_user(user, frames, min(count, ISBUF_FIFO_DEPTH))) {
		printk("%s %d: copy_to_user error\n",
			__FUNCTION__, __LINE__);
		return -EFAULT;
	}

	return count;
}

static unsigned int ark_isp_scale_poll(struct file *filp, poll_table *wait)
{
    struct ark_isp_scale_device *isp_scale = (struct ark_isp_scale_device *)filp->private_data;
    struct ark_isp_scale_context *context = &isp_scale->context;
	unsigned int mask = 0;
	unsigned long flags;

	poll_wait(filp, &isp_scale->frame_finish_waitq, wait);

	if (get_finish_frame_count(context) > 0)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static struct file_operations ark_isp_scale_fops = {
    .owner          = THIS_MODULE,
    .open           = ark_isp_scale_open,
    .unlocked_ioctl = ark_isp_scale_ioctl,
    .release        = ark_isp_scale_release,
    .fasync         = ark_isp_scale_fasync,
    .read			= ark_isp_scale_read,
  	.poll			= ark_isp_scale_poll,
};

/* This function is invoked when the device module is loaded into the
 * kernel. It allocates system resources for constructing driver control
 * data structures and initializes them accordingly.
 */
static int ark_isp_scale_probe(struct platform_device *pdev)
{
    struct ark_isp_scale_device *isp_scale;
    struct resource *res;
	dev_t dev;
	void __iomem *regs;
    int error = 0;

    isp_scale = devm_kzalloc(&pdev->dev, sizeof(struct ark_isp_scale_device), GFP_KERNEL);
    if (isp_scale == NULL) {
        dev_err(&pdev->dev, "%s %d: failed to allocate memory\n",
            __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    isp_scale->driver_name = "ark_isp_scale_drv";
    isp_scale->name = "ark_isp_scale";
    isp_scale->major = 0; /* if 0, let system choose */
    isp_scale->minor_start = 0;
    isp_scale->minor_num = 1; /* one dev only */
    isp_scale->num = 1;

    /* register char device */
    if (!isp_scale->major) {
        error = alloc_chrdev_region(
                    &dev,
                    isp_scale->minor_start,
                    isp_scale->num,
                    isp_scale->name
                    );
        if (!error) {
            isp_scale->major = MAJOR(dev);
            isp_scale->minor_start = MINOR(dev);
        }
    } else {
        dev = MKDEV(isp_scale->major, isp_scale->minor_start);
        error = register_chrdev_region(dev, isp_scale->num,
            (char *)isp_scale->name);
    }

    if (error < 0) {
        dev_err(&pdev->dev, "%s %d: register driver error\n",
            __FUNCTION__, __LINE__);
        goto err_driver_register;
    }

    /* associate the file operations */
    cdev_init(&isp_scale->cdev, &ark_isp_scale_fops);
    isp_scale->cdev.owner    = THIS_MODULE; //driver->owner;
    isp_scale->cdev.ops      = &ark_isp_scale_fops;
    error = cdev_add(&isp_scale->cdev, dev, isp_scale->num);
    if (error) {
        dev_err(&pdev->dev, "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
        goto err_cdev_add;
    }

	isp_scale->isp_scale_class = class_create(THIS_MODULE, "isp_scale_class");
    if(IS_ERR(isp_scale->isp_scale_class)) {
    	dev_err(&pdev->dev, "Err: failed in creating ark isp scale class.\n");
		isp_scale->isp_scale_class = NULL;
        goto err_cdev_add;
    }

	isp_scale->isp_scale_device = device_create(isp_scale->isp_scale_class, NULL, dev, NULL, "isp_scale");
	if (IS_ERR(isp_scale->isp_scale_device)) {
    	dev_err(&pdev->dev, "Err: failed in creating ark isp scale device.\n");
		isp_scale->isp_scale_device = NULL;
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
	isp_scale->context.mmio_base = regs;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (IS_ERR(res)) {
		error = PTR_ERR(res);
		goto err_mem_res_req;
	}
	regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
	if (IS_ERR(regs)) {
		error = PTR_ERR(regs);
		goto err_mem_res_req;
	}
	isp_scale->context.sys_base = regs;

	isp_scale->context.clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(isp_scale->context.clk)) {
		dev_err(&pdev->dev, "Err: failed in getting isp_scale clock.\n");
		error = PTR_ERR(isp_scale->context.clk);
		goto err_init;
	}

    /* initialize hardware and data structure */
    error = ark_isp_scale_dev_init(&isp_scale->context);
    if (error != 0) {
        dev_err(&pdev->dev, "%s %d: dev init err\n",
            __FUNCTION__, __LINE__);
        goto err_init;
    }

    isp_scale->context.irq = platform_get_irq(pdev, 0);
	if (isp_scale->context.irq < 0) {
		dev_err(&pdev->dev, "%s %d: can't get irq resource.\n", __FUNCTION__, __LINE__);
		goto err_irq;
	}
    error = devm_request_irq(
					&pdev->dev,
                    isp_scale->context.irq,
                    ark_isp_scale_intr_handler,
                    IRQF_SHARED, //SA_SHIRQ,
                    "isp_scale",
                    isp_scale
                    );
    if (error) {
        dev_err(&pdev->dev, "%s %d: can't get assigned irq %d, error %d\n",
            __FUNCTION__, __LINE__, isp_scale->context.irq, error);
		goto err_irq;
    }

	init_waitqueue_head(&isp_scale->frame_finish_waitq);

	isp_scale->context.dev = &pdev->dev;
	platform_set_drvdata(pdev, isp_scale);

	return 0;

err_irq:
err_init:
	iounmap(isp_scale->context.sys_base);
err_mem_res_req:
	cdev_del(&isp_scale->cdev);
err_cdev_add:
	if (isp_scale->isp_scale_class) {
		if (isp_scale->isp_scale_device) {
			device_destroy(isp_scale->isp_scale_class, dev);
		}
		class_destroy(isp_scale->isp_scale_class);
	}
	unregister_chrdev_region(dev, isp_scale->num);
err_driver_register:
	return error;
}

/* This function is invoked when the device module is removed from the
 * kernel. It releases all resources to the system.
 */
static int ark_isp_scale_remove(struct platform_device *pdev)
{
    struct ark_isp_scale_device *isp_scale;
    dev_t dev;

    isp_scale = platform_get_drvdata(pdev);
    if (isp_scale == NULL)
        return -ENODEV;

	iounmap(isp_scale->context.sys_base);

    dev = MKDEV(isp_scale->major, isp_scale->minor_start);

	cdev_del(&isp_scale->cdev);

	if (isp_scale->isp_scale_class) {
		if (isp_scale->isp_scale_device) {
			device_destroy(isp_scale->isp_scale_class, dev);
		}
		class_destroy(isp_scale->isp_scale_class);
	}

	unregister_chrdev_region(dev, isp_scale->num);

    return 0;
}

static const struct of_device_id isp_scale_of_match[] = {
	{ .compatible = "arkmicro,isp-scale", },
	{ }
};
MODULE_DEVICE_TABLE(of, isp_scale_of_match);

static struct platform_driver ark_isp_scale_driver = {
    .driver     = {
        .name   = "isp_scale",
		.of_match_table = of_match_ptr(isp_scale_of_match),
    },
    .probe      = ark_isp_scale_probe,
    .remove     = ark_isp_scale_remove,
};
module_platform_driver(ark_isp_scale_driver);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("ArkMicro ISP Scale Driver");
MODULE_LICENSE("GPL v2");
