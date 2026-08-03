/*
 * ARKADC - ark digital-to-analog converter
 *
 * 2026-08-02 investigation note: this file was suspected to be an
 * unfixed no-op stub analogous to ark1668-sddac-codec.c before its
 * 2026-07-18/07-28 fixes (which added real I2S_DACR0 register writes to
 * ark_sddac_mute/ark_sddac_set_l_r_playback_volume). That turned out NOT
 * to be the case here -- see the per-function notes below for the
 * evidence. No register-writing code was added to this file as a result.
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

/* ark_sdadc_startup/shutdown/hw_params/set_dai_sysclk/set_dai_fmt/
 * ark_sdadc_codec_probe -- confirmed via disassembly of the real stock
 * vmlinux.elf (2026-08-02) that ALL of these genuinely do no register
 * I/O in stock either, not just here. Stock's real equivalent of this
 * driver is named "cs4334" in its own symbol table (cs4334_startup @
 * 0x802f6604, cs4334_shutdown @ 0x802f660c, cs4334_hw_params @
 * 0x802f6610, cs4334_set_dai_sysclk @ 0x802f6618, cs4334_set_dai_fmt @
 * 0x802f6620, cs4334_codec_probe @ 0x802f663c) -- NOT the external SPI
 * "Voice Processor" chip from the schematic (already ruled out, see
 * docs/AUDIO_SUBSYSTEM_INVESTIGATION.md's "Voice Processor chip absent"
 * section) and not a real Cirrus Logic part either (already ruled out
 * for the DAC side, see that doc's "CS4334 pivot reverted" section) --
 * the name is just what stock's build system called this internal
 * ADC-codec source file/symbol set. Its DAPM widget table
 * (cs4334_dapm_widgets @ 0x80458be4) points at the same "MICIN"/
 * "RLINEIN"/"LLINEIN" rodata strings (0x80533334/0x8053333c/0x80533344)
 * that ark_sdadc_dapm_widgets[] below already declares, confirming this
 * is the right stock function set to compare against.
 *
 * Every one of cs4334_startup/shutdown/hw_params/set_dai_sysclk/
 * set_dai_fmt disassembles to nothing but `mov r0, #0; bx lr` (or, for
 * shutdown, just `bx lr`) -- no MMIO access whatsoever. Unlike
 * sddac_codec_probe (which writes I2S_DACR0 directly) and unlike
 * sddac_read/sddac_write (which exist as real register-access helpers
 * for the DAC side), there is no cs4334_read/cs4334_write pair anywhere
 * in stock -- this codec has no register-access helpers to call in the
 * first place. cs4334_codec_probe only calls snd_soc_register_codec();
 * it never touches hardware.
 *
 * This means the no-op stubs already in this file are not an unfixed
 * bug of the same class as the DAC side's -- they're a byte-accurate
 * match for stock's real behavior. The real hardware setup for capture
 * (SARADC power-up, MIC_LINE_SEL routing, RX DMA enable, etc., all in
 * the ARK_I2SSDDAC_SACR0/SACR1/SAIMR/SAICR registers) lives in the CPU
 * DAI driver instead (ark1668_i2s.c's ark_i2s_startup(), capture
 * branch) -- that side has already been substantially reconstructed in
 * this project (RDMAENA/MIC_LINE_SEL re-enabled, see that file's
 * history). One open item found during this pass: neither stock's real
 * ark_i2s_init_cfg() (0x802f5b4c, disassembled in full) nor this codec
 * ever writes I2S_ADCR0 (mic/line gain, sample-rate filter, and mic-
 * enhance bits, offset 0x1C in the same ARK_I2SSDDAC_* register block --
 * already-defined ADCR0_LVOL/RVOL/LFS/RFS/LME/RME macros exist in
 * ark_i2s.h but are unused by any compiled-in driver). Since stock does
 * not write it either, leaving it at hardware reset default is NOT a
 * deviation from stock and is not fixed here -- but it does mean mic
 * gain/enhance sits at whatever reset leaves it (register layout implies
 * 0, i.e. minimum gain, filters at 1/4 rate, mic-enhance off), which is
 * worth keeping in mind if capture is present but very quiet rather than
 * fully silent. That would be a real hardware/gain-staging question, not
 * a missing register write stock itself has -- deliberately NOT
 * "fixed" here since inventing a gain value stock never used would be
 * exactly the unverified-fix this project's norms warn against.
 */
static int ark_sdadc_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
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

	ret = devm_snd_soc_register_component(dev, &ark_sdadc_component_driver,
			&ark_sdadc_dai,
			1);
	if (ret) {
		dev_err(dev, "failed to register codec: %d\n", ret);
		return ret;
	}
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
