/*
 * arkmicro spi driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/scatterlist.h>

#define DRIVER_NAME "spi_ark"

#define USE_DMA_THRESHOLD	32

#define ARK_ECSPI_RXDATA	0x50
#define ARK_ECSPI_TXDATA	0x460

/* generic defines to abstract from the different register layouts */
#define ARK_INT_RR	(1 << 0) /* Receive data ready interrupt */
#define ARK_INT_TE	(1 << 1) /* Transmit FIFO empty interrupt */

/* The maximum  bytes that a sdma BD can transfer.*/
#define MAX_SDMA_BD_BYTES  (1 << 15)
#define ARK_ECSPI_CTRL_MAX_BURST	512

struct spi_ark_data;

struct spi_ark_devtype_data {
	void (*intctrl)(struct spi_ark_data *, int);
	int (*config)(struct spi_device *);
	void (*trigger)(struct spi_ark_data *);
	int (*rx_available)(struct spi_ark_data *);
	void (*reset)(struct spi_ark_data *);
	bool has_dmamode;
	unsigned int fifo_size;
};

struct spi_ark_data {
	struct spi_bitbang bitbang;
	struct device *dev;

	struct completion xfer_done;
	void __iomem *base;
	unsigned long base_phys;

	struct clk *clk_per;
	struct clk *clk_ipg;
	unsigned long spi_clk;
	unsigned int spi_bus_clk;

	unsigned int speed_hz;
	unsigned int bits_per_word;
	unsigned int spi_drctl;

	unsigned int count, remainder;
	void (*tx)(struct spi_ark_data *);
	void (*rx)(struct spi_ark_data *);
	void *rx_buf;
	const void *tx_buf;
	unsigned int txfifo; /* number of words pushed in tx FIFO */
	unsigned int read_u32;
	unsigned int word_mask;
	bool is_arke;

	/* DMA */
	bool usedma;
	u32 wml;
	struct completion dma_rx_completion;
	struct completion dma_tx_completion;
	struct sg_table *dma_rx_sg;
	struct spi_transfer dma_transfer;
	struct spi_transfer pio_transfer;

	const struct spi_ark_devtype_data *devtype_data;
};

static void spi_ark_buf_rx_u8(struct spi_ark_data *spi_ark)
{
	unsigned int val = readl(spi_ark->base + ARK_ECSPI_RXDATA);

	if (spi_ark->rx_buf) {
		if (spi_ark->is_arke)
			*(u8*)spi_ark->rx_buf = val & 0xff;
		else
			*(u8*)spi_ark->rx_buf = (val >> 24) & 0xff;
		spi_ark->rx_buf += 1;
	}
}

static void spi_ark_buf_rx_u16(struct spi_ark_data *spi_ark)
{
	unsigned int val = readl(spi_ark->base + ARK_ECSPI_RXDATA);

	if (spi_ark->rx_buf) {
		if (spi_ark->is_arke)
			*(u16*)spi_ark->rx_buf = val & 0xffff;
		else
			*(u16*)spi_ark->rx_buf = (val >> 16) & 0xffff;
		spi_ark->rx_buf += 2;
	}
}

static void spi_ark_buf_tx_u8(struct spi_ark_data *spi_ark)
{
	u32 val = 0;

	if (spi_ark->tx_buf) {
		if (spi_ark->is_arke)
			val = *(u8 *)spi_ark->tx_buf;
		else
			val = *(u8 *)spi_ark->tx_buf << 24;
		spi_ark->tx_buf += 1;
	}

	spi_ark->count -= 1;
	writel(val, spi_ark->base + ARK_ECSPI_TXDATA);
}

static void spi_ark_buf_tx_u16(struct spi_ark_data *spi_ark)
{
	u32 val = 0;

	if (spi_ark->tx_buf) {
		if (spi_ark->is_arke)
			val = *(u16 *)spi_ark->tx_buf;
		else
			val = *(u16 *)spi_ark->tx_buf << 16;
		spi_ark->tx_buf += 2;
	}

	spi_ark->count -=2;
	writel(val, spi_ark->base + ARK_ECSPI_TXDATA);
}

static int spi_ark_bytes_per_word(const int bits_per_word)
{
	return DIV_ROUND_UP(bits_per_word, BITS_PER_BYTE);
}

static bool spi_ark_can_dma(struct spi_master *master, struct spi_device *spi,
			 struct spi_transfer *transfer)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(master);
	const u32 mszs[] = {1, 4, 8, 16};
	int idx = ARRAY_SIZE(mszs) - 1;
	struct sg_table *tx;
	struct sg_table *rx;
	struct spi_transfer *dma_xfer = &spi_ark->dma_transfer;
	struct spi_transfer *pio_xfer = &spi_ark->pio_transfer;
	int len, remainder;

	if (!master->dma_rx)
		return false;

	pio_xfer->len = 0;
	memcpy(dma_xfer, transfer, sizeof(struct spi_transfer));
	tx = &dma_xfer->tx_sg;
	rx = &dma_xfer->rx_sg;
	remainder = transfer->len & 3;
	len = transfer->len - remainder;

	if (len < USE_DMA_THRESHOLD)
		return false;

	if (remainder) {
		if (tx->nents && tx->sgl[tx->nents - 1].length > remainder &&
			rx->nents && rx->sgl[rx->nents - 1].length > remainder) {
			tx->sgl[tx->nents - 1].length -= remainder;
			rx->sgl[rx->nents - 1].length -= remainder;
			dma_xfer->len = len;

			memcpy(pio_xfer, transfer, sizeof(struct spi_transfer));
			pio_xfer->len = remainder;
			if (pio_xfer->tx_buf)
				pio_xfer->tx_buf += len;
			if (pio_xfer->rx_buf)
				pio_xfer->rx_buf += len;
		} else return false;
	}

	/* dw dma busrt should be 16,8,4,1 */
	for (; idx >= 0; idx--) {
		if (!(len % (mszs[idx] * 4)))
			break;
	}

	spi_ark->wml = mszs[idx];

	return true;
}

#define ARK_ECSPI_CTRL		0x08
#define ARK_ECSPI_CTRL_ENABLE		(1 <<  0)
#define ARK_ECSPI_CTRL_XCH		(1 <<  2)
#define ARK_ECSPI_CTRL_SMC		(1 << 3)
#define ARK_ECSPI_CTRL_MODE_MASK	(0xf << 4)
#define ARK_ECSPI_CTRL_DRCTL(drctl)	((drctl) << 16)
#define ARK_ECSPI_CTRL_POSTDIV_OFFSET	8
#define ARK_ECSPI_CTRL_PREDIV_OFFSET	12
#define ARK_ECSPI_CTRL_CS(cs)		((cs) << 18)
#define ARK_ECSPI_CTRL_BL_OFFSET	20
#define ARK_ECSPI_CTRL_BL_MASK		(0xfff << 20)

#define ARK_ECSPI_CONFIG	0x0c
#define ARK_ECSPI_CONFIG_SCLKPHA(cs)	(1 << ((cs) +  0))
#define ARK_ECSPI_CONFIG_SCLKPOL(cs)	(1 << ((cs) +  4))
#define ARK_ECSPI_CONFIG_SBBCTRL(cs)	(1 << ((cs) +  8))
#define ARK_ECSPI_CONFIG_SSBPOL(cs)	(1 << ((cs) + 12))
#define ARK_ECSPI_CONFIG_SCLKCTL(cs)	(1 << ((cs) + 20))

#define ARK_ECSPI_INT		0x10
#define ARK_ECSPI_INT_TEEN		(1 <<  0)
#define ARK_ECSPI_INT_RREN		(1 <<  3)

#define ARK_ECSPI_DMA      0x14
#define ARK_ECSPI_DMA_TX_WML(wml)	((wml) & 0x3f)
#define ARK_ECSPI_DMA_RX_WML(wml)	(((wml) & 0x3f) << 16)
#define ARK_ECSPI_DMA_RXT_WML(wml)	(((wml) & 0x3f) << 24)

#define ARK_ECSPI_DMA_TEDEN		(1 << 7)
#define ARK_ECSPI_DMA_RXDEN		(1 << 23)
#define ARK_ECSPI_DMA_RXTDEN		(1 << 31)

#define ARK_ECSPI_STAT		0x18
#define ARK_ECSPI_STAT_REN		(1 << 8)
#define ARK_ECSPI_STAT_RR		(1 << 3)

#define ARK_ECSPI_TESTREG	0x20
#define ARK_ECSPI_TESTREG_LBC	BIT(31)

static void spi_ark_buf_rx_swap_u32(struct spi_ark_data *spi_ark)
{
	unsigned int val = readl(spi_ark->base + ARK_ECSPI_RXDATA);

	if (spi_ark->rx_buf) {
		val &= spi_ark->word_mask;
		*(u32 *)spi_ark->rx_buf = val;
		spi_ark->rx_buf += sizeof(u32);
	}
}

static void spi_ark_buf_rx_swap(struct spi_ark_data *spi_ark)
{
	unsigned int bytes_per_word;

	bytes_per_word = spi_ark_bytes_per_word(spi_ark->bits_per_word);
	if (spi_ark->read_u32) {
		spi_ark_buf_rx_swap_u32(spi_ark);
		return;
	}

	if (bytes_per_word == 1)
		spi_ark_buf_rx_u8(spi_ark);
	else if (bytes_per_word == 2)
		spi_ark_buf_rx_u16(spi_ark);
}

static void spi_ark_buf_tx_swap_u32(struct spi_ark_data *spi_ark)
{
	u32 val = 0;

	if (spi_ark->tx_buf) {
		val = *(u32 *)spi_ark->tx_buf;
		val &= spi_ark->word_mask;
		spi_ark->tx_buf += sizeof(u32);
	}
	spi_ark->count -= sizeof(u32);
	writel(val, spi_ark->base + ARK_ECSPI_TXDATA);
}

static void spi_ark_buf_tx_swap(struct spi_ark_data *spi_ark)
{
	u32 ctrl, val;
	unsigned int bytes_per_word;

	if (spi_ark->count == spi_ark->remainder) {
		ctrl = readl(spi_ark->base + ARK_ECSPI_CTRL);
		ctrl &= ~ARK_ECSPI_CTRL_BL_MASK;
		if (spi_ark->count > ARK_ECSPI_CTRL_MAX_BURST) {
			spi_ark->remainder = spi_ark->count %
					     ARK_ECSPI_CTRL_MAX_BURST;
			val = ARK_ECSPI_CTRL_MAX_BURST * 8 - 1;
		} else if (spi_ark->count >= sizeof(u32)) {
			spi_ark->remainder = spi_ark->count % sizeof(u32);
			val = (spi_ark->count - spi_ark->remainder) * 8 - 1;
		} else {
			spi_ark->remainder = 0;
			val = spi_ark->bits_per_word - 1;
			spi_ark->read_u32 = 0;
		}
		ctrl |= (val << ARK_ECSPI_CTRL_BL_OFFSET);
		writel(ctrl, spi_ark->base + ARK_ECSPI_CTRL);
	}

	if (spi_ark->count >= sizeof(u32)) {
		spi_ark_buf_tx_swap_u32(spi_ark);
		return;
	}

	bytes_per_word = spi_ark_bytes_per_word(spi_ark->bits_per_word);
	if (bytes_per_word == 1)
		spi_ark_buf_tx_u8(spi_ark);
	else if (bytes_per_word == 2)
		spi_ark_buf_tx_u16(spi_ark);
}

/* ARK eCSPI */
static unsigned int ark_ecspi_clkdiv(struct spi_ark_data *spi_ark,
				      unsigned int fspi, unsigned int *fres)
{
	/*
	 * there are two 4-bit dividers, the pre-divider divides by
	 * $pre, the post-divider by 2^$post
	 */
	unsigned int pre, post;
	unsigned int fin = spi_ark->spi_clk;

	if (unlikely(fspi > fin))
		return 0;

	post = fls(fin) - fls(fspi);
	if (fin > fspi << post)
		post++;

	/* now we have: (fin <= fspi << post) with post being minimal */

	post = max(4U, post) - 4;
	if (unlikely(post > 0xf)) {
		dev_err(spi_ark->dev, "cannot set clock freq: %u (base freq: %u)\n",
				fspi, fin);
		return 0xff;
	}

	pre = DIV_ROUND_UP(fin, fspi << post) - 1;

	dev_dbg(spi_ark->dev, "%s: fin: %u, fspi: %u, post: %u, pre: %u\n",
			__func__, fin, fspi, post, pre);

	/* Resulting frequency for the SCLK line. */
	*fres = (fin / (pre + 1)) >> post;

	return (pre << ARK_ECSPI_CTRL_PREDIV_OFFSET) |
		(post << ARK_ECSPI_CTRL_POSTDIV_OFFSET);
}

static void ark_ecspi_intctrl(struct spi_ark_data *spi_ark, int enable)
{
	unsigned val = 0;

	if (enable & ARK_INT_TE)
		val |= ARK_ECSPI_INT_TEEN;

	if (enable & ARK_INT_RR)
		val |= ARK_ECSPI_INT_RREN;

	writel(val, spi_ark->base + ARK_ECSPI_INT);
}

static void ark_ecspi_trigger(struct spi_ark_data *spi_ark)
{
	u32 reg;

	reg = readl(spi_ark->base + ARK_ECSPI_CTRL);
	reg |= ARK_ECSPI_CTRL_XCH;
	writel(reg, spi_ark->base + ARK_ECSPI_CTRL);
}

static int ark_ecspi_config(struct spi_device *spi)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(spi->master);
	u32 ctrl = ARK_ECSPI_CTRL_ENABLE;
	u32 clk = spi_ark->speed_hz, delay, reg;
	u32 cfg = readl(spi_ark->base + ARK_ECSPI_CONFIG);

	/*
	 * The hardware seems to have a race condition when changing modes. The
	 * current assumption is that the selection of the channel arrives
	 * earlier in the hardware than the mode bits when they are written at
	 * the same time.
	 * So set master mode for all channels as we do not support slave mode.
	 */
	ctrl |= ARK_ECSPI_CTRL_MODE_MASK;

	/*
	 * Enable SPI_RDY handling (falling edge/level triggered).
	 */
	if (spi->mode & SPI_READY)
		ctrl |= ARK_ECSPI_CTRL_DRCTL(spi_ark->spi_drctl);

	/* set clock speed */
	ctrl |= ark_ecspi_clkdiv(spi_ark, spi_ark->speed_hz, &clk);
	spi_ark->spi_bus_clk = clk;

	/* set chip select to use */
	ctrl |= ARK_ECSPI_CTRL_CS(spi->chip_select);

	ctrl |= (spi_ark->bits_per_word - 1) << ARK_ECSPI_CTRL_BL_OFFSET;

	cfg |= ARK_ECSPI_CONFIG_SBBCTRL(spi->chip_select);

	if (spi->mode & SPI_CPHA)
		cfg |= ARK_ECSPI_CONFIG_SCLKPHA(spi->chip_select);
	else
		cfg &= ~ARK_ECSPI_CONFIG_SCLKPHA(spi->chip_select);

	if (spi->mode & SPI_CPOL) {
		cfg |= ARK_ECSPI_CONFIG_SCLKPOL(spi->chip_select);
		cfg |= ARK_ECSPI_CONFIG_SCLKCTL(spi->chip_select);
	} else {
		cfg &= ~ARK_ECSPI_CONFIG_SCLKPOL(spi->chip_select);
		cfg &= ~ARK_ECSPI_CONFIG_SCLKCTL(spi->chip_select);
	}
	if (spi->mode & SPI_CS_HIGH)
		cfg |= ARK_ECSPI_CONFIG_SSBPOL(spi->chip_select);
	else
		cfg &= ~ARK_ECSPI_CONFIG_SSBPOL(spi->chip_select);

	if (spi_ark->usedma) {
		ctrl |= ARK_ECSPI_CTRL_SMC;
	}

	/* CTRL register always go first to bring out controller from reset */
	writel(ctrl, spi_ark->base + ARK_ECSPI_CTRL);

	reg = readl(spi_ark->base + ARK_ECSPI_TESTREG);
	if (spi->mode & SPI_LOOP)
		reg |= ARK_ECSPI_TESTREG_LBC;
	else
		reg &= ~ARK_ECSPI_TESTREG_LBC;
	writel(reg, spi_ark->base + ARK_ECSPI_TESTREG);

	writel(cfg, spi_ark->base + ARK_ECSPI_CONFIG);

	/*
	 * Wait until the changes in the configuration register CONFIGREG
	 * propagate into the hardware. It takes exactly one tick of the
	 * SCLK clock, but we will wait two SCLK clock just to be sure. The
	 * effect of the delay it takes for the hardware to apply changes
	 * is noticable if the SCLK clock run very slow. In such a case, if
	 * the polarity of SCLK should be inverted, the GPIO ChipSelect might
	 * be asserted before the SCLK polarity changes, which would disrupt
	 * the SPI communication as the device on the other end would consider
	 * the change of SCLK polarity as a clock tick already.
	 */
	delay = (2 * 1000000) / clk;
	if (likely(delay < 10))	/* SCLK is faster than 100 kHz */
		udelay(delay);
	else			/* SCLK is _very_ slow */
		usleep_range(delay, delay + 10);

	/* enable rx fifo */
	writel(ARK_ECSPI_STAT_REN, spi_ark->base + ARK_ECSPI_STAT);

	/*
	 * Configure the DMA register: setup the watermark
	 * and enable DMA request.
	 */
	if (spi_ark->usedma)
		writel(ARK_ECSPI_DMA_RX_WML(spi_ark->wml) |
			ARK_ECSPI_DMA_TX_WML(spi_ark->wml) |
			ARK_ECSPI_DMA_RXT_WML(spi_ark->wml) |
			ARK_ECSPI_DMA_TEDEN | ARK_ECSPI_DMA_RXDEN |
			ARK_ECSPI_DMA_RXTDEN, spi_ark->base + ARK_ECSPI_DMA);
	else writel(0, spi_ark->base + ARK_ECSPI_DMA);

	return 0;
}

static int ark_ecspi_rx_available(struct spi_ark_data *spi_ark)
{
	return readl(spi_ark->base + ARK_ECSPI_STAT) & ARK_ECSPI_STAT_RR;
}

static void ark_ecspi_reset(struct spi_ark_data *spi_ark)
{
	/* drain receive buffer */
	while (ark_ecspi_rx_available(spi_ark))
		readl(spi_ark->base + ARK_ECSPI_RXDATA);
}

static struct spi_ark_devtype_data ark_ecspi_devtype_data = {
	.intctrl = ark_ecspi_intctrl,
	.config = ark_ecspi_config,
	.trigger = ark_ecspi_trigger,
	.rx_available = ark_ecspi_rx_available,
	.reset = ark_ecspi_reset,
	.fifo_size = 64,
	.has_dmamode = true,
};

static const struct of_device_id spi_ark_dt_ids[] = {
	{ .compatible = "arkmicro,ark-ecspi", .data = &ark_ecspi_devtype_data, },
	{ .compatible = "arkmicro,arke-ecspi", .data = &ark_ecspi_devtype_data, },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, spi_ark_dt_ids);

static void spi_ark_chipselect(struct spi_device *spi, int is_active)
{
	int active = is_active != BITBANG_CS_INACTIVE;
	int dev_is_lowactive = !(spi->mode & SPI_CS_HIGH);

	if (spi->mode & SPI_NO_CS)
		return;

	if (!gpio_is_valid(spi->cs_gpio))
		return;

	gpio_set_value(spi->cs_gpio, dev_is_lowactive ^ active);
}

static void spi_ark_push(struct spi_ark_data *spi_ark)
{
	while (spi_ark->txfifo < spi_ark->devtype_data->fifo_size) {
		if (!spi_ark->count)
			break;
		if (spi_ark->txfifo && (spi_ark->count == spi_ark->remainder))
			break;
		spi_ark->tx(spi_ark);
		spi_ark->txfifo++;
	}
	spi_ark->devtype_data->trigger(spi_ark);
}

static irqreturn_t spi_ark_isr(int irq, void *dev_id)
{
	struct spi_ark_data *spi_ark = dev_id;

	while (spi_ark->devtype_data->rx_available(spi_ark)) {
		spi_ark->rx(spi_ark);
		spi_ark->txfifo--;
	}

	if (spi_ark->count) {
		spi_ark_push(spi_ark);
		return IRQ_HANDLED;
	}

	if (spi_ark->txfifo) {
		/* No data left to push, but still waiting for rx data,
		 * enable receive data available interrupt.
		 */
		spi_ark->devtype_data->intctrl(
				spi_ark, ARK_INT_RR);
		return IRQ_HANDLED;
	}

	spi_ark->devtype_data->intctrl(spi_ark, 0);
	complete(&spi_ark->xfer_done);

	return IRQ_HANDLED;
}

static int spi_ark_dma_configure(struct spi_master *master)
{
	int ret;
	struct dma_slave_config rx = {}, tx = {};
	struct spi_ark_data *spi_ark = spi_master_get_devdata(master);

	spi_ark->bits_per_word = 32;

	tx.direction = DMA_MEM_TO_DEV;
	tx.dst_addr = spi_ark->base_phys + ARK_ECSPI_TXDATA;
	tx.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	tx.dst_maxburst = spi_ark->wml;
	tx.device_fc = false;
	ret = dmaengine_slave_config(master->dma_tx, &tx);
	if (ret) {
		dev_err(spi_ark->dev, "TX dma configuration failed with %d\n", ret);
		return ret;
	}

	rx.direction = DMA_DEV_TO_MEM;
	rx.src_addr = spi_ark->base_phys + ARK_ECSPI_RXDATA;
	rx.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	rx.src_maxburst = spi_ark->wml;
	rx.device_fc = false;
	ret = dmaengine_slave_config(master->dma_rx, &rx);
	if (ret) {
		dev_err(spi_ark->dev, "RX dma configuration failed with %d\n", ret);
		return ret;
	}

	return 0;
}

static int spi_ark_setupxfer(struct spi_device *spi,
				 struct spi_transfer *t)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(spi->master);
	u32 mask;
	int ret;

	if (!t)
		return 0;

	spi_ark->bits_per_word = t->bits_per_word;
	spi_ark->speed_hz  = t->speed_hz;

	/* Initialize the functions for transfer */
	spi_ark->remainder = 0;
	spi_ark->read_u32  = 1;

	mask = (1 << spi_ark->bits_per_word) - 1;
	spi_ark->rx = spi_ark_buf_rx_swap;
	spi_ark->tx = spi_ark_buf_tx_swap;
	spi_ark->remainder = t->len;

	if (spi_ark->bits_per_word <= 8)
		spi_ark->word_mask = mask << 24 | mask << 16
						| mask << 8 | mask;
	else if (spi_ark->bits_per_word <= 16)
		spi_ark->word_mask = mask << 16 | mask;
	else
		spi_ark->word_mask = mask;

	if (spi_ark_can_dma(spi_ark->bitbang.master, spi, t))
		spi_ark->usedma = 1;
	else
		spi_ark->usedma = 0;

	if (spi_ark->usedma) {
		ret = spi_ark_dma_configure(spi->master);
		if (ret)
			return ret;
	}

	spi_ark->devtype_data->config(spi);

	return 0;
}

static void spi_ark_sdma_exit(struct spi_ark_data *spi_ark)
{
	struct spi_master *master = spi_ark->bitbang.master;

	if (master->dma_rx) {
		dma_release_channel(master->dma_rx);
		master->dma_rx = NULL;
	}

	if (master->dma_tx) {
		dma_release_channel(master->dma_tx);
		master->dma_tx = NULL;
	}
}

static int spi_ark_sdma_init(struct device *dev, struct spi_ark_data *spi_ark,
			     struct spi_master *master)
{
	int ret;

	spi_ark->wml = spi_ark->devtype_data->fifo_size / 2;

	/* Prepare for TX DMA: */
	master->dma_tx = dma_request_slave_channel_reason(dev, "tx");
	if (IS_ERR(master->dma_tx)) {
		ret = PTR_ERR(master->dma_tx);
		dev_dbg(dev, "can't get the TX DMA channel, error %d!\n", ret);
		master->dma_tx = NULL;
		goto err;
	}

	/* Prepare for RX : */
	master->dma_rx = dma_request_slave_channel_reason(dev, "rx");
	if (IS_ERR(master->dma_rx)) {
		ret = PTR_ERR(master->dma_rx);
		dev_dbg(dev, "can't get the RX DMA channel, error %d\n", ret);
		master->dma_rx = NULL;
		goto err;
	}

	init_completion(&spi_ark->dma_rx_completion);
	init_completion(&spi_ark->dma_tx_completion);
	master->can_dma = spi_ark_can_dma;
	master->max_dma_len = MAX_SDMA_BD_BYTES;
	spi_ark->bitbang.master->flags = SPI_MASTER_MUST_RX |
					 SPI_MASTER_MUST_TX;

	return 0;
err:
	spi_ark_sdma_exit(spi_ark);
	return ret;
}

static void spi_ark_dma_rx_callback(void *cookie)
{
	struct spi_ark_data *spi_ark = (struct spi_ark_data *)cookie;

	/* Invalidate cache after read */
	dma_sync_sg_for_cpu(spi_ark->dev,
				spi_ark->dma_rx_sg->sgl,
				spi_ark->dma_rx_sg->nents,
				DMA_FROM_DEVICE);
	complete(&spi_ark->dma_rx_completion);
}

static void spi_ark_dma_tx_callback(void *cookie)
{
	struct spi_ark_data *spi_ark = (struct spi_ark_data *)cookie;

	complete(&spi_ark->dma_tx_completion);
}

static int spi_ark_calculate_timeout(struct spi_ark_data *spi_ark, int size)
{
	unsigned long timeout = 0;

	/* Time with actual data transfer and CS change delay related to HW */
	timeout = (8 + 4) * size / spi_ark->spi_bus_clk;

	/* Add extra second for scheduler related activities */
	timeout += 1;

	/* Double calculated timeout */
	return msecs_to_jiffies(2 * timeout * MSEC_PER_SEC);
}

static int spi_ark_dma_transfer(struct spi_ark_data *spi_ark,
				struct spi_transfer *transfer)
{
	struct dma_async_tx_descriptor *desc_tx, *desc_rx;
	unsigned long transfer_timeout;
	unsigned long timeout;
	struct spi_master *master = spi_ark->bitbang.master;
	struct sg_table *tx = &transfer->tx_sg, *rx = &transfer->rx_sg;

	/*
	 * The TX DMA setup starts the transfer, so make sure RX is configured
	 * before TX.
	 */
	spi_ark->dma_rx_sg = rx;
	desc_rx = dmaengine_prep_slave_sg(master->dma_rx,
				rx->sgl, rx->nents, DMA_DEV_TO_MEM,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc_rx)
		return -EINVAL;

	desc_rx->callback = spi_ark_dma_rx_callback;
	desc_rx->callback_param = (void *)spi_ark;
	dmaengine_submit(desc_rx);
	reinit_completion(&spi_ark->dma_rx_completion);
	dma_async_issue_pending(master->dma_rx);

	desc_tx = dmaengine_prep_slave_sg(master->dma_tx,
				tx->sgl, tx->nents, DMA_MEM_TO_DEV,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc_tx) {
		dmaengine_terminate_all(master->dma_tx);
		return -EINVAL;
	}

	desc_tx->callback = spi_ark_dma_tx_callback;
	desc_tx->callback_param = (void *)spi_ark;
	dmaengine_submit(desc_tx);
	reinit_completion(&spi_ark->dma_tx_completion);
	/* Flush cache before write */
	dma_sync_sg_for_device(spi_ark->dev, tx->sgl,
					tx->nents, DMA_TO_DEVICE);
	dma_async_issue_pending(master->dma_tx);

	transfer_timeout = spi_ark_calculate_timeout(spi_ark, transfer->len);

	/* Wait SDMA to finish the data transfer.*/
	timeout = wait_for_completion_timeout(&spi_ark->dma_tx_completion,
						transfer_timeout);
	if (!timeout) {
		dev_err(spi_ark->dev, "I/O Error in DMA TX\n");
		dmaengine_terminate_all(master->dma_tx);
		dmaengine_terminate_all(master->dma_rx);
		return -ETIMEDOUT;
	}

	timeout = wait_for_completion_timeout(&spi_ark->dma_rx_completion,
					      transfer_timeout);
	if (!timeout) {
		dev_err(&master->dev, "I/O Error in DMA RX\n");
		spi_ark->devtype_data->reset(spi_ark);
		dmaengine_terminate_all(master->dma_rx);
		return -ETIMEDOUT;
	}

	return transfer->len;
}

static int spi_ark_pio_xfer(struct spi_device *spi,
				struct spi_transfer *transfer)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(spi->master);
	unsigned long transfer_timeout;
	unsigned long timeout;

	spi_ark->tx_buf = transfer->tx_buf;
	spi_ark->rx_buf = transfer->rx_buf;
	spi_ark->count = transfer->len;
	spi_ark->txfifo = 0;
	reinit_completion(&spi_ark->xfer_done);

	spi_ark_push(spi_ark);

	spi_ark->devtype_data->intctrl(spi_ark, ARK_INT_TE);

	transfer_timeout = spi_ark_calculate_timeout(spi_ark, transfer->len);

	timeout = wait_for_completion_timeout(&spi_ark->xfer_done,
					      transfer_timeout);
	if (!timeout) {
		dev_err(&spi->dev, "I/O Error in PIO\n");
		spi_ark->devtype_data->reset(spi_ark);
		return -ETIMEDOUT;
	}

	return transfer->len;
}

static int spi_ark_transfer(struct spi_device *spi,
				struct spi_transfer *transfer)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(spi->master);
	int ret;

	if (spi_ark->usedma) {
		if ((ret = spi_ark_dma_transfer(spi_ark, &spi_ark->dma_transfer)) < 0)
			return ret;
		if (spi_ark->pio_transfer.len > 0 &&
			(ret = spi_ark_pio_xfer(spi, &spi_ark->pio_transfer)) < 0)
			return ret;
		return transfer->len;
	}
	else {
		return spi_ark_pio_xfer(spi, transfer);
	}
}

static int spi_ark_setup(struct spi_device *spi)
{
	dev_dbg(&spi->dev, "%s: mode %d, %u bpw, %d hz\n", __func__,
		 spi->mode, spi->bits_per_word, spi->max_speed_hz);

	if (spi->mode & SPI_NO_CS)
		return 0;

	if (gpio_is_valid(spi->cs_gpio))
		gpio_direction_output(spi->cs_gpio,
				      spi->mode & SPI_CS_HIGH ? 0 : 1);

	spi_ark_chipselect(spi, BITBANG_CS_INACTIVE);

	return 0;
}

static void spi_ark_cleanup(struct spi_device *spi)
{
}

static int
spi_ark_prepare_message(struct spi_master *master, struct spi_message *msg)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(master);
	int ret;

	ret = clk_enable(spi_ark->clk_per);
	if (ret)
		return ret;

	ret = clk_enable(spi_ark->clk_ipg);
	if (ret) {
		clk_disable(spi_ark->clk_per);
		return ret;
	}

	return 0;
}

static int
spi_ark_unprepare_message(struct spi_master *master, struct spi_message *msg)
{
	struct spi_ark_data *spi_ark = spi_master_get_devdata(master);

	clk_disable(spi_ark->clk_ipg);
	clk_disable(spi_ark->clk_per);
	return 0;
}

static int spi_ark_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id =
			of_match_device(spi_ark_dt_ids, &pdev->dev);
	struct spi_master *master;
	struct spi_ark_data *spi_ark;
	struct resource *res;
	int i, ret, irq, spi_drctl;
	u32 num_chipselect, cs_gpio;

	if (!np) {
		dev_err(&pdev->dev, "can't get the platform data\n");
		return -EINVAL;
	}

	master = spi_alloc_master(&pdev->dev, sizeof(struct spi_ark_data));
	if (!master)
		return -ENOMEM;

	ret = of_property_read_u32(np, "ark,spi-rdy-drctl", &spi_drctl);
	if ((ret < 0) || (spi_drctl >= 0x3)) {
		/* '11' is reserved */
		spi_drctl = 0;
	}

	platform_set_drvdata(pdev, master);

	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(1, 32);
	master->bus_num = np ? -1 : pdev->id;

	spi_ark = spi_master_get_devdata(master);
	spi_ark->bitbang.master = master;
	spi_ark->dev = &pdev->dev;

	spi_ark->devtype_data = of_id ? of_id->data :
		(struct spi_ark_devtype_data *)pdev->id_entry->driver_data;

	ret = of_property_read_u32(np, "num-chipselect", &num_chipselect);
	if (ret < 0) {
		dev_err(&pdev->dev, "can't get num-chipselect: %d\n", ret);
		goto out_master_put;
	}
	master->num_chipselect = num_chipselect;
	master->cs_gpios = devm_kzalloc(&master->dev,
		sizeof(int) * master->num_chipselect, GFP_KERNEL);
	if (!master->cs_gpios)
		return -ENOMEM;

	if (of_device_is_compatible(pdev->dev.of_node, "arkmicro,arke-ecspi"))
		spi_ark->is_arke = true;
	else
		spi_ark->is_arke = false;

	for (i = 0; i < master->num_chipselect; i++) {
		of_property_read_u32_index(np, "chipselects", i, &cs_gpio);
		master->cs_gpios[i] = cs_gpio;
	}

	spi_ark->bitbang.chipselect = spi_ark_chipselect;
	spi_ark->bitbang.setup_transfer = spi_ark_setupxfer;
	spi_ark->bitbang.txrx_bufs = spi_ark_transfer;
	spi_ark->bitbang.master->setup = spi_ark_setup;
	spi_ark->bitbang.master->cleanup = spi_ark_cleanup;
	spi_ark->bitbang.master->prepare_message = spi_ark_prepare_message;
	spi_ark->bitbang.master->unprepare_message = spi_ark_unprepare_message;
	spi_ark->bitbang.master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
					     | SPI_NO_CS;
	spi_ark->bitbang.master->mode_bits |= SPI_LOOP | SPI_READY;
	spi_ark->spi_drctl = spi_drctl;

	init_completion(&spi_ark->xfer_done);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	spi_ark->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(spi_ark->base)) {
		ret = PTR_ERR(spi_ark->base);
		goto out_master_put;
	}
	spi_ark->base_phys = res->start;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto out_master_put;
	}

	ret = devm_request_irq(&pdev->dev, irq, spi_ark_isr, 0,
			       dev_name(&pdev->dev), spi_ark);
	if (ret) {
		dev_err(&pdev->dev, "can't get irq%d: %d\n", irq, ret);
		goto out_master_put;
	}

	spi_ark->clk_ipg = devm_clk_get(&pdev->dev, "ipg");
	if (IS_ERR(spi_ark->clk_ipg)) {
		ret = PTR_ERR(spi_ark->clk_ipg);
		goto out_master_put;
	}

	spi_ark->clk_per = devm_clk_get(&pdev->dev, "per");
	if (IS_ERR(spi_ark->clk_per)) {
		ret = PTR_ERR(spi_ark->clk_per);
		goto out_master_put;
	}

	ret = clk_prepare_enable(spi_ark->clk_per);
	if (ret)
		goto out_master_put;

	ret = clk_prepare_enable(spi_ark->clk_ipg);
	if (ret)
		goto out_put_per;

	spi_ark->spi_clk = clk_get_rate(spi_ark->clk_per);

	if (spi_ark->devtype_data->has_dmamode) {
		ret = spi_ark_sdma_init(&pdev->dev, spi_ark, master);
		if (ret == -EPROBE_DEFER)
			goto out_clk_put;

		if (ret < 0)
			dev_err(&pdev->dev, "dma setup error %d, use pio\n",
				ret);
	}

	spi_ark->devtype_data->reset(spi_ark);

	spi_ark->devtype_data->intctrl(spi_ark, 0);

	master->dev.of_node = pdev->dev.of_node;
	ret = spi_bitbang_start(&spi_ark->bitbang);
	if (ret) {
		dev_err(&pdev->dev, "bitbang start failed with %d\n", ret);
		goto out_clk_put;
	}

	if (!master->cs_gpios) {
		dev_err(&pdev->dev, "No CS GPIOs available\n");
		ret = -EINVAL;
		goto out_clk_put;
	}

	for (i = 0; i < master->num_chipselect; i++) {
		if (!gpio_is_valid(master->cs_gpios[i]))
			continue;

		ret = devm_gpio_request(&pdev->dev, master->cs_gpios[i],
					DRIVER_NAME);
		if (ret) {
			dev_err(&pdev->dev, "Can't get CS GPIO %i err = %d.\n",
				master->cs_gpios[i], ret);
			goto out_clk_put;
		}
	}

	dev_info(&pdev->dev, "probed\n");

	clk_disable(spi_ark->clk_ipg);
	clk_disable(spi_ark->clk_per);
	return ret;

out_clk_put:
	clk_disable_unprepare(spi_ark->clk_ipg);
out_put_per:
	clk_disable_unprepare(spi_ark->clk_per);
out_master_put:
	spi_master_put(master);

	return ret;
}

static int spi_ark_remove(struct platform_device *pdev)
{
	struct spi_master *master = platform_get_drvdata(pdev);
	struct spi_ark_data *spi_ark = spi_master_get_devdata(master);
	int ret;

	spi_bitbang_stop(&spi_ark->bitbang);

	ret = clk_enable(spi_ark->clk_per);
	if (ret)
		return ret;

	ret = clk_enable(spi_ark->clk_ipg);
	if (ret) {
		clk_disable(spi_ark->clk_per);
		return ret;
	}

	writel(0, spi_ark->base + ARK_ECSPI_CTRL);
	clk_disable_unprepare(spi_ark->clk_ipg);
	clk_disable_unprepare(spi_ark->clk_per);
	spi_ark_sdma_exit(spi_ark);
	spi_master_put(master);

	return 0;
}

static struct platform_driver spi_ark_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = spi_ark_dt_ids,
	},
	.probe = spi_ark_probe,
	.remove = spi_ark_remove,
};
module_platform_driver(spi_ark_driver);

MODULE_DESCRIPTION("SPI Master Controller driver");
MODULE_AUTHOR("Sim");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);

