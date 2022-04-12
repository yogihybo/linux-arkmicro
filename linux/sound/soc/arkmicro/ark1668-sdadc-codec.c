/*
 * ARKADC - ark digital-to-analog converter
 *
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>

#define ARKADC_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000)

#define ARKADC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE)

static int ark_sdadc_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	}else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	}
	return 0 ;
}

static void ark_sdadc_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}

static int ark_sdadc_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int ark_sdadc_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int ark_sdadc_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static const struct snd_soc_dai_ops ark_sdadc_dai_ops = {
	.startup	=  ark_sdadc_startup,
	.shutdown	=  ark_sdadc_shutdown,
	.hw_params	=  ark_sdadc_hw_params,
	.set_sysclk	=  ark_sdadc_set_dai_sysclk,
	.set_fmt	=  ark_sdadc_set_dai_fmt,
};

static struct snd_soc_dai_driver ark_sdadc_dai = {
	.name 		= "ark-sdadc-codec",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ARKADC_RATES,
		.formats = ARKADC_FORMATS,
	},
	.ops = &ark_sdadc_dai_ops,
};

static int ark_sdadc_codec_probe(struct snd_soc_component *component)
{
	return 0;
}

static const struct snd_soc_dapm_widget ark_sdadc_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MICIN"),
	SND_SOC_DAPM_INPUT("RLINEIN"),
	SND_SOC_DAPM_INPUT("LLINEIN"),
};

static const struct snd_soc_component_driver ark_sdadc_component_driver = {
	.probe			= ark_sdadc_codec_probe,
	.dapm_widgets		= ark_sdadc_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ark_sdadc_dapm_widgets),
};

static int ark_sdadc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;
	printk("start:================[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	ret = devm_snd_soc_register_component(dev, &ark_sdadc_component_driver,
			&ark_sdadc_dai,
			1);
	if (ret) {
		dev_err(dev, "failed to register codec: %d\n", ret);
		return ret;
	}
	printk("end:===============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	return 0;
}

static int ark_sdadc_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id ark_sdadc_match[] = {
	{ .compatible = "arkmicro,ark1668-sdadc", },
	{},
};

static struct platform_driver ark_sdadc_driver = {
	.driver = {
		.name 	= "ark1668-sdadc",
		.of_match_table = of_match_ptr(ark_sdadc_match),
	},
	.probe 	= ark_sdadc_probe,
	.remove = ark_sdadc_remove,
};
module_platform_driver(ark_sdadc_driver);

MODULE_DESCRIPTION("ARK adc codec driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
