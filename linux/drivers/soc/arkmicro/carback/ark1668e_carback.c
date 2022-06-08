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
#include <linux/of_gpio.h>
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
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <asm/setup.h>
#include <linux/spinlock.h>
#include <ark1668e_carback.h>
#include <ark_track.h>
#include <ark_mcu.h>
#include <linux/delay.h>

#ifdef CONFIG_VIDEO_ARK1668E_VIN
extern int dvr_enter_carback(void);
extern int dvr_exit_carback(void);
extern int dvr_set_mirror(int value);
extern int dvr_detect_carback_signal(void);
extern int dvr_get_layer_status(void);
extern int ark_disp_set_layer_en(int layer_id, int enable);
#endif

extern void ark_set_track_ready(void);
extern void ark_set_track_noready(void);
extern int get_bootanimation_status(void);

struct carback_context* g_carback_context = NULL;
static int delay_show_track = 0;

#ifdef CONFIG_REVERSING_TRACK
#define ARK_DISP_OSD_PIXFMT_RGBA888    6
 
int first_draw_track = 1;

#endif

static void carback_filter_timer_irq(struct timer_list *t)
{	
	queue_work(g_carback_context->carback_queue, &g_carback_context->carback_work);
}

void carback_int_work(struct work_struct *work)
{
	struct carback_context *context = container_of(work, struct carback_context, carback_work);
	struct ark_carback *carback = container_of(context, struct ark_carback, context);
	int status = !gpiod_get_value(context->detect);	

	printk(KERN_ALERT "carback_int_work in.status=%d.\n", status);
	if (status != context->carback_status) {	
		if (!status) {
#ifdef CONFIG_REVERSING_TRACK	
			del_timer(&context->track_timer);
            msleep(10);
			ark_disp_set_layer_en(2, 0);
#endif
			dvr_exit_carback();
		}
		context->carback_status = status;
		
		//poll
		context->carback_changed = 1; //set flag to wakeup carback_waiq
		wake_up_interruptible(&context->carback_waiq);

		//async
		if(carback->context.async_queue_cb != NULL) {
			printk(KERN_ALERT "kill_fasync carback.\n");
			kill_fasync(&carback->context.async_queue_cb, SIGIO, POLL_IN);
		}		

		if (status) {
			if (context->app_ready) {
				int ret = wait_event_interruptible_timeout(context->app_enter_waiq, 
					context->app_enter_done, msecs_to_jiffies(500));
				if (ret == 0) {
					printk(KERN_ALERT "wait for app enter carback timeout.close fb0 by kernel.\n");
					ark_disp_set_layer_en(3, 0);
				}
				context->app_enter_done = 0;
			} else ark_disp_set_layer_en(3, 0);
			if(get_bootanimation_status())
				ark_disp_set_layer_en(0, 0);   //close bootanmation
			context->track_data_status = 0;
			dvr_set_mirror(carback->mirror_config);
			dvr_enter_carback();
#ifdef CONFIG_REVERSING_TRACK
			if (*(volatile unsigned int*)g_carback_context->track_data_virtaddr == MKTAG('R', 'S', 'T', 'K')) {
				first_draw_track = 1;
				delay_show_track = 0;
				mod_timer(&context->track_timer, jiffies + msecs_to_jiffies(1));
			}
#endif
		} else {
			if (context->app_ready) {
				int ret = wait_event_interruptible_timeout(context->app_exit_waiq, 
					context->app_exit_done, msecs_to_jiffies(500));
				if (ret == 0) {
					printk(KERN_ALERT "wait for app exit carback timeout.\n");
					ark_disp_set_layer_en(3, 1);
				}
				context->app_exit_done = 0;
			} else ark_disp_set_layer_en(3, 1);	
			if(get_bootanimation_status())
				ark_disp_set_layer_en(0, 1);   //open bootanmation
			
#ifdef CONFIG_REVERSING_TRACK	
			context->track_data_status = 1;
			ark_disp_set_layer_en(2, 0);
#endif
		}
	} 
}

static int carback_detect_count = 0;
static void carback_signal_detect(void)
{
	struct carback_context *context = g_carback_context;
	int status = !gpiod_get_value(context->detect);	
	int temp_signal;
    if(status){
        temp_signal = dvr_detect_carback_signal();
        if(temp_signal == 0){
			carback_detect_count++;
			if(carback_detect_count == 10){
                printk(KERN_ALERT "carback delect itu no signal\n");
				if(status)
                context->carback_signal = temp_signal;
				else
					context->carback_signal = 1;
				carback_detect_count = 0;
            }
        }
        else{
            context->carback_signal = temp_signal;
            context->carback_count = 0;
			carback_detect_count = 0;
        }
    }
}

#ifdef CONFIG_REVERSING_TRACK

void track_paint_work(struct work_struct *work)
{
	struct carback_context *context = container_of(work, struct carback_context, track_work);
	struct ark_carback *carback = container_of(context, struct ark_carback, context);
	int delay_count,dst_phyaddr;
	void *dest;
	unsigned int dest_size;

	carback_signal_detect();
	context->layer_status = dvr_get_layer_status();
		
	if(context->carback_signal == 0)
		set_disp_signal_id(SIGNAL_NORMAL_STATUS_ID);

	if(context->carback_signal == 1){
		if(context->layer_status)
			set_disp_signal_id(IMAGE_ID_NONE);
	}

	if(context->carback_signal && !context->layer_status) {
		mod_timer(&context->track_timer, jiffies + msecs_to_jiffies(carback->context.track_frame_delay));
		return;
	}

	if(context && (!context->track_disp_width || !context->track_disp_height)) return;

	if (first_draw_track) {
		ark_track_display_init(carback->context.screen_width,carback->context.screen_height);
		ark_track_alpha_blend();
	}

	dest = (void*)context->tdisplay_virtaddr[context->buffer_index];
	dst_phyaddr = context->tdisplay_phyaddr[context->buffer_index];
	
	dest_size = context->track_display_size; 
	dest_size = track_paint_fill(dest, context->track_disp_width, context->track_disp_height);// need 30 ms   
	if (dest_size > 0) {
		context->buffer_index = (context->buffer_index + 1) % TRACK_FRAME_NUM;
		ark_track_set_display_addr(dst_phyaddr); 
	}

	if (first_draw_track) {
		first_draw_track = 0;
	}

    mod_timer(&context->track_timer, jiffies + msecs_to_jiffies(carback->context.track_frame_delay));//  1000/(100+30)=8 frame per sec 

    delay_count = 260/(carback->context.track_frame_delay+30) ;//delay 200ms show

	if(delay_show_track >= delay_count)
		return;

	if(++delay_show_track == delay_count)
	{
		context->track_data_status = 1;
		ark_disp_set_layer_en(2, 1);
	}
}

static void track_timer_handler(struct timer_list *t)
{
	struct carback_context *context = g_carback_context;
	
	queue_work(context->track_queue, &context->track_work);
}

#endif

static int ark_carback_dev_init(struct carback_context *context)
{
	context->carback_changed = 0;
	init_waitqueue_head(&context->carback_waiq);  
	init_waitqueue_head(&context->app_enter_waiq); 
	init_waitqueue_head(&context->app_exit_waiq);
	context->carback_queue = create_singlethread_workqueue("carback_queue");
	if(!context->carback_queue) {
    	printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);	
		return -1;
	}
	INIT_WORK(&context->carback_work, carback_int_work);
	timer_setup(&context->carback_filter_timer, carback_filter_timer_irq, 0);
	return 0;
}

static int ark_carback_dev_uninit(struct carback_context *context)
{
	del_timer(&context->carback_filter_timer);

	if(context->carback_queue)
		destroy_workqueue(context->carback_queue);

	gpio_free(context->gpio_id);
	return 0;
}

irqreturn_t ark_carback_intr_handler(int irq, void *dev_id)
{
	struct ark_carback *carback = (struct ark_carback *)dev_id;
	struct carback_context *context =&carback->context;	
	
	mod_timer(&context->carback_filter_timer, jiffies +  msecs_to_jiffies(50));

	return IRQ_HANDLED;
}

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

    if(filp->f_flags & FASYNC)
    {
        /* remove this filp from the asynchronusly notified filp's */
        ark_carback_fasync(-1, filp, 0);
    }


    return 0;
}

static long ark_carback_ioctl(struct file *filp,
                          unsigned int cmd, unsigned long arg)
{ 
    struct ark_carback *carback =
        (struct ark_carback *)filp->private_data;
    struct carback_context *context = &carback->context;	

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
                printk("cmd CARBACK_IOCTL_GET_STATUS %s %d: copy_to_user error\n",
                    __FUNCTION__, __LINE__);
                return -EFAULT;
			}
		}
		break;
	case CARBACK_IOCTL_DETECT_SIGNAL:
		{
			int signal = dvr_detect_carback_signal();
			if (copy_to_user((void*)arg, &signal, sizeof(signal))) {
                printk("cmd CARBACK_IOCTL_DETECT_SIGNAL %s %d: copy_to_user error\n",
                    __FUNCTION__, __LINE__);
                return -EFAULT;
			}			
		}
		break;
	case CARBACK_IOCTL_GET_HASTRACK:
		{
#ifdef CONFIG_REVERSING_TRACK
        		int track = 1;
#else
        		int track = 0;
#endif
			if (copy_to_user((void*)arg, &track, sizeof(track))) {
				printk("cmd CARBACK_IOCTL_GET_HASTRACK %s %d: copy_to_user error\n",
				__FUNCTION__, __LINE__);
				return -EFAULT;
			}						
		}
		break;
#ifdef CONFIG_REVERSING_TRACK
		case CARBACK_IOCTL_STRACK_INIT:
		{
			printk("vbox track paint init\n");
			ark_disp_set_layer_en(2, 0);
            context->track_frame_delay = 100;
			/*if(track_paint_init() < 0){
				printk(KERN_ERR "%s %d: ,track_paint_init fail.\n",__FUNCTION__, __LINE__); 
				break;
			}
			
			INIT_WORK(&carback->context.track_work, track_paint_work);
			setup_timer(&carback->context.track_timer, track_timer_handler, (unsigned long)&carback->context);*/
		}
		break;
		case CARBACK_IOCTL_STRACK_START:
		{
			if (*(volatile unsigned int*)g_carback_context->track_data_virtaddr == MKTAG('R', 'S', 'T', 'K')) {
				first_draw_track = 1;
				delay_show_track = 0;
				mod_timer(&context->track_timer, jiffies +	msecs_to_jiffies(1));
			}
	
		}
		break;

		case CARBACK_IOCTL_STRACK_STOP:
		{
			unsigned int ret;
			del_timer(&context->track_timer);
			msleep(10);
			ret = ark_disp_set_layer_en(2, 0);
			return ret;
            //delay_show_track = 0;
		}
		break;
		case CARBACK_IOCTL_SET_STRACKID:
		{
			unsigned int pic_id[4];
				
			if(copy_from_user(&pic_id, (void*)arg, sizeof(unsigned int)*4)){
				printk(KERN_ALERT "CARBACK_IOCTL_SET_STRACKID error\n");
				return -EFAULT;
			}
			
			if(pic_id[3] == 0) 
				pic_id[3] = 0xaaaaaaaa;
			
			set_disp_track_id(pic_id[0]);
			set_disp_car_id(pic_id[1]);
			set_disp_track2_id(pic_id[2]);
			set_disp_radar_id(pic_id[3]);
			
			//printk(KERN_ALERT "track_id=0x%0x,car_id=0x%0x,track2_id=0x%0x ,radar_id=0x%0x\n",pic_id[0],pic_id[1],pic_id[2],pic_id[3]);
		}
		break;
		
		case CARBACK_IOCTL_STRACK_SHOW:
		{
			printk("vbox track show\n");
			ark_disp_set_layer_en(2, 1);
		}
		break;

		case CARBACK_IOCTL_STRACK_CLOSE:
		{
			printk("vbox track close\n");
			ark_disp_set_layer_en(2, 0);
		}
		break;
		
		case CARBACK_IOCTL_STRACK_SETTING:
		{
			int set;
			
			if(copy_from_user(&set, (void*)arg, sizeof(int))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_SETTING error\n");
				return -EFAULT;
			}
			
			if(set == 0 || set == 1){
				context->track_setting = set;
				printk("track_setting=%d\n",set);
			}
		}
		break;
        case CARBACK_IOCTL_STRACK_FRAME_RATE:
		{
			int frame_rate;
			
			if(copy_from_user(&frame_rate, (void*)arg, sizeof(int))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_SETTING error\n");
				return -EFAULT;
			}
			
			if(frame_rate > 0 &&  frame_rate < 100){
				context->track_frame_delay = (1000/frame_rate);
				printk("set frame_rate=%d\n",frame_rate);
			}
		}
		break;
        case CARBACK_IOCTL_STRACK_SET_PARAM:
		{
			track_param_context track_param;
			track_param_context *p = &track_param;
			if(!context->ptrack_param){
				printk(KERN_ALERT "ptrack_param == null,error.\n");
				return -EFAULT;
			}

			if(copy_from_user(&track_param, (void*)arg, sizeof(track_param_context))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_SET_PARAM error.\n");
				return -EFAULT;
			}
			printk("%d %d %d %d.\n",p->track_rect.width,p->track_rect.height,p->car_rect.width,p->car_rect.height);

			if(track_param.track_rect.width == 0 || track_param.track_rect.height == 0){
				printk(KERN_ALERT "set track_param data error.\n");
				return -EFAULT;
			}

			memcpy(context->ptrack_param, &track_param, sizeof(track_param_context));
			context->track_disp_width = track_param.track_rect.width;
			context->track_disp_height= track_param.track_rect.height;
			context->track_disp_xpos  = track_param.track_rect.pos_x;
			context->track_disp_ypos  = track_param.track_rect.pos_y;
			
		}
		break;
        case CARBACK_IOCTL_STRACK_GET_PARAM:
		{	
			track_param_context *p = context->ptrack_param;
			if(!context->ptrack_param){
				printk(KERN_ALERT "ptrack_param == null,error\n");
				return -EFAULT;
			}
			if(copy_to_user((void*)arg, context->ptrack_param, sizeof(track_param_context))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_GET_PARAM error\n");
				return -EFAULT;
			}

			printk("%d %d %d %d.\n",p->track_rect.width,p->track_rect.height,p->car_rect.width,p->car_rect.height);
		}
		break;
        case CARBACK_IOCTL_STRACK_GET_FILETYPE:
		{	
			unsigned int file_type = context->file_type & ~(HEADER2_FILE_FLAG);
			if(copy_to_user((void*)arg, &file_type, sizeof(unsigned int))){
			        printk(KERN_ALERT "CARBACK_IOCTL_STRACK_GET_FILETYPE error\n");
			        return -EFAULT;
			}

			printk("reversingtrack file_type=%d.\n",file_type);
		}
		break;
        case CARBACK_IOCTL_STRACK_GET_IDENTITY:
		{	
			unsigned int identity = *(volatile unsigned int*)g_carback_context->track_data_virtaddr;
			if(copy_to_user((void*)arg, &identity, sizeof(unsigned int))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_GET_FILETYPE error\n");
				return -EFAULT;
			}

			printk("reversingtrack identity=0x%0x.\n",identity);
		}
		break;

        case CARBACK_IOCTL_MRADAR_SET_PARAM:
		{
			mradar_param_context mradar_param;
			mradar_param_context *p = &mradar_param;
			if(!context->pmradar_param){
				//printk(KERN_ALERT "MRADAR SET PARAM, context->pmradar_param null, exit.\n");
				return -EFAULT;
				break;
			}

			if(copy_from_user(&mradar_param, (void*)arg, sizeof(mradar_param_context))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_SET_PARAM error.\n");
				return -EFAULT;
			}
			
			printk("%d %d %d %d.\n",p->mradar_rect[0].pos_x,p->mradar_rect[0].pos_y, p->mradar_rect[0].width, p->mradar_rect[0].height);
			if(mradar_param.mradar_rect[0].width == 0 || mradar_param.mradar_rect[0].height == 0){
				printk(KERN_ALERT "mradar set param error.\n");
				return -EFAULT;
			}

			memcpy(context->pmradar_param, &mradar_param, sizeof(mradar_param_context));
		}
		break;
        case CARBACK_IOCTL_MRADAR_GET_PARAM:
		{	
			mradar_param_context *p = context->pmradar_param;
			if(!context->pmradar_param){
				//printk(KERN_ALERT "MRADAR GET PARAM: context->pmradar_param null, exit\n");
				return -EFAULT;
				break;
			}
			if(copy_to_user((void*)arg, context->pmradar_param, sizeof(mradar_param_context))){
				printk(KERN_ALERT "CARBACK_IOCTL_STRACK_GET_PARAM error\n");
				return -EFAULT;
			}

			printk("%d %d %d %d.\n",p->mradar_rect[0].pos_x,p->mradar_rect[0].pos_y,p->mradar_rect[0].width,p->mradar_rect[0].height);
		}
		break;
		case CARBACK_IOCTL_MRADAR_SET_ID:
		{
			unsigned char pic_id[MRADAR_MAX];

			if(!context->pmradar_param){
				//printk(KERN_ALERT "MRADAR SET ID: context->pmradar_param null, exit\n");
				return -EFAULT;
				break;
			}
				
			if(copy_from_user(&pic_id, (void*)arg, MRADAR_MAX)){
				printk(KERN_ALERT "CARBACK_IOCTL_MRADAR_SET_ID error\n");
				return -EFAULT;
			}
			
			set_disp_mradar_id(pic_id);
		}
		break;
		case CARBACK_IOCTL_GET_DATA_STATUS:
		{
			return context->track_data_status;
		}
		break;
#endif	

    default:
        printk("%s %d: undefined cmd (0x%2X)\n",
            __FUNCTION__, __LINE__, cmd);
        return -EINVAL;
    }
	
    return 0;
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

/* This function is invoked when the device module is loaded into the
 * kernel. It allocates system resources for constructing driver control
 * data structures and initializes them accordingly.
 */
static const struct of_device_id ark_carback_of_match[] = {
	{ .compatible = "arkmicro,ark1668e-carback", },
		{ /* sentinel */ }
};
static int  ark_carback_probe(struct platform_device *pdev)
{
    struct ark_carback *carback;
    struct resource *res; 
	dev_t dev;
    int error = 0,i;

    carback = devm_kzalloc(&pdev->dev, sizeof(*carback), GFP_KERNEL);
    if (!carback)
            return -ENOMEM;
    memset(carback,0,sizeof(struct ark_carback));
	g_carback_context =&carback->context;

    carback->context.detect = devm_gpiod_get(&pdev->dev, "detect", GPIOD_IN);
    if (IS_ERR(carback->context.detect))
            return PTR_ERR(carback->context.detect);

    if (of_property_read_u32(pdev->dev.of_node, "debounce-detect", &carback->debounce_detect)){
            carback->debounce_detect = 20;
    }

	carback->mirror_config = MIRROR_NO;
	if(!of_property_read_u32(pdev->dev.of_node, "mirror-config", &carback->mirror_config)) {
	    if(carback->mirror_config < MIRROR_NO && carback->mirror_config >= MIRROR_END)
	            carback->mirror_config = MIRROR_NO;
	}

	carback->dynamic_track_config = 0;
	if(!of_property_read_u32(pdev->dev.of_node, "dynamic-track", &carback->dynamic_track_config)) {
	    if(carback->dynamic_track_config < 0 && carback->dynamic_track_config > 1)
	            carback->dynamic_track_config = 0;
	}

	gpiod_set_debounce(carback->context.detect, carback->debounce_detect);

    carback->irq = platform_get_irq(pdev, 0);
    if (carback->irq < 0)
            return carback->irq;

    carback->pdev = pdev;

    carback->driver_name = "ark_carback_drv";
    carback->name = "ark_carback";
    carback->major = 0; /* if 0, let system choose */
    carback->minor_start = 0;
    carback->minor_num = 1; /* one dev only */
    carback->num = 1;

    /* register char device */
    if (!carback->major) {
        error = alloc_chrdev_region(
                    &dev,
                    carback->minor_start,
                    carback->num,
                    carback->name
                    );
        if (!error) {
            carback->major = MAJOR(dev);
            carback->minor_start = MINOR(dev);
            //printk(KERN_ERR "%s %d: allocate device major=%d minor=%d\n",
          //      __FUNCTION__, __LINE__,
           //     carback->major, carback->minor_start);
        }
    } else {
        dev = MKDEV(carback->major, carback->minor_start);
       // printk(KERN_ERR "%s %d: dev %d\n", __FUNCTION__, __LINE__, dev);
        error = register_chrdev_region(dev, carback->num,
            (char *)carback->name);
    }

    if (error < 0) {
        printk(KERN_ERR "%s %d: register driver error\n",
            __FUNCTION__, __LINE__);
        goto err_driver_register;
    }

    /* associate the file operations */
    cdev_init(&carback->cdev, &ark_carback_fops);
    carback->cdev.owner    = THIS_MODULE; //driver->owner;
    carback->cdev.ops      = &ark_carback_fops;
    error = cdev_add(&carback->cdev, dev, carback->num);
    if (error) {
        printk(KERN_ERR "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
        goto err_cdev_add;
    }
    //printk(KERN_ERR "%s %d: cdev made, name: %s, major: %d, minor: %d \n",
    //    __FUNCTION__, __LINE__, carback->name, carback->major,
    //    carback->minor_start);

	carback->carback_class = class_create(THIS_MODULE, "carback_class");
    if(IS_ERR(carback->carback_class)) {
    	printk(KERN_ERR "Err: failed in creating ark carback class.\n");
		carback->carback_class = NULL;
        goto err_cdev_add;
    }
	
	carback->carback_device = device_create(carback->carback_class, NULL, dev, NULL, "carback");
	if (IS_ERR(carback->carback_device)) {
    	printk(KERN_ERR "Err: failed in creating ark carback device.\n");
		carback->carback_device = NULL;
        goto err_cdev_add;
	}	
#ifdef CONFIG_REVERSING_TRACK
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ALERT "platform_get_resource faild ...\n");
		return 0;
	}
	printk("res->start = %x,size = %x\n",res->start,resource_size(res)); 

	carback->context.track_data_phyaddr = res->start; 
	carback->context.track_data_virtaddr= (unsigned int)ioremap(carback->context.track_data_phyaddr, resource_size(res)); 

	if(!carback->context.track_data_virtaddr){
		printk(KERN_ALERT "track_data_virtaddr is null");
		goto err_irq;
	}else{
		printk("track_data_virtaddr = %x \n",carback->context.track_data_virtaddr); 
	}
#endif

	register_mcu_interface();

#ifdef CONFIG_REVERSING_TRACK
    carback->context.track_frame_delay = 100;//100ms
    carback->context.track_data_status = 1;
	carback->context.carback_signal = 1;
	carback->context.track_setting = 1;
	carback->context.layer_status= 0;
	carback->context.buffer_num = TRACK_FRAME_NUM;
	carback->context.buffer_index = 0;

	ark_track_get_screen_info(&carback->context.screen_width,&carback->context.screen_height);

	/* initialize rt timer */
	carback->context.track_queue = create_singlethread_workqueue("track_queue");
	if(!carback->context.track_queue) {
    	printk(KERN_ERR "%s %d: , create_singlethread_workqueue fail.\n",__FUNCTION__, __LINE__);	
		goto err_track;
	}
	if(track_paint_init() < 0){
		printk(KERN_ERR "%s %d: ,track_paint_init fail.\n",__FUNCTION__, __LINE__); 
		if(carback->dynamic_track_config)
			ark_set_track_noready();
		unregister_mcu_interface();
	}else{
		if(carback->dynamic_track_config)
			ark_set_track_ready();
	}

	INIT_WORK(&carback->context.track_work, track_paint_work);
	carback->context.track_display_size = carback->context.screen_width * carback->context.screen_height * 4 * carback->context.buffer_num; 
		carback->context.track_display_virtaddr =
			    (unsigned int)dma_alloc_coherent(&pdev->dev, carback->context.track_display_size,
							     &carback->context.track_display_phyaddr, GFP_KERNEL);  
		if (!carback->context.track_display_virtaddr) { 
				printk(KERN_ALERT "%s %d: ,alloc track display buffer failed.\n",__FUNCTION__, __LINE__);  
			} else {   
				timer_setup(&carback->context.track_timer, track_timer_handler, 0);
				
				for(i = 0; i < carback->context.buffer_num; i++){
					carback->context.tdisplay_virtaddr[i] = carback->context.track_display_virtaddr + carback->context.screen_width * carback->context.screen_height * 4*i;
					carback->context.tdisplay_phyaddr[i] = carback->context.track_display_phyaddr + carback->context.screen_width * carback->context.screen_height * 4*i;
				}
			} 
#endif
	ark_carback_dev_init(&carback->context);
	if (!gpiod_get_value(carback->context.detect)) {
		dvr_set_mirror(carback->mirror_config);
		dvr_enter_carback();
		if(get_bootanimation_status())
			ark_disp_set_layer_en(0, 0);   //close bootanmation
		ark_disp_set_layer_en(3, 0);
		carback->context.carback_status = 1;
		//if(carback->context && carback->context.send_mcu_carback)
			//carback->context.send_mcu_carback(true);
#ifdef CONFIG_REVERSING_TRACK
	if (*(volatile unsigned int*)g_carback_context->track_data_virtaddr == MKTAG('R', 'S', 'T', 'K')) { 
			mod_timer(&carback->context.track_timer, jiffies +  msecs_to_jiffies(10));
		}
#endif		
	} else {
		carback->context.carback_status = 0;
		//if(carback_context && carback_context->send_mcu_carback)
		//	carback_context->send_mcu_carback(true);
	}
        error = devm_request_irq(&pdev->dev, carback->irq, ark_carback_intr_handler,0, "ark-carback", carback);
        if (error){
                printk(KERN_ERR "%s %d: can't get assigned carback irq %d, error %d\n",
                            __FUNCTION__, __LINE__, carback->irq, error);
				goto err_irq;
                return error;
        }

	carback->context.dev = &pdev->dev;
	platform_set_drvdata(pdev, carback);
	
	return 0;

err_irq:

	/* free the interrupt channel */
	if (carback->context.gpio_id >= 0) {
		free_irq(gpio_to_irq(carback->context.gpio_id), carback);
        carback->context.gpio_id = -1;
	}
	cdev_del(&carback->cdev);

#ifdef CONFIG_REVERSING_TRACK
	del_timer(&carback->context.track_timer);
err_track:
	/* initialize rt timer */
	if(carback->context.track_queue)
		destroy_workqueue(carback->context.track_queue);
#endif

err_cdev_add:
	if (carback->carback_class) {
		if (carback->carback_device) {
			device_destroy(carback->carback_class, dev);
		}
		class_destroy(carback->carback_class);
	}
	unregister_chrdev_region(dev, carback->num);
err_driver_register:
	kfree(carback);
	return error;
}


/* This function is invoked when the device module is removed from the
 * kernel. It releases all resources to the system.
 */
static int ark_carback_remove(struct platform_device *pdev)
{
    struct ark_carback *carback;
    dev_t dev;

	ark_set_track_noready();

    carback = platform_get_drvdata(pdev);
    if (carback == NULL)
        return -ENODEV;

    dev = MKDEV(carback->major, carback->minor_start);

    /* free the interrupt channel */
	if (carback->context.gpio_id >= 0) {
		free_irq(gpio_to_irq(carback->context.gpio_id), carback);
        carback->context.gpio_id = -1;
	}
    carback->context.track_data_status = 1;
	carback->context.carback_signal = 1;


#ifdef CONFIG_REVERSING_TRACK

	/* initialize rt timer */
	del_timer(&carback->context.track_timer);

	if(carback->context.track_queue)
		destroy_workqueue(carback->context.track_queue);
	
	track_paint_deinit();
	if(carback->context.track_display_virtaddr)
		dma_free_wc(&pdev->dev, carback->context.track_display_size, (void*)carback->context.track_display_virtaddr, 
			carback->context.track_display_phyaddr);
#endif			
	unregister_mcu_interface();

	ark_carback_dev_uninit(&carback->context);

	cdev_del(&carback->cdev);

	unregister_chrdev_region(dev, carback->num);

        kfree(carback);
        return 0;
}


static struct platform_driver ark_carback_driver = {
    .driver     = {
		.name = "ark1668e-carback",
		.of_match_table = of_match_ptr(ark_carback_of_match),
    },
    .probe      = ark_carback_probe,
    .remove     = ark_carback_remove,
};

static int __init ark_carback_init(void)
{
    int ret;

    ret = platform_driver_register(&ark_carback_driver);
    if (ret != 0) {
        printk(KERN_ERR "%s %d: failed to register ark_carback_driver\n",
            __FUNCTION__, __LINE__);
    }

    return ret;
}
late_initcall_sync(ark_carback_init);
