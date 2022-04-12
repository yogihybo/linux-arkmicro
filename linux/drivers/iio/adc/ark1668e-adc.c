#include <linux/kernel.h>
#include <linux/module.h> 
#include <linux/platform_device.h> 
#include <linux/io.h> 
#include <linux/iio/iio.h>
#include <linux/iio/machine.h>
#include <linux/iio/driver.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/of_platform.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/completion.h>


enum ADC_MODE{
	ARK_ADC_MODE_NONE,
	ARK_ADC_MODE_BATTERY,
	ARK_ADC_MODE_KEY,
	ARK_ADC_MODE_END,
};

struct ark_adc {
	struct ark_adc_data	*data;
	struct device		*dev;
	void __iomem		*sys_base;
	void __iomem		*adc_base;
	struct clk			*clk;
	struct completion	completion;
	u32					irq;
	u32 				mode[4];	//adc channel[n] mode(battery, key, ...)
	u32					value1[4];	//adc channel[n] temp value
	u32					value2[4];	//adc channel[n] final value
};

struct ark_adc_data {
	int num_channels;
	u32 mask;

	void (*init_hw)(struct ark_adc *info);
	void (*exit_hw)(struct ark_adc *info);
	//void (*clear_irq)(struct ark_adc *info);
	void (*start_conv)(struct ark_adc *info, unsigned long addr);
};

#define  BAT_CHANNEL           		1
#define  TOUCHPAN_CHANNEL      		2
#define  AUX0_CHANNEL          		3
#define  AUX1_CHANNEL          		4
#define  AUX2_CHANNEL          		5
#define  AUX3_CHANNEL          		6

#define  AUX0_START_INT       		(1<<0)
#define  AUX0_STOP_INT        		(1<<1)
#define  AUX0_VALUE_INT       		(1<<2)
#define  AUX1_START_INT       		(1<<3)
#define  AUX1_STOP_INT        		(1<<4)
#define  AUX1_VALUE_INT       		(1<<5)
#define  AUX2_START_INT       		(1<<6)
#define  AUX2_STOP_INT        		(1<<7)
#define  AUX2_VALUE_INT       		(1<<8)
#define  AUX3_START_INT       		(1<<9)
#define  AUX3_STOP_INT        		(1<<10)
#define  AUX3_VALUE_INT       		(1<<11)

#define	AUX_START_INT(ch)			(1 << ((ch)*3 + 0))
#define	AUX_STOP_INT(ch)       		(1 << ((ch)*3 + 1))
#define AUX_VALUE_INT(ch)   		(1 << ((ch)*3 + 2))

#define ARK1668_ADC_CHANNEL_MAX_NUMS	4
#define ARK1668E_ADC_CHANNEL_MAX_NUMS	4
#define ARKN141_ADC_CHANNEL_MAX_NUMS	2
#define ARK_ADC_DATX_MASK_12BIT			0xFFF
#define ARK_ADC_DATX_MASK_10BIT			0x3FF
#define ARK_BATTERY_VOLTAGE_REF			3300// 3.3V
#define ARK_ADC_TIMEOUT					(msecs_to_jiffies(100))
#define ARK_ADC_USE_PULL_UP_REGISTER	//ADC connect pull up register, have interrupe when voltage in 0V ~ 3V
//#define ARK_ADC_USE_PULL_DOWM_REGISTER	//ADC connect pull dowm register, have interrupt when voltage in 0.3V ~ 3.3V



#define ADC_CHANNEL(_index, _id) {		\
	.type = IIO_VOLTAGE,				\
	.indexed = 1,						\
	.channel = _index,					\
	.address = _index,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.datasheet_name = _id,				\
}

static const struct iio_chan_spec ark_adc_iio_channels[] = {
	ADC_CHANNEL(0, "adc0"),
	ADC_CHANNEL(1, "adc1"),
	ADC_CHANNEL(2, "adc2"),
	ADC_CHANNEL(3, "adc3"),
};

static int ark_adc_reg_access(struct iio_dev *indio_dev,
			      unsigned reg, unsigned writeval,
			      unsigned *readval)
{
	//struct ark_adc *info = iio_priv(indio_dev);
	//printk(KERN_ALERT "### ark_adc_reg_access\n");

	if (!readval)
		return -EINVAL;
	
	//*readval = readl(info->adc_base + reg);

	return 0;
}


static int ark_read_raw(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val,
				int *val2,
				long mask)
{
	struct ark_adc *info = iio_priv(indio_dev);
	unsigned long timeout;
	int ret = 0;
	
	mutex_lock(&indio_dev->mlock);
	reinit_completion(&info->completion);
	
	/* Select the channel to be used and Trigger conversion */
	if (info->data->start_conv)
		info->data->start_conv(info, chan->address);

	timeout = wait_for_completion_timeout(&info->completion, ARK_ADC_TIMEOUT);
	
	if (timeout == 0) {
		dev_warn(&indio_dev->dev, "Conversion timed out! Resetting\n");
		if (info->data->init_hw)
			info->data->init_hw(info);
		ret = -ETIMEDOUT;
	} else {
		*val = info->value2[chan->address];
		*val2 = 0;
		ret = IIO_VAL_INT;
	}
	mutex_unlock(&indio_dev->mlock);
	//printk(KERN_ALERT "### READ :%d\n", readl(info->adc_base+0x14));

	return ret;
}

static const struct iio_info ark_adc_iio_info = {
	.read_raw = &ark_read_raw,
	.debugfs_reg_access = &ark_adc_reg_access,
};

static void ark1668e_adc_init_hw(struct ark_adc *info)
{
	unsigned int val;

	val = readl(info->sys_base+0x74);
	val |= (1 << 2);
	writel(val, info->sys_base+0x74);


	val = readl(info->adc_base+0x4);
	val |= (0x3f<<16);
	writel(val, info->adc_base+0x4);

	val = readl(info->adc_base+0x0);
	val &=~(1<<31);
	writel(val, info->adc_base+0x0);
	val |=1<<26;
	writel(val, info->adc_base+0x0);
	val |=0xff;
	writel(val, info->adc_base+0x0);
	val |=0xff << 16;
	writel(val, info->adc_base+0x0);

	val = readl(info->adc_base+0x10);
	val |= 0xffff;
	writel(val, info->adc_base+0x10);

	val = readl(info->adc_base+0x08);
	val &=~0xffffff;
	writel(val, info->adc_base+0x08);

}

static void ark1668e_adc_exit_hw(struct ark_adc *info)
{
	//printk(KERN_ALERT "### exit\n");
	
}

static void ark1668e_adc_start_conv(struct ark_adc *info, unsigned long addr)
{
	u32 value = info->value1[addr];

#ifdef ARK_ADC_USE_PULL_UP_REGISTER
	if((value >= 0) && (value <= 3000))
		info->value2[addr] = value;
	else
		info->value2[addr] = ARK_BATTERY_VOLTAGE_REF;
	info->value1[addr] = ARK_BATTERY_VOLTAGE_REF;
#else
	info->value2[addr] = value;
#endif
	
	complete(&info->completion);
}

static const struct ark_adc_data ark1668e_adc_data = {
	.num_channels	= ARK1668E_ADC_CHANNEL_MAX_NUMS,
	.mask			= ARK_ADC_DATX_MASK_12BIT,
	.init_hw		= ark1668e_adc_init_hw,
	.exit_hw		= ark1668e_adc_exit_hw,
	.start_conv		= ark1668e_adc_start_conv,
};


static const struct of_device_id ark_adc_match[] = {
	{
		.compatible = "arkmicro,ark1668e-adc",
		.data = &ark1668e_adc_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, ark_adc_match);

static struct ark_adc_data *ark_adc_get_data(struct platform_device *pdev)
{
	const struct of_device_id *match = of_match_node(ark_adc_match, pdev->dev.of_node);
	
	return (struct ark_adc_data *)match->data;
}

static irqreturn_t ark_adc_intr_handler(int irq, void *dev_id)
{
	struct ark_adc *info = (struct ark_adc *)dev_id;
	u32 val,range,status;
	//printk(KERN_ALERT "++++++ark_adc_intr_handler entry\n");
	
	status = readl(info->adc_base+0x0c);

	if(status&(1<<23))
	{
		val = readl(info->adc_base+0x24);
		//printk(KERN_ALERT "aux7 value is 0x%x\r\n",(val>>16)&0xfff);
		val = ~(1<<23);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<22))
	{
		val = ~(1<<22);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<21))
	{
		val = ~(1<<21);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<20))
	{
		val = readl(info->adc_base+0x24);
		//printk(KERN_ALERT "aux6 value is 0x%x\r\n",(val)&0xfff);
		val = ~(1<<20);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<19))
	{
		val = ~(1<<19);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<18))
	{
		val = ~(1<<18);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<17))
	{
		val = readl(info->adc_base+0x20);
		//printk(KERN_ALERT "aux5 value is 0x%x\r\n",(val>>16)&0xfff);
		val = ~(1<<17);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<16))
	{
		val = ~(1<<16);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<15))
	{
		val = ~(1<<15);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<14))
	{
		val = readl(info->adc_base+0x20);
		//printk(KERN_ALERT "aux4 value is 0x%x\r\n",(val)&0xfff);
		val = ~(1<<14);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<13))
	{
		val = ~(1<<13);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<12))
	{
		val = ~(1<<12);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<11))
	{
		val = readl(info->adc_base+0x1c);
		range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024);
		info->value1[3] = ((val>>16)&0xfff) * ARK_BATTERY_VOLTAGE_REF / range;
		//printk(KERN_ALERT "aux3 value = %d \n",(val>>16)&0xfff);
		val = ~(1<<11);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<10))
	{
		val = ~(1<<10);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<9))
	{
		val = ~(1<<9);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<8))
	{
		val = readl(info->adc_base+0x1c);
		range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024);
		info->value1[2] = ((val)&0xfff) * ARK_BATTERY_VOLTAGE_REF / range;
		val = ~(1<<8);
		writel(val, info->adc_base+0x0c);
		
	}
	if(status&(1<<7))
	{
		val = ~(1<<7);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<6))
	{
		val = ~(1<<6);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<5))
	{
		val = readl(info->adc_base+0x18);
		range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024);
		info->value1[1] = ((val>>16)&0xfff) * ARK_BATTERY_VOLTAGE_REF / range;
		val = ~(1<<5);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<4))
	{
		val = ~(1<<4);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<3))
	{
		val = ~(1<<3);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<2))
	{
		val = readl(info->adc_base+0x18);
		range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024);
		info->value1[0] = ((val)&0xfff) * ARK_BATTERY_VOLTAGE_REF / range;
		//printk(KERN_ALERT "aux0 value = 0x%x,info->value1[0] = %d\n",val&0xfff,info->value1[0]);
		val = ~(1<<2);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<1))
	{
		val = ~(1<<1);
		writel(val, info->adc_base+0x0c);
	}
	if(status&(1<<0))
	{
		val = ~(1<<0);
		writel(val, info->adc_base+0x0c);
	}

	return IRQ_HANDLED;
}

static int ark_adc_probe(struct platform_device *pdev)
{
	//struct device_node *np = pdev->dev.of_node;
	struct iio_dev *indio_dev = NULL;
	struct ark_adc *info = NULL;
	struct resource	*res = NULL;
	int ret = -ENODEV;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(struct ark_adc));
	if (!indio_dev) {
		dev_err(&pdev->dev, "failed allocating iio device\n");
		return -ENOMEM;
	}
	
	info = iio_priv(indio_dev);

	//get data
	info->data = ark_adc_get_data(pdev);
	if (!info->data) {
		dev_err(&pdev->dev, "failed getting ark_adc_data\n");
		goto err_devm_iio_device_alloc;
	}

	//adc resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->adc_base= devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->adc_base)) {
		ret = PTR_ERR(info->adc_base);
		goto err_resource;
	}

	//sys resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (IS_ERR(res)) {
		ret = PTR_ERR(res);
		goto err_resource;
    }
	//info->sys_base = devm_ioremap_resource(&pdev->dev, res);
	info->sys_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(info->sys_base)) {
		ret = PTR_ERR(info->sys_base);
		goto err_resource;
	}

	//irq
	info->irq = platform_get_irq(pdev, 0);
	if (info->irq < 0) {
		dev_err(&pdev->dev, "no irq resource\n");
		ret = info->irq;
		goto err_irq;
	}

	//ret = request_irq(info->irq, ark_adc_intr_handler, 0, dev_name(&pdev->dev), info);
    ret = devm_request_irq(
                &pdev->dev,
                info->irq,
                ark_adc_intr_handler,
                0,//IRQF_SHARED|IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, //SA_SHIRQ,
                dev_name(&pdev->dev),//"adc",
                info
                );
	if (ret < 0) {
		dev_err(&pdev->dev, "failed requesting irq, irq = %d\n", info->irq);
		goto err_irq;
	}

	info->dev = &pdev->dev;
	info->value1[0] = info->value1[1] = info->value1[2] = info->value1[3] =ARK_BATTERY_VOLTAGE_REF;
	
	platform_set_drvdata(pdev, indio_dev);
	indio_dev->name = dev_name(&pdev->dev);
	indio_dev->dev.parent = &pdev->dev;
	indio_dev->dev.of_node = pdev->dev.of_node;
	indio_dev->info = &ark_adc_iio_info;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->channels = ark_adc_iio_channels;
	indio_dev->num_channels = info->data->num_channels;

	ret = iio_device_register(indio_dev);
	if(ret)
		goto err_iio_device_register;

	//ret = of_platform_populate(np, ark_adc_match, NULL, &indio_dev->dev);
	//if (ret < 0) {
	//	dev_err(&pdev->dev, "failed adding child nodes\n");
	//	goto err_of_populate;
	//}

	init_completion(&info->completion);

	if (info->data->init_hw)
		info->data->init_hw(info);

	printk(KERN_ALERT "++++++ark1668e_adc_probe success\n");
	return 0;

//err_iio:
	iio_device_unregister(indio_dev);
err_iio_device_register:
	//free_irq(info->irq, info);
	//devm_free_irq(&pdev->dev, info->irq, info);
err_irq: 
	//if(info->sys_base)
	//	devm_iounmap(&pdev->dev, info->sys_base);
err_resource:
	//if(info->adc_base)
	//	devm_iounmap(&pdev->dev, info->adc_base);
err_devm_iio_device_alloc:
	//if(indio_dev)
	//	devm_iio_device_free(&pdev->dev, indio_dev);
	
	return ret;
}

static int ark_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct ark_adc *info = iio_priv(indio_dev);

	if(!info)
		return -ENODEV;
	
	//device_for_each_child(&indio_dev->dev, NULL, ark_adc_remove_devices);
	iio_device_unregister(indio_dev);
	//free_irq(info->irq, info);
	//devm_free_irq(&pdev->dev, info->irq, info);
	//if(info->sys_base)
	//	devm_iounmap(&pdev->dev, info->sys_base);
	//if(info->adc_base)
	//	devm_iounmap(&pdev->dev, info->adc_base);
	//if(indio_dev)
	//	devm_iio_device_free(&pdev->dev, indio_dev);

	return 0;
}

static struct platform_driver ark_adc_driver = {
	.probe	= ark_adc_probe,
	.remove = ark_adc_remove,
	.driver	= {
		.name	= "ark1668e-adc",
		.owner	= THIS_MODULE,
		.of_match_table	= ark_adc_match,
	},
};


module_platform_driver(ark_adc_driver);


MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Arkmicro ADC Driver");
