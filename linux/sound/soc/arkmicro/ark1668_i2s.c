/*
 * ark_i2s.c -- ALSA SoC Audio Layer
 *
 * Jack Tang <jacktang@astri.org>
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/dma-mapping.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/clk.h>
#include <sound/dmaengine_pcm.h>
#include "ark1668_i2s.h"
#include "ark1668_i2s_sddac_regs.h"
#include "ark_i2s.h"	/* ark_audio_mute() -- see that header for why */

#define DRV_NAME	"ark1668-i2s"
#define DMA_ENABLE

/* 2026-08-05 AA audio-stutter investigation: bypasses the generic ASoC
 * dmaengine_pcm framework (devm_snd_dmaengine_pcm_register()) entirely,
 * replicating stock's own custom platform PCM driver -- ark_pcm_trigger
 * (0x802f5408), ark_pcm_prepare_dma (0x802f552c), ark_pcm_prepare
 * (0x802f55c8), ark_pcm_dma_period_done (0x802f5628), disassembled from
 * firmware_dumps/Prado firmware dump/mtd5_kernel/extracted/vmlinux.elf --
 * see docs/AUDIO_SUBSYSTEM_INVESTIGATION.md. Stock never called .pointer()
 * during normal playback the way the generic framework's own
 * dmaengine_pcm_dma_complete() does; its period callback goes straight to
 * snd_pcm_period_elapsed() with no independent hardware-position
 * reconciliation. That's the actual mechanism that lets stock's audio
 * tolerate the same DMA-controller-sharing/tasklet-priority conditions
 * (confirmed pre-existing, unmodified since this repo's first commit)
 * without ever hitting ALSA core's "Lost interrupts?" cross-check, which
 * only the generic framework's own residue-polling path can trigger.
 *
 * ark-dma.c's dw_dma_cyclic_* API isn't in a shared header (private to
 * drivers/dma/) -- struct dw_cyclic_desc below is a binary-compatible
 * mirror of drivers/dma/ark-dma.h's definition, declared here the same
 * way musb_host.c forward-declares musb_dma_channel_release() rather
 * than pulling in a foreign driver's private header. */
struct dw_cyclic_desc {
	void			*desc;		/* opaque to us */
	unsigned long		periods;	/* opaque to us */
	size_t			period_len;	/* opaque to us */
	void			(*period_callback)(void *param);
	void			*period_callback_param;
};

extern struct dw_cyclic_desc *dw_dma_cyclic_prep(struct dma_chan *chan,
		dma_addr_t buf_addr, size_t buf_len, size_t period_len,
		enum dma_transfer_direction direction);
extern void dw_dma_cyclic_free(struct dma_chan *chan);
extern int dw_dma_cyclic_start(struct dma_chan *chan);
extern void dw_dma_cyclic_stop(struct dma_chan *chan);
extern dma_addr_t dw_dma_get_src_addr(struct dma_chan *chan);
extern dma_addr_t dw_dma_get_dst_addr(struct dma_chan *chan);

#undef ARK_I2S_DEBUG
#ifdef ARK_I2S_DEBUG
#define DBG(f, a...) pr_debug("%s-%d: "f, __func__, __LINE__, ##a)
#else
#define DBG(...)
#endif

#define ERR(f, a...) pr_err("%s-%d: "f, __func__, __LINE__, ##a)


struct ark_i2s_dev {
	struct  device	*dev;
	void __iomem 	*base;
	struct clk 				*clk;
	u32	nco_reg;
	struct snd_dmaengine_dai_dma_data capture_dma_data;
	struct snd_dmaengine_dai_dma_data playback_dma_data;
	int master;
	u32 fmt;

	/* 2026-08-03: deferred-unmute timer, see ark_i2s_trigger()'s START
	 * case for why this replaced a synchronous mdelay(3). */
	struct timer_list unmute_timer;
};

static void i2s_poweron(struct ark_i2s_dev *i2s)
{
	uint32_t val;

	val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
	val &= ~(ARK_I2SSDDAC_SACR0_VREF_PD | ARK_I2SSDDAC_SACR0_DAC_PD);
	val |= (ARK_I2SSDDAC_SACR0_SARADC_POW_EN | ARK_I2SSDDAC_SACR0_BCKD);	// Bitclock output
	writel(val, i2s->base + ARK_I2SSDDAC_SACR0);
}


/* Deliberately empty, not an unfinished stub -- confirmed (2026-07-30
 * audio-stutter investigation, docs/AUDIO_SUBSYSTEM_INVESTIGATION.md)
 * against stock's own disassembled vmlinux that stock's equivalent
 * trigger-control functions are equally inert (`mov r0,#0; bx lr`).
 * The real hardware enable bits (I2SEN/TDMAENA/RDMAENA/SACR1 DIS_PLAY/
 * DIS_REC) are set once in ark_i2s_startup() at stream open, not per
 * trigger; DMA start/stop itself is owned entirely by the generic
 * ASoC dmaengine_pcm framework (devm_snd_dmaengine_pcm_register()
 * below), not this DAI driver. If re-flagged as a gap again, check
 * that history first before adding register writes here. */
static void ark_i2s_txctrl(struct ark_i2s_dev *i2s, int on)
{
}

static void ark_i2s_rxctrl(struct ark_i2s_dev *i2s, int on)
{
}

static int ark_i2s_startup(
	struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned int val;
	void __iomem	*Sys_base;
	struct platform_device *pdev = to_platform_device(i2s->dev);
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	unsigned long physical_base = res ? res->start : 0;

	Sys_base = ioremap(SYS_BASE, 0x1000);
	if(!Sys_base)
		goto unmap_sysreg;

	if (physical_base == I2S1_BASE) {
		// External I2S (I2S2/ADC block) pinmux configuration
		val = readl(Sys_base + ARK_SYS_PAD_CTRL09);
		val |= (ARK_SYS_I2S2_BCLK | ARK_SYS_I2S2_SADATA | ARK_SYS_I2S2_SYNC);
		writel(val, Sys_base + ARK_SYS_PAD_CTRL09);

		val = readl(Sys_base + ARK_SYS_PAD_CTRL0A);
		val |= (7 << 8);
		writel(val, Sys_base + ARK_SYS_PAD_CTRL0A);

		val = readl(Sys_base + ARK_SYS_PAD_CTRL0C);
		val |= ARK_SYS_I2S_MCLK_AUX;
		writel(val, Sys_base + ARK_SYS_PAD_CTRL0C);

		val = readl(Sys_base + ARK_SYS_PAD_CTRL06);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			val |= ARK_SYS_I2S_DATA_DIR_OUT; /* output mode for external DAC */
		} else {
			val &= ~ARK_SYS_I2S_DATA_DIR_OUT; /* input mode for external ADC */
		}
		writel(val, Sys_base + ARK_SYS_PAD_CTRL06);
	} else {
		// Internal I2S (I2S1/DAC block)
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			val = readl(Sys_base + ARK_SYS_PAD_CTRL0C);
			writel(val | ARK_SYS_I2S_MCLK_AUX, Sys_base + ARK_SYS_PAD_CTRL0C);
		}
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	{
		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
		val |= ARK_I2SSDDAC_SACR0_I2SEN;//i2s enable
		// cancel pop noise
		//val |= ARK_I2SSDDAC_SACR0_BCKD;// Bitclock output
		val |= ARK_I2SSDDAC_SACR0_TFTH; // set tfth
		val |= ARK_I2SSDDAC_SACR0_SARADC_DATA; // set saradc data
		// The CAPTURE branch below explicitly powers down the DAC and its
		// voltage reference (DAC_PD/VREF_PD) since they're not needed for
		// recording -- but this playback branch never powered them back
		// up, only ever OR'ing in unrelated bits via read-modify-write.
		// So the DAC/vref stay powered down across a prior capture, and
		// quite possibly by default at reset -- meaning digital I2S/DMA
		// could be (and, per live testing 2026-07-16, was) completely
		// correct with zero errors while no analog signal could ever
		// reach the speaker. See docs/AUDIO_SUBSYSTEM_INVESTIGATION.md.
		val &= ~(ARK_I2SSDDAC_SACR0_DAC_PD | ARK_I2SSDDAC_SACR0_VREF_PD);
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


		// Re-enabled (2026-07-13) -- was left commented out, so the I2S
		// block never asserted its hardware DMA-request line for TX.
		// devm_snd_dmaengine_pcm_register()'s generic dmaengine PCM
		// framework can arm/start the DMA *channel*, but without this
		// bit the peripheral itself never requests a transfer, so the
		// stream never actually progresses -- matches the observed
		// ALSA "unable to start PCM stream" failure (clean probe/open/
		// hw_params, silent failure only at trigger/start). See
		// docs/AUDIO_SUBSYSTEM_INVESTIGATION.md.
		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
		val |= ARK_I2SSDDAC_SACR0_TDMAENA;	// enable tx dma
		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);

		val = readl(i2s->base + ARK_I2SSDDAC_SACR1);
		val &= ~ARK_I2SSDDAC_SACR1_DIS_PLAY;	//enable play
		writel(val, i2s->base + ARK_I2SSDDAC_SACR1);

		/* Removed (2026-07-30): unconditional writel(0, DACR0) here zeroed
		 * both L/R volume on every single playback start/restart, racing
		 * with whatever volume the codec driver (ark_sddac_mute/set_l_r_
		 * playback_volume) had already set. Stock's real equivalent
		 * function (ark_i2s_init_cfg, decompiled from vmlinux.elf) never
		 * touches DACR0 at all -- that register is owned exclusively by
		 * the codec driver on stock. Confirmed no stock counterpart; see
		 * docs/AUDIO_SUBSYSTEM_INVESTIGATION.md for the comparison this
		 * came from. Given this ran on every XRUN-driven trigger restart
		 * (already-confirmed to happen up to ~24x/sec during a stutter
		 * burst), this was adding an extra, unnecessary volume-register
		 * write at exactly the same moments already under suspicion. */

		val = readl(i2s->base + ARK_I2SSDDAC_DACR1);
		val |= ARK_I2SSDDAC_DACR1_LRSW; 	// left/right audio play channel switch
		writel(val, i2s->base + ARK_I2SSDDAC_DACR1);
	}
	else if(substream->stream == SNDRV_PCM_STREAM_CAPTURE)
	{
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

		/* 2026-08-03 AA mic-quality investigation
		 * (docs/AUDIO_SUBSYSTEM_INVESTIGATION.md / project_mic_capture_investigation):
		 * SARADC_POW_EN was just asserted above in the same register
		 * write as everything else, with nothing giving the SAR-ADC's
		 * voltage reference (VREF) time to settle before RDMAENA
		 * enables the RX DMA a few lines below and real sampling
		 * begins. This is the same class of gap already found (and
		 * fixed) on the playback side -- ark_i2s_trigger()'s
		 * mdelay(3) settle window between starting the DAC and
		 * unmuting -- just never applied here. No datasheet-derived
		 * settle time is available for this specific SAR-ADC, so 3ms
		 * is a starting-point guess by analogy to the playback fix,
		 * not a measured value -- revisit if voice-recognition
		 * transcription quality doesn't improve, or if it turns out
		 * capture-direction ALSA XRUNs (dmesg "XRUN: pcmC0D0c",
		 * CONFIG_SND_PCM_XRUN_DEBUG) are the dominant mechanism
		 * instead. */
		mdelay(3);

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

		// Re-enabled (2026-07-13), same reasoning as TDMAENA above --
		// RDMAENA was explicitly cleared a few lines up as part of the
		// SACR0 mask/set sequence and never re-enabled here.
		val = readl(i2s->base + ARK_I2SSDDAC_SACR0);
		val |= ARK_I2SSDDAC_SACR0_RDMAENA;		 // enable rx dma
		writel(val, i2s->base + ARK_I2SSDDAC_SACR0);

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
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	u32 rate = params_rate(params);
	u32 step = 256 * 2, modulo;
	u32 val, freq;
	void *sysreg;

	/* AA audio-stutter investigation (resolved 2026-08-05): rare event
	 * (stream open/format-change, not per-period).
	 */
	dev_dbg(i2s->dev, "hw_params stream=%d rate=%u period_size=%u periods=%u format=%d\n",
		substream->stream, rate,
		(unsigned int)params_period_size(params), params_periods(params),
		params_format(params));

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

/* 2026-08-03: deferred-unmute callback, see ark_i2s_trigger()'s START
 * case for why this replaced a synchronous mdelay(3) there. Runs in
 * softirq (timer) context, not atomic/IRQ-disabled the way trigger()
 * itself is -- ark_audio_mute() is a plain register read-modify-write,
 * safe to call from here. */
static void ark_i2s_unmute_timer_cb(struct timer_list *t)
{
	/* Container lookup not actually needed -- ark_audio_mute() targets
	 * the single shared DAC instance on this board (see
	 * ark1668-sddac-codec.c), not a specific ark_i2s_dev. t is unused
	 * beyond identifying which timer fired. */
	ark_audio_mute(0);
}

static int ark_i2s_trigger(
	struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	DBG("-->\n");

	/* AA audio-stutter investigation (resolved 2026-08-05,
	 * docs/AUDIO_SUBSYSTEM_INVESTIGATION.md): trigger events are rare
	 * (once per start/stop/pause, not per-period).
	 */
	dev_dbg(i2s->dev, "trigger cmd=%d stream=%d (%s) at %lluns\n",
		cmd, substream->stream,
		substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "playback" : "capture",
		ktime_to_ns(ktime_get()));

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/* 2026-07-30: mute -> start -> settle delay -> unmute,
			 * matching stock's real custom ark_pcm_trigger() (see
			 * docs/AUDIO_SUBSYSTEM_INVESTIGATION.md) -- stock mutes
			 * before dw_dma_cyclic_start(), waits ~2.5ms, then
			 * unmutes, giving the DMA/FIFO a settle window before
			 * audio is audible again. Previously this driver had no
			 * equivalent: the real hardware mute was instead wired
			 * into ASoC's automatic .digital_mute callback (now a
			 * no-op, see ark1668-sddac-codec.c), which fires on
			 * ASoC's own internal timing with no synchronization to
			 * DMA readiness -- during an XRUN-recovery storm
			 * (confirmed up to ~24 trigger cycles/sec), that meant
			 * dozens of unsynchronized mute/unmute clicks instead of
			 * clean, settled transitions.
			 *
			 * 2026-08-03 CRITICAL CORRECTION: the mdelay(3) this
			 * comment used to describe was a real bug, not just a
			 * missed optimization. snd_pcm_start() (sound/core/
			 * pcm_native.c) wraps the entire .trigger() dispatch in
			 * snd_pcm_stream_lock_irq() -- genuine local_irq_disable()
			 * -- around *every* call to this function, including the
			 * ones ALSA's own start_threshold logic issues
			 * automatically after each XRUN-recovery snd_pcm_prepare().
			 * A synchronous mdelay(3) here was therefore blocking
			 * *all* interrupts system-wide (network RX, USB, timers,
			 * the audio DMA IRQ itself) for the full 3ms, and at the
			 * documented ~24 trigger cycles/sec during a storm, that
			 * is ~72ms/sec of total interrupt blackout concentrated
			 * in exactly the moments this single-core system can
			 * least afford it -- actively compounding the XRUN storm
			 * this fix was meant to make audible, not causing it.
			 * Fixed by deferring the unmute to i2s->unmute_timer
			 * (see struct ark_i2s_dev) instead of blocking here --
			 * mute and the settle window are preserved, but trigger()
			 * now returns immediately, giving ALSA core back its IRQs
			 * right away. */
			ark_audio_mute(1);
			ark_i2s_txctrl(i2s, 1);
			mod_timer(&i2s->unmute_timer, jiffies + msecs_to_jiffies(3));
		} else {
			ark_i2s_rxctrl(i2s, 1);
		}
		/* DMA start is owned entirely by the generic ASoC dmaengine_pcm
		 * framework (devm_snd_dmaengine_pcm_register() below), not this
		 * DAI trigger callback -- nothing to do here. */
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			/* del_timer() (not _sync()) -- this runs with IRQs
			 * disabled too (same snd_pcm_stream_lock_irq() wrapper
			 * as START), so it must not block waiting for the timer
			 * callback to finish if it's mid-flight; a race where
			 * the callback's ark_audio_mute(0) runs microseconds
			 * after this ark_audio_mute(1) is harmless (mute is
			 * simply reasserted on the very next STOP/START), the
			 * real hazard being a *stale* unmute firing long after
			 * this stream has stopped, which del_timer() prevents
			 * in the common case. */
			del_timer(&i2s->unmute_timer);
			ark_audio_mute(1);
			ark_i2s_txctrl(i2s, 0);
		} else {
			ark_i2s_rxctrl(i2s, 0);
		}

		break;
	default:
		ret = -EINVAL;
		break;
	}
	
	return ret;
}

static int ark_i2s_set_fmt(
	struct snd_soc_dai *dai, unsigned int fmt)
{
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
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

#ifdef DMA_ENABLE
	dai->capture_dma_data = &i2s->capture_dma_data;
	dai->playback_dma_data = &i2s->playback_dma_data;
#endif
	return ret;
}

static int ark_i2s_remove(struct snd_soc_dai *dai)
{
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);

	del_timer_sync(&i2s->unmute_timer);
	ark_i2s_txctrl(i2s, 0);
	ark_i2s_rxctrl(i2s, 0);

	return 0;
}

#ifdef CONFIG_PM
static int ark_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
	/* TODO: suspend i2s, disable clock */

	return 0;
}

static int ark_i2s_resume(struct snd_soc_dai *cpu_dai)
{

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
	.remove = ark_i2s_remove,
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
	.channels_min 		= 1,
	.channels_max 		= 2,
	.buffer_bytes_max 	= 64 * 4096,
	.period_bytes_min 	= 64,
	.period_bytes_max 	= 4096,
	.periods_min 		= 1,
	.periods_max 		= 64,
};

struct ark_pcm_rtd {
	struct dma_chan		*chan;
	struct dw_cyclic_desc	*cdesc;
	bool			prepared;
};

/* Matches stock's ark_pcm_dma_period_done (0x802f5628) exactly: no
 * residue read, no position reconciliation -- just forward the
 * interrupt-driven notification. This is what avoids ever engaging
 * ALSA core's independent hw_ptr cross-check ("Lost interrupts?"),
 * unlike the generic dmaengine_pcm framework's own
 * dmaengine_pcm_dma_complete(). */
static void ark_pcm_dma_period_done(void *param)
{
	struct snd_pcm_substream *substream = param;

	if (substream)
		snd_pcm_period_elapsed(substream);
}

static int ark_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = rtd->cpu_dai;
	struct ark_pcm_rtd *prtd;
	int ret;

	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (!prtd)
		return -ENOMEM;

	prtd->chan = dma_request_slave_channel(dai->dev,
		substream->stream == SNDRV_PCM_STREAM_PLAYBACK ? "tx" : "rx");
	if (!prtd->chan) {
		dev_err(dai->dev, "ark_pcm_open: failed to get DMA channel\n");
		kfree(prtd);
		return -ENODEV;
	}

	ret = snd_pcm_hw_constraint_integer(substream->runtime,
					     SNDRV_PCM_HW_PARAM_PERIODS);
	if (ret < 0) {
		dma_release_channel(prtd->chan);
		kfree(prtd);
		return ret;
	}

	snd_soc_set_runtime_hwparams(substream, &ark1668_pcm_hardware);
	substream->runtime->private_data = prtd;
	return 0;
}

static int ark_pcm_close(struct snd_pcm_substream *substream)
{
	struct ark_pcm_rtd *prtd = substream->runtime->private_data;

	dma_release_channel(prtd->chan);
	kfree(prtd);
	return 0;
}

static int ark_pcm_hw_params(struct snd_pcm_substream *substream,
			      struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = rtd->cpu_dai;
	struct ark_i2s_dev *i2s = snd_soc_dai_get_drvdata(dai);
	struct ark_pcm_rtd *prtd = substream->runtime->private_data;
	struct snd_dmaengine_dai_dma_data *dma_data;
	struct dma_slave_config slave_config;
	int ret;

	ret = snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
	if (ret < 0)
		return ret;

	dma_data = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			&i2s->playback_dma_data : &i2s->capture_dma_data;

	memset(&slave_config, 0, sizeof(slave_config));
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		slave_config.direction = DMA_MEM_TO_DEV;
		slave_config.dst_addr = dma_data->addr;
		slave_config.dst_addr_width = dma_data->addr_width;
		slave_config.dst_maxburst = dma_data->maxburst;
	} else {
		slave_config.direction = DMA_DEV_TO_MEM;
		slave_config.src_addr = dma_data->addr;
		slave_config.src_addr_width = dma_data->addr_width;
		slave_config.src_maxburst = dma_data->maxburst;
	}

	ret = dmaengine_slave_config(prtd->chan, &slave_config);
	if (ret) {
		dev_err(dai->dev, "ark_pcm_hw_params: slave_config failed: %d\n", ret);
		snd_pcm_lib_free_pages(substream);
		return ret;
	}
	return 0;
}

static int ark_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct ark_pcm_rtd *prtd = substream->runtime->private_data;

	if (prtd->prepared) {
		dw_dma_cyclic_free(prtd->chan);
		prtd->prepared = false;
	}
	return snd_pcm_lib_free_pages(substream);
}

static int ark_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ark_pcm_rtd *prtd = runtime->private_data;
	enum dma_transfer_direction direction;
	unsigned long flags;

	if (prtd->prepared)
		return 0;

	direction = substream->stream == SNDRV_PCM_STREAM_PLAYBACK ?
			DMA_MEM_TO_DEV : DMA_DEV_TO_MEM;

	prtd->cdesc = dw_dma_cyclic_prep(prtd->chan, runtime->dma_addr,
			frames_to_bytes(runtime, runtime->buffer_size),
			frames_to_bytes(runtime, runtime->period_size),
			direction);
	if (IS_ERR(prtd->cdesc))
		return PTR_ERR(prtd->cdesc);

	prtd->cdesc->period_callback = ark_pcm_dma_period_done;
	prtd->cdesc->period_callback_param = substream;

	/* Tiny critical section marking "prepared", matching stock's own
	 * cpsid-i-protected read-modify-write in ark_pcm_prepare_dma --
	 * not a big lock, just keeps the flag update atomic against IRQs. */
	local_irq_save(flags);
	prtd->prepared = true;
	local_irq_restore(flags);

	return 0;
}

static int ark_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct ark_pcm_rtd *prtd = substream->runtime->private_data;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		return dw_dma_cyclic_start(prtd->chan);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		dw_dma_cyclic_stop(prtd->chan);
		return 0;
	default:
		return -EINVAL;
	}
}

/* Matches stock's ark_pcm_pointer (0x802f52b4): playback reads the
 * *source* address (the memory side advances for MEM_TO_DEV; the FIFO
 * destination is fixed), capture reads the *destination* address (the
 * memory side advances for DEV_TO_MEM; the FIFO source is fixed). */
static snd_pcm_uframes_t ark_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ark_pcm_rtd *prtd = runtime->private_data;
	dma_addr_t pos;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		pos = dw_dma_get_src_addr(prtd->chan);
	else
		pos = dw_dma_get_dst_addr(prtd->chan);

	if (pos < runtime->dma_addr)
		return 0;

	return bytes_to_frames(runtime, pos - runtime->dma_addr) % runtime->buffer_size;
}

static int ark_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	int ret;

	ret = dma_coerce_mask_and_coherent(card->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	return snd_pcm_lib_preallocate_pages_for_all(rtd->pcm,
			SNDRV_DMA_TYPE_DEV, card->dev,
			ark1668_pcm_hardware.buffer_bytes_max,
			ark1668_pcm_hardware.buffer_bytes_max);
}

static const struct snd_pcm_ops ark_pcm_ops = {
	.open		= ark_pcm_open,
	.close		= ark_pcm_close,
	.hw_params	= ark_pcm_hw_params,
	.hw_free	= ark_pcm_hw_free,
	.prepare	= ark_pcm_prepare,
	.trigger	= ark_pcm_trigger,
	.pointer	= ark_pcm_pointer,
};

static const struct snd_soc_component_driver ark1668_i2s_component = {
	.name		= DRV_NAME,
	.ops		= &ark_pcm_ops,
	.pcm_new	= ark_pcm_new,
};

static void ark_i2s_clk_disable(void *data)
{
	clk_disable_unprepare(data);
}

static int ark1668_i2s_drv_probe(struct platform_device *pdev)
{
	struct ark_i2s_dev *i2s;
	struct resource *mem;
	u32 val;
	int ret = 0;

	dev_dbg(&pdev->dev, "probe start\n");
	i2s = devm_kzalloc(&pdev->dev, sizeof(struct ark_i2s_dev), GFP_KERNEL);
	if (!i2s) {
		ERR("Failed to allocate ark_i2s_dev\n");
		return -ENOMEM;
	}
	
	i2s->dev = &pdev->dev;
	timer_setup(&i2s->unmute_timer, ark_i2s_unmute_timer_cb, 0);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	i2s->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(i2s->base)) {
		dev_err(&pdev->dev, "no mem resource\n");
		return PTR_ERR(i2s->base);
	}
	dev_info(&pdev->dev, "mem resource at 0x%x\n", mem->start);

	// Release I2S controller soft reset (bit 0 for I2S2/ADC, bit 2 for I2S1/DAC)
	{
		void __iomem *sys_base = ioremap(SYS_BASE, 0x1000);
		if (sys_base) {
			u32 reset_val = readl(sys_base + 0x6c);
			if (mem->start == I2S1_BASE) {
				reset_val &= ~(1 << 0); /* Release I2S2 reset */
				dev_info(&pdev->dev, "releasing soft reset for I2S2 (ADC/external I2S) at 0xe8200000\n");
			} else if (mem->start == I2S_BASE) {
				reset_val &= ~(1 << 2); /* Release I2S1 reset */
				dev_info(&pdev->dev, "releasing soft reset for I2S1 (DAC/internal I2S) at 0xe4000000\n");
			}
			writel(reset_val, sys_base + 0x6c);
			iounmap(sys_base);
		}
	}

	/*
	 * Stock 3.4 kernel's ark_i2s_init_cfg()/setup_i2s2() (disassembled from
	 * vmlinux.elf, see docs/AUDIO_SUBSYSTEM_INVESTIGATION.md "CS4334 external
	 * I2S clock-gate register" section) additionally touches a register block
	 * at physical 0xe4a00000 -- the same page ark1668.dtsi's `timer@e4a00000`
	 * node covers, but these offsets (0x1d8-0x1f0) sit well above anything the
	 * timer driver itself uses (its counters live in the low offsets) and are
	 * almost certainly shared SoC clock-gate/mux bits for the audio block.
	 * Neither SYS_BASE (0xe4900000, pinctrl/pad-config) nor the soft-reset
	 * register above cover this page -- without these bits the external I2S2
	 * peripheral (feeding the CS4334 DAC) never actually gets its output
	 * clock domain enabled at the SoC level, even though probe/hw_params/
	 * DMA all report success. Not yet hardware-verified -- see the doc.
	 */
	{
		void __iomem *audio_clk_base = ioremap(0xe4a00000, 0x1000);
		if (audio_clk_base) {
			if (mem->start == I2S1_BASE) {
				val = readl(audio_clk_base + 0x1e4);
				writel(val | 0x3f000000, audio_clk_base + 0x1e4);
				val = readl(audio_clk_base + 0x1e8);
				writel(val | 0x700, audio_clk_base + 0x1e8);
				val = readl(audio_clk_base + 0x1f0);
				writel(val | 0x400, audio_clk_base + 0x1f0);
				val = readl(audio_clk_base + 0x1d8);
				writel(val & ~0x80000000, audio_clk_base + 0x1d8);
				dev_info(&pdev->dev, "enabled external I2S2/CS4334 clock-gate bits at 0xe4a00000\n");
			} else if (mem->start == I2S_BASE) {
				val = readl(audio_clk_base + 0x1f0);
				writel(val | 0x400, audio_clk_base + 0x1f0);
				dev_info(&pdev->dev, "enabled internal I2S1 clock-gate bit at 0xe4a00000+0x1f0\n");
			}
			iounmap(audio_clk_base);
		}
	}

	if (!of_property_read_u32(pdev->dev.of_node, "nco-reg", &val))
		i2s->nco_reg = val;
	
	/* Get i2s clock */
	i2s->clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(i2s->clk)) {
		dev_err(&pdev->dev, "Cannot get the i2s clock\n");
		return PTR_ERR(i2s->clk);
	}

	ret = clk_prepare_enable(i2s->clk);
	if (ret) {
		dev_err(&pdev->dev, "Cannot enable the i2s clock\n");
		return ret;
	}
	ret = devm_add_action_or_reset(&pdev->dev, ark_i2s_clk_disable, i2s->clk);
	if (ret) {
		return ret;
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

	/* PCM ops are now registered directly via ark1668_i2s_component's
	 * .ops/.pcm_new (custom platform driver bypassing the generic
	 * dmaengine_pcm framework, see the top-of-file comment) -- no
	 * separate devm_snd_dmaengine_pcm_register() call needed. */

	dev_dbg(&pdev->dev, "probe end\n");
	return 0;
}

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

