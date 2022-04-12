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

static const char * const ark_board_dt_compat[] = {
	"arkmicro,ark1668",
	"arkmicro,n141",
	NULL,
};

DT_MACHINE_START(ARKMIDRO_DT, "Arkmicro arm soc Family")
	.dt_compat	= ark_board_dt_compat,
	.l2c_aux_mask	= ~0UL,
MACHINE_END
