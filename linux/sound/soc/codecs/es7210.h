/* 
* ALSA SoC ES7210 adc driver 
*/

#ifndef _ES7210_H
#define _ES7210_H

#define ENABLE		1
#define DISABLE		0

#define MIC_CHN_16	16
#define MIC_CHN_14	14
#define MIC_CHN_12	12
#define MIC_CHN_10	10
#define MIC_CHN_8	8
#define MIC_CHN_6	6
#define MIC_CHN_4	4
#define MIC_CHN_2	2

#define ES7210_TDM_ENABLE	ENABLE
#define ES7210_CHANNELS_MAX	MIC_CHN_4

#define ES7210_MCLK_CTL_REG02       0x02
#define ES7210_MODE_CFG_REG08       0x08
#define ES7210_SDP_CFG1_REG11       0x11
#define ES7210_SDP_CFG2_REG12       0x12
#define ES7210_ADC34_MUTE_REG14       0x14
#define ES7210_ADC12_MUTE_REG15       0x15

#define ES7210_TDM_1LRCK_DSPA                 0
#define ES7210_TDM_1LRCK_DSPB                 1
#define ES7210_TDM_1LRCK_I2S                  2
#define ES7210_TDM_1LRCK_LJ                   3
#define ES7210_TDM_NLRCK_DSPA                 4
#define ES7210_TDM_NLRCK_DSPB                 5
#define ES7210_TDM_NLRCK_I2S                  6
#define ES7210_TDM_NLRCK_LJ                   7

#define ES7210_WORK_MODE    ES7210_TDM_NLRCK_I2S


#define ES7210_I2C_BUS_NUM 		0
#define ES7210_CODEC_RW_TEST_EN		0
#define ES7210_IDLE_RESET_EN		1	//reset ES7210 when in idle time
#define ES7210_MATCH_DTS_EN		1	//ES7210 match method select: 0: i2c_detect, 1:of_device_id

#if ES7210_CHANNELS_MAX == MIC_CHN_2
	#define ADC_DEV_MAXNUM	1
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_4
        #define ADC_DEV_MAXNUM  1
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_6
        #define ADC_DEV_MAXNUM  2
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_8
        #define ADC_DEV_MAXNUM  2
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_10
        #define ADC_DEV_MAXNUM  3
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_12
        #define ADC_DEV_MAXNUM  3
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_14
        #define ADC_DEV_MAXNUM  4
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_16
        #define ADC_DEV_MAXNUM  4
#endif
#endif