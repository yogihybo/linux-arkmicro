/*
 * Copyright 2018-2019 Arkmicro, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/log2.h>
#include <dt-bindings/clock/ark-clk.h>
#include "clk-ark.h"

static inline int get_div(int div, int divmode)
{
	switch(divmode) {
	case ARK_CLK_DIVMODE_NOZERO:
		div = div ? div : 1;
		break;
	case ARK_CLK_DIVMODE_PLUSONE:
		div = div + 1;
		break;
	case ARK_CLK_DIVMODE_DOUBLE:
		div *= 2;
		break;
	case ARK_CLK_DIVMODE_EXPONENT:
		div = 1 << div;
		break;
	case ARK_CLK_DIVMODE_PONEDOUBLE:
		div = (div + 1) * 2;
		break;
	}
	return div;
}

static inline int set_div(int div, int divmode)
{
	switch(divmode) {
	case ARK_CLK_DIVMODE_PLUSONE:
		div = div - 1;
		break;
	case ARK_CLK_DIVMODE_DOUBLE:
		div /= 2;
		break;
	case ARK_CLK_DIVMODE_EXPONENT:
		div = ilog2(div);
		break;
	case ARK_CLK_DIVMODE_PONEDOUBLE:
		div = div / 2 - 1;
		break;
	}
	return div;
}

static unsigned long clk_sys_recalc_rate(struct clk_hw *hwclk,
					 unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);

	pr_info("clk %s rate %lu.\n", clk_hw_get_name(hwclk), parent_rate / clk->div);

	return parent_rate / clk->div;
}

static int clk_sys_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	int i;

	for (i = 0; i < clk->enable_num; i++) {
		writel(readl(clk->enable_reg[i]) | (1 << clk->enable_offset[i]),
			clk->enable_reg[i]);
	}

	return 0;
}

static void clk_sys_disable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	int i;

	for (i = 0; i < clk->enable_num; i++) {
		writel(readl(clk->enable_reg[i]) & ~(1 << clk->enable_offset[i]),
			clk->enable_reg[i]);
	}
}

static int clk_sys_is_enabled(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	int i, enable = 0;

	for (i = 0; i < clk->enable_num; i++) {
		enable |= !!(readl(clk->enable_reg[i]) & (1 << clk->enable_offset[i]));
	}

	return enable;
}

static int clk_sys_set_rate(struct clk_hw *hwclk, unsigned long rate,
				    unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	int div;
	u32 reg;

	if (clk->can_change) {
		div = DIV_ROUND_UP(parent_rate, rate);
		clk->div = div;
		div = set_div(div, clk->divmode);
		reg = readl(clk->reg);
		reg &= ~(clk->divmask << clk->divoffset);
		reg |= ((div & clk->divmask) << clk->divoffset);
		writel(reg, clk->reg);
	}

	return 0;
}

static long clk_sys_round_rate(struct clk_hw *hwclk, unsigned long rate,
					unsigned long *parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	int div;

	if (clk->can_change) {
		div = DIV_ROUND_UP(*parent_rate, rate);
		div = set_div(div, clk->divmode);
		div = get_div(div, clk->divmode);
		return DIV_ROUND_UP(*parent_rate, div);
	}

	return rate;
}

static const struct clk_ops clk_sys_ops = {
	.recalc_rate = clk_sys_recalc_rate,
	.enable = clk_sys_enable,
	.disable = clk_sys_disable,
	.is_enabled = clk_sys_is_enabled,
	.set_rate = clk_sys_set_rate,
	.round_rate = clk_sys_round_rate,
};

static __init struct clk *ark_sys_clk_init(struct device_node *node,
											const struct clk_ops *ops)
{
	u32 reg, mask, offset, divmode = 0;
	void __iomem	 *reg_base;
	struct ark_clk *ark_clk;
	const char *clk_name = node->name;
	const char *parent_name;
	struct clk_init_data init;
	struct device_node *srnp;
	int rc, index = 0, div = 1;
	const __be32 *enable_list, *enable_offset_list;
	int enable_size, enable_offset_size;
	u32 val, inv_reg;
	int i;

	rc = of_property_read_u32(node, "reg", &reg);
	if (WARN_ON(rc))
		return NULL;

	ark_clk = kzalloc(sizeof(*ark_clk), GFP_KERNEL);
	if (WARN_ON(!ark_clk))
		return NULL;

	/* Map system registers */
	srnp = of_find_compatible_node(NULL, NULL, "arkmicro,ark-sregs");
	reg_base = of_iomap(srnp, 0);
	BUG_ON(!reg_base);
	ark_clk->reg = reg_base + reg;
	reg = readl(ark_clk->reg);

	of_property_read_string(node, "clock-output-names", &clk_name);

	init.name = clk_name;
	init.ops = ops;
	init.flags = 0;

	if (!of_property_read_u32(node, "index-mask", &mask) && !of_property_read_u32(node, "index-offset", &offset)) {
		if (!of_property_read_u32(node, "index-value", &val)) {
			index = val;
			if (val) val = 1 << (val - 1);
			reg &= ~(mask << offset);
			reg |= ((val & mask) << offset);
			writel(reg, ark_clk->reg);
		} else {
			index = (reg >> offset) & mask;
			if (index) index = ilog2(index) + 1;
		}
	}

	of_property_read_u32(node, "div-mode", &divmode);
	if (!of_property_read_u32(node, "div-mask", &mask) && !of_property_read_u32(node, "div-offset", &offset)) {
		ark_clk->divmode = divmode;
		ark_clk->divmask = mask;
		ark_clk->divoffset = offset;
		if (!of_property_read_u32(node, "div-value", &val)) {
			val = set_div(val, divmode);
			div = get_div(val, divmode);
			reg &= ~(mask << offset);
			reg |= ((val & mask) << offset);
			writel(reg, ark_clk->reg);
		} else {
			div = (reg >> offset) & mask;
			div = get_div(div, divmode);
		}
	}
	ark_clk->can_change = of_property_read_bool(node, "clk-can-change");

	enable_list = of_get_property(node, "enable-reg", &enable_size);
	enable_offset_list = of_get_property(node, "enable-offset", &enable_offset_size);
	if (enable_list && enable_offset_list && enable_size == enable_offset_size) {
		ark_clk->enable_num = min(ARK_CLK_MAX_ENABLE_NUM,
				(enable_size / sizeof(*enable_list)));
		for (i = 0; i < ark_clk->enable_num; i++) {
			ark_clk->enable_reg[i] = reg_base + be32_to_cpu(*enable_list++);
			ark_clk->enable_offset[i] = be32_to_cpu(*enable_offset_list++);
		}
	}

	if (!of_property_read_u32(node, "inv-reg", &inv_reg) &&
		!of_property_read_u32(node, "inv-offset", &offset) &&
		!of_property_read_u32(node, "inv-mask", &mask) &&
		!of_property_read_u32(node, "inv-value", &val)) {
		reg = readl(reg_base + inv_reg);
		reg &= ~(mask << offset);
		reg |= ((val & mask) << offset);
		writel(reg, reg_base + inv_reg);
	}

	parent_name = of_clk_get_parent_name(node, index);
	init.parent_names = &parent_name;
	init.num_parents = 1;
	ark_clk->div = div;
	ark_clk->hw.init = &init;
	rc = clk_hw_register(NULL, &ark_clk->hw);
	if (WARN_ON(rc)) {
		kfree(ark_clk);
		return NULL;
	}
	rc = of_clk_add_hw_provider(node, of_clk_hw_simple_get, &ark_clk->hw);
	return ark_clk->hw.clk;
}

static void __init of_ark_clk_sys_setup(struct device_node *np)
{
	ark_sys_clk_init(np, &clk_sys_ops);
}
CLK_OF_DECLARE(ark_clk_sys, "arkmiro,ark-clk-sys", of_ark_clk_sys_setup);
