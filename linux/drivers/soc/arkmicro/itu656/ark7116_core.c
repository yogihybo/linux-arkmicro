#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <asm/setup.h>
#include <linux/uaccess.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include "ark1668_itu656.h"
#include "ark7116.h"

/*struct ark7116_private_data{
	struct i2c_client *client;
};*/

static int ark7116_read_byte(unsigned char regaddr);
static int ark7116_write_byte(unsigned char regaddr, unsigned char regval);

struct ark7116_private_data *g_ark7116_pdata = NULL;
struct ark_private_data *g_itu656in_priv = NULL;
struct i2c_client *g_itu656in_client  = NULL;

/*****************************************************************************************************
 *	Before use RN6752 driver, we should complete some function as forllows.
 */

/* detect video signal */
static int ark7116_detect_signal(void)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;

	if(!ark7116_pdata)
		return 0;

        if(!ark7116_pdata->config_finish)
                return 0;

	client = ark7116_pdata->client;

	return ((ark7116_read_byte(0x26) & 0x6) == 0x6);
}

//Select itu656 input channel
static int ark7116_select_channel(int ch)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	u8 val;

	if(!ark7116_pdata)
		return -ENODEV;

	client = ark7116_pdata->client;
	if (client == NULL){
		printk("i2c_client null,set ark7116 channel failed.\n");
		return -1;
	}
	

	switch(ch) {
		case ARK7116_AV0:
			val = 0x0;
			break;
		case ARK7116_AV1:
			val = 0x10;
			break;
		case ARK7116_AV2:
			val = 0x30;
			break;
		default:
			return -EINVAL ;
            break;
	}    

	ark7116_write_byte(0xdc, val);
	
	return 0;
}

//confirm progressive or interlace based on in signal resolution.
static int ark7116_get_progressive(void)
{
        return 0;
}

static int ark7116_set_display_effect(int cmd, unsigned long arg)
{
	struct i2c_client *client;
	unsigned char addr;
	int error = 0;

	if(!g_ark7116_pdata || !g_ark7116_pdata->client)
		return -ENODEV;

	client = g_ark7116_pdata->client;
	addr = client->addr;
	client->addr = (0xB4>>1);

	switch(cmd)
	{
		case ARK_DVR_GET_BRIGHTNESS:
		{
			int brightness = ark7116_read_byte(0xD4);
			if(brightness < 0)
			{
				error = brightness;
				break;
			}
			if(copy_to_user((void  *)arg, &brightness, sizeof(int))){
				printk("%s: copy to carback_brightness error\n", __func__);
				error = -EFAULT;
			}
			break;
		}
		case ARK_DVR_SET_BRIGHTNESS:
		{
			int brightness;
			if(copy_from_user(&brightness, (void *)arg, sizeof(int))){
				printk("%s: copy from user frame error\n", __func__);
				error = -EFAULT;
			}
			else
			{
				ark7116_write_byte(0xD4, (brightness & 0xFF));
			}
			break;
		}
		case ARK_DVR_GET_CONTRAST:
		{
			int contrast = ark7116_read_byte(0xD3);
			if(contrast < 0)
			{
				error = contrast;
				break;
			}
			if(copy_to_user((void  *)arg, &contrast, sizeof(int))){
				printk("%s: copy to carback_contrast error\n", __func__);
				error = -EFAULT;
			}
			break;
		}
		case ARK_DVR_SET_CONTRAST:
		{
			int contrast;
			if(copy_from_user(&contrast, (void *)arg, sizeof(int))){
				printk("%s: copy from user frame error\n", __func__);
				error = -EFAULT;
			}
			else
			{
				ark7116_write_byte(0xD3, (contrast & 0xFF));
			}
			break;
		}
		case ARK_DVR_GET_SATURATION:
		{
			int saturation = ark7116_read_byte(0xD6);
			if(saturation < 0)
			{
				error = saturation;
				break;
			}
			if(copy_to_user((void  *)arg, &saturation, sizeof(int))){
				printk("%s: copy to carback_saturation error\n", __func__);
				error = -EFAULT;
			}
			break;
		}
		case ARK_DVR_SET_SATURATION:
		{
			int saturation;
			if(copy_from_user(&saturation, (void *)arg, sizeof(int))){
				printk("%s: copy from user frame error\n", __func__);
				error = -EFAULT;
			}
			else
			{
				ark7116_write_byte(0xD6, (saturation & 0xFF));
			}
			break;
		}
		case ARK_DVR_GET_HUE:
		case ARK_DVR_SET_HUE:
		case ARK_DVR_GET_SHARPNESS:
		case ARK_DVR_SET_SHARPNESS:
		default:
			error = -ENODEV;
			break;
	}

	client->addr = addr;

	return error;
}


/******************************************************************************************************/


static int ark7116_write_byte(unsigned char regaddr, unsigned char regval)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
	
	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;

	buf[0] = regaddr;
	buf[1] = regval;

	msg.flags = 0;
	//msg.addr  = 0xB2>>1;
	msg.addr  = client->addr;
	msg.len   = 2;
	msg.buf   = buf;

	while(retries < 5)
	{
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)break;
		retries++;
	}
	if((retries >= 5))
	{
		printk("ark7116_write_byte  write error\n");
	}
	
	return ret;
}

static int ark7116_read_byte(unsigned char regaddr)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg read_msgs[2];
	s32 ret = -1;
	s32 retries = 0;
	u8 regValue = 0x00;

	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;

        read_msgs[0].flags = !I2C_M_RD;
        //read_msgs[0].addr  = 0xB2>>1;
        read_msgs[0].addr  = client->addr;
        read_msgs[0].len   = 1;
        read_msgs[0].buf   = &regaddr;

        read_msgs[1].flags = I2C_M_RD;
        //read_msgs[1].addr  = 0xB2>>1;
        read_msgs[1].addr  = client->addr;
        read_msgs[1].len   = 1;
        read_msgs[1].buf   = &regValue;//low byte

        while(retries < 5)
        {
                ret = i2c_transfer(client->adapter, read_msgs, 2);
                if(ret == 2)break;
                retries++;
        }
        //printk("regValue=%d.\n", regValue);
        if (regValue == 0xFF) regValue = 0;
        if((retries >= 5))
        {
        	printk("ark7116_read_byte_data write error\n");
        }
        return regValue;
}

static const struct of_device_id ark7116_of_match[] = {
	{ .compatible = "arkmicro,arkn141_ark7116"},
	{ .compatible = "arkmicro,ark1668_ark7116"},
	{ }
};
MODULE_DEVICE_TABLE(of, ark7116_of_match);

static int ark7116_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ark7116_private_data *ark7116_pdata = NULL;
	const struct of_device_id *match = NULL;
	u32 value;
	int ret;

	ark7116_pdata = kzalloc(sizeof(struct ark7116_private_data), GFP_KERNEL);
	if (!ark7116_pdata)
	{
		printk("ERROR: %s kzalloc failure\n", __FUNCTION__);
		return -ENOMEM;
	}

	ark7116_pdata->gpio_reset = of_get_named_gpio(client->dev.of_node, "reset-gpio", 0);
	if (gpio_is_valid(ark7116_pdata->gpio_reset)) {
		ret = devm_gpio_request_one(&client->dev, ark7116_pdata->gpio_reset, GPIOF_OUT_INIT_LOW, "ark7116_reset");
		if (ret) {
			printk(KERN_ALERT "ERR: Failed to request ark7116 reset gpio:%d\n", ark7116_pdata->gpio_reset);
			goto err_ark_itu656_probe;
		}
	} else {
		printk(KERN_ALERT "ERR: Failed to get ark7116 reset gpio\n");
		goto err_ark_itu656_probe;
	}

	ark7116_pdata->client = client;
	ark7116_pdata->config_finish = 0;
	g_ark7116_pdata   = ark7116_pdata;
	g_itu656in_client = client;

	g_itu656in_priv = kzalloc(sizeof(struct ark7116_private_data), GFP_KERNEL);
	if (!g_itu656in_priv)
	{
		printk("ERROR: %s kzallocg_itu656in_priv  failure\n", __FUNCTION__);
		goto err_ark_itu656_probe;
	}

	match = of_match_device(&ark7116_of_match[0], &client->dev);
	if (match) {
		//match arkn141
	}
	match = of_match_device(&ark7116_of_match[1], &client->dev);
	if (match) {
		//match ark1668
	}

	memset(g_itu656in_priv, 0, sizeof(struct ark_private_data));
	g_itu656in_priv->detect_signal = ark7116_detect_signal;
	g_itu656in_priv->get_progressive = ark7116_get_progressive;
	g_itu656in_priv->select_channel = ark7116_select_channel;
	g_itu656in_priv->display_effect = ark7116_set_display_effect;
	g_itu656in_priv->support_max_resolution = TYPE_CVBS;
	g_itu656in_priv->ic_type = IC_TYPE_ARK7116;
	g_itu656in_priv->channel= ARK7116_AV0;
	g_itu656in_priv->dvr_config = NULL;
	g_itu656in_priv->init = 0;

	if(!of_property_read_u32(client->dev.of_node, "carback-config", &value)) {
		if(value == 1){
			g_itu656in_priv->dvr_config = ark7116_config;
			printk("dvr_config = ark7116_config. \n");
		}
	}

	if(!of_property_read_u32(client->dev.of_node, "default-channel", &value)) {
		g_itu656in_priv->channel = value;
	}

	if(!g_itu656in_priv->dvr_config){
		if(ark7116_config() == 0){
			g_itu656in_priv->init = 1;
	}
	printk(KERN_ALERT "### read reg 0x0x26:0x%x\n", ark7116_read_byte(0x26));

	switch(g_itu656in_priv->channel) {
		case ARK7116_AV0:
			ark7116_select_channel(ARK7116_AV0);
			break;
		case ARK7116_AV1:
			ark7116_select_channel(ARK7116_AV1);
			break;
		case ARK7116_AV2:
			ark7116_select_channel(ARK7116_AV2);
			break;
		default:
			break;
		}
	}

	i2c_set_clientdata(client, ark7116_pdata);

	printk("%s:init done\n", __func__);

	return 0;

err_ark_itu656_probe:
	kfree(ark7116_pdata);
	printk(KERN_ERR "### ERR: %s failed\n", __func__);
	return -1;
}

static int ark7116_remove(struct i2c_client *client)
{
	struct ark7116_private_data *ark7116_pdata = i2c_get_clientdata(client);

	if(ark7116_pdata)
		kfree(ark7116_pdata);
	i2c_set_clientdata(client, NULL);
	g_ark7116_pdata = NULL;

	return 0;
}


static const struct i2c_device_id ark7116_id[] = {
	{"dvr_ark7116", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ark7116_id);


static struct i2c_driver ark7116_driver = {
	.driver = {
		.name = "dvr_ark7116",
		.of_match_table = of_match_ptr(ark7116_of_match),
	},
	.probe = ark7116_probe,
	.remove = ark7116_remove,
	.id_table = ark7116_id,
};


static int __init ark7116_init(void)
{
	return i2c_add_driver(&ark7116_driver);
}

static void __exit ark7116_exit(void)
{
	i2c_del_driver(&ark7116_driver);
}

//subsys_initcall(ark7116_init);
module_init(ark7116_init);
//subsys_exitcall(ark7116_exit);
MODULE_AUTHOR("arkmicro.com");
MODULE_DESCRIPTION("ark7116 driver");
MODULE_LICENSE("GPL v2");



