/*
 * Arkmidro Reset Controller Driver
 * Author: Sim
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
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/property.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>

struct ark_reset {
	struct reset_controller_dev rcdev;
	void __iomem *base;
	spinlock_t lock;
};

static int ark_reset_update(struct reset_controller_dev *rcdev,
			unsigned long id, bool assert)
{
	struct ark_reset *ark_reset =
		container_of(rcdev, struct ark_reset, rcdev);
	unsigned long flags;
	u32 reg;
	u32 offset;
	u32 val;

	reg = (id >> 8) & 0xfff;
	offset = id & 0xff;

	if (offset > 31) {
		printk(KERN_ALERT "ark ret wrong bit offset %u.\n", offset);
		return -EINVAL;
	}

	spin_lock_irqsave(&ark_reset->lock, flags);
	val = readl(ark_reset->base + reg);
	if (assert)
		val &= ~BIT(offset);
	else
		val |= BIT(offset);
	writel(val, ark_reset->base + reg);
	spin_unlock_irqrestore(&ark_reset->lock, flags);

	return 0;
}

static int ark_reset_assert(struct reset_controller_dev *rcdev,
			unsigned long id)
{
	return ark_reset_update(rcdev, id, true);
}

static int ark_reset_deassert(struct reset_controller_dev *rcdev,
				unsigned long id)
{
	return ark_reset_update(rcdev, id, false);
}

static int ark_reset_status(struct reset_controller_dev *rcdev,
			unsigned long id)
{
	struct ark_reset *ark_reset =
		container_of(rcdev, struct ark_reset, rcdev);
	u32 val;

	val = readl(ark_reset->base);

	return !!(val & BIT(id));
}

static const struct reset_control_ops ark_reset_ops = {
	.assert = ark_reset_assert,
	.deassert = ark_reset_deassert,
	.status = ark_reset_status,
};

static int ark_reset_xlate(struct reset_controller_dev *rcdev,
				  const struct of_phandle_args *reset_spec)
{
	unsigned int reg, offset;

	reg = reset_spec->args[0];
	offset = reset_spec->args[1];

	return (reg << 8) | offset;
}

static int ark_reset_probe(struct platform_device *pdev)
{
	struct ark_reset *ark_reset;
	struct resource *res;
	int err;

	ark_reset = devm_kzalloc(&pdev->dev,
				sizeof(*ark_reset), GFP_KERNEL);
	if (!ark_reset)
		return -ENOMEM;

	platform_set_drvdata(pdev, ark_reset);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ark_reset->base = ioremap(res->start, resource_size(res)); /* baseaddr conflict */
	if (IS_ERR(ark_reset->base)) {
		return PTR_ERR(ark_reset->base);
	}

	spin_lock_init(&ark_reset->lock);
	ark_reset->rcdev.ops = &ark_reset_ops;
	ark_reset->rcdev.owner = THIS_MODULE;
	ark_reset->rcdev.of_node = pdev->dev.of_node;
	ark_reset->rcdev.of_reset_n_cells = 2;
	ark_reset->rcdev.of_xlate = ark_reset_xlate;

	err = devm_reset_controller_register(&pdev->dev, &ark_reset->rcdev);
	if (err)
		return err;

	return 0;
}

static const struct of_device_id ark_reset_dt_ids[] = {
	{
		.compatible = "arkmicro,ark-reset",
	} , {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ark_reset_dt_ids);

static struct platform_driver ark_reset_driver = {
	.probe	= ark_reset_probe,
	.driver = {
		.name = "ark-reset",
		.of_match_table = of_match_ptr(ark_reset_dt_ids),
	},
};

static int __init ark_reset_init(void)
{
	int ret;

	ret = platform_driver_register(&ark_reset_driver);
	if (ret)
		return ret;

	return 0;
}
postcore_initcall(ark_reset_init);
