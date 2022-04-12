/*
 * ADC - Sigma-delta digital-to-analog converter 
 *
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "ark_i2s_sddac_regs.h"


#define  CODEC_CS5343

//#define  CODEC_RECOD


#undef SDDAC_DEBUG
#ifdef SDDAC_DEBUG
#define DBG(f, a...) 	printk(KERN_DEBUG "ASOC: %s-%d: "f, __FUNCTION__, __LINE__, ##a)
#else
#define DBG(...)
#endif

#define ERR(f, a...) 	printk(KERN_ERR "ASOC: %s-%d: "f, __FUNCTION__, __LINE__, ##a)


#define SDDAC_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000)

#define SDDAC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE)

struct cs5343_priv {
	struct device *dev;
	void __iomem	*base;
	struct clk *mclk;
	unsigned int 	vol_l;
	unsigned int 	vol_r;
};

static int cs5343_read(
	struct snd_soc_component *component, unsigned int reg)
{
	struct cs5343_priv *cs5343 = snd_soc_component_get_drvdata(component);

	return 0;
}

static int cs5343_write(
	struct snd_soc_component *component, unsigned int reg, unsigned int value)
{
	struct cs5343_priv *cs5343 = snd_soc_component_get_drvdata(component);

	return 0;
}


static int cs5343_mute(struct snd_soc_dai *dai, int mute)
{
	//struct snd_soc_codec *codec = dai->codec;

	DBG("-->%d---\n",mute);
	
	return 0;
}

static int cs5343_set_bias_level(
	struct snd_soc_component *component, enum snd_soc_bias_level level)
{
	
	DBG("-->%d--------\n",level);
	return 0;
}


static int cs5343_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	DBG("-->play init start---cs5343----------\n");
	DBG("-->play init end-------------\n");
	return 0 ;
}

static void cs5343_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	DBG("-->-------\n");
	//dump_stack();
	return 0;
}

static int cs5343_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	DBG("-->-------\n");
	return 0;
}


static int cs5343_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	DBG("-->-------\n");
	return 0;
}

static int cs5343_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	DBG("-->-------\n");
	return 0;
}

static const struct snd_soc_dai_ops cs5343_dai_ops = {
	.startup	=  cs5343_startup,
	.shutdown	=  cs5343_shutdown,
	.hw_params	=  cs5343_hw_params,
	.set_sysclk	=  cs5343_set_dai_sysclk,
	.set_fmt	=  cs5343_set_dai_fmt,
};


static struct snd_soc_dai_driver cs5343_dai = {
	.name 		= "cs5343_codec",
	/*.playback 	= {
		.stream_name 	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates			= SDDAC_RATES,
		.formats		= SDDAC_FORMATS,
	},*/
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SDDAC_RATES,
		.formats = SDDAC_FORMATS,
	},
	.ops 		= &cs5343_dai_ops,
};


static int cs5343_probe(struct snd_soc_component *component)
{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct cs5343_priv *cs5343;
	int ret = -1;

	DBG("-->\n");
//	cs5343->mclk = devm_clk_get(codec->dev, "cs5343_mclk");
//	if (PTR_ERR(cs5343->mclk) == -EPROBE_DEFER)
//		return -EPROBE_DEFER;
	
	
	cs5343 = kzalloc(sizeof(struct cs5343_priv), GFP_KERNEL);
	if (cs5343 == NULL)
		return -ENOMEM;

	snd_soc_component_set_drvdata(component, cs5343);
	return 0;

err_free_mem:

	return ret;
}

static void cs5343_remove(struct snd_soc_component *component)
{
	struct cs5343_priv *cs5343 = snd_soc_component_get_drvdata(component);

	DBG("-->\n");
	if (cs5343)
		kfree(cs5343);

	return 0;
}

static const struct snd_soc_dapm_widget cs5343_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MICIN"),
	SND_SOC_DAPM_INPUT("RLINEIN"),
	SND_SOC_DAPM_INPUT("LLINEIN"),
};

static const struct snd_soc_dapm_route cs5343_intercon[] = {
//	{"Line Input", NULL, "LLINEIN"},
//	{"Line Input", NULL, "RLINEIN"},
};

static struct snd_soc_component_driver soc_component_dev_cs5343 = {
	.probe 				= cs5343_probe,
	.remove				= cs5343_remove,
	//.read				= cs5343_read,
	//.write				= cs5343_write,
	//.set_bias_level 			= cs5343_set_bias_level,
	//.component_driver = {
		.dapm_widgets 		= cs5343_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(cs5343_dapm_widgets),
	//}
};

static int cs5343_codec_probe(struct platform_device *pdev)
{
	int ret;

	DBG("-->\n");
	ret = devm_snd_soc_register_component(&pdev->dev, 
		&soc_component_dev_cs5343, &cs5343_dai, 1);
	return ret;
}

static int cs5343_codec_remove(struct platform_device *pdev)
{
	DBG("-->\n");
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static const struct of_device_id ark1668e_cs5343_match[] = {
	{ .compatible = "arkmicro,ark1668e_cs5343_codec", },
	{},
};


static struct platform_driver cs5343_codec_driver = {
	.driver = {
		.name 	= "ark_cs5343_dev",
		.of_match_table = of_match_ptr(ark1668e_cs5343_match),
		//.owner 	= THIS_MODULE,
	},
	.probe 	= cs5343_codec_probe,
	.remove 	= cs5343_codec_remove,
};

module_platform_driver(cs5343_codec_driver);


MODULE_DESCRIPTION("ARK cs5343 codec driver");
MODULE_AUTHOR("jacktang <jacktang@astri.org>");
MODULE_LICENSE("GPL");

