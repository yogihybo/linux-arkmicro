/*
 * ARKDAC - ark digital-to-analog converter
 *
 */
#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#include "ark_i2s.h"

#define ARKDAC_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 |SNDRV_PCM_RATE_8000)

#define ARKDAC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE)

struct ark_sddac {
	void __iomem	*base;
	unsigned int 	vol_l;
	unsigned int 	vol_r;
};

static int ark_sddac_get_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
//	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
//	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

//	ucontrol->value.integer.value[0] = dac->vol_l & 0x7f;
//	//printk("get_l_playback_volume = %ld\n",ucontrol->value.integer.value[0]);
	/* 2026-07-28 AA audio-stutter investigation: this get/set pair is a
	 * no-op stub (see the whole function body commented out above) --
	 * logging entry so it's visible in dmesg if/when MsnCoreApp/sink
	 * ever actually calls it, since right now there's no way to tell.
	 */
	printk(KERN_INFO "ark1668-sddac: get_l_playback_volume called at %lluns\n",
	       ktime_to_ns(ktime_get()));
	return 0;
}

static int ark_sddac_set_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
//	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
//	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
//	unsigned int val = readl(dac->base + I2S_DACR0);
//	dac->vol_l = ucontrol->value.integer.value[0];
//	//printk("set_l_playback_volume = %d\n",dac->vol_l);
//	val &= ~DACR0_LVOL_MASK;
//	val |= DACR0_LVOL(dac->vol_l);
//	//printk("new_l_playback_volume = 0x%x\n",DACR0_LVOL(dac->vol_l));
//	writel(val, dac->base + I2S_DACR0);

	printk(KERN_INFO "ark1668-sddac: set_l_playback_volume requested=%ld (NO-OP, has no hardware effect) at %lluns\n",
	       ucontrol->value.integer.value[0], ktime_to_ns(ktime_get()));
	return 0;
}

static int ark_sddac_get_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
//	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
//	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

//	ucontrol->value.integer.value[0] = dac->vol_r & 0x7f;
//	//printk("get_r_playback_volume = %ld\n",ucontrol->value.integer.value[0]);
	printk(KERN_INFO "ark1668-sddac: get_r_playback_volume called at %lluns\n",
	       ktime_to_ns(ktime_get()));
	return 0;
}

static int ark_sddac_set_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
//	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
//	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
//	unsigned int val = readl(dac->base + I2S_DACR0);
//	dac->vol_r = ucontrol->value.integer.value[0];
//	//printk("set_r_playback_volume = %d\n",dac->vol_r);
//	val &= ~DACR0_RVOL_MASK;
//	val |= DACR0_RVOL(dac->vol_r);
//	//printk("new_r_playback_volume = 0x%x\n",DACR0_RVOL(dac->vol_r));
//	writel(val, dac->base + I2S_DACR0);

	printk(KERN_INFO "ark1668-sddac: set_r_playback_volume requested=%ld (NO-OP, has no hardware effect) at %lluns\n",
	       ucontrol->value.integer.value[0], ktime_to_ns(ktime_get()));
	return 0;
}

static const struct snd_kcontrol_new  ark_sddac_snd_controls[] = {
	/* DAC volume control */
	SOC_SINGLE_EXT("Left Playback Volume 2", 0, 0, 127, 0,
			ark_sddac_get_l_playback_volume, ark_sddac_set_l_playback_volume),
	SOC_SINGLE_EXT("Right Playback Volume 2", 0, 0, 127, 0,
			ark_sddac_get_r_playback_volume, ark_sddac_set_r_playback_volume),
};

static const struct snd_soc_dapm_widget ark_sddac_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),
};

static int ark_sddac_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	}else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	}
	return 0 ;
}

static void ark_sddac_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}

static int ark_sddac_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int ark_sddac_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_component *component = dai->component;
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

	/* Was a no-op stub -- confirmed via stock's own boot log
	 * (2026-07-18, "ark_audio_mute" lines) that real hardware writes
	 * this register on every mute/unmute: 0 to mute, the configured
	 * L/R gain to unmute. Same expression ark_sddac_codec_probe()
	 * already uses at init; DACR0_LVOL/DACR0_RVOL (ark_i2s.h) already
	 * had the correct real-hardware bit layout, this function just
	 * never called them. */

	/* 2026-07-28 AA audio-stutter investigation
	 * (docs/AUDIO_SUBSYSTEM_INVESTIGATION.md): this is a REAL,
	 * hardware-active mute -- ASoC's core calls .digital_mute
	 * automatically around stream trigger/prepare transitions (and
	 * potentially DAPM power sequencing). If something is toggling
	 * this more often than expected during otherwise-continuous
	 * playback, that alone would produce an audible click/dropout
	 * completely independent of ALSA's digital buffer health -- which
	 * would explain why SND_PCM_XRUN_DEBUG found nothing. Rare enough
	 * event in normal operation to log unconditionally.
	 */
	printk(KERN_INFO "ark1668-sddac: digital_mute mute=%d at %lluns\n",
	       mute, ktime_to_ns(ktime_get()));

	if (mute)
		writel(0, dac->base + I2S_DACR0);
	else
		writel(DACR0_RVOL(dac->vol_r) | DACR0_LVOL(dac->vol_l), dac->base + I2S_DACR0);

	return 0;
}

static int ark_sddac_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int ark_sddac_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static const struct snd_soc_dai_ops ark_sddac_dai_ops = {
	.startup	=  ark_sddac_startup,
	.shutdown	=  ark_sddac_shutdown,
	.hw_params	=  ark_sddac_hw_params,
	.set_sysclk	=  ark_sddac_set_dai_sysclk,
	.set_fmt	=  ark_sddac_set_dai_fmt,
	.digital_mute	= ark_sddac_mute,
};


static struct snd_soc_dai_driver ark_sddac_dai = {
	.name 		= "ark-sddac-codec",
	.playback 	= {
			.stream_name 	= "Playback",
			.channels_min	= 1,
			.channels_max	= 2,
			.rates			= ARKDAC_RATES,
			.formats		= ARKDAC_FORMATS,
	},
	.ops 		= &ark_sddac_dai_ops,
};

static int ark_sddac_codec_probe(struct snd_soc_component *component)
{
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
	int ret = 0;

	writel(DACR0_RVOL(dac->vol_r) | DACR0_LVOL(dac->vol_l), dac->base + I2S_DACR0);

	return ret;
}

static const struct snd_soc_component_driver ark_sddac_component_driver = {
	.probe			= ark_sddac_codec_probe,
	.controls		= ark_sddac_snd_controls,
	.num_controls		= ARRAY_SIZE(ark_sddac_snd_controls),
	.dapm_widgets		= ark_sddac_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ark_sddac_dapm_widgets),
};

static int ark_sddac_probe(struct platform_device *pdev)
{
	struct ark_sddac *dac;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;
	printk("start:================[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	dac = devm_kzalloc(dev, sizeof(*dac), GFP_KERNEL);
	if (!dac)
		return -ENOMEM;

	if (of_property_read_u32(pdev->dev.of_node, "left-volume", &dac->vol_l))
		dac->vol_l = 100;

	if (of_property_read_u32(pdev->dev.of_node, "right-volume", &dac->vol_r))
		dac->vol_r = 100;

	platform_set_drvdata(pdev, dac);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dac->base = ioremap(res->start, resource_size(res));
	if (IS_ERR(dac->base))
		return PTR_ERR(dac->base);

	ret = devm_snd_soc_register_component(dev, &ark_sddac_component_driver,
			&ark_sddac_dai,
			1);
	if (ret) {
		dev_err(dev, "failed to register codec: %d\n", ret);
		goto err;
	}
	printk("end:===============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	return 0;

err:
	iounmap(dac->base);
	return ret;
}

static int ark_sddac_remove(struct platform_device *pdev)
{
	struct ark_sddac *dac = dev_get_drvdata(&pdev->dev);

	if (dac->base)
		iounmap(dac->base);

	return 0;
}

static const struct of_device_id ark_sddac_match[] = {
	{ .compatible = "arkmicro,ark1668-sddac", },
	{},
};

static struct platform_driver ark_sddac_driver = {
	.driver = {
		.name 	= "ark1668-sddac",
		.of_match_table = of_match_ptr(ark_sddac_match),
	},
	.probe 	= ark_sddac_probe,
	.remove = ark_sddac_remove,
};
module_platform_driver(ark_sddac_driver);

MODULE_DESCRIPTION("ARK dac codec driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");

