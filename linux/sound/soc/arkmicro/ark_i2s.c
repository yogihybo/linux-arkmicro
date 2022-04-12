/*
 * ark_i2s.c -- ALSA SoC Audio Layer
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/clk.h>
#include <sound/dmaengine_pcm.h>

#include "ark_i2s.h"

#define DRV_NAME	"ark-i2s"

struct ark_i2s_adc{
	unsigned int 	vol_l;
	unsigned int 	vol_r;
};

struct ark_i2s_dev {
	struct  device	*dev;
	void __iomem 	*base;
	struct	clk		*clk;
	u32	nco_reg;
	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;
	int master;
	u32 fmt;
	int extdata;
	struct ark_i2s_adc  adc;
};

static void i2s_poweron(struct ark_i2s_dev *i2s)
{
	uint32_t val;

	val = readl(i2s->base + I2S_SACR0);
	val &= ~(SACR0_VREF_PD | SACR0_DAC_PD);
	val |= SACR0_SDRADC_POWEN;
	writel(val, i2s->base + I2S_SACR0);
}

static int ark_i2s_startup(
	struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	//printk(">>>>>>>>>>>>>>>>>>>left-volume = %d  right-volume = %d \n",i2s->adc.vol_l,i2s->adc.vol_r);
	unsigned int val;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		val = readl(i2s->base + I2S_SACR0);
		val |= SACR0_DATA_SEL | SACR0_TFTH(15) | SACR0_BCKD | SACR0_ENB;
		writel(val, i2s->base + I2S_SACR0);

		val = readl(i2s->base + I2S_SAIMR);
		val &= ~(SAIMR_TUR | SAIMR_TFS);
		writel(val, i2s->base + I2S_SAIMR);

		val = readl(i2s->base + I2S_SAICR);
		val |= (SAICR_TUR | SAICR_TFS);
		writel(val, i2s->base + I2S_SAICR);

		val = readl(i2s->base + I2S_SAICR);
		val &= ~(SAICR_TUR | SAICR_TFS);
		writel(val, i2s->base + I2S_SAICR);

		val = readl(i2s->base + I2S_SACR0);
		val |= SACR0_TDMAEN;
		writel(val, i2s->base + I2S_SACR0);

		val = readl(i2s->base + I2S_SACR1);
		val &= ~SACR1_DRPL;                     //enable play
		writel(val, i2s->base + I2S_SACR1);
	} else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		val = readl(i2s->base + I2S_SACR0);
		val &= ~(SACR0_VREF_VOLSEL | SACR0_VREF_PD | SACR0_STRF |
				SACR0_ENLBF | SACR0_RFTH_MASK | SACR0_BCKD | SACR0_SYNCD);
		val |= (SACR0_SDRADC_POWEN | SACR0_DATA_SEL | SACR0_SARADC_DIS |
				SACR0_RFTH(16) | SACR0_ENB);
		if (i2s->master)
			val |= SACR0_BCKD | SACR0_SYNCD;
		if (i2s->extdata) {
			val &= ~SACR0_DATA_SEL;
			val |= SACR0_MIC_LINE_SEL | SACR0_DAC_PD | SACR0_VREF_PD;
		}
		writel(val, i2s->base + I2S_SACR0);

		val = readl(i2s->base + I2S_ADCR0);
		val &= ~(ADCR0_LVOL_MASK | ADCR0_RVOL_MASK | ADCR0_LFS_MASK | 
				ADCR0_RFS_MASK);
		//val |= ADCR0_LVOL(3) | ADCR0_RVOL(3) | ADCR0_LFS_1P4 |
		//		ADCR0_RFS_1P4 | ADCR0_LME | ADCR0_RME;
		val |= ADCR0_LVOL(i2s->adc.vol_l) | ADCR0_RVOL(i2s->adc.vol_r) | ADCR0_LFS_1P4 |
				ADCR0_RFS_1P4 | ADCR0_LME | ADCR0_RME;
		writel(val, i2s->base + I2S_ADCR0);

		val = readl(i2s->base + I2S_SAIMR);
		val &= ~(SAIMR_ROR | SAIMR_RFS);
		writel(val, i2s->base + I2S_SAIMR);

		val = readl(i2s->base + I2S_SAICR);
		val |= (SAICR_ROR | SAICR_RFS);
		writel(val, i2s->base + I2S_SAICR);

		val = readl(i2s->base + I2S_SAICR);
		val &= ~(SAICR_ROR | SAICR_RFS);
		writel(val, i2s->base + I2S_SAICR);

		val = readl(i2s->base + I2S_SACR0);
		val |= SACR0_RDMAEN;
		writel(val, i2s->base + I2S_SACR0);

		val = readl(i2s->base + I2S_SACR1);
		val &= ~SACR1_DREC;                     //enable record
		writel(val, i2s->base + I2S_SACR1);
	}

	return 0;
}

static int ark_i2s_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	u32 rate = params_rate(params);
	u32 step = 256 * 2, modulo;
	u32 val, freq;
	void *sysreg;

	if (!i2s->nco_reg)
		return 0;

	/* mclk = rate * 256, mclk = freq * step / (2 * modulo) */
	freq = clk_get_rate(i2s->clk);
	modulo = freq / rate;
	val = (step << 16) | modulo;
	sysreg = ioremap(i2s->nco_reg, 0x10);
	if (sysreg) {
		writel(val, sysreg);
		iounmap(sysreg);
	}

	return 0;
}


static int ark_i2s_trigger(
	struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			writel(readl(i2s->base + I2S_SACR1) & ~SACR1_DRPL, i2s->base + I2S_SACR1);
		else
			writel(readl(i2s->base + I2S_SACR1) & ~SACR1_DREC, i2s->base + I2S_SACR1);
		writel(readl(i2s->base + I2S_SACR0) | SACR0_ENB, i2s->base + I2S_SACR0);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int ark_i2s_set_fmt(
	struct snd_soc_dai *dai, unsigned int fmt)
{
	struct ark_i2s_dev *i2s =snd_soc_dai_get_drvdata(dai);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		i2s->fmt = 0;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		i2s->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		i2s->master = 0;
		break;
	default:
		break;
	}

	return 0;
}

static int ark_i2s_probe(struct snd_soc_dai *dai)
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	dai->capture_dma_data = &i2s->capture_dma_data;
	dai->playback_dma_data = &i2s->playback_dma_data;

	return 0;
}

/* I2S supported rate and format */
#define ARK_I2S_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000)

static const struct snd_soc_dai_ops ark_i2s_dai_ops = {
	.startup		= ark_i2s_startup,
	.trigger 		= ark_i2s_trigger,
	.hw_params 		= ark_i2s_hw_params,
	.set_fmt 		= ark_i2s_set_fmt,
};

static struct snd_soc_dai_driver ark_i2s_dai = {
	.probe = ark_i2s_probe,
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = ARK_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = ARK_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,},
	.ops = &ark_i2s_dai_ops,
	.symmetric_rates = 1,
};

static struct snd_pcm_hardware ark_pcm_hardware = {
	.info 				= (SNDRV_PCM_INFO_MMAP |
 						SNDRV_PCM_INFO_MMAP_VALID |
						SNDRV_PCM_INFO_PAUSE |
 						SNDRV_PCM_INFO_RESUME |
						SNDRV_PCM_INFO_INTERLEAVED |
						SNDRV_PCM_INFO_BLOCK_TRANSFER),
	.formats 			= SNDRV_PCM_FMTBIT_S16_LE,
	.rates 				= (SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 |
						SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 |
						SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
						SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 |
						SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
						SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000),
	.rate_min 			= 8000,
	.rate_max			= 192000,
	.channels_min 		= 2,
	.channels_max 		= 2,
	.buffer_bytes_max 	= 64 * 4096,
	.period_bytes_min 	= 64,
	.period_bytes_max 	= 4096,
	.periods_min 		= 1,
	.periods_max 		= 64,
};

static const struct snd_dmaengine_pcm_config
ark_i2s_dmaengine_pcm_config = {
	.prepare_slave_config	= snd_dmaengine_pcm_prepare_slave_config,
	.pcm_hardware		= &ark_pcm_hardware,
};

static const struct snd_soc_component_driver ark_i2s_component = {
	.name		= DRV_NAME,
};

static int ark_i2s_drv_probe(struct platform_device *pdev)
{
	struct ark_i2s_dev *i2s;
	struct resource *res;
	u32 val;
	int ret = 0;

	i2s = devm_kzalloc(&pdev->dev, sizeof(struct ark_i2s_dev), GFP_KERNEL);
	if (!i2s)
		return -ENOMEM;

	i2s->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2s->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2s->base))
		return PTR_ERR(i2s->base);

	if (!of_property_read_u32(pdev->dev.of_node, "nco-reg", &val))
		i2s->nco_reg = val;

	if (of_property_read_bool(pdev->dev.of_node, "external-i2s"))
		i2s->extdata = 1;

	if (of_property_read_u32(pdev->dev.of_node, "left-volume", &i2s->adc.vol_l))
		i2s->adc.vol_l = 3;

	if (of_property_read_u32(pdev->dev.of_node, "right-volume", &i2s->adc.vol_r))
		i2s->adc.vol_r = 3;

	i2s->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(i2s->clk))
		return PTR_ERR(i2s->clk);

	/* DMA parameters */
	i2s->playback_dma_data.addr = res->start + I2S_SADR;
	i2s->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->playback_dma_data.maxburst = 16;

	i2s->capture_dma_data.addr = res->start + I2S_SADR;
	i2s->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->capture_dma_data.maxburst = 16;

	dev_set_drvdata(&pdev->dev, i2s);

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &ark_i2s_component,
					      &ark_i2s_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI\n");
		return ret;
	}

	i2s_poweron(i2s);

	ret = devm_snd_dmaengine_pcm_register(&pdev->dev,
						&ark_i2s_dmaengine_pcm_config,
						0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id ark_i2s_match[] = {
	{ .compatible = "arkmicro,ark-i2s", },
	{},
};

static struct platform_driver ark_i2s_driver = {
	.probe 		= ark_i2s_drv_probe,
	.driver		= {
		.name 	= DRV_NAME,
		.of_match_table = of_match_ptr(ark_i2s_match),
	},
};
module_platform_driver(ark_i2s_driver);

MODULE_DESCRIPTION("ARK I2S SoC Interface");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");

