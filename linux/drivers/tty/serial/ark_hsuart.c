/*
 *  Driver for AMBA serial ports
 *
 *  Based on drivers/char/serial.c, by Linus Torvalds, Theodore Ts'o.
 *
 *  Copyright 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd.
 *  Copyright (C) 2010 ST-Ericsson SA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is a generic driver for ARM AMBA-type serial ports.  They
 * have a lot of 16550-like features, but are not register compatible.
 * Note that although they do have CTS, DCD and DSR inputs, they do
 * not have an RI input, nor do they have DTR or RTS outputs.  If
 * required, these have to be supplied via some other means (eg, GPIO)
 * and hooked into this driver.
 */
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/platform_device.h>

#include <asm/io.h>
#include <asm-generic/sizes.h>


#define HSUART_DEBUG
#ifdef HSUART_DEBUG
#define hsuart_printk(format, args...)	printk(format, ##args)
#else
#define hsuart_printk(format, args...)
#endif

#define PORT_ARK	120


#define HSUART_NR				2

#define ARK_HSUART_MAJOR		6
#define ARK_HSUART_MINOR		0

#define ARK_ISR_PASS_LIMIT		256

#define HSUART_DR_ERROR			(HSUART_DR_OE | HSUART_DR_BE | HSUART_DR_PE | HSUART_DR_FE)
#define HSUART_DUMMY_DR_RX		(1 << 16)


#define HSUART_WA_SAVE_NR 		13

/* -------------------------------------------------------------------------------
 *  UART Register Offsets.
 */
#define HSUART_RXD		0x00
#define HSUART_TXD		0x40
#define HSUART_UCR1		0x80
#define HSUART_UCR2		0x84
#define HSUART_UCR3		0x88
#define HSUART_UCR4		0x8C
#define HSUART_UFCR		0x90
#define HSUART_USR1		0x94
#define HSUART_USR2		0x98
#define HSUART_UESC		0x9C
#define HSUART_UTIM		0xA0
#define HSUART_UBIR		0xA4
#define HSUART_UBMR		0xA8
#define HSUART_UBRC		0xAC
#define HSUART_ONEMS	0xB0
#define HSUART_UTS		0xB4

#define HSUART_RXDMAEN	(1 << 8)
#define HSUART_TXDMAEN	(1 << 3)

#define HSUART_DR_OE		(1 << 13)
#define HSUART_DR_FE		(1 << 12)
#define HSUART_DR_BE		(1 << 11)
#define HSUART_DR_PE		(1 << 10)

#define HSUART_UTS_RXFF		0x08
#define HSUART_UTS_TXFF		0x10
#define HSUART_UTS_RXFE		0x20
#define HSUART_UTS_TXFE		0x40

#define HSUART_INT_RXD   			(1<<0)
#define HSUART_INT_RXTIMEOUT       	(1<<1)
#define HSUART_INT_TXD   			(1<<2)
#define HSUART_INT_ERR_PARITY		(1<<3)
#define HSUART_INT_ERR_FRAME		(1<<4)
#define HSUART_INT_ERR_OVERRUN		(1<<5)
#define HSUART_INT_RTSD				(1<<6)

#define HSUART_INT_ERR (HSUART_INT_ERR_PARITY | HSUART_INT_ERR_FRAME | HSUART_INT_ERR_OVERRUN)

struct ark_hsuart_data {
	bool (*dma_filter)(struct dma_chan *chan, void *filter_param);
	void *dma_rx_param;
	void *dma_tx_param;
	void (*init) (void);
	void (*exit) (void);
	void (*reset) (void);
};

static const char hsuartx_name[][sizeof("ark_hsuartx")]={
	"ark_hsuart0",
	"ark_hsuart1",
};

struct ark_uart_iface{
	void (*get_mcu_carback_data)(unsigned char ch);
	void (*set_uartx_app_used)(unsigned int uart,bool en);
	unsigned int kernel_uart;
};

static void ark_hsuart_lockup_wa(unsigned long data);

static const u32 hsuart_wa_reg[HSUART_WA_SAVE_NR] = {
	HSUART_UCR1,
	HSUART_UCR2,
	HSUART_UCR3,
	HSUART_UCR4,
	HSUART_UFCR,
	HSUART_USR1,
	HSUART_USR2,
	HSUART_UESC,
	HSUART_UTIM,
	HSUART_UBIR,
	HSUART_UBMR,
	HSUART_UBRC,
	HSUART_ONEMS	,
};

static u32 hsuart_wa_regdata[HSUART_WA_SAVE_NR];

static DECLARE_TASKLET(ark_hsuart_lockup_tlet, ark_hsuart_lockup_wa, 0);

/* Deals with DMA transactions */

struct ark_hsuart_sgbuf {
	struct scatterlist sg;
	char *buf;
};

struct ark_hsuart_dmarx_data {
	struct dma_chan		*chan;
	struct completion	complete;
	bool			use_buf_b;
	struct ark_hsuart_sgbuf	sgbuf_a;
	struct ark_hsuart_sgbuf	sgbuf_b;
	dma_cookie_t		cookie;
	bool			running;
};

struct ark_hsuart_dmatx_data {
	struct dma_chan		*chan;
	struct scatterlist	sg;
	char			*buf;
	bool			queued;
};

/*
 * We wrap our port structure around the generic uart_port.
 */
struct ark_hsuart_port {
	struct uart_port		port;
	struct clk		*clk;
	unsigned int		dmacr;		/* dma control reg */
	unsigned int		fifosize;	/* vendor-specific */
	bool			autorts;
	char			type[12];
	bool			interrupt_may_hang; /* vendor-specific */
	unsigned int	sigs;
#ifdef CONFIG_DMA_ENGINE
	/* DMA stuff */
	bool			using_tx_dma;
	bool			using_rx_dma;
	struct ark_hsuart_dmarx_data dmarx;
	struct ark_hsuart_dmatx_data dmatx;
	bool			dma_probed;
#endif
};

static struct ark_hsuart_port *ark_hsuart_ports[HSUART_NR];

static void ark_hsuart_disable_interrupt(struct ark_hsuart_port *uap, u32 intr)
{
	if(intr & HSUART_INT_RXD)
		writew(readl(uap->port.membase + HSUART_UCR1) & ~(1<<9), uap->port.membase + HSUART_UCR1);

	if(intr & HSUART_INT_RXTIMEOUT)
		writew(readl(uap->port.membase + HSUART_UCR2) & ~(1<<3), uap->port.membase + HSUART_UCR2);

   	if(intr & HSUART_INT_TXD)
		writew(readl(uap->port.membase + HSUART_UCR1) & ~(1<<13), uap->port.membase + HSUART_UCR1);

	if(intr & HSUART_INT_ERR_PARITY)
		writew(readl(uap->port.membase + HSUART_UCR3) & ~(1<<12), uap->port.membase + HSUART_UCR3);

	if(intr & HSUART_INT_ERR_FRAME)
		writew(readl(uap->port.membase + HSUART_UCR3) & ~(1<<11), uap->port.membase + HSUART_UCR3);

	if(intr & HSUART_INT_ERR_OVERRUN)
		writew(readl(uap->port.membase + HSUART_UCR4) & ~(1<<1), uap->port.membase + HSUART_UCR4);

	if(intr & HSUART_INT_RTSD)
		writew(readl(uap->port.membase + HSUART_UCR1) & ~(1<<5), uap->port.membase + HSUART_UCR1);
}

static void ark_hsuart_enable_interrupt(struct ark_hsuart_port *uap, u32 intr)
{
	if(intr & HSUART_INT_RXD)
		writew(readl(uap->port.membase + HSUART_UCR1) | (1<<9), uap->port.membase + HSUART_UCR1);

	if(intr & HSUART_INT_RXTIMEOUT)
		writew(readl(uap->port.membase + HSUART_UCR2) | (1<<3), uap->port.membase + HSUART_UCR2);

   	if(intr & HSUART_INT_TXD)
		writew(readl(uap->port.membase + HSUART_UCR1) | (1<<13), uap->port.membase + HSUART_UCR1);

	if(intr & HSUART_INT_ERR_PARITY)
		writew(readl(uap->port.membase + HSUART_UCR3) | (1<<12), uap->port.membase + HSUART_UCR3);

	if(intr & HSUART_INT_ERR_FRAME)
		writew(readl(uap->port.membase + HSUART_UCR3) | (1<<11), uap->port.membase + HSUART_UCR3);

	if(intr & HSUART_INT_ERR_OVERRUN)
		writew(readl(uap->port.membase + HSUART_UCR4) | (1<<1), uap->port.membase + HSUART_UCR4);

	if(intr & HSUART_INT_RTSD)
		writew(readl(uap->port.membase + HSUART_UCR1) | (1<<5), uap->port.membase + HSUART_UCR1);
}

static void ark_hsuart_clear_interrupt(struct ark_hsuart_port *uap, u32 intr)
{
	/* HSUART_INT_RXD (bit 9, "RX data available") was missing from
	 * this function -- every sibling status bit below (RXTIMEOUT,
	 * the error bits, RTSD) gets an explicit write-1-to-clear, but
	 * this one never did. Confirmed via the reference driver this
	 * file was adapted from (amba-pl011.c, same tree): real PL011-
	 * derived hardware requires an explicit clear of the equivalent
	 * RXIS bit as part of normal interrupt handling
	 * (`pl011_write(UART011_RTIS | UART011_RXIS, uap, REG_ICR)`) --
	 * it is not self-clearing via FIFO reads. Leaving it unacked
	 * means ark_hsuart_irq()'s drain loop (see ARK_ISR_PASS_LIMIT)
	 * can never fully clear pending status under sustained RX
	 * traffic, matching the lockup workaround's own description
	 * ("uart interrupt registers cannot be cleared... uart transfer
	 * gets blocked") -- found while investigating a Bluetooth
	 * firmware-download failure that only manifests on bulk/sustained
	 * transfer, not short AT commands (2026-07-18). */
	if(intr & HSUART_INT_RXD)
		writew(readl(uap->port.membase + HSUART_USR1) | (1<<9), uap->port.membase + HSUART_USR1);

	if(intr & HSUART_INT_RXTIMEOUT)
		writew(readl(uap->port.membase + HSUART_USR1) | (1<<8), uap->port.membase + HSUART_USR1);

	if(intr & HSUART_INT_ERR_PARITY)
		writew(readl(uap->port.membase + HSUART_USR1) | (1<<15), uap->port.membase + HSUART_USR1);

	if(intr & HSUART_INT_ERR_FRAME)
		writew(readl(uap->port.membase + HSUART_USR1) | (1<<10), uap->port.membase + HSUART_USR1);

	if(intr & HSUART_INT_ERR_OVERRUN)
		writew(readl(uap->port.membase + HSUART_USR2) | (1<<1), uap->port.membase + HSUART_USR2);

	if(intr & HSUART_INT_RTSD)
		writew(readl(uap->port.membase + HSUART_USR1) | (1<<12), uap->port.membase + HSUART_USR1);
}

static u32 ark_hsuart_get_interrupt_status(struct ark_hsuart_port *uap)
{
	u32 status = 0;

	if(readl(uap->port.membase + HSUART_USR1) & (1<<9))
		status |= HSUART_INT_RXD;

	if(readl(uap->port.membase + HSUART_USR1) & (1<<8))
		status |= HSUART_INT_RXTIMEOUT;

	if((readl(uap->port.membase + HSUART_USR1) & (1<<13)) &&
		(readl(uap->port.membase + HSUART_UCR1) & (1<<13)))
		status |= HSUART_INT_TXD;

	if(readl(uap->port.membase + HSUART_USR1) & (1<<15))
		status |= HSUART_INT_ERR_PARITY;

	if(readl(uap->port.membase + HSUART_USR1) & (1<<10))
		status |= HSUART_INT_ERR_FRAME;

	if(readl(uap->port.membase + HSUART_USR2) & (1<<1))
		status |= HSUART_INT_ERR_OVERRUN;

	if(readl(uap->port.membase + HSUART_USR1) & (1<<12))
		status |= HSUART_INT_RTSD;

	return status;
}

static inline void ark_hsuart_write_dmacr(struct ark_hsuart_port *uap, unsigned int dmacr)
{
	u32 ucr = readl(uap->port.membase + HSUART_UCR1);
	ucr &= ~(HSUART_RXDMAEN | HSUART_TXDMAEN);
	ucr |= dmacr;
	writew(ucr, uap->port.membase + HSUART_UCR1);
}

/*
 * Reads up to 256 characters from the FIFO or until it's empty and
 * inserts them into the TTY layer. Returns the number of characters
 * read from the FIFO.
 */
static int ark_hsuart_fifo_to_tty(struct ark_hsuart_port *uap)
{
	u16 status, ch;
	unsigned int flag, max_count = 256;
	int fifotaken = 0;
	int hsuart_port = 0;

	if(ark_hsuart_ports[0] == uap){
		hsuart_port = 4;
	}
	else if (ark_hsuart_ports[1] == uap){
		hsuart_port = 5;
	}

	while (max_count--) {
		status = readw(uap->port.membase + HSUART_UTS);
		if (status & HSUART_UTS_RXFE)
			break;

		/* Take chars from the FIFO and update status */
		ch = readw(uap->port.membase + HSUART_RXD) |
			HSUART_DUMMY_DR_RX;
		flag = TTY_NORMAL;
		uap->port.icount.rx++;
		fifotaken++;

		if (unlikely(ch & HSUART_DR_ERROR)) {
			if (ch & HSUART_DR_BE) {
				ch &= ~(HSUART_DR_FE | HSUART_DR_PE);
				uap->port.icount.brk++;
				if (uart_handle_break(&uap->port))
					continue;
			} else if (ch & HSUART_DR_PE)
				uap->port.icount.parity++;
			else if (ch & HSUART_DR_FE)
				uap->port.icount.frame++;
			if (ch & HSUART_DR_OE)
				uap->port.icount.overrun++;

			ch &= uap->port.read_status_mask;

			if (ch & HSUART_DR_BE)
				flag = TTY_BREAK;
			else if (ch & HSUART_DR_PE)
				flag = TTY_PARITY;
			else if (ch & HSUART_DR_FE)
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(&uap->port, ch & 255))
			continue;

		uart_insert_char(&uap->port, ch, HSUART_DR_OE, ch, flag);
	}

	return fifotaken;
}

/*
 * All the DMA operation mode stuff goes inside this ifdef.
 * This assumes that you have a generic DMA device interface,
 * no custom DMA interfaces are supported.
 */
#ifdef CONFIG_DMA_ENGINE

#define ARK_HSUART_DMA_BUFFER_SIZE PAGE_SIZE

static int ark_hsuart_sgbuf_init(struct dma_chan *chan, struct ark_hsuart_sgbuf *sg,
	enum dma_data_direction dir)
{
	dma_addr_t dma_addr;

	sg->buf = dma_alloc_coherent(chan->device->dev,
		ARK_HSUART_DMA_BUFFER_SIZE, &dma_addr, GFP_KERNEL);
	if (!sg->buf)
		return -ENOMEM;

	sg_init_table(&sg->sg, 1);
	sg_set_page(&sg->sg, phys_to_page(dma_addr),
		ARK_HSUART_DMA_BUFFER_SIZE, offset_in_page(dma_addr));
	sg_dma_address(&sg->sg) = dma_addr;
	sg_dma_len(&sg->sg) = ARK_HSUART_DMA_BUFFER_SIZE;

	return 0;
}

static void ark_hsuart_sgbuf_free(struct dma_chan *chan, struct ark_hsuart_sgbuf *sg,
	enum dma_data_direction dir)
{
	if (sg->buf) {
		dma_free_coherent(chan->device->dev,
			ARK_HSUART_DMA_BUFFER_SIZE, sg->buf,
			sg_dma_address(&sg->sg));
	}
}

static void ark_hsuart_dma_probe(struct ark_hsuart_port *uap)
{
	/* DMA is the sole user of the platform data right now */
	struct ark_hsuart_data *plat = dev_get_platdata(uap->port.dev);
	struct device *dev = uap->port.dev;
	struct dma_slave_config tx_conf = {
		.dst_addr = uap->port.mapbase + HSUART_TXD,
		.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
		.direction = DMA_MEM_TO_DEV,
		.dst_maxburst = uap->fifosize >> 1,
		.device_fc = false,
	};
	struct dma_chan *chan;
	dma_cap_mask_t mask;

	uap->dma_probed = true;
	chan = dma_request_slave_channel_reason(dev, "tx");
	if (IS_ERR(chan)) {
		if (PTR_ERR(chan) == -EPROBE_DEFER) {
			uap->dma_probed = false;
			return;
		}

		/* We need platform data */
		if (!plat || !plat->dma_filter) {
			dev_info(uap->port.dev, "no TX platform data\n");
		} else {
			/* Try to acquire a generic DMA engine slave TX channel */
			dma_cap_zero(mask);
			dma_cap_set(DMA_SLAVE, mask);

			chan = dma_request_channel(mask, plat->dma_filter,
							plat->dma_tx_param);
			if (!chan) {
				dev_err(uap->port.dev, "no TX DMA channel!\n");
			} else {
				dmaengine_slave_config(chan, &tx_conf);
				uap->dmatx.chan = chan;

				dev_info(uap->port.dev, "DMA channel TX %s\n",
					 dma_chan_name(uap->dmatx.chan));
			}
		}
	} else {
		dmaengine_slave_config(chan, &tx_conf);
		uap->dmatx.chan = chan;

		dev_info(uap->port.dev, "DMA channel TX %s\n",
			 dma_chan_name(uap->dmatx.chan));
	}

	/* Optionally make use of an RX channel as well */
	chan = dma_request_slave_channel(dev, "rx");

	if (!chan && plat && plat->dma_rx_param) {
		chan = dma_request_channel(mask, plat->dma_filter, plat->dma_rx_param);

		if (!chan) {
			dev_err(uap->port.dev, "no RX DMA channel!\n");
			return;
		}
	}

	if (chan) {
		struct dma_slave_config rx_conf = {
			.src_addr = uap->port.mapbase + HSUART_RXD,
			.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE,
			.direction = DMA_DEV_TO_MEM,
			.src_maxburst = uap->fifosize >> 2,
			.device_fc = false,
		};
		struct dma_slave_caps caps;

		/*
		 * Some DMA controllers provide information on their capabilities.
		 * If the controller does, check for suitable residue processing
		 * otherwise assime all is well.
		 */
		if (0 == dma_get_slave_caps(chan, &caps)) {
			if (caps.residue_granularity ==
					DMA_RESIDUE_GRANULARITY_DESCRIPTOR) {
				dma_release_channel(chan);
				dev_info(uap->port.dev,
					"RX DMA disabled - no residue processing\n");
				return;
			}
		}
		dmaengine_slave_config(chan, &rx_conf);
		uap->dmarx.chan = chan;

		dev_info(uap->port.dev, "DMA channel RX %s\n",
			 dma_chan_name(uap->dmarx.chan));
	}
}

static void ark_hsuart_dma_remove(struct ark_hsuart_port *uap)
{
	/* TODO: remove the initcall if it has not yet executed */
	if (uap->dmatx.chan)
		dma_release_channel(uap->dmatx.chan);
	if (uap->dmarx.chan)
		dma_release_channel(uap->dmarx.chan);
}

/* Forward declare this for the refill routine */
static int ark_hsuart_dma_tx_refill(struct ark_hsuart_port *uap);

/*
 * The current DMA TX buffer has been sent.
 * Try to queue up another DMA buffer.
 */
static void ark_hsuart_dma_tx_callback(void *data)
{
	struct ark_hsuart_port *uap = data;
	struct ark_hsuart_dmatx_data *dmatx = &uap->dmatx;
	unsigned long flags;
	u16 dmacr;

	spin_lock_irqsave(&uap->port.lock, flags);
	if (uap->dmatx.queued)
		dma_unmap_sg(dmatx->chan->device->dev, &dmatx->sg, 1,
			     DMA_TO_DEVICE);

	dmacr = uap->dmacr;
	uap->dmacr = dmacr & ~HSUART_TXDMAEN;
	ark_hsuart_write_dmacr(uap, uap->dmacr);

	/*
	 * If TX DMA was disabled, it means that we've stopped the DMA for
	 * some reason (eg, XOFF received, or we want to send an X-char.)
	 *
	 * Note: we need to be careful here of a potential race between DMA
	 * and the rest of the driver - if the driver disables TX DMA while
	 * a TX buffer completing, we must update the tx queued status to
	 * get further refills (hence we check dmacr).
	 */
	if (!(dmacr & HSUART_TXDMAEN) || uart_tx_stopped(&uap->port) ||
	    uart_circ_empty(&uap->port.state->xmit)) {
		uap->dmatx.queued = false;
		spin_unlock_irqrestore(&uap->port.lock, flags);
		return;
	}

	if (ark_hsuart_dma_tx_refill(uap) <= 0) {
		/*
		 * We didn't queue a DMA buffer for some reason, but we
		 * have data pending to be sent.  Re-enable the TX IRQ.
		 */
		ark_hsuart_enable_interrupt(uap, HSUART_INT_TXD);
	}
	spin_unlock_irqrestore(&uap->port.lock, flags);
}

/*
 * Try to refill the TX DMA buffer.
 * Locking: called with port lock held and IRQs disabled.
 * Returns:
 *   1 if we queued up a TX DMA buffer.
 *   0 if we didn't want to handle this by DMA
 *  <0 on error
 */
static int ark_hsuart_dma_tx_refill(struct ark_hsuart_port *uap)
{
	struct ark_hsuart_dmatx_data *dmatx = &uap->dmatx;
	struct dma_chan *chan = dmatx->chan;
	struct dma_device *dma_dev = chan->device;
	struct dma_async_tx_descriptor *desc;
	struct circ_buf *xmit = &uap->port.state->xmit;
	unsigned int count;

	/*
	 * Try to avoid the overhead involved in using DMA if the
	 * transaction fits in the first half of the FIFO, by using
	 * the standard interrupt handling.  This ensures that we
	 * issue a uart_write_wakeup() at the appropriate time.
	 */
	count = uart_circ_chars_pending(xmit);
	if (count < (uap->fifosize >> 1)) {
		uap->dmatx.queued = false;
		return 0;
	}

	/*
	 * Bodge: don't send the last character by DMA, as this
	 * will prevent XON from notifying us to restart DMA.
	 */
	count -= 1;

	/* Else proceed to copy the TX chars to the DMA buffer and fire DMA */
	if (count > ARK_HSUART_DMA_BUFFER_SIZE)
		count = ARK_HSUART_DMA_BUFFER_SIZE;

	if (xmit->tail < xmit->head)
		memcpy(&dmatx->buf[0], &xmit->buf[xmit->tail], count);
	else {
		size_t first = UART_XMIT_SIZE - xmit->tail;
		size_t second = xmit->head;

		memcpy(&dmatx->buf[0], &xmit->buf[xmit->tail], first);
		if (second)
			memcpy(&dmatx->buf[first], &xmit->buf[0], second);
	}

	dmatx->sg.length = count;

	if (dma_map_sg(dma_dev->dev, &dmatx->sg, 1, DMA_TO_DEVICE) != 1) {
		uap->dmatx.queued = false;
		dev_dbg(uap->port.dev, "unable to map TX DMA\n");
		return -EBUSY;
	}

	desc = dmaengine_prep_slave_sg(chan, &dmatx->sg, 1, DMA_MEM_TO_DEV,
					     DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!desc) {
		dma_unmap_sg(dma_dev->dev, &dmatx->sg, 1, DMA_TO_DEVICE);
		uap->dmatx.queued = false;
		/*
		 * If DMA cannot be used right now, we complete this
		 * transaction via IRQ and let the TTY layer retry.
		 */
		dev_dbg(uap->port.dev, "TX DMA busy\n");
		return -EBUSY;
	}

	/* Some data to go along to the callback */
	desc->callback = ark_hsuart_dma_tx_callback;
	desc->callback_param = uap;

	/* All errors should happen at prepare time */
	dmaengine_submit(desc);

	/* Fire the DMA transaction */
	dma_dev->device_issue_pending(chan);

	uap->dmacr |= HSUART_TXDMAEN;
	ark_hsuart_write_dmacr(uap, uap->dmacr);
	uap->dmatx.queued = true;

	/*
	 * Now we know that DMA will fire, so advance the ring buffer
	 * with the stuff we just dispatched.
	 */
	xmit->tail = (xmit->tail + count) & (UART_XMIT_SIZE - 1);
	uap->port.icount.tx += count;

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&uap->port);

	return 1;
}

/*
 * We received a transmit interrupt without a pending X-char but with
 * pending characters.
 * Locking: called with port lock held and IRQs disabled.
 * Returns:
 *   false if we want to use PIO to transmit
 *   true if we queued a DMA buffer
 */
static bool ark_hsuart_dma_tx_irq(struct ark_hsuart_port *uap)
{
	if (!uap->using_tx_dma)
		return false;

	/*
	 * If we already have a TX buffer queued, but received a
	 * TX interrupt, it will be because we've just sent an X-char.
	 * Ensure the TX DMA is enabled and the TX IRQ is disabled.
	 */
	if (uap->dmatx.queued) {
		uap->dmacr |= HSUART_TXDMAEN;
		ark_hsuart_write_dmacr(uap, uap->dmacr);
		ark_hsuart_disable_interrupt(uap, HSUART_INT_TXD);
		return true;
	}

	/*
	 * We don't have a TX buffer queued, so try to queue one.
	 * If we successfully queued a buffer, mask the TX IRQ.
	 */
	if (ark_hsuart_dma_tx_refill(uap) > 0) {
		ark_hsuart_disable_interrupt(uap, HSUART_INT_TXD);
		return true;
	}
	return false;
}

/*
 * Stop the DMA transmit (eg, due to received XOFF).
 * Locking: called with port lock held and IRQs disabled.
 */
static inline void ark_hsuart_dma_tx_stop(struct ark_hsuart_port *uap)
{
	if (uap->dmatx.queued) {
		uap->dmacr &= ~HSUART_TXDMAEN;
		ark_hsuart_write_dmacr(uap, uap->dmacr);
	}
}

/*
 * Try to start a DMA transmit, or in the case of an XON/OFF
 * character queued for send, try to get that character out ASAP.
 * Locking: called with port lock held and IRQs disabled.
 * Returns:
 *   false if we want the TX IRQ to be enabled
 *   true if we have a buffer queued
 */
static inline bool ark_hsuart_dma_tx_start(struct ark_hsuart_port *uap)
{
	u16 dmacr;

	if (!uap->using_tx_dma)
		return false;

	if (!uap->port.x_char) {
		/* no X-char, try to push chars out in DMA mode */
		bool ret = true;

		if (!uap->dmatx.queued) {
			if (ark_hsuart_dma_tx_refill(uap) > 0) {
				ark_hsuart_disable_interrupt(uap, HSUART_INT_TXD);
				ret = true;
			} else {
				ark_hsuart_enable_interrupt(uap, HSUART_INT_TXD);
				ret = false;
			}
		} else if (!(uap->dmacr & HSUART_TXDMAEN)) {
			uap->dmacr |= HSUART_TXDMAEN;
			ark_hsuart_write_dmacr(uap, uap->dmacr);
		}
		return ret;
	}

	/*
	 * We have an X-char to send.  Disable DMA to prevent it loading
	 * the TX fifo, and then see if we can stuff it into the FIFO.
	 */
	dmacr = uap->dmacr;
	uap->dmacr &= ~HSUART_TXDMAEN;
	ark_hsuart_write_dmacr(uap, uap->dmacr);

	if (readw(uap->port.membase + HSUART_UTS) & HSUART_UTS_TXFF) {
		/*
		 * No space in the FIFO, so enable the transmit interrupt
		 * so we know when there is space.  Note that once we've
		 * loaded the character, we should just re-enable DMA.
		 */
		return false;
	}

	writew(uap->port.x_char, uap->port.membase + HSUART_TXD);
	uap->port.icount.tx++;
	uap->port.x_char = 0;

	/* Success - restore the DMA state */
	uap->dmacr = dmacr;
	ark_hsuart_write_dmacr(uap, uap->dmacr);

	return true;
}

/*
 * Flush the transmit buffer.
 * Locking: called with port lock held and IRQs disabled.
 */
static void ark_hsuart_dma_flush_buffer(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;

	if (!uap->using_tx_dma)
		return;

	/* Avoid deadlock with the DMA engine callback */
	spin_unlock(&uap->port.lock);
	dmaengine_terminate_all(uap->dmatx.chan);
	spin_lock(&uap->port.lock);
	if (uap->dmatx.queued) {
		dma_unmap_sg(uap->dmatx.chan->device->dev, &uap->dmatx.sg, 1,
			     DMA_TO_DEVICE);
		uap->dmatx.queued = false;
		uap->dmacr &= ~HSUART_TXDMAEN;
		ark_hsuart_write_dmacr(uap, uap->dmacr);
	}
}

static void ark_hsuart_dma_rx_callback(void *data);

static int ark_hsuart_dma_rx_trigger_dma(struct ark_hsuart_port *uap)
{
	struct dma_chan *rxchan = uap->dmarx.chan;
	struct ark_hsuart_dmarx_data *dmarx = &uap->dmarx;
	struct dma_async_tx_descriptor *desc;
	struct ark_hsuart_sgbuf *sgbuf;

	if (!rxchan)
		return -EIO;

	/* Start the RX DMA job */
	sgbuf = uap->dmarx.use_buf_b ?
		&uap->dmarx.sgbuf_b : &uap->dmarx.sgbuf_a;
	desc = dmaengine_prep_slave_sg(rxchan, &sgbuf->sg, 1,
					DMA_DEV_TO_MEM,
					DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	/*
	 * If the DMA engine is busy and cannot prepare a
	 * channel, no big deal, the driver will fall back
	 * to interrupt mode as a result of this error code.
	 */
	if (!desc) {
		uap->dmarx.running = false;
		dmaengine_terminate_all(rxchan);
		return -EBUSY;
	}

	/* Some data to go along to the callback */
	desc->callback = ark_hsuart_dma_rx_callback;
	desc->callback_param = uap;
	dmarx->cookie = dmaengine_submit(desc);
	dma_async_issue_pending(rxchan);

	uap->dmacr |= HSUART_RXDMAEN;
	ark_hsuart_write_dmacr(uap, uap->dmacr);
	uap->dmarx.running = true;

	ark_hsuart_disable_interrupt(uap, HSUART_INT_RXD);

	return 0;
}

/*
 * This is called when either the DMA job is complete, or
 * the FIFO timeout interrupt occurred. This must be called
 * with the port spinlock uap->port.lock held.
 */
static void ark_hsuart_dma_rx_chars(struct ark_hsuart_port *uap,
			       u32 pending, bool use_buf_b,
			       bool readfifo)
{
	struct tty_port *port = &uap->port.state->port;
	struct ark_hsuart_sgbuf *sgbuf = use_buf_b ?
		&uap->dmarx.sgbuf_b : &uap->dmarx.sgbuf_a;
	int dma_count = 0;
	u32 fifotaken = 0; /* only used for vdbg() */

	/* Pick everything from the DMA first */
	if (pending) {
		/*
		 * First take all chars in the DMA pipe, then look in the FIFO.
		 * Note that tty_insert_flip_buf() tries to take as many chars
		 * as it can.
		 */
		dma_count = tty_insert_flip_string(port, sgbuf->buf, pending);
		uap->port.icount.rx += dma_count;
		if (dma_count < pending)
			dev_warn(uap->port.dev,
				 "couldn't insert all characters (TTY is full?)\n");
	}

	/*
	 * Only continue with trying to read the FIFO if all DMA chars have
	 * been taken first.
	 */
	if (dma_count == pending && readfifo) {
		/* Clear any error flags */
		ark_hsuart_clear_interrupt(uap, HSUART_INT_ERR);

		/*
		 * If we read all the DMA'd characters, and we had an
		 * incomplete buffer, that could be due to an rx error, or
		 * maybe we just timed out. Read any pending chars and check
		 * the error status.
		 *
		 * Error conditions will only occur in the FIFO, these will
		 * trigger an immediate interrupt and stop the DMA job, so we
		 * will always find the error in the FIFO, never in the DMA
		 * buffer.
		 */
		fifotaken = ark_hsuart_fifo_to_tty(uap);
	}

	spin_unlock(&uap->port.lock);
	dev_vdbg(uap->port.dev,
		 "Took %d chars from DMA buffer and %d chars from the FIFO\n",
		 dma_count, fifotaken);
	tty_flip_buffer_push(port);
	spin_lock(&uap->port.lock);
}

static void ark_hsuart_dma_rx_irq(struct ark_hsuart_port *uap)
{
	struct ark_hsuart_dmarx_data *dmarx = &uap->dmarx;
	struct dma_chan *rxchan = dmarx->chan;
	struct ark_hsuart_sgbuf *sgbuf = dmarx->use_buf_b ?
		&dmarx->sgbuf_b : &dmarx->sgbuf_a;
	size_t pending;
	struct dma_tx_state state;
	enum dma_status dmastat;

	/*
	 * Pause the transfer so we can trust the current counter,
	 * do this before we pause the ARK_HSUART block, else we may
	 * overflow the FIFO.
	 */
	if (dmaengine_pause(rxchan))
		dev_err(uap->port.dev, "unable to pause DMA transfer\n");
	dmastat = rxchan->device->device_tx_status(rxchan,
						   dmarx->cookie, &state);
	if (dmastat != DMA_PAUSED)
		dev_err(uap->port.dev, "unable to pause DMA transfer\n");

	/* Disable RX DMA - incoming data will wait in the FIFO */
	uap->dmacr &= ~HSUART_RXDMAEN;
	ark_hsuart_write_dmacr(uap, uap->dmacr);
	uap->dmarx.running = false;

	pending = sgbuf->sg.length - state.residue;
	BUG_ON(pending > ARK_HSUART_DMA_BUFFER_SIZE);
	/* Then we terminate the transfer - we now know our residue */
	dmaengine_terminate_all(rxchan);

	/*
	 * This will take the chars we have so far and insert
	 * into the framework.
	 */
	ark_hsuart_dma_rx_chars(uap, pending, dmarx->use_buf_b, true);

	/* Switch buffer & re-trigger DMA job */
	dmarx->use_buf_b = !dmarx->use_buf_b;
	if (ark_hsuart_dma_rx_trigger_dma(uap)) {
		dev_dbg(uap->port.dev, "could not retrigger RX DMA job "
			"fall back to interrupt mode\n");
		ark_hsuart_enable_interrupt(uap, HSUART_INT_RXD);
	}
}

static void ark_hsuart_dma_rx_callback(void *data)
{
	struct ark_hsuart_port *uap = data;
	struct ark_hsuart_dmarx_data *dmarx = &uap->dmarx;
	struct dma_chan *rxchan = dmarx->chan;
	bool lastbuf = dmarx->use_buf_b;
	struct ark_hsuart_sgbuf *sgbuf = dmarx->use_buf_b ?
		&dmarx->sgbuf_b : &dmarx->sgbuf_a;
	size_t pending;
	struct dma_tx_state state;
	int ret;

	/*
	 * This completion interrupt occurs typically when the
	 * RX buffer is totally stuffed but no timeout has yet
	 * occurred. When that happens, we just want the RX
	 * routine to flush out the secondary DMA buffer while
	 * we immediately trigger the next DMA job.
	 */
	spin_lock_irq(&uap->port.lock);
	/*
	 * Rx data can be taken by the UART interrupts during
	 * the DMA irq handler. So we check the residue here.
	 */
	rxchan->device->device_tx_status(rxchan, dmarx->cookie, &state);
	pending = sgbuf->sg.length - state.residue;
	BUG_ON(pending > ARK_HSUART_DMA_BUFFER_SIZE);
	/* Then we terminate the transfer - we now know our residue */
	dmaengine_terminate_all(rxchan);

	uap->dmarx.running = false;
	dmarx->use_buf_b = !lastbuf;
	ret = ark_hsuart_dma_rx_trigger_dma(uap);

	ark_hsuart_dma_rx_chars(uap, pending, lastbuf, false);
	spin_unlock_irq(&uap->port.lock);
	/*
	 * Do this check after we picked the DMA chars so we don't
	 * get some IRQ immediately from RX.
	 */
	if (ret) {
		dev_dbg(uap->port.dev, "could not retrigger RX DMA job "
			"fall back to interrupt mode\n");
		ark_hsuart_enable_interrupt(uap, HSUART_INT_RXD);
	}
}

/*
 * Stop accepting received characters, when we're shutting down or
 * suspending this port.
 * Locking: called with port lock held and IRQs disabled.
 */
static inline void ark_hsuart_dma_rx_stop(struct ark_hsuart_port *uap)
{
	/* FIXME.  Just disable the DMA enable */
	uap->dmacr &= ~HSUART_RXDMAEN;
	ark_hsuart_write_dmacr(uap, uap->dmacr);
}

static void ark_hsuart_dma_startup(struct ark_hsuart_port *uap)
{
	int ret;

	if (!uap->dma_probed)
		ark_hsuart_dma_probe(uap);

	if (uap->dmatx.chan) {
		uap->dmatx.buf = kmalloc(ARK_HSUART_DMA_BUFFER_SIZE, GFP_KERNEL | __GFP_DMA);
		if (!uap->dmatx.buf) {
			dev_err(uap->port.dev, "no memory for DMA TX buffer\n");
			uap->port.fifosize = uap->fifosize;
		} else {
			sg_init_one(&uap->dmatx.sg, uap->dmatx.buf, ARK_HSUART_DMA_BUFFER_SIZE);

			/* The DMA buffer is now the FIFO the TTY subsystem can use */
			uap->port.fifosize = ARK_HSUART_DMA_BUFFER_SIZE;
			uap->using_tx_dma = true;
		}
	}

	if (!uap->dmarx.chan)
		goto skip_rx;

	/* Allocate and map DMA RX buffers */
	ret = ark_hsuart_sgbuf_init(uap->dmarx.chan, &uap->dmarx.sgbuf_a,
			       DMA_FROM_DEVICE);
	if (ret) {
		dev_err(uap->port.dev, "failed to init DMA %s: %d\n",
			"RX buffer A", ret);
		goto skip_rx;
	}

	ret = ark_hsuart_sgbuf_init(uap->dmarx.chan, &uap->dmarx.sgbuf_b,
			       DMA_FROM_DEVICE);
	if (ret) {
		dev_err(uap->port.dev, "failed to init DMA %s: %d\n",
			"RX buffer B", ret);
		ark_hsuart_sgbuf_free(uap->dmarx.chan, &uap->dmarx.sgbuf_a,
				 DMA_FROM_DEVICE);
		goto skip_rx;
	}

	uap->using_rx_dma = true;

skip_rx:
	/* Turn on DMA error (RX/TX will be enabled on demand) */
	ark_hsuart_write_dmacr(uap, uap->dmacr);

	/*
	 * ST Micro variants has some specific dma burst threshold
	 * compensation. Set this to 16 bytes, so burst will only
	 * be issued above/below 16 bytes.
	 */
	if (uap->using_rx_dma) {
		if (ark_hsuart_dma_rx_trigger_dma(uap))
			dev_dbg(uap->port.dev, "could not trigger initial "
				"RX DMA job, fall back to interrupt mode\n");
	}
}

static void ark_hsuart_dma_shutdown(struct ark_hsuart_port *uap)
{
	if (!(uap->using_tx_dma || uap->using_rx_dma))
		return;

	/* Disable RX and TX DMA */
	while (!(readw(uap->port.membase + HSUART_UTS) & HSUART_UTS_TXFE))
		barrier();

	spin_lock_irq(&uap->port.lock);
	uap->dmacr &= ~(HSUART_RXDMAEN | HSUART_TXDMAEN);
	ark_hsuart_write_dmacr(uap, uap->dmacr);
	spin_unlock_irq(&uap->port.lock);

	if (uap->using_tx_dma) {
		/* In theory, this should already be done by ark_hsuart_dma_flush_buffer */
		dmaengine_terminate_all(uap->dmatx.chan);
		if (uap->dmatx.queued) {
			dma_unmap_sg(uap->dmatx.chan->device->dev, &uap->dmatx.sg, 1,
				     DMA_TO_DEVICE);
			uap->dmatx.queued = false;
		}

		kfree(uap->dmatx.buf);
		uap->using_tx_dma = false;
	}

	if (uap->using_rx_dma) {
		dmaengine_terminate_all(uap->dmarx.chan);
		/* Clean up the RX DMA */
		ark_hsuart_sgbuf_free(uap->dmarx.chan, &uap->dmarx.sgbuf_a, DMA_FROM_DEVICE);
		ark_hsuart_sgbuf_free(uap->dmarx.chan, &uap->dmarx.sgbuf_b, DMA_FROM_DEVICE);
		uap->using_rx_dma = false;
	}
}

static inline bool ark_hsuart_dma_rx_available(struct ark_hsuart_port *uap)
{
	return uap->using_rx_dma;
}

static inline bool ark_hsuart_dma_rx_running(struct ark_hsuart_port *uap)
{
	return uap->using_rx_dma && uap->dmarx.running;
}


#else /* CONFIG_DMA_ENGINE */
/* Blank functions if the DMA engine is not available */
static inline void ark_hsuart_dma_probe(struct ark_hsuart_port *uap)
{
}

static inline void ark_hsuart_dma_remove(struct ark_hsuart_port *uap)
{
}

static inline void ark_hsuart_dma_startup(struct ark_hsuart_port *uap)
{
}

static inline void ark_hsuart_dma_shutdown(struct ark_hsuart_port *uap)
{
}

static inline bool ark_hsuart_dma_tx_irq(struct ark_hsuart_port *uap)
{
	return false;
}

static inline void ark_hsuart_dma_tx_stop(struct ark_hsuart_port *uap)
{
}

static inline bool ark_hsuart_dma_tx_start(struct ark_hsuart_port *uap)
{
	return false;
}

static inline void ark_hsuart_dma_rx_irq(struct ark_hsuart_port *uap)
{
}

static inline void ark_hsuart_dma_rx_stop(struct ark_hsuart_port *uap)
{
}

static inline int ark_hsuart_dma_rx_trigger_dma(struct ark_hsuart_port *uap)
{
	return -EIO;
}

static inline bool ark_hsuart_dma_rx_available(struct ark_hsuart_port *uap)
{
	return false;
}

static inline bool ark_hsuart_dma_rx_running(struct ark_hsuart_port *uap)
{
	return false;
}

#define ark_hsuart_dma_flush_buffer	NULL
#endif /* CONFIG_DMA_ENGINE */


/*
 * ark_hsuart_lockup_wa
 * This workaround aims to break the deadlock situation
 * when after long transfer over uart in hardware flow
 * control, uart interrupt registers cannot be cleared.
 * Hence uart transfer gets blocked.
 *
 * It is seen that during such deadlock condition ICR
 * don't get cleared even on multiple write. This leads
 * pass_counter to decrease and finally reach zero. This
 * can be taken as trigger point to run this UART_BT_WA.
 *
 */
static void ark_hsuart_lockup_wa(unsigned long data)
{
	struct ark_hsuart_port *uap = ark_hsuart_ports[0];
	void __iomem *base = uap->port.membase;
	struct circ_buf *xmit = &uap->port.state->xmit;
	struct tty_struct *tty = uap->port.state->port.tty;
	int buf_empty_retries = 200;
	int loop;

	/* Stop HCI layer from submitting data for tx */
	tty->hw_stopped = 1;
	while (!uart_circ_empty(xmit)) {
		if (buf_empty_retries-- == 0)
			break;
		udelay(100);
	}

	/* Backup registers */
	for (loop = 0; loop < HSUART_WA_SAVE_NR; loop++)
		hsuart_wa_regdata[loop] = readl(base + hsuart_wa_reg[loop]);

	/* Disable UART so that FIFO data is flushed out */
	writew(0x00, uap->port.membase + HSUART_UCR1);

	/* Soft reset UART module */


	/* Restore registers */
	for (loop = 0; loop < HSUART_WA_SAVE_NR; loop++)
		writew(hsuart_wa_regdata[loop] ,
				uap->port.membase + hsuart_wa_reg[loop]);

	/* Initialise the old status of the modem signals */


	/* Start Tx/Rx */
	tty->hw_stopped = 0;
}

static void ark_hsuart_stop_tx(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;

	ark_hsuart_disable_interrupt(uap, HSUART_INT_TXD);
	ark_hsuart_dma_tx_stop(uap);
}

static void ark_hsuart_start_tx(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;

	if (!ark_hsuart_dma_tx_start(uap)) {
		ark_hsuart_enable_interrupt(uap, HSUART_INT_TXD);
	}
}

static void ark_hsuart_stop_rx(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;

	ark_hsuart_disable_interrupt(uap, HSUART_INT_RXD | HSUART_INT_RXTIMEOUT |
		HSUART_INT_ERR);

	ark_hsuart_dma_rx_stop(uap);
}

static void ark_hsuart_enable_ms(struct uart_port *port)
{
	//struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;

	//uap->im |= UART011_RIMIM|UART011_CTSMIM|UART011_DCDMIM|UART011_DSRMIM;
	//writew(uap->im, uap->port.membase + UART011_IMSC);
}

static void ark_hsuart_rx_chars(struct ark_hsuart_port *uap)
{
	struct tty_port *port = &uap->port.state->port;

	ark_hsuart_fifo_to_tty(uap);

	spin_unlock(&uap->port.lock);
	tty_flip_buffer_push(port);
	/*
	 * If we were temporarily out of DMA mode for a while,
	 * attempt to switch back to DMA mode again.
	 */
	if (ark_hsuart_dma_rx_available(uap)) {
		if (ark_hsuart_dma_rx_trigger_dma(uap)) {
			dev_dbg(uap->port.dev, "could not trigger RX DMA job "
				"fall back to interrupt mode again\n");
			ark_hsuart_enable_interrupt(uap, HSUART_INT_RXD);
		} else
			ark_hsuart_disable_interrupt(uap, HSUART_INT_RXD);
	}
	spin_lock(&uap->port.lock);
}

static void ark_hsuart_tx_chars(struct ark_hsuart_port *uap)
{
	struct circ_buf *xmit = &uap->port.state->xmit;
	int count;

	if (uap->port.x_char) {
		writew(uap->port.x_char, uap->port.membase + HSUART_TXD);
		uap->port.icount.tx++;
		uap->port.x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(&uap->port)) {
		ark_hsuart_stop_tx(&uap->port);
		return;
	}

	/* If we are using DMA mode, try to send some characters. */
	if (ark_hsuart_dma_tx_irq(uap))
		return;

	count = uap->fifosize >> 1;
	do {
		writew(xmit->buf[xmit->tail], uap->port.membase + HSUART_TXD);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		uap->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&uap->port);

	if (uart_circ_empty(xmit))
		ark_hsuart_stop_tx(&uap->port);
}

/* static void ark_hsuart_modem_status(struct ark_hsuart_port *uap)
{
	unsigned int status, delta;

	status = readw(uap->port.membase + UART01x_FR) & UART01x_FR_MODEM_ANY;

	delta = status ^ uap->old_status;
	uap->old_status = status;

	if (!delta)
		return;

	if (delta & UART01x_FR_DCD)
		uart_handle_dcd_change(&uap->port, status & UART01x_FR_DCD);

	if (delta & UART01x_FR_DSR)
		uap->port.icount.dsr++;

	if (delta & UART01x_FR_CTS)
		uart_handle_cts_change(&uap->port, status & UART01x_FR_CTS);

	wake_up_interruptible(&uap->port.state->port.delta_msr_wait);
} */

static irqreturn_t ark_hsuart_int(int irq, void *dev_id)
{
	struct ark_hsuart_port *uap = dev_id;
	unsigned long flags;
	unsigned int status, pass_counter = ARK_ISR_PASS_LIMIT;
	int handled = 0;

	spin_lock_irqsave(&uap->port.lock, flags);

	status = ark_hsuart_get_interrupt_status(uap);

	if (status) {
		do {
			ark_hsuart_clear_interrupt(uap, status);

			if (status & (HSUART_INT_RXD | HSUART_INT_RXTIMEOUT)) {
				if (ark_hsuart_dma_rx_running(uap))
					ark_hsuart_dma_rx_irq(uap);
				else
					ark_hsuart_rx_chars(uap);
			}
			/*if (status & (UART011_DSRMIS|UART011_DCDMIS|
				      UART011_CTSMIS|UART011_RIMIS))
				ark_hsuart_modem_status(uap);*/
			if (status & HSUART_INT_TXD)
				ark_hsuart_tx_chars(uap);

			if (status & HSUART_INT_RTSD)
				uart_handle_cts_change(&uap->port, readw(uap->port.membase + HSUART_USR1) & (1 << 14));

			if (pass_counter-- == 0) {
				if (uap->interrupt_may_hang)
					tasklet_schedule(&ark_hsuart_lockup_tlet);
				break;
			}

			status = ark_hsuart_get_interrupt_status(uap);
		} while (status != 0);
		handled = 1;
	}

	spin_unlock_irqrestore(&uap->port.lock, flags);

	return IRQ_RETVAL(handled);
}

static unsigned int ark_hsuart_tx_empty(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	unsigned int status = readw(uap->port.membase + HSUART_UTS);
	return status & HSUART_UTS_TXFE ? TIOCSER_TEMT : 0;
}

static unsigned int ark_hsuart_get_mctrl(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	unsigned int sigs = 0;

	sigs = (readw(port->membase + HSUART_USR1) & (1 << 14)) ? TIOCM_CTS : 0;
	sigs |= (uap->sigs & TIOCM_RTS);

	return sigs;
}

static void ark_hsuart_set_mctrl(struct uart_port *port, unsigned int sigs)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	unsigned int ucr2, ucr4;

	uap->sigs = sigs;

	if (sigs & TIOCM_RTS) {
		ucr2 = readw(port->membase + HSUART_UCR2);
		ucr2 |= (1 << 13);
		writew(ucr2, port->membase + HSUART_UCR2);
		ucr4 = readw(port->membase + HSUART_UCR4);
		ucr4 &= ~(0x3F << 10);
		ucr4 |= (uap->fifosize * 3 / 4) << 10;
		writew(ucr4, port->membase + HSUART_UCR4);
	} else {
		/* The sibling ark_uart.c driver (pl011_set_mctrl(), which
		 * works) handles both directions symmetrically via a
		 * set/clear macro applied to every mctrl bit. This function
		 * only ever had the assert path -- once anything called it
		 * with TIOCM_RTS set, RTS/auto-flow-control bit 13 could
		 * never be deasserted again through this entry point. */
		ucr2 = readw(port->membase + HSUART_UCR2);
		ucr2 &= ~(1 << 13);
		writew(ucr2, port->membase + HSUART_UCR2);
	}
}

static void ark_hsuart_break_ctl(struct uart_port *port, int break_state)
{
#if 0
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	unsigned long flags;
	unsigned int lcr_h;

	spin_lock_irqsave(&uap->port.lock, flags);
	lcr_h = readw(uap->port.membase + uap->lcrh_tx);
	if (break_state == -1)
		lcr_h |= UART01x_LCRH_BRK;
	else
		lcr_h &= ~UART01x_LCRH_BRK;
	writew(lcr_h, uap->port.membase + uap->lcrh_tx);
	spin_unlock_irqrestore(&uap->port.lock, flags);
#endif
}

static int ark_hsuart_startup(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	int retval;
	int hsuart_port= 0;

	if(ark_hsuart_ports[0] == uap){
		hsuart_port = 4;
	}
	else if (ark_hsuart_ports[1] == uap){
		hsuart_port = 5;
	}

	/* Stock's driver hardcodes this to 24000000 directly and never
	 * calls any clock-framework API at all (confirmed via Ghidra
	 * decompile of ark1680_hsuart_startup, 2026-07-18) -- it doesn't
	 * resolve this from a clk_get_rate() call the way we do. Matching
	 * that here rather than trusting clk_get_rate(uap->clk): our own
	 * boot log shows every other registered clock printing its
	 * resolved rate via clk_sys_recalc_rate()'s banner (cpupll,
	 * uart4clk, uart5clk, etc.) except hsuart0clk/hsuart1clk, which
	 * never print at all, anywhere in any capture -- strong evidence
	 * this call was returning something other than the correct 24MHz
	 * for the two high-speed UART ports specifically (the two-parent
	 * "arkmicro,ark-clk-sys" clocks, unlike their single-parent uart4/
	 * uart5clk siblings which do print correctly). A wrong uartclk
	 * here means every UBMR baud divisor for both ttyHS0 (MCU) and
	 * ttyHS1 (Bluetooth) would be computed wrong -- mistimed
	 * bit-level UART signaling that looks exactly like "the
	 * peripheral never responds", identically on both ports, which
	 * matches the symptom this was found while investigating. */
	uap->port.uartclk = 24000000;

	/* uap->clk (uart4clk/uart5clk) is fetched in probe() via
	 * devm_clk_get() but was never actually turned on anywhere in this
	 * driver -- no clk_prepare_enable() call existed at all, in probe()
	 * or here. clk-sys.c's clk_sys_ops has real .enable/.disable
	 * callbacks that write an actual gate-enable bit to a system
	 * register (not a no-op rate-only clock), and the sibling
	 * ark_uart.c driver (ttyS0-3, which works) does call
	 * clk_prepare_enable() in its own startup(). With nothing ever
	 * enabling it, Linux's common clock framework's boot-time
	 * "disable unused clocks" pass would gate this off outright --
	 * these ports are only ever opened later by userspace
	 * (blueware/MsnCoreApp), long after that pass runs, so the HS UART
	 * peripheral could be sitting on a genuinely disabled clock the
	 * entire time anything tries to talk over it. */
	retval = clk_prepare_enable(uap->clk);
	if (retval)
		return retval;

	/* Clear pending error and receive interrupts */
	ark_hsuart_clear_interrupt(uap, HSUART_INT_RXD | HSUART_INT_RXTIMEOUT |
		HSUART_INT_TXD | HSUART_INT_ERR);

	/*
	 * Allocate the IRQ
	 */
	retval = request_irq(uap->port.irq, ark_hsuart_int, 0,
				         hsuartx_name[uap->port.line], uap);
	if (retval)
		goto clk_dis;

	writew(((uap->fifosize >> 1) << 10) | (uap->fifosize >> 1) | (5 << 7),
			uap->port.membase + HSUART_UFCR);
	writew((1 << 14) | (1 << 12) | 7, uap->port.membase + HSUART_UCR2);
	writew(0xF, uap->port.membase + HSUART_UBIR);
	writew(1, uap->port.membase + HSUART_UCR1);

	/*
	 * Provoke TX FIFO interrupt into asserting.
	 */
	/*writew(0, uap->port.membase + HSUART_TXD);
	while (!(readw(uap->port.membase + HSUART_USR2) & (1 << 3)))
		barrier();*/

	/* restore RTS and DTR */

	/*
	 * initialise the old status of the modem signals
	 */

	/* Startup DMA */
	ark_hsuart_dma_startup(uap);

	/*
	 * Finally, enable interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	if (!ark_hsuart_dma_rx_running(uap))
		ark_hsuart_enable_interrupt(uap, HSUART_INT_RXD | HSUART_INT_RXTIMEOUT | HSUART_INT_ERR);
	else
		ark_hsuart_enable_interrupt(uap, HSUART_INT_RXTIMEOUT | HSUART_INT_ERR);
	spin_unlock_irq(&uap->port.lock);

	return 0;

 clk_dis:
	clk_disable_unprepare(uap->clk);
	return retval;
}


static void ark_hsuart_shutdown(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;

	/*
	 * disable all interrupts
	 */
	spin_lock_irq(&uap->port.lock);
	ark_hsuart_disable_interrupt(uap, HSUART_INT_RXD | HSUART_INT_RXTIMEOUT |
		HSUART_INT_TXD | HSUART_INT_ERR);
	spin_unlock_irq(&uap->port.lock);

	ark_hsuart_dma_shutdown(uap);

	/*
	 * Free the interrupt
	 */
	free_irq(uap->port.irq, uap);

	/*
	 * disable the port
	 * disable the port. It should not disable RTS and DTR.
	 * Also RTS and DTR state should be preserved to restore
	 * it during startup().
	 */
	uap->autorts = false;

	writew(readw(port->membase + HSUART_UCR1) & ~1, uap->port.membase + HSUART_UCR1);

	/*
	 * disable break condition and fifos
	 */

	/*
	 * Shut down the clock producer
	 */
	clk_disable_unprepare(uap->clk);
}

static void
ark_hsuart_set_termios(struct uart_port *port, struct ktermios *termios,
		     struct ktermios *old)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	unsigned int ucr2;
	unsigned long flags;
	unsigned int baud;

	/*
	 * Ask the core to calculate the divisor for us.
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);

	printk("%s %d hsuart%d baud:%d c_cflag:0x%x\n",
		__func__, __LINE__, port->line, baud, termios->c_cflag);

	writew(readw(port->membase + HSUART_UCR1) & ~1, port->membase + HSUART_UCR1);

	ucr2 = readw(port->membase + HSUART_UCR2);
	ucr2 &= ~(0xF << 5);

	switch (termios->c_cflag & CSIZE) {
	case CS8:
		ucr2 |= (1 << 5);
		break;
	}
	if (termios->c_cflag & CSTOPB)
		ucr2 |= (1 << 6);

	if (termios->c_cflag & PARENB) {
		ucr2 |= (1 << 8);
		if (termios->c_cflag & PARODD)
			ucr2 |= (1 << 7);
	}
	if (termios->c_cflag & CRTSCTS) {
		ucr2 &= ~(1 << 14);
		ark_hsuart_enable_interrupt(uap, HSUART_INT_RTSD);
	}
	else {
		ucr2 |= (1 << 14);
		ark_hsuart_disable_interrupt(uap, HSUART_INT_RTSD);
	}
	writew(ucr2, port->membase + HSUART_UCR2);

	spin_lock_irqsave(&port->lock, flags);

	/*
	 * Update the per-port timeout.
	 */
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask = HSUART_DR_OE | 255;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= HSUART_DR_FE | HSUART_DR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= HSUART_DR_BE;

	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= HSUART_DR_FE | HSUART_DR_PE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= HSUART_DR_BE;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= HSUART_DR_OE;
	}

	/*
	 * Ignore all characters if CREAD is not set.
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= HSUART_DUMMY_DR_RX;

	if (UART_ENABLE_MS(port, termios->c_cflag))
		ark_hsuart_enable_ms(port);

	/* Set baud rate */
	writew(0xF, port->membase + HSUART_UBIR);
	writew(port->uartclk / baud - 1, port->membase + HSUART_UBMR);

	writew(readw(port->membase + HSUART_UCR1) | 1, port->membase + HSUART_UCR1);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *ark_hsuart_type(struct uart_port *port)
{
	struct ark_hsuart_port *uap = (struct ark_hsuart_port *)port;
	return uap->port.type == PORT_ARK ? "ARK HS UART" : NULL;
}

/*
 * Configure/autoconfigure the port.
 */
static void ark_hsuart_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		port->type = PORT_ARK;
	}
}

/*
 * verify the new serial_struct (for TIOCSSERIAL).
 */
static int ark_hsuart_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	int ret = 0;
	if (ser->type != PORT_UNKNOWN && ser->type != PORT_ARK)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= nr_irqs)
		ret = -EINVAL;
	if (ser->baud_base < 9600)
		ret = -EINVAL;
	return ret;
}

static struct uart_ops ark_hsuart_pops = {
	.tx_empty	= ark_hsuart_tx_empty,
	.set_mctrl	= ark_hsuart_set_mctrl,
	.get_mctrl	= ark_hsuart_get_mctrl,
	.stop_tx		= ark_hsuart_stop_tx,
	.start_tx	= ark_hsuart_start_tx,
	.stop_rx		= ark_hsuart_stop_rx,
	.enable_ms	= ark_hsuart_enable_ms,
	.break_ctl	= ark_hsuart_break_ctl,
	.startup	= ark_hsuart_startup,
	.shutdown	= ark_hsuart_shutdown,
	.flush_buffer	= ark_hsuart_dma_flush_buffer,
	.set_termios	= ark_hsuart_set_termios,
	.type			= ark_hsuart_type,
	.config_port	= ark_hsuart_config_port,
	.verify_port	= ark_hsuart_verify_port,
};

static struct uart_driver ark_hsuart_reg = {
	.owner			= THIS_MODULE,
	.driver_name		= "ttyHS",
	.dev_name		= "ttyHS",
	.major			= ARK_HSUART_MAJOR,
	.minor			= ARK_HSUART_MINOR,
	.nr				= HSUART_NR,
};

/*
 * This function returns 1 iff pdev isn't a device instatiated by dt, 0 iff it
 * could successfully get all information from dt or a negative errno.
 */
static int ark_hsuart_probe_dt(struct ark_hsuart_port *uap,
		struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;

	ret = of_alias_get_id(np, "hsserial");
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to get alias id, errno %d\n", ret);
		return ret;
	}
	uap->port.line = ret;

	return 0;
}

static int ark_hsuart_probe(struct platform_device *pdev)
{
	struct ark_hsuart_port *uap;
	struct resource *res;
	int irq;
	void __iomem *base = NULL;
	int ret;

	uap = devm_kzalloc(&pdev->dev, sizeof(struct ark_hsuart_port), GFP_KERNEL);
	if (uap == NULL) {
		return -ENOMEM;
	}

	ret = ark_hsuart_probe_dt(uap, pdev);
	if (ret > 0)
		return ret;

	if (uap->port.line >= ARRAY_SIZE(ark_hsuart_ports)) {
		dev_err(&pdev->dev, "serial%d out of range\n",
			uap->port.line);
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq; /* -ENXIO */
	}

	uap->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(uap->clk)) {
		ret = PTR_ERR(uap->clk);
		dev_err(&pdev->dev, "failed to get per clk: %d\n", ret);
		return ret;
	}

	uap->fifosize = 32;
	uap->port.dev = &pdev->dev;
	uap->port.mapbase = res->start;
	uap->port.membase = base;
	uap->port.iotype = UPIO_MEM;
	uap->port.irq = irq;
	uap->port.fifosize = uap->fifosize;
	uap->port.ops = &ark_hsuart_pops;
	uap->port.flags = UPF_BOOT_AUTOCONF;
	spin_lock_init(&uap->port.lock);

	/* Ensure interrupts from this UART are masked and cleared */
	ark_hsuart_disable_interrupt(uap, HSUART_INT_RXD | HSUART_INT_RXTIMEOUT |
		HSUART_INT_TXD | HSUART_INT_ERR);
	ark_hsuart_clear_interrupt(uap, HSUART_INT_RXD | HSUART_INT_RXTIMEOUT |
		HSUART_INT_TXD | HSUART_INT_ERR);

	ark_hsuart_ports[uap->port.line] = uap;

	ret = uart_add_one_port(&ark_hsuart_reg, &uap->port);
	if (ret) {
		goto fail_uart_add_one_port;
	}

	platform_set_drvdata(pdev, uap);

	return 0;

fail_uart_add_one_port:
	platform_set_drvdata(pdev, NULL);
	ark_hsuart_ports[uap->port.line] = NULL;
	ark_hsuart_dma_remove(uap);

	return ret;
}

static int ark_hsuart_remove(struct platform_device *pdev)
{
	struct ark_hsuart_port *uap = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	uart_remove_one_port(&ark_hsuart_reg, &uap->port);
	ark_hsuart_dma_remove(uap);
	iounmap(uap->port.membase);
	kfree(uap);

	return 0;
}

#ifdef CONFIG_PM
static int ark_hsuart_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ark_hsuart_port *uap = platform_get_drvdata(pdev);

	if (!uap)
		return -EINVAL;

	return uart_suspend_port(&ark_hsuart_reg, &uap->port);
}

static int ark_hsuart_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ark_hsuart_port *uap = platform_get_drvdata(pdev);

	if (!uap)
		return -EINVAL;

	return uart_resume_port(&ark_hsuart_reg, &uap->port);
}

static const struct dev_pm_ops ark_hsuart_pm_ops = {
	.suspend = ark_hsuart_suspend,
	.resume = ark_hsuart_resume,
};
#endif

static const struct of_device_id ark_hsuart_dt_ids[] = {
	{ .compatible = "arkmicro,ark-hsuart", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ark_hsuart_dt_ids);

static struct platform_driver ark_hsuart_driver = {
	.probe          = ark_hsuart_probe,
	.remove         = ark_hsuart_remove,
	.driver		= {
		.name	= "ark-hsuart",
		.of_match_table = ark_hsuart_dt_ids,
#ifdef CONFIG_PM
		.pm	= &ark_hsuart_pm_ops,
#endif
	},
};

static int __init ark_hsuart_init(void)
{
	int ret;

	ret = uart_register_driver(&ark_hsuart_reg);
	if (ret == 0) {
		ret = platform_driver_register(&ark_hsuart_driver);
		if (ret)
			uart_unregister_driver(&ark_hsuart_reg);
	}
	return ret;
}

static void __exit ark_hsuart_exit(void)
{
	platform_driver_unregister(&ark_hsuart_driver);
	uart_unregister_driver(&ark_hsuart_reg);
}

/*
 * While this can be a module, if builtin it's most likely the console
 * So let's leave module_exit but move module_init to an earlier place
 */
arch_initcall(ark_hsuart_init);
module_exit(ark_hsuart_exit);

MODULE_AUTHOR("Sim, Arkmicro Ltd");
MODULE_DESCRIPTION("ARK high speed serial port driver");
MODULE_LICENSE("GPL");
