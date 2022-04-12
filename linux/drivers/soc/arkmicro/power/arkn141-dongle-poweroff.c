/*
*
*arkn141_dongle_power_off driver
*Author:		Arkmicro
*Created:	2020-05-11
* Suit:		arkn141 5G WiFi dongle board
*
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/setup.h>
#include <asm/gpio.h>    
#include <asm/io.h>
#include <asm/switch_to.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include<linux/timer.h>
#include<linux/jiffies.h>

//#define KILL_FASYNC_MODE
#define POLL_MODE

#define ARK_POWER_IOC_MAGIC  'm'
#define ARK_POWER_OFF       _IO(ARK_POWER_IOC_MAGIC,  0)
#define ARK_POWER_ON       	_IO(ARK_POWER_IOC_MAGIC,  1)
#define ARK_POWER_RESTART       	_IO(ARK_POWER_IOC_MAGIC,  2)
#define TIMER_TIMEOUT  200		//单位ms      //5*1000ms     //200ms
#define SIGIO_TIMEOUT  1000

/* Convert GPIO signal to GPIO pin number */
//#define GPIO_TO_PIN(bank, gpio)	(16 * (bank) + (gpio))	//old
//#define GPIO_TO_PIN(bank, gpio)	(32 * (bank) + (gpio))	//new
//#define GPIO_NUM GPIO_TO_PIN(3,5)  //:GPIO3_5
#define GPIO_PWR_OFF 30		//控制IO:PWR_OFF
#define GPIO_ACC_DE 32		//检测IO:ACC_DE
#define POWEROFF_MAJOR 200

static struct class *arkn141_dev_class;
static struct fasync_struct *fasync_queue; //异步通知队列

static unsigned int gpioToIrq;
int major;

struct timer_list timer;			//定义一个内核定时器
static bool is_power_off= false;
static bool wait_timeout= false;
struct device *poweroff_dev = NULL;
int gpio_acc_value;
unsigned int acc_Trigger;
//unsigned int wait_timeout = 0;

DECLARE_WAIT_QUEUE_HEAD(accoff_waitq);//注册一个等待队列button_waitq

int arkn141_dongle_poweroff_open(struct inode *inode,struct file *filp)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	return 0;

}

int arkn141_dongle_poweroff_release(struct inode *inode,struct file *filp)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	return 0;
}

/*
:write
*/
ssize_t arkn141_dongle_poweroff_write(struct file *file,const char __user *buf,size_t count,loff_t *f_pos)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	return 0;
}

/*
:read
*/
ssize_t arkn141_dongle_poweroff_read(struct file *filp,char __user *buf,size_t count,loff_t *f_pos)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	int ret;

    	if(count != 1)
    	{
		printk("%s----------------%d: read error!!!\n",__FUNCTION__, __LINE__);
        	return -1;
    	}

	wait_event_interruptible(accoff_waitq, gpio_acc_value);//将当前进程放入等待队列button_waitq中
	ret = copy_to_user(buf, &gpio_acc_value, 1);
    	acc_Trigger = 0;								//按键已经处理可以继续睡眠	

	if(ret)
    	{
        	printk("%s----------------%d: copy_to_user error!!!\n",__FUNCTION__, __LINE__);
       		return -1;
    	}
		
	return ret;
}

 /*
 .poll
 */
static unsigned int arkn141_dongle_poweroff_poll(struct file *fp, poll_table * wait)  //fp:文件  wait:
{
         unsigned int ret = 0; 
         poll_wait(fp, &accoff_waitq, wait);
		 
         if(acc_Trigger)              //中断事件标志, 1:退出休眠状态     0:进入休眠状态 
         	ret |= POLLIN | POLLRDNORM;

         return ret;     //当超时,就返给应用层为0 ,被唤醒了就返回POLLIN | POLLRDNORM ;

}

static long arkn141_dongle_poweroff_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	unsigned long error = 0;
	
	switch (cmd)
	{
		case ARK_POWER_OFF:
        	{
			printk("ARK_POWER_OFF\n");
			if (!gpio_is_valid(GPIO_PWR_OFF))
				return -1;
			
			gpio_direction_output(GPIO_PWR_OFF,0);
            		break;
        	}
		case ARK_POWER_ON:
        	{
			printk("ARK_POWER_ON\n");
			if (!gpio_is_valid(GPIO_PWR_OFF))
				return -1;
			
			gpio_direction_output(GPIO_PWR_OFF,1);
            		break;
        	}
		case ARK_POWER_RESTART:
        	{
			printk("ARK_POWER_RESTART\n");
            		break;
        	}
		default:
            		printk("%s: error cmd 0x%x\n", __func__, cmd);
            		error = -EFAULT;
	}
	
	 return error;
}

static void ark_power_off_handler(struct timer_list *t)
{
#ifdef POLL_MODE
	if(wait_timeout){
		printk("ACC OFF WAITING TIMEOUT:POWER_OFF\n");
		if (!gpio_is_valid(GPIO_PWR_OFF))
			return -1;
		gpio_direction_output(GPIO_PWR_OFF,0);
	}
	if(gpio_acc_value)
	{
		mod_timer(&timer, jiffies +  msecs_to_jiffies(SIGIO_TIMEOUT));//重置定时器，等待未收到反馈则自动关机
		wait_timeout = true;

		/**/
		//.poll mode
		wake_up_interruptible(&accoff_waitq);   //唤醒休眠的进程 
		acc_Trigger = 1;
		/**/

		/**
		//SIGIO mode
		printk("Send SIGIO to client\n");
		if (fasync_queue) {
			kill_fasync(&fasync_queue, SIGIO, POLL_IN);
		}
		**/
	}
	else{
		del_timer(&timer);			//删除定时器
		is_power_off = false;
	}
#endif
}

/***************************************************************
:fasync
***************************************************************/
static int arkn141_dongle_poweroff_fasync(int fd, struct file * filp, int on)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	//int retval;
	//retval=fasync_helper(fd,filp,on,&fasync_queue);
	//if(retval<0)
	//	return retval;
	return 0;
}

static const struct file_operations arkn141_dongle_poweroff_fops={
	.owner=THIS_MODULE,
	.open=arkn141_dongle_poweroff_open,
	.unlocked_ioctl= arkn141_dongle_poweroff_ioctl,
	.release=arkn141_dongle_poweroff_release,
	.fasync=arkn141_dongle_poweroff_fasync,
	.write=arkn141_dongle_poweroff_write,
	.read=arkn141_dongle_poweroff_read,
	.poll =arkn141_dongle_poweroff_poll,
    	
};

/*
irqreturn_t
*/ 
static irqreturn_t arkn141_dongle_poweroff_interrupt (int irq, void *dev_id)
{
	printk("arkn141 dongle power off driver irq work !!!\n");	
#ifdef POLL_MODE
	if (!gpio_is_valid(GPIO_ACC_DE))
		return -1;
	gpio_acc_value = gpio_get_value(GPIO_ACC_DE);//acc_off_value:1  acc_on_value:0
	//printk("get gpio(32) value= %d\n",gpio_acc_value);	

	if(!is_power_off)
	{
		//printk("------------>Start_timer:%d ms<------------\n",TIMER_TIMEOUT);	
		timer_setup(&timer, ark_power_off_handler, 0);     //初始化定时器
		timer.expires = jiffies +  msecs_to_jiffies(TIMER_TIMEOUT);	//设定超时时间，(TIMER_TIMEOUT)ms
		add_timer(&timer); 									//添加定时器，定时器开始生效
		is_power_off = true;
	}
	else
	{
		//printk("------------>Reset timer:%d ms<------------\n",TIMER_TIMEOUT);
		//del_timer(&timer);//删除定时器
		mod_timer(&timer, jiffies +  msecs_to_jiffies(TIMER_TIMEOUT));//重新设置定时器
	}
#endif

	return IRQ_HANDLED;
}

/**
static int arkn141_dongle_poweroff_probe(struct platform_device *pdev)
{
	printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	return 0;
}

static int arkn141_dongle_poweroff_remove(struct platform_device *pdev)
{
	printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	return 0;
}
**/

int arkn141_dongle_poweroff_drv_init(void)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	int rtn;
	unsigned long req_flags = IRQF_TRIGGER_RISING;//上升沿触发:IRQF_TRIGGER_RISING		下降沿触发:IRQF_TRIGGER_FALLING
	
	major = register_chrdev(0,"arkn141_poweroff",&arkn141_dongle_poweroff_fops);
	if(major<0){
		printk("Unable to register character device %d!/n",major);
		return major;
	}

	arkn141_dev_class = class_create(THIS_MODULE, "arkn141_poweroff_class");
	poweroff_dev = device_create(arkn141_dev_class, NULL, MKDEV(major, 0), NULL,"arkn141_poweroff");

	if (!gpio_is_valid(GPIO_ACC_DE)){
		printk("gpio_is_valid failed!!!!!\n"); 
		return -1;
	}

	//int ret = gpio_get_value(GPIO_ACC_DE);
	//printk("get gpio(32) value= %d\n",ret);
	
	rtn = gpio_request(GPIO_ACC_DE, "acc_off");
	if(rtn!=0){
		printk("acc_off irq pin request io failed.\n");
	}

	gpioToIrq = gpio_to_irq(GPIO_ACC_DE);
	          
	rtn = request_irq(gpioToIrq, arkn141_dongle_poweroff_interrupt,req_flags,"acc_off", NULL); //上升沿触发
	if (rtn<0) {
		printk("acc_off request irq false!!!!!\n");
	} else {
		printk("acc_off request irq success: %d\n",gpioToIrq);
	}
	printk("arkn141_dongle_poweroff_drv_init sucessful!!!\n");
	return 0;
}

void arkn141_dongle_poweroff_drv_exit(void)
{
	//printk("--------%s----------------%d\n", __FUNCTION__, __LINE__); 
	//卸载相应的设备驱动 
	unregister_chrdev(major,"arkn141_poweroff");     
	device_destroy(arkn141_dev_class,MKDEV(major, 0));
	class_destroy(arkn141_dev_class);
	
	//释放GPIO
	gpio_free(GPIO_ACC_DE);
	
	printk("arkn141_dongle_poweroff_drv_exit sucessful!!!\n");
}

/**
static const struct of_device_id arkn141_poweroff_of_match[] = {
	{ .compatible = "arkn141_poweroff", },
	{ }
};
MODULE_DEVICE_TABLE(of, arkn141_poweroff_of_match);

static struct platform_driver arkn141_poweroff_driver = {
	.driver = {
		.name = "arkn141_poweroff",
		//.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(arkn141_poweroff_of_match),
	},
	.probe = arkn141_dongle_poweroff_probe,
	.remove = arkn141_dongle_poweroff_remove,
};
**/

module_init(arkn141_dongle_poweroff_drv_init);
module_exit(arkn141_dongle_poweroff_drv_exit);
//module_platform_driver(arkn141_poweroff_driver);

MODULE_DESCRIPTION("arkn141 acc poweroff driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:arkn141-poweroff");
