// SPDX-License-Identifier:  GPL-2.0
// (C) 2017-2018 Synopsys, Inc. (www.synopsys.com)

/*
 * Synopsys DesignWare AXI DMA Controller driver.
 *
 * Author: Eugeniy Paltsev <Eugeniy.Paltsev@synopsys.com>
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/types.h>
#include <linux/of_dma.h>

#include "ark-axi-dma.h"
#include "dmaengine.h"
#include "virt-dma.h"

/*
 * This supports the Synopsys "DesignWare AHB Central DMA Controller",
 * (DW_ahb_dmac) which is used with various AMBA 2.0 systems (not all
 * of which use ARM any more).  See the "Databook" from Synopsys for
 * information beyond what licensees probably provide.
 *
 * The driver has been tested with the Atmel AT32AP7000, which does not
 * support descriptor writeback.
 */

#define DWC_DEFAULT_CTLLO(_chan) ({				\
		struct axi_dma_chan *_dwc = dchan_to_axi_dma_chan(_chan);	\
		struct dma_slave_config	*_sconfig = &_dwc->dma_sconfig;	\
		bool _is_slave = is_slave_direction(_dwc->direction);	\
		u8 _smsize = _is_slave ? _sconfig->src_maxburst :	\
			DWAXIDMAC_BURST_TRANS_LEN_16;			\
		u8 _dmsize = _is_slave ? _sconfig->dst_maxburst :	\
			DWAXIDMAC_BURST_TRANS_LEN_16;			\
		u8 _dms = (_dwc->direction == DMA_MEM_TO_DEV) ?		\
			_dwc->dws.p_master : _dwc->dws.m_master;	\
		u8 _sms = (_dwc->direction == DMA_DEV_TO_MEM) ?		\
			_dwc->dws.p_master : _dwc->dws.m_master;	\
								\
		(DWC_CTLL_DST_MSIZE(_dmsize)			\
		 | DWC_CTLL_SRC_MSIZE(_smsize)			\
		 | DWC_CTLL_DMS(_dms)				\
		 | DWC_CTLL_SMS(_sms));				\
	})

/*
 * The set of bus widths supported by the DMA controller. DW AXI DMAC supports
 * master data bus width up to 512 bits (for both AXI master interfaces), but
 * it depends on IP block configurarion.
 */
#define AXI_DMA_BUSWIDTHS		  \
	(DMA_SLAVE_BUSWIDTH_1_BYTE	| \
	DMA_SLAVE_BUSWIDTH_2_BYTES	| \
	DMA_SLAVE_BUSWIDTH_4_BYTES	| \
	DMA_SLAVE_BUSWIDTH_8_BYTES	| \
	DMA_SLAVE_BUSWIDTH_16_BYTES	| \
	DMA_SLAVE_BUSWIDTH_32_BYTES	| \
	DMA_SLAVE_BUSWIDTH_64_BYTES)

static inline void
axi_dma_iowrite32(struct axi_dma_chip *chip, u32 reg, u32 val)
{
	iowrite32(val, chip->regs + reg);
}

static inline u32 axi_dma_ioread32(struct axi_dma_chip *chip, u32 reg)
{
	return ioread32(chip->regs + reg);
}

static inline void
axi_chan_iowrite32(struct axi_dma_chan *chan, u32 reg, u32 val)
{
	iowrite32(val, chan->chan_regs + reg);
}

static inline u32 axi_chan_ioread32(struct axi_dma_chan *chan, u32 reg)
{
	return ioread32(chan->chan_regs + reg);
}

static inline void
axi_chan_iowrite64(struct axi_dma_chan *chan, u32 reg, u64 val)
{
	/*
	 * We split one 64 bit write for two 32 bit write as some HW doesn't
	 * support 64 bit access.
	 */
	iowrite32(lower_32_bits(val), chan->chan_regs + reg);
	iowrite32(upper_32_bits(val), chan->chan_regs + reg + 4);
}

static inline void axi_dma_disable(struct axi_dma_chip *chip)
{
	u32 val;

	val = axi_dma_ioread32(chip, DMAC_CFG);
	val &= ~DMAC_EN_MASK;
	axi_dma_iowrite32(chip, DMAC_CFG, val);
}

static inline void axi_dma_enable(struct axi_dma_chip *chip)
{
	u32 val;

	val = axi_dma_ioread32(chip, DMAC_CFG);
	val |= DMAC_EN_MASK;
	axi_dma_iowrite32(chip, DMAC_CFG, val);
}

static inline void axi_dma_irq_disable(struct axi_dma_chip *chip)
{
	u32 val;

	val = axi_dma_ioread32(chip, DMAC_CFG);
	val &= ~INT_EN_MASK;
	axi_dma_iowrite32(chip, DMAC_CFG, val);
}

static inline void axi_dma_irq_enable(struct axi_dma_chip *chip)
{
	u32 val;

	val = axi_dma_ioread32(chip, DMAC_CFG);
	val |= INT_EN_MASK;
	axi_dma_iowrite32(chip, DMAC_CFG, val);
}

static inline void axi_chan_irq_disable(struct axi_dma_chan *chan, u32 irq_mask)
{
	u32 val;

	if (likely(irq_mask == DWAXIDMAC_IRQ_ALL)) {
		axi_chan_iowrite32(chan, CH_INTSTATUS_ENA, DWAXIDMAC_IRQ_NONE);
	} else {
		val = axi_chan_ioread32(chan, CH_INTSTATUS_ENA);
		val &= ~irq_mask;
		axi_chan_iowrite32(chan, CH_INTSTATUS_ENA, val);
	}
}

static inline void axi_chan_irq_set(struct axi_dma_chan *chan, u32 irq_mask)
{
	axi_chan_iowrite32(chan, CH_INTSTATUS_ENA, irq_mask);
}

static inline void axi_chan_irq_sig_set(struct axi_dma_chan *chan, u32 irq_mask)
{
	axi_chan_iowrite32(chan, CH_INTSIGNAL_ENA, irq_mask);
}

static inline void axi_chan_irq_clear(struct axi_dma_chan *chan, u32 irq_mask)
{
	axi_chan_iowrite32(chan, CH_INTCLEAR, irq_mask);
}

static inline bool axi_chan_is_hw_enable(struct axi_dma_chan *chan)
{
	u32 val;

	val = axi_dma_ioread32(chan->chip, DMAC_CHEN);

	return !!(val & (BIT(chan->id) << DMAC_CHAN_EN_SHIFT));
}

static inline u32 axi_chan_irq_read(struct axi_dma_chan *chan)
{
	return axi_chan_ioread32(chan, CH_INTSTATUS);
}

static inline void axi_chan_disable(struct axi_dma_chan *chan)
{
	u32 val;

	val = axi_dma_ioread32(chan->chip, DMAC_CHEN);
	val &= ~(BIT(chan->id) << DMAC_CHAN_EN_SHIFT);
	val |=   BIT(chan->id) << DMAC_CHAN_EN_WE_SHIFT;
	axi_dma_iowrite32(chan->chip, DMAC_CHEN, val);
}

static inline void axi_chan_enable(struct axi_dma_chan *chan)
{
	u32 val;

	val = axi_dma_ioread32(chan->chip, DMAC_CHEN);
	val |= BIT(chan->id) << DMAC_CHAN_EN_SHIFT |
	       BIT(chan->id) << DMAC_CHAN_EN_WE_SHIFT;
	axi_dma_iowrite32(chan->chip, DMAC_CHEN, val);
}

static void axi_dma_hw_init(struct axi_dma_chip *chip)
{
	u32 i;

	for (i = 0; i < chip->dw->hdata->nr_channels; i++) {
		axi_chan_irq_disable(&chip->dw->chan[i], DWAXIDMAC_IRQ_ALL);
		axi_chan_disable(&chip->dw->chan[i]);
	}
}

static u32 axi_chan_get_xfer_width(struct axi_dma_chan *chan, dma_addr_t src,
				   dma_addr_t dst, size_t len)
{
	u32 max_width = chan->chip->dw->hdata->m_data_width;

	return __ffs(src | dst | len | BIT(max_width));
}

static inline const char *axi_chan_name(struct axi_dma_chan *chan)
{
	return dma_chan_name(&chan->vc.chan);
}

static struct axi_dma_desc *axi_desc_get(struct axi_dma_chan *chan)
{
	struct dw_axi_dma *dw = chan->chip->dw;
	struct axi_dma_desc *desc;
	dma_addr_t phys;

	desc = dma_pool_zalloc(dw->desc_pool, GFP_NOWAIT, &phys);
	if (unlikely(!desc)) {
		dev_err(chan2dev(chan), "%s: not enough descriptors available\n",
			axi_chan_name(chan));
		return NULL;
	}

	atomic_inc(&chan->descs_allocated);
	INIT_LIST_HEAD(&desc->xfer_list);
	desc->vd.tx.phys = phys;
	desc->chan = chan;

	return desc;
}

static void axi_desc_put(struct axi_dma_desc *desc)
{
	struct axi_dma_chan *chan = desc->chan;
	struct dw_axi_dma *dw = chan->chip->dw;
	struct axi_dma_desc *child, *_next;
	unsigned int descs_put = 0;

	list_for_each_entry_safe(child, _next, &desc->xfer_list, xfer_list) {
		list_del(&child->xfer_list);
		dma_pool_free(dw->desc_pool, child, child->vd.tx.phys);
		descs_put++;
	}

	dma_pool_free(dw->desc_pool, desc, desc->vd.tx.phys);
	descs_put++;

	atomic_sub(descs_put, &chan->descs_allocated);
	dev_vdbg(chan2dev(chan), "%s: %d descs put, %d still allocated\n",
		axi_chan_name(chan), descs_put,
		atomic_read(&chan->descs_allocated));
}

static void vchan_desc_put(struct virt_dma_desc *vdesc)
{
	axi_desc_put(vd_to_axi_desc(vdesc));
}

/* Returns how many bytes were already received from source */
static inline u32 dma_chan_get_sent(struct axi_dma_chan *chan)
{
	u32 block_ts = axi_chan_ioread32(chan, CH_STATUS);
	u32 ctllo = axi_chan_ioread32(chan, CH_CTL_L);
	//printk("dma_chan_get_sent %d.\n", (block_ts & DMAX_MAX_BLK_MASK) * (1 << (ctllo >> CH_CTL_L_SRC_WIDTH_POS & 7)));
	return (block_ts & DMAX_MAX_BLK_MASK) * (1 << (ctllo >> CH_CTL_L_SRC_WIDTH_POS & 7));
}

static u32 dma_chan_get_residue(struct axi_dma_chan *chan, dma_cookie_t cookie)
{
	struct axi_dma_desc *first, *desc;
	struct virt_dma_desc *vd;
	unsigned long flags;
	u32 residue = 0;
	u32 sentf, sentr;
	u32 llplo, llphi;
	u64 llp;

	spin_lock_irqsave(&chan->vc.lock, flags);

	sentf = dma_chan_get_sent(chan);
	llplo = axi_chan_ioread32(chan, CH_LLP);
	llphi = axi_chan_ioread32(chan, CH_LLP + 4);
	llp = ((u64)llphi << 32) | llplo;
	sentr = dma_chan_get_sent(chan);
	if (sentr < sentf) {
		llplo = axi_chan_ioread32(chan, CH_LLP);
		llphi = axi_chan_ioread32(chan, CH_LLP + 4);
		llp = ((u64)llphi << 32) | llplo;
	}

	vd = vchan_next_desc(&chan->vc);
	if (vd) {
		first = vd_to_axi_desc(vd);
		residue = first->total_len;
		if (llp == DWC_LLP_LOC(first->lli.llp)) {
			/* Currently in progress */
			residue -= sentr;
		} else {
			residue -= first->len;
			list_for_each_entry(desc, &first->xfer_list, xfer_list) {
				if (llp == DWC_LLP_LOC(desc->lli.llp)) {
					/* Currently in progress */
					residue -= sentr;
					break;
				} else {
					residue -= desc->len;
				}
			}
		}
	} else {
		residue = 0;
	}
	//printk("dma_chan_get_residue %d.\n", residue);
	spin_unlock_irqrestore(&chan->vc.lock, flags);
	return residue;
}

static enum dma_status
dma_chan_tx_status(struct dma_chan *dchan, dma_cookie_t cookie,
		  struct dma_tx_state *txstate)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	enum dma_status ret;

	ret = dma_cookie_status(dchan, cookie, txstate);
	if (ret == DMA_COMPLETE)
		return ret;

	dma_set_residue(txstate, dma_chan_get_residue(chan, cookie));

	if (chan->is_paused && ret == DMA_IN_PROGRESS)
		ret = DMA_PAUSED;

	return ret;
}

static void write_desc_llp(struct axi_dma_desc *desc, dma_addr_t adr)
{
	desc->lli.llp = cpu_to_le64(adr);
}

static void write_chan_llp(struct axi_dma_chan *chan, dma_addr_t adr)
{
	axi_chan_iowrite64(chan, CH_LLP, adr);
}

/* Called in chan locked context */
static void axi_chan_block_xfer_start(struct axi_dma_chan *chan,
				      struct axi_dma_desc *first)
{
	struct dma_slave_config	*sconfig = &chan->dma_sconfig;
	u32 priority = chan->chip->dw->hdata->priority[chan->id];
	u32 reg, irq_mask;
	u8 lms = 0; /* Select AXI0 master for LLI fetching */

	if (unlikely(axi_chan_is_hw_enable(chan))) {
		dev_err(chan2dev(chan), "%s is non-idle!\n",
			axi_chan_name(chan));

		return;
	}

	axi_dma_enable(chan->chip);

	reg = (chan->dws.dst_id << CH_CFG_L_DST_PER_POS |
		   chan->dws.src_id << CH_CFG_L_SRC_PER_POS |
		   DWAXIDMAC_MBLK_TYPE_LL << CH_CFG_L_DST_MULTBLK_TYPE_POS |
	       DWAXIDMAC_MBLK_TYPE_LL << CH_CFG_L_SRC_MULTBLK_TYPE_POS);
	axi_chan_iowrite32(chan, CH_CFG_L, reg);

	reg = (DWAXIDMAC_TT_FC_MEM_TO_MEM_DMAC << CH_CFG_H_TT_FC_POS |
	       priority << CH_CFG_H_PRIORITY_POS |
	       DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_DST_POS |
	       DWAXIDMAC_HS_SEL_HW << CH_CFG_H_HS_SEL_SRC_POS);
	if (sconfig->direction == DMA_MEM_TO_DEV)
		reg |= sconfig->device_fc ? DWC_CFGH_FC(DWAXIDMAC_TT_FC_MEM_TO_PER_DST) :
			DWC_CFGH_FC(DWAXIDMAC_TT_FC_MEM_TO_PER_DMAC);
	else if (sconfig->direction == DMA_DEV_TO_MEM)
		reg |= sconfig->device_fc ? DWC_CFGH_FC(DWAXIDMAC_TT_FC_PER_TO_MEM_SRC) :
			DWC_CFGH_FC(DWAXIDMAC_TT_FC_PER_TO_MEM_DMAC);
	axi_chan_iowrite32(chan, CH_CFG_H, reg);
	write_chan_llp(chan, first->vd.tx.phys | lms);

	irq_mask = DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_ALL_ERR;
	if (chan->cyclic)
		irq_mask |= DWAXIDMAC_IRQ_BLOCK_TRF;
	axi_chan_irq_sig_set(chan, irq_mask);

	/* Generate 'suspend' status but don't generate interrupt */
	irq_mask |= DWAXIDMAC_IRQ_SUSPENDED;
	axi_chan_irq_set(chan, irq_mask);

	axi_chan_enable(chan);
}

static void axi_chan_start_first_queued(struct axi_dma_chan *chan)
{
	struct axi_dma_desc *desc;
	struct virt_dma_desc *vd;

	vd = vchan_next_desc(&chan->vc);
	if (!vd)
		return;

	desc = vd_to_axi_desc(vd);
	dev_vdbg(chan2dev(chan), "%s: started %u\n", axi_chan_name(chan),
		vd->tx.cookie);
	axi_chan_block_xfer_start(chan, desc);
}

static void dma_chan_issue_pending(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);
	if (vchan_issue_pending(&chan->vc))
		axi_chan_start_first_queued(chan);
	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

static int dma_chan_alloc_chan_resources(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);

	/* ASSERT: channel is idle */
	if (axi_chan_is_hw_enable(chan)) {
		dev_err(chan2dev(chan), "%s is non-idle!\n",
			axi_chan_name(chan));
		return -EBUSY;
	}

	dev_vdbg(dchan2dev(dchan), "%s: allocating\n", axi_chan_name(chan));

	pm_runtime_get(chan->chip->dev);

	return 0;
}

static void dma_chan_free_chan_resources(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);

	/* ASSERT: channel is idle */
	if (axi_chan_is_hw_enable(chan))
		dev_err(dchan2dev(dchan), "%s is non-idle!\n",
			axi_chan_name(chan));

	axi_chan_disable(chan);
	axi_chan_irq_disable(chan, DWAXIDMAC_IRQ_ALL);

	vchan_free_chan_resources(&chan->vc);

	dev_vdbg(dchan2dev(dchan),
		 "%s: free resources, descriptor still allocated: %u\n",
		 axi_chan_name(chan), atomic_read(&chan->descs_allocated));

	pm_runtime_put(chan->chip->dev);
}

/*
 * If DW_axi_dmac sees CHx_CTL.ShadowReg_Or_LLI_Last bit of the fetched LLI
 * as 1, it understands that the current block is the final block in the
 * transfer and completes the DMA transfer operation at the end of current
 * block transfer.
 */
static void set_desc_last(struct axi_dma_desc *desc)
{
	u32 val;

	write_desc_llp(desc, 0);
	val = le32_to_cpu(desc->lli.ctl_hi);
	val |= CH_CTL_H_LLI_LAST;
	desc->lli.ctl_hi = cpu_to_le32(val);
}

static void write_desc_sar(struct axi_dma_desc *desc, dma_addr_t adr)
{
	desc->lli.sar = cpu_to_le64(adr);
}

static void write_desc_dar(struct axi_dma_desc *desc, dma_addr_t adr)
{
	desc->lli.dar = cpu_to_le64(adr);
}

static void set_desc_src_master(struct axi_dma_desc *desc)
{
	u32 val;

	/* Select AXI0 for source master */
	val = le32_to_cpu(desc->lli.ctl_lo);
	val &= ~CH_CTL_L_SRC_MAST;
	desc->lli.ctl_lo = cpu_to_le32(val);
}

static void set_desc_dest_master(struct axi_dma_desc *desc)
{
	u32 val;

	/* Select AXI1 for source master if available */
	val = le32_to_cpu(desc->lli.ctl_lo);
	if (desc->chan->chip->dw->hdata->nr_masters > 1)
		val |= CH_CTL_L_DST_MAST;
	else
		val &= ~CH_CTL_L_DST_MAST;

	desc->lli.ctl_lo = cpu_to_le32(val);
}

static struct dma_async_tx_descriptor *
dma_chan_prep_dma_memcpy(struct dma_chan *dchan, dma_addr_t dst_adr,
			 dma_addr_t src_adr, size_t len, unsigned long flags)
{
	struct axi_dma_desc *first = NULL, *desc = NULL, *prev = NULL;
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	size_t block_ts, max_block_ts, xfer_len;
	u32 xfer_width, reg;
	u8 lms = 0; /* Select AXI0 master for LLI fetching */

	dev_dbg(chan2dev(chan), "%s: memcpy: src: %pad dst: %pad length: %zd flags: %#lx",
		axi_chan_name(chan), &src_adr, &dst_adr, len, flags);

	max_block_ts = chan->chip->dw->hdata->block_size[chan->id];

	while (len) {
		xfer_len = len;

		/*
		 * Take care for the alignment.
		 * Actually source and destination widths can be different, but
		 * make them same to be simpler.
		 */
		xfer_width = axi_chan_get_xfer_width(chan, src_adr, dst_adr, xfer_len);

		/*
		 * block_ts indicates the total number of data of width
		 * to be transferred in a DMA block transfer.
		 * BLOCK_TS register should be set to block_ts - 1
		 */
		block_ts = xfer_len >> xfer_width;
		if (block_ts > max_block_ts) {
			block_ts = max_block_ts;
			xfer_len = max_block_ts << xfer_width;
		}

		desc = axi_desc_get(chan);
		if (unlikely(!desc))
			goto err_desc_get;

		write_desc_sar(desc, src_adr);
		write_desc_dar(desc, dst_adr);
		desc->lli.block_ts_lo = cpu_to_le32(block_ts - 1);

		reg = CH_CTL_H_LLI_VALID;
		if (chan->chip->dw->hdata->restrict_axi_burst_len) {
			u32 burst_len = chan->chip->dw->hdata->axi_rw_burst_len;

			reg |= (CH_CTL_H_ARLEN_EN |
				burst_len << CH_CTL_H_ARLEN_POS |
				CH_CTL_H_AWLEN_EN |
				burst_len << CH_CTL_H_AWLEN_POS);
		}
		desc->lli.ctl_hi = cpu_to_le32(reg);

		reg = (DWAXIDMAC_BURST_TRANS_LEN_4 << CH_CTL_L_DST_MSIZE_POS |
		       DWAXIDMAC_BURST_TRANS_LEN_4 << CH_CTL_L_SRC_MSIZE_POS |
		       xfer_width << CH_CTL_L_DST_WIDTH_POS |
		       xfer_width << CH_CTL_L_SRC_WIDTH_POS |
		       DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_DST_INC_POS |
		       DWAXIDMAC_CH_CTL_L_INC << CH_CTL_L_SRC_INC_POS);
		desc->lli.ctl_lo = cpu_to_le32(reg);

		set_desc_src_master(desc);
		set_desc_dest_master(desc);

		desc->len = xfer_len;

		/* Manage transfer list (xfer_list) */
		if (!first) {
			first = desc;
		} else {
			list_add_tail(&desc->xfer_list, &first->xfer_list);
			write_desc_llp(prev, desc->vd.tx.phys | lms);
		}
		prev = desc;

		/* update the length and addresses for the next loop cycle */
		len -= xfer_len;
		dst_adr += xfer_len;
		src_adr += xfer_len;
	}

	/* Total len of src/dest sg == 0, so no descriptor were allocated */
	if (unlikely(!first))
		return NULL;

	/* Set end-of-link to the last link descriptor of list */
	set_desc_last(desc);
	first->total_len = len;

	return vchan_tx_prep(&chan->vc, &first->vd, flags);

err_desc_get:
	if (first)
		axi_desc_put(first);
	return NULL;
}

static struct dma_async_tx_descriptor *
dma_chan_prep_slave_sg(struct dma_chan *dchan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct axi_dma_chan	*chan = dchan_to_axi_dma_chan(dchan);
	struct dma_slave_config	*sconfig = &chan->dma_sconfig;
	struct axi_dma_desc		*prev;
	struct axi_dma_desc		*first;
	struct axi_dma_desc		*desc;
	u32			ctllo, ctlhi;
	u8			m_master = chan->dws.m_master;
	u8			lms = DWC_LLP_LMS(m_master);
	dma_addr_t		reg;
	unsigned int		reg_width;
	unsigned int		i;
	struct scatterlist	*sg;
	size_t			total_len = 0;
	size_t			max_block_ts;

	dev_vdbg(chan2dev(chan), "%s\n", __func__);

	if (unlikely(!is_slave_direction(direction) || !sg_len))
		return NULL;

	max_block_ts = chan->chip->dw->hdata->block_size[chan->id];

	chan->direction = direction;

	prev = first = NULL;

	ctlhi = CH_CTL_H_LLI_VALID;
	if (chan->chip->dw->hdata->restrict_axi_burst_len) {
		u32 burst_len = chan->chip->dw->hdata->axi_rw_burst_len;

		ctlhi |= (CH_CTL_H_ARLEN_EN |
			burst_len << CH_CTL_H_ARLEN_POS |
			CH_CTL_H_AWLEN_EN |
			burst_len << CH_CTL_H_AWLEN_POS);
	}

	switch (direction) {
	case DMA_MEM_TO_DEV:
		reg_width = __ffs(sconfig->dst_addr_width);
		reg = sconfig->dst_addr;
		ctllo = (DWC_DEFAULT_CTLLO(dchan)
				| DWC_CTLL_DST_WIDTH(reg_width)
				| DWC_CTLL_DST_FIX
				| DWC_CTLL_SRC_INC);

		for_each_sg(sgl, sg, sg_len, i) {
			u32		len, dlen, mem;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);

slave_sg_todev_fill_desc:
			desc = axi_desc_get(chan);
			if (!desc)
				goto err_desc_get;

			write_desc_sar(desc, mem);
			write_desc_dar(desc, reg);

			desc->lli.ctl_hi = cpu_to_le32(ctlhi);
			desc->lli.ctl_lo = cpu_to_le32(ctllo | DWC_CTLL_SRC_WIDTH(reg_width));
			if ((len >> reg_width) > max_block_ts) {
				dlen = max_block_ts << reg_width;
				mem += dlen;
				len -= dlen;
			} else {
				dlen = len;
				len = 0;
			}
			desc->lli.block_ts_lo = cpu_to_le32((dlen >> reg_width) - 1);

			desc->len = dlen;

			if (!first) {
				first = desc;
			} else {
				list_add_tail(&desc->xfer_list, &first->xfer_list);
				write_desc_llp(prev, desc->vd.tx.phys | lms);
			}
			prev = desc;
			total_len += dlen;

			if (len)
				goto slave_sg_todev_fill_desc;
		}
		break;
	case DMA_DEV_TO_MEM:
		reg_width = __ffs(sconfig->src_addr_width);
		reg = sconfig->src_addr;
		ctllo = (DWC_DEFAULT_CTLLO(dchan)
				| DWC_CTLL_SRC_WIDTH(reg_width)
				| DWC_CTLL_DST_INC
				| DWC_CTLL_SRC_FIX);

		for_each_sg(sgl, sg, sg_len, i) {
			u32		len, dlen, mem;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);

slave_sg_fromdev_fill_desc:
			desc = axi_desc_get(chan);
			if (!desc)
				goto err_desc_get;

			write_desc_sar(desc, reg);
			write_desc_dar(desc, mem);

			desc->lli.ctl_hi = cpu_to_le32(ctlhi);
			desc->lli.ctl_lo = cpu_to_le32(ctllo | DWC_CTLL_DST_WIDTH(reg_width));
			if ((len >> reg_width) > max_block_ts) {
				dlen = max_block_ts << reg_width;
				mem += dlen;
				len -= dlen;
			} else {
				dlen = len;
				len = 0;
			}
			desc->lli.block_ts_lo = cpu_to_le32((dlen >> reg_width) - 1);
			desc->len = dlen;

			if (!first) {
				first = desc;
			} else {
				list_add_tail(&desc->xfer_list, &first->xfer_list);
				write_desc_llp(prev, desc->vd.tx.phys | lms);
			}
			prev = desc;
			total_len += dlen;

			if (len)
				goto slave_sg_fromdev_fill_desc;
		}
		break;
	default:
		return NULL;
	}

	/* Total len of src/dest sg == 0, so no descriptor were allocated */
	if (unlikely(!first))
		return NULL;

	/* Set end-of-link to the last link descriptor of list */
	set_desc_last(desc);
	first->total_len = total_len;

	return vchan_tx_prep(&chan->vc, &first->vd, flags);

err_desc_get:
	dev_err(chan2dev(chan),
		"not enough descriptors available. Direction %d\n", direction);
	axi_desc_put(first);

	return NULL;
}

static struct dma_async_tx_descriptor *dma_chan_prep_dma_cyclic(
		struct dma_chan *dchan, dma_addr_t buf_addr, size_t len,
		size_t period_len, enum dma_transfer_direction direction,
		unsigned long flags)
{
	struct axi_dma_chan	*chan = dchan_to_axi_dma_chan(dchan);
	struct dma_slave_config	*sconfig = &chan->dma_sconfig;
	struct axi_dma_desc		*prev;
	struct axi_dma_desc		*first;
	struct axi_dma_desc		*desc;
	u8			m_master = chan->dws.m_master;
	u8			lms = DWC_LLP_LMS(m_master);
	unsigned int			reg_width;
	size_t					max_block_ts;
	u32						ctlhi;
	unsigned int			periods;
	unsigned int			i;

	chan->direction = direction;
	max_block_ts = chan->chip->dw->hdata->block_size[chan->id];

	if (direction == DMA_MEM_TO_DEV)
		reg_width = __ffs(sconfig->dst_addr_width);
	else
		reg_width = __ffs(sconfig->src_addr_width);

	periods = len / period_len;

	/* Check for too big/unaligned periods and unaligned DMA buffer. */
	if (period_len > (max_block_ts << reg_width))
		goto out_err;
	if (unlikely(period_len & ((1 << reg_width) - 1)))
		goto out_err;
	if (unlikely(buf_addr & ((1 << reg_width) - 1)))
		goto out_err;

	prev = first = NULL;

	ctlhi = CH_CTL_H_LLI_VALID | CH_CTL_H_IOC_BLKTFR_EN;
	if (chan->chip->dw->hdata->restrict_axi_burst_len) {
		u32 burst_len = chan->chip->dw->hdata->axi_rw_burst_len;

		ctlhi |= (CH_CTL_H_ARLEN_EN |
			burst_len << CH_CTL_H_ARLEN_POS |
			CH_CTL_H_AWLEN_EN |
			burst_len << CH_CTL_H_AWLEN_POS);
	}

	for (i = 0; i < periods; i++) {
		desc = axi_desc_get(chan);
		if (!desc)
			goto out_err_desc_get;

		switch (direction) {
		case DMA_MEM_TO_DEV:
			write_desc_sar(desc, buf_addr + period_len * i);
			write_desc_dar(desc, sconfig->dst_addr);

			desc->lli.ctl_lo = (DWC_DEFAULT_CTLLO(dchan)
				| DWC_CTLL_DST_WIDTH(reg_width)
				| DWC_CTLL_SRC_WIDTH(reg_width)
				| DWC_CTLL_DST_FIX
				| DWC_CTLL_SRC_INC);
			desc->lli.ctl_hi = cpu_to_le32(ctlhi);
			desc->lli.block_ts_lo = cpu_to_le32((period_len >> reg_width) - 1);			
			break;
		case DMA_DEV_TO_MEM:
			write_desc_sar(desc, sconfig->src_addr);
			write_desc_dar(desc, buf_addr + period_len * i);

			desc->lli.ctl_lo = (DWC_DEFAULT_CTLLO(dchan)
				| DWC_CTLL_SRC_WIDTH(reg_width)
				| DWC_CTLL_DST_WIDTH(reg_width)
				| DWC_CTLL_DST_INC
				| DWC_CTLL_SRC_FIX);
			desc->lli.ctl_hi = cpu_to_le32(ctlhi);
			desc->lli.block_ts_lo = cpu_to_le32((period_len >> reg_width) - 1);	
			break;
		default:
			break;
		}

		desc->len = period_len;

		if (!first) {
			first = desc;
		} else {
			list_add_tail(&desc->xfer_list, &first->xfer_list);
			write_desc_llp(prev, desc->vd.tx.phys | lms);
		}
		prev = desc;
	}

	first->total_len = len;

	/* Let's make a cyclic list */
	write_desc_llp(prev, first->vd.tx.phys | lms);

	dev_dbg(chan2dev(chan),
			"cyclic prepared buf %pad len %zu period %zu periods %d\n",
			&buf_addr, len, period_len, periods);
	
	chan->cyclic = true;

	return vchan_tx_prep(&chan->vc, &first->vd, flags);

out_err_desc_get:
	axi_desc_put(first);
out_err:
	return NULL;
}

bool dw_axi_dma_filter(struct dma_chan *dchan, void *param)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	struct dw_axi_dma_slave *dws = param;

	if (dws->dma_dev != dchan->device->dev)
		return false;

	/* We have to copy data since dws can be temporary storage */
	memcpy(&chan->dws, dws, sizeof(struct dw_axi_dma_slave));

	return true;
}
EXPORT_SYMBOL_GPL(dw_axi_dma_filter);

static void axi_chan_dump_lli(struct axi_dma_chan *chan,
			      struct axi_dma_desc *desc)
{
	dev_err(dchan2dev(&chan->vc.chan),
		"SAR: 0x%llx DAR: 0x%llx LLP: 0x%llx BTS 0x%x CTL: 0x%x:%08x",
		le64_to_cpu(desc->lli.sar),
		le64_to_cpu(desc->lli.dar),
		le64_to_cpu(desc->lli.llp),
		le32_to_cpu(desc->lli.block_ts_lo),
		le32_to_cpu(desc->lli.ctl_hi),
		le32_to_cpu(desc->lli.ctl_lo));
}

static void axi_chan_list_dump_lli(struct axi_dma_chan *chan,
				   struct axi_dma_desc *desc_head)
{
	struct axi_dma_desc *desc;

	axi_chan_dump_lli(chan, desc_head);
	list_for_each_entry(desc, &desc_head->xfer_list, xfer_list)
		axi_chan_dump_lli(chan, desc);
}

static noinline void axi_chan_handle_err(struct axi_dma_chan *chan, u32 status)
{
	struct virt_dma_desc *vd;
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);

	axi_chan_disable(chan);

	/* The bad descriptor currently is in the head of vc list */
	vd = vchan_next_desc(&chan->vc);
	/* Remove the completed descriptor from issued list */
	list_del(&vd->node);

	/* WARN about bad descriptor */
	dev_err(chan2dev(chan),
		"Bad descriptor submitted for %s, cookie: %d, irq: 0x%08x\n",
		axi_chan_name(chan), vd->tx.cookie, status);
	axi_chan_list_dump_lli(chan, vd_to_axi_desc(vd));

	vchan_cookie_complete(vd);

	/* Try to restart the controller */
	axi_chan_start_first_queued(chan);

	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

static void axi_chan_block_xfer_complete(struct axi_dma_chan *chan)
{
	struct virt_dma_desc *vd;
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);

	/* The completed descriptor currently is in the head of vc list */
	vd = vchan_next_desc(&chan->vc);
	if (chan->cyclic) {
		struct axi_dma_desc *first, *desc, *cur_desc;
		u32 llplo, llphi;
		u64 llp;

		first = vd_to_axi_desc(vd);
		llplo = axi_chan_ioread32(chan, CH_LLP);
		llphi = axi_chan_ioread32(chan, CH_LLP + 4);
		llp = ((u64)llphi << 32) | llplo;
		//printk("llp=0x%llx, first->lli.llp=0x%llx.\n", llp, first->lli.llp);
		if (llp == DWC_LLP_LOC(first->lli.llp)) {
			cur_desc = list_prev_entry(first, xfer_list);
			cur_desc->lli.ctl_hi |= CH_CTL_H_LLI_VALID;
		} else {
			list_for_each_entry(desc, &first->xfer_list, xfer_list) {
				//printk("llp=0x%llx, desc->lli.llp=0x%llx.\n", llp, desc->lli.llp);
				if (llp == DWC_LLP_LOC(desc->lli.llp)) {
					cur_desc = list_prev_entry(desc, xfer_list);
					cur_desc->lli.ctl_hi |= CH_CTL_H_LLI_VALID;
					break;
				}
			}
		}
		vchan_cyclic_callback(vd);
	} else {
		if (unlikely(axi_chan_is_hw_enable(chan))) {
			dev_err(chan2dev(chan), "BUG: %s caught DWAXIDMAC_IRQ_DMA_TRF, but channel not idle!\n",
				axi_chan_name(chan));
			axi_chan_disable(chan);
		}

		/* Remove the completed descriptor from issued list before completing */
		list_del(&vd->node);
		vchan_cookie_complete(vd);
		/* Submit queued descriptors after processing the completed ones */
		axi_chan_start_first_queued(chan);
	}

	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

static irqreturn_t dw_axi_dma_interrupt(int irq, void *dev_id)
{
	struct axi_dma_chip *chip = dev_id;
	struct dw_axi_dma *dw = chip->dw;
	struct axi_dma_chan *chan;

	u32 status, i;

	/* Disable DMAC inerrupts. We'll enable them after processing chanels */
	axi_dma_irq_disable(chip);

	/* Poll, clear and process every chanel interrupt status */
	for (i = 0; i < dw->hdata->nr_channels; i++) {
		chan = &dw->chan[i];
		status = axi_chan_irq_read(chan);
		axi_chan_irq_clear(chan, status);
		//if (status) printk("ch %d status 0x%x.\n", i, status);
		dev_vdbg(chip->dev, "%s %u IRQ status: 0x%08x\n",
			axi_chan_name(chan), i, status);
		if (status & DWAXIDMAC_IRQ_ALL_ERR)
			axi_chan_handle_err(chan, status);
		else if (status & (DWAXIDMAC_IRQ_DMA_TRF | DWAXIDMAC_IRQ_BLOCK_TRF))
			axi_chan_block_xfer_complete(chan);
	}

	/* Re-enable interrupts */
	axi_dma_irq_enable(chip);

	return IRQ_HANDLED;
}

/*
 * Fix sconfig's burst size according to dw_dmac. We need to convert them as:
 * 1 -> 0, 4 -> 1, 8 -> 2, 16 -> 3.
 *
 * NOTE: burst size 2 is not supported by controller.
 *
 * This can be done by finding least significant bit set: n & (n - 1)
 */
static inline void convert_burst(u32 *maxburst)
{
	if (*maxburst > 1)
		*maxburst = fls(*maxburst) - 2;
	else
		*maxburst = 0;
}

static int dma_chan_config(struct dma_chan *dchan, struct dma_slave_config *sconfig)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);

	/* Check if chan will be configured for slave transfers */
	if (!is_slave_direction(sconfig->direction))
		return -EINVAL;

	memcpy(&chan->dma_sconfig, sconfig, sizeof(*sconfig));
	chan->direction = sconfig->direction;

	if (chan->dma_sconfig.src_maxburst == 0)
		chan->dma_sconfig.src_maxburst = chan->dma_sconfig.dst_maxburst;

	if (chan->dma_sconfig.dst_maxburst == 0)
		chan->dma_sconfig.dst_maxburst = chan->dma_sconfig.src_maxburst;

	convert_burst(&chan->dma_sconfig.src_maxburst);
	convert_burst(&chan->dma_sconfig.dst_maxburst);

	return 0;
}

static int dma_chan_terminate_all(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	u32 chan_active = BIT(chan->id) << DMAC_CHAN_EN_SHIFT;
	unsigned long flags;
	u32 val;
	int ret;
	LIST_HEAD(head);

	axi_chan_disable(chan);

	ret = readl_poll_timeout_atomic(chan->chip->regs + DMAC_CHEN, val,
					!(val & chan_active), 1000, 10000);
	if (ret == -ETIMEDOUT)
		dev_warn(dchan2dev(dchan),
			 "%s failed to stop\n", axi_chan_name(chan));

	spin_lock_irqsave(&chan->vc.lock, flags);

	vchan_get_all_descriptors(&chan->vc, &head);

	chan->cyclic = false;
	spin_unlock_irqrestore(&chan->vc.lock, flags);

	vchan_dma_desc_free_list(&chan->vc, &head);

	dev_vdbg(dchan2dev(dchan), "terminated: %s\n", axi_chan_name(chan));

	return 0;
}

static int dma_chan_pause(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;
	unsigned int timeout = 20; /* timeout iterations */
	u32 val;

	spin_lock_irqsave(&chan->vc.lock, flags);

	val = axi_dma_ioread32(chan->chip, DMAC_CHEN);
	val |= BIT(chan->id) << DMAC_CHAN_SUSP_SHIFT |
	       BIT(chan->id) << DMAC_CHAN_SUSP_WE_SHIFT;
	axi_dma_iowrite32(chan->chip, DMAC_CHEN, val);

	do  {
		if (axi_chan_irq_read(chan) & DWAXIDMAC_IRQ_SUSPENDED)
			break;

		udelay(2);
	} while (--timeout);

	axi_chan_irq_clear(chan, DWAXIDMAC_IRQ_SUSPENDED);

	chan->is_paused = true;

	spin_unlock_irqrestore(&chan->vc.lock, flags);

	return timeout ? 0 : -EAGAIN;
}

/* Called in chan locked context */
static inline void axi_chan_resume(struct axi_dma_chan *chan)
{
	u32 val;

	val = axi_dma_ioread32(chan->chip, DMAC_CHEN);
	val &= ~(BIT(chan->id) << DMAC_CHAN_SUSP_SHIFT);
	val |=  (BIT(chan->id) << DMAC_CHAN_SUSP_WE_SHIFT);
	axi_dma_iowrite32(chan->chip, DMAC_CHEN, val);

	chan->is_paused = false;
}

static int dma_chan_resume(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);

	if (chan->is_paused)
		axi_chan_resume(chan);

	spin_unlock_irqrestore(&chan->vc.lock, flags);

	return 0;
}

static int axi_dma_suspend(struct axi_dma_chip *chip)
{
	axi_dma_irq_disable(chip);
	axi_dma_disable(chip);

	clk_disable_unprepare(chip->core_clk);
	clk_disable_unprepare(chip->cfgr_clk);

	return 0;
}

static int axi_dma_resume(struct axi_dma_chip *chip)
{
	int ret;

	ret = clk_prepare_enable(chip->cfgr_clk);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(chip->core_clk);
	if (ret < 0)
		return ret;

	axi_dma_enable(chip);
	axi_dma_irq_enable(chip);

	return 0;
}

static int __maybe_unused axi_dma_runtime_suspend(struct device *dev)
{
	struct axi_dma_chip *chip = dev_get_drvdata(dev);

	return axi_dma_suspend(chip);
}

static int __maybe_unused axi_dma_runtime_resume(struct device *dev)
{
	struct axi_dma_chip *chip = dev_get_drvdata(dev);

	return axi_dma_resume(chip);
}

static int parse_device_properties(struct axi_dma_chip *chip)
{
	struct device *dev = chip->dev;
	u32 tmp, carr[DMAC_MAX_CHANNELS];
	int ret;

	ret = device_property_read_u32(dev, "dma-channels", &tmp);
	if (ret)
		return ret;
	if (tmp == 0 || tmp > DMAC_MAX_CHANNELS)
		return -EINVAL;

	chip->dw->hdata->nr_channels = tmp;

	ret = device_property_read_u32(dev, "snps,dma-masters", &tmp);
	if (ret)
		return ret;
	if (tmp == 0 || tmp > DMAC_MAX_MASTERS)
		return -EINVAL;

	chip->dw->hdata->nr_masters = tmp;

	ret = device_property_read_u32(dev, "snps,data-width", &tmp);
	if (ret)
		return ret;
	if (tmp > DWAXIDMAC_TRANS_WIDTH_MAX)
		return -EINVAL;

	chip->dw->hdata->m_data_width = tmp;

	ret = device_property_read_u32_array(dev, "snps,block-size", carr,
					     chip->dw->hdata->nr_channels);
	if (ret)
		return ret;
	for (tmp = 0; tmp < chip->dw->hdata->nr_channels; tmp++) {
		if (carr[tmp] == 0 || carr[tmp] > DMAC_MAX_BLK_SIZE)
			return -EINVAL;

		chip->dw->hdata->block_size[tmp] = carr[tmp];
	}

	ret = device_property_read_u32_array(dev, "snps,priority", carr,
					     chip->dw->hdata->nr_channels);
	if (ret)
		return ret;
	/* Priority value must be programmed within [0:nr_channels-1] range */
	for (tmp = 0; tmp < chip->dw->hdata->nr_channels; tmp++) {
		if (carr[tmp] >= chip->dw->hdata->nr_channels)
			return -EINVAL;

		chip->dw->hdata->priority[tmp] = carr[tmp];
	}

	/* axi-max-burst-len is optional property */
	ret = device_property_read_u32(dev, "snps,axi-max-burst-len", &tmp);
	if (!ret) {
		if (tmp > DWAXIDMAC_ARWLEN_MAX + 1)
			return -EINVAL;
		if (tmp < DWAXIDMAC_ARWLEN_MIN + 1)
			return -EINVAL;

		chip->dw->hdata->restrict_axi_burst_len = true;
		chip->dw->hdata->axi_rw_burst_len = tmp - 1;
	}

	return 0;
}

static struct dma_chan *dw_axi_dma_of_xlate(struct of_phandle_args *dma_spec,
					struct of_dma *ofdma)
{
	struct dw_axi_dma *dw = ofdma->of_dma_data;
	struct dw_axi_dma_slave slave = {
		.dma_dev = dw->dma.dev,
	};
	dma_cap_mask_t cap;

	if (dma_spec->args_count != 3)
		return NULL;

	slave.src_id = dma_spec->args[0];
	slave.dst_id = dma_spec->args[0];
	slave.m_master = dma_spec->args[1];
	slave.p_master = dma_spec->args[2];

	if (WARN_ON(slave.src_id >= DMAC_MAX_NR_REQUESTS ||
		    slave.dst_id >= DMAC_MAX_NR_REQUESTS ||
		    slave.m_master >= dw->hdata->nr_masters ||
		    slave.p_master >= dw->hdata->nr_masters))
		return NULL;

	dma_cap_zero(cap);
	dma_cap_set(DMA_SLAVE, cap);

	/* TODO: there should be a simpler way to do this */
	return dma_request_channel(cap, dw_axi_dma_filter, &slave);
}

static int dw_probe(struct platform_device *pdev)
{
	struct axi_dma_chip *chip;
	struct resource *mem;
	struct dw_axi_dma *dw;
	struct dw_axi_dma_hcfg *hdata;
	u32 i;
	int ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	dw = devm_kzalloc(&pdev->dev, sizeof(*dw), GFP_KERNEL);
	if (!dw)
		return -ENOMEM;

	hdata = devm_kzalloc(&pdev->dev, sizeof(*hdata), GFP_KERNEL);
	if (!hdata)
		return -ENOMEM;

	chip->dw = dw;
	chip->dev = &pdev->dev;
	chip->dw->hdata = hdata;

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0)
		return chip->irq;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->regs = devm_ioremap_resource(chip->dev, mem);
	if (IS_ERR(chip->regs))
		return PTR_ERR(chip->regs);

	chip->core_clk = devm_clk_get(chip->dev, "core-clk");
	if (IS_ERR(chip->core_clk))
		return PTR_ERR(chip->core_clk);

	chip->cfgr_clk = devm_clk_get(chip->dev, "cfgr-clk");
	if (IS_ERR(chip->cfgr_clk))
		return PTR_ERR(chip->cfgr_clk);

	ret = parse_device_properties(chip);
	if (ret)
		return ret;

	dw->chan = devm_kcalloc(chip->dev, hdata->nr_channels,
				sizeof(*dw->chan), GFP_KERNEL);
	if (!dw->chan)
		return -ENOMEM;

	ret = devm_request_irq(chip->dev, chip->irq, dw_axi_dma_interrupt,
			       IRQF_SHARED, KBUILD_MODNAME, chip);
	if (ret)
		return ret;

	/* Lli address must be aligned to a 64-byte boundary */
	dw->desc_pool = dmam_pool_create(KBUILD_MODNAME, chip->dev,
					 sizeof(struct axi_dma_desc), 64, 0);
	if (!dw->desc_pool) {
		dev_err(chip->dev, "No memory for descriptors dma pool\n");
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&dw->dma.channels);
	for (i = 0; i < hdata->nr_channels; i++) {
		struct axi_dma_chan *chan = &dw->chan[i];

		chan->chip = chip;
		chan->id = i;
		chan->chan_regs = chip->regs + COMMON_REG_LEN + i * CHAN_REG_LEN;
		atomic_set(&chan->descs_allocated, 0);

		chan->vc.desc_free = vchan_desc_put;
		vchan_init(&chan->vc, &dw->dma);
	}

	/* Set capabilities */
	dma_cap_set(DMA_MEMCPY, dw->dma.cap_mask);
	dma_cap_set(DMA_SLAVE, dw->dma.cap_mask);

	/* DMA capabilities */
	dw->dma.chancnt = hdata->nr_channels;
	dw->dma.src_addr_widths = AXI_DMA_BUSWIDTHS;
	dw->dma.dst_addr_widths = AXI_DMA_BUSWIDTHS;
	dw->dma.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV) |
			     BIT(DMA_MEM_TO_MEM);
	dw->dma.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;//DMA_RESIDUE_GRANULARITY_DESCRIPTOR;//DMA_RESIDUE_GRANULARITY_BURST;

	dw->dma.dev = chip->dev;
	dw->dma.device_config = dma_chan_config;
	dw->dma.device_tx_status = dma_chan_tx_status;
	dw->dma.device_issue_pending = dma_chan_issue_pending;
	dw->dma.device_terminate_all = dma_chan_terminate_all;
	dw->dma.device_pause = dma_chan_pause;
	dw->dma.device_resume = dma_chan_resume;

	dw->dma.device_alloc_chan_resources = dma_chan_alloc_chan_resources;
	dw->dma.device_free_chan_resources = dma_chan_free_chan_resources;

	dw->dma.device_prep_dma_memcpy = dma_chan_prep_dma_memcpy;
	dw->dma.device_prep_slave_sg = dma_chan_prep_slave_sg;
	dw->dma.device_prep_dma_cyclic	= dma_chan_prep_dma_cyclic;

	platform_set_drvdata(pdev, chip);

	if (pdev->dev.of_node) {
		ret = of_dma_controller_register(pdev->dev.of_node,
						 dw_axi_dma_of_xlate, dw);
		if (ret)
			dev_err(&pdev->dev,
				"could not register of_dma_controller\n");
	}

	pm_runtime_enable(chip->dev);

	/*
	 * We can't just call pm_runtime_get here instead of
	 * pm_runtime_get_noresume + axi_dma_resume because we need
	 * driver to work also without Runtime PM.
	 */
	pm_runtime_get_noresume(chip->dev);
	ret = axi_dma_resume(chip);
	if (ret < 0)
		goto err_pm_disable;

	axi_dma_hw_init(chip);

	pm_runtime_put(chip->dev);

	ret = dmaenginem_async_device_register(&dw->dma);
	if (ret)
		goto err_pm_disable;

	dev_info(chip->dev, "DesignWare AXI DMA Controller, %d channels\n",
		 dw->hdata->nr_channels);

	return 0;

err_pm_disable:
	pm_runtime_disable(chip->dev);

	return ret;
}

static int dw_remove(struct platform_device *pdev)
{
	struct axi_dma_chip *chip = platform_get_drvdata(pdev);
	struct dw_axi_dma *dw = chip->dw;
	struct axi_dma_chan *chan, *_chan;
	u32 i;

	/* Enable clk before accessing to registers */
	clk_prepare_enable(chip->cfgr_clk);
	clk_prepare_enable(chip->core_clk);
	axi_dma_irq_disable(chip);
	for (i = 0; i < dw->hdata->nr_channels; i++) {
		axi_chan_disable(&chip->dw->chan[i]);
		axi_chan_irq_disable(&chip->dw->chan[i], DWAXIDMAC_IRQ_ALL);
	}
	axi_dma_disable(chip);

	pm_runtime_disable(chip->dev);
	axi_dma_suspend(chip);

	devm_free_irq(chip->dev, chip->irq, chip);

	list_for_each_entry_safe(chan, _chan, &dw->dma.channels,
			vc.chan.device_node) {
		list_del(&chan->vc.chan.device_node);
		tasklet_kill(&chan->vc.task);
	}

	return 0;
}

static const struct dev_pm_ops dw_axi_dma_pm_ops = {
	SET_RUNTIME_PM_OPS(axi_dma_runtime_suspend, axi_dma_runtime_resume, NULL)
};

static const struct of_device_id dw_dma_of_id_table[] = {
	{ .compatible = "snps,axi-dma-1.01a" },
	{}
};
MODULE_DEVICE_TABLE(of, dw_dma_of_id_table);

static struct platform_driver dw_driver = {
	.probe		= dw_probe,
	.remove		= dw_remove,
	.driver = {
		.name	= KBUILD_MODNAME,
		.of_match_table = of_match_ptr(dw_dma_of_id_table),
		.pm = &dw_axi_dma_pm_ops,
	},
};
module_platform_driver(dw_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Synopsys DesignWare AXI DMA Controller platform driver");
MODULE_AUTHOR("Eugeniy Paltsev <Eugeniy.Paltsev@synopsys.com>");
