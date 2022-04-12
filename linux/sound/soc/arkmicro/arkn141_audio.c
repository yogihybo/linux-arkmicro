/*
 * ark_sddac.c -- ALSA SoC audio for ark
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/clk.h>

#define ARK_OUT_HP				0
#define ARK_OUT_SPK				1

#define ARK_STREAM_AVIN1			0
#define ARK_STREAM_AVIN2			1
#define ARK_STREAM_PLATFORM			2
#define ARK_STREAM_BLUETOOTH		3

static int ark_ir_ch_sel = 0;
static int ark_ir_en_sel = 0;
static int ark_out_sel = ARK_OUT_SPK;
static int ark_stream_sel = ARK_STREAM_PLATFORM;
static int ark_amp_volume = 80;
static int ark_fm_freq = 765;
static int ark_pa_mute = 0;

static int ark_startup(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	/* variable MCLK */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 0, SND_SOC_CLOCK_IN);

	return ret;
}

#define SYS_BASE			0x40408000
#define DAC_I2S_NCO_CFG		0x16c
#define ADC_I2S_NCO_CFG		0x15c
#define I2S_CLK_FREQ		240000000
static int ark_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
	unsigned int val;
	void *sysreg;
	u32 rate = params_rate(params);
	u32 step, modulo = 18750;

	step =  rate / (I2S_CLK_FREQ / 256 / 2 / modulo);
	val = (step << 16) | modulo;
	sysreg = ioremap(SYS_BASE, 0x1000);
	if (!sysreg) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			writel(val, sysreg + DAC_I2S_NCO_CFG);
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			writel(val, sysreg + ADC_I2S_NCO_CFG);
		}
		iounmap(sysreg);
	}

	return 0;
}

static struct snd_soc_ops ark_ops = {
	.startup	= ark_startup,
	.hw_params	= ark_hw_params,
};

static int ark_get_output (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_out_sel;
	return 0;
}

static int ark_set_output (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_out_sel == ucontrol->value.integer.value[0])
		return 0;

	ark_out_sel = ucontrol->value.integer.value[0];

	return 0;
}

static int ark_get_stream (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_stream_sel;
	return 0;
}

static int ark_set_stream (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_stream_sel == ucontrol->value.integer.value[0])
		return 0;

	ark_stream_sel = ucontrol->value.integer.value[0];

	return 0;
}

static int ark_get_ir_status (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_ir_en_sel;
	return 0;
}

static int ark_set_ir_status (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_ir_en_sel == ucontrol->value.integer.value[0])
		return 0;

	ark_ir_en_sel = ucontrol->value.integer.value[0];

	return 0;
}

static int ark_get_ir_channel (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_ir_ch_sel;
	return 0;
}

static int ark_set_ir_channel (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_ir_en_sel == 0 || ark_ir_ch_sel == ucontrol->value.integer.value[0])
		return 0;

	ark_ir_ch_sel = ucontrol->value.integer.value[0];

	return 0;
}

static int ark_get_pa_mute_status (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_pa_mute;
	return 0;
}

static int ark_set_pa_mute_status (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_pa_mute == ucontrol->value.integer.value[0])
		return 0;

	ark_pa_mute = ucontrol->value.integer.value[0];

	return 0;
}

static int ark_get_amp_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_amp_volume;
	return 0;
}

static int ark_set_amp_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_amp_volume == ucontrol->value.integer.value[0])
		return 0;

	ark_amp_volume = ucontrol->value.integer.value[0];

	return 0;
}

static int ark_get_fm_freq (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	ucontrol->value.integer.value[0] = ark_fm_freq;
	return 0;
}
extern void fm_set_transfer_rate(int freq);
static int ark_set_fm_freq (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	//struct snd_soc_card *card =  snd_kcontrol_chip(kcontrol);

	if (ark_fm_freq == ucontrol->value.integer.value[0])
		return 0;

	ark_fm_freq = ucontrol->value.integer.value[0];

	return 0;
}

static const char *stream_select[] = {
	"AVIN1",
	"AVIN2",
	"Platform",
	"Bluetooth"
};

static const char *out_select[] = {
	"Headphone Jack",
	"Ext Spk",
};

static const char *ir_status_select[] = {
	"IR_Off",
	"IR_On",
};

static const char *ir_ch_select[] = {
	"IR_ch0",
	"IR_ch1",
};

static const char *pa_mute_switch[] = {
	"PA_Mute_Off",
	"PA_Mute_On",
};

static const struct soc_enum ark_stream_sel_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(stream_select), stream_select);
static const struct soc_enum ark_out_sel_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(out_select), out_select);
static const struct soc_enum ark_ir_status_sel_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ir_status_select), ir_status_select);
static const struct soc_enum ark_ir_ch_sel_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(ir_ch_select), ir_ch_select);
static const struct soc_enum ark_pa_mute_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pa_mute_switch), pa_mute_switch);

static const struct snd_soc_dapm_widget ark_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
};

static const struct snd_soc_dapm_route ark_audio_map[] = {
	{"Headphone Jack", NULL, "LOUT"},
	{"Headphone Jack", NULL, "ROUT"},
	{"Ext Spk", NULL, "ROUT"},
	{"Ext Spk", NULL, "LOUT"},
};

static const struct snd_kcontrol_new ark_controls[] = {
	SOC_ENUM_EXT("Stream Select", ark_stream_sel_enum,
			ark_get_stream, ark_set_stream),
	SOC_ENUM_EXT("Output Select", ark_out_sel_enum,
			ark_get_output, ark_set_output),
	SOC_SINGLE_EXT("AMP Volume", 0, 0, 99, 0,
			ark_get_amp_volume, ark_set_amp_volume),
	SOC_SINGLE_EXT("FM Freq", 0, 0, 1272, 0,
			ark_get_fm_freq, ark_set_fm_freq),
	SOC_ENUM_EXT("IR Status", ark_ir_status_sel_enum,
			ark_get_ir_status, ark_set_ir_status),
	SOC_ENUM_EXT("IR Channel", ark_ir_ch_sel_enum,
			ark_get_ir_channel, ark_set_ir_channel),
	SOC_ENUM_EXT("PA Mute", ark_pa_mute_switch_enum,
			ark_get_pa_mute_status, ark_set_pa_mute_status),
};

static struct snd_soc_dai_link ark_dai[] = {
	{
		.name 			= "sddac",
		.stream_name 	= "sddac",
		.codec_dai_name	= "ark-sddac-codec",
		.cpu_dai_name	= "ark-i2s-dac",
		.codec_name 	= "ark-sddac",
		.ops 			= &ark_ops,
		.dai_fmt 		= SND_SOC_DAIFMT_I2S,
	},
	{
		.name 			= "sdadc",
		.stream_name 	= "sdadc",
		.codec_dai_name	= "ark-sdadc-codec",
		.cpu_dai_name	= "ark-i2s-adc",
		.codec_name 	= "ark-sdadc",
		.ops 			= &ark_ops,
		.dai_fmt 		= SND_SOC_DAIFMT_I2S,
	},
#if 0
	{
		.name 			= "es8156",
		.stream_name 	= "es8156 "
		.codec_dai_name	= "es8156-hifi"//es8156 dai_driver
		//cpu_dai_name	= "ark-i2s-dac",
		.codec_name 	= "es8156",//i2c_driver name
		.ops 			= &ark_ops,
		.dai_fmt 		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBS_CFS,
	},
	{
		.name 			= "es7243e ",
		.stream_name 	= "es7243e "
		.codec_dai_name	= "es7243e-hifi",//es7243e dai_driver
		//cpu_dai_name	= "ark-i2s-dac",
		.codec_name 	= "es7243e",//i2c_driver name
		.ops 			= &ark_ops,
		.dai_fmt 		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBS_CFS,
	},
#endif
};

static struct snd_soc_dai_link ark_dai_es7243e[] = {
		.name 			= "es7243e ",
		.stream_name 	= "es7243e "
		.codec_dai_name	= "es7243e-hifi",//es7243e dai_driver
		//cpu_dai_name	= "ark-i2s-dac",
		.codec_name 	= "es7243e",//i2c_driver name
		.ops 			= &ark_ops,
		.dai_fmt 		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBS_CFS,
};

static struct snd_soc_dai_link ark_dai_es8156[] = {
		.name 			= "es8156",
		.stream_name 	= "es8156 "
		.codec_dai_name	= "es8156-hifi"//es8156 dai_driver
		//cpu_dai_name	= "ark-i2s-dac",
		.codec_name 	= "es8156",//i2c_driver name
		.ops 			= &ark_ops,
		.dai_fmt 		= SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF
				| SND_SOC_DAIFMT_CBS_CFS,
};

static struct snd_soc_card arkn141_audio = {
	{
		.name 			= "arkn141-audio",
		.owner			= THIS_MODULE,
		.dai_link 		= ark_dai,
		.num_links 		= ARRAY_SIZE(ark_dai),

		.controls = ark_controls,
		.num_controls = ARRAY_SIZE(ark_controls),
		.dapm_widgets =  ark_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(ark_dapm_widgets),
		.dapm_routes =  ark_audio_map,
		.num_dapm_routes = ARRAY_SIZE(ark_audio_map),
	}
#if 1
	{
		.name 			= "es8156",/* proc/asound/cards */
		.owner			= THIS_MODULE,
		.dai_link 		= ark_dai_es8156,
		.num_links 		= ARRAY_SIZE(ark_dai_es8156),

		.controls = ark_controls,
		.num_controls = ARRAY_SIZE(ark_controls),
		.dapm_widgets =  ark_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(ark_dapm_widgets),
		.dapm_routes =  ark_audio_map,
		.num_dapm_routes = ARRAY_SIZE(ark_audio_map),
	}
	{
		.name 			= "es7243e",/* proc/asound/cards */
		.owner			= THIS_MODULE,
		.dai_link 		= ark_dai_es7243e,
		.num_links 		= ARRAY_SIZE(ark_dai_es7243e),

		.controls = ark_controls,
		.num_controls = ARRAY_SIZE(ark_controls),
		.dapm_widgets =  ark_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(ark_dapm_widgets),
		.dapm_routes =  ark_audio_map,
		.num_dapm_routes = ARRAY_SIZE(ark_audio_map),
	}
#endif
};

static int arkn141_audio_probe(struct platform_device *pdev)
{
#if 0
	struct snd_soc_card *card = &arkn141_audio;
	int ret;

	card->dev = &pdev->dev;

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret)
		dev_err(&pdev->dev, "snd_soc_register_card() failed: %d\n",
			ret);
#else
	int ret;

	arkn141_audio[pdev->id].dev = &pdev->dev;

	ret = devm_snd_soc_register_card(&pdev->dev, &arkn141_audio[pdev->id]);
	if (ret)
		dev_err(&pdev->dev,
			"snd_soc_register_card(%s) failed: %d\n",
			arkn141_audio[pdev->id].name, ret);
#endif
	return ret;
}

static const struct of_device_id arkn141_audio_of_match[] = {
	{
		.compatible = "arkmicro,arkn141-audio",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, arkn141_audio_of_match);

static struct platform_driver arkn141_audio_driver = {
	.probe 		= arkn141_audio_probe,
	.driver 	= {
		.name	= "arkn141-audio",
		.of_match_table	= of_match_ptr(arkn141_audio_of_match),
	},
};
module_platform_driver(arkn141_audio_driver);

MODULE_DESCRIPTION("Arkmicro audio driver under ALSA SoC architecture");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
