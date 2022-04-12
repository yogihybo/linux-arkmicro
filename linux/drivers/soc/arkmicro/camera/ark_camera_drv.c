/*
 * Arkmicro ISP Camera driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/gpio/consumer.h>
#include <linux/clk.h>
#include "ark_camera.h"
#include "Gem_isp.h"
#include "Gem_isp_sys.h"


static struct ark_camera_device *ark_camera = NULL;

unsigned long isp_get_clk(void)
{
	if(ark_camera)
		return clk_get_rate(ark_camera->isp_clk);

	return 0;
}
EXPORT_SYMBOL(isp_get_clk);

unsigned long isp_get_sensor_mclk(void)
{
	if(ark_camera)
		return clk_get_rate(ark_camera->sensor_mclk);

	return 0;
}
EXPORT_SYMBOL(isp_get_sensor_mclk);

static int ark_camera_open(struct inode *inode, struct file *filp)
{
    struct ark_camera_device *dev;
	struct ark_camera_context *context;

    dev = container_of(inode->i_cdev, struct ark_camera_device, cdev);
	context = &dev->context;
    filp->private_data = dev;

    return 0;
}

static int ark_camera_fasync(int fd, struct file *filp, int mode)
{
	int ret;
    struct ark_camera_device *camera =
        (struct ark_camera_device *)filp->private_data;

    ret = fasync_helper(fd, filp, mode, &camera->async_queue_cam);

	return ret;
}

static int ark_camera_release(struct inode *inode, struct file *filp)
{
    struct ark_camera_device *dev;

    dev = container_of(inode->i_cdev, struct ark_camera_device, cdev);

    if(filp->f_flags & FASYNC)
    {
        /* remove this filp from the asynchronusly notified filp's */
        ark_camera_fasync(-1, filp, 0);
    }


    return 0;
}

static int ark_camera_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
    int count;
	struct ark_camera_device *camera = (struct ark_camera_device *)filp->private_data;
	if (size > ISP_FRAME_NUM)
		return -EINVAL;

	wait_event_interruptible(camera->context.frame_finish_waitq, camera->context.frame_finish_count > 0);
	spin_lock_irq(&camera->context.lock);
	count = min(size, (size_t)camera->context.frame_finish_count);
	if (copy_to_user(user, &camera->context.frame_finish, count)) {
        printk("%s %d: copy_to_user error\n",
			__FUNCTION__, __LINE__);
		return -EFAULT;
    }
	camera->context.frame_finish_count -= count;
	if (camera->context.frame_finish_count > 0) {
		int i;
		char tmp;
		for (i = 0; i < camera->context.frame_finish_count; i++) {
			tmp = camera->context.frame_finish[count + i];
			camera->context.frame_finish[i] = tmp;
		}
	}
	spin_unlock_irq(&camera->context.lock);

	return count;
}

static unsigned int ark_camera_poll(struct file *filp, poll_table *wait)
{
	struct ark_camera_device *camera = (struct ark_camera_device *)filp->private_data;
	unsigned int mask = 0;

	poll_wait(filp, &camera->context.frame_finish_waitq, wait);

	spin_lock_irq(&camera->context.lock);
	if (camera->context.frame_finish_count > 0)
		mask |= POLLIN | POLLRDNORM;
	spin_unlock_irq(&camera->context.lock);

	return mask;
}

extern isp_param_t g_isp_param;
static long ark_camera_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
    /* struct ark_camera_device *camera =
        (struct ark_camera_device *)filp->private_data; */

    switch (cmd)
    {
	case ARK_CAMERA_IOCTL_START:
		isp_enable();
		break;

	case ARK_CAMERA_IOCTL_STOP:
		isp_disable();
		break;

    case ARK_CAMERA_IOCTL_GET_YUV_BUFFER:
		if(copy_to_user((void*)arg, g_isp_param.y_addr, sizeof(g_isp_param.y_addr)))
			return -EFAULT;
		break;

	case ARK_CAMERA_IOCTL_SET_FRAME_READY:
		{
			int frame_id;
			if(copy_from_user(&frame_id, (void*)arg, sizeof(frame_id)))
				return -EFAULT;
			isp_sys_set_frame_ready(frame_id);

		}
		break;

	case ARK_CAMERA_IOCTL_GET_FRAME_SIZE:
		{
			unsigned int frame_size = 0;
			frame_size = (IMAGE_V_SZ << 16) | IMAGE_H_SZ;
			if(copy_to_user((void*)arg, &frame_size, sizeof(frame_size)))
				return -EFAULT;
		}
		break;

	case ARK_CAMERA_IOCTL_GET_FRAME_FORMAT:
		{
			unsigned int format = isp_get_video_format();
			if(copy_to_user((void*)arg, &format, sizeof(format)))
				return -EFAULT;
		}
		break;

	case ARK_CAMERA_IOCTL_GET_FPS:
		{
			unsigned int fps = isp_get_sensor_fps();
			if(copy_to_user((void*)arg, &fps, sizeof(fps)))
				return -EFAULT;
		}
		break;

    default:
        printk("%s %d: undefined cmd (0x%2X)\n",
            __FUNCTION__, __LINE__, cmd);
        return -EINVAL;
    }

    return 0;
}

static struct file_operations ark_camera_fops = {
    .owner          = THIS_MODULE,
    .open           = ark_camera_open,
    .unlocked_ioctl = ark_camera_ioctl,
    .release        = ark_camera_release,
    .fasync         = ark_camera_fasync,
    .read			= ark_camera_read,
    .poll			= ark_camera_poll,
};

/* This function is invoked when the device module is loaded into the
 * kernel. It allocates system resources for constructing driver control
 * data structures and initializes them accordingly.
 */
static int ark_camera_probe(struct platform_device *pdev)
{
    struct ark_camera_device *camera;
	dev_t dev;
   	struct resource *res;
	void __iomem *regs;
    int error = 0;

    camera = devm_kzalloc(&pdev->dev, sizeof(struct ark_camera_device), GFP_KERNEL);
    if (camera == NULL) {
        printk(KERN_ERR "%s %d: failed to allocate memory\n",
            __FUNCTION__, __LINE__);
        return -ENOMEM;
    }

    camera->context.dev = &pdev->dev;

    camera->context.sensor_reset = devm_gpiod_get(&pdev->dev, "sensor-reset", GPIOD_OUT_LOW);
	if (IS_ERR(camera->context.sensor_reset))
		return PTR_ERR(camera->context.sensor_reset);

    //camera->context.sensor_standby = devm_gpiod_get(&pdev->dev, "sensor-standby", GPIOD_OUT_HIGH);
	//if (IS_ERR(camera->context.sensor_standby))
	//	return PTR_ERR(camera->context.sensor_standby);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (IS_ERR(res))
		return PTR_ERR(res);
	regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	camera->context.base = regs;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (IS_ERR(res))
		return PTR_ERR(res);
	regs = ioremap(res->start, resource_size(res));
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	camera->context.scalebase = regs;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if (IS_ERR(res)) {
        error = PTR_ERR(res);
        goto err_sysmem_res_req;
    }
	regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
	if (IS_ERR(regs)) {
		error = PTR_ERR(regs);
		goto err_sysmem_res_req;
	}
	
	camera->isp_clk = devm_clk_get(&pdev->dev, "isp-clk");
	if(IS_ERR(camera->isp_clk)) {
		error = PTR_ERR(camera->isp_clk);
		camera->isp_clk = NULL;
		goto err_sysmem_res_req;
	}
	
	camera->sensor_mclk = devm_clk_get(&pdev->dev, "sensor-mclk");
	if(IS_ERR(camera->sensor_mclk)) {
		error = PTR_ERR(camera->sensor_mclk);
		camera->sensor_mclk = NULL;
		goto err_sysmem_res_req;
	}
	
	camera->context.sysbase = regs;

    camera->driver_name = "ark_camera_drv";
    camera->name = "ark_camera";
    camera->major = 0; /* if 0, let system choose */
    camera->minor_start = 0;
    camera->minor_num = 1; /* one dev only */
    camera->num = 1;
	ark_camera = camera;

    /* register char device */
    if (!camera->major) {
        error = alloc_chrdev_region(
                    &dev,
                    camera->minor_start,
                    camera->num,
                    camera->name
                    );
        if (!error) {
            camera->major = MAJOR(dev);
            camera->minor_start = MINOR(dev);
            //printk(KERN_ERR "%s %d: allocate device major=%d minor=%d\n",
          //      __FUNCTION__, __LINE__,
           //     camera->major, camera->minor_start);
        }
    } else {
        dev = MKDEV(camera->major, camera->minor_start);
       // printk(KERN_ERR "%s %d: dev %d\n", __FUNCTION__, __LINE__, dev);
        error = register_chrdev_region(dev, camera->num,
            (char *)camera->name);
    }

    if (error < 0) {
        printk(KERN_ERR "%s %d: register driver error\n",
            __FUNCTION__, __LINE__);
        goto err_driver_register;
    }

    /* associate the file operations */
    cdev_init(&camera->cdev, &ark_camera_fops);
    camera->cdev.owner    = THIS_MODULE; //driver->owner;
    camera->cdev.ops      = &ark_camera_fops;
    error = cdev_add(&camera->cdev, dev, camera->num);
    if (error) {
        printk(KERN_ERR "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
        goto err_cdev_add;
    }
    //printk(KERN_ERR "%s %d: cdev made, name: %s, major: %d, minor: %d \n",
    //    __FUNCTION__, __LINE__, camera->name, camera->major,
    //    camera->minor_start);

	camera->camera_class = class_create(THIS_MODULE, "camera_class");
    if(IS_ERR(camera->camera_class)) {
    	printk(KERN_ERR "Err: failed in creating ark camera class.\n");
		camera->camera_class = NULL;
        goto err_cdev_add;
    }

	camera->camera_device = device_create(camera->camera_class, NULL, dev, NULL, "camera");
	if (IS_ERR(camera->camera_device)) {
    	printk(KERN_ERR "Err: failed in creating ark camera device.\n");
		camera->camera_device = NULL;
        goto err_cdev_add;
	}

    /* initialize hardware and data structure */
    error = ark_camera_dev_init(camera);
    if (error != 0) {
        printk(KERN_ERR "%s %d: dev init err\n",
            __FUNCTION__, __LINE__);
        goto err_init;
    }
    //printk(KERN_ERR "%s %d: dev init done\n", __FUNCTION__, __LINE__);

    camera->context.irq = platform_get_irq(pdev, 0);
	if (camera->context.irq < 0) {
		printk(KERN_ERR "%s %d: can't get irq resource.\n", __FUNCTION__, __LINE__);
		goto err_irq;
	}
    error = devm_request_irq(
                    &pdev->dev,
                    camera->context.irq,
                    ark_camera_intr_handler,
                    IRQF_SHARED, //SA_SHIRQ,
                    "camera",
                    camera
                    );
    if (!error) {
        //printk(KERN_ERR "%s %d: succeed to get assigned irq %d\n",
        //    __FUNCTION__, __LINE__, camera->context.irq);
    } else {
        printk(KERN_ERR "%s %d: can't get assigned irq %d, error %d\n",
            __FUNCTION__, __LINE__, camera->context.irq, error);
    }

	return 0;

err_irq:
err_init:
	cdev_del(&camera->cdev);
err_cdev_add:
	if (camera->camera_class) {
		if (camera->camera_device) {
			device_destroy(camera->camera_class, dev);
		}
		class_destroy(camera->camera_class);
	}
	unregister_chrdev_region(dev, camera->num);
err_driver_register:
	iounmap(camera->context.sysbase);
err_sysmem_res_req:
	iounmap(camera->context.scalebase);
	ark_camera = NULL;

	return error;
}

/* This function is invoked when the device module is removed from the
 * kernel. It releases all resources to the system.
 */
static int ark_camera_remove(struct platform_device *pdev)
{
    struct ark_camera_device *camera;
	dev_t dev;

    camera = platform_get_drvdata(pdev);
    if (camera == NULL)
        return -ENODEV;

    dev = MKDEV(camera->major, camera->minor_start);

	if(camera->context.camera_queue)
		destroy_workqueue(camera->context.camera_queue);

	cdev_del(&camera->cdev);

	unregister_chrdev_region(dev, camera->num);

    iounmap(camera->context.sysbase);
    iounmap(camera->context.scalebase);

	ark_camera = NULL;
	
    return 0;
}

static const struct of_device_id camera_of_match[] = {
	{ .compatible = "arkmicro,isp-camera", },
	{ }
};
MODULE_DEVICE_TABLE(of, camera_of_match);

static struct platform_driver ark_camera_driver = {
	.driver = {
		.name = "ark_camera",
		.owner = THIS_MODULE,
        .of_match_table = of_match_ptr(camera_of_match),
	},
	.probe		= ark_camera_probe,
	.remove		= ark_camera_remove,
};
module_platform_driver(ark_camera_driver);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("ArkMicro Camera Driver");
MODULE_LICENSE("GPL v2");
