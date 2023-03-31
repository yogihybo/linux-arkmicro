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

#define ADC_CHANNEL_MAX_NUMS		8
#define ADC_ARK1668E_CHANNEL_NUM	4

struct ark_adc {
	struct ark_adc_data	*data;
	struct device		*dev;
	void __iomem		*sys_base;
	void __iomem		*adc_base;
	struct clk			*clk;
	struct completion	completion;
	u32					irq;
	bool				pull_down;
	u32					value[ADC_CHANNEL_MAX_NUMS];
};

struct ark_adc_data {
	int num_channels;
	u32 mask;

	void (*init_hw)(struct ark_adc *info);
	void (*exit_hw)(struct ark_adc *info);
	//void (*clear_irq)(struct ark_adc *info);
	void (*start_conv)(struct ark_adc *info, unsigned long addr);
};

#define ADC_CTL0		0x0
#define ADC_CTL1		0x4
#define ADC_IMR			0x8
#define ADC_STA			0xc
#define ADC_INTER		0x10
#define ADC_DBNCNT		0x14
#define ADC_AUX10		0x18
#define ADC_AUX32		0x1c
#define ADC_AUX54		0x20
#define ADC_AUX76		0x24

#define	AUX_START_INT(ch)			(1 << ((ch) * 3 + 0))
#define	AUX_STOP_INT(ch)       		(1 << ((ch) * 3 + 1))
#define AUX_VALUE_INT(ch)   		(1 << ((ch) * 3 + 2))

#define	AUX_START_INTMASK(ch)			(1 << ((ch) + 0))
#define	AUX_STOP_INTMASK(ch)       		(1 << ((ch) + 8))
#define AUX_VALUE_INTMASK(ch)   		(1 << ((ch) + 16))

#define ARK_ADC_DATX_MASK_12BIT			0xFFF
#define ARK_ADC_DATX_MASK_10BIT			0x3FF
#define ARK_ADC_VOLTAGE_REF				3300// 3.3V
#define ARK_ADC_TIMEOUT					(msecs_to_jiffies(20))


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

static u32 ark1668e_get_aux_value(struct ark_adc *info, unsigned int ch)
{
	u32 val;

	val = readl(info->adc_base + ADC_AUX10 + ch / 2 * 4);
	if (ch & 1)
		val = (val >> 16) & 0xfff;
	else
		val = val & 0xfff;

	return val;
}

static void ark1668e_adc_disable_channel(struct ark_adc *info, unsigned int ch)
{
	u32 val;

	if (ch >= ADC_CHANNEL_MAX_NUMS)
		return;

	val = readl(info->adc_base + ADC_CTL0);
	val &= ~(1 << ch);
	writel(val, info->adc_base + ADC_CTL0);
}

static void ark1668e_adc_enable_channel(struct ark_adc *info, unsigned int ch)
{
	u32 val;

	if (ch >= ADC_CHANNEL_MAX_NUMS)
		return;

	val = readl(info->adc_base + ADC_CTL0);
	val |= 1 << ch;
	writel(val, info->adc_base + ADC_CTL0);
}

static int ark_adc_reg_access(struct iio_dev *indio_dev,
			      unsigned reg, unsigned writeval,
			      unsigned *readval)
{
	struct ark_adc *info = iio_priv(indio_dev);
	u32 val;

	if (readval != NULL) {
		val = readl(info->adc_base + reg);
		*readval = val;
	} else {
		writel(val, info->adc_base + reg);
	}

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
	if (timeout == 0)
		dev_warn(&indio_dev->dev, "Conversion timed out!\n");

	*val = info->value[chan->address];
	*val2 = 0;
	ret = IIO_VAL_INT;

	ark1668e_adc_disable_channel(info, chan->address);

	mutex_unlock(&indio_dev->mlock);

	return ret;
}

static const struct iio_info ark_adc_iio_info = {
	.read_raw = &ark_read_raw,
	.debugfs_reg_access = &ark_adc_reg_access,
};

static void ark1668e_adc_init_hw(struct ark_adc *info)
{
	unsigned int val;

	//soft reset
	val = readl(info->sys_base + 0x74);
	val &= ~(1 << 2);
	writel(val, info->sys_base + 0x74);
	udelay(1);
	val |= 1 << 2;
	writel(val, info->sys_base + 0x74);
	udelay(1);

	val = readl(info->adc_base + ADC_CTL1);
	val |= 0x3f << 16;
	writel(val, info->adc_base + ADC_CTL1);

	val = readl(info->adc_base + ADC_CTL0);
	val &= ~(1 << 31);
	val &= ~0xff;
	val |= 0xff << 16;
	writel(val, info->adc_base + ADC_CTL0);

	//set transform interval 10ms
	val = readl(info->adc_base + ADC_INTER);
	val &= ~0xffff;;
	val |= 10000;
	writel(val, info->adc_base + ADC_INTER);

	//enable interrupt
	writel(0x0, info->adc_base + ADC_IMR);
}

static void ark1668e_adc_exit_hw(struct ark_adc *info)
{
	//printk(KERN_ALERT "### exit\n");

}

static void ark1668e_adc_start_conv(struct ark_adc *info, unsigned long addr)
{
	int i;

	if (addr >= ADC_CHANNEL_MAX_NUMS)
		return;

	if (info->pull_down)
		info->value[addr] = 0;
	else
		info->value[addr] = ARK_ADC_VOLTAGE_REF;

	for (i = 0; i < ADC_CHANNEL_MAX_NUMS; i++) {
		if (i == addr)
			ark1668e_adc_enable_channel(info, i);
		else
			ark1668e_adc_disable_channel(info, i);
	}
}

static const struct ark_adc_data ark1668e_adc_data = {
	.num_channels	= ADC_ARK1668E_CHANNEL_NUM,
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
	u32 val, range, status;
	int i;

	status = readl(info->adc_base + ADC_STA);
	writel(0, info->adc_base + ADC_STA);

	//printk(KERN_ALERT "++++++ark_adc_intr_handler status=0x%x\n", status);
	for (i = 0; i < ADC_CHANNEL_MAX_NUMS; i++) {
		if (status & AUX_START_INT(i)) {
			//printk(KERN_ALERT "ch%d start int.\n", i);
		}

		if (status & AUX_STOP_INT(i)) {
			//printk(KERN_ALERT "ch%d stop int.\n", i);
			if (info->pull_down)
				info->value[i] = 0;
			else
				info->value[i] = ARK_ADC_VOLTAGE_REF;
		}

		if (status & AUX_VALUE_INT(i)) {
			//printk(KERN_ALERT "ch%d value int.\n", i);
			val = ark1668e_get_aux_value(info, i);
			range = (info->data->mask == ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024;
			info->value[i] = val * ARK_ADC_VOLTAGE_REF / range;
			complete(&info->completion);
		}
	}

	return IRQ_HANDLED;
}

static int ark_adc_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct iio_dev *indio_dev = NULL;
	struct ark_adc *info = NULL;
	struct resource	*res = NULL;
	int ret = -ENODEV;
	int i;

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
	info->adc_base = devm_ioremap_resource(&pdev->dev, res);
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

	info->pull_down = of_property_read_bool(np, "pull-down");
	for (i = 0; i < ADC_CHANNEL_MAX_NUMS; i++) {
		if (info->pull_down)
			info->value[i] = 0;
		else
			info->value[i] = ARK_ADC_VOLTAGE_REF;
	}

	info->dev = &pdev->dev;

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

	//printk(KERN_ALERT "++++++ark1668e_adc_probe success\n");
	return 0;

err_iio_device_register:
err_irq:
	if(info->sys_base)
		devm_iounmap(&pdev->dev, info->sys_base);
err_resource:
err_devm_iio_device_alloc:

	return ret;
}

static int ark_adc_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);
	struct ark_adc *info = iio_priv(indio_dev);

	if(!info)
		return -ENODEV;

	iio_device_unregister(indio_dev);
	if(info->sys_base)
		devm_iounmap(&pdev->dev, info->sys_base);

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
