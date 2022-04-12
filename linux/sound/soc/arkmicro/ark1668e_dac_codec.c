/*
 * ARKDAC - ark digital-to-analog converter
 *
 */
#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>

//#include "ark_i2s.h"
#include "ark1668e_i2s.h"
#define HPOUT_EN

#define ARKDAC_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 |SNDRV_PCM_RATE_8000)

#define ARKDAC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE)

struct ark_sddac {
	struct device *dev;
	void __iomem *sys_base;//sys_base
	void __iomem	*base;//i2s_base
	unsigned int 	vol_l;
	unsigned int 	vol_r;
	unsigned 	hp_out;//headphone-out
	int extdata;			//true:external-i2s	or	false:internal-codec-in(LINEIN/MICIN)
	//struct ark1668e_i2s_cfg i2s_cfg[2];
};

static int ark_sddac_get_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = dac->vol_l & 0x3f;
	//printk("get_l_playback_volume = %ld\n",ucontrol->value.integer.value[0]);
	return 0;
}

static int ark_sddac_set_l_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
#if 1
	//lineout
	unsigned int val = readl(dac->sys_base + rSYS_AUDIO_CFG_3);
	dac->vol_l = ucontrol->value.integer.value[0];
	//printk("set_l_playback_volume = %d\n",dac->vol_l);

	val &= ~DACR0_LVOL_MASK;
	val |= DACR0_LVOL(dac->vol_l);
	//printk("new_l_playback_volume = 0x%x\n",DACR0_LVOL(dac->vol_l));
	writel(val, dac->sys_base + rSYS_AUDIO_CFG_3);
#else
	//hpout
	unsigned int val = readl(dac->sys_base + rSYS_AUDIO_CFG_4);
	dac->vol_l = ucontrol->value.integer.value[0];
	//printk("set_l_playback_volume = %d\n",dac->vol_l);

	val &= ~DACR0_LHPVOL_MASK;
	val |= DACR0_LHPVOL(dac->vol_l);
	//printk("new_l_playback_volume = 0x%x\n",DACR0_LHPVOL(dac->vol_l));
	writel(val, dac->sys_base + rSYS_AUDIO_CFG_4);
#endif

	return 0;
}

static int ark_sddac_get_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = dac->vol_r & 0x3f;
	//printk("get_r_playback_volume = %ld\n",ucontrol->value.integer.value[0]);
	return 0;
}

static int ark_sddac_set_r_playback_volume (struct snd_kcontrol * kcontrol, struct snd_ctl_elem_value * ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);
	unsigned int val = readl(dac->sys_base + rSYS_AUDIO_CFG_3);
	dac->vol_r = ucontrol->value.integer.value[0];
	//printk("set_r_playback_volume = %d\n",dac->vol_r);
#if 1
	//lineout
	val &= ~DACR0_RVOL_MASK;
	val |= DACR0_RVOL(dac->vol_r);
	//printk("new_r_playback_volume = 0x%x\n",DACR0_RVOL(dac->vol_r));
#else
	//hpout
	val &= ~DACR0_RHPVOL_MASK;
	val |= DACR0_RHPVOL(dac->vol_r);
	//printk("new_r_playback_volume = 0x%x\n",DACR0_RHPVOL(dac->vol_r));
#endif
	writel(val, dac->sys_base + rSYS_AUDIO_CFG_3);

	return 0;
}

static const struct snd_kcontrol_new  ark_sddac_snd_controls[] = {
	/* DAC volume control */
	SOC_SINGLE_EXT("Left Playback Volume 2", 0, 0, 63, 0,
			ark_sddac_get_l_playback_volume, ark_sddac_set_l_playback_volume),
	SOC_SINGLE_EXT("Right Playback Volume 2", 0, 0, 63, 0,
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

		unsigned int val,val_test;
		struct ark_sddac *i2s = snd_soc_dai_get_drvdata(dai);
		//struct ark1668e_i2s_cfg *i2s_cfg;
#ifndef CONFIG_ARK1668E_I2S_FULL_DUPLEX_MODE
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
#endif
			val = readl(i2s->sys_base+ rSYS_DEVICE_CLK_CFG3);
			val &= ~(0xff<<1);//I2S1CLK   //default:~(0xff<<0)
			val |=(0x1<<2)|(0x1<<6)|(0x1<<7);
			writel(val,i2s->sys_base +rSYS_DEVICE_CLK_CFG3);

			val = readl(i2s->sys_base + rSYS_PLL_RFCK_CTL);
			val |=(0x1<<17);//IntI2S1Clk_pre
			writel(val, i2s->sys_base + rSYS_PLL_RFCK_CTL);

#ifndef CONFIG_ARK1668E_I2S_FULL_DUPLEX_MODE
			val = readl(i2s->sys_base + rSYS_PAD_CTRL0F);
//			val &= ~(1<<12);//0:i2s1_sadata_out 1: 0
//			val |= (1 << 29)|(0x1<<15)|(0x1<<11)|(0x0<<12);//(29)1:i2s1 sadata out  (15)1:i2s1_sync_out  0: i2s_sync_out    (11)1:i2s1_bclk_out  0: i2s_bclk_out  (12)0:i2s1_sadata_out  1: 0
//			//val &= ~(0xff<<8);//I2S1CLK
//			//val |= (1 << 29)|(0x1<<15)|(0x1<<11);

			//add 20220225
			val &= ~((0x1<<12)|(0x1<<13)|(0x1<<14));
			val &= ~(1<<29);

			writel(val, i2s->sys_base + rSYS_PAD_CTRL0F);
#else
			val = readl(i2s->sys_base + rSYS_PAD_CTRL0F);
			val &= ~(1<<12);//0:i2s1_sadata_out 1: 0
			val |= (0x1<<15)|(0x1<<14)|(0x1<<11)|(0x0<<12);//add 20211120
			val &= ~(1<<29);//add 20211120
			writel(val, i2s->sys_base + rSYS_PAD_CTRL0F);

#endif
			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_0);
			//val &= ~(1<<0);//Slave
			val |= (1<<0);//Master
			val |=(0x1<<9)|(0xf<<1);
			writel(val, i2s->sys_base + rSYS_AUDIO_CFG_0);

//			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_2);
//			val &= ~(3<<27);
//			writel(val, i2s->sys_base + rSYS_AUDIO_CFG_2);

			if(i2s->hp_out){
				//PHOUT
				val = readl(i2s->sys_base + rSYS_AUDIO_CFG_3);
				val &= ~(1<<23);
				val &= ~(1<<15);
				val |=(0x0<<23)|(0x1<<22)|(0x1<<21)|(0x1<<18)|(0x1<<17)|(0x0<<15);
				writel(val, i2s->sys_base + rSYS_AUDIO_CFG_3);
			}else{
				//default:LINEOUT
				val = readl(i2s->sys_base + rSYS_AUDIO_CFG_3);
				val &= ~(1<<23);
				val &= ~(1<<22);
				val &= ~(1<<21);
				val &= ~(1<<15);
				val |=(0x0<<23)|(0x0<<22)|(0x0<<21)|(0x0<<15);
				writel(val, i2s->sys_base + rSYS_AUDIO_CFG_3);
			}

			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_4);
			val &= ~(1<<9 | (1 << 8));
			val |= (1 << 7);
			writel(val, i2s->sys_base + rSYS_AUDIO_CFG_4);

			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_5);
			val |=(0x1<<31);
			writel(val,i2s->sys_base + rSYS_AUDIO_CFG_5);

#ifdef CONFIG_ARK1668E_I2S_FULL_DUPLEX_MODE
			//i2s controller
			val = readl(i2s->base + I2S_SACR0);
			val |=((1<<2)|(1<<1));
			val |=(5<<8)|1;
			writel(val, i2s->base + I2S_SACR0);

			val = readl(i2s->base + I2S_SACR0);
			val &= ~(0x1f<<8);
			val |= (0xf<<8);//|(0x1<<23);
			//val |= (0x1<<5);
			writel(val, i2s->base + I2S_SACR0);

			val = readl(i2s->base + I2S_SACR1);
			val &= ~SACR1_DISABLE_REPLAYING;
			val &= ~SACR1_DISABLE_RECORD;
			writel(val, i2s->base + I2S_SACR1);

			val = readl(i2s->base + I2S_SACR0);
			val |= (0x1 << 3)|(0x1<<6);//DMA
			writel(val, i2s->base + I2S_SACR0);

			val = readl(i2s->base + I2S_SAICR);
			val = 0xFFFFFFFF;
			writel(val, i2s->base + I2S_SAICR);

			val = readl(i2s->base + I2S_SAICR);
			val = 0;
			writel(val, i2s->base + I2S_SAICR);
#else
		}else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		}
#endif
	return 0;
}

static int ark_sddac_mute(struct snd_soc_dai *dai, int mute)
{
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
#ifdef CONFIG_ARK1668E_I2S_FULL_DUPLEX_MODE
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = ARKDAC_RATES,
		.formats = ARKDAC_FORMATS,
	},
#endif
	.ops 		= &ark_sddac_dai_ops,
};

static int ark_sddac_codec_probe(struct snd_soc_component *component)
{
	struct ark_sddac *dac = snd_soc_component_get_drvdata(component);

	writel(DACR0_RVOL(dac->vol_r) | DACR0_LVOL(dac->vol_l), dac->base + I2S_DACR0);

	return 0;
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
		dac->vol_l = 50;

	if (of_property_read_u32(pdev->dev.of_node, "right-volume", &dac->vol_r))
		dac->vol_r = 50;
	//printk(">>>>>>>>>>>>>>>>>>left-volume = %d,right-volume = %d \n",dac->vol_l,dac->vol_r);

	if (of_property_read_bool(pdev->dev.of_node, "external-i2s"))
		dac->extdata = 1;
	printk(">>>>>>>>>>>>>>>>>>dac->extdata = %d \n",dac->extdata);

	if (of_property_read_bool(pdev->dev.of_node, "headphone-out"))
		dac->hp_out = 1;

	platform_set_drvdata(pdev, dac);

	//i2s resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dac->base = ioremap(res->start, resource_size(res));
	if (IS_ERR(dac->base))
		return PTR_ERR(dac->base);

	//sys resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	dac->sys_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(dac->sys_base))
		return PTR_ERR(dac->sys_base);
	//printk("DAC:==============[sys_base = 0x%08x   i2s_base = 0x%08x]\n",dac->sys_base,dac->base);

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
	{ .compatible = "arkmicro,ark1668e-sddac", },
	{},
};

static struct platform_driver ark_sddac_driver = {
	.driver = {
		.name 	= "ark-sddac",
		.of_match_table = of_match_ptr(ark_sddac_match),
	},
	.probe 	= ark_sddac_probe,
	.remove = ark_sddac_remove,
};
module_platform_driver(ark_sddac_driver);

MODULE_DESCRIPTION("ARK dac codec driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");

