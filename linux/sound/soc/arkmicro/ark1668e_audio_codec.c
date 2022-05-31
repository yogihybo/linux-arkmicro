/*
 * ARK1668E - ark internal codec
 *
 */
#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>

#include "ark1668e_i2s.h"
#define HPOUT_EN

//Mute State
#define MUTE_OFF		0
#define MUTE_ON		1
extern  int mute_status;

#define ARKDAC_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 |SNDRV_PCM_RATE_8000)

#define ARKDAC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE)

struct ark_sddac {
	struct device *dev;
	void __iomem *sys_base;//sys_base
	unsigned int 	vol_l;
	unsigned int 	vol_r;
	int master;
	int mute_status;
};

static const char *mute_switch[] = {
	"UNMUTE",
	"MUTE",
};
static const struct soc_enum mute_switch_enum =
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mute_switch), mute_switch);

/***************************************************************************************************
 *
 *	Mute setting
 *
 ***************************************************************************************************/
static int get_lineout_mute_status(struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *adac = snd_soc_component_get_drvdata(component);
	printk("get:lineout_pa_mute_status= %d\n",adac->mute_status);
	ucontrol->value.integer.value[0] = adac->mute_status;
	return 0;
}

static int set_lineout_mute_status (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *adac = snd_soc_component_get_drvdata(component);
	unsigned int mute;
	int ret = -1;

	unsigned int val = readl(adac->sys_base + rSYS_AUDIO_CFG_2);

	printk("set:lineout_pa_mute_status= %ld\n",ucontrol->value.integer.value[0]);
	mute = ucontrol->value.integer.value[0];
	
	switch (mute)
	{
		case MUTE_OFF:
			//unmute PA
			//val &= ~(3<<27);
			val &= ~(0x3<<29);//unmute
			writel(val, adac->sys_base + rSYS_AUDIO_CFG_2);
			ret = adac->mute_status = MUTE_OFF;
			break;
		case MUTE_ON:
			//mute PA
			//val |= (3<<27);
			val |= (0x3<<29);//mute
			writel(val, adac->sys_base + rSYS_AUDIO_CFG_2);
			ret = adac->mute_status = MUTE_ON;
			break;
		default:
			break;
	}

	return ret;
}


static int ark_adac_get_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *adac = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = adac->vol_l & 0x3f;
	//printk("get_l_playback_volume = %ld\n",ucontrol->value.integer.value[0]);
	return 0;
}

static int ark_adac_set_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *adac = snd_soc_component_get_drvdata(component);
	{
		//lineout
		unsigned int val = readl(adac->sys_base + rSYS_AUDIO_CFG_3);
		adac->vol_l = ucontrol->value.integer.value[0];
		//printk("set_l_playback_volume = %d\n",dac->vol_l);

		val &= ~DACR0_LVOL_MASK;
		val |= DACR0_LVOL(adac->vol_l);
		//printk("new_l_playback_volume = 0x%x\n",DACR0_LVOL(dac->vol_l));
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_3);
	}
	{
		//hpout
		unsigned int val = readl(adac->sys_base + rSYS_AUDIO_CFG_4);
		adac->vol_l = ucontrol->value.integer.value[0];
		//printk("set_l_playback_volume = %d\n",dac->vol_l);

		val &= ~DACR0_LHPVOL_MASK;
		val |= DACR0_LHPVOL(adac->vol_l);
		//printk("new_l_playback_volume = 0x%x\n",DACR0_LHPVOL(dac->vol_l));
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_4);
	}

	return 0;
}

static int ark_adac_get_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *adac = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = adac->vol_r & 0x3f;
	//printk("get_r_playback_volume = %ld\n",ucontrol->value.integer.value[0]);
	return 0;
}

static int ark_adac_set_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *adac = snd_soc_component_get_drvdata(component);
	//unsigned int val = readl(dac->sys_base + rSYS_AUDIO_CFG_3);
	//dac->vol_r = ucontrol->value.integer.value[0];
	//printk("set_r_playback_volume = %d\n",dac->vol_r);
	{
		//lineout
		unsigned int val = readl(adac->sys_base + rSYS_AUDIO_CFG_3);
		adac->vol_r = ucontrol->value.integer.value[0];
		val &= ~DACR0_RVOL_MASK;
		val |= DACR0_RVOL(adac->vol_r);
		//printk("new_r_playback_volume = 0x%x\n",DACR0_RVOL(dac->vol_r));
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_3);
	}
	{
		//hpout
		unsigned int val = readl(adac->sys_base + rSYS_AUDIO_CFG_3);
		adac->vol_r = ucontrol->value.integer.value[0];
		val &= ~DACR0_RHPVOL_MASK;
		val |= DACR0_RHPVOL(adac->vol_r);
		//printk("new_r_playback_volume = 0x%x\n",DACR0_RHPVOL(dac->vol_r));
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_3);
	}
		

	return 0;
}

static const struct snd_kcontrol_new  ark_adac_snd_controls[] = {
	/* DAC volume control */
	SOC_SINGLE_EXT("DAC Left Playback Volume", 0, 0, 63, 0,
			ark_adac_get_l_playback_volume, ark_adac_set_l_playback_volume),
	SOC_SINGLE_EXT("DAC Right Playback Volume", 0, 0, 63, 0,
			ark_adac_get_r_playback_volume, ark_adac_set_r_playback_volume),
	SOC_ENUM_EXT("DAC PA Mute Control", mute_switch_enum,
		get_lineout_mute_status, set_lineout_mute_status),
};

static const struct snd_soc_dapm_widget ark_adac_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("ROUT"),
};

static int ark_adac_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0 ;
}

static void ark_adac_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
}

static int ark_adac_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	unsigned int val;
	struct ark_sddac *adac = snd_soc_dai_get_drvdata(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		//printk(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>PLAYBACK\n");
		val = readl(adac->sys_base + rSYS_DEVICE_CLK_CFG3);
		val |= (1 << 7) | (1 << 6); //select i2s1 clk
		writel(val, adac->sys_base + rSYS_DEVICE_CLK_CFG3);

		val = readl(adac->sys_base + rSYS_PAD_CTRL0F);
		val |= (1 << 11) | (1 << 15);	//select i2s1 bclk and sync
		val &= ~((1 << 13) | (1 << 14)); //select internal codec
		writel(val, adac->sys_base + rSYS_PAD_CTRL0F);
		
		val = readl(adac->sys_base + rSYS_AUDIO_CFG_0);
		if(adac->master)
			val |= (1<<0);//Master
		else
			val &= ~(1<<0);//Slave
		val |=(0x1<<9)|(0xf<<1);
		//val &= ~(0x3<<13);//0:RMICIN/LMICIN	1: RLINEIN/LLINEIN
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_0);

		val = readl(adac->sys_base + rSYS_AUDIO_CFG_3);
		val &= ~(0x3<<15);//DAC_PD
		val &= ~(0x7<<21);//HP_PD
		val |= (0x3<<17);//
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_3);
		
		val = readl(adac->sys_base + rSYS_AUDIO_CFG_2);
		val &= ~(0x3<<27);//LINEOUT_PD
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_2);

		val = readl(adac->sys_base + rSYS_AUDIO_CFG_4);
		val &= ~(0x1<<9);//SPK_PD
		val &=~(0x1 << 8);
		val |= (0x1 << 7);
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_4);

		val = readl(adac->sys_base + rSYS_AUDIO_CFG_5);
		val |=(0x1<<31);
		writel(val,adac->sys_base + rSYS_AUDIO_CFG_5);

	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		//printk(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>CAPTURE\n");
		udelay(2);
		val = readl(adac->sys_base + rSYS_DEVICE_CLK_CFG3);
		val |= (1 << 7);
		val &= ~(1 << 6); //select i2s0 clk
		writel(val, adac->sys_base + rSYS_DEVICE_CLK_CFG3);

		val = readl(adac->sys_base + rSYS_PAD_CTRL0F);
		val &= ~((1 << 11) | (1 << 15)); //select i2s0 bclk and sync
		val &= ~((1 << 8) | (1 << 9) | (1 << 10)); //select internal codec
		writel(val, adac->sys_base + rSYS_PAD_CTRL0F);

		val = readl(adac->sys_base + rSYS_AUDIO_CFG_0);
		if(adac->master)
			val |= (1<<0);//Master
		else
			val &= ~(1<<0);//Slave

		//val &= ~(0x3<<13);
		val &= ~(1<<13);//0:RMICIN	1: RLINEIN
		val &= ~(1<<14);//0:LMICIN	1:LLINEIN
		//val &= ~(1<<15);//
		//val &= ~(1<<16);//
		val &= ~(1<<21);
		val &= ~(1<<22);
		//val |=(0x3<<13)|(0x1<<9)|(0xf<<1);
		val |=(0x1<<9)|(0xf<<1);
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_0);

		val = readl(adac->sys_base + rSYS_AUDIO_CFG_1);
		val &= ~(0xf<<6);
		//val |= ((0x1<<7)|(0x1<<9));//L: Power-down mode	R:Normal mode			//for ksw only
		//val |= ((0x1<<6)|(0x1<<8));//L:Normal mode			R: Power-down mode
		val |= (0x5f<<19)|(0x5f<<12);//volume gain control
		writel(val, adac->sys_base + rSYS_AUDIO_CFG_1);

		val = readl(adac->sys_base + rSYS_AUDIO_CFG_5);
		val &=~(0x1<<17);//MICBPD
		writel(val,adac->sys_base + rSYS_AUDIO_CFG_5);
	}

	return 0;
}

static int ark_adac_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int ark_adac_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int ark_adac_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	struct ark_sddac *i2s = snd_soc_dai_get_drvdata(codec_dai);
	
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		printk("##############ark audio codec master\n");
		i2s->master= 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		printk("##############ark audio codec slave\n");
		i2s->master = 0;
		break;
	default:
		break;
	}
	return 0;
}

static const struct snd_soc_dai_ops ark_adac_dai_ops = {
	.startup	=  ark_adac_startup,
	.shutdown	=  ark_adac_shutdown,
	.hw_params	=  ark_adac_hw_params,
	.set_sysclk	=  ark_adac_set_dai_sysclk,
	.set_fmt	=  ark_adac_set_dai_fmt,
	.digital_mute	= ark_adac_mute,
};


static struct snd_soc_dai_driver ark_adac_dai = {
	.name 		= "ark1668e-audio-codec",
	.playback 	= {
			.stream_name 	= "Playback",
			.channels_min	= 1,
			.channels_max	= 2,
			.rates			= ARKDAC_RATES,
			.formats		= ARKDAC_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ARKDAC_RATES,
		.formats = ARKDAC_FORMATS,
	},
	.ops 		= &ark_adac_dai_ops,
};

static int ark_adac_codec_probe(struct snd_soc_component *component)
{
	printk("############[%s]:\n",__FUNCTION__);
	return 0;
}

static const struct snd_soc_component_driver ark_adac_component_driver = {
	.probe			= ark_adac_codec_probe,
	.controls		= ark_adac_snd_controls,
	.num_controls		= ARRAY_SIZE(ark_adac_snd_controls),
	.dapm_widgets		= ark_adac_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ark_adac_dapm_widgets),
};

static int ark_adac_probe(struct platform_device *pdev)
{
	struct ark_sddac *adac;
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	adac = devm_kzalloc(dev, sizeof(*adac), GFP_KERNEL);
	if (!adac)
		return -ENOMEM;

	adac->mute_status = MUTE_OFF;

	if (of_property_read_u32(pdev->dev.of_node, "left-volume", &adac->vol_l))
		adac->vol_l = 50;

	if (of_property_read_u32(pdev->dev.of_node, "right-volume", &adac->vol_r))
		adac->vol_r = 50;
	//printk(">>>>>>>>>>>>>>>>>>dac:left-volume = %d,right-volume = %d \n",dac->vol_l,dac->vol_r);

	platform_set_drvdata(pdev, adac);

	//sys resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	adac->sys_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(adac->sys_base))
		return PTR_ERR(adac->sys_base);
	//printk("==============[sys_base = 0x%08x ]\n",adac->sys_base);

	ret = devm_snd_soc_register_component(dev, &ark_adac_component_driver,
			&ark_adac_dai,
			1);
	if (ret) {
		dev_err(dev, "failed to register codec: %d\n", ret);
		goto err;
	}

	return 0;

err:
	return ret;
}

static int ark_adac_remove(struct platform_device *pdev)
{
	struct ark_sddac *adac = dev_get_drvdata(&pdev->dev);

	if (adac->sys_base)
		iounmap(adac->sys_base);
	return 0;
}

static const struct of_device_id ark_adac_match[] = {
	{ .compatible = "arkmicro,ark-audio-codec", },
	{},
};

static struct platform_driver ark_adac_driver = {
	.driver = {
		.name 	= "ark-adac",
		.of_match_table = of_match_ptr(ark_adac_match),
	},
	.probe 	= ark_adac_probe,
	.remove = ark_adac_remove,
};
module_platform_driver(ark_adac_driver);

MODULE_DESCRIPTION("ARK dac codec driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");

