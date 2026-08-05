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

/* Single active DAC instance on this board -- needed so ark_audio_mute()
 * below can be called from ark1668_i2s.c without a struct snd_soc_dai/
 * component handle, matching how stock's own ark_audio_mute() (a real,
 * EXPORT_SYMBOL'd function, confirmed via disassembly of the real stock
 * vmlinux -- see docs/AUDIO_SUBSYSTEM_INVESTIGATION.md) is called directly
 * from stock's custom ark_pcm_trigger(), not through ASoC's automatic
 * .digital_mute dispatch. */
static struct ark_sddac *g_ark_sddac;

static int ark_sddac_get_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

	/* 2026-07-28: was a no-op stub, same bug class as ark_sddac_mute
	 * before its 2026-07-18 fix. Confirmed via disassembly of stock's
	 * real vmlinux (sddac_get_l_playback_volume @ 0x802f63c4): reads
	 * back the cached software value (dac->vol_l), not a live register
	 * read -- exactly what this commented-out body already did.
	 */
	ucontrol->value.integer.value[0] = dac->vol_l & 0x7f;
	printk(KERN_INFO "ark1668-sddac: get_l_playback_volume = %ld at %lluns\n",
	       ucontrol->value.integer.value[0], ktime_to_ns(ktime_get()));
	return 0;
}

static int ark_sddac_set_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
	unsigned int val = readl(dac->base + I2S_DACR0);

	/* 2026-07-28: was a no-op stub. Confirmed via disassembly of
	 * stock's real vmlinux (sddac_set_l_playback_volume @ 0x802f63ec):
	 * caches the new value, then read-modify-writes I2S_DACR0 with the
	 * L-channel field replaced (bits [6:0]) while preserving R's field
	 * (bits [14:8]) -- byte-exact match for DACR0_LVOL_MASK/
	 * DACR0_LVOL() already defined in ark_i2s.h and already used by
	 * ark_sddac_mute's real implementation. This is the same register
	 * ark_sddac_mute writes -- see that function's comment for the
	 * broader implication of a volume-set landing here concurrently
	 * with a mute/unmute.
	 */
	dac->vol_l = ucontrol->value.integer.value[0];
	val &= ~DACR0_LVOL_MASK;
	val |= DACR0_LVOL(dac->vol_l);
	writel(val, dac->base + I2S_DACR0);

	printk(KERN_INFO "ark1668-sddac: set_l_playback_volume = %d (I2S_DACR0=0x%x) at %lluns\n",
	       dac->vol_l, val, ktime_to_ns(ktime_get()));
	return 0;
}

static int ark_sddac_get_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = dac->vol_r & 0x7f;
	printk(KERN_INFO "ark1668-sddac: get_r_playback_volume = %ld at %lluns\n",
	       ucontrol->value.integer.value[0], ktime_to_ns(ktime_get()));
	return 0;
}

static int ark_sddac_set_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
	unsigned int val = readl(dac->base + I2S_DACR0);

	dac->vol_r = ucontrol->value.integer.value[0];
	val &= ~DACR0_RVOL_MASK;
	val |= DACR0_RVOL(dac->vol_r);
	writel(val, dac->base + I2S_DACR0);

	printk(KERN_INFO "ark1668-sddac: set_r_playback_volume = %d (I2S_DACR0=0x%x) at %lluns\n",
	       dac->vol_r, val, ktime_to_ns(ktime_get()));
	return 0;
}

static const struct snd_kcontrol_new  ark_sddac_snd_controls[] = {
	/* DAC volume control. Stock's real kcontrol names (confirmed via
	 * strings on stock's vmlinux) are "Left Playback Volume"/"Right
	 * Playback Volume" -- no "2" suffix. Renamed to match: if
	 * userspace (SoftVolCtrl, or any amixer-name-based lookup) was
	 * ever trying to reach these by stock's real name, the mismatched
	 * "2" suffix would make that lookup silently fail today.
	 */
	SOC_SINGLE_EXT("Left Playback Volume", 0, 0, 127, 0,
			ark_sddac_get_l_playback_volume, ark_sddac_set_l_playback_volume),
	SOC_SINGLE_EXT("Right Playback Volume", 0, 0, 127, 0,
			ark_sddac_get_r_playback_volume, ark_sddac_set_r_playback_volume),
};

static const struct snd_soc_dapm_widget ark_sddac_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),
};

static int ark_sddac_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
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

/* ark_audio_mute() -- the REAL, hardware-active mute, matching stock's
 * own exported function of the same name (confirmed via disassembly of
 * the real stock vmlinux.elf, 2026-07-30: EXPORT_SYMBOL'd, called
 * explicitly from stock's custom ark_pcm_trigger() with a mute -> 2.5ms
 * delay -> unmute sequence around every DMA cyclic start/stop, NOT from
 * ASoC's automatic .digital_mute dispatch -- see docs/
 * AUDIO_SUBSYSTEM_INVESTIGATION.md for the full comparison). Exported so
 * ark1668_i2s.c's trigger function can call it directly with the same
 * timing stock uses, tied to actual DMA readiness instead of ASoC's own
 * internal (and XRUN-recovery-storm-prone) trigger/DAPM scheduling. */
void ark_audio_mute(int mute)
{
	struct ark_sddac *dac = g_ark_sddac;

	if (!dac)
		return;

	pr_debug("ark1668-sddac: ark_audio_mute mute=%d at %lluns\n",
		 mute, ktime_to_ns(ktime_get()));

	if (mute)
		writel(0, dac->base + I2S_DACR0);
	else
		writel(DACR0_RVOL(dac->vol_r) | DACR0_LVOL(dac->vol_l), dac->base + I2S_DACR0);
}
EXPORT_SYMBOL(ark_audio_mute);

static int ark_sddac_mute(struct snd_soc_dai *dai, int mute)
{
	/* 2026-07-30: confirmed via disassembly of the real stock vmlinux
	 * that stock's own .digital_mute callback (sddac_mute) is a genuine
	 * no-op -- it touches no hardware. The real, hardware-active mute
	 * on stock is a *separate* exported function (ark_audio_mute,
	 * above) called explicitly from stock's custom PCM trigger with a
	 * deliberate 2.5ms settle delay, not from this automatic ASoC
	 * callback. Matching that here: this function now only logs (kept
	 * for tracing how often ASoC's own internal machinery attempts a
	 * digital_mute, for comparison against the explicit
	 * ark_audio_mute() calls below) and does not touch the register --
	 * previously this function itself did the real write, meaning the
	 * mute was being driven by ASoC's own trigger/DAPM timing instead
	 * of by DMA readiness, with no settle delay at all. See
	 * docs/AUDIO_SUBSYSTEM_INVESTIGATION.md. */
	pr_debug("ark1668-sddac: digital_mute (ASoC-driven, no-op, matches stock) mute=%d at %lluns\n",
		 mute, ktime_to_ns(ktime_get()));
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

	g_ark_sddac = dac;

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

