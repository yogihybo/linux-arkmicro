/*
 * Texas Instruments DSPS platforms "glue layer"
 *
 * Copyright (C) 2012, by Texas Instruments
 *
 * Based on the am35x "glue layer" code.
 *
 * This file is part of the Inventra Controller Driver for Linux.
 *
 * The Inventra Controller Driver for Linux is free software; you
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 2 as published by the Free Software
 * Foundation.
 *
 * The Inventra Controller Driver for Linux is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with The Inventra Controller Driver for Linux ; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 *
 * musb_ark.c will be a common file for all the TI DSPS platforms
 * such as dm64x, dm36x, dm35x, da8x, am35x and ti81x.
 * For now only ti81x is using this and in future davinci.c, am35x.c
 * da8xx.c would be merged to this file after testing.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/usb/usb_phy_generic.h>
#include <linux/platform_data/usb-omap.h>
#include <linux/sizes.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/usb/of.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>
#include <linux/debugfs.h>
#include <linux/usb/musb.h>
#include <linux/usb.h>


#include "musb_core.h"
#include "musb_ark.h"
#include "musb_gadget.h"
#include "cppi_dma.h"
#include "musb_trace.h"



/**
 * ARK glue structure.
 */
struct ark_glue {
	struct device			*dev;
	struct platform_device	*plat_dev;
	struct musb *musb;
	//struct platform_device	*phy;
	struct clk				*clk;
	struct timer_list timer;	/* otg_workaround timer */
	int	gpio_id;
	int	gpio_pwr;
	int	usb_id_reg;
	int	usb_id_offset;
	void __iomem 	*sys_base;
	unsigned int usb_softrest_reg_offset;
	unsigned int usb_softrest_bit_offset;
	unsigned int usbphy_softrest_bit_offset;
};


/* USBSS  / USB ARK */
#define USBSS_IRQ_STATUS	0x28
#define USBSS_IRQ_ENABLER	0x2c
#define USBSS_IRQ_CLEARR	0x30
#define USBSS_IRQ_PD_COMP	(1 << 2)

#define	POLL_SECONDS	2
#define	is_peripheral_enabled(musb)	((musb)->port_mode != MUSB_HOST)
#define	is_host_enabled(musb)		((musb)->port_mode != MUSB_PERIPHERAL)
#define	is_otg_enabled(musb)		((musb)->port_mode == MUSB_OTG)
#define	portstate(stmt)		stmt


static int dma_off = 1;
static u64 musb_dmamask = DMA_BIT_MASK(32);


/**
 * ark_musb_enable - enable interrupts
 */
static void ark_musb_enable(struct musb *musb)
{
	musb_writeb(musb->ctrl_base, MUSB_INTRRXE, BIT(1) | BIT(2) | BIT(3) | BIT(4));	
	musb_writeb(musb->ctrl_base, MUSB_INTRTXE, BIT(1) | BIT(2) | BIT(3) | BIT(4));	
	musb_writeb(musb->ctrl_base, MUSB_INTRUSBE, (0xff & (~(BIT(3)))) );	

	musb_readb(musb->ctrl_base, MUSB_INTRRX);
	musb_readb(musb->ctrl_base, MUSB_INTRTX);
	musb_readb(musb->ctrl_base, MUSB_INTRUSB);

	if (is_dma_capable() && !dma_off)
		dev_warn(musb->controller, "%s: dma not reactivated\n", __func__);
	else
		dma_off = 0;
}

/**
 * ark_musb_disable - disable HDRC and flush interrupts
 */
static void ark_musb_disable(struct musb *musb)
{
	musb_writeb(musb->mregs, MUSB_DEVCTL, 0);

	if (is_dma_capable() && !dma_off)
		WARNING("dma still active\n");
}

static void ark_musb_otg_timer(struct timer_list *t)
{
	struct ark_glue *glue = from_timer(glue, t, timer);
	struct musb *musb = glue->musb;
	void __iomem		*mregs = musb->mregs;
	unsigned long flags;
	u8 devctl;

	/* We poll because ArkMicro's won't expose several OTG-critical
	* status change events (from the transceiver) otherwise.
	 */
	devctl = musb_readb(mregs, MUSB_DEVCTL);
	//dev_dbg(musb->controller, "poll devctl %02x (%s)\n", devctl,
	//	otg_state_string(musb->xceiv->state));
	dev_dbg(musb->controller, "poll devctl %02x (%d)\n", devctl, musb->xceiv->otg->state);

	spin_lock_irqsave(&musb->lock, flags);
	//switch (musb->xceiv->state) {
	switch (musb->xceiv->otg->state) {
		case OTG_STATE_A_WAIT_VFALL:
			/* Wait till VBUS falls below SessionEnd (~0.2V); the 1.3 RTL
			 * seems to mis-handle session "start" otherwise (or in our
			 * case "recover"), in routine "VBUS was valid by the time
			 * VBUSERR got reported during enumeration" cases.
			 */
			if (devctl & MUSB_DEVCTL_VBUS) {
				mod_timer(&glue->timer, jiffies + POLL_SECONDS * HZ);
				break;
			}
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_VRISE;
			musb_writel(musb->ctrl_base, ARK_USB_INT_SET_REG,
				MUSB_INTR_VBUSERROR << ARK_USB_USBINT_SHIFT);
			break;
		case OTG_STATE_B_IDLE:
			if (!is_peripheral_enabled(musb))
				break;

			/* There's no ID-changed IRQ, so we have no good way to tell
			 * when to switch to the A-Default state machine (by setting
			 * the DEVCTL.SESSION flag).
			 *
			 * Workaround:	whenever we're in B_IDLE, try setting the
			 * session flag every few seconds.	If it works, ID was
			 * grounded and we're now in the A-Default state machine.
			 *
			 * NOTE setting the session flag is _supposed_ to trigger
			 * SRP, but clearly it doesn't.
			 */
			musb_writeb(mregs, MUSB_DEVCTL, devctl | MUSB_DEVCTL_SESSION);
			devctl = musb_readb(mregs, MUSB_DEVCTL);
			if (devctl & MUSB_DEVCTL_BDEVICE)
				mod_timer(&glue->timer, jiffies + POLL_SECONDS * HZ);
			else
				musb->xceiv->otg->state = OTG_STATE_A_IDLE;
			break;
		default:
			break;
	}
	spin_unlock_irqrestore(&musb->lock, flags);
}

static irqreturn_t ark_musb_interrupt(int irq, void *hci)
{
	struct musb *musb = (struct musb *)hci;
	void __iomem *reg_base = musb->ctrl_base;
	struct device *dev = musb->controller;
	struct ark_glue *glue = dev_get_drvdata(dev->parent);
	struct cppi	*cppi;
	irqreturn_t ret = IRQ_NONE;
	unsigned long	flags;

	spin_lock_irqsave(&musb->lock, flags);

	cppi = container_of(musb->dma_controller, struct cppi, controller);
	if (is_cppi_enabled() && musb->dma_controller && !cppi->irq)
		ret = cppi_interrupt(irq, hci);

	musb->int_rx = musb_readb(reg_base, MUSB_INTRRX); 
	musb->int_tx = musb_readb(reg_base, MUSB_INTRTX);
	musb->int_usb = musb_readb(reg_base, MUSB_INTRUSB);

	/* DRVVBUS irqs are the only proxy we have (a very poor one!) for
	 * ArkMicro's missing ID change IRQ.  We need an ID change IRQ to
	 * switch appropriately between halves of the OTG state machine.
	 * Managing DEVCTL.SESSION per Mentor docs requires we know its
	 * value, but DEVCTL.BDEVICE is invalid without DEVCTL.SESSION set.
	 * Also, DRVVBUS pulses for SRP (but not at 5V) ...
	 */
	if(0) {
		int err = 0;
		struct usb_otg *otg = musb->xceiv->otg;

		err = is_host_enabled(musb) && (musb->int_usb & MUSB_INTR_VBUSERROR);
		if (err) {
			/* The Mentor core doesn't debounce VBUS as needed
			 * to cope with device connect current spikes. This
			 * means it's not uncommon for bus-powered devices
			 * to get VBUS errors during enumeration.
			 *
			 * This is a workaround, but newer RTL from Mentor
			 * seems to allow a better one: "re"starting sessions
			 * without waiting (on EVM, a **long** time) for VBUS
			 * to stop registering in devctl.
			 */
			musb->int_usb &= ~MUSB_INTR_VBUSERROR;
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_VFALL;
			mod_timer(&glue->timer, jiffies + POLL_SECONDS * HZ);
			WARNING("VBUS error workaround (delay coming)\n");
		} 
		else if (is_host_enabled(musb)) {
			MUSB_HST_MODE(musb);
			otg->default_a = 1;
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_VRISE;
			portstate(musb->port1_status |= USB_PORT_STAT_POWER);
			del_timer(&glue->timer);
		}
		else {
			musb->is_active = 0;
			MUSB_DEV_MODE(musb);
			otg->default_a = 0;
			musb->xceiv->otg->state = OTG_STATE_B_IDLE;
			portstate(musb->port1_status &= ~USB_PORT_STAT_POWER);
		}

		/* NOTE:  this must complete poweron within 100 msec
		 * (OTG_TIME_A_WAIT_VRISE) but we don't check for that.
		 */
		ret = IRQ_HANDLED;
	}

	if (musb->int_tx || musb->int_rx || musb->int_usb)
		ret |= musb_interrupt(musb);

	/* poll for ID change */
	if (is_otg_enabled(musb) && (musb->xceiv->otg->state == OTG_STATE_B_IDLE))
		mod_timer(&glue->timer, jiffies + POLL_SECONDS * HZ);

	spin_unlock_irqrestore(&musb->lock, flags);

	return ret;
}

/*
 * DIAGNOSTIC ONLY -- added for the usb0 dr_mode=host regression
 * investigation (docs/WIRELESS_AND_INIT.md sec 7). Reads both ports'
 * soft-reset registers and both ports' known id/pwr GPIOs regardless of
 * which glue instance is calling, to check for any cross-port coupling
 * when one port's negotiation (or its absence) changes timing. The DTS
 * softrest reg-offset/bit-offset values (usb0: 0x74 bit 5, usb1: 0x78
 * bit 6) are distinct words/bits, so no register collision is expected
 * here -- this is to catch anything the DTS values don't capture
 * (shared rail, PHY reference, bus-probe-order-driven renumbering).
 * Remove once the regression is understood; not for production.
 */
static void ark_musb_dump_cross_port_state(struct ark_glue *glue, int port_id, const char *tag)
{
	u32 rest0, rest1;

	if (!glue->sys_base)
		return;

	rest0 = readl((void __iomem *)((unsigned int)glue->sys_base + 0x74));
	rest1 = readl((void __iomem *)((unsigned int)glue->sys_base + 0x78));

	pr_info("ark_musb_diag[port%d %s] t=%u ms: softrest0(0x74)=0x%08x softrest1(0x78)=0x%08x "
		"usb0_id(gpio76)=%d usb0_pwr(gpio126)=%d usb1_id(gpio1)=%d usb1_pwr(gpio117)=%d\n",
		port_id, tag, jiffies_to_msecs(jiffies), rest0, rest1,
		gpio_get_value(76), gpio_get_value(126),
		gpio_get_value(1), gpio_get_value(117));
}

static int ark_musb_init(struct musb *musb)
{
	struct device *dev = musb->controller;
	struct ark_glue *glue = dev_get_drvdata(dev->parent);
	struct platform_device *parent = to_platform_device(dev->parent);
	u32 val;

	if(!glue || !parent)
		return -ENODEV;

	glue->musb = musb;

	if (NULL == musb->xceiv) {
		char name[16] = {0};
		struct device_node *phynode = NULL;
		sprintf(name, "usb%d-phy", parent->id);
		phynode = of_find_node_by_name(phynode, name);
		if (NULL == phynode) {
			dev_err(dev, "%s: failed to of_find_node_by_name(%s)\n", __func__, name);
			return PTR_ERR(musb->xceiv);
		}

		musb->xceiv = devm_usb_get_phy_by_node(&parent->dev, phynode, NULL);
		of_node_put(phynode);
		if (IS_ERR(musb->xceiv)) {
			dev_err(dev, "%s: failed to usb_get_phy_by_node\n", __func__);
			return PTR_ERR(musb->xceiv);
		}
	}

	if(is_host_enabled(musb)) {
		timer_setup(&glue->timer, ark_musb_otg_timer, 0);
	}

	/* Reset the musb */
	val = musb_readb(musb->ctrl_base, 0x01);
	val |= BIT(3);
	musb_writeb(musb->ctrl_base, 0x01, val);

	msleep(5);
	musb->isr = ark_musb_interrupt;

	if(glue->gpio_pwr >= 0) {
		if (devm_gpio_request(dev, glue->gpio_pwr, "usb_pwr") < 0) {
			dev_err(dev, "Failed to request usb pwr gpio%d\n", glue->gpio_pwr);
			goto fail;
		}
		gpio_direction_output(glue->gpio_pwr, 1);
	}
	if(glue->gpio_id >= 0) {
		if (devm_gpio_request(dev, glue->gpio_id, "usb_id") < 0) {
			dev_err(dev, "Failed to request usb id gpio%d\n", glue->gpio_id);
			goto fail;
		} 
		gpio_direction_output(glue->gpio_id, 0);
	}
	ark_musb_dump_cross_port_state(glue, parent->id, "init-done");
	return 0;

fail:
	return -ENODEV;
}

static int ark_musb_exit(struct musb *musb)
{
	struct device *dev = musb->controller;
	struct ark_glue *glue = dev_get_drvdata(dev->parent);

	if(!glue || !musb->xceiv)
		return -ENODEV;

	if (is_host_enabled(musb))
		del_timer_sync(&glue->timer);

	/* delay, to avoid problems with module reload */
	if (is_host_enabled(musb) && musb->xceiv->otg->default_a) {
		int	maxdelay = 2;
		u8	devctl, warn = 0;

		/* if there's no peripheral connected, this can take a
		 * long time to fall, especially on EVM with huge C133.
		 */
		do {
			devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
			if (!(devctl & MUSB_DEVCTL_VBUS))
				break;
			if ((devctl & MUSB_DEVCTL_VBUS) != warn) {
				warn = devctl & MUSB_DEVCTL_VBUS;
				dev_dbg(musb->controller, "VBUS %d\n",
					warn >> MUSB_DEVCTL_VBUS_SHIFT);
			}
			msleep(10);
			maxdelay--;
		} while (maxdelay > 0);

		/* in OTG mode, another host might be connected */
		if (devctl & MUSB_DEVCTL_VBUS)
			dev_dbg(musb->controller, "VBUS off timeout (devctl %02x)\n", devctl);
	}
	
	usb_put_phy(musb->xceiv);

	return 0;
}

static int ark_musb_set_mode(struct musb *musb, u8 mode)
{
	struct device *dev = musb->controller;
	struct ark_glue *glue = dev_get_drvdata(dev->parent);
	struct platform_device *parent = to_platform_device(dev->parent);
	int gpio_pwr = glue->gpio_pwr;
	int gpio_id = glue->gpio_id;
	int usb_id_reg = glue->usb_id_reg;
	int usb_id_offset = glue->usb_id_offset;
	u32 bitoffset = glue->usb_softrest_bit_offset;
	u32 phybitoffset = glue->usbphy_softrest_bit_offset;
	void __iomem *sys_softrest_base = (void __iomem *)((unsigned int)glue->sys_base + glue->usb_softrest_reg_offset);
	u32 regval;

	ark_musb_dump_cross_port_state(glue, parent->id, "set_mode-entry");

	/*usb_hcd_resume_root_hub(musb->hcd);
	musb_root_disconnect(musb);
	musb_g_reset(musb);*/

	switch (mode) {
		case MUSB_HOST:
			break;
		case MUSB_PERIPHERAL:
			dev_info(musb->controller, "+Switch peripheral %d  %d=== \n", gpio_id, gpio_pwr);
			musb->xceiv->otg->state = OTG_STATE_B_IDLE;

			if (gpio_id >= 0)
				gpio_set_value(gpio_id, 1);
			else if((usb_id_reg >= 0) && (usb_id_offset >= 0)) {
				void __iomem *sys_usb_id_base = (void __iomem *)((unsigned int)glue->sys_base + glue->usb_id_reg);
				regval = readl(sys_usb_id_base);
				regval |= (1<<usb_id_offset);
				writel(regval, sys_usb_id_base);
			}

			regval = readl(sys_softrest_base);
			regval &= (~((1 << bitoffset) | (1 << phybitoffset)));//usb and usbphy
			writel(regval, sys_softrest_base);
			mdelay(1);
			regval = readl(sys_softrest_base);
			regval |= ((1 << bitoffset) | (1 << phybitoffset));
			writel(regval, sys_softrest_base);

			mdelay(1);

			musb_writeb(musb->mregs, MUSB_DEVCTL, 0x99);
			musb_writeb(musb->mregs,0x01,0xf0);
			musb_g_reset(musb);
			
			regval = musb_readb(musb->mregs, MUSB_INTRUSBE); 
			musb_writeb(musb->mregs, MUSB_INTRUSBE, regval | MUSB_INTR_SUSPEND);
			if (musb->is_runtime_suspended) {
				printk("%s:%d suspend save\n", __func__, __LINE__);
				musb->context.power = musb_readb(musb->mregs, MUSB_POWER);
				musb->context.intrusbe = musb_readb(musb->mregs, MUSB_INTRUSBE);
				musb->context.devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
			}
			break;
		case MUSB_OTG:
			dev_info(musb->controller, "+++Switch OTG %d  %d===+++ \n", gpio_id, gpio_pwr);
			musb->xceiv->otg->state = OTG_STATE_A_HOST;
			if (gpio_id >= 0)
				gpio_set_value(gpio_id, 0);
			else if((usb_id_reg >= 0) && (usb_id_offset >= 0)) {
				void __iomem *sys_usb_id_base = (void __iomem *)((unsigned int)glue->sys_base + glue->usb_id_reg);
				regval = readl(sys_usb_id_base);
				regval &= ~(1<<usb_id_offset);
				writel(regval, sys_usb_id_base);
			}
			regval = readl(sys_softrest_base);
			regval &= (~(1 << bitoffset));
			writel(regval, sys_softrest_base);
			mdelay(1);
			regval = readl(sys_softrest_base);
			regval |= ((1 << bitoffset));
			writel(regval, sys_softrest_base);

			mdelay(1);
			
			musb_writeb(musb->mregs, MUSB_INTRUSBE, 0xf4);
			musb_writeb(musb->mregs, MUSB_DEVCTL, 0x99);
			if (gpio_pwr >= 0) {
				gpio_set_value(gpio_pwr, 0);
				mdelay(10);
				gpio_set_value(gpio_pwr, 1);
				/* If a device is already plugged into this port when
				 * this VBUS power-cycle happens (e.g. a USB stick
				 * present at boot), it loses power and has to fully
				 * re-initialize internally before it can respond to
				 * enumeration again — confirmed on real hardware via
				 * the stock application's own log, which shows several
				 * seconds between insertion and the device being
				 * usable, plus explicit open() retry logic on its side
				 * for exactly this reason. This function returns right
				 * after this block with no wait-for-connect check at
				 * all (see ark_musb_set_vbus() below for the pattern
				 * this should arguably use instead — a real status-bit
				 * poll with a timeout — but that needs correctly
				 * understanding this register's semantics, which
				 * hasn't been verified). The previous 10ms-then-nothing
				 * gap was nowhere near enough settle time, and with
				 * nothing here to keep the port from being probed as
				 * "connected" too early, the generic USB core's own
				 * hotplug polling can end up never re-noticing the
				 * device at all if it wasn't ready yet — matching the
				 * "device never enumerates on this port no matter how
				 * long you wait afterward" symptom root-caused this
				 * session. Give the device real time to power back up
				 * before this function returns and driver init
				 * continues. */
				mdelay(300);
			}

			if (musb->is_runtime_suspended) {
				printk("%s:%d suspend save\n", __func__, __LINE__);
				musb->context.power = musb_readb(musb->mregs, MUSB_POWER);
				musb->context.intrusbe = musb_readb(musb->mregs, MUSB_INTRUSBE);
				musb->context.devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
			}
			break;
		default:
			dev_err(glue->dev, "unsupported mode %d\n", mode);
			return -EINVAL;
	}

	ark_musb_dump_cross_port_state(glue, parent->id, "set_mode-exit");
	return 0;
}

static void ark_musb_set_vbus(struct musb *musb, int is_on)
{
	struct usb_otg	*otg = musb->xceiv->otg;
	u8 devctl;
	unsigned long timeout = jiffies + msecs_to_jiffies(1000);
	int ret = 1;

	/* HDRC controls CPEN, but beware current surges during device
	 * connect.  They can trigger transient overcurrent conditions
	 * that must be ignored.
	 */
		
	devctl = musb_readb(musb->mregs, MUSB_DEVCTL);

	if (is_on) {
		if (musb->xceiv->otg->state == OTG_STATE_A_IDLE) {
			/* start the session */
			devctl |= MUSB_DEVCTL_SESSION;
			musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
			/*
			 * Wait for the musb to set as A device to enable the VBUS
			 */
			while (musb_readb(musb->mregs, MUSB_DEVCTL) & 0x80) {
				cpu_relax();

				if (time_after(jiffies, timeout)) {
					dev_err(musb->controller, "configured as A device timeout");
					ret = -EINVAL;
					break;
				}
			}

			if (ret && otg->set_vbus)
				otg_set_vbus(otg, 1);
		} else {
			musb->is_active = 1;
			otg->default_a = 1;
			musb->xceiv->otg->state = OTG_STATE_A_WAIT_VRISE;
			devctl |= MUSB_DEVCTL_SESSION;
			MUSB_HST_MODE(musb);
		}
	} else {
		musb->is_active = 0;

		/* NOTE:  we're skipping A_WAIT_VFALL -> A_IDLE and
		 * jumping right to B_IDLE...
		 */

		otg->default_a = 0;
		musb->xceiv->otg->state = OTG_STATE_B_IDLE;
		devctl &= ~MUSB_DEVCTL_SESSION;

		MUSB_DEV_MODE(musb);
	}

	musb_writeb(musb->mregs, MUSB_DEVCTL, devctl);
}

#ifdef CONFIG_USB_TI_CPPI41_DMA
static void ark_dma_controller_callback(struct dma_controller *c)
{
	struct musb *musb = c->musb;
	struct ark_glue *glue = dev_get_drvdata(musb->controller->parent);
	void __iomem *usbss_base =  musb->ctrl_base;
	u32 status;

	status = musb_readl(usbss_base, USBSS_IRQ_STATUS);
	if (status & USBSS_IRQ_PD_COMP)
		musb_writel(usbss_base, USBSS_IRQ_STATUS, USBSS_IRQ_PD_COMP);
}

static struct dma_controller *ark_dma_controller_create(struct musb *musb, void __iomem *base)
{
	struct dma_controller *controller;
	struct ark_glue *glue = dev_get_drvdata(musb->controller->parent);
	void __iomem *usbss_base =  musb->ctrl_base;

	controller = cppi41_dma_controller_create(musb, base);
	if (IS_ERR_OR_NULL(controller))
		return controller;

	musb_writel(usbss_base, USBSS_IRQ_ENABLER, USBSS_IRQ_PD_COMP);
	controller->dma_callback = ark_dma_controller_callback;

	return controller;
}
#endif /* CONFIG_USB_TI_CPPI41_DMA */


static struct musb_platform_ops musb_ark_ops = {
	.init		= ark_musb_init,
	.exit		= ark_musb_exit,
#if defined(CONFIG_USB_TI_CPPI41_DMA)
	.quirks		= MUSB_DMA_CPPI41 | MUSB_INDEXED_EP,
	.dma_init	= ark_dma_controller_create,
	.dma_exit	= cppi41_dma_controller_destroy,
#elif defined(CONFIG_USB_INVENTRA_DMA)
	.quirks		= MUSB_INDEXED_EP | MUSB_DMA_INVENTRA,
	.dma_init	= musbhs_dma_controller_create,
	.dma_exit	= musbhs_dma_controller_destroy,
#endif
	.enable		= ark_musb_enable,
	.disable	= ark_musb_disable,
	.set_mode	= ark_musb_set_mode,
	.set_vbus	= ark_musb_set_vbus,
	.fifo_mode	= 2,
};

static int get_int_prop(struct device_node *dn, const char *s)
{
	int ret;
	u32 val;

	ret = of_property_read_u32(dn, s, &val);
	if (ret)
		return 0;
	return val;
}

static int get_musb_port_mode(struct device *dev)
{
	enum usb_dr_mode mode;

	mode = usb_get_dr_mode(dev);
	switch (mode) {
		case USB_DR_MODE_HOST:
			return MUSB_HOST;

		case USB_DR_MODE_PERIPHERAL:
			return MUSB_PERIPHERAL;

		case USB_DR_MODE_UNKNOWN:
		case USB_DR_MODE_OTG:
		default:
			return MUSB_OTG;
	}
}

static const struct of_device_id musb_ark_of_match[] = {
	{ .compatible = "arkmicro,ark-musb"},
	{  },
};
MODULE_DEVICE_TABLE(of, musb_ark_of_match);

static int musb_ark_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct ark_glue *glue;
	struct musb_hdrc_platform_data pdata;
	struct resource	resources[3];
	struct resource	*res;
	struct musb_hdrc_config	*config;
	struct platform_device *plat_dev;
	struct device_node *dn = pdev->dev.of_node;
	int val;
	int ret = 0;

	match = of_match_node(musb_ark_of_match, dn);
	if (!match) {
		dev_err(&pdev->dev, "Failed to get matching of_match struct\n");
		return -EINVAL;
	}
	
	memset(resources, 0, sizeof(resources));

	/* allocate glue */
	glue = devm_kzalloc(&pdev->dev, sizeof(*glue), GFP_KERNEL);
	if (!glue) {
		dev_err(&pdev->dev, "Failed to devm_kzalloc flue\n");
		return -ENOMEM;
	}

	glue->dev = &pdev->dev;
	ret = of_property_read_u32(dn, "gpio-id", &glue->gpio_id);
	if(ret) {
		return -EINVAL;
	}
	ret = of_property_read_u32(dn, "usb-id-reg", &glue->usb_id_reg);
	if(ret == 0) {
		ret = of_property_read_u32(dn, "usb-id-offset", &glue->usb_id_offset);
		if(ret) {
			glue->usb_id_offset = -1;
		}
	} else {
		glue->usb_id_reg = -1;
	}
	ret = of_property_read_u32(dn, "gpio-pwr", &glue->gpio_pwr);
	if(ret) {
		return -EINVAL;
	}
	ret = of_property_read_u32(dn, "sys-softrest-regoffset", &glue->usb_softrest_reg_offset);
	if(ret) {
		return -EINVAL;
	}
	ret = of_property_read_u32(dn, "usb-softrest-bitoffset", &glue->usb_softrest_bit_offset);
	if(ret) {
		return -EINVAL;
	}
	ret = of_property_read_u32(dn, "usbphy-softrest-bitoffset", &glue->usbphy_softrest_bit_offset);
	if(ret) {
		glue->usbphy_softrest_bit_offset = glue->usb_softrest_bit_offset + 1;
	}

	//if (usb_get_dr_mode(&pdev->dev) == USB_DR_MODE_PERIPHERAL) {
	//}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get usb_base memory.\n");
		return -EINVAL;
	}
	resources[0] = *res;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get sys_base memory.\n");
		return -EINVAL;
	}
	glue->sys_base = ioremap(res->start, resource_size(res));
	if(IS_ERR(glue->sys_base)) {
		dev_err(&pdev->dev, "Failed to devm_ioremap_resource\n");
		return PTR_ERR(glue->sys_base);
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get irq.\n");
		ret = -EINVAL;
		goto err_iounmap;
	}
	resources[1] = *res;
	
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!res) {
		dev_err(&pdev->dev, "Failed to get dma irq.\n");
		ret = -EINVAL;
		goto err_iounmap;
	}
	resources[2] = *res;

	//glue->phy = usb_phy_generic_register();
	pdev->id = of_alias_get_id(dn, "usb");
	if(pdev->id < 0) {
		dev_err(&pdev->dev, "Failed to get usb id\n");
	}

	platform_set_drvdata(pdev, glue);

	/* allocate the child platform device */
	plat_dev = platform_device_alloc("musb-hdrc", pdev->id);
	if (!plat_dev) {
		dev_err(&pdev->dev, "Failed to allocate musb device\n");
		ret = -EINVAL;
		goto err_iounmap;
	}

	plat_dev->dev.parent		= &pdev->dev;
	plat_dev->dev.dma_mask		= &musb_dmamask;
	plat_dev->dev.coherent_dma_mask	= musb_dmamask;
	glue->plat_dev = plat_dev;

	ret = platform_device_add_resources(plat_dev, resources,
			ARRAY_SIZE(resources));
	if (ret) {
		dev_err(&pdev->dev, "Failed to add resources\n");
		goto err;
	}

	config = devm_kzalloc(&pdev->dev, sizeof(*config), GFP_KERNEL);
	if (!config) {
		ret = -ENOMEM;
		goto err;
	}
	pdata.config = config;
	pdata.platform_ops = &musb_ark_ops;

	config->num_eps = get_int_prop(dn, "num-eps");
	config->ram_bits = get_int_prop(dn, "ram-bits");
	//config->host_port_deassert_reset_at_resume = 1;
	pdata.mode = get_musb_port_mode(&pdev->dev);
	/* DT keeps this entry in mA, musb expects it as per USB spec */
	pdata.power = get_int_prop(dn, "power") / 2;

	ret = of_property_read_u32(dn, "multipoint", &val);
	if (!ret && val)
		config->multipoint = true;
#if 0
	ret = of_property_read_u32(dn, "dma", &val);
	if (!ret && val)
		config->dma = true;
	config->dma_channels = get_int_prop(dn, "dma_channels");
#endif
	config->maximum_speed = usb_get_maximum_speed(&pdev->dev);
	switch (config->maximum_speed) {
		case USB_SPEED_LOW:
		case USB_SPEED_FULL:
			break;
		case USB_SPEED_SUPER:
			dev_warn(&pdev->dev, "ignore incorrect maximum_speed "
					"(super-speed) setting in dts");
			/* fall through */
		default:
			config->maximum_speed = USB_SPEED_HIGH;
			break;
	}

	ret = platform_device_add_data(plat_dev, &pdata, sizeof(pdata));
	if (ret) {
		dev_err(&pdev->dev, "Failed to add platform_data\n");
		goto err;
	}

	ret = platform_device_add(plat_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register musb device\n");
		goto err;
	}
	
	printk("musb_ark_probe succss\n");

	return 0;

err:
	platform_device_put(plat_dev);
err_iounmap:
	//if(glue->phy)
	//	usb_phy_generic_unregister(glue->phy);
	if(glue->sys_base)
		iounmap(glue->sys_base);

	return ret;
}

static int musb_ark_remove(struct platform_device *pdev)
{
	struct ark_glue *glue = platform_get_drvdata(pdev);

	if(glue->plat_dev)
		platform_device_unregister(glue->plat_dev);
	//if(glue->phy)
	//	usb_phy_generic_unregister(glue->phy);

	if(glue->sys_base)
	iounmap(glue->sys_base);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int musb_ark_suspend(struct device *dev)
{
	return 0;
}

static int musb_ark_resume(struct device *dev)
{
	return 0;
}
#endif

//static SIMPLE_DEV_PM_OPS(musb_ark_pm_ops, musb_ark_suspend, musb_ark_resume);

static struct platform_driver musb_ark_driver = {
	.probe		= musb_ark_probe,
	.remove		= musb_ark_remove,
	.driver		= {
		.name	= "musb-ark",
		//.pm		= &musb_ark_pm_ops,
		.of_match_table	= musb_ark_of_match,
	},
};

MODULE_DESCRIPTION("ARKMICRO MUSB Glue Layer");
MODULE_AUTHOR("Arkmicro");
MODULE_LICENSE("GPL v2");

module_platform_driver(musb_ark_driver);
