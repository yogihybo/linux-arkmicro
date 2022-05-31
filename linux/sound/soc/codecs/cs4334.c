/*
 * DAC - Sigma-delta digital-to-analog converter 
 *
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>
//#include "cs4334.h"


#define  CODEC_CS4334

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

struct cs4334_priv {
	struct device *dev;
	void __iomem	*base;
	struct clk *mclk;
	unsigned int 	vol_l;
	unsigned int 	vol_r;
};

static int cs4334_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	DBG("-->play init start---cs4334----------\n");
	DBG("-->play init end-------------\n");
	return 0 ;
}

static void cs4334_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	DBG("-->-------\n");
}

static int cs4334_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	DBG("-->-------\n");
	return 0;
}


static int cs4334_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	DBG("-->-------\n");
	return 0;
}

static int cs4334_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	DBG("-->-------\n");
	return 0;
}

static const struct snd_soc_dai_ops cs4334_dai_ops = {
	.startup	=  cs4334_startup,
	.shutdown	=  cs4334_shutdown,
	.hw_params	=  cs4334_hw_params,
	.set_sysclk	=  cs4334_set_dai_sysclk,
	.set_fmt	=  cs4334_set_dai_fmt,
};


static struct snd_soc_dai_driver cs4334_dai = {
	.name 		= "cs4334",
	.playback 	= {
		.stream_name 	= "Playback",
		.channels_min	= 1,
		.channels_max	= 2,
		.rates			= SDDAC_RATES,
		.formats		= SDDAC_FORMATS,
	},
	.ops 		= &cs4334_dai_ops,
};


static int cs4334_probe(struct snd_soc_component *component)
{
	struct cs4334_priv *cs4334;

	DBG("-->\n");

	cs4334 = kzalloc(sizeof(struct cs4334_priv), GFP_KERNEL);
	if (cs4334 == NULL)
		return -ENOMEM;

	snd_soc_component_set_drvdata(component, cs4334);
	return 0;
}

static void cs4334_remove(struct snd_soc_component *component)
{
	struct cs4334_priv *cs4334 = snd_soc_component_get_drvdata(component);
	DBG("-->\n");
	if (cs4334)
		kfree(cs4334);

	return;
}

static const struct snd_soc_dapm_widget cs4334_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MICIN"),
	SND_SOC_DAPM_INPUT("RLINEIN"),
	SND_SOC_DAPM_INPUT("LLINEIN"),
};

static const struct snd_soc_dapm_route cs4334_intercon[] = {
//	{"Line Input", NULL, "LLINEIN"},
//	{"Line Input", NULL, "RLINEIN"},
};

static struct snd_soc_component_driver soc_component_dev_cs4334 = {
	.probe 				= cs4334_probe,
	.remove				= cs4334_remove,
	.dapm_widgets 		= cs4334_dapm_widgets,
	.num_dapm_widgets 	= ARRAY_SIZE(cs4334_dapm_widgets),
};

static int cs4334_codec_probe(struct platform_device *pdev)
{
	int ret;

	DBG("-->\n");
	ret = devm_snd_soc_register_component(&pdev->dev, 
		&soc_component_dev_cs4334, &cs4334_dai, 1);
	return ret;
}

static int cs4334_codec_remove(struct platform_device *pdev)
{
	DBG("-->\n");

	//snd_soc_unregister_codec(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}

static const struct of_device_id ark1668e_cs4334_match[] = {
	{ .compatible = "arkmicro,ark1668e_cs4334_codec", },
	{},
};


static struct platform_driver cs4334_codec_driver = {
	.driver = {
		.name 	= "ark_cs4334_dev",
		.of_match_table = of_match_ptr(ark1668e_cs4334_match),
		//.owner 	= THIS_MODULE,
	},
	.probe 	= cs4334_codec_probe,
	.remove 	= cs4334_codec_remove,
};

module_platform_driver(cs4334_codec_driver);


MODULE_DESCRIPTION("ARK cs4334 codec driver");
MODULE_AUTHOR("jacktang <jacktang@astri.org>");
MODULE_LICENSE("GPL");

