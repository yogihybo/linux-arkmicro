/*
 * arkmicro pinctrl driver
 *
 * Licensed under GPLv2 or later.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>

#include "core.h"

#define MAX_PIN_PER_BANK	32
#define PAD_NUMS_PER_REG	10
#define PAD_BITS_MASK		0x7
#define BITS_PER_PAD		3

#define PIN_REG_OFFSET(x)	(((x) / PAD_NUMS_PER_REG) * 4)
#define PIN_BIT_OFFSET(x)	(((x) % PAD_NUMS_PER_REG) * BITS_PER_PAD)

#define ARKE_PAD_CTL2A		0xa8

struct ark_pad_ctrl {
	int reg;
	int offset;
	int mask;
};

static struct ark_pad_ctrl ark1668_pin_map[] = {
	{0x1e4, 0, 0x3},
	{0x1e4, 2, 0x3},
	{0x1c0, 0, 0xf},
	{0x1c0, 4, 0xf},
	{0x1c0, 8, 0xf},
	{0x1c0, 12, 0xf},
	{0x1c0, 16, 0xf},
	{0x1c0, 20, 0xf},
	{0x1c0, 24, 0xf},
	{0x1c0, 28, 0xf},
	{0x1c4, 0, 0xf},
	{0x1c4, 4, 0xf},
	{0x1c4, 8, 0xf},
	{0x1c4, 12, 0xf},
	{0x1c4, 16, 0xf},
	{0x1c4, 20, 0xf},
	{0x1c4, 24, 0xf},
	{0x1c4, 28, 0xf},
	{0x1c8, 0, 0xf},
	{0x1c8, 4, 0xf},
	{0x1c8, 8, 0xf},
	{0x1c8, 12, 0xf},
	{0x1c8, 16, 0xf},
	{0x1c8, 20, 0xf},
	{0x1c8, 24, 0xf},
	{0x1c8, 28, 0xf},
	{0x1cc, 0, 0xf},
	{0x1cc, 4, 0xf},
	{0x1cc, 8, 0xf},
	{0x1cc, 12, 0xf},
	{0x1dc, 0, 0x3},
	{0x1dc, 2, 0x3},
	{0x1dc, 4, 0x3},
	{0x1dc, 6, 0x3},
	{0x1dc, 8, 0x3},
	{0x1dc, 10, 0x3},
	{0x1dc, 12, 0x3},
	{0x1dc, 14, 0x3},
	{0x1dc, 16, 0x3},
	{0x1d0, 0, 0xf},
	{0x1d0, 4, 0xf},
	{0x1d0, 8, 0xf},
	{0x1d0, 12, 0xf},
	{0x1d0, 16, 0xf},
	{0x1d0, 20, 0xf},
	{0x1d0, 24, 0xf},
	{0x1d0, 28, 0xf},
	{0x1d4, 0, 0xf},
	{0x1d4, 4, 0xf},
	{0x1d4, 8, 0xf},
	{0x1d4, 12, 0xf},
	{0x1d4, 16, 0xf},
	{0x1d4, 20, 0xf},
	{0x1d4, 24, 0xf},
	{0x1d4, 28, 0xf},
	{0x1d8, 0, 0xf},
	{0x1d8, 4, 0xf},
	{0x1d8, 8, 0xf},
	{0x1d8, 12, 0xf},
	{0x1d8, 16, 0xf},
	{0x1d8, 20, 0xf},
	{0x1d8, 24, 0xf},
	{0x1e0, 0, 0x3},
	{0x1e0, 2, 0x3},
	{0x1e0, 4, 0x3},
	{0x1e0, 6, 0x3},
	{0x1e0, 8, 0x3},
	{0x1e0, 10, 0x3},
	{0x1e0, 12, 0x3},
	{0x1e0, 14, 0x3},
	{0x1e0, 16, 0x3},
	{0x1e0, 18, 0x3},
	{0x1e4, 4, 0x3},
	{0x1e4, 6, 0x3},
	{0x1e4, 8, 0x3},
	{0x1e4, 10, 0x3},
	{0x1e4, 12, 0x3},
	{0x1e4, 14, 0x3},
	{0x1e4, 16, 0x3},
	{0x1e4, 18, 0x3},
	{0x1e4, 20, 0x3},
	{0x1e4, 22, 0x3},
	{0x1e4, 24, 0x3},
	{0x1e4, 26, 0x3},
	{0x1e4, 28, 0x3},
	{0x1ec, 0, 0x1},
	{0x1ec, 1, 0x1},
	{0x1ec, 2, 0x1},
	{0x1ec, 3, 0x1},
	{0x1ec, 4, 0x1},
	{0x1ec, 5, 0x1},
	{0x1ec, 6, 0x1},
	{0x1ec, 7, 0x1},
	{0x1ec, 8, 0x1},
	{0x1ec, 9, 0x1},
	{0x1ec, 10, 0x1},
	{0x1ec, 11, 0x1},
	{0x1ec, 12, 0x1},
	{0x1ec, 13, 0x1},
	{0x1ec, 14, 0x1},
	{0x1ec, 15, 0x1},
	{0x1ec, 16, 0x1},
	{0x1ec, 17, 0x1},
	{0x1ec, 18, 0x1},
	{0x1ec, 19, 0x1},
	{0x1ec, 20, 0x1},
	{0x1ec, 21, 0x1},
	{0x1ec, 22, 0x1},
	{0x1ec, 23, 0x1},
	{0x1ec, 24, 0x1},
	{0x1ec, 25, 0x1},
	{0x1ec, 26, 0x1},
	{0x1ec, 27, 0x1},
	{0x1ec, 28, 0x1},
	{0x1ec, 29, 0x1},
	{0x1ec, 30, 0x1},
	{0x1ec, 31, 0x1},
	{0x1f0, 0, 0x1},
	{0x1f0, 1, 0x1},
	{0x1f0, 2, 0x1},
	{0x1f0, 3, 0x1},
	{0x1f0, 4, 0x1},
	{0x1f0, 5, 0x1},
	{0x1f0, 6, 0x1},
	{0x1f0, 7, 0x1},
	{0x1f0, 8, 0x1},
	{0x1f0, 9, 0x1},
	{0x1f0, 10, 0x1},

	/* pad not mux with gpio */
	{0x1e4, 30, 0x1},	/* LVDS CN */
	{0x1e4, 31, 0x1},	/* LVDS CP */
	{0x1d8, 31, 0x1},	/* I2S1 SADATA IN/OUT */
};

static struct ark_pad_ctrl arkn141_pin_map[] = {
	{0x1e8, 4, 0x3},
	{0x1e8, 6, 0x3},
	{0x1e4, 24, 0x3},
	{0x1e4, 26, 0x3},
	{0x1c0, 12, 0x7},
	{0x1c0, 15, 0x7},
	{0x1c0, 18, 0x7},
	{0x1c0, 21, 0x7},
	{0x1c0, 24, 0x7},
	{0x1c0, 27, 0x7},
	{0x1c4, 0, 0x7},
	{0x1c4, 3, 0x7},
	{0x1c4, 6, 0x7},
	{0x1c4, 9, 0x7},
	{0x1c4, 12, 0x7},
	{0x1c4, 15, 0x7},
	{0x1c4, 18, 0x7},
	{0x1c4, 21, 0x7},
	{0x1c4, 24, 0x7},
	{0x1c4, 27, 0x7},
	{0x1c8, 0, 0x7},
	{0x1c8, 3, 0x7},
	{0x1c8, 6, 0x7},
	{0x1c8, 9, 0x7},
	{0x1c8, 12, 0x7},
	{0x1c8, 15, 0x7},
	{0x1c8, 18, 0x7},
	{0x1c8, 21, 0x7},
	{0x1c8, 24, 0x7},
	{0x1c8, 27, 0x7},
	{0x1cc, 0, 0x7},
	{0x1cc, 3, 0x7},
	{0x1cc, 6, 0x7},
	{0x1cc, 9, 0x7},
	{0x1cc, 12, 0x7},
	{0x1cc, 15, 0x7},
	{0x1cc, 18, 0x7},
	{0x1cc, 21, 0x7},
	{0x1cc, 24, 0x7},
	{0x1cc, 27, 0x7},
	{0x1d0, 0, 0x7},
	{0x1d0, 3, 0x7},
	{0x1d0, 6, 0x7},
	{0x1d0, 9, 0x7},
	{0x1d0, 12, 0x7},
	{0x1d0, 15, 0x7},
	{0x1d0, 18, 0x7},
	{0x1d0, 21, 0x7},
	{0x1d0, 24, 0x7},
	{0x1d0, 27, 0x7},
	{0x1d0, 30, 0x1},
	{0x1d0, 31, 0x1},
	{0x1d4, 0, 0x7},
	{0x1d4, 3, 0x7},
	{0x1d4, 6, 0x7},
	{0x1d4, 9, 0x7},
	{0x1d4, 12, 0x7},
	{0x1d4, 15, 0x7},
	{0x1d4, 18, 0x7},
	{0x1d4, 21, 0x7},
	{0x1d4, 24, 0x7},
	{0x1d4, 27, 0x7},
	{0x1d8, 0, 0x7},
	{0x1d8, 3, 0x7},
	{0x1d8, 6, 0x7},
	{0x1d8, 9, 0x7},
	{0x1d8, 12, 0x7},
	{0x1d8, 15, 0x7},
	{0x1d8, 18, 0x7},
	{0x1d8, 21, 0x7},
	{0x1d8, 24, 0x7},
	{0x1d8, 27, 0x7},
	{0x1dc, 0, 0x7},
	{0x1dc, 3, 0x7},
	{0x1dc, 6, 0x7},
	{0x1dc, 9, 0x7},
	{0x1dc, 12, 0x7},
	{0x1dc, 15, 0x7},
	{0x1dc, 18, 0x7},
	{0x1dc, 21, 0x7},
	{0x1e0, 0, 0x3},
	{0x1e0, 2, 0x3},
	{0x1e0, 4, 0x3},
	{0x1e0, 6, 0x3},
	{0x1e0, 8, 0x3},
	{0x1e0, 10, 0x3},
	{0x1e0, 12, 0x3},
	{0x1e0, 14, 0x3},
	{0x1e0, 16, 0x3},
	{0x1e0, 18, 0x3},
	{0x1e0, 20, 0x3},
	{0x1e0, 22, 0x3},
	{0x1e0, 24, 0x3},
	{0x1e0, 26, 0x3},
	{0x1e4, 0, 0x1},
	{0x1e4, 1, 0x1},
	{0x1e4, 2, 0x1},
	{0x1e4, 3, 0x1},
	{0x1e4, 4, 0x1},
	{0x1e4, 5, 0x1},
	{0x1e4, 6, 0x1},
	{0x1e4, 7, 0x1},
	{0x1e4, 8, 0x1},
	{0x1e4, 9, 0x1},
	{0x1e4, 10, 0x1},
	{0x1e4, 11, 0x1},
	{0x1e4, 12, 0x1},
	{0x1e4, 13, 0x1},
	{0x1e4, 16, 0x3},
	{0x1e4, 18, 0x3},
	{0x1e4, 20, 0x3},
	{0x1e4, 22, 0x3},
	{0x1e4, 32, 0},
	{0x1e4, 32, 0},
	{0x1e4, 28, 0x3},
	{0x1e4, 30, 0x3},
	{0x1e8, 0, 0x3},
	{0x1e8, 2, 0x3},
	{0x1e8, 32, 0},
	{0x1e8, 8, 0x1},
	{0x1e8, 9, 0x1},
	{0x1e8, 10, 0x1},
	{0x1e8, 11, 0x1},
	{0x1e8, 32, 0},
	{0x1c0, 9, 0x7},
	{0x1c0, 6, 0x7},
	{0x1c0, 3, 0x7},
	{0x1c0, 0, 0x7},
	/* pad not mux with gpio */

};

static struct ark_pad_ctrl amt630h_pin_map[] = {
	{0xc0, 0, 0x3},
	{0xc0, 2, 0x3},
	{0xc0, 4, 0x3},
	{0xc0, 6, 0x3},
	{0xc0, 8, 0x3},
	{0xc0, 10, 0x3},
	{0xc0, 12, 0x3},
	{0xc0, 14, 0x3},
	{0xc0, 16, 0x3},
	{0xc0, 18, 0x3},
	{0xc0, 20, 0x3},
	{0xc0, 22, 0x3},
	{0xc0, 24, 0x3},
	{0xc0, 26, 0x3},
	{0xc0, 28, 0x3},
	{0xc0, 30, 0x3},
	{0xc4, 0, 0x3},
	{0xc4, 2, 0x3},
	{0xc4, 4, 0x3},
	{0xc4, 6, 0x3},
	{0xc4, 8, 0x3},
	{0xc4, 10, 0x3},
	{0xc4, 12, 0x3},
	{0xc4, 14, 0x3},
	{0xc4, 16, 0x3},
	{0xc4, 18, 0x3},
	{0xc4, 20, 0x3},
	{0xc4, 22, 0x3},
	{0xc4, 24, 0x3},
	{0xc4, 26, 0x3},
	{0xc4, 28, 0x3},
	{0xc4, 30, 0x3},
	{0xc8, 0, 0x3},
	{0xc8, 2, 0x3},
	{0xc8, 4, 0x3},
	{0xc8, 6, 0x3},
	{0xc8, 8, 0x3},
	{0xc8, 10, 0x3},
	{0xc8, 12, 0x3},
	{0xc8, 14, 0x3},
	{0xc8, 16, 0x3},
	{0xc8, 18, 0x3},
	{0xc8, 20, 0x3},
	{0xc8, 22, 0x3},
	{0xc8, 24, 0x3},
	{0xc8, 26, 0x3},
	{0xc8, 28, 0x3},
	{0xc8, 30, 0x3},
	{0xcc, 0, 0x3},
	{0xcc, 2, 0x3},
	{0xcc, 4, 0x3},
	{0xcc, 6, 0x3},
	{0xcc, 8, 0x3},
	{0xcc, 10, 0x3},
	{0xcc, 12, 0x3},
	{0xcc, 14, 0x3},
	{0xcc, 16, 0x3},
	{0xcc, 18, 0x3},
	{0xcc, 20, 0x3},
	{0xcc, 22, 0x3},
	{0xcc, 24, 0x3},
	{0xcc, 26, 0x3},
	{0xcc, 28, 0x3},
	{0xcc, 30, 0x3},
	{0xd0, 0, 0x3},
	{0xd0, 2, 0x3},
	{0xd0, 4, 0x3},
	{0xd0, 6, 0x3},
	{0xd0, 8, 0x3},
	{0xd0, 10, 0x3},
	{0xd0, 12, 0x3},
	{0xd0, 14, 0x3},
	{0xd0, 16, 0x3},
	{0xd0, 18, 0x3},
	{0xd0, 20, 0x3},
	{0xd0, 22, 0x3},
	{0xd0, 24, 0x3},
	{0xd0, 26, 0x3},
	{0xd0, 28, 0x3},
	{0xd0, 30, 0x3},
	{0xd4, 0, 0x3},
	{0xd4, 2, 0x3},
	{0xd4, 4, 0x3},
	{0xd4, 6, 0x3},
	{0xd4, 8, 0x3},
	{0xd4, 10, 0x3},
	{0xd4, 12, 0x3},
	{0xd4, 14, 0x3},
	{0xd4, 16, 0x3},
	{0xd4, 18, 0x3},
	{0xd4, 20, 0x3},
	{0xd4, 22, 0x3},
	{0xd4, 24, 0x3},
	{0xd4, 26, 0x3},
	{0xd4, 28, 0x3},
	{0xd4, 30, 0x3},
	{0xd8, 0, 0x3},
	{0xd8, 2, 0x3},
	{0xd8, 4, 0x3},
	{0xd8, 6, 0x3},
	{0xd8, 8, 0x3},
	{0xd8, 10, 0x3},
	/* pad not mux with gpio */
};

struct ark_pmx_func {
	const char *name;
	const char **groups;
	unsigned ngroups;
};

struct ark_pmx_pin {
	uint32_t bank;
	uint32_t pin;
	uint32_t val;
	unsigned long conf;
};

struct ark_group_mux {
	uint32_t group_mux_reg;
	uint32_t group_mux_offset;
	uint32_t group_mux_mask;
	uint32_t group_mux_val;
};

struct ark_pin_group {
	const char *name;
	struct ark_pmx_pin *pins_conf;
	unsigned int *pins;
	unsigned npins;
	struct ark_group_mux *group_muxs;
	unsigned ngpmuxs;
};

struct ark_pinctrl {
	struct device *dev;
	struct pinctrl_dev *ctldev;
	void __iomem *regbase;
	struct ark_pad_ctrl *pctrl;
	int npins;
	int gpio_mux_pins;
	int is_arke;
	u32 pad_reg_offset;
	struct ark_pmx_func *functions;
	int nfunctions;

	struct ark_pin_group *groups;
	int ngroups;
};

static inline const struct ark_pin_group *ark_pinctrl_find_group_by_name(const struct ark_pinctrl *info,
									 const char *name)
{
	const struct ark_pin_group *grp = NULL;
	int i;

	for (i = 0; i < info->ngroups; i++) {
		if (strcmp(info->groups[i].name, name))
			continue;

		grp = &info->groups[i];
		dev_dbg(info->dev, "%s: %d 0:%d\n", name, grp->npins, grp->pins[0]);
		break;
	}

	return grp;
}

static int ark_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->ngroups;
}

static const char *ark_get_group_name(struct pinctrl_dev *pctldev, unsigned selector)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->groups[selector].name;
}

static int ark_get_group_pins(struct pinctrl_dev *pctldev, unsigned selector, const unsigned **pins, unsigned *npins)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= info->ngroups)
		return -EINVAL;

	*pins = info->groups[selector].pins;
	*npins = info->groups[selector].npins;

	return 0;
}

static void ark_pin_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s, unsigned offset)
{
	seq_printf(s, "%s", dev_name(pctldev->dev));
}

static int ark_dt_node_to_map(struct pinctrl_dev *pctldev,
			      struct device_node *np, struct pinctrl_map **map, unsigned *num_maps)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	const struct ark_pin_group *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	int map_num = 1;
	int i;

	/*
	 * first find the group of this node and check if we need to create
	 * config maps for pins
	 */
	grp = ark_pinctrl_find_group_by_name(info, np->name);
	if (!grp) {
		dev_err(info->dev, "unable to find group for node %s\n", np->name);
		return -EINVAL;
	}

	map_num += grp->npins;
	new_map = devm_kzalloc(pctldev->dev, sizeof(*new_map) * map_num, GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = map_num;

	/* create mux map */
	parent = of_get_parent(np);
	if (!parent) {
		devm_kfree(pctldev->dev, new_map);
		return -EINVAL;
	}
	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = parent->name;
	new_map[0].data.mux.group = np->name;
	of_node_put(parent);

	/* create config map */
	new_map++;
	for (i = 0; i < grp->npins; i++) {
		new_map[i].type = PIN_MAP_TYPE_CONFIGS_PIN;
		new_map[i].data.configs.group_or_pin = pin_get_name(pctldev, grp->pins[i]);
		new_map[i].data.configs.configs = &grp->pins_conf[i].conf;
		new_map[i].data.configs.num_configs = 1;
	}

	dev_dbg(pctldev->dev, "maps: function %s group %s num %d\n",
		(*map)->data.mux.function, (*map)->data.mux.group, map_num);

	return 0;
}

static void ark_dt_free_map(struct pinctrl_dev *pctldev, struct pinctrl_map *map, unsigned num_maps)
{
}

static const struct pinctrl_ops ark_pctrl_ops = {
	.get_groups_count = ark_get_groups_count,
	.get_group_name = ark_get_group_name,
	.get_group_pins = ark_get_group_pins,
	.pin_dbg_show = ark_pin_dbg_show,
	.dt_node_to_map = ark_dt_node_to_map,
	.dt_free_map = ark_dt_free_map,
};

static int ark_pmx_set(struct pinctrl_dev *pctldev, unsigned selector, unsigned group)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	const struct ark_pin_group *grp = &info->groups[group];
	const struct ark_pmx_pin *pins_conf = grp->pins_conf;
	const struct ark_pmx_pin *pin;
	uint32_t npins = info->groups[group].npins;
	int i;
	struct ark_pad_ctrl *pctrl;
	uint32_t val;

	dev_dbg(info->dev, "enable function %s group %s\n", info->functions[selector].name, info->groups[group].name);

	for (i = 0; i < npins; i++) {
		pin = &pins_conf[i];
		if (info->is_arke) {
			u32 npin = info->groups[group].pins[i];

			val = readl_relaxed(info->regbase + info->pad_reg_offset + PIN_REG_OFFSET(npin));
			val &= ~(PAD_BITS_MASK << PIN_BIT_OFFSET(npin));
			val |= pin->val << PIN_BIT_OFFSET(npin);
			writel_relaxed(val, info->regbase + info->pad_reg_offset + PIN_REG_OFFSET(npin));
		} else {
			pctrl = &info->pctrl[info->groups[group].pins[i]];
			val = readl_relaxed(info->regbase + pctrl->reg);
			val &= ~(pctrl->mask << pctrl->offset);
			val |= pin->val << pctrl->offset;
			writel_relaxed(val, info->regbase + pctrl->reg);
		}
	}

	for (i = 0; i < grp->ngpmuxs; i++) {
		val = readl_relaxed(info->regbase + grp->group_muxs[i].group_mux_reg);
		val &= ~(grp->group_muxs[i].group_mux_mask << grp->group_muxs[i].group_mux_offset);
		val |= grp->group_muxs[i].group_mux_val << grp->group_muxs[i].group_mux_offset;
		writel_relaxed(val, info->regbase + grp->group_muxs[i].group_mux_reg);
	}

	return 0;
}

static int ark_pmx_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->nfunctions;
}

static const char *ark_pmx_get_func_name(struct pinctrl_dev *pctldev, unsigned selector)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->functions[selector].name;
}

static int ark_pmx_get_groups(struct pinctrl_dev *pctldev, unsigned selector,
			      const char *const **groups, unsigned *const num_groups)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	*groups = info->functions[selector].groups;
	*num_groups = info->functions[selector].ngroups;

	return 0;
}

static int ark_gpio_request_enable(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range, unsigned offset)
{
	struct ark_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	struct ark_pad_ctrl *pctrl;
	u32 val;

	if (offset >= info->npins)
		return -EINVAL;

	dev_dbg(info->dev, "enable pin %u as GPIO\n", offset);

	if (info->is_arke) {
		if (offset < info->gpio_mux_pins) {
			val = readl_relaxed(info->regbase + info->pad_reg_offset + PIN_REG_OFFSET(offset));
			val &= ~(PAD_BITS_MASK << PIN_BIT_OFFSET(offset));
			writel_relaxed(val, info->regbase + info->pad_reg_offset + PIN_REG_OFFSET(offset));
		} else {
			if (offset >= 174 && offset <= 181) {
				val = readl_relaxed(info->regbase + ARKE_PAD_CTL2A);
				val &= ~(0x3 << 30);
				writel_relaxed(val, info->regbase + ARKE_PAD_CTL2A);
			}
		}
	} else {
		pctrl = &info->pctrl[offset];
		val = readl_relaxed(info->regbase + pctrl->reg);
		val &= ~(pctrl->mask << pctrl->offset);
		writel_relaxed(val, info->regbase + pctrl->reg);
	}

	return 0;
}

static void ark_gpio_disable_free(struct pinctrl_dev *pctldev, struct pinctrl_gpio_range *range, unsigned offset)
{
	struct ark_pinctrl *npct = pinctrl_dev_get_drvdata(pctldev);

	dev_dbg(npct->dev, "disable pin %u as GPIO\n", offset);
	/* Set the pin to some default state, GPIO is usually default */
}

static const struct pinmux_ops ark_pmx_ops = {
	.get_functions_count = ark_pmx_get_funcs_count,
	.get_function_name = ark_pmx_get_func_name,
	.get_function_groups = ark_pmx_get_groups,
	.set_mux = ark_pmx_set,
	.gpio_request_enable = ark_gpio_request_enable,
	.gpio_disable_free = ark_gpio_disable_free,
};

static int ark_pinconf_get(struct pinctrl_dev *pctldev, unsigned pin_id, unsigned long *config)
{
	return 0;
}

static int ark_pinconf_set(struct pinctrl_dev *pctldev, unsigned pin_id, unsigned long *configs, unsigned num_configs)
{
	return 0;
}

static void ark_pinconf_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s, unsigned pin_id)
{
}

static void ark_pinconf_group_dbg_show(struct pinctrl_dev *pctldev, struct seq_file *s, unsigned group)
{
}

static const struct pinconf_ops ark_pinconf_ops = {
	.pin_config_get = ark_pinconf_get,
	.pin_config_set = ark_pinconf_set,
	.pin_config_dbg_show = ark_pinconf_dbg_show,
	.pin_config_group_dbg_show = ark_pinconf_group_dbg_show,
};

static struct pinctrl_desc ark_pinctrl_desc = {
	.pctlops = &ark_pctrl_ops,
	.pmxops = &ark_pmx_ops,
	.confops = &ark_pinconf_ops,
	.owner = THIS_MODULE,
};

static void ark_pinctrl_child_count(struct ark_pinctrl *info, struct device_node *np)
{
	struct device_node *child;

	for_each_child_of_node(np, child) {
		info->nfunctions++;
		info->ngroups += of_get_child_count(child);
	}
}

static int ark_pinctrl_parse_groups(struct device_node *np,
				    struct ark_pin_group *grp, struct ark_pinctrl *info, u32 index)
{
	struct ark_pmx_pin *pin;
	int size;
	const __be32 *list;
	int i;

	dev_dbg(info->dev, "group(%d): %s\n", index, np->name);

	/* Initialise group */
	grp->name = np->name;

	/*
	 * the binding format is atmel,pins = <bank pin mux CONFIG ...>,
	 * do sanity check and calculate pins number
	 */
	list = of_get_property(np, "ark,pins", &size);
	if (!list)
		return -EINVAL;

	/* we do not check return since it's safe node passed down */
	size /= sizeof(*list);
	if (!size || size % 3) {
		dev_err(info->dev, "wrong pins number or pins and configs should be by 3\n");
		return -EINVAL;
	}

	grp->npins = size / 3;
	pin = grp->pins_conf = devm_kzalloc(info->dev, grp->npins * sizeof(struct ark_pmx_pin), GFP_KERNEL);
	grp->pins = devm_kzalloc(info->dev, grp->npins * sizeof(unsigned int), GFP_KERNEL);
	if (!grp->pins_conf || !grp->pins)
		return -ENOMEM;

	for (i = 0; i < grp->npins; i++) {
		pin->bank = be32_to_cpu(*list++);
		pin->pin = be32_to_cpu(*list++);
		grp->pins[i] = pin->bank * MAX_PIN_PER_BANK + pin->pin;
		pin->val = be32_to_cpu(*list++);

		dev_dbg(info->dev, "pin%d val=%d.\n", grp->pins[i], pin->val);
		pin++;
	}

	list = of_get_property(np, "group-mux", &size);
	if (list) {
		size /= sizeof(*list);
		if (size % 4) {
			dev_err(info->dev, "wrong groupmux number or groupmuxs and configs should be by 4\n");
			return -EINVAL;
		}
		grp->ngpmuxs = size / 4;
		grp->group_muxs = devm_kzalloc(info->dev, grp->ngpmuxs * sizeof(struct ark_group_mux), GFP_KERNEL);
		if (!grp->group_muxs)
			return -ENOMEM;
		for (i = 0; i < grp->ngpmuxs; i++) {
			grp->group_muxs[i].group_mux_reg = be32_to_cpu(*list++);
			grp->group_muxs[i].group_mux_offset = be32_to_cpu(*list++);
			grp->group_muxs[i].group_mux_mask = be32_to_cpu(*list++);
			grp->group_muxs[i].group_mux_val = be32_to_cpu(*list++);
		}
	}

	return 0;
}

static int ark_pinctrl_parse_functions(struct device_node *np, struct ark_pinctrl *info, u32 index)
{
	struct device_node *child;
	struct ark_pmx_func *func;
	struct ark_pin_group *grp;
	int ret;
	static u32 grp_index;
	u32 i = 0;

	dev_dbg(info->dev, "parse function(%d): %s\n", index, np->name);

	func = &info->functions[index];

	/* Initialise function */
	func->name = np->name;
	func->ngroups = of_get_child_count(np);
	if (func->ngroups == 0) {
		dev_err(info->dev, "no groups defined\n");
		return -EINVAL;
	}
	func->groups = devm_kzalloc(info->dev, func->ngroups * sizeof(char *), GFP_KERNEL);
	if (!func->groups)
		return -ENOMEM;

	for_each_child_of_node(np, child) {
		func->groups[i] = child->name;
		grp = &info->groups[grp_index++];
		ret = ark_pinctrl_parse_groups(child, grp, info, i++);
		if (ret) {
			of_node_put(child);
			return ret;
		}
	}

	return 0;
}

static const struct of_device_id ark_pinctrl_of_match[] = {
	{.compatible = "arkmicro,ark1668-pinctrl",},
	{.compatible = "arkmicro,arkn141-pinctrl",},
	{.compatible = "arkmicro,arke-pinctrl",},
	{.compatible = "arkmicro,amt630h-pinctrl",},
	{ /* sentinel */ }
};

static int ark_pinctrl_probe_dt(struct platform_device *pdev, struct ark_pinctrl *info)
{
	int ret = 0;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *child;
	int i = 0;

	if (!np)
		return -ENODEV;

	info->dev = &pdev->dev;
	ark_pinctrl_child_count(info, np);

	dev_dbg(&pdev->dev, "nfunctions = %d\n", info->nfunctions);
	dev_dbg(&pdev->dev, "ngroups = %d\n", info->ngroups);

	info->functions = devm_kzalloc(&pdev->dev, info->nfunctions * sizeof(struct ark_pmx_func), GFP_KERNEL);
	if (!info->functions)
		return -ENOMEM;

	info->groups = devm_kzalloc(&pdev->dev, info->ngroups * sizeof(struct ark_pin_group), GFP_KERNEL);
	if (!info->groups)
		return -ENOMEM;

	dev_dbg(&pdev->dev, "nfunctions = %d\n", info->nfunctions);
	dev_dbg(&pdev->dev, "ngroups = %d\n", info->ngroups);

	for_each_child_of_node(np, child) {
		ret = ark_pinctrl_parse_functions(child, info, i++);
		if (ret) {
			dev_err(&pdev->dev, "failed to parse function\n");
			of_node_put(child);
			return ret;
		}
	}

	return 0;
}

static int ark_pinctrl_probe(struct platform_device *pdev)
{
	struct ark_pinctrl *info;
	struct pinctrl_pin_desc *pdesc;
	struct resource *res;
	int ret, i;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->regbase = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(info->regbase)) {
		return PTR_ERR(info->regbase);
	}

	ret = ark_pinctrl_probe_dt(pdev, info);
	if (ret)
		return ret;

	ark_pinctrl_desc.name = dev_name(&pdev->dev);
	if (of_device_is_compatible(pdev->dev.of_node, "arkmicro,ark1668-pinctrl")) {
		info->pctrl = ark1668_pin_map;
		info->npins = ark_pinctrl_desc.npins = ARRAY_SIZE(ark1668_pin_map);
	} else if (of_device_is_compatible(pdev->dev.of_node, "arkmicro,arkn141-pinctrl")) {
		info->pctrl = arkn141_pin_map;
		info->npins = ark_pinctrl_desc.npins = ARRAY_SIZE(arkn141_pin_map);
	} else if (of_device_is_compatible(pdev->dev.of_node, "arkmicro,amt630h-pinctrl")) {
		info->pctrl = amt630h_pin_map;
		info->npins = ark_pinctrl_desc.npins = ARRAY_SIZE(amt630h_pin_map);
	} else if (of_device_is_compatible(pdev->dev.of_node, "arkmicro,arke-pinctrl")) {
		info->is_arke = 1;
		if (of_property_read_u32(pdev->dev.of_node, "npins", &info->npins)) {
			dev_err(&pdev->dev, "could not get npins.\n");
			return -EINVAL;
		}
		ark_pinctrl_desc.npins = info->npins;
		if (of_property_read_u32(pdev->dev.of_node, "gpio-mux-pins", &info->gpio_mux_pins)) {
			dev_err(&pdev->dev, "could not get gpio_mux_pins.\n");
			return -EINVAL;
		}
		if (of_property_read_u32(pdev->dev.of_node, "pad-reg-offset", &info->pad_reg_offset)) {
			dev_err(&pdev->dev, "could not get pad_reg_offset.\n");
			return -EINVAL;
		}
	}
	ark_pinctrl_desc.pins = pdesc = devm_kzalloc(&pdev->dev, sizeof(*pdesc) * ark_pinctrl_desc.npins, GFP_KERNEL);

	if (!ark_pinctrl_desc.pins)
		return -ENOMEM;

	for (i = 0; i < ark_pinctrl_desc.npins; i++) {
		pdesc->number = i;
		pdesc->name = kasprintf(GFP_KERNEL, "pin%d", i);
		pdesc++;
	}

	platform_set_drvdata(pdev, info);
	info->ctldev = devm_pinctrl_register(&pdev->dev, &ark_pinctrl_desc, info);

	if (IS_ERR(info->ctldev)) {
		dev_err(&pdev->dev, "could not register ark pinctrl driver\n");
		return PTR_ERR(info->ctldev);
	}

	dev_info(&pdev->dev, "initialized ark pinctrl driver\n");

	return 0;
}

static struct platform_driver ark_pinctrl_driver = {
	.driver = {
		   .name = "pinctrl-ark",
		   .of_match_table = of_match_ptr(ark_pinctrl_of_match),
		   },
	.probe = ark_pinctrl_probe,
};

static int __init ark_pinctrl_init(void)
{
	return platform_driver_register(&ark_pinctrl_driver);
}

postcore_initcall(ark_pinctrl_init);

MODULE_AUTHOR("Sim");
MODULE_DESCRIPTION("Arkmicro Pinctrl driver");
MODULE_LICENSE("GPL v2");
