#include<linux/miscdevice.h>
#include<linux/fs.h>
#include<linux/file.h>
#include<linux/cdev.h>
#include<linux/slab.h>
#include<linux/module.h>
#include<linux/delay.h>
#include<linux/device.h>
#include<linux/platform_device.h>
#include<asm/uaccess.h>
#include<asm/mach-types.h>
#include<linux/regmap.h>
#include<linux/i2c.h>
#include<linux/mutex.h>
#include<linux/i2c.h>
#include<linux/random.h>
#include <linux/uaccess.h>
#include"xs508.h"

#define XS508_SLEEP
//#define XS508_TEST		//if open it, you should compile X508_IicTestDrv.c file so that supported XS508_I2C_Handshake()
//extern int XS508_I2C_Handshake(unsigned char *XS508_16B_Ukey,unsigned char *XS508_16B_ID);
extern int XS508_Handshake(unsigned char *XS508_16B_Ukey,unsigned char *XS508_16B_ID);


static struct xs508_private_data *xs508_priv_data;

#ifdef XS508_SLEEP
static bool xs508_sleep = true;
#endif

static int xs508_i2c_write(uint8_t *buf, uint8_t len)
{
	struct i2c_msg msg;
	struct i2c_client *client = xs508_priv_data->client;
	int32_t ret = -1;
	int8_t retries = 0;

	msg.flags = !I2C_M_RD;
	msg.addr  = client->addr;
	msg.len   = len;
	msg.buf   = buf;
	
	do
	{
		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret == 1)
		{
			return 0;
		}
		retries++;
	}while(retries < XS508_I2C_RETRY_COUNT);
		
	//printk(KERN_ALERT "%s timeout\n", __FUNCTION__);

	return -1;
}

static int xs508_i2c_read(unsigned char addr, unsigned char *buf, unsigned int len)
{
	struct i2c_msg msgs[2];
	struct i2c_client *client = xs508_priv_data->client;
	int8_t retries = 0;
	int32_t ret = -1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 1;
	msgs[0].buf   = &addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len;
	msgs[1].buf   = buf;
	
	do
	{
		ret = i2c_transfer(client->adapter, msgs, 2);
		if(ret == 2)
		{
			return 0;
		}
		retries++;
	}while(retries < XS508_I2C_RETRY_COUNT);
	
	//printk(KERN_ALERT "%s timeout\n", __FUNCTION__);
	
	return -1;
}


/*********************************************
 *	return value:	  0: write pass
 *	     			-1: write error
*********************************************/
int  HI2C_write25Byte( unsigned char *buffer)
{
	unsigned char buf[26];
	buf[0] = 0x07;
	memcpy(&buf[1], buffer, 25);
	
	return xs508_i2c_write(buf, 26);
}

/*********************************************
 *	return value:	  0	: write pass
 *				-1	: write error
*********************************************/
int  HI2C_read32Byte(unsigned char *buffer)
{
	return xs508_i2c_read(0, buffer, 32);
}

/*********************************************
 * Delay ms 
 *********************************************/
void XS508_delay_ms(int D_TIME)
{
	msleep(D_TIME);
}

/*********************************************
 *	description	: get Random
 *	return value	: random value
 *********************************************/
int get_xs_srand(void)
{
	int rand_num[1];
	
	get_random_bytes(rand_num, sizeof(int));

	return rand_num[0];
}

/*********************************************
 * XS508G -> stop XS508: release i2c bus, avoid have an effect on other i2c device
 * IN: null
 * OUT: 0->ok
 * -1->I2C WRITE OR READ ERROR
 * -2->other error
*********************************************/
#if 0	//if stop, xs508 will not work at next time, so, do not stop
static int xs508_stop_handshake(void)
{
	unsigned char wr_data_25_byte[25];
	unsigned char rd_data_32_byte[32] = {0};
	int i,retry;

	wr_data_25_byte[0] = 0xF2;
	
	for(i=1; i<22; i++)
		wr_data_25_byte[i] = (char)get_xs_srand();

	wr_data_25_byte[22] = 0x5A;	//stop cmd: 3 byte 0x5A
	wr_data_25_byte[23] = 0x5A;
	wr_data_25_byte[24] = 0x5A;

	retry = 0;
	while(HI2C_write25Byte(wr_data_25_byte))
	{
		XS508_delay_ms(1);
		retry ++;
		if(retry > 100)
		{
			return -1;
		}
	}

	XS508_delay_ms(10);

	retry = 0;
	while(HI2C_read32Byte(rd_data_32_byte))
	{
		XS508_delay_ms(1);
		retry++;
		if (retry > 5)
		{
			return 0;
		}
	}
	return -2;
}
#endif

#ifdef XS508_SLEEP
/*********************************************
// XS508G -> sleep	: when xs508 is sleeping,  it will not pass while handshake again, need one more time(twice).
// IN: null
// OUT: 0-> sleep
// -1->I2C WRITE OR READ ERROR
*********************************************/
static int xs508_sleep_handshake(void)
{
	unsigned char wr_data_25_byte[25];
	int i,retry;

	wr_data_25_byte[0] = 0xF2;
	
	for(i=1; i<22; i++)
		wr_data_25_byte[i] = (char)get_xs_srand();

	wr_data_25_byte[22] = 0xA5;	//sleep cmd: 3 byte 0x5A
	wr_data_25_byte[23] = 0xA5;
	wr_data_25_byte[24] = 0xA5;

	retry = 0;
	while(HI2C_write25Byte(wr_data_25_byte))
	{
		XS508_delay_ms(1);
		retry ++;
		if(retry > 100)
		{
			return -1;
		}
	}

	return 0;
}
#endif

/******************************************************************************************
 *
 *	return value :
 *		success	: 0
 *		fail	: <0
 *
 *****************************************************************************************/
 #if 0
static int xs508_handshake(void)
{
	unsigned char XS508_DAT[16] = {0};
	int i1, i2,t1_cnt;

	for(t1_cnt=0; t1_cnt<2; t1_cnt++)//retry 2 times if exception
	{
#ifdef XS508_TEST
		i1=XS508_I2C_Handshake(XS508_KEY, XS508_DAT);
		if(i1 != 0)
		{
			i1 = -1;
			continue;
		}
		XS508_delay_ms(10);
#endif
		i1=XS508_Handshake(XS508_KEY, XS508_DAT);
		if(i1 == 0)
		{
			//printk(KERN_ALERT "XS508_Handshake() Pass,ID=" );
			for(i2=0; i2<16; i2++)
			{
				//printk(KERN_ALERT "0x%2x,",XS508_DAT[i2] );
				if (XS508_DAT[i2] != XS508_id0[i2])
				{
					i1 = -5;
					break;
				}
			}
			//printk(KERN_ALERT "\n");

			if(i1 == 0)	//pass
			{
				printk(KERN_INFO "XS508 verify pass\n" );
				goto xs508_handshake_over;
			}
		}
	}

xs508_handshake_over:
	for(i2=0; i2<16; i2++)
	{
		//destroy key and id
		//XS508_KEY[i2] += get_xs_srand();
		XS508_DAT[i2] += get_xs_srand();
		//XS508_id0[i2] += get_xs_srand();
	}		

	return i1;
}
#endif


static int xs508_open(struct inode *inode, struct file *file)
{
	if(xs508_priv_data)
	{
		file->private_data = xs508_priv_data;
	    //nonseekable_open(inode, file);
	}

	return 0;
}

static int xs508_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
	
#ifdef XS508_SLEEP
	//xs508 sleep
	if(xs508_sleep_handshake() == 0)
	{
		xs508_sleep = true;
	}
#endif

    return 0;
}

static long xs508_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned char key[16] = {0};
	unsigned char id[16] = {0};
	int ret;
	
	switch(cmd)
	{
		case XS508_IOCTL_CMD_HANDSHAKE_KEY:
			if(copy_from_user(key, (unsigned char *)arg, 16))
				return -EFAULT;

			ret = XS508_Handshake(key, id);
			
#ifdef XS508_SLEEP
			//when xs508 is sleeping,  it will not pass while handshake again, need one more time(twice).
			if(ret != 0)
			{
				if(xs508_sleep == true)
				{
					XS508_Handshake(key, id);
				}
			}
			xs508_sleep = false;
#endif
			if(copy_to_user((unsigned char *)arg, &id, 16))
				return -EFAULT;
			break;

		default:
			break;
	}
		
	return 0;
}

static struct file_operations xs508_fops = {
	.owner			= THIS_MODULE,
	.open			= xs508_open,
	.release		= xs508_release,
	.unlocked_ioctl	= xs508_ioctl,
};

static struct miscdevice xs508_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = XS508_DEVICE_NAME,
	.fops =&xs508_fops,
};

static int xs508_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct xs508_private_data *pdata;
	int ret = 0;

	printk(KERN_DEBUG "%s\n", __FUNCTION__);

	pdata = kzalloc(sizeof(struct xs508_private_data), GFP_KERNEL);
	if(pdata == NULL)
	{
		printk(KERN_ERR "err %s, kzmalloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}

	pdata->client = client;
	pdata->id = (struct i2c_device_id *)id;
	mutex_init(&(pdata->lock));
	xs508_priv_data = pdata;
	
	if(misc_register(&xs508_device))
	{
		ret = -ENOMEM;
		goto fail_misc_register;
	}
	
	i2c_set_clientdata(client, pdata);
	
	return 0;

fail_misc_register:
	mutex_destroy(&pdata->lock);
	kfree(pdata);
	xs508_priv_data = NULL;
	return ret;
}

static int xs508_remove(struct i2c_client *client)
{
	struct xs508_private_data *pdata = i2c_get_clientdata(client);

	if(pdata == NULL)
	{
		printk(KERN_ERR "err %s, no device to remove\n",__FUNCTION__);
		return -ENODEV;
	}
	
	misc_deregister(&xs508_device);
	mutex_destroy(&pdata->lock);
	kfree(pdata);
   	i2c_set_clientdata(client, NULL);
	xs508_priv_data = NULL;
	
	return 0;
}

#if 0
static void xs508_shutdown(struct i2c_client *client)
{
 	struct xs508_private_data *pdata = i2c_get_clientdata(client);

	if(pdata)
	{
	
	}
}
#endif 

static const struct i2c_device_id xs508_id[] = {
	{XS508_I2C_NAME, 0},
	{}
};

static const struct of_device_id xs508_of_match[] = {
	{ .compatible = "arkmicro,dvr_xs508", },
	{ }
};

static struct i2c_driver xs805_driver = {
	.driver 	= {
		.name 	= XS508_I2C_NAME,
		.of_match_table = xs508_of_match,
	},
	.id_table	= xs508_id,
	.probe		= xs508_probe,
	.remove		= xs508_remove,
	//.shutdown	= xs508_shutdown,
};

module_i2c_driver(xs805_driver);

MODULE_AUTHOR("arkmicro");
MODULE_DESCRIPTION("I2C Encrypt IC XS805 Driver");
MODULE_LICENSE("GPL");
