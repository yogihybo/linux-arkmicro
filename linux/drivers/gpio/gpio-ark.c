/*
 * arkmicro gpio driver
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/gpio/driver.h>
#include <linux/gpio.h>

#define GPIO_MOD			0x00
#define	GPIO_RDATA			0x04
#define GPIO_INTEN			0x08
#define GPIO_INTLEVEL		0x0C
#define GPIO_INTGEN			0x10
#define GPIO_DEBOUNCE_EN	0xA0
#define GPIO_DEBOUNCE_CNT0	0x80

#define GPIO_INT_LOW_LEV	0
#define GPIO_INT_HIGH_LEV	1

struct ark_gpio_port {
	struct list_head node;
	void __iomem *base;
	struct clk	*clk;
	int irq;
	struct irq_domain *domain;
	struct gpio_chip gc;
	struct device *dev;
	u32 both_edges;
};

static const struct of_device_id ark_gpio_of_match[] = {
	{ .compatible = "arkmicro,ark-gpio", },
	{ /* sentinel */ }
};

/*
 * ark gpio has one interrupt *for all* gpio ports. The list is used
 * to save the references to all ports, so that ark_gpio_irq_handler
 * can walk through all interrupt status registers.
 */
static LIST_HEAD(ark_gpio_ports);

/* Note: This driver assumes 32 GPIOs are handled in one register */

static int ark_gpio_irq_set_type(struct irq_data *d, u32 type)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct ark_gpio_port *port = gc->private;
	u32 val;
	u32 gpio_idx = d->hwirq;
	u32 gpio = port->gc.base + gpio_idx;
	int edge;

	port->both_edges &= ~(1 << gpio_idx);
	switch (type) {
	case IRQ_TYPE_EDGE_RISING:
		edge = GPIO_INT_HIGH_LEV;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		edge = GPIO_INT_LOW_LEV;
		break;
	case IRQ_TYPE_EDGE_BOTH:
		val = gpio_get_value(gpio);
		if (val) {
			edge = GPIO_INT_LOW_LEV;
			pr_debug("ark: set GPIO %d to low trigger\n", gpio);
		} else {
			edge = GPIO_INT_HIGH_LEV;
			pr_debug("ark: set GPIO %d to high trigger\n", gpio);
		}
		port->both_edges |= 1 << gpio_idx;
		break;
	default:
		return -EINVAL;
	}

	val = readl(port->base + GPIO_INTLEVEL);
	val &= ~(1 << gpio_idx);
	val |= edge << gpio_idx;
	writel(val, port->base + GPIO_INTLEVEL);

	return 0;
}

static void ark_flip_edge(struct ark_gpio_port *port, u32 gpio)
{
	u32 val;
	int edge;

	val = readl(port->base + GPIO_INTLEVEL);
	edge = !(val & (1 << gpio));
	val &= ~(1 << gpio);
	writel(val | (edge << gpio), port->base + GPIO_INTLEVEL);
}

/* ark gpio has one interrupt *for all* gpio ports */
static void ark_gpio_irq_handler(struct irq_desc *desc)
{
	u32 irq_msk, irq_stat;
	struct ark_gpio_port *port;
	struct irq_chip *chip = irq_desc_get_chip(desc);

	chained_irq_enter(chip, desc);

	/* walk through all interrupt status registers */
	list_for_each_entry(port, &ark_gpio_ports, node) {
		irq_msk = readl(port->base + GPIO_INTEN);
		if (!irq_msk)
			continue;

		irq_stat = readl(port->base + GPIO_INTGEN) & irq_msk;
		if (irq_stat) {
			/* handle 32 interrupts in one status register */
			while (irq_stat != 0) {
				int irqoffset = fls(irq_stat) - 1;

				if (port->both_edges & (1 << irqoffset))
					ark_flip_edge(port, irqoffset);

				generic_handle_irq(irq_find_mapping(port->domain, irqoffset));

				irq_stat &= ~(1 << irqoffset);

				writel(readl(port->base + GPIO_INTGEN) & ~(1 << irqoffset), port->base + GPIO_INTGEN);
			}
		}
	}
	chained_irq_exit(chip, desc);
}

/*
 * Set interrupt number "irq" in the GPIO as a wake-up source.
 * While system is running, all registered GPIO interrupts need to have
 * wake-up enabled. When system is suspended, only selected GPIO interrupts
 * need to have wake-up enabled.
 * @param  irq          interrupt source number
 * @param  enable       enable as wake-up if equal to non-zero
 * @return       This function returns 0 on success.
 */
static int ark_gpio_irq_set_wake(struct irq_data *d, u32 enable)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct ark_gpio_port *port = gc->private;
	int ret;

	if (enable) {
		ret = enable_irq_wake(port->irq);
	} else {
		ret = disable_irq_wake(port->irq);
	}

	return ret;
}

static int ark_gpio_init_gc(struct ark_gpio_port *port, int irq_base)
{
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	int rv;

	gc = devm_irq_alloc_generic_chip(port->dev, "gpio-ark", 1, irq_base,
					 port->base, handle_level_irq);
	if (!gc)
		return -ENOMEM;
	gc->private = port;

	ct = gc->chip_types;
	//ct->chip.irq_ack = irq_gc_ack_clr_bit;
	ct->chip.irq_mask = irq_gc_mask_clr_bit;
	ct->chip.irq_unmask = irq_gc_mask_set_bit;
	ct->chip.irq_set_type = ark_gpio_irq_set_type;
	ct->chip.irq_set_wake = ark_gpio_irq_set_wake;
	ct->chip.flags = IRQCHIP_MASK_ON_SUSPEND;
	//ct->regs.ack = GPIO_INTGEN;
	ct->regs.mask = GPIO_INTEN;

	rv = devm_irq_setup_generic_chip(port->dev, gc, IRQ_MSK(32),
					 IRQ_GC_INIT_NESTED_LOCK,
					 IRQ_NOREQUEST, 0);

	return rv;
}


static int ark_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct ark_gpio_port *port = gpiochip_get_data(gc);

	return irq_find_mapping(port->domain, offset);
}

static int ark_gpio_set_debounce(struct ark_gpio_port *port, unsigned offset,
				    unsigned debounce_ms)
{
	int debounce_count;
	int gpio;
	u32 val;

	if (offset < port->gc.ngpio) {
		gpio = offset + port->gc.base;
		if (gpio < 0 || gpio > 7)
			return -EINVAL;
	} else {
		return -EINVAL;
	}

	port->clk = devm_clk_get(port->dev, NULL);
	if (IS_ERR(port->clk))
		return PTR_ERR(port->clk);

	debounce_count = clk_get_rate(port->clk) / 1000 * debounce_ms;

	writel(debounce_count, port->base + GPIO_DEBOUNCE_CNT0 + gpio * 4);
	val = readl(port->base + GPIO_DEBOUNCE_EN);
	val |= 1 << offset;
	writel(val, port->base + GPIO_DEBOUNCE_EN);

	return 0;
}

static int ark_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
				    unsigned long config)
{
	struct ark_gpio_port *port = gpiochip_get_data(gc);

	switch (pinconf_to_config_param(config)) {
	case PIN_CONFIG_INPUT_DEBOUNCE:
		return ark_gpio_set_debounce(port, offset,
			pinconf_to_config_argument(config));
	default:
		break;
	}

	return -ENOTSUPP;
}

static int ark_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct ark_gpio_port *port = gpiochip_get_data(gc);
	u32 val = readl(port->base + GPIO_MOD);
	val |= 1 << offset;
	writel(val, port->base + GPIO_MOD);

	return 0;
}

static int ark_gpio_direction_output(struct gpio_chip *gc,
				     unsigned offset, int value)
{
	struct ark_gpio_port *port = gpiochip_get_data(gc);
	u32 val = readl(port->base + GPIO_MOD);
	val &= ~(1 << offset);
	writel(val, port->base + GPIO_MOD);
	gc->set(gc, offset, value);

	return 0;
}

static int ark_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct ark_gpio_port *port = gpiochip_get_data(gc);
	u32 val = readl(port->base + GPIO_RDATA);

	return !!(val & (1 << offset));
}

static void ark_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct ark_gpio_port *port = gpiochip_get_data(gc);
	u32 val = readl(port->base + GPIO_RDATA);
	if (value) val |= (1 << offset);
	else val &= ~(1 << offset);
	writel(val, port->base + GPIO_RDATA);
}

static int ark_gpio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct ark_gpio_port *port;
	struct resource *res;
	int irq_base;
	int ret;

	port = devm_kzalloc(&pdev->dev, sizeof(*port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	port->dev = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	port->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(port->base)) {
		return PTR_ERR(port->base);
	}

	port->irq = platform_get_irq(pdev, 0);
	if (port->irq < 0)
		return port->irq;

	/* disable the interrupt and clear the status */
	writel(0, port->base + GPIO_INTEN);
	writel(0, port->base + GPIO_INTGEN);

	/*
	* Setup one handler for all GPIO interrupts. Actually setting
	* the handler is needed only once, but doing it for every port
	* is more robust and easier.
	*/
	irq_set_chained_handler(port->irq, ark_gpio_irq_handler);

	if (of_property_read_bool(np, "gpio-ranges")) {
		port->gc.request = gpiochip_generic_request;
		port->gc.free = gpiochip_generic_free;
	}

	port->gc.parent = port->dev;
	port->gc.label = dev_name(port->dev);
	port->gc.direction_input  = ark_gpio_direction_input;
	port->gc.direction_output = ark_gpio_direction_output;
	port->gc.get = ark_gpio_get;
	port->gc.set = ark_gpio_set;
	port->gc.to_irq = ark_gpio_to_irq;
	port->gc.set_config = ark_gpio_set_config;
	port->gc.base = (pdev->id < 0) ? of_alias_get_id(np, "gpio") * 32 :
					     pdev->id * 32;
	port->gc.ngpio = 32;

	ret = devm_gpiochip_add_data(&pdev->dev, &port->gc, port);
	if (ret)
		goto out_bgio;

	irq_base = devm_irq_alloc_descs(&pdev->dev, -1, 0, 32, numa_node_id());
	if (irq_base < 0) {
		ret = irq_base;
		goto out_bgio;
	}

	port->domain = irq_domain_add_legacy(np, 32, irq_base, 0,
					     &irq_domain_simple_ops, NULL);
	if (!port->domain) {
		ret = -ENODEV;
		goto out_bgio;
	}

	/* gpio-ark can be a generic irq chip */
	ret = ark_gpio_init_gc(port, irq_base);
	if (ret < 0)
		goto out_irqdomain_remove;

	list_add_tail(&port->node, &ark_gpio_ports);

	return 0;

out_irqdomain_remove:
	irq_domain_remove(port->domain);
out_bgio:
	dev_info(&pdev->dev, "%s failed with errno %d\n", __func__, ret);
	return ret;

	return 0;
}

static struct platform_driver ark_gpio_driver = {
	.driver = {
		.name = "gpio-ark",
		.of_match_table = of_match_ptr(ark_gpio_of_match),
	},
	.probe = ark_gpio_probe,
};

static int __init ark_gpio_init(void)
{
	return platform_driver_register(&ark_gpio_driver);
}
postcore_initcall(ark_gpio_init);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro Watchdog Timer driver");
MODULE_LICENSE("GPL v2");
