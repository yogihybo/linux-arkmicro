/*
 * This header provides constants for ark clk.
 *
 * The constants defined in this header are being used in dts.
 *
 * Licensed under GPLv2 or later.
 */

#ifndef _DT_BINDINGS_CLK_ARK_H
#define _DT_BINDINGS_CLK_ARK_H

#define ARK_CLK_DIVMODE_NOZERO		0		/* div = div ? div : 1 */
#define ARK_CLK_DIVMODE_PLUSONE		1		/* div = div + 1 */
#define ARK_CLK_DIVMODE_DOUBLE		2		/* div = div * 2 */
#define ARK_CLK_DIVMODE_EXPONENT	3		/* div = 1 << div */
#define ARK_CLK_DIVMODE_PONEDOUBLE	4		/* div = (div + 1) * 2 */

#define ARK_CLK_WDT_DIV16           0
#define ARK_CLK_WDT_DIV32           1
#define ARK_CLK_WDT_DIV64           2
#define ARK_CLK_WDT_DIV128          3

#endif
