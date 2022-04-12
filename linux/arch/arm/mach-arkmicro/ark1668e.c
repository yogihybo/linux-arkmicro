/*
 *  Setup code for arkmicro soc
 *
 * Licensed under GPLv2 or later.
 */
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/amba/serial.h>
#include <linux/dma/dw.h>
#include <linux/device.h>
#include <linux/of.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/system_misc.h>
#include <asm/smp_scu.h>

void __iomem *ark_scu_base;

static struct map_desc ark_cortex_a9_scu_map __initdata = {
	.length	= SZ_256,
	.type	= MT_DEVICE,
};

static void __init ark_scu_map_io(void)
{
	unsigned long base;

	base = scu_a9_get_base();
	ark_cortex_a9_scu_map.pfn = __phys_to_pfn(base);
	/* Expected address is in vmalloc area that's why simple assign here */
	ark_cortex_a9_scu_map.virtual = base;
	iotable_init(&ark_cortex_a9_scu_map, 1);
	ark_scu_base = (void __iomem *)base;
	BUG_ON(!ark_scu_base);
}

/**
 * ark_map_io - Create memory mappings needed for early I/O.
 */
static void __init ark_map_io(void)
{
	debug_ll_io_init();
	ark_scu_map_io();
}

static const char * const ark_board_dt_compat[] = {
	"arkmicro,ark1668e",
	NULL,
};

DT_MACHINE_START(ARKMIDRO_DT, "Arkmicro arm soc Family")
	.map_io		= ark_map_io,
	.dt_compat	= ark_board_dt_compat,
MACHINE_END
