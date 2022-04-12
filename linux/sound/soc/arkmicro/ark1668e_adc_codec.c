/*
 * ARKADC - ark digital-to-analog converter
 *
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/io.h>
#include <linux/slab.h>

#include "ark1668e_i2s.h"

struct ark_sdadc {
	struct device *dev;
	void __iomem  *sys_base;//sys_base
	void __iomem	*i2s_base;//i2s_base
	unsigned int 	vol_l;
	unsigned int 	vol_r;
	int extdata;			//true:external-i2s	or	false:internal-codec-in(LINEIN/MICIN)
};

#define ARKADC_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000)

#define ARKADC_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE)

static int ark_sdadc_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
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

		unsigned int val,val_test;
		struct ark_sdadc *i2s = snd_soc_dai_get_drvdata(dai);
		//printk("==============[sys_base = 0x%08x   i2s_base = 0x%08x]\n",i2s->sys_base,i2s->i2s_base);
		if(substream->stream == SNDRV_PCM_STREAM_CAPTURE){
			val = readl(i2s->sys_base + rSYS_DEVICE_CLK_CFG3);
			//val &= ~(1<<6);//I2SCLK
			//val |=(0x1<<0)|(0x1<<7)|(0x1<<2);
			if(i2s->extdata){
				//external codec i2s
				val &= ~(0xff<<1);
				val |= (1<<6);//I2S1CLK
				val |=(0x1<<0)|(1<<6)|(0x1<<7)|(0x1<<2);
			}else{
				//internal codec audio
				val &= ~(1<<6);//I2SCLK
				val |=(0x1<<0)|(0x1<<7)|(0x1<<2);
			}
			writel(val, i2s->sys_base + rSYS_DEVICE_CLK_CFG3);

			val = readl(i2s->sys_base + rSYS_PLL_RFCK_CTL);
			//val &= ~(1<<16);//IntI2SClk_pre
			val |=(0x1<<17);//IntI2S1Clk_pre
			writel(val, i2s->sys_base + rSYS_PLL_RFCK_CTL);

			val = readl(i2s->sys_base + rSYS_PAD_CTRL0F);
			val &= ~(1<<28);
			//val &= ~(0xf<<12);
			//val &= ~(1<<11);
			//val |= (0x7<<8);
			if(i2s->extdata){
				val |= (0xf<<8);					//external codec i2s
			}else{
				val &= ~((1<<8)|(1<<9)|(1<<10));	//internal codec audio
				//val |= (0x1<<11);
				//val &= ~(1<<11);//0:i2s_bclk_out	1: i2s1_bclk_out
				//val &= ~(1<<15);//0:i2s_sync_out	1: i2s1_sync_out
			}

			//val &= ~(1<<8);//0;audio_codec_dout  1: i2s_sadata_in
			//val &= ~(1<<11);//0:i2s_bclk_out	1: i2s1_bclk_out
			//val &= ~(1<<15);//0:i2s_sync_out	1: i2s1_sync_out
			writel(val, i2s->sys_base + rSYS_PAD_CTRL0F);

			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_0);

			//val &= ~(1<<0);//Slave
			val |= (1<<0);//Master
			if(!i2s->extdata){
				val &= ~(1<<13);//0:RMICIN	1: RLINEIN
				val &= ~(1<<14);//0:LMICIN	1:LLINEIN
			}

			val &= ~(1<<21);
			val &= ~(1<<22);
			//val |=(0x3<<13)|(0x1<<9)|(0xf<<1);
			val |=(0x1<<9)|(0xf<<1);
			writel(val, i2s->sys_base + rSYS_AUDIO_CFG_0);

			//set volume
			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_1);
			val &= ~(0xf<<6);
			//val |= ((0x1<<7)|(0x1<<9));//L: Power-down mode	R:Normal mode			//for ksw only
			//val |= ((0x1<<6)|(0x1<<8));//L:Normal mode			R: Power-down mode
			val |= (0x5f<<19)|(0x5f<<12);//volume gain control
			writel(val, i2s->sys_base + rSYS_AUDIO_CFG_1);

			if(!i2s->extdata){
				val = readl(i2s->sys_base + rSYS_AUDIO_CFG_5);
				val &=~(0x1<<17);//MICBPD	add:20220214
				writel(val,i2s->sys_base + rSYS_AUDIO_CFG_5);
			}

//			val = readl(i2s->sys_base + rSYS_AUDIO_CFG_5);
//			val |=(0x1<<31);
//			writel(val,i2s->sys_base + rSYS_AUDIO_CFG_5);

			val = readl(i2s->sys_base + rSYS_PAD_CTRL06);
			val  &= ~(1<<31);
			writel(val, i2s->sys_base + rSYS_PAD_CTRL06);

		}else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		}
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
		.channels_min = 2,
		.channels_max = 2,
		.rates = ARKADC_RATES,
		.formats = ARKADC_FORMATS,
	},
	.ops = &ark_sdadc_dai_ops,
};

//static int ark_sdadc_codec_probe(struct snd_soc_codec *codec)
static int ark_sdadc_codec_probe(struct snd_soc_component *component)
{
	return 0;
}

static const struct snd_soc_dapm_widget ark_sdadc_dapm_widgets[] = {
	SND_SOC_DAPM_INPUT("MICIN"),
	SND_SOC_DAPM_INPUT("RLINEIN"),
	SND_SOC_DAPM_INPUT("LLINEIN"),
};

static struct snd_soc_component_driver ark_sdadc_component_driver = {
	.probe 				= ark_sdadc_codec_probe,
	.dapm_widgets		= ark_sdadc_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ark_sdadc_dapm_widgets),
};

static int ark_sdadc_probe(struct platform_device *pdev)
{
	struct ark_sdadc *adc;
	struct resource *res;
	struct device *dev = &pdev->dev;
	int ret;

	adc = devm_kzalloc(dev, sizeof(*adc), GFP_KERNEL);
	if (!adc)
		return -ENOMEM;

	if (of_property_read_u32(pdev->dev.of_node, "left-volume", &adc->vol_l))
		adc->vol_l = 110;

	if (of_property_read_u32(pdev->dev.of_node, "right-volume", &adc->vol_r))
		adc->vol_r = 110;
	//printk(">>>>>>>>>>>>>>>>>>left-volume = %d,right-volume = %d \n",adc->vol_l,adc->vol_r);

	if (of_property_read_bool(pdev->dev.of_node, "external-i2s"))
		adc->extdata = 1;
	//printk(">>>>>>>>>>>>>>>>>>adc->extdata = %d \n",adc->extdata);
	platform_set_drvdata(pdev, adc);
	
	//i2s resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	adc->i2s_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(adc->i2s_base))
		return PTR_ERR(adc->i2s_base);

	//sys resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	//printk("==============[resource_size = 0x%08x]\n",resource_size(res));
	adc->sys_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(adc->sys_base))
		return PTR_ERR(adc->sys_base);
	//printk("ADC:==============[sys_base = 0x%08x   i2s_base = 0x%08x]\n",adc->sys_base,adc->i2s_base);
//	ret = snd_soc_register_codec(dev, &ark_sdadc_codec_driver,
//			&ark_sdadc_dai,
//			1);
	ret = devm_snd_soc_register_component(dev, &ark_sdadc_component_driver,
			&ark_sdadc_dai,
			1);
	if (ret) {
		dev_err(dev, "failed to register codec: %d\n", ret);
		goto err;
	}
	return 0;
	
err:
	iounmap(adc->i2s_base);
err_2:
	iounmap(adc->sys_base);

	return ret;
}

static int ark_sdadc_remove(struct platform_device *pdev)
{
	//snd_soc_unregister_codec(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	struct ark_sdadc *adc = dev_get_drvdata(&pdev->dev);
	iounmap(adc->i2s_base);
	iounmap(adc->sys_base);
	return 0;
}

static const struct of_device_id ark_sdadc_match[] = {
	{ .compatible = "arkmicro,ark1668e-sdadc", },
	{},
};

static struct platform_driver ark_sdadc_driver = {
	.driver = {
		.name 	= "ark-sdadc",
		.of_match_table = of_match_ptr(ark_sdadc_match),
	},
	.probe 	= ark_sdadc_probe,
	.remove = ark_sdadc_remove,
};
module_platform_driver(ark_sdadc_driver);

MODULE_DESCRIPTION("ARK adc codec driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
