/*
 * This file contains Xilinx specific SMP code, used to start up
 * the second processor.
 *
 * Copyright (C) 2011-2013 Xilinx
 *
 * based on linux/arch/arm/mach-realview/platsmp.c
 *
 * Copyright (C) 2002 ARM Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/export.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/cacheflush.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/delay.h>

#include "common.h"

#define SYS_CPU_CTL     0x208

int ark_cpun_start(u32 address, int cpu)
{
	u32 trampoline_code_size = &ark_secondary_trampoline_end -
						&ark_secondary_trampoline;
	struct device_node *node, *sysnode;
	struct resource res;
	int ret;
	
	node = of_find_compatible_node(NULL, NULL, "arkmicro,arke-iram");
	if (!node) {
		pr_err("%s: could not find iram dt node\n", __func__);
		return -1;
	}

	sysnode = of_find_compatible_node(NULL, NULL, "arkmicro,ark-sregs");
	if (!sysnode) {
		pr_err("%s: could not find sregs dt node\n", __func__);
		return -1;
	}

	ret = of_address_to_resource(node, 0, &res);
	if (ret < 0) {
		pr_err("%s: could not get address for node %pOF\n",
		       __func__, node);
		return ret;
	}

	/* MS: Expectation that SLCR are directly map and accessible */
	/* Not possible to jump to non aligned address */
	if (!(address & 3) && (!address || (address >= trampoline_code_size))) {
		/* Store pointer to ioremap area which points to address 0x0 */
		static u8 __iomem *iram_base, *sys_base;
        u32 val;
		u32 trampoline_size = &ark_secondary_trampoline_jump -
						&ark_secondary_trampoline;

		if (address) {
			iram_base = of_iomap(node, 0);
			if (!iram_base) {
				pr_warn("BOOTUP jump vectors not accessible\n");
				return -1;
			}
            
            sys_base = of_iomap(sysnode, 0);
            if (!sys_base) {
                pr_warn("sysreg not accessible\n");
                return -1;
            }

			/*
			* This is elegant way how to jump to any address
			* 0x0: Load address at 0x8 to r0
			* 0x4: Jump by mov instruction
			* 0x8: Jumping address
			*/
			memcpy((__force void *)iram_base, &ark_secondary_trampoline,
							trampoline_size);
			writel(address, iram_base + trampoline_size);

			flush_cache_all();
			outer_flush_range(res.start, trampoline_code_size);
            smp_wmb();

            /* start cpun */
            val = readl(sys_base + SYS_CPU_CTL);
            val |= 1 << (6 + cpu);
            writel(val, sys_base +  SYS_CPU_CTL);

            //iounmap(iram_base);
            //iounmap(sys_base);
		}

		return 0;
	}

	pr_warn("Can't start CPU%d: Wrong starting address %x\n", cpu, address);

	return -1;
}
EXPORT_SYMBOL(ark_cpun_start);

static int ark_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	return ark_cpun_start(__pa_symbol(secondary_startup), cpu);
}

static void __init ark_smp_prepare_cpus(unsigned int max_cpus)
{
}

const struct smp_operations ark_smp_ops __initconst = {
	.smp_prepare_cpus	= ark_smp_prepare_cpus,
	.smp_boot_secondary	= ark_boot_secondary,
};

CPU_METHOD_OF_DECLARE(arke_smp, "arkmicro,arke-smp", &ark_smp_ops);
