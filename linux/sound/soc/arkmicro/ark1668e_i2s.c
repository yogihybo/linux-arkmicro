/*
 * ark1668e_i2s.c -- ALSA SoC Audio Layer
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>
#include <linux/clk.h>

#include "ark1668e_i2s.h"

#define DRV_NAME	"ark1668e-i2s"

//struct ark1668e_i2s1_data_in i2s_data;
int audio_codec_mode= SLAVE_MODE;////only for junjie

struct ark1668e_i2s_dev {
	struct  device	*dev;
	void __iomem 	*base;		//i2s_base
	struct	clk		*clk;
	int irq;
	u32	nco_reg;
	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;
	int master;
	u32 fmt;
	int full_duplex_en;
};

static void i2s_poweron(struct ark1668e_i2s_dev *i2s)
{
	return;
}

static int ark1668e_i2s_startup(
	struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct ark1668e_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned int sacr0 = 0;

	if(readl(i2s->base + I2S_SACR0) & SACR0_ENB)
		return 0;

	/* reset */
	writel(SACR0_RST, i2s->base + I2S_SACR0);
	udelay(1);
	writel(0, i2s->base + I2S_SACR0);

	if(i2s->full_duplex_en){
		sacr0 = SACR0_TLFIRST | SACR0_CH_LOCK | SACR0_TFTH(15) | SACR0_TDMAEN;
		sacr0 |= SACR0_RLFIRST | SACR0_CH_LOCK | SACR0_RFTH(16) | SACR0_RDMAEN;

		if (i2s->master)
			sacr0 |= SACR0_BCKD | SACR0_SYNCD;//ark1668e-i2s:Master mode
		else
			sacr0 &= ~(SACR0_BCKD | SACR0_SYNCD);//ark1668e-i2s:slave mode
		writel(sacr0, i2s->base + I2S_SACR0);

		writel(0x7f, i2s->base + I2S_SAICR);
		writel(0, i2s->base + I2S_SAICR);

	}else{
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/*i2s_regs_init*/
			sacr0 = SACR0_TLFIRST | SACR0_CH_LOCK | SACR0_TFTH(15) | SACR0_TDMAEN;
			if (i2s->master)
				sacr0 |=  SACR0_BCKD | SACR0_SYNCD;//ark1668e-i2s:Master mode
			else
				sacr0 &=  ~(SACR0_BCKD | SACR0_SYNCD);//ark1668e-i2s:slave mode
			writel(sacr0, i2s->base + I2S_SACR0);

			//writel(SAIMR_TUR, i2s->base + I2S_SAIMR);

			writel(0x7f, i2s->base + I2S_SAICR);
			writel(0, i2s->base + I2S_SAICR);
		} else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE){
			/*i2s_regs_init*/
			sacr0 |= SACR0_RLFIRST | SACR0_CH_LOCK | SACR0_RFTH(16) | SACR0_RDMAEN;
			if (i2s->master)
				sacr0 |= SACR0_BCKD | SACR0_SYNCD;//ark1668e-i2s:Master mode
			else
				sacr0 &= ~(SACR0_BCKD | SACR0_SYNCD);//ark1668e-i2s:slave mode
			writel(sacr0, i2s->base + I2S_SACR0);

			//writel(SAIMR_ROR, i2s->base + I2S_SAIMR);

			writel(0x7f, i2s->base + I2S_SAICR);
			writel(0, i2s->base + I2S_SAICR);
		}
	}
	udelay(1);
	sacr0 &= ~SACR0_CH_LOCK;
	writel(sacr0, i2s->base + I2S_SACR0);

	return 0;
}

static int ark1668e_i2s_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct ark1668e_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	u32 val;
#ifndef BOARD_ARK1668E_FPGA
	u32 rate = params_rate(params);
	u32 step = 256 * 2, modulo;
	u32 freq;
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
#endif

	val = readl(i2s->base + I2S_SACR0);
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		val &= ~SACR0_32BIT_MODE;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		val |= SACR0_32BIT_MODE;
		val &= ~(SACR0_RFTH_MASK | SACR0_TFTH_MASK);
		val |= SACR0_TFTH(7) | SACR0_RFTH(8);
		break;
	default:
		return -EINVAL;
	}

	if (params_channels(params) == 1)
		val |= SACR0_MOLO_MODE;

	writel(val, i2s->base + I2S_SACR0);

	return 0;
}


static int ark1668e_i2s_trigger(
	struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	struct ark1668e_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	int ret = 0;
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		if(!i2s->full_duplex_en){
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				writel(readl(i2s->base + I2S_SACR1) & ~SACR1_DRPL, i2s->base + I2S_SACR1);
			else
				writel(readl(i2s->base + I2S_SACR1) & ~SACR1_DREC, i2s->base + I2S_SACR1);
		}else{
			writel(readl(i2s->base + I2S_SACR1) & ~SACR1_DRPL, i2s->base + I2S_SACR1);
			writel(readl(i2s->base + I2S_SACR1) & ~SACR1_DREC, i2s->base + I2S_SACR1);
		}
		writel(readl(i2s->base + I2S_SACR0) | SACR0_ENB, i2s->base + I2S_SACR0);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
		/*if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			writel(readl(i2s->base + I2S_SACR1) | SACR1_DRPL, i2s->base + I2S_SACR1);
		else
			writel(readl(i2s->base + I2S_SACR1) | SACR1_DREC, i2s->base + I2S_SACR1);
		writel(readl(i2s->base + I2S_SACR0) & ~SACR0_ENB, i2s->base + I2S_SACR0); */
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static int ark1668e_i2s_set_fmt(
	struct snd_soc_dai *dai, unsigned int fmt)
{
	struct ark1668e_i2s_dev *i2s =snd_soc_dai_get_drvdata(dai);
	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		i2s->fmt = 0;
		break;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		dev_dbg(i2s->dev, "i2s master.\n");
		i2s->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		dev_dbg(i2s->dev, "i2s slave.\n");
		i2s->master = 0;
		break;
	default:
		break;
	}
	return 0;
}

static int ark1668e_i2s_probe(struct snd_soc_dai *dai)
{
	struct ark1668e_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	dai->capture_dma_data = &i2s->capture_dma_data;
	dai->playback_dma_data = &i2s->playback_dma_data;

	return 0;
}

/* I2S supported rate and format */
#define ARK1668E_I2S_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000)

static const struct snd_soc_dai_ops ark1668e_i2s_dai_ops = {
	.startup		= ark1668e_i2s_startup,
	.trigger 		= ark1668e_i2s_trigger,
	.hw_params 		= ark1668e_i2s_hw_params,
	.set_fmt 		= ark1668e_i2s_set_fmt,
};

static struct snd_soc_dai_driver ark1668e_i2s_dai = {
	.probe = ark1668e_i2s_probe,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = ARK1668E_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
					SNDRV_PCM_FMTBIT_S32_LE,},
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = ARK1668E_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
					SNDRV_PCM_FMTBIT_S32_LE,},
	.ops = &ark1668e_i2s_dai_ops,
	.symmetric_rates = 1,
};

static struct snd_pcm_hardware ark1668e_pcm_hardware = {
	.info 				= (SNDRV_PCM_INFO_MMAP |
 						SNDRV_PCM_INFO_MMAP_VALID |
						SNDRV_PCM_INFO_PAUSE |
 						SNDRV_PCM_INFO_RESUME |
						SNDRV_PCM_INFO_INTERLEAVED |
						SNDRV_PCM_INFO_BLOCK_TRANSFER),
	.formats 			= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
						SNDRV_PCM_FMTBIT_S32_LE,
	.rates 				= (SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 |
						SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 |
						SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |
						SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 |
						SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |
						SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000),
	.rate_min 			= 8000,
	.rate_max			= 192000,
	.channels_min 		= 1,
	.channels_max 		= 2,
	.buffer_bytes_max 	= 64 * 65536,
	.period_bytes_min 	= 64,
	.period_bytes_max 	= 65536,
	.periods_min 		= 1,
	.periods_max 		= 64,
};

static const struct snd_dmaengine_pcm_config
ark1668e_i2s_dmaengine_pcm_config = {
	.prepare_slave_config	= snd_dmaengine_pcm_prepare_slave_config,
	.pcm_hardware		= &ark1668e_pcm_hardware,
};

static const struct snd_soc_component_driver ark1668e_i2s_component = {
	.name		= DRV_NAME,
};

static irqreturn_t ark1668e_i2s_interrupt(int irq, void *dev_id)
{
	struct ark1668e_i2s_dev *i2s = dev_id;
	u32 status;

	status = readl(i2s->base + I2S_SASR0);

	dev_dbg(i2s->dev, "ark1668e_i2s_interrupt status=0x%x.0x%x.\n", status, readl(i2s->base + I2S_SACR0));
	//printk("ark1668e_i2s_interrupt status=0x%x.0x%x.\n", status, readl(i2s->base + I2S_SACR0));

	writel(status, i2s->base + I2S_SAICR);
	//writel(0, i2s->base + I2S_SAICR);

	return IRQ_HANDLED;
}

static int ark1668e_i2s_drv_probe(struct platform_device *pdev)
{
	struct ark1668e_i2s_dev *i2s;
	struct resource *res;
	u32 val;
	int ret = 0;

	i2s = devm_kzalloc(&pdev->dev, sizeof(struct ark1668e_i2s_dev), GFP_KERNEL);
	if (!i2s)
		return -ENOMEM;

	i2s->dev = &pdev->dev;

	//i2s resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2s->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2s->base))
		return PTR_ERR(i2s->base);

	if (!of_property_read_u32(pdev->dev.of_node, "nco-reg", &val))
		i2s->nco_reg = val;

	if (of_property_read_bool(pdev->dev.of_node, "full-duplex-mode"))
		i2s->full_duplex_en = 1;
	//printk(">>>>>>>>>>>>>>>>>>i2s->full_duplex_en = %d \n",i2s->full_duplex_en);

	i2s->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(i2s->clk))
		return PTR_ERR(i2s->clk);

	i2s->irq = platform_get_irq(pdev, 0);
	if (i2s->irq < 0)
		return i2s->irq;

	ret = devm_request_irq(i2s->dev, i2s->irq, ark1668e_i2s_interrupt,
			       IRQF_SHARED, KBUILD_MODNAME, i2s);
	if (ret)
		return ret;


	/* DMA parameters */
	i2s->playback_dma_data.addr = res->start + I2S_SADR;
	i2s->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->playback_dma_data.maxburst = 16;

	i2s->capture_dma_data.addr = res->start + I2S_SADR;
	i2s->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->capture_dma_data.maxburst = 16;

	dev_set_drvdata(&pdev->dev, i2s);

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &ark1668e_i2s_component,
					      &ark1668e_i2s_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI\n");
		return ret;
	}

	i2s_poweron(i2s);

	ret = devm_snd_dmaengine_pcm_register(&pdev->dev,
						&ark1668e_i2s_dmaengine_pcm_config,
						0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM\n");
		return ret;
	}

	return 0;
}

static const struct of_device_id ark1668e_i2s_match[] = {
	{ .compatible = "arkmicro,ark1668e-i2s", },
	{},
};

static struct platform_driver ark1668e_i2s_driver = {
	.probe 		= ark1668e_i2s_drv_probe,
	.driver		= {
		.name 	= DRV_NAME,
		.of_match_table = of_match_ptr(ark1668e_i2s_match),
	},
};
module_platform_driver(ark1668e_i2s_driver);

MODULE_DESCRIPTION("ARK I2S SoC Interface");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");

