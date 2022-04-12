/*
 * arkmicro pwmtest driver
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#define PWM_MAX_NUM		4

#define PWMTEST_IOCTL_BASE							0x9F
#define PWMTEST_IOCTL_GET_PWM0_COUNT				_IOR(PWMTEST_IOCTL_BASE, 0, int)
#define PWMTEST_IOCTL_GET_PWM1_COUNT				_IOR(PWMTEST_IOCTL_BASE, 1, int)
#define PWMTEST_IOCTL_GET_PWM2_COUNT				_IOR(PWMTEST_IOCTL_BASE, 2, int)
#define PWMTEST_IOCTL_GET_PWM3_COUNT				_IOR(PWMTEST_IOCTL_BASE, 3, int)

struct ark_pwmtest {
    int irqs[PWM_MAX_NUM];
    struct gpio_desc *gpios[PWM_MAX_NUM];
	unsigned int pwmcount[PWM_MAX_NUM];
    int debounce_detect;
};

static struct ark_pwmtest *g_pwmtest = NULL; 

static int pwmtest_misc_open(struct inode *inode, struct file *filp)
{
    filp->private_data = g_pwmtest;

    return 0;
}

static int pwmtest_misc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static long pwmtest_misc_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	struct ark_pwmtest *pwmtest = (struct ark_pwmtest *)filp->private_data;
	int error = 0;

	switch (cmd) {
	case PWMTEST_IOCTL_GET_PWM0_COUNT:
        if (copy_to_user((void*)arg, &pwmtest->pwmcount[0], sizeof(pwmtest->pwmcount[0]))) {
            printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
            error = -EFAULT;
        }
		break;

	case PWMTEST_IOCTL_GET_PWM1_COUNT:
        if (copy_to_user((void*)arg, &pwmtest->pwmcount[1], sizeof(pwmtest->pwmcount[1]))) {
            printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
            error = -EFAULT;
        }
		break;

	case PWMTEST_IOCTL_GET_PWM2_COUNT:
        if (copy_to_user((void*)arg, &pwmtest->pwmcount[2], sizeof(pwmtest->pwmcount[2]))) {
            printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
            error = -EFAULT;
        }
		break;

	case PWMTEST_IOCTL_GET_PWM3_COUNT:
        if (copy_to_user((void*)arg, &pwmtest->pwmcount[3], sizeof(pwmtest->pwmcount[3]))) {
            printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
            error = -EFAULT;
        }
		break;

	default:
	    printk("%s %d: undefined cmd (0x%2X)\n", __FUNCTION__, __LINE__, cmd);
	    error = -EINVAL;
	}

	return error;
}

const struct file_operations pwmtest_misc_fops = {
	.owner          =	THIS_MODULE,
	.llseek         =	no_llseek,
	.open           =	pwmtest_misc_open,
	.release        =	pwmtest_misc_release,
	.unlocked_ioctl =	pwmtest_misc_ioctl,
};

static struct miscdevice pwmtest_misc_device = {
	MISC_DYNAMIC_MINOR,
	"pwmtest",
	&pwmtest_misc_fops
};

static irqreturn_t pwmtest_interrupt(int irq, void *dev_id)
{
    unsigned int *pwmcount = (unsigned int*)dev_id;

	*pwmcount = *pwmcount + 1;

    return IRQ_HANDLED;
}

static int ark_pwmtest_probe(struct platform_device *pdev)
{
    struct ark_pwmtest *pwmtest;
    int err;
	int i;

    pwmtest = devm_kzalloc(&pdev->dev, sizeof(*pwmtest), GFP_KERNEL);
    if (!pwmtest)
    	return -ENOMEM;

    if (of_property_read_u32(pdev->dev.of_node, "debounce-detect", &pwmtest->debounce_detect)) {
    	pwmtest->debounce_detect = 20;
    }

	for (i = 1; i < PWM_MAX_NUM; i++) {
		pwmtest->gpios[i] = devm_gpiod_get_index(&pdev->dev, "pwm", i - 1, GPIOD_IN);
		if (IS_ERR(pwmtest->gpios[i]))
			return PTR_ERR(pwmtest->gpios[i]);	
		gpiod_set_debounce(pwmtest->gpios[i], pwmtest->debounce_detect);


    	pwmtest->irqs[i] = platform_get_irq(pdev, i - 1);
    	if (pwmtest->irqs[i] < 0)
            return pwmtest->irqs[i];

		err = devm_request_irq(&pdev->dev, pwmtest->irqs[i], pwmtest_interrupt, 0, "ark-pwmtest", &pwmtest->pwmcount[i]);
		if (err){
	        printk(KERN_ERR "%s %d: can't get assigned pwmtest irq %d, error %d\n",
	                    __FUNCTION__, __LINE__, pwmtest->irqs[i], err);
	        return err;
		}		
	}

    /* Register the miscdevice */
	err = misc_register(&pwmtest_misc_device);
	if (err) {
		dev_err(&pdev->dev, "unable to register miscdevice\n");
		return err;
	}

    g_pwmtest =  pwmtest;       

    return 0;  
}

static int ark_pwmtest_remove(struct platform_device *pdev)
{
	misc_deregister(&pwmtest_misc_device);
	return 0;
}

static const struct of_device_id ark_pwmtest_of_match[] = {
	{ .compatible = "arkmicro,ark-pwmtest", },
	{ /* sentinel */ }
};

static struct platform_driver vdec_of_driver = {
	.driver		= {
		.name	= "ark-pwmtest",
		.owner	= THIS_MODULE,
		.of_match_table	= of_match_ptr(ark_pwmtest_of_match),
	},
	.probe 		= ark_pwmtest_probe,
	.remove		= ark_pwmtest_remove,
};

module_platform_driver(vdec_of_driver);

