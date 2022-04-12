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
#include <linux/delay.h>
#include "clk-ark.h"

static int clk_pll_is_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);

	return !!(reg & ARK_PLL_ENA);
}

static int clk_pll_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);
	reg |= ARK_PLL_ENA;
	writel(reg, clk->reg);

	return 0;
}

static void clk_pll_disable(struct clk_hw *hwclk)
{
	/* struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);
	reg &= ~ARK_PLL_ENA;
	writel(reg, clk->reg); */
	return;
}

static unsigned long clk_pll_recalc_rate(struct clk_hw *hwclk,
					 unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	unsigned long div, no, reg;

	reg = readl(clk->reg);
	no = (reg >> ARK_PLL_NO_OFFSET) & ARK_PLL_NO_MASK;
	div = (reg >> ARK_PLL_DIV_OFFSET) & ARK_PLL_DIV_MASK;

	pr_info("clk %s rate %lu.\n", clk_hw_get_name(hwclk), (parent_rate * div) / (1 << no));

	return (parent_rate * div) / (1 << no);
}

static const struct clk_ops clk_pll_ops = {
	.is_enabled = clk_pll_is_enable,
	.enable = clk_pll_enable,
	.disable = clk_pll_disable,
	.recalc_rate = clk_pll_recalc_rate,
};

static int clk_dds_is_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg2);

	return !!(reg & ARK_DDS_ENA);
}

static int clk_dds_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg2);
	reg |= ARK_DDS_ENA;
	writel(reg, clk->reg2);

	return 0;
}

static void clk_dds_disable(struct clk_hw *hwclk)
{
	/* struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg2);
	reg &= ~ARK_DDS_ENA;
	writel(reg, clk->reg2); */
	return;
}

static unsigned long clk_dds_recalc_rate(struct clk_hw *hwclk,
					 unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 dto_inc, mul, div, reg, reg2;
	u64 freqm;

	reg = readl(clk->reg);
	reg2 = readl(clk->reg2);

	div = (reg2 >> ARK_DDS_DIV_OFFSET) & ARK_DDS_DIV_MASK;
	div = 1 << div;
	mul = (reg2 >> ARK_DDS_MUL_OFFSET) & ARK_DDS_MUL_MASK;
	mul = 32 * (1 << mul);
	dto_inc = (reg >> ARK_DDS_DTOINC_OFFSET) & ARK_DDS_DTOINC_MASK;
	freqm = parent_rate * dto_inc * mul;

	return do_div(freqm, (1 << 22) * div);
}

static void clk_dds_calc(unsigned long rate, unsigned long ref_freq, u32 *dto_inc)
{
	u64 freqm;

	if (rate < ARK_DDS_MIN_FREQ)
		rate = ARK_DDS_MIN_FREQ;
	if (rate > ARK_DDS_MAX_FREQ)
		rate = ARK_DDS_MAX_FREQ;


	freqm = rate * (1 << 22);

	*dto_inc = do_div(freqm, ref_freq * 32);
}

static long clk_dds_round_rate(struct clk_hw *hwclk, unsigned long rate,
			       unsigned long *parent_rate)
{
	u32 dto_inc;
	u32 ref_freq = *parent_rate;
	u64 freqm;

	clk_dds_calc(rate, ref_freq, &dto_inc);

	freqm = ref_freq * dto_inc * 32;

	return do_div(freqm, 1 << 22);
}

static int clk_dds_set_rate(struct clk_hw *hwclk, unsigned long rate,
			    unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 dto_inc, reg, reg2;

	reg = readl(clk->reg);
	reg2 = readl(clk->reg2);

	reg2 &= ~((ARK_DDS_DIV_MASK << ARK_DDS_DIV_OFFSET) | (ARK_DDS_MUL_MASK << ARK_DDS_MUL_OFFSET));
	writel(reg2, clk->reg2);

	clk_dds_calc(rate, parent_rate, &dto_inc);

	reg &= ~(ARK_DDS_DTOINC_MASK << ARK_DDS_DTOINC_OFFSET);
	reg |= (dto_inc << ARK_DDS_DTOINC_OFFSET);
	writel(reg, clk->reg);

	return 0;
}

static const struct clk_ops clk_dds_ops = {
	.is_enabled = clk_dds_is_enable,
	.enable = clk_dds_enable,
	.disable = clk_dds_disable,
	.recalc_rate = clk_dds_recalc_rate,
	.round_rate = clk_dds_round_rate,
	.set_rate = clk_dds_set_rate,
};

static int clk_arke_sscg_is_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);

	return !!(reg & ARKE_SSCG_ENA);
}

static int clk_arke_sscg_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);
	reg |= ARKE_SSCG_ENA;
	writel(reg, clk->reg);

	return 0;
}

static void clk_arke_sscg_disable(struct clk_hw *hwclk)
{
	/* struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);
	reg &= ~ARKE_SSCG_ENA;
	writel(reg, clk->reg); */
	return;
}

/*
 * fref=24MHz, nr=3, fint=8MHz
 * fvco=1000~2000MHz
 * rs = 7 (fvcomax/fint=250)
 * nf[23:15] integer nf[14:0] fraction
*/
static int clk_arke_sscg_set_rate(struct clk_hw *hwclk, unsigned long rate,
				    unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	unsigned int nr = 3, fint = 8000, rs = 7;
	unsigned int od, fvco, nfx, nff, cpa, cpax;
	int i;
	u32 regval;
	u32 freq_khz = rate / 1000;

	regval = readl(clk->reg);
	nr = (regval >> ARKE_SSCG_NR_OFFSET) & ARKE_SSCG_NR_MASK;
	if (nr == 2) {
		fint = 12000;
		rs = 4;
	} else if (nr == 3) {
		fint = 8000;
		rs = 7;
	}

	if (!clk->can_change)
		return 0;

	freq_khz = freq_khz / fint * fint;
	for (i = 0; i < 8; i++) {
		fvco = freq_khz * (1 << i);
		if (fvco >= 1000 * 1000 && fvco <= 2000 * 1000) {
			od = i;
			break;
		}
	}
	if (i == 8)
		goto fail;

	nfx = fvco / fint;
	if (nfx >= 50 && nfx <= 100) {
		cpa = nfx * 4;
		cpax = 3;
	} else if (nfx > 100 && nfx <= 200) {
		cpa = nfx * 2;
		cpax = 1;
	} else if (nfx > 200 && nfx <= 400) {
		cpa = nfx;
		cpax = 0;
	}
	else
		goto fail;

	nff = ((1 << 15) * (fvco / 1000) / (fint / 1000)) & ((1 << 15) - 1);

	/* disable clk first */
	writel(readl(clk->reg2) | (1 << 27), clk->reg2);

	regval = readl(clk->reg);
	regval &= ~0x3FFFFF;
	regval |= (cpa << 0) | (cpax << 9) | (rs << 11) | (nr << 15);
	writel(regval, clk->reg);

	regval = readl(clk->reg2);
	regval &= ~0x7FFFFFF;
	regval |= ((nfx<<15) | nff) | (od << 24);
	writel(regval, clk->reg2)	;

	/* enable clk */
	writel(readl(clk->reg2) & ~(1 << 27), clk->reg2);

	mdelay(10);

	return 0;

fail:
	pr_err("Unsupported sscg freq.\n");

	return -1;
}

static unsigned long clk_arke_sscg_recalc_rate(struct clk_hw *hwclk,
					 unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 nr, fint, fvco, nfx, nff, od;
	u32 cfg0, cfg1;

	cfg0 = readl(clk->reg);
	nr = (cfg0 >> ARKE_SSCG_NR_OFFSET) & ARKE_SSCG_NR_MASK;
	nr = nr ? nr : 1;
	fint = parent_rate / nr;
	cfg1 = readl(clk->reg2);
	nff = (cfg1 >> ARKE_SSCG_NFF_OFFSET) & ARKE_SSCG_NFF_MASK;
	nfx = (cfg1 >> ARKE_SSCG_NFX_OFFSET) & ARKE_SSCG_NFX_MASK;
	fvco = fint * nfx + (u64)fint * nff / (ARKE_SSCG_NFF_MASK + 1);
	od = (cfg1 >> ARKE_SSCG_OD_OFFSET) & ARKE_SSCG_OD_MASK;

	pr_info("clk %s rate %u.\n", clk_hw_get_name(hwclk), fvco / (1 << od));
	return fvco / (1 << od);
}

static long clk_arke_sscg_round_rate(struct clk_hw *hwclk, unsigned long rate,
					unsigned long *parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 regval, nr, freq;

	if (clk->can_change) {
		regval = readl(clk->reg);
		nr = (regval >> ARKE_SSCG_NR_OFFSET) & ARKE_SSCG_NR_MASK;
		freq = *parent_rate / nr;
		freq = rate / freq * freq;
		return freq;
	}

	return rate;
}

static const struct clk_ops clk_arke_sscg_ops = {
	.is_enabled = clk_arke_sscg_is_enable,
	.enable = clk_arke_sscg_enable,
	.disable = clk_arke_sscg_disable,
	.recalc_rate = clk_arke_sscg_recalc_rate,
	.set_rate = clk_arke_sscg_set_rate,
	.round_rate = clk_arke_sscg_round_rate,
};

static int clk_arke_pll_is_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);

	return !!(reg & ARKE_PLL_ENA);
}

static int clk_arke_pll_enable(struct clk_hw *hwclk)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);
	reg |= ARKE_PLL_ENA;
	writel(reg, clk->reg);

	return 0;
}

static void clk_arke_pll_disable(struct clk_hw *hwclk)
{
	/* struct ark_clk *clk = to_ark_clk(hwclk);
	u32 reg;

	reg = readl(clk->reg);
	reg &= ~ARKE_PLL_ENA;
	writel(reg, clk->reg); */
	return;
}

static unsigned long clk_arke_pll_recalc_rate(struct clk_hw *hwclk,
					 unsigned long parent_rate)
{
	struct ark_clk *clk = to_ark_clk(hwclk);
	u32 ns, ms, ps;
	u32 cfg0;

	cfg0 = readl(clk->reg);
	ms = (cfg0 >> ARKE_PLL_MS_OFFSET) & ARKE_PLL_MS_MASK;
	ns = (cfg0 >> ARKE_PLL_NS_OFFSET) & ARKE_PLL_NS_MASK;
	ps = (cfg0 >> ARKE_PLL_PS_OFFSET) & ARKE_PLL_PS_MASK;

	ms = ms ? ms : 1;
	ns = ns ? ns : 1;
	ps = ps ? ps : 1;

	pr_info("clk %s rate %lu.\n", clk_hw_get_name(hwclk), parent_rate / ms * ns / ps / 2);
	return parent_rate / ms * ns / ps / 2;
}

static const struct clk_ops clk_arke_pll_ops = {
	.is_enabled = clk_arke_pll_is_enable,
	.enable = clk_arke_pll_enable,
	.disable = clk_arke_pll_disable,
	.recalc_rate = clk_arke_pll_recalc_rate,
};

static __init struct clk *ark_clk_init(struct device_node *node, enum ARK_CLK_TYPE clk_type, const struct clk_ops *ops)
{
	u32 reg, reg2;
	struct ark_clk *ark_clk;
	const char *clk_name = node->name;
	const char *parent_name;
	struct clk_init_data init;
	struct device_node *srnp;
	int rc;

	rc = of_property_read_u32(node, "reg", &reg);
	if (WARN_ON(rc))
		return NULL;

	ark_clk = kzalloc(sizeof(*ark_clk), GFP_KERNEL);
	if (WARN_ON(!ark_clk))
		return NULL;

	/* Map system registers */
	srnp = of_find_compatible_node(NULL, NULL, "arkmicro,ark-sregs");
	ark_clk->reg = of_iomap(srnp, 0);
	BUG_ON(!ark_clk->reg);

	of_property_read_string(node, "clock-output-names", &clk_name);

	init.name = clk_name;
	init.ops = ops;
	init.flags = 0;
	if (clk_type == ARK_CLK_TYPE_PLL) {
		u32 offset, mask;
		int index;

		rc = of_property_read_u32(node, "reg2", &reg2);
		if (WARN_ON(rc))
			return NULL;
		rc = of_property_read_u32(node, "offset", &offset);
		if (WARN_ON(rc))
			return NULL;
		rc = of_property_read_u32(node, "mask", &mask);
		if (WARN_ON(rc))
			return NULL;
		index = (readl(ark_clk->reg + reg2) >> offset) & mask;
		parent_name = of_clk_get_parent_name(node, index);
	} else if (clk_type == ARK_CLK_TYPE_DDS || clk_type == ARKE_CLK_TYPE_SSCG) {
		rc = of_property_read_u32(node, "reg2", &reg2);
		if (WARN_ON(rc))
			return NULL;
		ark_clk->reg2 = ark_clk->reg + reg2;
		parent_name = of_clk_get_parent_name(node, 0);
	} else {
		parent_name = of_clk_get_parent_name(node, 0);
	}
	init.parent_names = &parent_name;
	init.num_parents = 1;

	ark_clk->can_change = of_property_read_bool(node, "clk-can-change");

	ark_clk->reg += reg;
	ark_clk->hw.init = &init;
	rc = clk_hw_register(NULL, &ark_clk->hw);
	if (WARN_ON(rc)) {
		kfree(ark_clk);
		return NULL;
	}
	rc = of_clk_add_hw_provider(node, of_clk_hw_simple_get, &ark_clk->hw);
	return ark_clk->hw.clk;
}

static void __init of_ark_clk_dds_setup(struct device_node *np)
{
	ark_clk_init(np, ARK_CLK_TYPE_DDS, &clk_dds_ops);
}
CLK_OF_DECLARE(ark_clk_dds, "arkmiro,ark-clk-dds",
	       of_ark_clk_dds_setup);

static void __init of_ark_clk_pll_setup(struct device_node *np)
{
	ark_clk_init(np, ARK_CLK_TYPE_PLL, &clk_pll_ops);
}
CLK_OF_DECLARE(ark_clk_pll, "arkmiro,ark-clk-pll",
	       of_ark_clk_pll_setup);

static void __init of_arke_clk_sscg_setup(struct device_node *np)
{
	ark_clk_init(np, ARKE_CLK_TYPE_SSCG, &clk_arke_sscg_ops);
}
CLK_OF_DECLARE(arke_clk_sscg, "arkmiro,arke-clk-sscg",
	       of_arke_clk_sscg_setup);

static void __init of_arke_clk_pll_setup(struct device_node *np)
{
	ark_clk_init(np, ARKE_CLK_TYPE_PLL, &clk_arke_pll_ops);
}
CLK_OF_DECLARE(arke_clk_pll, "arkmiro,arke-clk-pll",
	       of_arke_clk_pll_setup);
