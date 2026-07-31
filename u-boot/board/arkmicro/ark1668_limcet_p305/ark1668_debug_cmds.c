/*
 * gpiotest / jpeghw / itu656 — ported from the stock production u-boot
 * binary (mtd1_uboot.bin), which has no released source anywhere. Recovered
 * via Ghidra decompilation of the stock command table; see
 * docs/uboot_build.md for the reverse-engineering trail.
 *
 * Each port below carries a different confidence level — read the comment
 * at the top of each command before trusting it on real hardware:
 *
 *   regr/regw/pmem  -> ported in ark1668_display_cfg.c, high confidence
 *                      (plain address peek/poke, no protocol to get wrong)
 *   gpiotest        -> high confidence for modes 0/1 (pure GPIO read/clear).
 *                      Mode 2 (interrupt test) is NOT ported — see below.
 *   jpeghw          -> moderate confidence. The register sequence and
 *                      offsets are decompiled faithfully, but the stock
 *                      version is interrupt-driven (VIC line 10) and we do
 *                      NOT port the VIC/ARM-IRQ-vector plumbing that would
 *                      require — u-boot here runs with IRQs masked and
 *                      porting a working IRQ vector is a separate, riskier
 *                      task (get it wrong and the CPU hangs/crashes). This
 *                      version polls the JPEG status register directly
 *                      instead, using the exact same status-bit logic the
 *                      stock ISR uses (see FUN_00069980 in the RE trail).
 *   itu656          -> lowest confidence. The NTSC timing constants below
 *                      are real, confirmed from two independent sources
 *                      (display/arkdata.ini AND the stock binary's own
 *                      built-in default table — they match exactly), but
 *                      the *bit-packing* of those constants into the
 *                      ITU656_BASE registers (FUN_0006e870) is a literal
 *                      instruction-by-instruction translation with no
 *                      datasheet to confirm field semantics against.
 */

#include "ark1668_lcd.h"
#include <console.h>
#include <nand.h>
#include <linux/mtd/mtd.h>

/* ---------------------------------------------------------------------
 * gpiotest
 * --------------------------------------------------------------------- */

#define GPIO_GROUP_STRIDE	0x20
#define GPIO_NUM_GROUPS		4

static void gpio_clear_bit_a(unsigned int pin)
{
	/* stock FUN_0006906c: GPIO_BASE + {0x00,0x20,0x40,0x60}, one bit per pin */
	unsigned int grp = pin / 32;
	unsigned int bit = pin % 32;
	if (grp >= GPIO_NUM_GROUPS) {
		printf("[gpiotest] pin %u out of range\n", pin);
		return;
	}
	*(volatile unsigned int *)(GPIO_BASE + grp * GPIO_GROUP_STRIDE) &= ~(1U << bit);
}

static void gpio_clear_bit_b(unsigned int pin)
{
	/* stock FUN_00068fd0: GPIO_BASE + {0x04,0x24,0x44,0x64} */
	unsigned int grp = pin / 32;
	unsigned int bit = pin % 32;
	if (grp >= GPIO_NUM_GROUPS) {
		printf("[gpiotest] pin %u out of range\n", pin);
		return;
	}
	*(volatile unsigned int *)(GPIO_BASE + 0x04 + grp * GPIO_GROUP_STRIDE) &= ~(1U << bit);
}

/* mode 0: watch GPIO_BASE+0x04 (input data, pins 0-5) for changes against
 * the value read at start. Stock version loops forever with no escape;
 * we add a ctrlc() check as a safety improvement over the original. */
static void gpiotest_input(void)
{
	unsigned int baseline, i;

	gpio_clear_bit_a(0); gpio_clear_bit_a(1); gpio_clear_bit_a(2);
	gpio_clear_bit_b(0); gpio_clear_bit_b(1); gpio_clear_bit_b(2);

	baseline = *(volatile unsigned int *)(GPIO_BASE + 0x04);
	printf("[gpiotest] input: baseline=0x%08x (Ctrl-C to stop)\n", baseline);

	while (!ctrlc()) {
		unsigned int cur = *(volatile unsigned int *)(GPIO_BASE + 0x04);
		for (i = 0; i < 6; i++) {
			if ((cur ^ baseline) & (1U << i))
				printf("[gpiotest] pin %u changed (now %u)\n", i,
				       (cur >> i) & 1);
		}
	}
}

/* mode 1: blink GPIO_BASE+0x04 bits 0-5 on/off with ~100ms spacing.
 * Stock version loops forever with no escape; ctrlc() added here too. */
static void gpiotest_output(void)
{
	unsigned int i;

	gpio_clear_bit_a(0); gpio_clear_bit_a(1); gpio_clear_bit_a(2);
	gpio_clear_bit_a(3); gpio_clear_bit_a(4); gpio_clear_bit_a(5);

	printf("[gpiotest] output: blinking pins 0-5 (Ctrl-C to stop)\n");
	while (!ctrlc()) {
		volatile unsigned int *reg = (volatile unsigned int *)(GPIO_BASE + 0x04);
		for (i = 0; i < 6; i++)
			*reg |= (1U << i);
		mdelay(100);
		if (ctrlc())
			break;
		for (i = 0; i < 6; i++)
			gpio_clear_bit_b(i);
		mdelay(100);
	}
}

int do_gpiotest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long mode;

	if (argc < 2)
		return cmd_usage(cmdtp);
	if (strict_strtoul(argv[1], 10, &mode) < 0) {
		printf("[gpiotest] error format mode\n");
		return 1;
	}

	switch (mode) {
	case 0:
		gpiotest_input();
		break;
	case 1:
		gpiotest_output();
		break;
	case 2:
		/* Stock mode 2 combines JPEG clock bring-up with registering
		 * dummy interrupt callbacks through the VIC — deliberately
		 * not ported, see file header. */
		printf("[gpiotest] mode 2 (interrupt test) requires the VIC/IRQ "
		       "plumbing, which is intentionally not ported here.\n");
		break;
	default:
		printf("[gpiotest] wrong select\n");
		break;
	}

	return 0;
}

U_BOOT_CMD(
	gpiotest, 4, 0, do_gpiotest,
	"test gpio input | output",
	"gpiotest mode\n"
	"  0 = watch input pins 0-5 for changes (Ctrl-C to stop)\n"
	"  1 = blink output pins 0-5 (Ctrl-C to stop)\n"
	"  2 = not supported (would need ported VIC/IRQ support)\n"
);

/* ---------------------------------------------------------------------
 * jpeghw — hardware JPEG decode, polling variant (see file header)
 * --------------------------------------------------------------------- */

#define JREG(off)	(*(volatile unsigned int *)(JPEG_BASE + (off)))

/* status bits at JPEG_BASE+0x34, as read by the stock ISR (FUN_00069980) */
#define JPEG_STAT_ERROR		(1 << 2)
#define JPEG_STAT_DONE		(1 << 0)
#define JPEG_STAT_BUSY		(1 << 5)

static int jpeg_hw_decode(unsigned int src, unsigned int dst,
			   unsigned int *out_w, unsigned int *out_h)
{
	unsigned int status;
	long timeout;

	JREG(0x3c) = 0x3f;		/* INTCLR: clear pending */
	JREG(0x2c) = 0;		/* CTRL = 0 */
	JREG(0x2c) |= 3;
	JREG(0x2c) &= ~3;		/* reset pulse */
	JREG(0x04) = 0x108;
	JREG(0x2c) = 0x28000738;	/* CTRL: main mode config */
	JREG(0x50) = 0xff;		/* COUNT */
	JREG(0x38) = 0x2f;		/* INTMASK (unused here — polling, not IRQ) */
	JREG(0x5c) = src;		/* dec_rd_base_addr: JPEG source in RAM */
	JREG(0x24) = dst;		/* decoded Y-plane destination */
	JREG(0x28) = dst + 0x200000;	/* decoded chroma-plane destination (+2MB) */
	JREG(0x48) = 0;
	JREG(0x4c) = 0;
	JREG(0x54) = 0;
	JREG(0x58) = 0;
	JREG(0x2c) |= 0x2000;
	JREG(0x00) = 1;
	JREG(0x30) |= 0x80000000;	/* START */

	timeout = 100000;
	do {
		status = JREG(0x34);
		if (status & (JPEG_STAT_ERROR | JPEG_STAT_DONE))
			break;
	} while (--timeout > 0);

	if (timeout <= 0) {
		printf("[jpeghw] timed out waiting for decode\n");
		return -1;
	}
	if (status & JPEG_STAT_ERROR) {
		printf("[jpeghw] decode error, status=0x%08x\n", status);
		JREG(0x3c) = 0xff;
		return -1;
	}

	JREG(0x3c) = 1;
	/* decoded width/height live in the upper 16 bits of these two regs,
	 * per the stock ISR (FUN_00069980) */
	*out_w = *(volatile unsigned int *)(JPEG_BASE + 0x04) >> 16;
	*out_h = *(volatile unsigned int *)(JPEG_BASE + 0x0c) >> 16;
	return 0;
}

int do_jpeghw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long src, dst, w = 0, h = 0;

	if (argc < 3)
		return cmd_usage(cmdtp);

	if (strict_strtoul(argv[1], 16, &src) < 0 ||
	    strict_strtoul(argv[2], 16, &dst) < 0) {
		printf("[jpeghw] error format addr\n");
		return 1;
	}

	if (jpeg_hw_decode(src, dst, (unsigned int *)&w, (unsigned int *)&h) == 0)
		printf("[jpeghw] decoded %lux%lu -> 0x%08lx (Y), 0x%08lx (chroma)\n",
		       w, h, dst, dst + 0x200000);

	return 0;
}

U_BOOT_CMD(
	jpeghw, 3, 0, do_jpeghw,
	"decode a JPEG using the hardware codec (polling, not IRQ-driven)",
	"jpeghw src_addr dst_addr\n"
	"  src_addr: RAM address of the JPEG file (hex)\n"
	"  dst_addr: RAM address to decode into (hex); chroma plane written\n"
	"            at dst_addr+0x200000\n"
);

/* ---------------------------------------------------------------------
 * itu656 — NTSC composite video input timing setup
 * --------------------------------------------------------------------- */

/* Confirmed from two independent sources: display/arkdata.ini (the actual
 * calibration shipped for this camera) and the stock binary's own built-in
 * default table (found alongside the command table in mtd1_uboot.bin) —
 * both agree exactly. */
#define NTSC_MODE_CONTROL	0x00001D80
#define NTSC_VGATE_DELAY	0
#define NTSC_DEN_V_STOP		10
#define NTSC_DEN_V_START	10
#define NTSC_TVGDEL		2
#define NTSC_TVSYNC		2
#define NTSC_THGDEL		20
#define NTSC_THSYNC		2
#define NTSC_THLEN		954
#define NTSC_THGATE		800
#define NTSC_TVLEN		525
#define NTSC_TVGATE		480
#define NTSC_VFZ		516
#define NTSC_HFZ		916
#define NTSC_SYNC_UP		4
#define NTSC_SYNC_DOWN		3
#define NTSC_DATENA_INV		0
#define NTSC_VSYNC_INV		0
#define NTSC_HSYNC_INV		0
#define NTSC_FIELD_INV		0
#define NTSC_HV_DELAY		1

/* 2026-07-31: extracted from do_itu656() below so the automatic
 * reverse-gear boot-time camera preview (ark_carback_camera_check(),
 * ark1668_display_cfg.c) can call the exact same, already-ported
 * register sequence instead of duplicating it. Behavior unchanged --
 * do_itu656 still calls this and is still manually invokable for
 * testing. See this function's own history (docs/UBOOT_REVERSE_ENGINEERING.md
 * §7) before trusting it further: never hardware-tested as of this
 * refactor. */
void ark_itu656_camera_bypass_enable(void)
{
	/* Enable pad/clock for the ITU656 block (stock FUN_0006e800) */
	*(volatile unsigned int *)(SYS_BASE + 0x1ec) |= 0x1ff0000;
	*(volatile unsigned int *)(SYS_BASE + 0x1e8) =
		(*(volatile unsigned int *)(SYS_BASE + 0x1e8) & 0xffffff0f) | 0x50;

	/* Timing/sync registers, transcribed field-for-field from the stock
	 * FUN_0006e870 (verified against the decompiled shift amounts, not
	 * guessed) with real NTSC constants substituted in. Field semantics
	 * for the individual bit positions are inferred from those shift
	 * amounts, not a datasheet — see file header. PAL registers exist in
	 * the same block but are intentionally not written here since we
	 * only have/need NTSC values. */
	*(volatile unsigned int *)(LCD_BASE + 0x3d0) = NTSC_MODE_CONTROL;
	*(volatile unsigned int *)(LCD_BASE + 0x3fc) =
		NTSC_DEN_V_START | 0x43000000 |
		(NTSC_VGATE_DELAY << 16) | (NTSC_DEN_V_STOP << 8);
	*(volatile unsigned int *)(LCD_BASE + 0x3d4) =
		(NTSC_TVSYNC << 16) | (NTSC_TVGDEL << 24) | NTSC_THSYNC |
		(NTSC_THGDEL << 8);
	*(volatile unsigned int *)(LCD_BASE + 0x3d8) = NTSC_THGATE | (NTSC_THLEN << 16);
	*(volatile unsigned int *)(LCD_BASE + 0x3dc) = NTSC_TVGATE | (NTSC_TVLEN << 16);
	*(volatile unsigned int *)(LCD_BASE + 0x3e0) = NTSC_HFZ | (NTSC_VFZ << 16);
	*(volatile unsigned int *)(LCD_BASE + 0x3e4) =
		(NTSC_SYNC_DOWN << 12) | (NTSC_SYNC_UP << 16) | NTSC_HV_DELAY |
		(NTSC_DATENA_INV << 11) | (NTSC_VSYNC_INV << 10) |
		(NTSC_HSYNC_INV << 9) | (NTSC_FIELD_INV << 8);

	/* Enable the ITU656 block itself (stock FUN_0006e9d8) */
	*(volatile unsigned int *)(LCD_BASE + 0x320) |= 0x800;
	*(volatile unsigned int *)(SYS_BASE + 0x1ec) |= 0x1ff0000;
	*(volatile unsigned int *)(SYS_BASE + 0x1e8) =
		(*(volatile unsigned int *)(SYS_BASE + 0x1e8) & 0xffffff0f) | 0x50;

	/* 2026-07-31: previously-omitted VIDEO2 layer configuration, stock
	 * FUN_0006e9f0 (the caller of both the block above and the one
	 * below) -- this is the block a prior version of this comment
	 * flagged as "left out rather than guessed... if the picture looks
	 * wrong, that block is the first place to look." Confirmed on real
	 * hardware (2026-07-31) that the picture DOES look wrong without
	 * it (visible screen flickering) -- decoded and cross-validated
	 * against this project's OWN independently-written kernel LCDC
	 * driver (drivers/video/fbdev/arkmicro/ark1668_lcdc_funcs.c, the
	 * already-hardware-confirmed Android Auto video scaler-bypass
	 * fix), which uses these exact registers/bit positions for
	 * VIDEO_LAYER2 -- not just a fresh guess at the disassembly.
	 *
	 * rLCD_VIDEO2_WIN_SIZE (0x32c): stock's literal 0x1202d0 decodes
	 * as width=720 height=288, packed (height&0xfff)<<12 |
	 * (width&0xfff) -- the exact convention
	 * ark1668_lcdc_set_video_win_size() uses in that kernel driver.
	 * This is the camera's own native source window, not
	 * panel-dependent, reused verbatim.
	 *
	 * rLCD_VIDEO2_SIZE (0x330): stock computes this from a runtime
	 * "current screen" struct (the active PANEL's own width/height,
	 * same packing) -- for this board that's the confirmed 800x480
	 * panel (every boot log's own "[screen_info] ... Width:800,
	 * Height:480" line), hardcoded below the same way the NTSC_*
	 * constants above are also fixed, board-specific values rather
	 * than dynamically read.
	 *
	 * rLCD_VIDEO2_SCALE_CTL (0x354): stock's literal 0x80 = bit 7 only
	 * ("YUV format" per that same kernel driver's own documented bit
	 * meaning for this register) -- board-independent, reused
	 * verbatim.
	 *
	 * rLCD_CONTROL (0x4) read-modify-write: sets bit 6, clears bits 7
	 * and 9. Bit 6 is independently confirmed as VIDEO_LAYER2's enable
	 * bit by ark1668_lcdc_set_video_en() in that same kernel driver
	 * file (offset=6 for VIDEO_LAYER2) -- code written completely
	 * independently of this U-Boot disassembly, a real cross-check,
	 * not a guess. Bits 7/9's exact meaning isn't independently
	 * confirmed; replicated verbatim rather than reinterpreted.
	 *
	 * Stock branches here on a runtime screen-type value (unresolved
	 * in this port) between this rLCD_CONTROL path and an alternate
	 * rLCD_TV_CONTROL path (LCD_BASE+0x2b0) for TV/composite output.
	 * This board's screen type is confirmed RGB (ScreenType:0 in every
	 * boot log), not a TV/composite output, so the rLCD_CONTROL path
	 * is hardcoded here rather than conditionally selected. */
#define ARK1668_ITU656_PANEL_WIDTH	800
#define ARK1668_ITU656_PANEL_HEIGHT	480
	*(volatile unsigned int *)(LCD_BASE + 0x32c) = 0x1202d0;
	*(volatile unsigned int *)(LCD_BASE + 0x330) =
		((ARK1668_ITU656_PANEL_HEIGHT & 0xfff) << 12) |
		(ARK1668_ITU656_PANEL_WIDTH & 0xfff);
	*(volatile unsigned int *)(LCD_BASE + 0x354) = 0x80;
	*(volatile unsigned int *)(LCD_BASE + 0x4) =
		(*(volatile unsigned int *)(LCD_BASE + 0x4) | 0x40) & ~(0x80 | 0x200);

	/* stock FUN_0006e82c(1) */
	*(volatile unsigned int *)(ITU656_BASE + 0x900) |= 1;
	*(volatile unsigned int *)(ITU656_BASE) |= 6;
	*(volatile unsigned int *)(ITU656_BASE + 0x8fc) = 0x1e0a;
}

int do_itu656(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ark_itu656_camera_bypass_enable();
	printf("[itu656] NTSC timing configured\n");
	return 0;
}

U_BOOT_CMD(
	itu656, 1, 0, do_itu656,
	"configure NTSC composite video input timing",
	"itu656\n"
);

/* ---------------------------------------------------------------------
 * nandoobcheck — dump the raw OOB bytes of a NAND page (MTD_OPS_RAW,
 * bypassing this driver's ECC/BBT interpretation — the same path the
 * built-in `nand dump.oob` command uses) side by side with what this
 * driver's own cached bad-block table currently believes about that
 * block.
 *
 * Added to test the theory documented in
 * docs/HANDOFF_touch_and_bootargs_fix.md ("Fix C") and re-raised on real
 * hardware: this driver sets NAND_BBT_USE_FLASH (see
 * ark1668_display_cfg.c... actually ark_nand.c board_nand_init), which
 * means it only does a full factory-marker OOB scan ONCE, then caches
 * the result to a reserved area of flash and just reads that cache back
 * on every later boot. If that first scan misread the OOB layout (this
 * chip uses NAND_ECC_HW_SYNDROME, where ECC bytes are interleaved into
 * the OOB in a driver-specific order, not the conventional
 * trailing-bytes layout), it would have permanently cached a wrong
 * table — matching the "417 false bad blocks" signature seen before, and
 * matching the "stock loader shows no bad blocks" observation, since
 * stock uses its own separate driver/cache and never looks at this
 * one's cached table.
 *
 * If the raw marker byte reads 0xFF (factory-good) on a block this
 * driver's block_isbad() reports as bad, that's the smoking gun: the
 * cached table is wrong, not the physical chip.
 * --------------------------------------------------------------------- */

int do_nandoobcheck(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct mtd_info *mtd;
	struct nand_chip *chip;
	struct mtd_oob_ops ops;
	u_char *oobbuf;
	loff_t off;
	int ret, is_bad, i, pos;

	if (argc < 2) {
		printf("[nandoobcheck] usage: nandoobcheck <page-or-block-offset (hex)>\n");
		return 1;
	}

	mtd = get_nand_dev_by_index(nand_curr_device);
	if (!mtd) {
		printf("[nandoobcheck] no NAND device\n");
		return 1;
	}
	chip = mtd_to_nand(mtd);

	off = (loff_t)simple_strtoul(argv[1], NULL, 16);
	off &= ~((loff_t)mtd->writesize - 1);

	oobbuf = memalign(ARCH_DMA_MINALIGN, mtd->oobsize);
	if (!oobbuf) {
		printf("[nandoobcheck] out of memory\n");
		return 1;
	}

	memset(&ops, 0, sizeof(ops));
	ops.oobbuf = oobbuf;
	ops.ooblen = mtd->oobsize;
	ops.mode = MTD_OPS_RAW;

	ret = mtd_read_oob(mtd, off, &ops);
	if (ret < 0) {
		printf("[nandoobcheck] raw OOB read failed at 0x%llx (ret=%d)\n",
		       (unsigned long long)off, ret);
		free(oobbuf);
		return 1;
	}

	printf("[nandoobcheck] page 0x%llx, oobsize=%u, raw OOB (bypasses ECC/BBT):\n",
	       (unsigned long long)off, mtd->oobsize);
	for (i = 0; i < mtd->oobsize; i += 16) {
		int j, n = (mtd->oobsize - i < 16) ? mtd->oobsize - i : 16;
		printf("  %02x:", i);
		for (j = 0; j < n; j++)
			printf(" %02x", oobbuf[i + j]);
		printf("\n");
	}

	pos = chip->badblockpos;
	printf("[nandoobcheck] factory bad-block marker byte (offset %d) = 0x%02x -> %s\n",
	       pos, oobbuf[pos],
	       oobbuf[pos] == 0xFF ? "GOOD (factory-good marker)" : "BAD (factory bad marker)");

	is_bad = mtd_block_isbad(mtd, off);
	printf("[nandoobcheck] this driver's cached bad-block table says: %s\n",
	       is_bad ? "BAD" : "GOOD");

	if (oobbuf[pos] == 0xFF && is_bad) {
		printf("[nandoobcheck] MISMATCH — raw marker is factory-good but the "
		       "cached bad-block table says bad. This confirms the driver's "
		       "cached NAND_BBT_USE_FLASH table is wrong, not the chip.\n");
	} else if (oobbuf[pos] != 0xFF && !is_bad) {
		printf("[nandoobcheck] MISMATCH — raw marker is factory-bad but the "
		       "cached table says good. Unexpected either way.\n");
	} else {
		printf("[nandoobcheck] raw marker and cached table agree.\n");
	}

	free(oobbuf);
	return 0;
}

U_BOOT_CMD(
	nandoobcheck, 2, 0, do_nandoobcheck,
	"dump raw NAND OOB for a page and compare against the cached bad-block table",
	"nandoobcheck <offset-hex>\n"
	"    Reads the OOB bytes directly from flash (MTD_OPS_RAW, bypassing\n"
	"    this driver's ECC/BBT interpretation) and prints them alongside\n"
	"    what the driver's cached bad-block table currently believes about\n"
	"    that block, to check whether false bad blocks are a chip issue or\n"
	"    a stale cached table (see docs/HANDOFF_touch_and_bootargs_fix.md).\n"
);
