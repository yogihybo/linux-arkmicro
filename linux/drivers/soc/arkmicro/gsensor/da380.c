#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/workqueue.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>


struct gsensor_data {
	short x;
	short y;
	short z;
	short delta_x;
	short delta_y;
	short delta_z;
};

enum _GSENSOR_SENSITIVITY_LEVEL
{
	GSENSOR_OFF = 0,
	GSENSOR_LOW,
	GSENSOR_MED,
	GSENSOR_HIGH,
	GSENSOR_ID_MAX
};

enum {
	GSENSOR_I2C_GPIO_OUTPUT,
	GSENSOR_I2C_GPIO_INPUT
};

enum int_latch{
	NONE_LATCH = 0,
	LATCH_250MS,
	LATCH_500MS,
	LATCH_1S,
	LATCH_2S,
	LATCH_4S,
	LATCH_8S,
	LATCH_1MS = 10,
	LATCH_2MS,
	LATCH_25MS,
	LATCH_50MS,
	LATCH_100MS,
	LATCHED
};

//register
#define NSA_REG_SPI_I2C                 0x00
#define NSA_REG_WHO_AM_I                0x01
#define NSA_REG_ACC_X_LSB               0x02
#define NSA_REG_ACC_X_MSB               0x03
#define NSA_REG_ACC_Y_LSB               0x04
#define NSA_REG_ACC_Y_MSB               0x05
#define NSA_REG_ACC_Z_LSB               0x06
#define NSA_REG_ACC_Z_MSB               0x07
#define NSA_REG_MOTION_FLAG				0x09
#define NSA_REG_G_RANGE                 0x0f
#define NSA_REG_ODR_AXIS_DISABLE        0x10
#define NSA_REG_POWERMODE_BW            0x11
#define NSA_REG_SWAP_POLARITY           0x12
#define NSA_REG_FIFO_CTRL               0x14
#define NSA_REG_INTERRUPT_SETTINGS1     0x16
#define NSA_REG_INTERRUPT_SETTINGS2     0x17
#define NSA_REG_INTERRUPT_MAPPING1      0x19
#define NSA_REG_INTERRUPT_MAPPING2      0x1a
#define NSA_REG_INTERRUPT_MAPPING3      0x1b
#define NSA_REG_INT_PIN_CONFIG          0x20
#define NSA_REG_INT_LATCH               0x21
#define NSA_REG_ACTIVE_DURATION         0x27
#define NSA_REG_ACTIVE_THRESHOLD        0x28
#define NSA_REG_TAP_DURATION            0x2A
#define NSA_REG_TAP_THRESHOLD           0x2B
#define NSA_REG_CUSTOM_OFFSET_X         0x38
#define NSA_REG_CUSTOM_OFFSET_Y         0x39
#define NSA_REG_CUSTOM_OFFSET_Z         0x3a
#define NSA_REG_ENGINEERING_MODE        0x7f
#define NSA_REG_SENSITIVITY_TRIM_X      0x80
#define NSA_REG_SENSITIVITY_TRIM_Y      0x81
#define NSA_REG_SENSITIVITY_TRIM_Z      0x82
#define NSA_REG_COARSE_OFFSET_TRIM_X    0x83
#define NSA_REG_COARSE_OFFSET_TRIM_Y    0x84
#define NSA_REG_COARSE_OFFSET_TRIM_Z    0x85
#define NSA_REG_FINE_OFFSET_TRIM_X      0x86
#define NSA_REG_FINE_OFFSET_TRIM_Y      0x87
#define NSA_REG_FINE_OFFSET_TRIM_Z      0x88
#define NSA_REG_SENS_COMP               0x8c
#define NSA_REG_SENS_COARSE_TRIM        0xd1

// Ioctl command definition
#define GSENSOR_IOCTL_BASE						0xA0
#define GSENSOR_IOCTL_SET_G_RANGE				_IOW(GSENSOR_IOCTL_BASE, 0, int)
#define GSENSOR_IOCTL_GET_G_RANGE				_IOW(GSENSOR_IOCTL_BASE, 1, int)
#define GSENSOR_IOCTL_GET_GSENSOR_DATA			_IOW(GSENSOR_IOCTL_BASE, 2, struct gsensor_data)
#define GSENSOR_IOCTL_GET_BOOT_STATE			_IOW(GSENSOR_IOCTL_BASE, 3, int)
#define GSENSOR_IOCTL_SET_INT_LATCH				_IOW(GSENSOR_IOCTL_BASE, 4, int)
#define GSENSOR_IOCTL_SET_I2C_GPIO_DIRECTION	_IOW(GSENSOR_IOCTL_BASE, 5, int)
#define GSENSOR_IOCTL_SET_LOW_POWER_MODE		_IOW(GSENSOR_IOCTL_BASE, 6, int)


//#define	DA380_SLAVE_ADDRESS			  	0x4e
#define DA380_DEFAULT_G_RANGE					GSENSOR_HIGH
#define DA380_DEBUG(fmt,arg...)					do{	\
													if(1) printk(KERN_ALERT "###"fmt,##arg);	\
												}while(0)

struct da380_private_data {
	struct i2c_client *i2c_client;
	struct workqueue_struct *work_queue;
	struct work_struct work;
	struct delayed_work delay_work;
	spinlock_t spinlock;
	int i2c_gpio_direction;
	int gpio_sda;
	int gpio_scl;
	int gpio_irq;
	struct gsensor_data g_data;
	short threhold[4];
	volatile int is_collision;
	int tmp_collision;
	int g_range;
	//cdev
	struct cdev cdev;
	struct class *da380_class;
	struct device *da380_device;
	const char *name; 
	int major;
    int minor_start;
    int minor_num;
    int num;
	int boot_state;
	wait_queue_head_t da380_waitq;
};

static struct da380_private_data *g_da380_pdata = NULL;
static int da380_debug = 0;


static unsigned char mir3da_register_read (unsigned char addr,char *rxdata)
{
#if 1
	struct i2c_msg msgs[2];
	int retries = 0;
	char buf[2];
	int ret = -1;

	if(g_da380_pdata && rxdata)
	{
		buf[0] = (addr&0xFF);

		msgs[0].flags = !I2C_M_RD;
		msgs[0].addr  = g_da380_pdata->i2c_client->addr;
		msgs[0].len   = 1;
		msgs[0].buf   = buf;
		msgs[1].flags = I2C_M_RD;
		msgs[1].addr  = g_da380_pdata->i2c_client->addr;
		msgs[1].len   = 1;
		msgs[1].buf   = rxdata;

		while(retries < 5)
		{
			if(i2c_transfer(g_da380_pdata->i2c_client->adapter, msgs, 2) == 2)
			{
				//if(da380_debug)
					//DA380_DEBUG("%s read addr:0x%x,val:0x%x\n",__FUNCTION__, addr, buf[0]);
				ret = 0;
				break;
			}
			retries++;
		}
		if(retries >= 5)
			printk(KERN_ERR "%s timeout read reg:0x%x\n", __FUNCTION__, addr);

		if(g_da380_pdata->i2c_gpio_direction == GSENSOR_I2C_GPIO_INPUT)
		{
			if(gpio_is_valid(g_da380_pdata->gpio_scl) && gpio_is_valid(g_da380_pdata->gpio_sda))
			{
				gpio_direction_input(g_da380_pdata->gpio_scl);
				gpio_direction_input(g_da380_pdata->gpio_sda);
			}
		}
	}
	return ret;
#else
	*rxdata = i2c_smbus_read_byte_data(g_da380_pdata->i2c_client, addr);

	return 0;
#endif
}

static int mir3da_register_write (unsigned char addr, unsigned char data)
{
#if 1
	struct i2c_msg msg;
	u8 retries = 0;
	u8 buf[2];
	int ret = -1;

	if(g_da380_pdata)
	{
		buf[0] = (addr&0xFF);
		buf[1] = (data&0xFF);

		msg.flags = !I2C_M_RD;
		msg.addr  = g_da380_pdata->i2c_client->addr;
		msg.len   = sizeof(buf);
		msg.buf   = buf;

		while(retries < 5)
		{
			if(i2c_transfer(g_da380_pdata->i2c_client->adapter, &msg, 1) == 1)
			{
				//if(da380_debug)
					//DA380_DEBUG("%s write addr:0x%x,val:0x%x\n",__FUNCTION__, addr, data);
				ret = 0;
				break;
			}
			retries++;
		}
		if(retries >= 5)
			printk(KERN_ERR "%s timeout\n", __FUNCTION__);

		if(g_da380_pdata->i2c_gpio_direction == GSENSOR_I2C_GPIO_INPUT)
		{
			if(gpio_is_valid(g_da380_pdata->gpio_scl) && gpio_is_valid(g_da380_pdata->gpio_sda))
			{
				gpio_direction_input(g_da380_pdata->gpio_scl);
				gpio_direction_input(g_da380_pdata->gpio_sda);
			}
		}
	}
	return ret;
#else
	//return i2c_smbus_write_byte_data(g_da380_pdata->i2c_client, addr, data);
	return da380_i2c_write(addr, data);
#endif
}

static int mir3da_register_mask_write(unsigned char addr, unsigned char mask, unsigned char data)
{
    int  res = 0;
    unsigned char tmp_data;

    mir3da_register_read(addr, &tmp_data);

    tmp_data &= ~mask; 
    tmp_data |= (data & mask);
    res = mir3da_register_write(addr, tmp_data);

    return res;
}

/*return value: 0: is count    other: is failed*/
static int i2c_read_block_data( unsigned char base_addr, unsigned char count, unsigned char *data)
{
	int i = 0;
		
	for(i = 0; i < count;i++)
	{
		//if(mir3da_read_byte_data(base_addr+i,(data+i)))
		if(mir3da_register_read(base_addr+i,(data+i)))
		{
			return -1;		
		}
	}	
	return count;
}

static int mir3da_register_read_continuously( unsigned char addr, unsigned char count, unsigned char *data)
{
    int res = 0;
	
    res = ((count==i2c_read_block_data(addr, count, data)) ? 0 : 1);
	
    return res;
}

/*return value: 0: is ok    other: is failed*/
static int mir3da_read_data(short *x, short *y, short *z)
{
    unsigned char tmp_data[6] = {0};

    if (mir3da_register_read_continuously(NSA_REG_ACC_X_LSB, 6, tmp_data) != 0) {
        return -1;
    }
    
    *x = ((short)(tmp_data[1] << 8 | tmp_data[0]))>> 4;
    *y = ((short)(tmp_data[3] << 8 | tmp_data[2]))>> 4;
    *z = ((short)(tmp_data[5] << 8 | tmp_data[4]))>> 4;

	if(da380_debug)
    	DA380_DEBUG("oringnal x y z %d %d %d\n",*x,*y,*z); 	

    return 0;
}

static int mir3da_set_enable(char enable)
{
	int res = 0;
	
	if(enable)
	{
		//res = mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x40);	//low power mode
		res = mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x0);	//normal power mode
	}
	else
	{
		res = mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x80);	//suspend power mode
	}
	
	return res;	
}

static int mir3da_open_interrupt(int num)
{
	int res = 0;
	
	res = mir3da_register_write(NSA_REG_INTERRUPT_SETTINGS1,0x07); 
	//0x03->active_int_en_y,active_int_en_x;0x07->active_int_en_z,active_int_en_y,active_int_en_x
	res = mir3da_register_write(NSA_REG_ACTIVE_DURATION,0x03 );
	res = mir3da_register_write(NSA_REG_ACTIVE_THRESHOLD,0x2B/*0x1B*/);
			
	switch(num){
		case 0:
			res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING1,0x04 );
			break;
		case 1:
			res = mir3da_register_write(NSA_REG_INTERRUPT_MAPPING3,0x04 );
			break;
		default:
			res = -1;
			break;
	}
	return res;
}


static int mir3da_init(void)
{
	int  res = 0;

	res |=mir3da_register_mask_write(NSA_REG_SPI_I2C, 0x24, 0x24);
	mdelay(5);
#if 0
	res |=mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 0x02/*0x00*/);// 00: +/-2g. 01: +/-4g 10:+/-8g 11:+/-16g
#else
	if(g_da380_pdata->g_range == GSENSOR_LOW)
		mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 0);
	else if(g_da380_pdata->g_range == GSENSOR_MED)
		mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 1);
	else //if(g_da380_pdata->g_range == GSENSOR_HIGH)
		mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 2);
#endif
	//res |=mir3da_register_mask_write(NSA_REG_POWERMODE_BW, 0xFF, 0x5E);
	res |=mir3da_register_mask_write(NSA_REG_POWERMODE_BW, 0xFF, 0x1E);		//normal power mode
	res |=mir3da_register_mask_write(NSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x06);
	
	res |=mir3da_register_mask_write(NSA_REG_INT_PIN_CONFIG, 0x0F, 0x01);//set int_pin level
	//res |=mir3da_register_mask_write(NSA_REG_INT_LATCH, 0x8F, 0x81);//clear latch and set latch mode
	res |=mir3da_register_mask_write(NSA_REG_INT_LATCH, 0x8F, 0x80|NONE_LATCH);//clear latch and set latch mode
	//0x81->250ms,0x82->500ms,0x83->1s,0x84->2s,0x85->4s,0x86->8s,0x8f yiz
	res |=mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x83);
	res |=mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0x69);
	res |=mir3da_register_mask_write(NSA_REG_ENGINEERING_MODE, 0xFF, 0xBD);
	res |=mir3da_register_mask_write(NSA_REG_SWAP_POLARITY, 0xFF, 0x00);
	mdelay(10);
	
	return res;
}

static void da380_open_parking_interrupt(int enable)
{	
	mir3da_init();

	mir3da_open_interrupt(0);
	
	if(g_da380_pdata->g_range > GSENSOR_OFF)
	{
		mir3da_set_enable(1);
	}
	else
	{
		mir3da_set_enable(0);
	}
	mir3da_read_data(&g_da380_pdata->g_data.x,&g_da380_pdata->g_data.y,&g_da380_pdata->g_data.z);
	if(da380_debug)
		DA380_DEBUG("prev x y z %d %d %d\n",g_da380_pdata->g_data.x,g_da380_pdata->g_data.y,g_da380_pdata->g_data.z); 
}

static int da380_identify (void)
{
	unsigned char cid = 0, mid = 0;
	int loop = 5;
	
	while(loop > 0)
	{
		mir3da_register_read(NSA_REG_WHO_AM_I,&cid);
		mir3da_register_read(NSA_REG_FIFO_CTRL,&mid);
		
		if((cid == 0x13)&&(mid == 0x00))
		{
			break;
		}
		loop --;
	}
	if(loop == 0)
	{
		return -1;
	}
	else
	{
		return  0;	
	}
}

static void da380_select_pad(void)
{
#if 0
	//interrupt pin gpio59
	rSYS_PAD_CTRL05 &= ~(0xf<<21);
	//SCL pin gpio60
	rSYS_PAD_CTRL05 &= ~(0xf<<24);
	//SDA pin gpio61
	rSYS_PAD_CTRL05 &= ~(0xf<<27);
#endif
}

#if 0
static void da380_work(struct work_struct *work)
{
	struct da380_private_data *da380_pdata = container_of(work, struct da380_private_data, work);
	short x = 0, y = 0, z = 0;
	int level;

	if(!da380_pdata)
		return -ENODEV;

	spin_lock(&da380_pdata->spinlock);
	if(mir3da_read_data(&x,&y,&z))
		return;	

	level = da380_pdata->g_range;

	if(da380_debug)
	{
		DA380_DEBUG("old (%d, %d, %d), new (%d, %d, %d)\n", da380_pdata->g_data.x, da380_pdata->g_data.y, da380_pdata->g_data.z, x, y, z);
		DA380_DEBUG("dif (%d, %d, %d)\n", abs(x - da380_pdata->g_data.x), abs(y - da380_pdata->g_data.y),  abs(z - da380_pdata->g_data.z));
	}
	da380_pdata->g_data.delta_x = abs(x - da380_pdata->g_data.x);
 	da380_pdata->g_data.delta_y = abs(y - da380_pdata->g_data.y);
 	da380_pdata->g_data.delta_z = abs(z - da380_pdata->g_data.z);
	
	if((da380_pdata->g_data.delta_x > da380_pdata->threhold[level])||
		(da380_pdata->g_data.delta_y > da380_pdata->threhold[level])||
		(da380_pdata->g_data.delta_z > da380_pdata->threhold[level]))
			da380_pdata->is_collision = 1;
			
	da380_pdata->g_data.x = x;
	da380_pdata->g_data.y = y;
	da380_pdata->g_data.z = z;
	spin_unlock(&da380_pdata->spinlock);
}
#else
static void da380_work(struct work_struct *work)
{
	struct da380_private_data *da380_pdata = container_of(work, struct da380_private_data, work);
	short x = 0, y = 0, z = 0;
	int level;

	if(!da380_pdata)
		return;

	spin_lock(&da380_pdata->spinlock);
	if(mir3da_read_data(&x,&y,&z))
	{
		spin_unlock(&da380_pdata->spinlock);
		return;
	}
	cancel_delayed_work(&da380_pdata->delay_work);
	da380_pdata->tmp_collision = 0;

	level = da380_pdata->g_range;

	if(da380_debug)
	{
		DA380_DEBUG("work: old(%d, %d, %d),new(%d, %d, %d),diff(%d, %d, %d)\n",
			da380_pdata->g_data.x, da380_pdata->g_data.y, da380_pdata->g_data.z, x, y, z,
			abs(x - da380_pdata->g_data.x), abs(y - da380_pdata->g_data.y),  abs(z - da380_pdata->g_data.z));
	}
	da380_pdata->g_data.delta_x = abs(x - da380_pdata->g_data.x);
	da380_pdata->g_data.delta_y = abs(y - da380_pdata->g_data.y);
	da380_pdata->g_data.delta_z = abs(z - da380_pdata->g_data.z);

	if((da380_pdata->g_data.delta_x > da380_pdata->threhold[level])||
		(da380_pdata->g_data.delta_y > da380_pdata->threhold[level])||
		(da380_pdata->g_data.delta_z > da380_pdata->threhold[level]))
	{
		da380_pdata->is_collision = 1;
		da380_pdata->tmp_collision = 1;
		schedule_delayed_work(&da380_pdata->delay_work, msecs_to_jiffies(5000));
	}
	else
	{
		schedule_delayed_work(&da380_pdata->delay_work, msecs_to_jiffies(1000));
	}

	da380_pdata->g_data.x = x;
	da380_pdata->g_data.y = y;
	da380_pdata->g_data.z = z;

	spin_unlock(&da380_pdata->spinlock);
}
#endif

static void da380_delay_work(struct work_struct *work)
{
	struct da380_private_data *da380_pdata = container_of(work, struct da380_private_data, delay_work.work);
	short x = 0, y = 0, z = 0;
	int level;

	if(!da380_pdata)
		return;

	spin_lock(&da380_pdata->spinlock);
	if(mir3da_read_data(&x,&y,&z))
	{
		spin_unlock(&da380_pdata->spinlock);
		return;
	}

	level = da380_pdata->g_range;

	if(da380_debug)
	{
		DA380_DEBUG("delay_work: old(%d, %d, %d),new(%d, %d, %d),diff(%d, %d, %d)\n\n",
			da380_pdata->g_data.x, da380_pdata->g_data.y, da380_pdata->g_data.z, x, y, z,
			abs(x - da380_pdata->g_data.x), abs(y - da380_pdata->g_data.y),  abs(z - da380_pdata->g_data.z));
	}
	da380_pdata->g_data.delta_x = abs(x - da380_pdata->g_data.x);
	da380_pdata->g_data.delta_y = abs(y - da380_pdata->g_data.y);
	da380_pdata->g_data.delta_z = abs(z - da380_pdata->g_data.z);
	if(!da380_pdata->tmp_collision) //have not collision in da380_work.
	{
		if((da380_pdata->g_data.delta_x > da380_pdata->threhold[level])||
			(da380_pdata->g_data.delta_y > da380_pdata->threhold[level])||
			(da380_pdata->g_data.delta_z > da380_pdata->threhold[level]))
		{
			da380_pdata->is_collision = 1;
		}
	}

	da380_pdata->g_data.x = x;
	da380_pdata->g_data.y = y;
	da380_pdata->g_data.z = z;
	spin_unlock(&da380_pdata->spinlock);
}

static irqreturn_t da380_irq_handler(int irq, void *data)
{
	struct da380_private_data *da380_pdata = (struct da380_private_data *)data;
	if(da380_pdata)
		queue_work(da380_pdata->work_queue, &da380_pdata->work);
	
	return IRQ_HANDLED;
}

static int da380_open(struct inode *inode, struct file *filp)
{
    struct da380_private_data *da380_pdata = container_of(inode->i_cdev, struct da380_private_data, cdev);

	filp->private_data = da380_pdata;

    return 0;
}

static int da380_release(struct inode *inode, struct file *filp)
{
   //struct da380_private_data *da380_pdata = container_of(inode->i_cdev, struct da380_private_data, cdev);

	filp->private_data = NULL;

    return 0;
}

static long da380_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct da380_private_data *da380_pdata = (struct da380_private_data *)filp->private_data;
	int ret = 0;

	if(!da380_pdata)
		return -ENODEV;

	spin_lock(&da380_pdata->spinlock);
    switch (cmd)
    {
    	case GSENSOR_IOCTL_GET_GSENSOR_DATA:
		{
			if (copy_to_user((unsigned char *)arg, (unsigned char *)&da380_pdata->g_data, sizeof(struct gsensor_data))) {
				printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
				ret = -EINVAL;
				break;
			}
			break;
		}
	    case GSENSOR_IOCTL_SET_G_RANGE:
		{
			int level = da380_pdata->g_range;

			if(level == arg)
				break;

			switch(arg)
			{
				case GSENSOR_OFF:
					ret = mir3da_set_enable(0);
					if(!ret)
						da380_pdata->g_range = arg;
					break;
				case GSENSOR_LOW:
					ret = mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 0);
					if(!ret)
					{
						da380_pdata->g_range = arg;
						if(level == GSENSOR_OFF)
							mir3da_set_enable(1);
					}
					break;
				case GSENSOR_MED:
					ret = mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 1);
					if(!ret)
					{
						da380_pdata->g_range = arg;
						if(level == GSENSOR_OFF)
							mir3da_set_enable(1);
					}
					break;
				case GSENSOR_HIGH:
					ret = mir3da_register_mask_write(NSA_REG_G_RANGE, 0x03, 2);
					if(!ret)
					{
						da380_pdata->g_range = arg;
						if(level == GSENSOR_OFF)
							mir3da_set_enable(1);
					}
					break;
				default:
					ret =  -EINVAL;
					break;
			}

			break;
	    }
		case GSENSOR_IOCTL_GET_G_RANGE:
		{
			if (copy_to_user((unsigned char *)arg, (unsigned char *)&da380_pdata->g_range, sizeof(int))) {
				printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
				ret = -EINVAL;
				break;
			}
			break;
		}
		case GSENSOR_IOCTL_GET_BOOT_STATE:
		{
			if (copy_to_user((unsigned char *)arg, (unsigned char *)&da380_pdata->boot_state, sizeof(int))) {
				printk("%s %d: copy_to_user error\n", __FUNCTION__, __LINE__);
				ret = -EINVAL;
				break;
			}
			break;
		}
		case GSENSOR_IOCTL_SET_INT_LATCH:
		{
			switch(arg)
			{
				case NONE_LATCH:
				case LATCH_250MS:
				case LATCH_500MS:
				case LATCH_1S:
				case LATCH_2S:
				case LATCH_4S:
				case LATCH_8S:
				case LATCH_1MS:
				case LATCH_2MS:
				case LATCH_25MS:
				case LATCH_50MS:
				case LATCH_100MS:
				case LATCHED:
				{
					unsigned char value = (arg&0x0f);
					ret = mir3da_register_mask_write(NSA_REG_INT_LATCH, 0x8F, 0x80|value);
					break;
				}
				default:
					ret = -EBUSY;
					break;
			}
			break;
		}
		case GSENSOR_IOCTL_SET_I2C_GPIO_DIRECTION:
		{
			if(arg == GSENSOR_I2C_GPIO_INPUT)
			{
				mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x40);    //low power mode
				if(gpio_is_valid(da380_pdata->gpio_scl) && gpio_is_valid(da380_pdata->gpio_sda))
				{
					da380_pdata->i2c_gpio_direction = GSENSOR_I2C_GPIO_INPUT;
					gpio_direction_input(da380_pdata->gpio_scl);
					gpio_direction_input(da380_pdata->gpio_sda);
				}
			}
			else
			{
				if(gpio_is_valid(da380_pdata->gpio_scl) && gpio_is_valid(da380_pdata->gpio_sda))
				{
					da380_pdata->i2c_gpio_direction = GSENSOR_I2C_GPIO_OUTPUT;
					gpio_direction_output(da380_pdata->gpio_scl, 1);
					gpio_direction_output(da380_pdata->gpio_sda, 1);
				}
			}
			break;
		}
		case GSENSOR_IOCTL_SET_LOW_POWER_MODE:
		{
			ret = mir3da_register_mask_write(NSA_REG_POWERMODE_BW,0xC0,0x40);    //low power mode
			break;
		}
		default:
			ret = -EINVAL;
			break;
    }
	spin_unlock(&da380_pdata->spinlock);

	return ret;
}

static unsigned int da380_poll(struct file *filp, poll_table *wait)
{
    struct da380_private_data *da380_pdata = (struct da380_private_data *)filp->private_data;
	unsigned int mask = 0;
	unsigned long flags;

	if(!da380_pdata)
		return -ENODEV;

	poll_wait(filp, &da380_pdata->da380_waitq, wait);

	spin_lock_irqsave(&da380_pdata->spinlock, flags);
	if (da380_pdata->is_collision == 1)
	{
		mask |= POLLIN | POLLRDNORM;
		da380_pdata->is_collision = 0;
	}
	spin_unlock_irqrestore(&da380_pdata->spinlock, flags);

	return mask;
}

static struct file_operations da380_fops = {
    .owner          = THIS_MODULE,
    .open           = da380_open,
    .unlocked_ioctl = da380_ioctl,
    .release        = da380_release,
  	.poll			= da380_poll,
};

static ssize_t gsensor_get(struct device *dev, struct device_attribute *attr, char *buf)
{
	short x = 0, y = 0, z = 0;
	if(g_da380_pdata)
	{
		spin_lock(&g_da380_pdata->spinlock);
		if(!mir3da_read_data(&x,&y,&z))
			printk(KERN_ALERT "x:%d,y:%d,z:%d\n",x,y,z);
		spin_unlock(&g_da380_pdata->spinlock);

		printk(KERN_ALERT "gseneor threhold level:%d, [0:off] [1:low(%d)] [2:medium(%d)] [3:hight(%d)]\n",
			g_da380_pdata->g_range, g_da380_pdata->threhold[1],g_da380_pdata->threhold[2],g_da380_pdata->threhold[3]);
	}

	return 0;
}

static ssize_t gsensor_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(!strncmp(buf, "threhold", 7))
	{
		unsigned int level,val;
		sscanf(buf,"%*s%d%d",&level,&val);
		if(level < GSENSOR_ID_MAX)
		{
			if(g_da380_pdata)
			{
				spin_lock(&g_da380_pdata->spinlock);
				g_da380_pdata->threhold[level] = val;
				spin_unlock(&g_da380_pdata->spinlock);
				printk(KERN_ALERT "set gseneor threhold[%d] = val:%d\n",level,val);
			}
		}
	}

	if(!strncmp(buf, "debug", 5))
	{
		sscanf(buf,"%*s%d",&da380_debug);
		printk(KERN_ALERT "gseneor debug %d\n",da380_debug);
	}

	if(!strncmp(buf, "write", 5))
	{
		unsigned int reg = 0,val = 0;
		sscanf(buf,"%*s%x%x",&reg,&val);
		if((reg <= 0xff) && (val <= 0xff))
		{
			mir3da_register_mask_write(reg, 0xFF, (unsigned char)val);
			mir3da_register_read(reg, (char *)&val);
			printk(KERN_ALERT "gsensor write reg:0x%x, val:0x%x\n", reg, val);
		}
	}

	if(!strncmp(buf, "read", 4))
	{
		unsigned int reg = 0,val = 0;
		sscanf(buf,"%*s%x",&reg);
		if(reg <= 0xff)
		{
			mir3da_register_read(reg, (char *)&val);
			printk(KERN_ALERT "gsensor read reg:0x%x, val:0x%x\n", reg, val);
		}
	}
	
	return count;
}

static DEVICE_ATTR(gsensor, 0664, gsensor_get, gsensor_set);

static struct attribute *gsensor_sysfs_attrs[] = {
    &dev_attr_gsensor.attr,
    NULL
};

static const struct attribute_group gsensor_sysfs = {
    .attrs  = gsensor_sysfs_attrs,
};

static int da380_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct da380_private_data *da380_pdata = (struct da380_private_data *)kzalloc(sizeof(struct da380_private_data), GFP_KERNEL);
	dev_t dev;
	int ret = 0;

	if(!da380_pdata || !client)
		return -ENOMEM;

	da380_pdata->name = "da380";
	da380_pdata->major = 0;
    da380_pdata->minor_start = 0;
    da380_pdata->minor_num = 1;
    da380_pdata->num = 1;
	da380_pdata->threhold[0] = 10000; //value 10000 means turn off collision check
	//da380_pdata->threhold[1] = 1200;
	//da380_pdata->threhold[2] = 600;
	//da380_pdata->threhold[3] = 300;
	da380_pdata->threhold[1] = 700; //600 - 900
	da380_pdata->threhold[2] = 500;	//400 - 700
	da380_pdata->threhold[3] = 250;	//120 - 300
	da380_pdata->is_collision = 0;
	da380_pdata->g_range = DA380_DEFAULT_G_RANGE;
	da380_pdata->i2c_client = client;
	da380_pdata->boot_state = 0;
	da380_pdata->i2c_gpio_direction = GSENSOR_I2C_GPIO_OUTPUT;
	if(client->adapter && client->adapter->dev.of_node)
	{
		da380_pdata->gpio_scl = of_get_gpio(client->adapter->dev.of_node, 0);
		da380_pdata->gpio_sda = of_get_gpio(client->adapter->dev.of_node, 1);
		if(!gpio_is_valid(da380_pdata->gpio_scl))
			da380_pdata->gpio_scl = -1;
		if(!gpio_is_valid(da380_pdata->gpio_sda))
			da380_pdata->gpio_sda = -1;
	}
	else
	{
		da380_pdata->gpio_scl = -1;
		da380_pdata->gpio_sda = -1;
	}

	g_da380_pdata = da380_pdata;
	spin_lock_init(&da380_pdata->spinlock);

	da380_select_pad();
	
	da380_pdata->gpio_irq = of_get_gpio(client->dev.of_node, 0);
	if (!gpio_is_valid(da380_pdata->gpio_irq))
	{
		printk(KERN_ERR "%s of_get_gpio failure\n",__FUNCTION__);
		goto err_gpio;
	}
	
	ret = devm_gpio_request(&client->dev, da380_pdata->gpio_irq, "da308_gpio");
	if (ret) {
		printk(KERN_ERR "%s devm_gpio_request failure\n",__FUNCTION__);
		goto err_gpio;
	}
	gpio_direction_input(da380_pdata->gpio_irq);
	da380_pdata->boot_state = gpio_get_value(da380_pdata->gpio_irq);
	if(da380_pdata->boot_state)
		printk(KERN_ALERT "Gsensor crash boot\n"); 

	da380_pdata->work_queue = alloc_workqueue("da380-workqueue", 0, 1);
	if (da380_pdata->work_queue == NULL) {
		goto err_gpio;
	}
	INIT_WORK(&da380_pdata->work, da380_work);
	INIT_DELAYED_WORK(&da380_pdata->delay_work, da380_delay_work);

	if(da380_identify() < 0)
	{
		ret = -1;
		printk(KERN_ERR "%s da380_identify failure\n",__FUNCTION__);
		goto err_work_queue;
	}

	da380_open_parking_interrupt(1);//

	if(!client->dev.of_node)
	{
		ret = -1;
		goto err_work_queue;
	}

#if 0
	ret = devm_request_irq(&client->dev, 
			gpio_to_irq(da380_pdata->gpio_irq),
					da380_irq_handler, 
					IRQF_TRIGGER_HIGH,	//hight level trigger
					"da380_irq", 
					da380_pdata);
#else
	ret = request_irq(gpio_to_irq(da380_pdata->gpio_irq),
			da380_irq_handler,
			IRQF_TRIGGER_RISING,
			"da380_irq", 
			da380_pdata);
#endif
	if(ret)
	{
		printk(KERN_ERR "%s request irq failure\n",__FUNCTION__);
		goto err_work_queue;
	}
	
    /* register char device */
    if (!da380_pdata->major) {
        ret = alloc_chrdev_region(
                    &dev,
                    da380_pdata->minor_start,
                    da380_pdata->num,
                    da380_pdata->name
                    );
        if (!ret) {
            da380_pdata->major = MAJOR(dev);
            da380_pdata->minor_start = MINOR(dev);
        }
    } else {
        dev = MKDEV(da380_pdata->major, da380_pdata->minor_start);
        ret = register_chrdev_region(dev, da380_pdata->num, (char *)da380_pdata->name);
    }

    if (ret < 0) {
        printk(KERN_ERR "%s %d: register driver error\n", __FUNCTION__, __LINE__);
        goto err_irq;
    }

    /* associate the file operations */
    cdev_init(&da380_pdata->cdev, &da380_fops);
    da380_pdata->cdev.owner    = THIS_MODULE; //driver->owner;
    da380_pdata->cdev.ops      = &da380_fops;
    ret = cdev_add(&da380_pdata->cdev, dev, da380_pdata->num);
    if (ret) {
        printk(KERN_ERR "%s %d: cdev add error\n", __FUNCTION__, __LINE__);
        goto err_cdev_add;
    }

	da380_pdata->da380_class = class_create(THIS_MODULE, "da380_class");
    if(IS_ERR(da380_pdata->da380_class)) {
    	printk(KERN_ERR "Err: failed in creating da380 class.\n");
		da380_pdata->da380_class = NULL;
        goto err_cdev_add;
    }

	da380_pdata->da380_device = device_create(da380_pdata->da380_class, NULL, dev, NULL, "gsensor");
	if (IS_ERR(da380_pdata->da380_device)) {
    	printk(KERN_ERR "Err: failed in creating da380 device.\n");
		da380_pdata->da380_device = NULL;
        goto err_cdev_add;
	}

	init_waitqueue_head(&da380_pdata->da380_waitq);

	ret = sysfs_create_group(&client->dev.kobj, &gsensor_sysfs);
	if (ret){
		printk(KERN_ERR "error dvr sysfs_create\n");
		goto err_cdev_add;
	}

	mir3da_read_data(&da380_pdata->g_data.x, &da380_pdata->g_data.y, &da380_pdata->g_data.z);

	i2c_set_clientdata(client, da380_pdata);
	
	printk("%s success\n", __FUNCTION__);
	
	return 0;

err_cdev_add:
	if (da380_pdata->da380_class) {
		if (da380_pdata->da380_device) {
			device_destroy(da380_pdata->da380_class, dev);
		}
		class_destroy(da380_pdata->da380_class);
		cdev_del(&da380_pdata->cdev);
	}
	unregister_chrdev_region(dev, da380_pdata->num);
err_irq:
	free_irq(gpio_to_irq(da380_pdata->gpio_irq), da380_pdata);
err_work_queue:
	destroy_workqueue(da380_pdata->work_queue);
err_gpio:
	g_da380_pdata = NULL;
	kfree(da380_pdata);
	
	return ret;
}

static int da380_remove(struct i2c_client *client)
{
	struct da380_private_data *da380_pdata = i2c_get_clientdata(client);
	dev_t dev;

	if(!da380_pdata)
		return -ENODEV;
	
	sysfs_remove_group(&client->dev.kobj, &gsensor_sysfs);
	
    dev = MKDEV(da380_pdata->major, da380_pdata->minor_start);

	cdev_del(&da380_pdata->cdev);
	if (da380_pdata->da380_class) {
		if (da380_pdata->da380_device) {
			device_destroy(da380_pdata->da380_class, dev);
		}
		class_destroy(da380_pdata->da380_class);
	}
	unregister_chrdev_region(dev, da380_pdata->num);
	destroy_workqueue(da380_pdata->work_queue);
	free_irq(gpio_to_irq(da380_pdata->gpio_irq), da380_pdata);
	g_da380_pdata = NULL;
	kfree(da380_pdata);
	
	return 0;
}

static const struct of_device_id da380_of_match[] = {
	{ .compatible = "arkmicro,da380" },
	{ }
};
MODULE_DEVICE_TABLE(of, da380_of_match);

static const struct i2c_device_id da380_id[] = {
	{ "da380", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, imx322_id);


static struct i2c_driver da380_driver = {
	.driver = {
		.name = "da380",
		.of_match_table = of_match_ptr(da380_of_match),
	},
	.probe = da380_probe,
	.remove = da380_remove,
    .id_table = da380_id,
};

module_i2c_driver(da380_driver);


MODULE_DESCRIPTION("ArkMicro g_sensor da380 Driver");
MODULE_LICENSE("GPL v2");
