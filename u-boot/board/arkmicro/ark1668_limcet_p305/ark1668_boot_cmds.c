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
 *   bootstock — chainloads the ORIGINAL stock U-Boot binary (U-Boot
 *              2012.10) from an SD file (stockubootfile env var,
 *              stock_uboot.bin by default, sourced from Prado firmware
 *              dump/mtd1-mtd2_uboot/extracted/uboot.bin) and jumps into
 *              it. NOT sourced from the "U-boot" NAND partition — that
 *              was tried and reliably corrupted console output / left
 *              NAND in a bad state on real hardware (reads targeting
 *              address 0x30000 specifically; the same read to other
 *              addresses works fine). Root cause not yet found; SD
 *              avoids it and is the path confirmed working end-to-end.
 *
 *              Exists because `bootnand` above hits uncorrectable ECC
 *              errors ("err more than 8 bit") reading the kernel
 *              partition — root-caused live on real hardware (see the
 *              nand_hw_eccoob_64_2seg13b comment in drivers/mtd/nand/
 *              ark_nand.c) to this build's NAND driver using the wrong
 *              ECC layout for this chip's actual on-flash format;
 *              `switchecc 2` now fixes it (baked into `nandboot`). Even
 *              with the ECC read fixed, direct `bootnand` still hits an
 *              "undefined instr resetting" crash right at kernel entry —
 *              this stock 3.4 kernel has only ever shipped paired with
 *              the stock 2012.10 U-Boot, and jumping into it from this
 *              2018.07 fork is untested, unproven territory (ATAGS/CPU
 *              state expectations may differ between U-Boot versions).
 *              `bootstock` sidesteps that entirely by handing off to the
 *              ORIGINAL bootloader, so IT boots the kernel it was always
 *              paired with. This is a WARM handoff, not a real reset —
 *              the stock binary re-runs its own hardware init on top of
 *              whatever this build already configured, rather than
 *              starting from Stepldr's known-clean state. See
 *              docs/UBOOT_BOOTLOGO_AND_RE_PORTS.md §8.3 for the general
 *              chainload caveats this borrows from, and note the cache
 *              flush below (cleanup_before_linux()) — do_go() (cmd/
 *              boot.c) does a bare jump with no cache maintenance at
 *              all, which caused its own "undefined instr resetting"
 *              crashes here independent of the NAND issues above.
 *
 *              NOTE: if the SD card is inserted, a genuine hardware
 *              reset at ANY point after this chainload (e.g. a watchdog
 *              fire from the stock binary re-arming/feeding the
 *              watchdog differently than this build does) will still go
 *              through Stepldr, which prefers booting UBOOT.BIN from SD
 *              p1 over NAND — landing straight back in THIS build, not
 *              stock, regardless of where bootstock sourced the stock
 *              image from. Sourcing from NAND instead of SD only
 *              changes where the bytes come from for the initial jump;
 *              it does not change Stepldr's own boot-device preference
 *              on a subsequent reset. If that loop is what's actually
 *              being observed, the fix is diagnosing why the stock
 *              binary resets after being jumped into, or removing/
 *              renaming UBOOT.BIN from the SD card for that test.
 *
 * The sequencing/error-checking below stays in C (worth keeping robust —
 * this project has had a lot of debugging pain from silent failures this
 * session). The tunable values it uses — kernel/DTB filenames, root
 * device, the common bootargs — are read from env vars (with compiled-in
 * defaults set in CONFIG_EXTRA_ENV_SETTINGS), so they can be changed via
 * `setenv`/uEnv.txt without recompiling U-Boot.
 */

#include "ark1668_lcd.h"

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
	printf("bootnand: booting from NAND with original dumped settings (ubi.mtd=6 root=ubi0:rootfs)\n");
	return run_command("run nandboot", 0);
}

U_BOOT_CMD(
	bootnand, 1, 0, do_bootnand,
	"boot from NAND using the original dumped stock settings",
	"bootnand\n"
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

int do_bootstock(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[64];
	const char *stockfile = env_or_default("stockubootfile", "stock_uboot.bin");
	unsigned long magic, ep, filesize;

	/* SD file only for now, NOT the "U-boot" NAND partition. A NAND-partition
	 * read was tried here and reliably failed on real hardware — `nand read
	 * 0x30000 U-boot` (and even a plain offset/size read to that same
	 * address, no partition name involved) corrupts console output and
	 * appears to leave the NAND controller/cache in a bad state for
	 * subsequent commands. Root cause not yet found — it reproduces
	 * specifically for reads targeting 0x30000, while the exact same read
	 * to other addresses (e.g. the kernel partition to 0x1000000) works
	 * fine, so it isn't a partition-name or generic-NAND-read problem.
	 * Needs its own investigation before being trusted; SD avoids it
	 * entirely and is the path already confirmed working end-to-end. */
	sprintf(cmd, "fatload mmc 0:1 0x%x %s", STOCK_UBOOT_LOAD_ADDR, stockfile);
	if (run_command(cmd, 0) != 0) {
		printf("bootstock: fatload of %s failed — copy it to the SD "
		       "card FAT partition (see Prado firmware dump/"
		       "mtd1-mtd2_uboot/extracted/uboot.bin)\n", stockfile);
		return 1;
	}

	magic = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x3c);
	if (magic != ARK_HEADER_MAGIC) {
		printf("bootstock: bad ARK header magic 0x%lx (expected 0x%x) — "
		       "refusing to jump into garbage\n", magic, ARK_HEADER_MAGIC);
		return 1;
	}

	filesize = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x50);
	ep = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x44);
	printf("bootstock: header OK, entry point 0x%lx — flushing caches and "
	       "jumping now (warm handoff, watch serial closely)\n", ep);

	/* do_go() (cmd/boot.c) does a bare jump with NO cache maintenance at
	 * all — unlike bootm/bootz (used by bootmmc/bootusb), which call
	 * cleanup_before_linux() internally before handing off. Without any
	 * cache maintenance, stale/dirty icache lines from whatever ran
	 * before this command can survive under the freshly-loaded code and
	 * get executed instead of it.
	 *
	 * Deliberately NOT using cleanup_before_linux() here, even though
	 * that's what bootm/bootz call — it's built for handing off to a
	 * KERNEL, which does its own from-scratch MMU/interrupt/cache init.
	 * It disables the MMU, interrupts, and both caches entirely. This is
	 * a bootloader-to-bootloader handoff via the same EP-jump entry
	 * Stepldr normally uses, and Stepldr's handoff is what the stock
	 * binary actually expects/was built for — Stepldr's own DDR init
	 * almost certainly leaves the MMU/caches in some usable state, not
	 * fully torn down, and the stock binary may not expect the harder
	 * teardown. (An earlier version of this fix used
	 * cleanup_before_linux() and was suspected of causing its own
	 * immediate crash-and-reset back through Stepldr into THIS build,
	 * separate from the original stale-icache crash it was meant to
	 * fix.) Just flush the range we actually wrote and invalidate the
	 * icache globally — enough to guarantee the CPU executes what was
	 * just loaded, without touching MMU/interrupt state. */
	flush_cache(STOCK_UBOOT_LOAD_ADDR, filesize);
	invalidate_icache_all();

	sprintf(cmd, "go 0x%lx", ep);
	return run_command(cmd, 0);
}

U_BOOT_CMD(
	bootstock, 1, 0, do_bootstock,
	"chainload the original stock dumped U-Boot from the SD card (bypasses this build's NAND driver)",
	"bootstock\n"
);

static int boot_from_block_dev(const char *iface)
{
	char cmd[192];
	unsigned long kerneladdr = env_or_default_hex("kerneladdr", 0x1000000);
	unsigned long dtbaddr = env_or_default_hex("dtbaddr", 0x2000000);
	const char *kernelfile = env_or_default("kernelfile", "zImage");
	const char *dtbfile = env_or_default("dtbfile", "ark1668_limcet_p305.dtb");
	const char *mmcroot = env_or_default("mmcroot", "/dev/mmcblk0p2");
	const char *bootargs_common = env_or_default("bootargs_common",
		"console=ttyS0,115200n8 mem=180M earlyprintk=serial rootfstype=ext4 rootwait rw screen=0 user_debug=8");

	sprintf(cmd, "setenv bootargs root=%s %s", mmcroot, bootargs_common);
	run_command(cmd, 0);

	sprintf(cmd, "fatload %s 0:1 0x%lx %s", iface, kerneladdr, kernelfile);
	if (run_command(cmd, 0) != 0) {
		printf("boot%s: failed to load %s from %s 0:1\n", iface, kernelfile, iface);
		return 1;
	}

	sprintf(cmd, "fatload %s 0:1 0x%lx %s", iface, dtbaddr, dtbfile);
	if (run_command(cmd, 0) != 0) {
		printf("boot%s: failed to load %s from %s 0:1\n", iface, dtbfile, iface);
		return 1;
	}

	sprintf(cmd, "bootz 0x%lx - 0x%lx", kerneladdr, dtbaddr);
	return run_command(cmd, 0);
}

int do_bootmmc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("bootmmc: booting kernel+DTB from SD card (mmc 0:1), rootfs on %s\n",
	       env_or_default("mmcroot", "/dev/mmcblk0p2"));
	return boot_from_block_dev("mmc");
}

U_BOOT_CMD(
	bootmmc, 1, 0, do_bootmmc,
	"boot kernel+DTB from the SD card (current default boot path)",
	"bootmmc\n"
);

int do_bootusb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("bootusb: starting USB...\n");
	if (run_command("usb start", 0) != 0) {
		printf("bootusb: usb start failed\n");
		return 1;
	}
	printf("bootusb: booting kernel+DTB from USB stick (usb 0:1), rootfs still on %s\n",
	       env_or_default("mmcroot", "/dev/mmcblk0p2"));
	return boot_from_block_dev("usb");
}

U_BOOT_CMD(
	bootusb, 1, 0, do_bootusb,
	"boot kernel+DTB from a USB stick (same as bootmmc, rootfs unchanged)",
	"bootusb\n"
);
