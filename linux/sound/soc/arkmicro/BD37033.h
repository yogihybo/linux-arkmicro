/*
 * Copyright(c) 2017 Arkmicro Science and Technology
 * all rights reserved
 * Proprietary and Confidential Information.
 *
 * This source file is the property of ASTRI, and may not be copied or
 * distributed in any isomorphic form without the prior written consent
 * of ASTRI.
 *
 * Name:
 *              BD37033.h
 *
 * Description:
 *              This file contains BD37033 register definitions
 *
 * Author:
 *              ark
 *
 * Remarks:
 *
 */

#ifndef _BD37033_H_
#define _BD37033_H_

#include <linux/platform_device.h>
#include <linux/i2c.h>

#define BD37033_I2C_NAME	"drv_bd37033"

//Selector Register Address
enum{
	SELECT_ADDR_INITIAL_SETUP 	= 0x01,
	SELECT_ADDR_LPF_SETUP		= 0x02,
	SELECT_ADDR_MIXING_SETUP	= 0x03,
	SELECT_ADDR_INPUT_SELECTOR	= 0x05,
	SELECT_ADDR_INPUT_GAIN		= 0x06,
	SELECT_ADDR_VOLUME_GAIN		= 0x20,
	SELECT_ADDR_FADER_1CH_FRONT	= 0x28,
	SELECT_ADDR_FADER_2CH_FRONT	= 0x29,
	SELECT_ADDR_FADER_1CH_REAR	= 0x2A,
	SELECT_ADDR_FADER_2CH_REAR	= 0x2B,
	SELECT_ADDR_FADER_1CH_SUB	= 0x2C,
	SELECT_ADDR_MIXINT_2CH_SUB	= 0x30,
	SELECT_ADDR_BASS_SETUP		= 0x41,
	SELECT_ADDR_MIDDLE_SETUP	= 0x44,
	SELECT_ADDR_TREBLE_SETUP	= 0x47,
	SELECT_ADDR_BASS_GAIN		= 0x51,
	SELECT_ADDR_MIDDLE_GAIN		= 0x54,
	SELECT_ADDR_TREBLE_GAIN		= 0x57,
	SELECT_ADDR_LOUDNESS_GAIN	= 0x75,
	SELECT_ADDR_SYSTEM_RESET	= 0xFE,
};

//Selector Register Data
typedef struct _select_data{
	unsigned char select_data_initial_setup;
	unsigned char select_data_lpf_setup;
	unsigned char select_data_mixing_setup;
	unsigned char select_data_input_selector;
	unsigned char select_data_input_gain;
	unsigned char select_data_volume_gain;
	unsigned char select_data_fader_1ch_front_gain;
	unsigned char select_data_fader_2ch_front_gain;
	unsigned char select_data_fader_1ch_rear_gain;
	unsigned char select_data_fader_2ch_rear_gain;
	unsigned char select_data_fader_1ch_sub_gain;
	unsigned char select_data_mixing_2ch_sub_gain;
	unsigned char select_data_bass_setup;
	unsigned char select_data_middle_setup;
	unsigned char select_data_treble_setup;
	unsigned char select_data_bass_gain;
	unsigned char select_data_middle_gain;
	unsigned char select_data_treble_gain;
	unsigned char select_data_loudness_gain;
	unsigned char select_data_system_reset;
}SELECT_DARA;

struct sddac_platform_data {
	unsigned int 	reg_base;
	unsigned int 	reg_size;
};

struct ark1668e_dac_gain{
	unsigned int 	vol_flout;
	unsigned int 	vol_frout;
	unsigned int 	vol_rlout;
	unsigned int 	vol_rrout;
};

struct bd37033_data{
    //spinlock_t spin_lock;
	struct i2c_client *client;
	struct i2c_device_id *id;
	struct sddac_platform_data pd;
	struct ark1668e_dac_gain  dac;
	//struct platform_device *bd37033_snd_device;
};

struct bd37033_module_param
{
	int mute;
	int input_channel;
	int volume;
	int input_gain;
	int fl_gain;
	int fr_gain;
	int rl_gain;
	int rr_gain;
	int bass_gain;
	int middle_gain;
	int treble_gain;
};


//Printk Level
#define KERN_LEVEL	KERN_ALERT

//Mute State
#define MUTE_OFF			0
#define MUTE_ON				1

//Subwoofer
#define BD37033_LPF_SETUP_SUB_LPF_FC_TYPE						0x0
#define BD37033_LPF_SETUP_SUB_INPUT_SELECT_TYPE					0x01
#define BD37033_LPF_SETUP_SUB_OUTPUT_SELECT_TYPE				0x02
#define BD37033_LPF_SETUP_LEVEL_METER_RESET_TYPE				0x03
#define BD37033_LPF_SETUP_LPF_PHASE_TYPE						0x04

//Mixing
#define BD37033_MIXING_1CH_SWITCH_TYPE							0x0
#define BD37033_MIXING_2CH_SWITCH_TYPE							0x01
#define BD37033_MIXING_LOUDNESS_F0_TYPE							0x02
#define BD37033_MIXING_INPUT_SELECT_TYPE						0x03

//Secect Input Channel
#define BD37033_INPUT_SELECTOR_A_SINGLE							0x0
#define BD37033_INPUT_SELECTOR_B_SINGLE							0x01
#define BD37033_INPUT_SELECTOR_C_SINGLE							0x02
#define BD37033_INPUT_SELECTOR_D_SINGLE							0x03
#define BD37033_INPUT_SELECTOR_E1_SINGLE						0x0A
#define BD37033_INPUT_SELECTOR_E2_SINGLE						0x0B
#define BD37033_INPUT_SELECTOR_D_DIFF							0x06
#define BD37033_INPUT_SELECTOR_E_FULL_DIFF						0x08
#define BD37033_INPUT_SELECTOR_INPUT_SHORT						0x09
#define BD37033_INPUT_SELECTOR_FULL_DIFF_TYPE_NEGATIVE_INPUT	0x0f
#define BD37033_INPUT_SELECTOR_FULL_DIFF_TYPE_BIAS				0x1f

#define BD37033_STREAM_A_SINGLE									0x0
#define BD37033_STREAM_B_SINGLE		  	    					0x01
#define BD37033_STREAM_C_SINGLE		       						0x02
#define BD37033_STREAM_D_SINGLE      							0x03
#define BD37033_STREAM_E1_SINGLE       							0x04
#define BD37033_STREAM_E2_SINGL       							0x05
#define BD37033_STREAM_D_DIF       								0x06
#define BD37033_STREAM_E_FULL_DIFF       						0x07
#define BD37033_STREAM_INPUT_SHORT								0x08

//Tone
#define BASS_Q				0x10
#define BASS_F0				0x11
#define MIDDLE_Q			0x12
#define MIDDLE_F0			0x13
#define TREBLE_Q			0x14
#define TREBLE_F0			0x15

#define I2S_BASE				0xE4000000
#endif

