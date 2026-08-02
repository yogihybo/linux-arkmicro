/*
 * arkmicro carback driver
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/err.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/spinlock.h>

#include <linux/delay.h>

#define CARBACK_IOCTL_BASE							0x9A
#define CARBACK_IOCTL_SET_APP_READY					_IO(CARBACK_IOCTL_BASE, 0)
#define CARBACK_IOCTL_APP_ENTER_DONE				_IO(CARBACK_IOCTL_BASE, 1)
#define CARBACK_IOCTL_APP_EXIT_DONE					_IO(CARBACK_IOCTL_BASE, 2)	
#define CARBACK_IOCTL_GET_STATUS					_IOR(CARBACK_IOCTL_BASE, 3, int)	
#define CARBACK_IOCTL_DETECT_SIGNAL					_IOR(CARBACK_IOCTL_BASE, 4, int)	


struct carback_context {
        struct device *dev;
        unsigned char carback_status;
        int carback_changed;
        int app_ready;
        int app_enter_done;
        int app_exit_done;
        wait_queue_head_t carback_waiq;
        wait_queue_head_t app_enter_waiq;
        wait_queue_head_t app_exit_waiq;
        struct work_struct carback_work;
        struct workqueue_struct *carback_queue;
        struct fasync_struct *async_queue_cb;
        spinlock_t spin_lock;
        int itu656_init;
};


struct ark_carback {
        int irq;
        struct gpio_desc	*detect;
        int debounce_detect;
        struct work_struct carback_work;
        struct workqueue_struct *carback_queue;


        struct platform_device *pdev;
        const char *driver_name;
        const char *name;
        int major;
        int minor_start;
        int minor_num;
        int num;
        struct cdev cdev;
        struct class *carback_class;
        struct device *carback_device;
        struct carback_context context;
};

extern int dvr_enter_carback(void);
extern int dvr_exit_carback(void);
extern int dvr_detect_carback_signal(void);
extern int ark_disp_set_layer_en(int layer_id, int enable);
extern int dvr_exit_wait(void);
extern int ark1668_itu656_is_probed(void);

static struct ark_carback *g_carback = NULL; 


/* 2026-08-03: found via a full dmesg capture that this never ran on
 * real hardware -- rn6752_probe() (drivers/soc/arkmicro/itu656/
 * ark1668_itu656.c's own probe, ark1668_itu656_probe()) calls this at
 * the very end of ITS probe, but ark-carback's platform driver hadn't
 * probed yet at that point in the real boot sequence, so g_carback
 * was still NULL here and this bailed out via the check below every
 * single time. Real consequence, bigger than just the at-boot
 * reverse-gear check: itu656_init only ever gets set to 1 inside this
 * function, and carback_int_work() (the reverse-gear GPIO IRQ's
 * deferred work handler) unconditionally no-ops whenever
 * itu656_init==0 -- so this silently broke reverse-gear detection
 * entirely, not just its at-boot state, for as long as the two
 * drivers happened to probe in this order. Fixed at the other end
 * too (ark_carback_probe() below, right after g_carback is set) so
 * whichever of the two drivers actually probes second is the one that
 * ends up triggering this, regardless of link/registration order --
 * see that call site's own comment for why it's gated on
 * ark1668_itu656_is_probed() rather than called unconditionally. */
void carback_first_enter(void)
{
        if(!g_carback){
                printk("%s g_carback null error\n",__FUNCTION__);
                return;
        }

        if (!gpiod_get_value(g_carback->detect)) {
            dvr_enter_carback();
            g_carback->context.carback_status = 1;
        } else {
            g_carback->context.carback_status = 0;
        }

        g_carback->context.itu656_init = 1;

}

EXPORT_SYMBOL(carback_first_enter);

static ssize_t ark_carback_read(struct file *filp, char __user *user, size_t size,loff_t *ppos)
{
        struct ark_carback *carback = (struct ark_carback *)filp->private_data;
        unsigned long flags;

        if (size != 1)
                return -EINVAL;

        // if backcar changed ,will enter ark_backcar_intr_handler, set carback_changed
        wait_event_interruptible(carback->context.carback_waiq, carback->context.carback_changed);
        if (copy_to_user(user, &carback->context.carback_status, 1)) {
                printk("%s %d: copy_to_user error\n",__FUNCTION__, __LINE__);
                return -EFAULT;
        }
        spin_lock_irqsave(&carback->context.spin_lock, flags);
        /* clear backcar_changed*/
        carback->context.carback_changed = 0;
        spin_unlock_irqrestore(&carback->context.spin_lock, flags);

        return 1;
}

static unsigned int ark_carback_poll(struct file *filp, poll_table *wait)
{
        struct ark_carback *carback = (struct ark_carback *)filp->private_data;
        unsigned int mask = 0;

        poll_wait(filp, &carback->context.carback_waiq, wait);

        // if backcar changed ,will enter ark_backcar_intr_handler, set carback_changed
        if(carback->context.carback_changed)
        {
                mask |= POLLIN | POLLRDNORM;
        }

        return mask;
}

static int ark_carback_open(struct inode *inode, struct file *filp)
{
        struct ark_carback *dev;
        struct carback_context *context;

        dev = container_of(inode->i_cdev, struct ark_carback, cdev);
        context = &dev->context;
        filp->private_data = dev;

        return 0;
}

static int ark_carback_fasync(int fd, struct file *filp, int mode)
{
        int ret;
        struct ark_carback *carback = (struct ark_carback *)filp->private_data;

        ret = fasync_helper(fd, filp, mode, &carback->context.async_queue_cb);

        return ret;
}

static int ark_carback_release(struct inode *inode, struct file *filp)
{
        struct ark_carback *dev;

        dev = container_of(inode->i_cdev, struct ark_carback, cdev);

        if(filp->f_flags & FASYNC){
                /* remove this filp from the asynchronusly notified filp's */
                ark_carback_fasync(-1, filp, 0);
        }


        return 0;
}

static long ark_carback_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
        struct ark_carback *carback = (struct ark_carback *)filp->private_data;
        struct carback_context *context = &carback->context;
        int error = 0;
    
        switch (cmd)
        {
        case CARBACK_IOCTL_SET_APP_READY:
        	context->app_ready = 1;
        	break;
        case CARBACK_IOCTL_APP_ENTER_DONE:
        	context->app_enter_done = 1;
        	wake_up_interruptible(&context->app_enter_waiq);
        	break;
        case CARBACK_IOCTL_APP_EXIT_DONE:
        	context->app_exit_done = 1;
        	wake_up_interruptible(&context->app_exit_waiq);
        	break;
        case CARBACK_IOCTL_GET_STATUS:
        	{
        		int status = context->carback_status;
                        if (copy_to_user((void*)arg, &status, sizeof(status))) {
                                printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
                                error = -EFAULT;
                        }
        	}
        	break;
        case CARBACK_IOCTL_DETECT_SIGNAL:
        	{
        		int signal = dvr_detect_carback_signal();
        		if (copy_to_user((void*)arg, &signal, sizeof(signal))) {
                                printk("%s %d: copy_to_user error\n",__FUNCTION__, __LINE__);
                                error = -EFAULT;
        		}			
        	}
        	break;
        default:
                printk("%s %d: undefined cmd (0x%2X)\n", __FUNCTION__, __LINE__, cmd);
                error = -EINVAL;
        }


        return error;
}

static struct file_operations ark_carback_fops = {
        .owner          = THIS_MODULE,
        .open           = ark_carback_open,
        .unlocked_ioctl = ark_carback_ioctl,
        .release        = ark_carback_release,		
        .fasync         = ark_carback_fasync,
        .read           = ark_carback_read,
        .poll           = ark_carback_poll,
};

void carback_int_work(struct work_struct *work)
{
        struct ark_carback *carback = container_of(work, struct ark_carback, carback_work);
        struct carback_context *context = &carback->context;
        unsigned long flags;
        int status ,ret;

        if(!context->itu656_init) {
                return ;
        }

        status = !gpiod_get_value(carback->detect);
        if (status == context->carback_status)
                return ;

        if (!status) {
                dvr_exit_carback();
        }
        context->carback_status = status;
        dvr_exit_wait();

        spin_lock_irqsave(&context->spin_lock, flags);
        context->carback_changed = 1; // set flag to wakeup carback_waiq
        spin_unlock_irqrestore(&context->spin_lock, flags);
        wake_up_interruptible(&context->carback_waiq);//poll 

        if(context->async_queue_cb != NULL) {
                printk(KERN_DEBUG "kill_fasync carback.\n");
                kill_fasync(&context->async_queue_cb, SIGIO, POLL_IN);//async
        }   

        if(status){
                if (context->app_ready) {
                        ret = wait_event_interruptible_timeout(context->app_enter_waiq, context->app_enter_done, msecs_to_jiffies(500));
                        if (ret == 0) {
                                printk(KERN_ALERT "wait for app enter carback timeout. close fb0 by kernel.\n");
                                ark_disp_set_layer_en(0, 0);
                        }
                        context->app_enter_done = 0;
                } else ark_disp_set_layer_en(0, 0);

                dvr_enter_carback();
        }else{
                if (context->app_ready) {
                        ret = wait_event_interruptible_timeout(context->app_exit_waiq, context->app_exit_done, msecs_to_jiffies(500));
                        if (ret == 0) {
                                printk(KERN_ALERT "wait for app exit carback timeout.\n");
                                ark_disp_set_layer_en(0, 1);
                        }
                        context->app_exit_done = 0;
                } else ark_disp_set_layer_en(0, 1); 

                //dvr_exit_carback();
        }

}

static irqreturn_t carback_interrupt(int irq, void *dev_id)
{
        struct ark_carback *carback = (struct ark_carback *)dev_id;

        printk(KERN_DEBUG "carback_interrupt %d.\n", gpiod_get_value(carback->detect));

        queue_work(carback->carback_queue, &carback->carback_work);

        return IRQ_HANDLED;
}

static const struct of_device_id ark_carback_of_match[] = {
	{ .compatible = "arkmicro,ark-carback", },
	{ /* sentinel */ }
};

static int ark_carback_probe(struct platform_device *pdev)
{
        struct ark_carback *carback;
        dev_t dev;
        int err;

        carback = devm_kzalloc(&pdev->dev, sizeof(*carback), GFP_KERNEL);
        if (!carback)
                return -ENOMEM;
        memset(carback,0,sizeof(struct ark_carback));

        carback->detect = devm_gpiod_get(&pdev->dev, "detect", GPIOD_IN);
        if (IS_ERR(carback->detect))
                return PTR_ERR(carback->detect);

        if (of_property_read_u32(pdev->dev.of_node, "debounce-detect", &carback->debounce_detect)){
                carback->debounce_detect = 20;
        }

	gpiod_set_debounce(carback->detect, carback->debounce_detect);

        carback->irq = platform_get_irq(pdev, 0);
        if (carback->irq < 0)
                return carback->irq;

        carback->carback_queue = create_singlethread_workqueue("carback_queue");
        if(!carback->carback_queue) {
                printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);	
                return -1;
        }

        INIT_WORK(&carback->carback_work, carback_int_work);

        carback->pdev = pdev;
        carback->driver_name = "ark_carback_drv";
        carback->name = "ark_carback";
        carback->major = 0; /* if 0, let system choose */
        carback->minor_start = 0;
        carback->minor_num = 1; /* one dev only */
        carback->num = 1;
    
        /* register char device */
        if (!carback->major) {
                err = alloc_chrdev_region(&dev, carback->minor_start, carback->num, carback->name);
                if (!err) {
                        carback->major = MAJOR(dev);
                        carback->minor_start = MINOR(dev);
                }
        } else {
                dev = MKDEV(carback->major, carback->minor_start);
                err = register_chrdev_region(dev, carback->num,(char *)carback->name);
        }

        if (err < 0) {
                printk(KERN_ERR "%s %d: register driver error\n", __FUNCTION__, __LINE__);
                goto err_driver_register;
        }

        /* associate the file operations */
        cdev_init(&carback->cdev, &ark_carback_fops);
        carback->cdev.owner    = THIS_MODULE; //driver->owner;
        carback->cdev.ops      = &ark_carback_fops;
        err = cdev_add(&carback->cdev, dev, carback->num);
        if (err) {
                printk(KERN_ERR "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
                goto err_cdev_add;
        }

        carback->carback_class = class_create(THIS_MODULE, "carback_class");
        if(IS_ERR(carback->carback_class)) {
        	printk(KERN_ERR "Err: failed in creating ark carback class.\n");
        	carback->carback_class = NULL;
                goto err_carback_class;
        }
	
        carback->carback_device = device_create(carback->carback_class, NULL, dev, NULL, "carback");
        if (IS_ERR(carback->carback_device)) {
        	printk(KERN_ERR "Err: failed in creating ark carback device.\n");
        	carback->carback_device = NULL;
                goto err_carback_class;
        }	
    
        carback->context.dev = &pdev->dev;
        platform_set_drvdata(pdev, carback);
        init_waitqueue_head(&carback->context.carback_waiq);  
        init_waitqueue_head(&carback->context.app_enter_waiq); 
        init_waitqueue_head(&carback->context.app_exit_waiq);
        spin_lock_init(&carback->context.spin_lock);

    
        err = devm_request_irq(&pdev->dev, carback->irq, carback_interrupt,0, "ark-carback", carback);
        if (err){
                printk(KERN_ERR "%s %d: can't get assigned carback irq %d, error %d\n",
                            __FUNCTION__, __LINE__, carback->irq, err);
                return err;
        }
    
        g_carback =  carback;

        /* 2026-08-03: symmetric half of the probe-order fix -- see
         * carback_first_enter()'s own comment above for the full
         * explanation. Only call through if itu656 has already probed
         * (g_dvr_dev set): carback_first_enter() unconditionally calls
         * dvr_enter_carback() whenever the detect GPIO reads "active",
         * which dereferences itu656's g_dvr_dev with no NULL check of
         * its own -- calling this unconditionally here, before itu656
         * has necessarily probed, would Oops instead of just silently
         * no-opping the way the original ordering bug did. If itu656
         * probes first, its own existing call (end of
         * ark1668_itu656_probe()) already covers this case; if
         * ark-carback probes first, this call covers it instead. */
        if (ark1668_itu656_is_probed())
                carback_first_enter();

        return 0;

err_carback_class:
        if (carback->carback_class) {
                if (carback->carback_device)
                        device_destroy(carback->carback_class, dev);
                class_destroy(carback->carback_class);
        }
err_cdev_add:
        cdev_del(&carback->cdev);
err_driver_register:    
        unregister_chrdev_region(dev, carback->num);
        return err;    
}

static struct platform_driver ark_carback_driver = {
	.driver = {
		.name = "ark-carback",
		.of_match_table = of_match_ptr(ark_carback_of_match),
	},
	.probe = ark_carback_probe,
};

static int __init ark_carback_init(void)
{
	return platform_driver_register(&ark_carback_driver);
}
arch_initcall(ark_carback_init);

