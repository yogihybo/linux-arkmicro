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

static void arn141_adc_set_transform_interval(struct ark_adc *info, u32 interval_millisecond)
{
	u32 reg;

	// 1M Hz sample clock
	// adc_clk = clk_24m/((adc_clk_div+1)*2).
	reg = readl(info->sys_base+0x64);
	reg &= ~0x7FFF;
	reg |= (12 - 1);
	writel(reg, info->sys_base+0x64);
	
	reg = interval_millisecond * 1000;		// 转换为微秒计数
	if(reg > 0xFFFF)
		reg = 0xFFFF;
	writel(reg, info->adc_base+0x30);
}

static u32 arkn141_get_cpu_pll_freq(struct ark_adc *info)
{
	u32 val;
	u32 ref_clk;
	u32 no,nf,enable;
	u32 speed;
	
	speed = 0;
	val = readl(info->sys_base+0x14c);
	
	ref_clk = 12000000 * ((val & 0x1));
	
	val = readl(info->sys_base+0x150);
	
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = (val & 0xFF)+1;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	
	//printk(KERN_ALERT "### CPUPLL=%d\n", speed);
	return speed;
}

static u32 arkn141_get_sys_pll_freq(struct ark_adc *info)
{
	u32 val;
	u32 ref_clk;
	u32 no,nf,enable;
	u32 speed;
	
	speed = 0;
	val = readl(info->sys_base+0x14c);
	ref_clk = 12000000 * (((val>>3) & 0x1));
	
	val = readl(info->sys_base+0x154);
	
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = (val & 0xFF)+1;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	
	//printk(KERN_ALERT "### SYSPLL=%d\n", speed);
	return speed;
}

static u32 arkn141_get_aud_pll_freq(struct ark_adc *info)
{
	u32 val;
	u32 ref_clk;
	u32 no,nf,enable;
	u32 speed;
	
	speed = 0;
	val = readl(info->sys_base+0x14c);
	ref_clk = 12000000 * (((val>>6)&0x1));
	
	val = readl(info->sys_base+0x158);
	no = (val >> 12) & 0x03;
	if(no == 0)
		no = 1;
	else if(no == 1)
		no = 2;
	else if(no == 2)
		no = 4;
	else
		no = 8;
	
	nf = val & 0xFF;
	
	enable = (val >> 14) & 0x1;

	if(enable)
		speed = ref_clk * nf /no;
	
	//printk(KERN_ALERT "### AUDPLL=%d\n", speed);
	return speed;
}

static u32 arkn141_get_ext24_excrypt_freq(struct ark_adc *info)
{
	//printk(KERN_ALERT "### 24M\n");
	return 24000000;
}

static u32 arkn141_get_clk(struct ark_adc *info)
{
	u32 val;
	u32 main_hclk_source;
	u32 hclk_div;
	u32 main_hclk;
	u32 speed;
	
	val = readl(info->sys_base+0x40);
	
	// main_hclk_clk_sel
	// SYS_CLK_SEL [17-15]
	// 3b000 : cpupll_clk
	// 3b001 : syspll_clk
	// 3b010 : audpll_clk
	// 3b100 : clk_24m
	main_hclk_source = (val >> 15) & 0x07;
	
	// hclk_div
	// SYS_CLK_SEL [21-19]
	// hclk = main_hclk / (hclk_div + 1)
	hclk_div = (val >> 19) & 0x07;
	hclk_div = hclk_div + 1;
	
	switch(main_hclk_source)
	{
		case 0:
			main_hclk = arkn141_get_cpu_pll_freq(info);
			break;
		case 1:
			main_hclk = arkn141_get_sys_pll_freq(info);
			break;
		case 2:
			main_hclk = arkn141_get_aud_pll_freq(info);
			break;
		case 4:
			main_hclk = arkn141_get_ext24_excrypt_freq(info);
			break;
		default :
			main_hclk = 0;
			break;
	}
	speed = main_hclk / hclk_div;
	
	return speed;
}

static u32 arkn141_adc_get_apb_clk(struct ark_adc *info)
{
	u32 val;
	u32 hclk;
	u32 pclk_div;
	u32 pclk;
	
	val = readl(info->sys_base+0x40);
	// pclk_div
	// SYS_CLK_SEL [24-23]
	// 2'b00: pclk = hclk
	// 2'b01: pclk = hclk/2
	// 2'b10: pclk = hclk/4

	pclk_div = (val >> 23) & 0x3;
	hclk = arkn141_get_clk(info);
	pclk = hclk / ( 1<< pclk_div);
	
	return pclk;
}

static void arkn141_adc_set_debounce_time(struct ark_adc *info, u32 debounce_time_us)
{
	u32 dbcount;

	dbcount =  (debounce_time_us * arkn141_adc_get_apb_clk(info) / 1000000) ;

	if(dbcount >= 0xFFFFF)
		dbcount = 0xFFFFF;
	
	writel(dbcount, info->adc_base+0x2c);
}

static void arkn141_adc_soft_reset(struct ark_adc *info)
{
	unsigned int val;

	val = readl(info->sys_base+0x74);
	val &= ~(1 << 18);
	writel(val, info->sys_base+0x74);
	msleep (1);
	val = readl(info->sys_base+0x74);
	val |= (1 << 18);
	writel(val, info->sys_base+0x74);
}

static void arkn141_adc_set_clk(struct ark_adc *info, u32 enable)
{
	u32 val;
	
	if(enable)
	{
		//enable pclk
		val = readl(info->sys_base+0x48);
		val |= (1 << 3);
		writel(val, info->sys_base+0x48);
		
		//enable clk
		val = readl(info->sys_base+0x50);
		val |= (1 << 16);
		writel(val, info->sys_base+0x50);
	}
	else
	{
		//disable pclk
		val = readl(info->sys_base+0x48);
		val &= ~(1 << 3);
		writel(val, info->sys_base+0x48);
		
		//disable clk
		val = readl(info->sys_base+0x50);
		val &= ~(1 << 16);
		writel(val, info->sys_base+0x50);
	}
}

static void arkn141_enable_adc_channel(struct ark_adc *info, u32 channel)
{
	u32 value;
	
	switch(channel)
	{
		case AUX0_CHANNEL:		//battery voltage detect
		{
			//clear interrupt state
			value = readl(info->adc_base+0x0c);
			value &= ~((1<<2)|(1<<1)|(1<<0));
			writel(value, info->adc_base+0x0c);

			//mask start/stop interrupt
			value = readl(info->adc_base+0x08);
			value &= ~((1<<2)|(1<<1)|(1<<0));
			//value |= ((1<<2)|(1<<1)|(1<<0));	//mask start/stop interrupt
			writel(value, info->adc_base+0x08);

			//enable channel
			value = readl(info->adc_base+0x0);
			value |= (1<<AUX0_CHANNEL);
			writel(value, info->adc_base+0x0);
			
			break;
		}
		case AUX1_CHANNEL:		// key 
		{
			//enable channel
			value = readl(info->adc_base+0x0);
			value |= (1<<AUX1_CHANNEL);
			writel(value, info->adc_base+0x0);

			//mask start/stop interrupt
			value = readl(info->adc_base+0x08);
			value &= ~((1<<5)|(1<<4)|(1<<3));
			//value &= ~((1<<5)|(1<<4));	//not use start
			writel(value, info->adc_base+0x08);
			break;
		}
		default:
			break;
	}
}

static void arkn141_adc_init_hw(struct ark_adc *info)
{
	unsigned int val;
		
	arkn141_adc_set_clk(info, 0);
	msleep(1);
	arkn141_adc_soft_reset(info);
	msleep(1);
	arkn141_adc_set_clk(info, 1);
	msleep(1);

	//set referance voltage
	val = readl(info->sys_base+0x140);
	val &= ~(1 << 22);
	val |= (1 << 21);
	writel(val, info->sys_base+0x140);
	
	//reset adc module
	val = readl(info->adc_base+0x0);
	val |= (1 << 0);
	writel(val, info->adc_base+0x0);
	msleep(1);
	//val = readl(info->sys_base+0x0);
	val = 0;
	writel(val, info->adc_base+0x0);

	//KEY set debounce time 100us
	arkn141_adc_set_debounce_time (info, 100);

	//set adc transform interval
	arn141_adc_set_transform_interval(info, 16);

	//enable adc interrupt
	val = (0x07 << 0) | (0x07 << 3);	//AUX0(BAT) | AUX1(KEY)
	writel(val, info->adc_base+0x08);

	//adc control register
	val = 0;
	writel(val, info->adc_base+0x0);

	//clear all intertupt
	val = 0;	//bit clear
	writel(val, info->adc_base+0x0c);

	// sch  default is high , key press is low
	val = readl(info->sys_base+0x144);
#ifdef ARK_ADC_USE_PULL_UP_REGISTER
	val &= ~(1 << 14);		//   0: connect a pull-up resister  (3.0v - 0v have interrupt)
#else
	val |= (1 << 14);		//   1: connect a pull-down resister  (0.3v - 3.3v have interrupt)  -- based on hard ware
#endif
	writel(val, info->sys_base+0x144);

	//set detect level
	val = readl(info->adc_base+0x0);
	val |= (1<<8);	// 1: aux0_det high valid.
	writel(val, info->adc_base+0x0);
	val = readl(info->adc_base+0x0);
	val |= (1<<9);	// 1: aux1_det high valid.
	writel(val, info->adc_base+0x0);

	//arkn141_enable_adc_channel(info, AUX1_CHANNEL);	// 按键
	arkn141_enable_adc_channel(info, AUX0_CHANNEL);	// 电压检测
}

static void arkn141_adc_exit_hw(struct ark_adc *info)
{
	//printk(KERN_ALERT "### exit\n");
	
}

static void arkn141_adc_start_conv(struct ark_adc *info, unsigned long addr)
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

static void ark1668_adc_init_hw(struct ark_adc *info)
{
	//ADC pclk disable
	
}

static void ark1668_adc_exit_hw(struct ark_adc *info)
{

}

static void ark1668_adc_start_conv(struct ark_adc *info, unsigned long addr)
{

}


static const struct ark_adc_data arkn141_adc_data = {
	.num_channels	= ARKN141_ADC_CHANNEL_MAX_NUMS,
	.mask			= ARK_ADC_DATX_MASK_12BIT,
	.init_hw		= arkn141_adc_init_hw,
	.exit_hw		= arkn141_adc_exit_hw,
	.start_conv		= arkn141_adc_start_conv,
};

static const struct ark_adc_data ark1668_adc_data = {
	.num_channels	= ARK1668_ADC_CHANNEL_MAX_NUMS,
	.mask			= ARK_ADC_DATX_MASK_12BIT,
	.init_hw		= ark1668_adc_init_hw,
	.exit_hw		= ark1668_adc_exit_hw,
	.start_conv		= ark1668_adc_start_conv,
};

static const struct of_device_id ark_adc_match[] = {
	{
		.compatible = "arkmicro,ark1668-adc",
		.data = &ark1668_adc_data,
	}, 
	{
		.compatible = "arkmicro,arkn141-adc",
		.data = &arkn141_adc_data,
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
	u32 value,status;
#if 1
	status = readl(info->adc_base+0x0c);

	//AUX0(BAT)
	if(status & AUX0_START_INT)
	{
		value = readl(info->adc_base+0x0c);
		value &= ~AUX0_START_INT;
		writel(value, info->adc_base+0x0c);
	}
	if(status & AUX0_VALUE_INT)
	{
		u32 range,val;
		
		value = readl(info->adc_base+0x14);

		val = readl(info->adc_base+0x0c);
		val &= ~AUX0_VALUE_INT;
		writel(val, info->adc_base+0x0c);

		//range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024);
		range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4095 : 1023);
		info->value1[0] = value * ARK_BATTERY_VOLTAGE_REF / range;
		//printk(KERN_ALERT "AUX0### value:%d,info->value:%d\n",value,info->value1[0]);
	}
	if(status & AUX0_STOP_INT)
	{
		value = readl(info->adc_base+0x0c);
		value &= ~AUX0_STOP_INT;
		writel(value, info->adc_base+0x0c);
	}

	//AUX1(KEY)
	if(status & AUX1_START_INT)
	{
		value = readl(info->adc_base+0x0c);
		value &= ~AUX1_START_INT;
		writel(value, info->adc_base+0x0c);
		// 关闭AUX0. ADC是互斥资源, 只允许一个AUX使用.
		// 当AUX1有需要转换的数据时(按键电压值转换), 将AUX0关闭, 分配ADC资源给AUX1使用.
		// AUX1转换完毕后(STOP_INT), 重新使能AUX0
		//close AUX0, because only one channel can be used once a time.
		//when AUX1 working (key value convert to voltage value), we must close AUX0, ADC distribute resource to AUX1.
		//when AUX1 convert over, enable AUX0 again.
		value = readl(info->adc_base+0x0);
		value &= ~(1 << AUX0_CHANNEL);
		writel(value, info->adc_base+0x0);
	}
	if(status & AUX1_VALUE_INT)
	{
		u32 aux1_range,aux1_val;

		value =readl(info->adc_base+0x18);;	// 读取ADC采样值
		
		aux1_val = readl(info->adc_base+0x0c);
		aux1_val &= ~AUX1_VALUE_INT;
		writel(aux1_val, info->adc_base+0x0c);

		aux1_range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4095 : 1023);
		info->value1[1] = value * ARK_BATTERY_VOLTAGE_REF / aux1_range;
		//printk(KERN_ALERT "AUX1### value:%d,info->value:%d\n",value,info->value1[1]);

	}
	
	if((status & AUX1_STOP_INT))
	{
		// 按键释放
		value = readl(info->adc_base+0x0c);
		value &= ~AUX1_STOP_INT;
		writel(value, info->adc_base+0x0c);
		
		// 使用完毕后, 重新允许AUX0. AUX0可以继续采样电压值.
		value = readl(info->adc_base+0x0);
		value |= (1 << AUX0_CHANNEL);
		writel(value, info->adc_base+0x0);
	}

#else
	//解决按键检测时影响电压检测值问题   ，针对成都N141 钓鱼板项目
	int aux0_start = 0;
	int aux1_start = 0;
	static int key_count = 0;
	status = readl(info->adc_base+0x0c);
	//printk(KERN_ALERT ">>>>>>>>>>>>>>>>>>>>status:%d<<<<<<<<<<<<<<<<<<<<<<<\n",status);
	//AUX0(BAT)
	if((status & AUX0_START_INT))
	{//printk(KERN_ALERT "AUX0_START_INT++++++++++++++++++++++++++++++++++++++\n");
		value = readl(info->adc_base+0x0c);
		value &= ~AUX0_START_INT;
		writel(value, info->adc_base+0x0c);
	}
	if(status & AUX0_VALUE_INT)
	{
		u32 range,val;
		value = readl(info->adc_base+0x14);

		val = readl(info->adc_base+0x0c);
		val &= ~AUX0_VALUE_INT;
		writel(val, info->adc_base+0x0c);

		//range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4096 : 1024);
		range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4095 : 1023);
		info->value1[0] = value * ARK_BATTERY_VOLTAGE_REF / range;
		printk(KERN_ALERT "AUX0### value:%d,info->value:%d\n",value,info->value1[0]);
		aux0_start = 1;
	}

	if(status & AUX0_STOP_INT)
	{//printk(KERN_ALERT "AUX0_STOP_INT----------------------------------------------\n");
		value = readl(info->adc_base+0x0c);
		value &= ~AUX0_STOP_INT;
		writel(value, info->adc_base+0x0c);
	}

	//AUX1(KEY)
	if((status & AUX1_START_INT))
	{//printk(KERN_ALERT "AUX1_START_INT++++++++++++++++++++++++++++++++++++++\n");
		value = readl(info->adc_base+0x0c);
		value &= ~AUX1_START_INT;
		writel(value, info->adc_base+0x0c);
		// 关闭AUX0. ADC是互斥资源, 只允许一个AUX使用.
		// 当AUX1有需要转换的数据时(按键电压值转换), 将AUX0关闭, 分配ADC资源给AUX1使用.
		// AUX1转换完毕后(STOP_INT), 重新使能AUX0
		//close AUX0, because only one channel can be used once a time.
		//when AUX1 working (key value convert to voltage value), we must close AUX0, ADC distribute resource to AUX1.
		//when AUX1 convert over, enable AUX0 again.
		value = readl(info->adc_base+0x0);
		value &= ~(1 << AUX0_CHANNEL);
		writel(value, info->adc_base+0x0);

		key_count = 1;
	}

	if(key_count >1)
	{
		value = readl(info->adc_base+0x0c);
		value &= ~AUX1_START_INT;
		writel(value, info->adc_base+0x0c);

		value = readl(info->adc_base+0x0);
		value &= ~(1 << AUX0_CHANNEL);
		writel(value, info->adc_base+0x0);

		key_count = 1;
	}

	if(status & AUX1_VALUE_INT)
	{
		u32 aux1_range,aux1_val;

		value =readl(info->adc_base+0x18);;	// 读取ADC采样值

		aux1_val = readl(info->adc_base+0x0c);
		aux1_val &= ~AUX1_VALUE_INT;
		writel(aux1_val, info->adc_base+0x0c);

		aux1_range = ((info->data->mask==ARK_ADC_DATX_MASK_12BIT) ? 4095 : 1023);
		info->value1[1] = value * ARK_BATTERY_VOLTAGE_REF / aux1_range;
		printk(KERN_ALERT "AUX1### value:%d,info->value:%d\n",value,info->value1[1]);

		//aux1_start = 1;
		key_count ++;

	}

	if((status & AUX1_STOP_INT))
	{//printk(KERN_ALERT "AUX1_STOP_INT----------------------------------------------\n");
		// 按键释放
		value = readl(info->adc_base+0x0c);
		value &= ~AUX1_STOP_INT;
		writel(value, info->adc_base+0x0c);

		// 使用完毕后, 重新允许AUX0. AUX0可以继续采样电压值.
		value = readl(info->adc_base+0x0);
		value |= (1 << AUX0_CHANNEL);
		writel(value, info->adc_base+0x0);

		key_count = 0;
	}

	if(key_count >1)
	{
		// 按键释放
		value = readl(info->adc_base+0x0c);
		value &= ~AUX1_STOP_INT;
		writel(value, info->adc_base+0x0c);

		// 使用完毕后, 重新允许AUX0. AUX0可以继续采样电压值.
		value = readl(info->adc_base+0x0);
		value |= (1 << AUX0_CHANNEL);
		writel(value, info->adc_base+0x0);
	}
#endif
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

	printk("ark_adc_probe success\n");
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
		.name	= "ark-adc",
		.owner	= THIS_MODULE,
		.of_match_table	= ark_adc_match,
	},
};


module_platform_driver(ark_adc_driver);


MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Arkmicro ADC Driver");
