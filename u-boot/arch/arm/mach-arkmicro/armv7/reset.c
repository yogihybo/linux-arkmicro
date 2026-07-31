/*
 * (C) Copyright 2013 ASTRI
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/io.h>

/* WDT */
#define WDT_CR	0x00
#define WDT_PSR	0x04
#define WDT_LDR	0x08
#define WDT_VLR	0x0C
#define WDT_ISR	0x10
#define WDT_RCR	0x14
#define WDT_TMR	0x18
#define WDT_TCR	0x1C

#define rSYS_SOFT_RSTNA			0x74
#define rSYS_SOFT_RSTNB			0x78

extern unsigned long ark_get_apb_clock(void);

#define div_round_up(x, div) (((x) + (div) - 1) / (div))

/*
 * Arms the hardware watchdog (RSTEN set -- a real, unassisted SoC reset,
 * no OS involvement needed) to fire after timeout_ms if nothing
 * reprograms it first. Originally private to reset_cpu() below with a
 * short timeout as a "force an immediate reset" trick; also reused
 * (2026-07-31) by board/arkmicro/ark1668_limcet_p305/ark1668_boot_cmds.c
 * to arm a longer boot-supervision timeout right before jumping into the
 * kernel -- same primitive, just a different caller-supplied timeout.
 * Returns immediately, does not block.
 *
 * 2026-07-31: WDT_PSR (the prescaler register) REQUIRES a power-of-two
 * value -- confirmed empirically on real hardware (prescaler=2 and
 * prescaler=256 both correctly fired a reset at the expected time;
 * prescaler=290, the plain div_round_up() result for a 30s timeout,
 * never fired at all even after 30+ seconds). Very likely a one-hot/
 * shift-selector field internally (each bit position N meaning
 * "divide by 2^N", only one bit meant to ever be set) rather than a
 * literal linear divisor -- 290 = 0b100100010 has three bits set,
 * 2 and 256 each have exactly one. No datasheet to confirm the exact
 * mechanism against, but the empirical A/B result across three real
 * hardware tests is unambiguous. This was a real, previously-undiscovered
 * bug: the original reset_cpu() usage (100ms/2000ms) never exercised
 * this path at all (both fit under the register's max with prescaler=1),
 * so it was never caught until this session's longer boot-supervision
 * timeouts needed it. See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md §85.
 */
void ark_wdt_arm(u32 timeout_ms)
{
	u32 regbase = CONFIG_WATCHDOG_BASEADDR;
	u32 freq = ark_get_apb_clock();
	u32 count, divisor = 1;

	freq = div_round_up(freq, 128);
	count = timeout_ms * (u64)freq / 1000;
	if (count > 0xffff) {
		u32 min_divisor = div_round_up(count, 0xffff);

		divisor = 1;
		while (divisor < min_divisor)
			divisor <<= 1;
		if (divisor > 0xffff) {
			printf("timeout %d too big\n", divisor);
			return;
		}
	}
	count = div_round_up(count, divisor);
	writel(0, regbase + WDT_CR);
	writel(divisor, regbase + WDT_PSR);
	writel(count, regbase + WDT_LDR);
	writel((0x3 << 4) | 3, regbase + WDT_CR);
}
static void ark1668e_softreset(void)
{
	u32 sysregbase = 0xe4900000;
	writel(0, sysregbase + rSYS_SOFT_RSTNA);
	writel(0, sysregbase + rSYS_SOFT_RSTNB);
}

void reset_cpu(ulong addr)
{
	/* TODO: Program the system controller to reset */
#ifdef CONFIG_ARK1668EFAMILY
	ark_wdt_arm(2000);
	ark1668e_softreset();
#else
	ark_wdt_arm(100);
#endif
	/* loop for waiting reset */
	while(1);
}
