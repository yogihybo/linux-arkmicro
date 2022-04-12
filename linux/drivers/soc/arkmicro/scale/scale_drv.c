/*
 * Arkmicro Scale driver
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
#include <linux/of_device.h>
#include <linux/irq.h>

#include "scale.h"

struct ark_scale_context *scale_context = NULL;

static int ark_scale_open(struct inode *inode, struct file *filp)
{
    struct ark_scale_device *dev;
	struct ark_scale_context *context;

    dev = container_of(inode->i_cdev, struct ark_scale_device, cdev);
	context = &dev->context;
    filp->private_data = dev;

    return 0;
}

static int ark_scale_fasync(int fd, struct file *filp, int mode)
{
	int ret;
    struct ark_scale_device *scale =
        (struct ark_scale_device *)filp->private_data;

    ret = fasync_helper(fd, filp, mode, &scale->async_queue_wb);

	return ret;
}

static int ark_scale_release(struct inode *inode, struct file *filp)
{
    struct ark_scale_device *dev;

    dev = container_of(inode->i_cdev, struct ark_scale_device, cdev);

    if(filp->f_flags & FASYNC)
    {
        /* remove this filp from the asynchronusly notified filp's */
        ark_scale_fasync(-1, filp, 0);
    }

    return 0;
}

static long ark_scale_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
	int err = 0;
	struct ark_scale_device *scale = (struct ark_scale_device *)filp->private_data;
	struct ark_scale_context *context = &scale->context;

	switch (cmd)
	{
	    case SCALE_IOCTL_START:
		case SCALE_IOCTL_START_NO_WAIT:
		{
			struct ark_scale_param input_arg;
			if(copy_from_user(&input_arg, (void*)arg, sizeof(input_arg))) {
				printk(KERN_ERR "%s, SCALE_IOCTL_START copy_from_user failed\n", __FUNCTION__);
				return -EFAULT;
			}
			if(cmd == SCALE_IOCTL_START) {
				err = ark_scale_start(context, &input_arg);
			} else {
				err = ark_scale_start_nowait(context, &input_arg);
			}
			break;
		}

		//Old interface only for arkn141,you'd better not use it, please use SCALE_IOCTL_START above.
		case ARKN141_SCALE_IOCTL_START_OLD:
		{
			struct ark_scale_param input_arg;
			memset(&input_arg, 0, sizeof(input_arg));
			if(copy_from_user(&input_arg, (void*)arg, sizeof(struct ark_scale_param_old))) {
				printk("%s, copy_from_user failed\n", __FUNCTION__);
				return -EFAULT;
			}
			err = ark_scale_start(context, &input_arg);
			break;
		}
		case SCALE_IOCTL_WAIT_IDLE:
		{
			err = ark_scale_wait_idle(context);
			break;
		}
		case SCALE_IOCTL_GET_BUSY_SYATUS:
		{
			int busy = ark_scale_get_busy_status(context);
			if(copy_to_user((void*)arg, &busy, sizeof(int))) {
				printk(KERN_ERR "%s, SCALE_IOCTL_GET_BUSY_SYATUS copy_to_user failed\n", __FUNCTION__);
				return -EFAULT;
			}
			break;
		}
	    default:
			printk("%s %d: undefined cmd (0x%2X)\n", __FUNCTION__, __LINE__, cmd);
			err = -EINVAL;
		break;
	}

	return err;
}

static struct file_operations ark_scale_fops = {
    .owner          = THIS_MODULE,
    .open           = ark_scale_open,
    .unlocked_ioctl = ark_scale_ioctl,
    .release        = ark_scale_release,
    .fasync         = ark_scale_fasync,
};

static const struct of_device_id axi_scale_of_match[] = {
	{ .compatible = "arkmicro,arkn141-axi-scale", },
	{ .compatible = "arkmicro,ark1668-axi-scale", },
	{ .compatible = "arkmicro,ark1668e-axi-scale", },
	{ }
};
MODULE_DEVICE_TABLE(of, axi_scale_of_match);

/* This function is invoked when the device module is loaded into the
 * kernel. It allocates system resources for constructing driver control
 * data structures and initializes them accordingly.
 */
static int ark_axi_scale_probe(struct platform_device *pdev)
{
    struct ark_scale_device *scale;
    struct resource *res;
	const struct of_device_id *match = NULL;
	int int_trigger_type = 0;
	dev_t dev;
    void __iomem *regs;
    int error = 0;

    scale = devm_kzalloc(&pdev->dev, sizeof(struct ark_scale_device), GFP_KERNEL);
    if (scale == NULL) {
        dev_err(&pdev->dev, "%s %d: failed to allocate memory\n",
            __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

	scale_context = &scale->context;
	sema_init(&scale_context->scale_sem, 1);

	match = of_match_device(axi_scale_of_match, &pdev->dev);
	if(!strcmp(match->compatible,axi_scale_of_match[SOC_TYPE_ARKN141].compatible)){
		//match arkn141
		scale->context.soc_type = SOC_TYPE_ARKN141;
		int_trigger_type = IRQF_SHARED;
	} else if(!strcmp(match->compatible,axi_scale_of_match[SOC_TYPE_ARK1668].compatible)) {
		//match ark1668
		scale->context.soc_type = SOC_TYPE_ARK1668;
		int_trigger_type = IRQF_TRIGGER_RISING;
	} else if(!strcmp(match->compatible,axi_scale_of_match[SOC_TYPE_ARK1668E].compatible)) {
		//match ark1668e
		scale->context.soc_type = SOC_TYPE_ARK1668E;
		int_trigger_type = IRQF_TRIGGER_RISING;
	} else {
		printk("%s, Invalid soc_type:%d\n", __FUNCTION__, scale->context.soc_type);
		return -EINVAL;
	}
    scale->driver_name = "ark_axi_scale_drv";
    scale->name = "ark_axi_scale";
    scale->major = 0; /* if 0, let system choose */
    scale->minor_start = 0;
    scale->minor_num = 1; /* one dev only */
    scale->num = 1;

    /* register char device */
    if (!scale->major) {
        error = alloc_chrdev_region(
                    &dev,
                    scale->minor_start,
                    scale->num,
                    scale->name
                    );
        if (!error) {
            scale->major = MAJOR(dev);
            scale->minor_start = MINOR(dev);
        }
    } else {
        dev = MKDEV(scale->major, scale->minor_start);
        error = register_chrdev_region(dev, scale->num,
            (char *)scale->name);
    }

    if (error < 0) {
        dev_err(&pdev->dev, "%s %d: register driver error\n",
            __FUNCTION__, __LINE__);
        goto err_driver_register;
    }

    /* associate the file operations */
    cdev_init(&scale->cdev, &ark_scale_fops);
    scale->cdev.owner    = THIS_MODULE; //driver->owner;
    scale->cdev.ops      = &ark_scale_fops;
    error = cdev_add(&scale->cdev, dev, scale->num);
    if (error) {
        dev_err(&pdev->dev, "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
        goto err_cdev_add;
    }

	scale->scale_class = class_create(THIS_MODULE, "scale_class");
    if(IS_ERR(scale->scale_class)) {
    	dev_err(&pdev->dev, "Err: failed in creating ark scale class.\n");
		scale->scale_class = NULL;
        goto err_cdev_add;
    }

	scale->scale_device = device_create(scale->scale_class, NULL, dev, NULL, "axi_scale");
	if (IS_ERR(scale->scale_device)) {
    	dev_err(&pdev->dev, "Err: failed in creating ark scale device.\n");
		scale->scale_device = NULL;
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
	scale->context.mmio_base = regs;

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
	scale->context.sys_base = regs;

    scale->context.clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(scale->context.clk)) {
		dev_err(&pdev->dev, "Err: failed in getting scale clock.\n");
		error = PTR_ERR(scale->context.clk);
		goto err_init;
	}
	if(of_property_read_u32(pdev->dev.of_node, "softreset-reg", &scale->context.softreset_reg)) {
		scale->context.softreset_reg = -1;
	}
	if(of_property_read_u32(pdev->dev.of_node, "softreset-offset", &scale->context.softreset_offset)) {
		scale->context.softreset_offset = -1;
	}

    /* initialize hardware and data structure */
    error = ark_scale_dev_init(&scale->context);
    if (error != 0) {
        dev_err(&pdev->dev, "%s %d: dev init err\n",
            __FUNCTION__, __LINE__);
        goto err_init;
    }

    scale->context.irq = platform_get_irq(pdev, 0);
	if (scale->context.irq < 0) {
		dev_err(&pdev->dev, "%s %d: can't get irq resource.\n", __FUNCTION__, __LINE__);
		goto err_irq;
	}
    error = devm_request_irq(
                    &pdev->dev,
                    scale->context.irq,
                    ark_scale_intr_handler,
                    int_trigger_type,//IRQF_SHARED, //SA_SHIRQ,
                    "axi_scale",
                    scale
                    );
    if (error) {
        dev_err(&pdev->dev, "%s %d: can't get assigned irq %d, error %d\n",
            __FUNCTION__, __LINE__, scale->context.irq, error);
        goto err_irq;
    }

	scale->context.dev = &pdev->dev;
	platform_set_drvdata(pdev, scale);

	return 0;

err_irq:
err_init:
	iounmap(scale->context.sys_base);
err_mem_res_req:
	cdev_del(&scale->cdev);
err_cdev_add:
	if (scale->scale_class) {
		if (scale->scale_device) {
			device_destroy(scale->scale_class, dev);
		}
		class_destroy(scale->scale_class);
	}
	unregister_chrdev_region(dev, scale->num);
err_driver_register:
	return error;
}


/* This function is invoked when the device module is removed from the
 * kernel. It releases all resources to the system.
 */
static int ark_axi_scale_remove(struct platform_device *pdev)
{
    struct ark_scale_device *scale;
    dev_t dev;

    scale = platform_get_drvdata(pdev);
    if (scale == NULL)
        return -ENODEV;

    dev = MKDEV(scale->major, scale->minor_start);

    cdev_del(&scale->cdev);

	if (scale->scale_class) {
		if (scale->scale_device) {
			device_destroy(scale->scale_class, dev);
		}
		class_destroy(scale->scale_class);
	}
	unregister_chrdev_region(dev, scale->num);

    return 0;
}

static struct platform_driver ark_axi_scale_driver = {
    .driver     = {
        .name   = "axi_scale",
		.of_match_table = of_match_ptr(axi_scale_of_match),
    },
    .probe      = ark_axi_scale_probe,
    .remove     = ark_axi_scale_remove,
};
module_platform_driver(ark_axi_scale_driver);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("ArkMicro Scaler Driver");
MODULE_LICENSE("GPL v2");
