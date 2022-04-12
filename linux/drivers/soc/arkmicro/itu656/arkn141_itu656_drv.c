#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>


#include "arkn141_itu656.h"


static int get_one_finish_frame(struct ark_itu656in_context *context)
{
    int i;
    char tmp;
    int id = -1;
    
    if (context->frame_finish_count > 0) {
         id = context->frame_finish[0];
         context->frame_finish_count -= 1;
        for (i = 0; i < context->frame_finish_count; i++) {
            tmp = context->frame_finish[1 + i];
            context->frame_finish[i] = tmp;
        }
    }
    
	return id;
}

static void dvr_init(struct ark_itu656in_context *context, struct itu656in_para *para)
{
    int i;
    memcpy(&context->itu656in, para, sizeof(struct itu656in_para));

    for(i = 0; i < context->framebuf_num; i++){
        context->framebuf_phyaddr[i].yaddr = context->buffer_phyaddr + ITU656_FRAME_SIZE*i;     
        if (context->itu656in.interlace)
            context->framebuf_phyaddr[i].uvaddr = context->framebuf_phyaddr[i].yaddr + \
                                                  context->itu656in.width * context->itu656in.height * 2;
        else
            context->framebuf_phyaddr[i].uvaddr = context->framebuf_phyaddr[i].yaddr + \
                                                  context->itu656in.width * context->itu656in.height;
    }
}

static long dvr_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
    struct dvr_dev *dvr_dev = (struct dvr_dev *)filp->private_data;
    struct ark_itu656in_context *context;
    unsigned long error = 0;
    unsigned long flags;
    int i;

    if(!dvr_dev)
    {
        itu656_ERROR("error null device");
        return -ENXIO;
    }
	context = &dvr_dev->context;

    spin_lock_irqsave(&context->spin_lock, flags);

    switch (cmd)
    {
        case ARK_DVR_START:
        {
            itu656_INFO("camera startting.");
            if(dvr_dev->start)
                dvr_dev->start(context);
            break;
        }
        case ARK_DVR_STOP:
        {
            if(dvr_dev->stop)
                dvr_dev->stop(context);
            break;
        }
        case ARK_DVR_INIT:
        {
            struct itu656in_para para = {0};
            if(copy_from_user(&para, (void  *)arg, sizeof(struct itu656in_para))){
                printk("%s: copy from user para error\n", __func__);
                error = -EFAULT;
                goto end;
            }
            dvr_init(context, &para);
            
            break;
        }
        case ARK_DVR_GET_BUFFER_INFO:
        {
            struct itu656_framebuf_para para;
            para.num = context->framebuf_num;
            memcpy(&para.buf, &context->framebuf_phyaddr, sizeof(context->framebuf_phyaddr));

            if(copy_to_user((void*)arg, &para, sizeof(struct itu656_framebuf_para))) {
                error = -EFAULT;
                printk("%s %d: copy from user para error\n", __func__, __LINE__);
                goto end;
            }
            
            break;
        }

        case ARK_DVR_GET_BUFFER_READY:
        {
			struct itu656_framebuf_addr buf = {0};
            int id;

            id = get_one_finish_frame(context);
            if(id < 0 || context->framebuf_status[id] != FRAMEBUF_STATUS_READY){
                error = -EFAULT;
                goto end;
            }

            buf = context->framebuf_phyaddr[id];
            if (copy_to_user((void*)arg, &buf, sizeof(buf))) {
                printk("%s %d: copy_to_user error\n",__FUNCTION__, __LINE__);
                error = -EFAULT;
                goto end;
            }
            
            break;
        }

        case ARK_DVR_SET_BUFFER_FREE:
        {
			struct itu656_framebuf_addr buf;

			if(copy_from_user(&buf, (void  *)arg, sizeof(buf))){
                printk("%s %d: copy_from_user error\n",__FUNCTION__, __LINE__);
                error = -EFAULT;
                goto end;
			}

			for (i = 0; i < context->framebuf_num; i++) {
				if (buf.yaddr == context->framebuf_phyaddr[i].yaddr && buf.uvaddr == context->framebuf_phyaddr[i].uvaddr) {
					//if (context->framebuf_status[i] == FRAMEBUF_STATUS_READY)
					if (context->framebuf_status[i] == FRAMEBUF_STATUS_BUSY) {
						context->framebuf_status[i] = FRAMEBUF_STATUS_FREE;
						//printk(KERN_ALERT "### set free buf(0x%x) %s.\n", context->framebuf_phyaddr[i].yaddr, state?"success":"failed");
					} else {
						printk(KERN_ALERT "itu656 app free no-ready buf %d. status:%d\n", i, context->framebuf_status[i]);
					}
					break;
				}
			}
           break;
        }
        default:
            printk("%s: error cmd 0x%x\n", __func__, cmd);
            error = -EFAULT;
            goto end;
    }

end:
    spin_unlock_irqrestore(&context->spin_lock, flags);

    return error;
}

static int dvr_open(struct inode *inode, struct file *filp)
{
    struct dvr_dev * dvr_dev =
        container_of(inode->i_cdev, struct dvr_dev, cdev);

    if(dvr_dev)
        filp->private_data = dvr_dev;
    else
        itu656_ERROR("err open null cdev.");
    return 0;
}

static int dvr_fasync(int fd, struct file * filp, int on)
{
    struct dvr_dev *dvr_dev = (struct dvr_dev*)filp->private_data;
    int retval;
    
    retval = fasync_helper(fd,filp,on,&dvr_dev->fasync_queue);
    if(retval < 0)
      return retval;
    
    return 0;
}

static int dvr_release(struct inode *inode, struct file *filp)
{
    if(filp->f_flags & FASYNC)
    {
        /* remove this filp from the asynchronusly notified filp's */
        dvr_fasync(-1, filp, 0);
    }

    return 0;
}

static int dvr_read(struct file *filp, char __user *user, size_t size, loff_t *ppos)
{
    struct dvr_dev *dvr_dev = (struct dvr_dev *)filp->private_data;
    struct ark_itu656in_context *context = &dvr_dev->context;
	unsigned int count = 0;
	unsigned long flags;
    
    //printk(KERN_ALERT "### camera read size: %d\n", size);

    if (size > context->framebuf_num)
        return -EINVAL;

    wait_event_interruptible(dvr_dev->frame_finish_waitq, context->frame_finish_count > 0);

    spin_lock_irqsave(&context->spin_lock, flags);
#if 0	//test
    count = min(size, (size_t)context->frame_finish_count);
#else
    count = min(1, (size_t)context->frame_finish_count);
    if(context->framebuf_status[context->frame_finish[0]] == FRAMEBUF_STATUS_READY)
    {
		context->framebuf_status[context->frame_finish[0]] = FRAMEBUF_STATUS_BUSY;
	}
	else
	{
		printk(KERN_ALERT "### arkn141 itu656 read error state:%hhd\n", context->framebuf_status[context->frame_finish[0]]);
        goto exit;
	}
#endif
	if (copy_to_user(user, context->frame_finish, count)) {
		printk("%s %d: copy_to_user error\n",
			__FUNCTION__, __LINE__);
		spin_unlock_irqrestore(&context->spin_lock, flags);
		return -EFAULT;
	}
exit:
    context->frame_finish_count -= count;
    if (context->frame_finish_count > 0) {
        int i;
        char tmp;
        for (i = 0; i < context->frame_finish_count; i++) {
            tmp = context->frame_finish[count + i];
            context->frame_finish[i] = tmp;
        }
    }
    spin_unlock_irqrestore(&context->spin_lock, flags);

    return count;
}

static unsigned int dvr_poll(struct file *filp, poll_table *wait)
{
    struct dvr_dev *dvr_dev = (struct dvr_dev *)filp->private_data;
    struct ark_itu656in_context *context = &dvr_dev->context;
    unsigned int mask = 0;

    poll_wait(filp, &dvr_dev->frame_finish_waitq, wait);

    spin_lock_irq(&context->spin_lock);
    if (context->frame_finish_count > 0)
		mask |= POLLIN | POLLRDNORM;
    spin_unlock_irq(&context->spin_lock);

    return mask;
}

/* VFS methods */
static struct file_operations dvr_fops = {
    .owner          = THIS_MODULE,
	.open 			= dvr_open,
	.release		= dvr_release,
	.unlocked_ioctl	= dvr_ioctl,
	.fasync			= dvr_fasync,
	.read			= dvr_read,
	.poll			= dvr_poll,
};

static int arkn141_dvr_probe(struct platform_device *pdev)
{
    struct ark_itu656in_context *context;
    struct dvr_dev *dvr_dev;
    struct resource *res;
    void __iomem *regs;
    dev_t dev;
	u32 value;
    int ret = -1;
    int i;

    dvr_dev = devm_kzalloc(&pdev->dev, sizeof(struct dvr_dev), GFP_KERNEL);
    if (dvr_dev == NULL) {
    	dev_err(&pdev->dev, "%s %d: failed to allocate memory\n",
    	    __FUNCTION__, __LINE__);
    	return -ENOMEM;
    }

    //g_dvr_dev = dvr_dev;	
    dvr_dev->driver_name = "dvr";
    dvr_dev->name = "ark_dvr";
    dvr_dev->major = DVR_MAJOR; /* if 0, let system choose */
    dvr_dev->minor_start = 0;
    dvr_dev->minor_num = 1; /* one dev only */
    dvr_dev->num = 1;

    dvr_dev->start = dvr_start;
    dvr_dev->stop = dvr_stop;
    dvr_dev->fasync_queue = NULL;
    
    dev = MKDEV(dvr_dev->major, dvr_dev->minor_start);
    ret= register_chrdev_region(dev, dvr_dev->num, (char *)dvr_dev->name);
    if (ret != 0){
    	dev_err(&pdev->dev, "%s %d: register driver error\n",
    	    __FUNCTION__, __LINE__);
    	goto err_driver_register;
    }

    cdev_init(&dvr_dev->cdev, &dvr_fops);
    ret= cdev_add(&dvr_dev->cdev, dev, dvr_dev->num);
    if (ret != 0){
    	dev_err(&pdev->dev, "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
    	goto err_cdev_add;
    }
    
#if ITU656_DEV_CLASS_CREATE
    dvr_dev->itu656_class = class_create(THIS_MODULE, "itu656_class");
    if(IS_ERR(dvr_dev->itu656_class)) {
    	dev_err(&pdev->dev, "Err: failed in creating ark isp scale class.\n");
    	dvr_dev->itu656_class = NULL;
        goto err_class_add;
    }

    dvr_dev->itu656_device = device_create(dvr_dev->itu656_class, NULL, dev, NULL, dvr_dev->driver_name);
    if (IS_ERR(dvr_dev->itu656_device)) {
    	dev_err(&pdev->dev, "Err: failed in creating ark isp scale device.\n");
    	dvr_dev->itu656_device = NULL;
        goto err_class_add;
    }
#endif

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (IS_ERR(res)) {
    	ret = PTR_ERR(res);
    	goto err_mem_res_req;
    }
    regs = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(regs)) {
    	ret = PTR_ERR(regs);
    	goto err_mem_res_req;
    }
    dvr_dev->context.itu656_base = regs;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (IS_ERR(res)) {
    	ret = PTR_ERR(res);
    	goto err_mem_res_req;
    }
    regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
    if (IS_ERR(regs)) {
    	ret = PTR_ERR(regs);
    	goto err_mem_res_req;
    }
    dvr_dev->context.sys_base = regs;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if (IS_ERR(res)) {
    	ret = PTR_ERR(res);
    	goto err_mem_res1_req;
    }
    regs = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
    if (IS_ERR(regs)) {
    	ret = PTR_ERR(regs);
    	goto err_mem_res1_req;
    }
    dvr_dev->context.deinterlace_base = regs;

    dvr_dev->context.itu656_irq = platform_get_irq(pdev, 0);
    if (dvr_dev->context.itu656_irq < 0) {
    	dev_err(&pdev->dev, "%s %d: can't get itu656_irq resource.\n", __FUNCTION__, __LINE__);
    	goto err_irq;
    }

    ret = devm_request_irq(
    				&pdev->dev,
    				dvr_dev->context.itu656_irq, 
    				ark_itu656_int_handler, 
    				IRQF_SHARED, 
    				"dvr_itu656", 
    				dvr_dev
    				);
    if(ret){
        dev_err(&pdev->dev, "%s %d: can't get assigned itu656_irq %d, error %d\n",
            __FUNCTION__, __LINE__, dvr_dev->context.itu656_irq, ret);
        goto err_irq;
    }

    dvr_dev->context.deinterlace_irq = platform_get_irq(pdev, 1);
    if (dvr_dev->context.deinterlace_irq < 0) {
    	dev_err(&pdev->dev, "%s %d: can't get deinterlace_irq resource.\n", __FUNCTION__, __LINE__);
    	goto err_irq;
    }

    ret = devm_request_irq(
    			&pdev->dev,
    			dvr_dev->context.deinterlace_irq, 
    			ark_deinterlace_int_handler, 
    			IRQF_SHARED, 
    			"dvr_deinterlace", 
    			dvr_dev
    			);
    if(ret){
        dev_err(&pdev->dev, "%s %d: can't get assigned deinterlace_irq %d, error %d\n",
            __FUNCTION__, __LINE__, dvr_dev->context.deinterlace_irq, ret);
        goto err_irq;
    }

    //init context
    context = &dvr_dev->context;
    
    context->work_status = 0;
    context->deinter_status = 0;
    context->frame_finish_count = 0;
    context->framebuf_num = ITU656_FRAME_NUM;
#ifdef ITU656_STATIC_FRAME_SIZE
    context->buffer_size = ITU656_FRAME_SIZE*ITU656_FRAME_NUM;// + 0x200000; //extra 2M memory for write overflow bug
#else
	context->buffer_size = ITU656_FRAME_SIZE*ITU656_FRAME_NUM + 0x200000; //extra 2M memory for write overflow bug
#endif
#if 0
    context->buffer_virtaddr = (void *)__get_free_pages(GFP_KERNEL, get_order(context->buffer_size));
	if (!context->buffer_virtaddr){
    	dev_err(&pdev->dev, "%s %d: get buffer failed.\n", __FUNCTION__, __LINE__);
        ret = -ENOMEM;
        goto err_buffer;
    }

    context->buffer_phyaddr = virt_to_phys(context->buffer_virtaddr);
#else
	context->buffer_virtaddr = dma_alloc_wc(&pdev->dev, context->buffer_size,
					 (dma_addr_t *)&context->buffer_phyaddr,
					 GFP_KERNEL);
	if (!context->buffer_virtaddr){
		dev_err(&pdev->dev, "%s %d: get buffer failed.\n", __FUNCTION__, __LINE__);
		ret = -ENOMEM;
		goto err_buffer;
	}
#endif


    for(i = 0; i < context->framebuf_num; i++){
        context->framebuf_phyaddr[i].yaddr = context->buffer_phyaddr + ITU656_FRAME_SIZE*i;
    }
	if(!of_property_read_u32(pdev->dev.of_node, "channel", &value)) {
		if(value >= ITU656_CH0 && value <= ITU656_CH0_CH1)
			context->itu_channel = value;
	}
    INIT_LIST_HEAD(&context->framebuf_push_list);
    spin_lock_init(&context->spin_lock);
    init_waitqueue_head(&dvr_dev->frame_finish_waitq);

    dvr_dev->context.dev = &pdev->dev;
    platform_set_drvdata(pdev, dvr_dev);

    timer_setup(&dvr_dev->timer, dither_timeout_timer, 0);
	printk("arkn141 itu656 probe success\n");

    return 0;
	
err_buffer:
err_irq:
    iounmap(dvr_dev->context.deinterlace_base);
err_mem_res1_req:
    iounmap(dvr_dev->context.sys_base);
err_mem_res_req:
#if ITU656_DEV_CLASS_CREATE
    if (dvr_dev->itu656_class) {
    	if (dvr_dev->itu656_device) {
    		device_destroy(dvr_dev->itu656_class, dev);
    	}
    	class_destroy(dvr_dev->itu656_class);
    }
#endif
err_class_add:
    cdev_del(&dvr_dev->cdev);
err_cdev_add:
    unregister_chrdev_region(dev, dvr_dev->num);
err_driver_register:
	printk(KERN_ALERT "arkn141 itu656 probe failed\n");
    return ret;
}

static int arkn141_dvr_remove(struct platform_device *pdev)
{
    dev_t dev;
    struct dvr_dev  *dvr_dev;
    
    dvr_dev = platform_get_drvdata(pdev);
    if (dvr_dev == NULL)
        return -ENODEV;

    iounmap(dvr_dev->context.deinterlace_base);
    iounmap(dvr_dev->context.sys_base);
    dev = MKDEV(dvr_dev->major, dvr_dev->minor_start);
#if ITU656_DEV_CLASS_CREATE
    if (dvr_dev->itu656_class) {
    	if (dvr_dev->itu656_device) {
    		device_destroy(dvr_dev->itu656_class, dev);
    	}
    	class_destroy(dvr_dev->itu656_class);
    }
#endif

    cdev_del(&dvr_dev->cdev);
    unregister_chrdev_region(dev, dvr_dev->num);
#if 0
    free_pages((unsigned long)dvr_dev->context.buffer_virtaddr, get_order(dvr_dev->context.buffer_size));
#else
	if(dvr_dev->context.buffer_virtaddr)
		dma_free_wc(&pdev->dev, dvr_dev->context.buffer_size, dvr_dev->context.buffer_virtaddr, dvr_dev->context.buffer_phyaddr);
#endif
    return 0;
}

static const struct of_device_id arkn141_itu656_of_match[] = {
	{ .compatible = "arkmicro,arkn141-itu656", },
	{ }
};
MODULE_DEVICE_TABLE(of, arkn141_itu656_of_match);

static struct platform_driver arkn141_itu656_driver = {
    .driver     = {
        .name   = "arkn141-itu656",
		.of_match_table = of_match_ptr(arkn141_itu656_of_match),
    },
    .probe      = arkn141_dvr_probe,
    .remove     = arkn141_dvr_remove,
};
module_platform_driver(arkn141_itu656_driver);


MODULE_AUTHOR("Leo");
MODULE_DESCRIPTION("ArkMicro arkn141 Itu656 Driver");
MODULE_LICENSE("GPL v2");

