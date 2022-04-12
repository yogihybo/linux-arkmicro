/*
 * ark_i2s.c -- ALSA SoC Audio Layer
 *
 * Jack Tang <jacktang@astri.org>
 *
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
#include <linux/clk.h>
#include <sound/dmaengine_pcm.h>
//#include <mach/va_map.h>
#include "ark1668_i2s.h"
//#include "ark1668_pcm.h"
#include "ark1668_i2s_sddac_regs.h"
//#include <linux/ark/ark_i2s.h>

#define DRV_NAME	"ark1668-i2s"
#define DMA_ENABLE

#undef ARK_I2S_DEBUG
#ifdef ARK_I2S_DEBUG
#define DBG(f, a...) printk(KERN_DEBUG "ASOC %s-%d: "f, __FUNCTION__, __LINE__, ##a)
#else
#define DBG(...)
#endif

#define ERR(f, a...) printk(KERN_ERR "ASOC %s-%d: "f, __FUNCTION__, __LINE__, ##a)


struct ark_i2s_dev {
	//struct  platform_device		*pdev;
	struct  device	*dev;
	void __iomem 	*base; 
	struct clk 				*clk;
	u32	nco_reg;
	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;
	int master;
	u32 fmt;
	//struct ark_pcm_dma_params 	dma_params[2];
};

static void i2s_poweron(struct ark_i2s_dev *i2s)
{
	uint32_t val;
//	void __iomem	*Sys_base;
//	void __iomem	*i2s_base;
//	Sys_base = ioremap(SYS_BASE, 0x1000);
//	if(!Sys_base)
//		goto unmap_sysreg;
//	i2s_base = ioremap(I2S_BASE, 0x1000);
//	if(!i2s_base)
//		goto unmap_i2sreg;
	
//	if(i2s->pdev->id == 0)
//	{
		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
		//val &= ~(ARK_I2SSDDAC_SACR0_VREF_PD | ARK_I2SSDDAC_SACR0_DAC_PD | ARK_I2SSDDAC_SACR0_SARADC_EN);
		val &= ~(ARK_I2SSDDAC_SACR0_VREF_PD | ARK_I2SSDDAC_SACR0_DAC_PD);
		val |= (ARK_I2SSDDAC_SACR0_SARADC_POW_EN | ARK_I2SSDDAC_SACR0_BCKD);	// Bitclock output
		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);
//	}
//	else if(i2s->pdev->id == 1)
//	{
//#if 0	//set in ark_i2s_init_cfg()
//		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
//		val &= ~(ARK_I2SSDDAC_SACR0_BCKD);		// Bitclock intput
//		val |= ARK_I2SSDDAC_SACR0_SARADC_POW_EN;
//		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);
//#endif
//	}

//unmap_sysreg:
//			iounmap(Sys_base);
//unmap_i2sreg:
//			iounmap(i2s_base);
}

static void setup_i2s2(int id)
{
}

/*static void dump_i2s_registers(struct ark_i2s_dev *i2s)
{
	printk("ARK_I2SSDDAC_SACR0  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_SACR0));
	printk("ARK_I2SSDDAC_SACR1  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_SACR1));
	printk("ARK_I2SSDDAC_SACR1  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_DACR0));
	printk("ARK_I2SSDDAC_SASR0  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_SASR0));
	printk("ARK_I2SSDDAC_DACR1  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_DACR1));
	printk("ARK_I2SSDDAC_SAIMR  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_SAIMR));
	printk("ARK_I2SSDDAC_SAICR  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_SAICR));
	printk("ARK_I2SSDDAC_ADCR0  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_ADCR0));
	printk("ARK_I2SSDDAC_SADR  = 0x%08x\n", readl(i2s->base + ARK_I2SSDDAC_SADR));
}*/


static void ark_i2s_txctrl(struct ark_i2s_dev *i2s, int on)
{
}

static void ark_i2s_rxctrl(struct ark_i2s_dev *i2s, int on)
{
}

static void ark_i2s_init_cfg(struct ark_i2s_dev *i2s, int id)
{

}

static int ark_i2s_startup(
	struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned int val;
	void __iomem	*Sys_base;
	void __iomem	*i2s_base;

	Sys_base = ioremap(SYS_BASE, 0x1000);
	if(!Sys_base)
		goto unmap_sysreg;

	//printk("==============[sys_base = 0x%08x   i2s->base = 0x%08x]\n",Sys_base,i2s->base);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	//val = readl(ARK1680_VA_SYS + ARK_SYS_PAD_CTRL06);
		//writel(val | ARK_SYS_I2S_DATA_DIR_OUT, ARK1680_VA_SYS + ARK_SYS_PAD_CTRL06);
		val = readl(Sys_base + ARK_SYS_PAD_CTRL0C);
		writel(val | ARK_SYS_I2S_MCLK_AUX, Sys_base + ARK_SYS_PAD_CTRL0C);

		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
		val |= ARK_I2SSDDAC_SACR0_I2SEN;//i2s enable
		// cancel pop noise
		//val |= ARK_I2SSDDAC_SACR0_BCKD;// Bitclock output
		val |= ARK_I2SSDDAC_SACR0_TFTH; // set tfth
		val |= ARK_I2SSDDAC_SACR0_SARADC_DATA; // set saradc data
		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);

		val = readl(i2s->base + ARK_I2SSDDAC_SAIMR);
		val &= ~(ARK_I2SSDDAC_SAIMR_TFS 
				|ARK_I2SSDDAC_SAIMR_TUR);
		writel(val, i2s->base + ARK_I2SSDDAC_SAIMR);

		val = readl(i2s->base + ARK_I2SSDDAC_SAICR);
		val |= (ARK_I2SSDDAC_SAICR_TXCR 
				|ARK_I2SSDDAC_SAICR_TUR);
		writel(val, i2s->base + ARK_I2SSDDAC_SAICR);

		val = readl(i2s->base + ARK_I2SSDDAC_SAICR);
		val &= ~(ARK_I2SSDDAC_SAICR_TXCR 
				|ARK_I2SSDDAC_SAICR_TUR);
		writel(val, i2s->base + ARK_I2SSDDAC_SAICR);

		
//		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
//		val |= ARK_I2SSDDAC_SACR0_TDMAENA;	// enable tx dma
//		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);

		val = readl(i2s->base + ARK_I2SSDDAC_SACR1);
		val &= ~ARK_I2SSDDAC_SACR1_DIS_PLAY;	//enable play
		writel(val, i2s->base + ARK_I2SSDDAC_SACR1);
		writel(0, i2s->base + ARK_I2SSDDAC_DACR0);

		val = readl(i2s->base + ARK_I2SSDDAC_DACR1);
		val |= ARK_I2SSDDAC_DACR1_LRSW; 	// left/right audio play channel switch
		writel(val, i2s->base + ARK_I2SSDDAC_DACR1);
	}
	else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE)
	{printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
		//
		val = readl(Sys_base + ARK_SYS_PAD_CTRL09);
		val |= (ARK_SYS_I2S2_BCLK
			|ARK_SYS_I2S2_SADATA
			|ARK_SYS_I2S2_SYNC);
		writel(val, Sys_base + ARK_SYS_PAD_CTRL09);
		val = readl(Sys_base + ARK_SYS_PAD_CTRL0A);
		val  |= (7<<8);
		writel(val, Sys_base + ARK_SYS_PAD_CTRL0A);
		val = readl(Sys_base + ARK_SYS_PAD_CTRL0C);
		val |= ARK_SYS_I2S_MCLK_AUX;
		writel(val, Sys_base + ARK_SYS_PAD_CTRL0C);
		//
		
		val = readl(Sys_base + ARK_SYS_PAD_CTRL06);
		val &= ~ARK_SYS_I2S_DATA_DIR_OUT;
		writel(val, Sys_base + ARK_SYS_PAD_CTRL06);

		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
		val &= ~(ARK_I2SSDDAC_SACR0_SARADC_VREF
				|ARK_I2SSDDAC_SACR0_SARADC_DATA
				|ARK_I2SSDDAC_SACR0_DACCLK_EDGE
				|ARK_I2SSDDAC_SACR0_RDMAENA
				|ARK_I2SSDDAC_SACR0_LOOPBACK
				|ARK_I2SSDDAC_SACR0_BCKD
				|ARK_I2SSDDAC_SACR0_SYNCD);
		val |= (ARK_I2SSDDAC_SACR0_MIC_LINE_SEL
				|ARK_I2SSDDAC_SACR0_SARADC_POW_EN
				|ARK_I2SSDDAC_SACR0_SARADC_EN
				|ARK_I2SSDDAC_SACR0_DAC_PD
				|ARK_I2SSDDAC_SACR0_VREF_PD
				|ARK_I2SSDDAC_SACR0_RFTH
#if defined(CONFIG_FM1288_Driver)
				|ARK_I2SSDDAC_SACR0_BCKD		//the module of fm1288 is weird, when capturing, 
				|ARK_I2SSDDAC_SACR0_SYNCD		//you must set the direction of the tow guys output as playback
#endif
				|ARK_I2SSDDAC_SACR0_I2SEN);
		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);

		val = readl(i2s->base + ARK_I2SSDDAC_SAIMR);
		val &= ~(ARK_I2SSDDAC_SAIMR_RFS 
				|ARK_I2SSDDAC_SAIMR_ROR);
		writel(val, i2s->base + ARK_I2SSDDAC_SAIMR);

		val = readl(i2s->base + ARK_I2SSDDAC_SAICR);
		val |= (ARK_I2SSDDAC_SAICR_RXCR 
				|ARK_I2SSDDAC_SAICR_ROR);
		writel(val, i2s->base + ARK_I2SSDDAC_SAICR);

		val = readl(i2s->base + ARK_I2SSDDAC_SAICR);
		val &= ~(ARK_I2SSDDAC_SAICR_RXCR 
				|ARK_I2SSDDAC_SAICR_ROR);
		writel(val, i2s->base + ARK_I2SSDDAC_SAICR);

//		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
//		val |= ARK_I2SSDDAC_SACR0_RDMAENA;		 // enable rx dma
//		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);

		val = readl(i2s->base + ARK_I2SSDDAC_SACR1);
		val &= ~ARK_I2SSDDAC_SACR1_DIS_REC; 	//enable record
		writel(val, i2s->base + ARK_I2SSDDAC_SACR1);
	}
	/* Tx/Rx Config */
	//snd_soc_dai_set_dma_data(dai, substream, &i2s->dma_params[substream->stream]);
unmap_sysreg:
			iounmap(Sys_base);	
	return 0;
}

static int ark_i2s_hw_params(
	struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{	//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	//struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
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
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	int ret = 0;
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	DBG("-->\n");

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* TODO: start i2s */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			ark_i2s_txctrl(i2s, 1);
		else
			ark_i2s_rxctrl(i2s, 1);
		/* TODO: Start DMA */
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* TODO: stop i2s */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			ark_i2s_txctrl(i2s, 0);
		else
			ark_i2s_rxctrl(i2s, 0);

		break;
	default:
		ret = -EINVAL;
		break;
	}
	
	return ret;
}

static int ark_i2s_set_fmt(
	struct snd_soc_dai *dai, unsigned int fmt)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	
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
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

#ifdef DMA_ENABLE
	dai->capture_dma_data = &i2s->capture_dma_data;
	dai->playback_dma_data = &i2s->playback_dma_data;
#endif
	return ret;
}

static int ark_i2s_remove(struct snd_soc_dai *dai)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	ark_i2s_txctrl(i2s, 0);
	ark_i2s_rxctrl(i2s, 0);

	return 0;
}

#ifdef CONFIG_PM
static int ark_i2s_suspend(struct snd_soc_dai *cpu_dai)
{//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);

	/* TODO: suspend i2s, disable clock */

	return 0;
}

static int ark_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	//printk("==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);

	/* TODO: enable clock and resume i2s */

	return 0;
}
#else
#define ark_i2s_suspend 	NULL
#define ark_i2s_resume 		NULL
#endif

/* I2S supported rate and format */
#define ARK_I2S_RATES \
	(SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | \
	SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 | \
	SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_176400 | SNDRV_PCM_RATE_192000 | SNDRV_PCM_RATE_8000)

#define ARK_I2S_FORMAT (SNDRV_PCM_FMTBIT_S16_LE) /* TODO: 18 and 20bits width */

static const struct snd_soc_dai_ops ark_i2s_dai_ops = {
	.startup		= ark_i2s_startup,
	.trigger 		= ark_i2s_trigger,
	.hw_params 	= ark_i2s_hw_params,
	.set_fmt 		= ark_i2s_set_fmt,
};

static struct snd_soc_dai_driver ark_i2s_dai = {
	.probe = ark_i2s_probe,
	.playback = {
		.channels_min 	= 1,
		.channels_max 	= 2,
		.rates = ARK_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
					SNDRV_PCM_FMTBIT_S32_LE,},
	.capture = {
		.channels_min 	= 2,
		.channels_max 	= 2,
		.rates = ARK_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
					SNDRV_PCM_FMTBIT_S32_LE,},
	.ops = &ark_i2s_dai_ops,
	.symmetric_rates = 1,
};

static struct snd_pcm_hardware ark1668_pcm_hardware = {
#if 0
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
	.buffer_bytes_max 	= 64 * 65536,//64 * 4096,
	.period_bytes_min 	= 64,
	.period_bytes_max 	= 65536,//4096,
	.periods_min 		= 1,
	.periods_max 		= 64,
#else
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
	.rate_min 			= 8000,//11025
	.rate_max			= 192000,
	.channels_min 		= 1,
	.channels_max 		= 2,
	.buffer_bytes_max 	= 64 * 4096,
	.period_bytes_min 	= 64,
	.period_bytes_max 	= 4096,
	.periods_min 		= 1,
	.periods_max 		= 64,
//	.fifo_size 			= 32,
#endif
};

static const struct snd_dmaengine_pcm_config 
ark1668_i2s_dmaengine_pcm_config = {
	.prepare_slave_config	= snd_dmaengine_pcm_prepare_slave_config,
	.pcm_hardware		= &ark1668_pcm_hardware,
};

static const struct snd_soc_component_driver ark1668_i2s_component = {
	.name		= DRV_NAME,
};

static int ark1668_i2s_drv_probe(struct platform_device *pdev)
{
	struct ark_i2s_dev *i2s;
	struct resource *mem;
	u32 val;
	int ret = 0;

	printk("start:==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	i2s = devm_kzalloc(&pdev->dev, sizeof(struct ark_i2s_dev), GFP_KERNEL);
	if (!i2s) {
		ERR("Failed to allocate ark_i2s_dev\n");
		return -ENOMEM;
	}
	
	i2s->dev = &pdev->dev;
	
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2s->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(i2s->base)) {
		dev_err(&pdev->dev, "no mem resource\n");
		return PTR_ERR(i2s->base);
	}
	printk(KERN_ALERT "dev %s 0x%x.\n", dev_name(&pdev->dev), mem->start);
	
	if (!of_property_read_u32(pdev->dev.of_node, "nco-reg", &val))
		i2s->nco_reg = val;
	
	/* Get i2s clock */
	i2s->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(i2s->clk)) {
		dev_err(&pdev->dev, "Cannot get the i2s clock\n");
		return PTR_ERR(i2s->clk);
	}

	/* DMA parameters */
#ifdef DMA_ENABLE
	i2s->playback_dma_data.addr = mem->start + ARK_I2SSDDAC_SADR;
	i2s->playback_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->playback_dma_data.maxburst = 16;

	i2s->capture_dma_data.addr = mem->start + ARK_I2SSDDAC_SADR;
	i2s->capture_dma_data.addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	i2s->capture_dma_data.maxburst = 16;
#endif
	/* Pre-assign snd_soc_dai_set_drvdata */
	dev_set_drvdata(&pdev->dev, i2s);

	ret = devm_snd_soc_register_component(&pdev->dev,
					      &ark1668_i2s_component,
					      &ark_i2s_dai, 1);
	if (ret !=0){
		dev_err(&pdev->dev, "Could not register DAI\n");
		return ret;
	}
	i2s_poweron(i2s);

	ret = devm_snd_dmaengine_pcm_register(&pdev->dev,
						&ark1668_i2s_dmaengine_pcm_config,
						0);
//	ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
	if (ret) {
		dev_err(&pdev->dev, "Could not register PCM\n");
		return ret;
	}
	
	printk("end:==============[%s]:[ %d]\n", __FUNCTION__, __LINE__);
	return 0;
}

//static int ark1668_i2s_drv_remove(struct platform_device *pdev)
//{
//	struct ark_i2s_dev *i2s = platform_get_drvdata(pdev);
//	struct resource *mem;

//	return 0;
//}

static const struct of_device_id ark1668_i2s_match[] = {
	{ .compatible = "arkmicro,ark1668-i2s", },
	{},
};

static struct platform_driver ark1668_i2s_driver = {
	.probe 		= ark1668_i2s_drv_probe,
	.driver		= {
		.name 	= DRV_NAME,
		.of_match_table = of_match_ptr(ark1668_i2s_match),
	},
};

module_platform_driver(ark1668_i2s_driver);


MODULE_AUTHOR("Jack Tang, jacktang@astri.org");
MODULE_DESCRIPTION("ARK I2S SoC Interface");
MODULE_LICENSE("GPL");

