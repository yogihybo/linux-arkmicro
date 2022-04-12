/*
 * arkmicro interrupt controller driver
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/cpu_pm.h>
#include <linux/cpumask.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/irqchip.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <asm/exception.h>

#include "irq-gic-common.h"

#define ARK_MAX_IRQS		32

#define ICSET			0x00
#define ICPEND			0x04
#define ICMODE			0x08
#define ICMASK			0x0c
#define ICLEVEL			0x10
#define IRQISPR			0x3c
#define IRQISPC			0x40
#define IVEC_ADDR      	0x78

struct ark_irq_data {
	void __iomem *base;
	struct irq_domain *domain;
	unsigned int nr_irqs;
};

static DEFINE_RAW_SPINLOCK(irq_controller_lock);

/*
 * The GIC mapping of CPU interfaces does not necessarily match
 * the logical CPU numbering.  Let's use a mapping as returned
 * by the GIC itself.
 */
static struct ark_irq_data ark_data __read_mostly;

static inline void __iomem *ark_base(struct irq_data *d)
{
	struct ark_irq_data *ark_data = irq_data_get_irq_chip_data(d);
	return ark_data->base;
}

static inline unsigned int ark_irq(struct irq_data *d)
{
	return d->hwirq;
}

static asmlinkage void __exception_irq_entry
ark_intc_handle(struct pt_regs *regs)
{
	u32 pnd;
	u32 offset;

	pnd = readl_relaxed(ark_data.base + ICPEND);
	if (!pnd)
		return;

	offset = readl_relaxed(ark_data.base + IVEC_ADDR);
    offset >>= 2;

	handle_domain_irq(ark_data.domain, offset, regs);
}

/*
 * Routines to acknowledge, disable and enable interrupts
 */
static void ark_mask_irq(struct irq_data *d)
{
	u32 mask = 1 << ark_irq(d);

	raw_spin_lock(&irq_controller_lock);
	writel_relaxed(readl_relaxed(ark_base(d) + ICMASK) | mask, ark_base(d) + ICMASK);
	raw_spin_unlock(&irq_controller_lock);
}

static void ark_unmask_irq(struct irq_data *d)
{
	u32 mask = 1 << ark_irq(d);

	raw_spin_lock(&irq_controller_lock);
	writel_relaxed(readl_relaxed(ark_base(d) + ICMASK) & ~mask, ark_base(d) + ICMASK);
	raw_spin_unlock(&irq_controller_lock);
}

static void ark_ack_irq(struct irq_data *d)
{
	u32 mask = 1 << ark_irq(d);

	raw_spin_lock(&irq_controller_lock);
	writel_relaxed(mask, ark_base(d) + IRQISPC);
	raw_spin_unlock(&irq_controller_lock);
}

static struct irq_chip ark_irq_chip = {
	.name			= "ark-intc",
	.irq_mask		= ark_mask_irq,
	.irq_unmask		= ark_unmask_irq,
	.irq_ack		= ark_ack_irq,
};

static void __init ark_irq_hw_init(struct ark_irq_data *intc)
{
    writel_relaxed(0x08, intc->base + ICSET);
	udelay(10);
    writel_relaxed(0x05, intc->base + ICSET);   /* enabel irq disable fiq */
	writel_relaxed(0x00, intc->base + ICMODE);  /* 0--> irq type, 1 --> fiq type */
    writel_relaxed(0xffffffff, intc->base + ICMASK);
    writel_relaxed(0xffffffff, intc->base + ICLEVEL);
    writel_relaxed(0xffffffff, intc->base + IRQISPC);
}

static int ark_irq_domain_map(struct irq_domain *d, unsigned int irq,
				irq_hw_number_t hw)
{
	irq_set_chip_and_handler(irq, &ark_irq_chip,
						 handle_level_irq);
	irq_set_chip_data(irq, d->host_data);
	return 0;
}

static int ark_irq_domain_xlate(struct irq_domain *d,
				  struct device_node *controller,
				  const u32 *intspec, unsigned int intsize,
				  unsigned long *out_hwirq,
				  unsigned int *out_type)
{
	unsigned long ret = 0;

	if (irq_domain_get_of_node(d) != controller)
		return -EINVAL;
	if (intsize < 1)
		return -EINVAL;

	*out_hwirq = intspec[0];
	*out_type = IRQ_TYPE_LEVEL_HIGH;

	return ret;
}

static const struct irq_domain_ops ark_irq_domain_ops = {
	.map	= ark_irq_domain_map,
	.xlate	= ark_irq_domain_xlate,
};

static int __init
ark_irq_of_init(struct device_node *node, struct device_node *parent)
{
	irq_hw_number_t hwirq_base = 0;
	int nr_irqs, irq_base;

	if (WARN_ON(!node))
		return -ENODEV;

	ark_data.base = of_iomap(node, 0);
	WARN(!ark_data.base, "fail to map ark intc dist registers\n");

	ark_data.nr_irqs = nr_irqs = ARK_MAX_IRQS;
	irq_base = irq_alloc_descs(-1, hwirq_base, nr_irqs, numa_node_id());
	if (irq_base < 0) {
		pr_err("failed to allocate IRQ numbers\n");
		return -EINVAL;
	}

	ark_data.domain = irq_domain_add_legacy(node, nr_irqs, irq_base,
						  hwirq_base,
						  &ark_irq_domain_ops,
						  &ark_data);

	if (WARN_ON(!ark_data.domain))
		return -EINVAL;

	ark_irq_hw_init(&ark_data);
    set_handle_irq(ark_intc_handle);

	return 0;
}
IRQCHIP_DECLARE(ark_intc, "arkmicro,ark-intc", ark_irq_of_init);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro irq controller driver");
MODULE_LICENSE("GPL v2");