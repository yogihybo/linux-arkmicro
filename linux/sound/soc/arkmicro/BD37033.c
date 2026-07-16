#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
//#include <mach/va_map.h>
//#include <mach/irqs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/pwm.h>
#include <linux/err.h>
//#include <mach/hardware.h>
//#include <linux/ark/sddac.h>
#include <linux/init.h>  
#include <linux/moduleparam.h> 
#include <linux/string.h>
#include "BD37033.h"



static struct bd37033_data *my_bd;
static SELECT_DARA select_data;
static int mute_status = MUTE_OFF;
static struct bd37033_module_param mp = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};


static int bd37033_set_stream(unsigned char input_ch);
static int bd37033_set_volume(unsigned char volume);
static int bd37033_set_fader(unsigned char reg_addr, unsigned char value);
static int set_tone_gain(unsigned char reg_addr, unsigned char value);
static int bd37033_set_mute(struct bd37033_data *bd, int state);
static void bd37033_init(struct bd37033_data *bd);

module_param_named(mute_state, mp.mute, int, 0644); 
module_param_named(input_channel, mp.input_channel, int, 0644); 
module_param_named(volume_gain, mp.volume, int, 0644); 
module_param_named(input_gain, mp.input_gain, int, 0644); 
module_param_named(fl_gain, mp.fl_gain, int, 0644); 
module_param_named(fr_gain, mp.fr_gain, int, 0644); 
module_param_named(rl_gain, mp.rl_gain, int, 0644); 
module_param_named(rr_gain, mp.rr_gain, int, 0644);
module_param_named(bass_gain, mp.bass_gain, int, 0644);
module_param_named(middle_gain, mp.middle_gain, int, 0644);
module_param_named(treble_gain, mp.treble_gain, int, 0644);

static int bd37033_write_bytes(struct i2c_client *client,  
                                   unsigned char subaddr, 
                                   unsigned char regaddr, 
                                   unsigned char* regval,
                                   int len)
{
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[255] = {0};

#ifdef CONFIG_SND_SOC_BD37033_NOP
	return 0;
#endif
	buf[0] = regaddr;
	memcpy(&buf[1],regval,len);
	//printk(KERN_DEBUG "regaddr:0x%x regval: 0x%x.\n",regaddr,regval);

	msg.flags = 0;
	msg.addr  = subaddr;
	msg.len   = len+1;
	msg.buf   = buf;

	while(retries < 5)
	{
		ret = i2c_transfer(client->adapter, &msg, 1);
		//printk(KERN_ERR "%s ret = %d\n",__FUNCTION__,ret);
		if(ret == 1)
			break;
		retries++;
	}
	if((retries >= 5))
	{
		dev_err(&client->dev, "%s: i2c_transfer failed\n", __func__);
	}

	return ret;
}

static int bd37033_write_byte(struct i2c_client *client,  
                                   unsigned char subaddr, 
                                   unsigned char regaddr, 
                                   unsigned char regval)
{
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};

#ifdef CONFIG_SND_SOC_BD37033_NOP
	return 0;
#endif
	buf[0] = regaddr;
	buf[1] = regval;
	//printk(KERN_DEBUG "regaddr:0x%x regval: 0x%x.\n",regaddr,regval);

	msg.flags = 0;
	msg.addr  = subaddr;
	msg.len   = 2;
	msg.buf   = buf;

	while(retries < 5)
	{
		ret = i2c_transfer(client->adapter, &msg, 1);
		//printk(KERN_ERR "%s ret = %d\n",__FUNCTION__,ret);
		if(ret == 1)
			break;
		retries++;
	}
	
	if((retries >= 5))
	{
		dev_err(&client->dev, "%s: i2c_transfer timeout\n", __func__);
	}
	
	return (ret>0 ? 0 : -1);
}

static unsigned char *get_data_addr(unsigned char regaddr)
{
	unsigned char *addr = NULL;
	
	switch(regaddr)
	{
		case SELECT_ADDR_INITIAL_SETUP:
			addr =  &select_data.select_data_initial_setup;
			break;
		case SELECT_ADDR_LPF_SETUP:
			addr =  &select_data.select_data_lpf_setup;
			break;
		case SELECT_ADDR_MIXING_SETUP:
			addr =  &select_data.select_data_mixing_setup;
			break;
		case SELECT_ADDR_INPUT_SELECTOR:
			addr =  &select_data.select_data_input_selector;
			break;
		case SELECT_ADDR_INPUT_GAIN:
			addr =  &select_data.select_data_input_gain;
			break;
		case SELECT_ADDR_VOLUME_GAIN:
			addr =  &select_data.select_data_volume_gain;
			break;
		case SELECT_ADDR_FADER_1CH_FRONT:
			addr =  &select_data.select_data_fader_1ch_front_gain;
			break;
		case SELECT_ADDR_FADER_2CH_FRONT:
			addr =  &select_data.select_data_fader_2ch_front_gain;
			break;
		case SELECT_ADDR_FADER_1CH_REAR:
			addr =  &select_data.select_data_fader_1ch_rear_gain;
			break;
		case SELECT_ADDR_FADER_2CH_REAR:
			addr =  &select_data.select_data_fader_2ch_rear_gain;
			break;
		case SELECT_ADDR_FADER_1CH_SUB:
			addr =  &select_data.select_data_fader_1ch_sub_gain;
			break;
		case SELECT_ADDR_MIXINT_2CH_SUB:
			addr =  &select_data.select_data_mixing_2ch_sub_gain;
			break;
		case SELECT_ADDR_BASS_SETUP:
			addr =  &select_data.select_data_bass_setup;
			break;
		case SELECT_ADDR_MIDDLE_SETUP:
			addr =  &select_data.select_data_middle_setup;
			break;
		case SELECT_ADDR_TREBLE_SETUP:
			addr =  &select_data.select_data_treble_setup;
			break;
		case SELECT_ADDR_BASS_GAIN:
			addr =  &select_data.select_data_bass_gain;
			break;
		case SELECT_ADDR_MIDDLE_GAIN:
			addr =  &select_data.select_data_middle_gain;
			break;
		case SELECT_ADDR_TREBLE_GAIN:
			addr =  &select_data.select_data_treble_gain;
			break;
		case SELECT_ADDR_LOUDNESS_GAIN:
			addr =  &select_data.select_data_loudness_gain;
			break;
		case SELECT_ADDR_SYSTEM_RESET:
			addr =  &select_data.select_data_system_reset;
			break;
		default:
			break;
	}

	return addr;
}

#if 0
static int register_write(unsigned char regaddr, unsigned char regval)
{
	unsigned char *data_addr;
	struct i2c_client *client = NULL;
	int ret = -1;

	if(!my_bd)
		return ret;
	
	client = my_bd->client;
	
	ret = bd37033_write_byte(client,client->addr,regaddr,regval);
	if(!ret)
	{
		data_addr = get_data_addr(regaddr);
		if(data_addr)
		{
			*data_addr = regval;
		}
	}
	return ret;
}

static int register_read(unsigned char regaddr)
{
	unsigned char *data_addr;
	int regval = -1;
	
	data_addr = get_data_addr(regaddr);
	if(data_addr)
	{
		regval = *data_addr;
		pr_debug("regval:%x\n",regval);
	}

	return regval;
}
#endif


/*************************************************************************************************
 **************************
 **************************		Register Operation Functions
 **************************
 *************************************************************************************************/
static int bd37033_mixing_setup(struct bd37033_data *bd, int type, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char mixing  = select_data.select_data_mixing_setup;
	int ret = -1;

	switch(type)
	{
		case BD37033_MIXING_1CH_SWITCH_TYPE:
			mixing &= ~(0x01<<1);
			mixing |= value<<1;
			break;
		case BD37033_MIXING_2CH_SWITCH_TYPE:
			mixing &= ~(0x01<<2);
			mixing |= value<<2;
			break;
		case BD37033_MIXING_LOUDNESS_F0_TYPE:
			mixing &= ~(0x03<<3);
			mixing |= value<<3;
			break;
		case BD37033_MIXING_INPUT_SELECT_TYPE:
			mixing &= ~(0x03<<6);
			mixing |= value<<6;
			break;
		default:
			break;
	}
	
	ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_MIXING_SETUP, mixing);
	if(ret == 0)
		select_data.select_data_mixing_setup = mixing;
	
	return ret;
}

static int bd37033_select_input_channel(struct bd37033_data *bd, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char input_select = select_data.select_data_input_selector;
	unsigned char mute_state = select_data.select_data_input_gain >> 7;
	int ret = -1;

	switch(value)
	{
		case BD37033_INPUT_SELECTOR_A_SINGLE:
		case BD37033_INPUT_SELECTOR_B_SINGLE:
		case BD37033_INPUT_SELECTOR_C_SINGLE:
		case BD37033_INPUT_SELECTOR_D_SINGLE:
		case BD37033_INPUT_SELECTOR_E1_SINGLE:
		case BD37033_INPUT_SELECTOR_E2_SINGLE:
		case BD37033_INPUT_SELECTOR_D_DIFF:
		case BD37033_INPUT_SELECTOR_E_FULL_DIFF:
		case BD37033_INPUT_SELECTOR_INPUT_SHORT:
			bd37033_set_mute(bd,MUTE_ON);	//mute while switch input channel
			input_select &= ~(0x1f);
			input_select |= value;
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_INPUT_SELECTOR, input_select);
			if(ret == 0)
				select_data.select_data_input_selector = input_select;
			bd37033_set_mute(bd,mute_state);
			break;
		case BD37033_INPUT_SELECTOR_FULL_DIFF_TYPE_NEGATIVE_INPUT:
			input_select &= ~(0x1<<7);
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_INPUT_SELECTOR, input_select);
			if(ret == 0)
				select_data.select_data_input_selector = input_select;
			break;
		case BD37033_INPUT_SELECTOR_FULL_DIFF_TYPE_BIAS:
			input_select |= (0x1<<7);
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_INPUT_SELECTOR, input_select);
			if(ret == 0)
				select_data.select_data_input_selector = input_select;
			break;
		default:
			break;
	}	

	return ret;
}

static int bd37033_set_input_gain(struct bd37033_data *bd, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char input_gain  = select_data.select_data_input_gain;
	int ret = -1;

	/* maximum input gain 16dB */
	if(value > 0x10)
		value = 0x10;

	/* minimum input gain 0dB */
	if(value >= 0)
	{
		input_gain &= ~(0x1f);
		input_gain |= value;
		
		ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_INPUT_GAIN, input_gain);
		if(ret == 0)
			select_data.select_data_input_gain = input_gain;
	}
	return ret;
}

static int bd37033_set_mute(struct bd37033_data *bd, int state)
{
	struct i2c_client *client = bd->client;
	unsigned char mute_state  = select_data.select_data_input_gain;
	int ret = -1;
	
	if(state == MUTE_ON)
	{
		mute_state |= (1<<7);
	}
	else if(state == MUTE_OFF)
	{
		mute_state &= ~(1<<7);
	}
	
	ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_INPUT_GAIN, mute_state);
	if(ret == 0)
		select_data.select_data_input_gain = mute_state;

	return ret;
}

static int bd37033_set_volume_gain(struct bd37033_data *bd, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char volume  = select_data.select_data_volume_gain;
	int ret = -1;

	if(value == 0xff)
	{
		volume = 0xff;
	}
	else if(value >= 0x80)
	{
		if(value > 0xcf)
			value = 0xcf;
		volume = value;
	}
	else
	{
		if(value < 0x71)
			value = 0x71;
		volume = value;
	}

	ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_VOLUME_GAIN, volume);
	if(ret == 0)
		select_data.select_data_volume_gain = volume;

	return ret;
}


static int bd37033_set_fader_mixing_gain(struct bd37033_data *bd, unsigned char select_addr, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char *data_addr = get_data_addr(select_addr);
	unsigned char gain = *data_addr;
	int ret = -1;
	
	if(value == 0xff)
	{
		gain = value;
	}
	else if(value > 0x80)
	{
		if(value > 0xcf)
			value = 0xcf;
		gain = value;
	}
	else
	{
		if(value < 0x71)
			value = 0x71;
		gain = value;
	}
	
	ret = bd37033_write_byte(client, client->addr, select_addr, gain);
	if(ret == 0)
		*data_addr = gain;
	
	return ret;
}


static int bd37033_bass_middle_treble_setup(struct bd37033_data *bd, unsigned char mode, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char select = 0;
	int ret = -1;

	
	switch(mode)
	{
		case BASS_Q:
		{
			select = select_data.select_data_bass_setup;
			select &= ~(0x3<<0);
			select |= value;
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_BASS_SETUP, select);
			if(ret == 0)
				select_data.select_data_bass_setup = select;
			break;
		}
		case BASS_F0:
		{
			select = select_data.select_data_bass_setup;
			select &= ~(0x3<<4);
			select |= (value<<4);
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_BASS_SETUP, select);
			if(ret == 0)
				select_data.select_data_bass_setup = select;
			break;
		}
		case MIDDLE_Q:
		{
			select = select_data.select_data_middle_setup;
			select &= ~(0x3<<0);
			select |= value;
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_MIDDLE_SETUP, select);
			if(ret == 0)
				select_data.select_data_middle_setup = select;
			break;
		}
		case MIDDLE_F0:
		{
			select = select_data.select_data_middle_setup;
			select &= ~(0x3<<4);
			select |= (value<<4);
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_MIDDLE_SETUP, select);
			if(ret == 0)
				select_data.select_data_middle_setup = select;
			break;
		}
		case TREBLE_Q:
		{
			select = select_data.select_data_treble_setup;
			select &= ~(0x1<<0);
			select |= value;
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_TREBLE_SETUP, select);
			if(ret == 0)
				select_data.select_data_treble_setup = select;
			break;
		}
		case TREBLE_F0:
		{
			select = select_data.select_data_treble_setup;
			select &= ~(0x3<<4);
			select |= (value<<4);
			ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_TREBLE_SETUP, select);
			if(ret == 0)
				select_data.select_data_treble_setup = select;
			break;
		}
		default:
			break;
	}

	return ret;
}
static int bd37033_set_bass_middle_treble_gain(struct bd37033_data *bd, unsigned char select_addr, unsigned char value)
{
	struct i2c_client *client = bd->client;
	unsigned char *data_addr = get_data_addr(select_addr);
	int ret = -1;

	ret = bd37033_write_byte(client, client->addr, select_addr, value);
	if(ret == 0)
		*data_addr = value;
	
	return ret;
}

static int bd37033_set_loudness_gain(struct bd37033_data *bd, signed char value)
{
	struct i2c_client *client = bd->client;
	unsigned char loudness  = select_data.select_data_loudness_gain;
	int ret = -1;

	if(value > 0x0f)
		value = 0x0f;
	if(value < 0)
		return ret;
	
	loudness &= ~(0x1f);
	loudness |= value;
	ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_LOUDNESS_GAIN, loudness);
	if(ret == 0)
		select_data.select_data_loudness_gain = loudness;
	
	return ret;
}
static int bd37033_set_loudness_hicut(struct bd37033_data *bd, signed char value)
{
	struct i2c_client *client = bd->client;
	unsigned char hicut  = select_data.select_data_loudness_gain;
	int ret = -1;
	
	hicut &= ~(3<<5);
	hicut |= value<<5;
	
	ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_LOUDNESS_GAIN, hicut);
	if(ret == 0)
		select_data.select_data_loudness_gain = hicut;
	
	return ret;
}

static int bd37033_lpf_setup(struct bd37033_data *bd, int type, signed char value)
{
	struct i2c_client *client = bd->client;
	unsigned char lpf  = select_data.select_data_lpf_setup;
	int ret = -1;

	switch(type)
	{
		case BD37033_LPF_SETUP_SUB_LPF_FC_TYPE:
			lpf &= ~(0x07<<0);
			lpf |= value<<0;
			if(value == 0x05)	//fc = pass ==> parse = 0
			{
				lpf &= ~(0x01<<7);
			}
			break;
		case BD37033_LPF_SETUP_SUB_INPUT_SELECT_TYPE:
			lpf &= ~(0x01<<3);
			lpf |= value<<3;
			break;
		case BD37033_LPF_SETUP_SUB_OUTPUT_SELECT_TYPE:
			lpf &= ~(0x03<<4);
			lpf |= value<<4;
			break;
		case BD37033_LPF_SETUP_LEVEL_METER_RESET_TYPE:
			lpf &= ~(0x01<<6);
			lpf |= value<<6;
			break;
		case BD37033_LPF_SETUP_LPF_PHASE_TYPE:
			lpf &= ~(0x01<<7);
			lpf |= value<<7;
			break;
		default:
			break;
	}

	ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_LPF_SETUP, lpf);
	if(ret == 0)
		select_data.select_data_lpf_setup = lpf;

	return ret;
}

int bd37033_sys_reset(struct bd37033_data *bd)
{
	//struct i2c_client *client = bd->client;
	//int ret = -1;
	
	//ret = bd37033_write_byte(client, client->addr, SELECT_ADDR_SYSTEM_RESET, 0x81);

	if(bd37033_set_mute(bd,MUTE_ON) == 0)
		mute_status = MUTE_ON;
	
	bd37033_init(bd);

	return 0;
}

int bd37033_set_default(struct bd37033_data *bd)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);		
	//input channel select
	if(mp.input_channel >= 0)
		bd37033_set_stream(mp.input_channel);
	else
		bd37033_select_input_channel(bd,BD37033_INPUT_SELECTOR_B_SINGLE);	//default: BD37033_INPUT_SELECTOR_D_SINGLE
		//bd37033_select_input_channel(bd,BD37033_INPUT_SELECTOR_D_SINGLE);       //default: navi
	
	//set loudness f0
	bd37033_mixing_setup(my_bd,BD37033_MIXING_LOUDNESS_F0_TYPE,1);	//1: 800HZ

	//set volume
	if(mp.volume >= 0)
		bd37033_set_volume(mp.volume);
	else	//default : 0x94
		bd37033_set_volume_gain(bd,0x94);
	
	//set input gain
	if(mp.input_gain >= 0)
		bd37033_set_input_gain(bd,mp.input_gain);
	else	//default: 0
		bd37033_set_input_gain(bd,0);
	
	//select out channel
	if (!of_property_read_u32(bd->client->dev.of_node, "flout-gain", &bd->dac.vol_flout))
		mp.fl_gain = bd->dac.vol_flout;
	if (!of_property_read_u32(bd->client->dev.of_node, "frout-gain", &bd->dac.vol_flout))
		mp.fr_gain = bd->dac.vol_flout;
	if (!of_property_read_u32(bd->client->dev.of_node, "rlout-gain", &bd->dac.vol_flout))
		mp.rl_gain = bd->dac.vol_flout;
	if (!of_property_read_u32(bd->client->dev.of_node, "rrout-gain", &bd->dac.vol_flout))
		mp.rr_gain = bd->dac.vol_flout;
	dev_dbg(&bd->client->dev, "fl_gain=%d fr_gain=%d rl_gain=%d rr_gain=%d\n",mp.fl_gain,mp.fr_gain,mp.rl_gain,mp.rr_gain);
	if(mp.fl_gain >= 0)
		bd37033_set_fader(SELECT_ADDR_FADER_1CH_FRONT,mp.fl_gain);
	if(mp.fr_gain >= 0)
		bd37033_set_fader(SELECT_ADDR_FADER_2CH_FRONT,mp.fr_gain);
	if(mp.rl_gain >= 0)
		bd37033_set_fader(SELECT_ADDR_FADER_1CH_REAR,mp.rl_gain);
	if(mp.rr_gain >= 0)
		bd37033_set_fader(SELECT_ADDR_FADER_2CH_REAR,mp.rr_gain);
	if((mp.fl_gain < 0) && (mp.fr_gain < 0) && (mp.rl_gain < 0) && (mp.rr_gain < 0))	//default: fl
		bd37033_set_fader_mixing_gain(bd,SELECT_ADDR_FADER_1CH_FRONT,0);	//FL  

	//mp.mute  -1: default unmute; 0->unmute; others:mute
	if(mp.mute <= 0)
	{
		if(bd37033_set_mute(bd,MUTE_OFF) == 0)
			mute_status = MUTE_OFF;
	}
	else
	{
		if(bd37033_set_mute(bd,MUTE_ON) == 0)
			mute_status = MUTE_ON;
	}

	if(mp.bass_gain >= 0)
		set_tone_gain(SELECT_ADDR_BASS_GAIN, mp.bass_gain);
	if(mp.middle_gain >= 0)
		set_tone_gain(SELECT_ADDR_MIDDLE_GAIN, mp.middle_gain);
	if(mp.treble_gain >= 0)
		set_tone_gain(SELECT_ADDR_TREBLE_GAIN, mp.treble_gain);
	
	memset(&mp, -1, sizeof(struct bd37033_module_param));
	
	return 0;
}



static void bd37033_init(struct bd37033_data *bd)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct i2c_client *client = NULL;

	if (bd == NULL)
	{
		pr_err("%s: bd37033_data null\n",__FUNCTION__);
		return ;
	}

	client = bd->client;
	if (client == NULL)
	{
		pr_err("%s: i2c_client null\n",__FUNCTION__);
		return ;
	}

	//set default value
	select_data.select_data_initial_setup = 0xA4;
	select_data.select_data_lpf_setup = 0x00;
	select_data.select_data_mixing_setup = 0x00;
	select_data.select_data_input_selector = 0x0;
	select_data.select_data_input_gain = 0x80;
	mute_status = MUTE_ON;
	select_data.select_data_volume_gain = 0xff;
	select_data.select_data_fader_1ch_front_gain = 0xff;
	select_data.select_data_fader_2ch_front_gain = 0xff;
	select_data.select_data_fader_1ch_rear_gain = 0xff;
	select_data.select_data_fader_2ch_rear_gain = 0xff;
	select_data.select_data_fader_1ch_sub_gain = 0xff;
	select_data.select_data_mixing_2ch_sub_gain = 0xff;
	select_data.select_data_bass_setup = 0x0;
	select_data.select_data_middle_setup = 0x0;
	select_data.select_data_treble_setup = 0x0;
	select_data.select_data_bass_gain = 0x80;
	select_data.select_data_middle_gain = 0x80;
	select_data.select_data_treble_gain = 0x80;
	select_data.select_data_loudness_gain = 0x0;
	select_data.select_data_system_reset = 0x81;

	bd37033_write_bytes(client, client->addr, SELECT_ADDR_INITIAL_SETUP, (unsigned char *)(&select_data),sizeof(select_data));

	bd37033_set_default(bd);
}


/*************************************************************************************************
 **************************
 **************************		ALSA Controls Interface Functions
 **************************
 *************************************************************************************************/
static int get_stream (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{	
	unsigned char input_ch = select_data.select_data_input_selector & 0x1f;
	
	switch(input_ch)
	{
		case BD37033_INPUT_SELECTOR_A_SINGLE:
			ucontrol->value.integer.value[0] = BD37033_STREAM_A_SINGLE;
			break;
		case BD37033_INPUT_SELECTOR_B_SINGLE:
			ucontrol->value.integer.value[0] = BD37033_STREAM_B_SINGLE;
			break;
		case BD37033_INPUT_SELECTOR_C_SINGLE:
			ucontrol->value.integer.value[0] = BD37033_STREAM_C_SINGLE;
			break;
		case BD37033_INPUT_SELECTOR_D_SINGLE:
			ucontrol->value.integer.value[0] = BD37033_STREAM_D_SINGLE;
			break;
		case BD37033_INPUT_SELECTOR_E1_SINGLE:
			ucontrol->value.integer.value[0] = BD37033_STREAM_E1_SINGLE;
			break;
		case BD37033_INPUT_SELECTOR_E2_SINGLE:
			ucontrol->value.integer.value[0] = BD37033_STREAM_E2_SINGL;
			break;
		case BD37033_INPUT_SELECTOR_D_DIFF:
			ucontrol->value.integer.value[0] = BD37033_STREAM_D_DIF;
			break;
		case BD37033_INPUT_SELECTOR_E_FULL_DIFF:
			ucontrol->value.integer.value[0] = BD37033_STREAM_E_FULL_DIFF;
			break;
		case BD37033_INPUT_SELECTOR_INPUT_SHORT:
			ucontrol->value.integer.value[0] = BD37033_STREAM_INPUT_SHORT;
			break;
		default:
			break;
	}

	return 0;
}

static int bd37033_set_stream(unsigned char input_ch)
{
	int ret = -1;
	switch (input_ch) {
		case BD37033_STREAM_A_SINGLE:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_A_SINGLE);
			break;
		case BD37033_STREAM_B_SINGLE:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_B_SINGLE);
			break;
		case BD37033_STREAM_C_SINGLE:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_C_SINGLE);
			break;
		case BD37033_STREAM_D_SINGLE:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_D_SINGLE);
			break;
		case BD37033_STREAM_E1_SINGLE:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_E1_SINGLE);
			break;
		case BD37033_STREAM_E2_SINGL:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_E2_SINGLE);
			break;
		case BD37033_STREAM_D_DIF:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_D_DIFF);
			break;
		case BD37033_STREAM_E_FULL_DIFF:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_E_FULL_DIFF);
			break;
		case BD37033_STREAM_INPUT_SHORT:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_INPUT_SHORT);
			break;
		default:
			break;
	}

	return ret;
}
static int set_stream (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	int ret = -1;
	unsigned char input_ch = select_data.select_data_input_selector & 0x1f;

	if (input_ch == ucontrol->value.integer.value[0])
		return 0;

	input_ch = ucontrol->value.integer.value[0];

	ret = bd37033_set_stream(input_ch);

	return ret;
}

static int get_stream_full_diff_type (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{	
	ucontrol->value.integer.value[0] = (select_data.select_data_input_selector >> 7) & 0x1;

	return 0;
}
static int set_stream_full_diff_type (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{	
	unsigned char type = (select_data.select_data_input_selector >> 7) & 0x1;
	int ret = -1;

	if(type == ucontrol->value.integer.value[0])
		return 0;

	type = ucontrol->value.integer.value[0];

	switch(type)
	{
		case 0:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_FULL_DIFF_TYPE_NEGATIVE_INPUT);
			break;
		case 1:
			ret = bd37033_select_input_channel(my_bd,BD37033_INPUT_SELECTOR_FULL_DIFF_TYPE_BIAS);
			break;
		default:
			break;
	}
	
	return ret;
}


/***************************************************************************************************
 *
 *	Input Gain Setting
 *
 ***************************************************************************************************/
static int get_stream_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{	
	ucontrol->value.integer.value[0] = (select_data.select_data_input_gain & 0x1f);

	return 0;
}

static int set_stream_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	int ret = -1;
	unsigned char gain = (select_data.select_data_input_gain & 0x1f);

	if (gain == ucontrol->value.integer.value[0])
		return 0;

	gain = ucontrol->value.integer.value[0];
	ret = bd37033_set_input_gain(my_bd,gain);
	
	return ret;
}

/***************************************************************************************************
 *
 *	Fader Gain Setting
 *
 ***************************************************************************************************/
static int bd37033_get_fader(unsigned char reg_addr)
{
	unsigned char *fader_addr = get_data_addr(reg_addr);
	unsigned char fader = *fader_addr;

	if(fader >= 0x80)
	{
		if(fader == 0xff)
		{
			fader = 0;
		}
		else if(fader >= 0xcf)
		{
			fader = 1;
		}
		else
		{
			fader &= ~(0x1<<7);
			fader = 80 - fader;
		}
	}
	else
	{
		if(fader < 0x71)	//make sure fader gain <= 15db
			fader = 95;
		else
			fader = 80 + (0x80 - fader);
	}

	return fader;
}
static int bd37033_set_fader(unsigned char reg_addr, unsigned char value)
{
	unsigned char fader = bd37033_get_fader(reg_addr);
	int ret = -1;

	if (fader == value)
		return 0;

	fader = value;

	if(fader <= 80)
	{
		if(fader == 0)	// mute
		{
			fader = 0xff;
		}
		else	// -79db < fader gain <= 0db
		{
			fader = 80 - fader;
			fader |= (1<<7);
		}
	}
	else
	{
		if(fader > 95)	// fader gain > +15db
		{
			fader = 0x71;	//+15db
		}
		else	//0db < fader gain < +15db
		{
			fader -= 80;
			fader = 0x80 - fader;
		}
	}
	ret = bd37033_set_fader_mixing_gain(my_bd,reg_addr,fader);

	return ret;
}


static int get_fl_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = bd37033_get_fader(SELECT_ADDR_FADER_1CH_FRONT);
		
	return 0;
}
static int set_fl_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return bd37033_set_fader(SELECT_ADDR_FADER_1CH_FRONT,ucontrol->value.integer.value[0]);
}

static int get_fr_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = bd37033_get_fader(SELECT_ADDR_FADER_2CH_FRONT);
		
	return 0;
}
static int set_fr_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return bd37033_set_fader(SELECT_ADDR_FADER_2CH_FRONT,ucontrol->value.integer.value[0]);
}

static int get_rl_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = bd37033_get_fader(SELECT_ADDR_FADER_2CH_REAR);
		
	return 0;
}
static int set_rl_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return bd37033_set_fader(SELECT_ADDR_FADER_2CH_REAR,ucontrol->value.integer.value[0]);
}

static int get_rr_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = bd37033_get_fader(SELECT_ADDR_FADER_1CH_REAR);
		
	return 0;
}
static int set_rr_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return bd37033_set_fader(SELECT_ADDR_FADER_1CH_REAR,ucontrol->value.integer.value[0]);
}

static int get_sub1_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = bd37033_get_fader(SELECT_ADDR_FADER_1CH_SUB);
		
	return 0;
}
static int set_sub1_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return bd37033_set_fader(SELECT_ADDR_FADER_1CH_SUB,ucontrol->value.integer.value[0]);
}

static int get_sub2_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = bd37033_get_fader(SELECT_ADDR_MIXINT_2CH_SUB);
		
	return 0;
}
static int set_sub2_fader (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return bd37033_set_fader(SELECT_ADDR_MIXINT_2CH_SUB,ucontrol->value.integer.value[0]);
}


/***************************************************************************************************
 *
 *	Volume Gain Setting
 *
 ***************************************************************************************************/
static int bd37033_get_volume(void)
{
	unsigned char volume  = select_data.select_data_volume_gain;

	if(volume >= 0x80)
	{
		if(volume == 0xff)
		{
			volume = 0;
		}
		else if(volume >= 0xcf)
		{
			volume = 1;
		}
		else
		{
			volume &= ~(0x1<<7);
			volume = 80 - volume;
		}
	}
	else
	{
		if(volume < 0x71)	//volume gain < 15db
			volume = 95;
		else
			volume = 80 + (0x80 - volume);
	}

	return volume;
}

static int bd37033_set_volume(unsigned char volume)
{
	int ret;
#if 1	//if volume=0, then set mute
	if(volume == 0)
	{
		if((select_data.select_data_input_gain & 0x80) == 0x0)
			bd37033_set_mute(my_bd,MUTE_ON);
	}
	else
	{
		if((select_data.select_data_input_gain & 0x80) == 0x80)
		{
			if(mute_status == MUTE_OFF)//when register is set mute, if mute_status==false, the register is set by volume=0, not mute setting,so when volume > 0, do not set mute off
				bd37033_set_mute(my_bd,MUTE_OFF);
		}
	}
#endif
	
	if(volume <= 80)
	{
		if(volume == 0) // volume gain < -79db 
		{
			volume = 0xff;
		}
		else	// -79db < volume gain <= 0db
		{
			volume = 80 - volume;
			volume |= (1<<7);
		}
	}
	else
	{
		if(volume > 95) // volume gain > +15db
		{
			volume = 0x71;	//+15db
		}
		else	//0db < volume gain < +15db
		{
			volume -= 80;
			volume = 0x80 - volume;
		}
	}

	ret = bd37033_set_volume_gain(my_bd,volume);

	return ret;
}

static int get_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{	
	ucontrol->value.integer.value[0] = bd37033_get_volume();

	return 0;
}

static int set_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char volume = bd37033_get_volume();

	if (volume == ucontrol->value.integer.value[0])
		return 0;

	volume = ucontrol->value.integer.value[0];
	
	ret = bd37033_set_volume(volume);

	return ret;
}


/***************************************************************************************************
 *
 *	Mixing Setting
 *
 ***************************************************************************************************/
static int get_mixing_channel1_switch(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_mixing_setup >> 1) & 0x01;
	
	return 0;
}

static int set_mixing_channel1_switch (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	unsigned char ch1 = (select_data.select_data_mixing_setup >> 1) & 0x01;
	int ret = -1;
	
	if (ch1 == ucontrol->value.integer.value[0])
		return 0;

	ch1 = ucontrol->value.integer.value[0];
	
	switch (ch1)
	{
		case 0:
		case 1:
			ret = bd37033_mixing_setup(my_bd,BD37033_MIXING_1CH_SWITCH_TYPE,ch1);
			break;
		default:
			break;
	}

	return ret;
}

static int get_mixing_channel2_switch(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_mixing_setup >> 2) & 0x01;
	
	return 0;
}

static int set_mixing_channel2_switch (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	unsigned char ch2 = (select_data.select_data_mixing_setup >> 2) & 0x01;
	int ret = -1;
	
	if (ch2 == ucontrol->value.integer.value[0])
		return 0;

	ch2 = ucontrol->value.integer.value[0];
	
	switch (ch2)
	{
		case 0:
		case 1:
			ret = bd37033_mixing_setup(my_bd,BD37033_MIXING_2CH_SWITCH_TYPE,ch2);
			break;
		default:
			break;
	}

	return ret;
}

static int get_loudness_f0(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_mixing_setup>>3) & 0x03;
	
	return 0;
}

static int set_loudness_f0 (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	unsigned char f0 = (select_data.select_data_mixing_setup>>3) & 0x03;
	int ret = -1;
	
	if (f0 == ucontrol->value.integer.value[0])
		return 0;

	f0 = ucontrol->value.integer.value[0];
	
	switch (f0)
	{
		case 0:
		case 1:
		case 2:
			ret = bd37033_mixing_setup(my_bd,BD37033_MIXING_LOUDNESS_F0_TYPE,f0);
			break;
		default:
			break;
	}

	return ret;
}

static int get_mixing_input_select(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_mixing_setup >> 6) & 0x03;
	
	return 0;
}

static int set_mixing_input_select (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	unsigned char input = (select_data.select_data_mixing_setup >> 6) & 0x03;
	int ret = -1;
	
	if (input == ucontrol->value.integer.value[0])
		return 0;

	input = ucontrol->value.integer.value[0];
	
	switch (input)
	{
		case 0:
		case 1:
		case 2:
			ret = bd37033_mixing_setup(my_bd,BD37033_MIXING_INPUT_SELECT_TYPE,input);
			break;
		default:
			break;
	}

	return ret;
}


/***************************************************************************************************
 *
 *	Mute setting
 *
 ***************************************************************************************************/
static int get_mute_status(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
#if 0
	if((select_data.select_data_input_gain & 0x80) == 0x80)
		ucontrol->value.integer.value[0] = MUTE_ON;
	else
		ucontrol->value.integer.value[0] = MUTE_OFF;
#else
	ucontrol->value.integer.value[0] = mute_status;
#endif

	return 0;
}

static int set_mute_status (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	unsigned char mute = (select_data.select_data_input_gain & 0x80)>>7;
	int ret = -1;
	
	if (mute == ucontrol->value.integer.value[0])
		return 0;

	mute = ucontrol->value.integer.value[0];
	
	switch (mute)
	{
		case MUTE_OFF:
			ret = bd37033_set_mute(my_bd,MUTE_OFF);
			if(ret == 0)
				mute_status = MUTE_OFF;
			break;
		case MUTE_ON:
			ret = bd37033_set_mute(my_bd,MUTE_ON);
			if(ret == 0)
				mute_status = MUTE_ON;
			break;
		default:
			break;
	}

	return ret;
}


/***************************************************************************************************
 *
 *	Tone F0 and Q Setting
 *
 ***************************************************************************************************/
static int set_tone_q(unsigned char reg_addr, unsigned char type, unsigned char value)
{
	int ret = -1;
	unsigned char q = *(get_data_addr(reg_addr));
	
	q = (q>>0)&(0x3);
	
	if (q == value)
		return 0;
	
	switch (value) {
		case 0:
		case 1:
		case 2:
		case 3:
			if(type == TREBLE_Q)
			{
				if(value < 2)
					ret = bd37033_bass_middle_treble_setup(my_bd,type,value);
			}
			else
			{
				ret = bd37033_bass_middle_treble_setup(my_bd,type,value);
			}
			break;
		default:
			break;
	}

	return ret;
}
static int set_tone_f0(unsigned char reg_addr, unsigned char type, unsigned char value)
{
	int ret = -1;
	unsigned char f0 = *(get_data_addr(reg_addr));
	
	f0 = (f0>>4)&(0x3);
	
	if (f0 == value)
		return 0;
	
	switch (value) {
		case 0:
		case 1:
		case 2:
		case 3:
			ret = bd37033_bass_middle_treble_setup(my_bd,type,value);
			break;
		default:
			break;
	}

	return ret;
}

static int get_bass_q(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_bass_setup & 0x3);

	return 0;
}
static int set_bass_q (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_q(SELECT_ADDR_BASS_SETUP,BASS_Q,ucontrol->value.integer.value[0]);
}
static int get_middle_q(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_middle_setup & 0x3);

	return 0;
}
static int set_middle_q (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_q(SELECT_ADDR_MIDDLE_SETUP,MIDDLE_Q,ucontrol->value.integer.value[0]);
}
static int get_treble_q(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_treble_setup & 0x1);

	return 0;
}
static int set_treble_q (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_q(SELECT_ADDR_TREBLE_SETUP,TREBLE_Q,ucontrol->value.integer.value[0]);
}

static int get_bass_f0(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_bass_setup>>4) & 0x3;

	return 0;
}
static int set_bass_f0 (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_f0(SELECT_ADDR_BASS_SETUP,BASS_F0,ucontrol->value.integer.value[0]);
}
static int get_middle_f0(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_middle_setup>>4) & 0x3;

	return 0;
}
static int set_middle_f0 (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_f0(SELECT_ADDR_MIDDLE_SETUP,MIDDLE_F0,ucontrol->value.integer.value[0]);
}
static int get_treble_f0(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_treble_setup>>4) & 0x3;

	return 0;
}
static int set_treble_f0 (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_f0(SELECT_ADDR_TREBLE_SETUP,TREBLE_F0,ucontrol->value.integer.value[0]);
}


/***************************************************************************************************
 *
 *	Tone Gain Setting
 *
 ***************************************************************************************************/
static int get_tone_gain(unsigned char reg_addr)
{
	unsigned char gain = *(get_data_addr(reg_addr));

	if(gain & (0x1<<7))	//boost
		gain = 15 - (gain & 0x1f);
	else
		gain = 15 + (gain & 0x1f);
	
	return gain;
}

static int set_tone_gain(unsigned char reg_addr, unsigned char value)
{
	int ret = -1;
	unsigned char gain = get_tone_gain(reg_addr);
	
	if (gain == value)
		return 0;

	if(value <= 15)	//cut : 0-14, 15(boost or cut) ,boost : 16-30
	{
		gain = 15 - value;
		gain |= (0x1<<7);
	}
	else
	{
		gain = value - 15;
	}

	ret = bd37033_set_bass_middle_treble_gain(my_bd, reg_addr, gain);
	
	return ret;
}

static int get_bass_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = get_tone_gain(SELECT_ADDR_BASS_GAIN);

	return 0;
}
static int set_bass_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_gain(SELECT_ADDR_BASS_GAIN, ucontrol->value.integer.value[0]);
}

static int get_middle_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = get_tone_gain(SELECT_ADDR_MIDDLE_GAIN);

	return 0;
}
static int set_middle_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_gain(SELECT_ADDR_MIDDLE_GAIN, ucontrol->value.integer.value[0]);
}

static int get_treble_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = get_tone_gain(SELECT_ADDR_TREBLE_GAIN);

	return 0;
}
static int set_treble_gain (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	return set_tone_gain(SELECT_ADDR_TREBLE_GAIN, ucontrol->value.integer.value[0]);
}


/***************************************************************************************************
 *
 *	bd37033 reset
 *
 ***************************************************************************************************/
static int get_reset (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//ucontrol->value.integer.value[0] = select_data.select_data_system_reset;

	return 0;
}
static int set_reset (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);
	int ret = -1;
	
	ret = bd37033_sys_reset(my_bd);
	
	return ret;
}


/***************************************************************************************************
 *
 *	loudness setting
 *
 ***************************************************************************************************/
static int get_loudness (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = select_data.select_data_loudness_gain & 0x1f;

	return 0;
}
static int set_loudness (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char gain = select_data.select_data_loudness_gain & 0x1f;

	if (gain == ucontrol->value.integer.value[0])
		return 0;

	gain = ucontrol->value.integer.value[0];
	ret = bd37033_set_loudness_gain(my_bd, gain);
	return ret;
}
static int get_loudness_hicut (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_loudness_gain>>5) & 0x03;

	return 0;
}
static int set_loudness_hicut (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char hicut = (select_data.select_data_loudness_gain>>5) & 0x03;

	if (hicut == ucontrol->value.integer.value[0])
		return 0;

	hicut = ucontrol->value.integer.value[0];
	switch(hicut)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			ret = bd37033_set_loudness_hicut(my_bd, hicut);
			break;
		default:
			break;
	}
	return ret;
}


/***************************************************************************************************
 *
 *	subwoofer setting
 *
 ***************************************************************************************************/
static int get_lpf_fc (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = select_data.select_data_lpf_setup & 0x07;

	return 0;
}
static int set_lpf_fc (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char fc = select_data.select_data_lpf_setup & 0x07;

	if (fc == ucontrol->value.integer.value[0])
		return 0;

	fc = ucontrol->value.integer.value[0];

	switch(fc)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			ret = bd37033_lpf_setup(my_bd, BD37033_LPF_SETUP_SUB_LPF_FC_TYPE,fc);
			break;
		default:
			break;
	}
	
	return ret;
}
static int get_sub_input_select (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_lpf_setup >> 3) & 0x01;

	return 0;
}
static int set_sub_input_select (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char input = (select_data.select_data_lpf_setup >> 3) & 0x01;

	if (input == ucontrol->value.integer.value[0])
		return 0;

	input = ucontrol->value.integer.value[0];

	switch(input)
	{
		case 0:
		case 1:
			ret = bd37033_lpf_setup(my_bd, BD37033_LPF_SETUP_SUB_INPUT_SELECT_TYPE,input);
			break;
		default:
			break;
	}
	
	return ret;
}
static int get_sub_output_select (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_lpf_setup >> 4) & 0x03;

	return 0;
}
static int set_sub_output_select (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char output = (select_data.select_data_lpf_setup >> 4) & 0x03;

	if (output == ucontrol->value.integer.value[0])
		return 0;

	output = ucontrol->value.integer.value[0];

	switch(output)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			ret = bd37033_lpf_setup(my_bd, BD37033_LPF_SETUP_SUB_OUTPUT_SELECT_TYPE,output);
			break;
		default:
			break;
	}
	
	return ret;
}
static int get_sub_lpf_parse (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = (select_data.select_data_lpf_setup >> 7) & 0x01;

	return 0;
}
static int set_sub_lpf_parse (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	int ret = -1;
	unsigned char parse = (select_data.select_data_lpf_setup >> 7) & 0x01;

	if (parse == ucontrol->value.integer.value[0])
		return 0;

	parse = ucontrol->value.integer.value[0];

	switch(parse)
	{
		case 0:
		case 1:
			ret = bd37033_lpf_setup(my_bd, BD37033_LPF_SETUP_LPF_PHASE_TYPE,parse);
			break;
		default:
			break;
	}
	
	return ret;
}


static const char *stream_select[] = {
	"A_SINGLE",
	"B_SINGLE",
	"C_SINGLE:BT",
	"D_SINGLE:NAVI",
	"E1_SINGLE:RADIO",
	"E2_SINGLE:AUX",
	"D_DIFF:D",
	"E_FULL_DIFF:E1+E2",
	"INPUT_SHORT:ALL",
};
static const char *stream_full_diff_type_select[] = {
	"FULL_DIFF_TYPE:NEGATIVE INPUT",
	"FULL_DIFF_TYPE:BIAS",
};
static const char *bass_q_switch[] = {
	"BASS_Q:0.5",
	"BASS_Q:1.0",
	"BASS_Q:1.5",
	"BASS_Q:2.0",
};
static const char *middle_q_switch[] = {
	"MIDDLE_Q:0.75",
	"MIDDLE_Q:1.00",
	"MIDDLE_Q:1.25",
	"MIDDLE_Q:1.50",
};
static const char *treble_q_switch[] = {
	"TREBLE_Q:0.75",
	"TREBLE_Q:1.25",
};
static const char *bass_f0_switch[] = {
	"BASS_F0:60HZ",
	"BASS_F0:80HZ",
	"BASS_F0:100HZ",
	"BASS_F0:120HZ",
};
static const char *middle_f0_switch[] = {
	"MIDDLE_F0:0.5KHZ",
	"MIDDLE_F0:1KHZ",
	"MIDDLE_F0:1.5KHZ",
	"MIDDLE_F0:2.5KHZ",
};
static const char *treble_f0_switch[] = {
	"TREBLE_F0:7.5KHZ",
	"TREBLE_F0:10KHZ",
	"TREBLE_F0:12.5KHZ",
	"TREBLE_F0:15KHZ",
};
static const char *loudness_f0_switch[] = {
	"LOUDNESS_F0:400HZ",
	"LOUDNESS_F0:800HZ",
	"LOUDNESS_F0:2400HZ",
};
static const char *loudness_hicut_switch[] = {
	"HI_CUT:0",
	"HI_CUT:1",
	"HI_CUT:2",
	"HI_CUT:3",
};

static const char *mixing_input_select[] = {
	"MIN",
	"A_SINGLE",
	"B_SINGLE",
};
static const char *mixing_channel_select[] = {
	"ON",
	"OFF",
};

static const char *mute_switch[] = {
	"OFF",
	"ON",
};
static const char *reset_switch[] = {
	"RESET",
};
static const char *lpf_fc[] = {
	"OFF",
	"55HZ",
	"85HZ",
	"120HZ",
	"160HZ",
	"PASS",
};
static const char *sub_input_select[] = {
	"LOUDNESS",
	"INPUT SELECTOR",
};
static const char *sub_output_select[] = {
	"LPF",
	"FRONT",
	"REAR",
	"SUBWOOFER",
};
static const char *sub_lpf_parse[] = {
	"0",
	"180",
};
static const struct soc_enum stream_sel_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(stream_select), stream_select);
static const struct soc_enum stream_full_diff_type_sel_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(stream_full_diff_type_select), stream_full_diff_type_select);

static const struct soc_enum mixing_channel1_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mixing_channel_select), mixing_channel_select);
static const struct soc_enum mixing_channel2_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mixing_channel_select), mixing_channel_select);
static const struct soc_enum loudness_f0_select_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(loudness_f0_switch), loudness_f0_switch);
static const struct soc_enum mixing_input_select_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mixing_input_select), mixing_input_select);

static const struct soc_enum loudness_hicut_select_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(loudness_hicut_switch), loudness_hicut_switch);



static const struct soc_enum bass_q_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(bass_q_switch), bass_q_switch);
static const struct soc_enum middle_q_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(middle_q_switch), middle_q_switch);
static const struct soc_enum treble_q_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(treble_q_switch), treble_q_switch);
static const struct soc_enum bass_f0_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(bass_f0_switch), bass_f0_switch);
static const struct soc_enum middle_f0_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(middle_f0_switch), middle_f0_switch);
static const struct soc_enum treble_f0_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(treble_f0_switch), treble_f0_switch);

static const struct soc_enum mute_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mute_switch), mute_switch);

static const struct soc_enum reset_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(reset_switch), reset_switch);

static const struct soc_enum sub_lpf_fc_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(lpf_fc), lpf_fc);
static const struct soc_enum sub_input_select_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sub_input_select), sub_input_select);
static const struct soc_enum sub_output_select_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sub_output_select), sub_output_select);
static const struct soc_enum sub_lpf_parse_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(sub_lpf_parse), sub_lpf_parse);


static const struct snd_kcontrol_new bd37033_controls[] = {
	SOC_SINGLE_EXT("EQ Bass", 0, 0, 30, 0,
		get_bass_gain, set_bass_gain),
	SOC_ENUM_EXT("EQ BassF0", bass_f0_switch_enum,
		get_bass_f0, set_bass_f0),
	SOC_ENUM_EXT("EQ BassQ", bass_q_switch_enum,
		get_bass_q, set_bass_q),
	SOC_SINGLE_EXT("EQ Middle", 0, 0, 30, 0,
		get_middle_gain, set_middle_gain),
	SOC_ENUM_EXT("EQ MiddleF0", middle_f0_switch_enum,
		get_middle_f0, set_middle_f0),
	SOC_ENUM_EXT("EQ MiddleQ", middle_q_switch_enum,
		get_middle_q, set_middle_q),
	SOC_SINGLE_EXT("EQ Treble", 0, 0, 30, 0,
		get_treble_gain, set_treble_gain),
	SOC_ENUM_EXT("EQ TrebleF0", treble_f0_switch_enum,
		get_treble_f0, set_treble_f0),
	SOC_ENUM_EXT("EQ TrebleQ", treble_q_switch_enum,
		get_treble_q, set_treble_q),

	SOC_SINGLE_EXT("PA Input-Gain", 0, 0, 16, 0,
		get_stream_gain, set_stream_gain),
	SOC_ENUM_EXT("PA Input Select", stream_sel_enum,
		get_stream, set_stream),
	SOC_ENUM_EXT("PA Input-Full-Diff-Type Select", stream_full_diff_type_sel_enum,
		get_stream_full_diff_type, set_stream_full_diff_type),
			
	SOC_SINGLE_EXT("PA Fader-FL", 0, 0, 95, 0,
		get_fl_fader, set_fl_fader),
	SOC_SINGLE_EXT("PA Fader-FR", 0, 0, 95, 0,
		get_fr_fader, set_fr_fader),
	SOC_SINGLE_EXT("PA Fader-RL", 0, 0, 95, 0,
		get_rl_fader, set_rl_fader),
	SOC_SINGLE_EXT("PA Fader-RR", 0, 0, 95, 0,
		get_rr_fader, set_rr_fader),
	SOC_SINGLE_EXT("PA Fader-Sub1", 0, 0, 95, 0,
		get_sub1_fader, set_sub1_fader),
	SOC_SINGLE_EXT("PA Fader-Sub2", 0, 0, 95, 0,
		get_sub2_fader, set_sub2_fader),
		
	SOC_ENUM_EXT("PA Mute", mute_switch_enum,
		get_mute_status, set_mute_status),

	SOC_ENUM_EXT("PA Reset", reset_enum,
		get_reset, set_reset),
		
	SOC_SINGLE_EXT("PA Volume", 0, 0, 95, 0,
		get_volume, set_volume),
			
	SOC_SINGLE_EXT("PA Loudness", 0, 0, 15, 0,
		get_loudness, set_loudness),
	SOC_ENUM_EXT("PA Loudness-HiCut Select", loudness_hicut_select_enum,
		get_loudness_hicut, set_loudness_hicut),
		
	SOC_ENUM_EXT("PA Loudness-F0 Select", loudness_f0_select_enum,
		get_loudness_f0, set_loudness_f0),
	SOC_ENUM_EXT("PA Mixing-CH1 Switch", mixing_channel1_enum,
		get_mixing_channel1_switch, set_mixing_channel1_switch),
	SOC_ENUM_EXT("PA Mixing-CH2 Switch", mixing_channel2_enum,
		get_mixing_channel2_switch, set_mixing_channel2_switch),
	SOC_ENUM_EXT("PA Mixing-Input Select", mixing_input_select_enum,
		get_mixing_input_select, set_mixing_input_select),

	SOC_ENUM_EXT("PA Sub-Input Select", sub_input_select_enum,
		get_sub_input_select, set_sub_input_select),
	SOC_ENUM_EXT("PA Sub-LPF-FC Select", sub_lpf_fc_enum,
		get_lpf_fc, set_lpf_fc),
	SOC_ENUM_EXT("PA Sub-LPF-Parse Select", sub_lpf_parse_enum,
		get_sub_lpf_parse, set_sub_lpf_parse),
	SOC_ENUM_EXT("PA Sub-Output Select", sub_output_select_enum,
		get_sub_output_select, set_sub_output_select),
};

static int bd37033_startup(struct snd_pcm_substream *substream,struct snd_soc_dai *dai)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	/* variable MCLK */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 0, SND_SOC_CLOCK_IN);

	return ret;
}

static int bd37033_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,struct snd_soc_dai *dai)
{
	void __iomem 	*Sys_base;
	unsigned int val;
	
	Sys_base = ioremap(0xe4900000, 0x1ff);

	//cancel pop noise
	/*val = readl(Sys_base + 0x6c);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		val &= ~(1 << 2);
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		val &= ~(1 << 0);
	}
	writel(val,Sys_base+0x6c);*/
	unsigned int rate = params_rate(params);
	//printk("==============[%s]:[ %d] rate = %d\n", __FUNCTION__, __LINE__,rate);
	switch (params_rate(params)) {
		case 192000:
			val = (768 << 16) | 1875;
			break;

		case 96000:
			val = (384 << 16) | 1875;
			break;
			
		case 48000:
			val = (192 << 16) | 1875;
		    break;
			
		case 44100:
			val = (1764 << 16) | 18750;
		    break;
			
		case 32000:
			val = (128 << 16) | 1875;
		    break;

		case 24000:
			val = (96 << 16) | 1875;
		    break;
			
		case 22050:
			val = (882 << 16) | 18750;
		    break;

		case 16000:
			val = (64 << 16) | 1875;
		    break;

		case 12000:
			val = (48 << 16) | 1875;
		    break;
			
		case 11025:
			val = (441 << 16) | 18750;
			break;

 		case 8000:
			val = (32 << 16) | 1875;
			break;

 		case 4000:
			val = (16 << 16) | 1875;
			break;
		default:
			val = (1764 << 16) | 18750;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		writel(val, Sys_base + 0x19c);
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		writel(val, Sys_base + 0x174);
	}
	iounmap(Sys_base);
	return 0;
}

#if 1
#define bd37033_RATES SNDRV_PCM_RATE_8000_96000

#define bd37033_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE)

//static int bd37033_probe(struct snd_soc_codec *codec)
static int bd37033_probe(struct snd_soc_component *component)
{
	return 0;
}

//static int bd37033_remove(struct snd_soc_codec *codec)
static void bd37033_remove(struct snd_soc_component *component)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
}

//static int bd37033_suspend(struct snd_soc_codec *codec)
static int bd37033_suspend(struct snd_soc_component *component)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	return 0;
}

//static int bd37033_resume(struct snd_soc_codec *codec)
static int bd37033_resume(struct snd_soc_component *component)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	return 0;
}

//static int bd37033_set_bias_level(struct snd_soc_codec *codec,
//					enum snd_soc_bias_level level)
static int  bd37033_set_bias_level(struct snd_soc_component *component,
				enum snd_soc_bias_level level)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	return 0;
}

static struct snd_soc_dai_ops  bd37033_dai_ops = {
	.startup	= bd37033_startup,
	.hw_params	= bd37033_hw_params,
};

static struct snd_soc_dai_driver bd37033_dai[]={
#if 1
	{
		.name = "bd37033-lineout",
		.playback = {
			.stream_name = "Lineout Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = bd37033_RATES,
			.formats = bd37033_FORMATS,
		},
		.ops = &bd37033_dai_ops,
	},
#else
	{
		.name = "bd37033-rlineout",
		.playback = {
			.stream_name = "RLineout Playback",//RL_OUT  RR_OUT
			.channels_min = 1,
			.channels_max = 2,
			.rates = bd37033_RATES,
			.formats = bd37033_FORMATS,
		},
		.ops = &bd37033_dai_ops,
	},
	{
		.name = "bd37033-flineout",
		.playback = {
			.stream_name = "FLineout Playback",//FL_OUT  FR_OUT
			.channels_min = 1,
			.channels_max = 2,
			.rates = bd37033_RATES,
			.formats = bd37033_FORMATS,
		},
		.ops = &bd37033_dai_ops,
	},
#endif
};

//static struct snd_soc_codec_driver soc_codec_dev_bd37033= {
static struct snd_soc_component_driver soc_component_dev_bd37033= {
	.probe =	bd37033_probe,
	.remove =	bd37033_remove,
	.suspend =	bd37033_suspend,
	.resume =	bd37033_resume,
	.set_bias_level = bd37033_set_bias_level,
	
	//.component_driver = {
		.controls = bd37033_controls,
		.num_controls = ARRAY_SIZE(bd37033_controls),
	//},
};
#endif

//static void clock_init(void)
//{
//	void __iomem *Sys_base;
//	unsigned int val;

//	//select 240M clk source
//	Sys_base = ioremap(0xe4900000, 0x1ff);
//	val = readl(Sys_base + 0x6c);
//	val &= ~((1 << 2) | (1 << 0));
//	writel(val,Sys_base+0x6c);
//	iounmap(Sys_base);
//}

static int bd37033_drv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{ //printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct bd37033_data *bd;
	int ret = -1;	

	//printk(KERN_INFO "==============>%s\n", __FUNCTION__);

	bd = devm_kzalloc(&client->dev, sizeof(struct bd37033_data), GFP_KERNEL);
	if (!bd)
		return -ENOMEM;

	bd->pd.reg_base			= I2S_BASE,
	bd->pd.reg_size			= 0xFF,
	bd->client 			= client;
	bd->id				= id;
	my_bd = bd;

	i2c_set_clientdata(client, bd);

	bd37033_init(bd);	//have to behind i2c_set_clientdata(client, bd);
	
//	ret =  snd_soc_register_codec(&client->dev,
//				&soc_codec_dev_bd37033, bd37033_dai,
//				ARRAY_SIZE(bd37033_dai));
	ret =  devm_snd_soc_register_component(&client->dev,
				&soc_component_dev_bd37033, bd37033_dai,
				ARRAY_SIZE(bd37033_dai));
	if (ret < 0) {
		dev_err(&client->dev, "Failed to register codec: %d\n", ret);
	}
	return ret;
	//printk(KERN_INFO "==============>%s end\n", __FUNCTION__);
}

static int bd37033_drv_remove(struct i2c_client *client)
{
 	struct bd37033_data *bd = i2c_get_clientdata(client);
	
	if (!bd)
	{
		dev_err(&client->dev, "%s: no device to remove\n",__FUNCTION__);
		return -ENODEV;
	}

	if(bd)
		kfree(bd);
	
   	i2c_set_clientdata(client, NULL);	
	
	return 0;
}

static void bd37033_drv_shutdown(struct i2c_client *client)
{
 	struct bd37033_data *bd = i2c_get_clientdata(client);

	if(bd)
	{
		if(bd37033_set_mute(bd,MUTE_ON) == 0)
			mute_status = MUTE_ON;
	}
}

static const struct i2c_device_id BD37033_drv_id[] = {
	{"drv_bd37033", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, BD37033_drv_id);

static const struct of_device_id BD37033_drv_of_match[] = {
	{ .compatible = "arkmicro,drv_bd37033" },
	{ }
};
MODULE_DEVICE_TABLE(of, BD37033_drv_of_match);


static struct i2c_driver drv_bd37033_driver = {
	.driver = {
		.name = "drv_bd37033",
		.of_match_table = BD37033_drv_of_match,
		.owner = THIS_MODULE,
	},
	.id_table = BD37033_drv_id,
	.probe = bd37033_drv_probe,
	.remove = bd37033_drv_remove,
	.shutdown = bd37033_drv_shutdown,
};

static int __init drv_bd37033_init(void)
{
	return i2c_add_driver(&drv_bd37033_driver);
}

static void __exit drv_bd37033_exit(void)
{
	i2c_del_driver(&drv_bd37033_driver);
}

module_init(drv_bd37033_init);
module_exit(drv_bd37033_exit);
MODULE_AUTHOR("n@n.com");
MODULE_DESCRIPTION("ark_drv");
MODULE_LICENSE("GPL");
