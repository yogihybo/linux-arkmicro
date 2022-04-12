#if 0
/*
 *  ILI8961 spi-based driver
 *
 *  Copyright (C) 2020 Arkmicro
 *
 */
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/lcd.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/string.h>


struct ili8961 {
	struct spi_device	*spi;
	struct mutex		lock;
	int 				max_speed_hz;
};

static struct ili8961 *g_ili8961 = NULL;


static int ili8961_write(struct spi_device *spi, u8 reg, u8 value)
{
	struct spi_message msg;
	struct spi_transfer xfer;
	unsigned char tbuf[4];
	int ret = 0;

	if(!spi) {
		printk(KERN_ERR "ERR: %s, Invalid argument\n", __FUNCTION__);
		return -ENXIO;
	}
	memset(&xfer, 0, sizeof(xfer));
	xfer.tx_buf = tbuf;
	xfer.rx_buf = NULL;
	xfer.cs_change = 0;
	//CHECK_FREQ_REG(spi, &xfer);
	tbuf[0] = reg;
	tbuf[1] = value;
	xfer.bits_per_word = 8;
	xfer.len = 2;

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	ret = spi_sync(spi, &msg);
	if(ret < 0) {
		printk(KERN_ERR "ERR: %s, Error sending SPI message ret:0x%x\n", __FUNCTION__, ret);
		return ret;
	}
	
	printk(KERN_ALERT "### write reg:0x%x=0x%x, ret:%d\n", reg, value, ret);

	return 0;
}

static int ili8961_display_init(struct spi_device *spi)
{
	int ret = 0;
	int i;
	const unsigned char data[28][2] = {
		{0x05,0x1E},
		{0x05,0x5E},
		{0x2B,0x01},
		{0x0B,0x81},	//80      //0：VCOMDC Output Voltage is read from MTP memory. 
		{0x00,0x0E},	// CPT=0C 
		{0x01,0xAF},	//AF       // VCOMDC  调VCOMDC  根据panel来调整
		{0x0D,0x45},	// contrast  dafault=40
		{0x04,0x0F},	//1F 为dummy RGB， 0F为RGB input
		{0x2F,0x61},
		{0x95,0x00},	//80    // DOT  INVERSION 
		{0x16,0x00},	//00      //gamma enable 
		{0x17,0x77},	//P gamma
		{0x18,0x77},
		{0x19,0x33},
		{0x1A,0x43},
		{0x3C,0x34},	//34  
		{0x3E,0x77},	//N  gamma
		{0x3F,0x37},
		{0x80,0x23},
		{0x81,0x23},
		{0x3D,0x07},
		{0x87,0x1C},
		{0x88,0x42},
		{0x89,0x85},
		{0x8A,0xCA},
		{0xAA,0x03},
		{0xAB,0x02},
		{0x2B,0x01}
	};

#if 1
	ret = ili8961_write(spi, data[0][0], data[0][1]);
	msleep(50);
	for(i=1; i<sizeof(data)/sizeof(data[0]); i++) {
		ret = ili8961_write(spi, data[i][0], data[i][1]);
	}
#else
	ret = spi_write(spi, data[0], 2);
	msleep(50);
	for(i=1; i<sizeof(data)/sizeof(data[0]); i++) {
		ret = spi_write(spi, data[i], 2);
		printk(KERN_ALERT "### spi_write reg:0x%x=0x%x, ret:%d\n", data[i][0], data[i][1], ret);
	}
#endif
	return ret;
}

static ssize_t ili8961_set(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	if(!strncmp(buf, "write", 5))
	{
		unsigned int reg,val;
		sscanf(buf,"%*s%x%x",&reg,&val);
		if(g_ili8961) {
			ili8961_write(g_ili8961->spi, reg, val);
			printk(KERN_ALERT "write reg[0x%02x]:0x%02x\n",reg,val);
		}
	}

    return count;
}

static DEVICE_ATTR(ili8961, S_IWUSR | S_IRUGO,//static DEVICE_ATTR(dvr, S_IWUGO | S_IRUGO,
        NULL, ili8961_set);

static struct attribute *ili8961_sysfs_attrs[] = {
        &dev_attr_ili8961.attr,
        NULL
};

static const struct attribute_group ili8961_sysfs = {
        .attrs  = ili8961_sysfs_attrs,
};

static int ili8961_probe(struct spi_device *spi)
{
	struct ili8961 *ili;
	int ret = -1;

	ili = devm_kzalloc(&spi->dev, sizeof(*ili), GFP_KERNEL);
	if (!ili) {
		printk(KERN_ERR "%s devm_kzalloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}

	ret = of_property_read_u32(spi->dev.of_node, "spi-max-frequency", &ili->max_speed_hz);
	if (ret < 0) {
		dev_err(&spi->dev, "spi-max-frequency property not found\n");
		ili->max_speed_hz = 0;
	}

	ili->spi = spi;
	spi_set_drvdata(spi, ili);

	mutex_init(&ili->lock);
	g_ili8961 = ili;

	ret = ili8961_display_init(spi);
	if (ret) {
		printk(KERN_ERR "%s ili8961_display_init failed\n", __FUNCTION__);
		goto exit_destroy;
	}

	ret = sysfs_create_group(&spi->dev.kobj, &ili8961_sysfs);
	if (ret) {
		printk(KERN_ERR "***ERR: sysfs_create_group failed\n");
	}
	printk("%s success\n", __FUNCTION__);

	return 0;

exit_destroy:
	mutex_destroy(&ili->lock);
	g_ili8961 = NULL;
	printk(KERN_ERR "### %s failed\n", __FUNCTION__);

	return ret;
}

static int ili8961_remove(struct spi_device *spi)
{
	struct ili8961 *ili = spi_get_drvdata(spi);

	if(g_ili8961) {
        sysfs_remove_group(&spi->dev.kobj, &ili8961_sysfs);
		mutex_destroy(&ili->lock);
		g_ili8961 = NULL;
	}

	return 0;
}

static const struct of_device_id ili8961_dt_ids[] = {
	{ .compatible = "arkmicro,ili8961" },
	{},
};
MODULE_DEVICE_TABLE(of, ili8961_dt_ids);

static struct spi_driver ili8961_driver = {
	.driver = {
		.name		= "ili8961",
		.of_match_table	= ili8961_dt_ids,
	},
	.probe		= ili8961_probe,
	.remove		= ili8961_remove,
};

#if 0
module_spi_driver(ili8961_driver);
#else
static int __init ili8961_init(void)
{
	return spi_register_driver(&ili8961_driver);
}

static void __exit ili8961_exit(void)
{
	return spi_unregister_driver(&ili8961_driver);
}

late_initcall(ili8961_init);
module_exit(ili8961_exit);
#endif

MODULE_AUTHOR("Arkmicro");
MODULE_DESCRIPTION("lcd ili8961 driver");
MODULE_LICENSE("GPL v2");
#else
/*
 *  ILI8961 spi-based driver
 *
 *  Copyright (C) 2020 Arkmicro
 *
 */
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/lcd.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/string.h>

#define SPI_GPIO_NO_CHIPSELECT		((unsigned long)-1l)
#define SPI_GPIO_NO_MISO			((unsigned long)-1l)
#define SPI_GPIO_NO_MOSI			((unsigned long)-1l)
#define INI8961_DELAY				ndelay(100)

struct ili8961_pdata {
	struct device 		*dev;
	struct mutex		lock;
	int 				max_speed_hz;
	int					sck;
	unsigned long		mosi;
	unsigned long		miso;
	int					cs;
	int					num_chipselect;
	u16 				master_flags;
};

static struct ili8961_pdata *g_ili8961 = NULL;

static void ili8961_set_cs(u8 state)
{
	if(g_ili8961)
    	gpio_direction_output(g_ili8961->cs, state);
}

static void ili8961_set_clk(u8 state)
{
	if(g_ili8961)
		gpio_direction_output(g_ili8961->sck, state);
}

static void ili8961_set_data(u8 state)
{
	if(g_ili8961)
		gpio_direction_output(g_ili8961->mosi, state);
}

static u8  ili8961_get_data(void)
{
	if(g_ili8961) {
		gpio_direction_input(g_ili8961->miso);
		return gpio_get_value(g_ili8961->miso);
	}

	return 0;
}

static void ili8961_start(void)
{
	ili8961_set_cs(0);
	INI8961_DELAY; //min 50 NS
}

static void ili8961_write_bit(u8 state)
{
	ili8961_set_clk(0);
	ili8961_set_data(state);
	INI8961_DELAY; //min 50 NS
	ili8961_set_clk(1);
	INI8961_DELAY;
}

static u8 ili8961_read_bit(void )
{
	 u8 state;

	 ili8961_set_clk(0);
	 INI8961_DELAY; //min 50 NS	
	 ili8961_set_clk(1);
	 state = ili8961_get_data();
	 INI8961_DELAY; //min 50 NS	
	 return state;
}

static void ili8961_end(void)
{  
	ili8961_set_cs(1);
	ndelay(1000);  //min 400 NS
}

static u8 ili8961_read_reg(u8 reg)
{
	u8 value = 0;
	u8 state;
	int i;

	reg |= 0x40;  //bit 6 -> 1  /read
	ili8961_start();

	for(i=7; i>=0; i--)
	{
		if((reg>>i) & 0x01)
			ili8961_write_bit(1);
		else
			ili8961_write_bit(0);
	}

	for(i=7; i>=0; i--)
	{
		state = ili8961_read_bit();
		value |= (state<<i);
	}

	ili8961_end();

	return value;
}

static void ili8961_write_reg(u8 reg, u8 value)
{
	int i;
	u16 write_data;

	write_data = (u16)(((reg & 0xbf)<<8) | value);  //bit 6 -> 0 write

	ili8961_start();

	for(i=15; i>=0; i--)
	{
		if((write_data>>i) & 0x01)
			ili8961_write_bit(1);
		else
			ili8961_write_bit(0);
	}
	ili8961_end();

	//printk(KERN_ALERT "### write reg:0x%02x=0x%02x, read:0x%02x\n", reg, value, ili8961_read_reg(reg));
}

static int ili8961_gpio_alloc(unsigned pin, const char *label, bool is_in)
{
	int value;

	value = gpio_request(pin, label);
	if (value == 0) {
		if (is_in)
			value = gpio_direction_input(pin);
		else
			value = gpio_direction_output(pin, 0);
	}
	return value;
}

static int ili8961_gpio_request(struct ili8961_pdata *ili, const char *label, u16 *res_flags)
{
	int value;

	/* NOTE:  SPI_*_GPIO symbols may reference "pdata" */

	if (ili->mosi != SPI_GPIO_NO_MOSI) {
		value = ili8961_gpio_alloc(ili->mosi, label, false);
		if (value)
			goto done;
	} else {
		/* HW configuration without MOSI pin */
		*res_flags |= SPI_MASTER_NO_TX;
	}

	if (ili->miso != SPI_GPIO_NO_MISO) {
		if(ili->mosi != ili->miso) {
			value = ili8961_gpio_alloc(ili->miso, label, true);
			if (value)
				goto free_mosi;
		}
	} else {
		/* HW configuration without MISO pin */
		*res_flags |= SPI_MASTER_NO_RX;
	}

	value = ili8961_gpio_alloc(ili->sck, label, false);
	if (value)
		goto free_miso;

	goto done;

free_miso:
	if (ili->miso != SPI_GPIO_NO_MISO) {
		if(ili->mosi != ili->miso)
			gpio_free(ili->miso);
	}
free_mosi:
	if (ili->mosi != SPI_GPIO_NO_MOSI)
		gpio_free(ili->mosi);
done:
	return value;
}

static int ili8961_gpio_probe_dt(struct ili8961_pdata *ili)
{
	int ret;
	u32 tmp;
	struct device_node *np = ili->dev->of_node;

	ret = of_get_named_gpio(np, "gpio-sck", 0);
	if (ret < 0) {
		printk(KERN_ERR "ERR: %s, gpio-sck property not found\n", __FUNCTION__);
		return ret;
	}
	ili->sck = ret;

	ret = of_get_named_gpio(np, "gpio-miso", 0);
	if (ret < 0) {
		printk(KERN_ERR "ERR: %s, gpio-miso property not found, switching to no-rx mode\n", __FUNCTION__);
		ili->miso = SPI_GPIO_NO_MISO;
	} else
		ili->miso = ret;

	ret = of_get_named_gpio(np, "gpio-mosi", 0);
	if (ret < 0) {
		printk(KERN_ERR "ERR: %s, gpio-mosi property not found, switching to no-tx mode\n", __FUNCTION__);
		ili->mosi = SPI_GPIO_NO_MOSI;
	} else
		ili->mosi = ret;

	ret = of_get_named_gpio(np, "cs-gpios", 0);
	if (ret < 0) {
		printk(KERN_ERR "ERR: %s, cs-gpios not found\n", __FUNCTION__);
		return ret;
	} else {
		ili->cs = ret;
	}

	ret = of_property_read_u32(np, "num-chipselects", &tmp);
	if (ret < 0) {
		//printk(KERN_ERR "ERR: %s, num-chipselects property not found\n", __FUNCTION__);
		//return ret;
	}

	ili->num_chipselect = tmp;

	return 0;
}

static int ili8961_reg_init(struct ili8961_pdata *ili)
{
	int ret = 0;
	int i;
	u8 value;
	
	const u16 data[] = {
		0x055E,
		0x2B00,
		0x000a,	// CPT=0C    0a
		0x0B80,	//0：VCOMDC Output Voltage is read from MTP memory. 
		0x01A3,	//01ac // VCOMDC  调VCOMDC	根据panel来调整
		0x0D40,	// contrast  dafault=40
		0x040b,
		0x0315,
		0x0E40,	// 3A 第二组	  //R_CONT	
		0x0F4A,	// A  第二组	//R_BRIGHT	
		0x1040,	//47 第二组	//B_CONT  
		0x114A,	// A 第二组    //B_BRIGHT	
		0x2F61,
		0x1600,	//gamma enable 
		0x9580,	// DOT  INVERSION 
		0x1777,
		0x1847,
		0x1922,
		0x1A33,
		0x3CB7,	//34 
		0x3E77,
		0x3F37,
		0x8023,
		0x8123,
		0x3D07,
		0x871C,
		0x8842,
		0x8985,
		0x8ACA,
		0xAA03,
		0xAB02,
		0x0732,	//33
		0x2B01
	};

	ili8961_write_reg((data[0]>>8)&0xFF, data[0]&0xFF);
	for(i=1; i<sizeof(data)/sizeof(data[0]); i++) {
		ili8961_write_reg((data[i]>>8)&0xFF, data[i]&0xFF);

		value = ili8961_read_reg((data[i]>>8)&0xFF);
		if(value != ((data[i]&0xFF))) {
			printk(KERN_ERR "ERR: %s, write reg[0x%x]=0x%x, but read(0x%x)\n", __FUNCTION__, (data[i]>>8)&0xFF, data[i]&0xFF, value);
			ret = -1;
		}
	}

	return ret;
}

extern int arkn141_lcd_srgb_cfg_test(void);

static ssize_t ili8961_set(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
	if(!strncmp(buf, "write", 5))
	{
		unsigned int reg,val;
		sscanf(buf,"%*s%x%x",&reg,&val);
		ili8961_write_reg(reg, val);
		printk(KERN_ALERT "write reg[0x%x]:0x%x, read again:0x%x\n", reg, val, ili8961_read_reg(reg));
	}

	if(!strncmp(buf, "read", 4))
	{
		unsigned int reg;
		sscanf(buf,"%*s%x",&reg);
		printk(KERN_ALERT "read reg[0x%x]:0x%x\n", reg, ili8961_read_reg(reg));
	}

    return count;
}

static DEVICE_ATTR(ili8961, S_IWUSR | S_IRUGO,//static DEVICE_ATTR(dvr, S_IWUGO | S_IRUGO,
        NULL, ili8961_set);

static struct attribute *ili8961_sysfs_attrs[] = {
        &dev_attr_ili8961.attr,
        NULL
};

static const struct attribute_group ili8961_sysfs = {
        .attrs  = ili8961_sysfs_attrs,
};

static int ili8961_probe(struct platform_device *pdev)
{
	struct ili8961_pdata *ili;
	int ret = -1;

	ili = devm_kzalloc(&pdev->dev, sizeof(*ili), GFP_KERNEL);
	if (!ili) {
		printk(KERN_ERR "%s devm_kzalloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}
	ili->dev = &pdev->dev;

	ret = ili8961_gpio_probe_dt(ili);
	if (ret < 0) {
		return -ENXIO;
	}
	ret = ili8961_gpio_request(ili, dev_name(&pdev->dev), &ili->master_flags);
	if (ret < 0) {
		printk(KERN_ERR "%s ili8961_gpio_request failed\n", __FUNCTION__);
		return ret;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "spi-max-frequency", &ili->max_speed_hz);
	if (ret < 0) {
		dev_err(&pdev->dev, "spi-max-frequency property not found\n");
		ili->max_speed_hz = 0;
	}

    platform_set_drvdata(pdev, ili);

	mutex_init(&ili->lock);
	g_ili8961 = ili;

	ret = ili8961_reg_init(ili);
	if (ret) {
		printk(KERN_ERR "%s ili8961_display_init failed\n", __FUNCTION__);
		goto exit_destroy;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &ili8961_sysfs);
	if (ret) {
		printk(KERN_ERR "***Warring: sysfs_create_group failed\n");
	}
	printk("%s success\n", __FUNCTION__);

	return 0;

exit_destroy:
	mutex_destroy(&ili->lock);
	g_ili8961 = NULL;
	printk(KERN_ERR "### %s failed\n", __FUNCTION__);

	return ret;
}

static int ili8961_remove(struct platform_device *pdev)
{
	struct ili8961_pdata *ili = platform_get_drvdata(pdev);

	if(g_ili8961) {
        sysfs_remove_group(&pdev->dev.kobj, &ili8961_sysfs);
		mutex_destroy(&ili->lock);
		g_ili8961 = NULL;
	}

	return 0;
}

static const struct of_device_id ili8961_dt_ids[] = {
	{ .compatible = "arkmicro,ili8961" },
	{},
};
MODULE_DEVICE_TABLE(of, ili8961_dt_ids);

static struct platform_driver ili8961_driver = {
	.driver = {
		.name		= "ili8961",
		.of_match_table	= of_match_ptr(ili8961_dt_ids),
	},
	.probe		= ili8961_probe,
	.remove		= ili8961_remove,
};

module_platform_driver(ili8961_driver);


MODULE_AUTHOR("Arkmicro");
MODULE_DESCRIPTION("lcd ili8961 driver");
MODULE_LICENSE("GPL v2");
#endif
