/*
 * Three explicit, named boot paths — one per boot medium — so any of them
 * can be picked directly at the prompt instead of relying on autoboot's
 * single CONFIG_BOOTCOMMAND flow.
 *
 *   bootnand — kernel + rootfs exactly as stock shipped them, straight
 *              from NAND. Just wraps the existing `nandboot`/`nandargs`
 *              env scripts (see NANDARGS in
 *              include/configs/ark1668_limcet_p305.h) — that logic
 *              already matches the original dumped NAND settings
 *              (ubi.mtd=6 root=ubi0:rootfs rootfstype=ubifs, real
 *              mtdparts layout — cross-checked against the real dumped
 *              env, Prado firmware dump/mtd3_env/extracted/uboot-env.txt),
 *              nothing new to get right here.
 *   bootmmc  — the current SD-card boot path (kernel/DTB from mmc 0:1,
 *              rootfs on mmcblk0p2), matching what this project's
 *              build_bootable_sdcard.sh sets up.
 *   bootusb  — identical to bootmmc, except the kernel/DTB are fatload'd
 *              from the USB stick instead of the SD card. Rootfs stays
 *              on the SD card (bootargs unchanged from bootmmc) — this
 *              is deliberately the "iterate on kernel/DTB from a USB
 *              stick without touching the SD card" workflow discussed in
 *              docs/UBOOT_BOOTLOGO_AND_RE_PORTS.md §8.2, not a full
 *              USB-hosted rootfs.
 *   bootstock / bootstockusb — chainloads the ORIGINAL stock U-Boot binary
 *              (U-Boot 2012.10) from a FAT file on SD or USB respectively
 *              (stockubootfile env var, uboot_stock.bin by default,
 *              sourced from Prado firmware dump/mtd1-mtd2_uboot/extracted/
 *              uboot.bin) and jumps into it, which then boots the stock
 *              kernel+rootfs+UI from NAND with its own NAND driver.
 *              CONFIRMED WORKING END-TO-END on real hardware
 *              (2026-07-13) — this is the reliable path to a working
 *              stock NAND boot from this fork.
 *
 *              NOT sourced from the "U-boot" NAND partition — every ECC
 *              scheme tried (ours and stock's own native switchecc) fails
 *              to read it; proven via Stepldr disassembly (see
 *              docs/HANDOFF_nand_ecc_uboot_vs_kernel.md §3) that Stepldr
 *              reads it via a raw, BCH_CR-free path no U-Boot-level tool
 *              replicates. Not a bug — don't revisit NAND-sourcing here.
 *
 *              Exists because `bootnand` above, even with its NAND ECC
 *              issue fixed (`switchecc 2`, see
 *              docs/HANDOFF_nand_ecc_uboot_vs_kernel.md §1) and machid
 *              set correctly, still hangs silently at kernel entry — this
 *              stock 3.4 kernel has only ever shipped paired with the
 *              stock 2012.10 U-Boot, and jumping into it from this
 *              2018.07 fork's bootz is untested, unproven territory.
 *              `bootstock` sidesteps that entirely by handing the kernel
 *              boot to the binary it was actually built against.
 *
 *              This is a WARM handoff, not a real reset — the stock
 *              binary re-runs its own hardware init on top of whatever
 *              this build already configured, rather than starting from
 *              Stepldr's known-clean state. Two real bugs were found and
 *              fixed here to get this working:
 *              1. Cache maintenance before the jump: do_go() (cmd/boot.c)
 *                 does a bare jump with NO cache maintenance at all.
 *                 cleanup_before_linux() (what bootm/bootz use) is too
 *                 aggressive for a bootloader-to-bootloader handoff — it's
 *                 built for kernel handoff and fully disables MMU/
 *                 interrupts/both caches, which the stock binary may not
 *                 expect from this entry path. Uses flush_cache() +
 *                 invalidate_icache_all() instead, narrow enough to just
 *                 guarantee the CPU executes what was actually loaded.
 *              2. Jump target: was jumping to the ARK header's EP field.
 *                 Verified via objdump disassembly of the real
 *                 Stepldr.bin that Stepldr's own load routine hardcodes
 *                 `mov r0, #0x30000; blx r0` — it jumps to the LOAD
 *                 ADDRESS (reset vector / _start), completely ignoring
 *                 the header's EP. See docs/UBOOT_BOOTLOGO_AND_RE_PORTS.md
 *                 §8.3 for the correction. This was the fix that made it
 *                 reliable — jumping to EP was skipping required
 *                 _start/vector-table setup, causing intermittent
 *                 "undefined instr resetting" crashes.
 *
 * The sequencing/error-checking below stays in C (worth keeping robust —
 * this project has had a lot of debugging pain from silent failures this
 * session). The tunable values it uses — kernel/DTB filenames, root
 * device, the common bootargs — are read from env vars (with compiled-in
 * defaults set in CONFIG_EXTRA_ENV_SETTINGS), so they can be changed via
 * `setenv`/uEnv.txt without recompiling U-Boot.
 */

#include "ark1668_lcd.h"

/* 2026-07-31: watchdog block already used by reset_cpu() (see
 * arch/arm/mach-arkmicro/armv7/reset.c) reused here to arm a real
 * boot-supervision timeout right before jumping into THIS PROJECT'S OWN
 * kernel in boot_from_block_dev() below. The kernel's own ark_wdt driver
 * reprograms the timer to its own default (15s) as soon as it probes
 * (clean stop+reprogram, no register conflict) -- every boot log checked
 * shows that happening within ~1s of the jump, so 20s gives a large,
 * safe margin against a false trigger during a legitimately-successful
 * boot.
 *
 * Originally deliberately NOT used anywhere near do_bootnand()/
 * `nandboot` or bootstock/bootstockusb/boothybrid below, on the theory
 * that those boot STOCK's original, untouched kernel+rootfs (or a fully
 * separate chainloaded stock U-Boot), which has no knowledge of our
 * busybox watchdog feeder -- arming it there could fire mid-boot on a
 * currently-working path.
 *
 * REVERSED for do_bootnand() specifically (2026-07-31, same day):
 * hardware testing found `bootnand` itself can hang, with no recovery
 * short of a manual power-cycle -- clearly worse than the theoretical
 * regression risk above. Accepted trade-off, not fully eliminated risk:
 * stock's own kernel almost certainly has the same historically-dormant
 * ark_wdt driver lineage ours had before this session's soft_noboot fix
 * (auto-starts at probe, defaults to interrupt-only/self-healing mode)
 * -- if so, stock's own driver probing within the armed window will
 * silently neutralize our timer the same way ours used to, and this is
 * effectively risk-free. If stock's kernel *doesn't* reach that probe
 * for some reason, a long-running stock boot could theoretically still
 * get an unexpected reset once the timeout elapses unfed -- not
 * confirmed either way, flagging honestly rather than asserting this is
 * fully safe. Still NOT applied to bootstock/bootstockusb/boothybrid
 * (the fully-chainloaded-stock-U-Boot paths) -- no hang reported there,
 * don't touch what isn't broken. */
#define KERNEL_HANDOFF_WDT_MS	20000
#define NANDBOOT_WDT_MS		30000
extern void ark_wdt_arm(unsigned int timeout_ms);

static const char *env_or_default(const char *name, const char *fallback)
{
	const char *v = env_get(name);
	return v ? v : fallback;
}

static unsigned long env_or_default_hex(const char *name, unsigned long fallback)
{
	const char *v = env_get(name);
	return v ? simple_strtoul(v, NULL, 16) : fallback;
}

int do_bootnand(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("[bootnand] booting from NAND with original dumped settings (ubi.mtd=6 root=ubi0:rootfs)\n");

	/* 2026-07-31: clear bootcheck's boot-attempt counter here, before
	 * running nandboot below. U-Boot successfully reaching this point
	 * (not hung/crashed earlier in its own init or in a failed bootusb
	 * attempt) is itself evidence U-Boot/hardware is fundamentally
	 * healthy -- that's the actual thing bootcount tracks, independent
	 * of whether bootusb's kernel image specifically was good or bad.
	 * Without this, bootcount would only ever get cleared by our own
	 * rcS (which never runs under nandboot -- see the comment below),
	 * so once it exceeded bootlimit the device would be PERMANENTLY
	 * stuck skipping bootusb forever, even after whatever was wrong
	 * with it got fixed. Deliberately done here in U-Boot itself, not
	 * by patching stock's NAND-resident rootfs (which this project has
	 * never written to and treats as the guaranteed-untouched, known-
	 * good fallback -- see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
	 * §79). */
	env_set_ulong("bootcount", 0);
	env_save();

	/* 2026-07-31: arm the watchdog here too -- see the "REVERSED" note
	 * above KERNEL_HANDOFF_WDT_MS/NANDBOOT_WDT_MS for why. 30s (vs 20s
	 * for boot_from_block_dev) since this path does its own NAND I/O
	 * (disconfig, reversingtrack read, kernel read) before ever reaching
	 * bootz, not just a straight fatload+bootz on already-loaded data. */
	ark_wdt_arm(NANDBOOT_WDT_MS);

	/* nandboot itself calls bootlogofile, positioned after its own
	 * disconfig 0 (which resets OSD1 layer state via ark_display_init()
	 * and would otherwise wipe out a bootlogofile call made here first). */
	return run_command("run nandboot", 0);
}

U_BOOT_CMD(
	bootnand, 1, 0, do_bootnand,
	"boot from NAND using the original dumped stock settings",
	"bootnand\n"
);

/* 2026-07-31: boot-attempt counter for the default autoboot chain only
 * (CONFIG_BOOTCOMMAND calls this, not do_bootmmc/do_bootusb/do_bootnand
 * themselves -- manually typing bootusb/bootmmc/bootnand at the prompt
 * never touches bootcount at all, matching this file's existing
 * "commands stay independently invokable" design). Persisted in the
 * environment (NAND-backed, /dev/mtd3, survives a real power-cycle) --
 * plain env_get_ulong/env_set_ulong/env_save rather than the generic
 * drivers/bootcount/* framework, which isn't wired into board_r.c for
 * this board anyway (would need the same explicit calls regardless) and
 * whose one persistent backend (bootcount_env.c) is gated behind an
 * upgrade_available flag meant for a different A/B-OTA use case. */
int do_bootcheck(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong bootcount = env_get_ulong("bootcount", 10, 0);
	ulong bootlimit = env_get_ulong("bootlimit", 10, 3); /* default 3 */

	bootcount++;
	env_set_ulong("bootcount", bootcount);
	env_save();

	printf("[bootcheck] bootcount=%lu bootlimit=%lu\n", bootcount, bootlimit);

	if (bootlimit && bootcount > bootlimit) {
		printf("[bootcheck] %lu consecutive unconfirmed boots (limit %lu) "
		       "-- falling back to nandboot directly\n", bootcount, bootlimit);
		return 1;
	}
	return 0;
}

U_BOOT_CMD(
	bootcheck, 1, 0, do_bootcheck,
	"increment/check the boot-attempt counter used by the default autoboot chain",
	"bootcheck\n"
);

/* Fixed by the ARK header format (see inject_ark_header.py / docs) — Stepldr
 * always loads a headered U-Boot image to this address, and the stock
 * binary's own header (checked against the real dump) says the same. This
 * is safe to overwrite here: by the time bootstock runs, this build has
 * long since relocated itself to high RAM (see bdinfo), so nothing is still
 * executing out of the low copy at this address. */
#define STOCK_UBOOT_LOAD_ADDR	0x30000
#define ARK_HEADER_MAGIC	0x12345678

extern int cleanup_before_linux(void);

static int bootstock_file_from_block_dev(const char *iface, const char *env_var, const char *default_file)
{
	char cmd[64];
	const char *stockfile = env_or_default(env_var, default_file);
	unsigned long magic, ep, filesize;
	int attempt;

	/*
	 * File on a FAT block device only, NOT the "U-boot" NAND partition.
	 * Retried a few times on a bad ARK header magic: this check has been
	 * seen to intermittently fail ("comes and goes" across otherwise
	 * identical boots), which fits a transient/marginal SD or USB read
	 * corrupting part of the loaded binary far better than a
	 * deterministic logic bug -- a single bad `fatload` shouldn't be
	 * treated as a hard failure when a clean re-read is cheap and the
	 * alternative is falling all the way back to `nandboot`.
	 */
	for (attempt = 1; attempt <= 3; attempt++) {
		sprintf(cmd, "fatload %s 0:1 0x%x %s", iface, STOCK_UBOOT_LOAD_ADDR, stockfile);
		if (run_command(cmd, 0) != 0) {
			printf("[chainload] fatload of %s from %s 0:1 failed — copy it to "
			       "the %s FAT partition\n", stockfile, iface, iface);
			return 1;
		}

		magic = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x3c);
		if (magic == ARK_HEADER_MAGIC)
			break;

		printf("[chainload] bad ARK header magic 0x%lx (expected 0x%x) on "
		       "attempt %d/3", magic, ARK_HEADER_MAGIC, attempt);
		if (attempt == 3) {
			printf(" — giving up, refusing to jump into garbage\n");
			return 1;
		}
		printf(" — retrying fatload\n");
	}

	filesize = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x50);
	ep = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x44);
	printf("[chainload] header OK (header EP 0x%lx, unused), "
	       "flushing caches and jumping to 0x%x now (warm handoff, watch "
	       "serial closely)\n", ep, STOCK_UBOOT_LOAD_ADDR);

	flush_cache(STOCK_UBOOT_LOAD_ADDR, filesize);
	invalidate_icache_all();

	/*
	 * Wait for the NAND/BCH controller's own FSM to reach idle before
	 * zeroing anything below. This build's own driver (ark_nand.c)
	 * always does this same poll (rBCH_NAND_STATUS bits [5:0] == 0)
	 * after every real NAND transaction before touching these same
	 * control registers -- reused here defensively, since whatever
	 * NAND op ran last (kernel/arkdata/reservingtrack load, switchecc,
	 * etc, run earlier in this build's own boot sequence -- note
	 * `bootstock`/`boothybrid` are reached automatically via the
	 * default CONFIG_BOOTCOMMAND fallback chain on every cold boot,
	 * not typed by hand, so any variance here is real hardware/timing
	 * marginality rather than user-command variability) might not have
	 * fully drained yet. Zeroing BCH_CR/
	 * NAND_CR out from under an in-flight transaction would leave the
	 * controller in a genuinely undefined state for stock U-Boot's own
	 * NAND driver to inherit -- a plausible explanation for
	 * intermittent (not 100%-reproducible) "chainloads fine, then
	 * fails to boot the stock kernel" reports, since a fixed,
	 * always-present gap would be expected to fail consistently
	 * instead. Bounded with a timeout (rather than looping forever
	 * like the driver's own wait) so a genuinely wedged controller
	 * doesn't hang the chainload silently -- prints a warning and
	 * proceeds with the reset anyway rather than blocking forever.
	 */
	{
		int timeout = 100000;
		while (((*(volatile unsigned int *)(0xec000000 + 0x280)) & 0x3F) != 0
		       && timeout--)
			;
		if (timeout <= 0)
			printf("[chainload] warning: NAND/BCH FSM did not reach idle "
			       "(BCH_NAND_STATUS=0x%08x) -- proceeding with reset anyway\n",
			       *(volatile unsigned int *)(0xec000000 + 0x280));
	}

	printf("[chainload] resetting NAND/BCH controller registers (was BCH_CR=0x%08x)\n",
	       *(volatile unsigned int *)(0xec000000 + 0x27c));

	*(volatile unsigned int *)(0xec000000 + 0x27c) = 0;          /* rBCH_CR */
	*(volatile unsigned int *)(0xec000000 + 0x288) = 0x0000000f; /* Clear pending BCH interrupts */
	*(volatile unsigned int *)(0xec000000 + 0x28c) = 0;          /* Mask BCH interrupts */
	*(volatile unsigned int *)(0xec000000 + 0x290) = 0;          /* rNAND_DMA_CTRL */
	*(volatile unsigned int *)(0xec000000 + 0x294) = 0;          /* rNAND_GLOBAL_CTL */
	*(volatile unsigned int *)(0xec000000 + 0x298) = 0;          /* rNAND_JUMP_CTL */
	*(volatile unsigned int *)(0xec000000 + 0x00) = 0;           /* rNAND_CR */

	sprintf(cmd, "go 0x%x", STOCK_UBOOT_LOAD_ADDR);
	return run_command(cmd, 0);
}

int do_bootstock(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return bootstock_file_from_block_dev("mmc", "stockubootfile", "uboot_stock.bin");
}

U_BOOT_CMD(
	bootstock, 1, 0, do_bootstock,
	"chainload the original stock dumped U-Boot from the SD card (bypasses this build's NAND driver)",
	"bootstock\n"
);

int do_boothybrid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return bootstock_file_from_block_dev("mmc", "hybridubootfile", "uboot_hybrid.bin");
}

U_BOOT_CMD(
	boothybrid, 1, 0, do_boothybrid,
	"chainload the patched hybrid U-Boot (uboot_hybrid.bin) from the SD card",
	"boothybrid\n"
);

int do_bootstockusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("[bootstockusb] starting USB...\n");
	if (run_command("usb start", 0) != 0) {
		printf("[bootstockusb] usb start failed\n");
		return 1;
	}
	return bootstock_file_from_block_dev("usb", "stockubootfile", "uboot_stock.bin");
}

U_BOOT_CMD(
	bootstockusb, 1, 0, do_bootstockusb,
	"chainload the original stock dumped U-Boot from a USB stick (same as bootstock, different source)",
	"bootstockusb\n"
);

static int boot_from_block_dev(const char *iface)
{
	char cmd[192];
	unsigned long kerneladdr = env_or_default_hex("kerneladdr", 0x1000000);
	unsigned long dtbaddr = env_or_default_hex("dtbaddr", 0x2000000);
	const char *kernelfile = env_or_default("kernelfile", "zImage");
	const char *dtbfile = env_or_default("dtbfile", "ark1668_limcet_p305.dtb");
	const char *mmcroot = env_or_default("mmcroot", "/dev/mmcblk0p2");
	const char *usbroot = env_or_default("usbroot", "/dev/sda2");
	const char *root = (strcmp(iface, "usb") == 0) ? usbroot : mmcroot;
	const char *bootargs_common = env_or_default("bootargs_common",
		"console=ttyS0,115200n8 mem=180M earlyprintk=serial rootfstype=ext4 rootwait rw screen=0 user_debug=8");
	unsigned long machid = env_or_default_hex("machid", 0x1068);

	/* bootusb previously always used mmcroot here regardless of iface —
	 * loaded the kernel from USB but still told it to mount root from the
	 * SD card. root= now follows the actual boot device. Note this only
	 * covers the kernel's root filesystem; whether /data (userdata) also
	 * ends up on USB depends on how the rootfs's own init script (rcS)
	 * derives that partition — that's a rootfs-level concern, not
	 * something U-Boot's bootargs alone control. Check/patch rcS
	 * separately if userdata needs to follow root onto USB too. */
	sprintf(cmd, "setenv bootargs root=%s %s", root, bootargs_common);
	run_command(cmd, 0);

	/* Same fix as nandboot's machid — ARK1680's machine ID, needed even
	 * on the DT boot path (bootz still checks it before the kernel gets
	 * far enough to fall back on the DTB's compatible string). Without
	 * it: "Error: unrecognized/unsupported machine ID (r1 = 0x00000000)". */
	sprintf(cmd, "setenv machid 0x%lx", machid);
	run_command(cmd, 0);

	sprintf(cmd, "fatload %s 0:1 0x%lx %s", iface, kerneladdr, kernelfile);
	if (run_command(cmd, 0) != 0) {
		printf("[boot%s] failed to load %s from %s 0:1\n", iface, kernelfile, iface);
		return 1;
	}

	sprintf(cmd, "fatload %s 0:1 0x%lx %s", iface, dtbaddr, dtbfile);
	if (run_command(cmd, 0) != 0) {
		printf("[boot%s] failed to load %s from %s 0:1\n", iface, dtbfile, iface);
		return 1;
	}

	/* 2026-07-31: reverse-camera parking-guide overlay data (RSTK-magic'd,
	 * kernel's track_paint_init() checks for it at fixed physical
	 * 0xfd00000 -- see NANDARGS's nandboot comment in
	 * include/configs/ark1668_limcet_p305.h). Previously only ever loaded
	 * by nandboot's own `nand read`; this path (bootmmc/bootusb) never
	 * loaded it at all, so the guideline overlay silently never worked
	 * here even though these are now the primary confirmed-working boot
	 * paths. Always sourced from the SD card's FAT partition (mmc 0:1)
	 * regardless of iface, same convention display_bootlogo_file() already
	 * uses for its own assets -- even a bootusb test boot still has the SD
	 * card mounted for auxiliary, non-kernel assets like this. Not fatal
	 * if missing/fails: the kernel's own check already degrades gracefully
	 * (prints "reservingtrack check failed!", skips the overlay) if this
	 * memory region was never populated, so a warning is enough here. */
	if (run_command("fatload mmc 0:1 0xfd00000 reversingtrack.raw", 0) != 0)
		printf("[boot%s] warning: failed to load reversingtrack.raw -- "
		       "reverse-camera guideline overlay will be unavailable\n", iface);

	/* usb0 (the board's single external USB port) ships with
	 * dr_mode="otg" in the DTS so bootmmc (the real vehicle boot path)
	 * keeps real gadget capability for wired CarPlay. bootusb already
	 * knows for certain no wired CarPlay cable is in the picture --
	 * the whole point of that path is booting off a USB stick plugged
	 * into that same port -- so patch the in-RAM DTB to force
	 * dr_mode="host" here instead, skipping the several seconds of
	 * ID-pin negotiation retries "otg" costs at every boot. Patching
	 * the loaded DTB (not the DTS default) keeps this a boot-command
	 * -local decision instead of a compile-time one -- see
	 * linux-arkmicro's ark1668.dtsi usb0 node comment. */
	if (strcmp(iface, "usb") == 0) {
		sprintf(cmd, "fdt addr 0x%lx", dtbaddr);
		run_command(cmd, 0);
		/* The loaded blob has zero slack space (fatload gives it its
		 * exact on-disk size) -- fdt_setprop() fails FDT_ERR_NOSPACE
		 * growing dr_mode from "otg" (4 bytes incl NUL) to "host" (5
		 * bytes), even though that's a trivially small grow. `fdt
		 * resize` pads the working copy first. */
		run_command("fdt resize 64", 0);
		if (run_command("fdt set /ahb/usb@E0100000 dr_mode \"host\"", 0) != 0)
			printf("[bootusb] warning: failed to force usb0 dr_mode=host in DTB, "
			       "keeping DTS default (otg) -- boot will be slower but should still work\n");
	}

	/* If the kernel/DTB were fatload'd from USB, U-Boot's own musb-hdrc
	 * driver is left mid-enumeration on the controller. Without stopping
	 * it here, the kernel's musb-hdrc probe hangs trying to take over a
	 * controller that was never quiesced — cleanup_before_linux() (called
	 * inside bootz below) resets CPU/cache/MMU state but knows nothing
	 * about USB hardware state. Only applies to the "usb" iface; mmc never
	 * touches the USB controller so there's nothing to stop there. */
	if (strcmp(iface, "usb") == 0)
		run_command("usb stop", 0);

	/* Arm the hardware watchdog right before the jump, after everything
	 * above has already succeeded -- see the comment above
	 * KERNEL_HANDOFF_WDT_MS for why this is safe and why it's only here,
	 * not in do_bootnand()/bootstock. Catches a hung/failed jump (bad
	 * kernel image, bad DTB, early kernel panic before our own ark_wdt
	 * driver probes) by auto-resetting back to U-Boot instead of hanging
	 * forever. */
	ark_wdt_arm(KERNEL_HANDOFF_WDT_MS);

	sprintf(cmd, "bootz 0x%lx - 0x%lx", kerneladdr, dtbaddr);
	return run_command(cmd, 0);
}

int do_bootmmc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("[bootmmc] booting kernel+DTB from SD card (mmc 0:1), rootfs on %s\n",
	       env_or_default("mmcroot", "/dev/mmcblk0p2"));
	run_command("bootlogofile bootlogo_sd.raw", 0);
	return boot_from_block_dev("mmc");
}

U_BOOT_CMD(
	bootmmc, 1, 0, do_bootmmc,
	"boot kernel+DTB from the SD card (current default boot path)",
	"bootmmc\n"
);

int do_bootusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("[bootusb] starting USB...\n");
	if (run_command("usb start", 0) != 0) {
		printf("[bootusb] usb start failed\n");
		return 1;
	}
	printf("[bootusb] booting kernel+DTB+rootfs from USB stick (usb 0:1), root=%s\n",
	       env_or_default("usbroot", "/dev/sda2"));
	run_command("bootlogofile bootlogo_usb.raw", 0);
	return boot_from_block_dev("usb");
}

U_BOOT_CMD(
	bootusb, 1, 0, do_bootusb,
	"boot kernel+DTB from a USB stick (same as bootmmc, rootfs unchanged)",
	"bootusb\n"
);

