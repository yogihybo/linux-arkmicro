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

static int bootstock_file_from_block_dev(const char *iface, const char *env_var, const char *default_file)
{
	char cmd[64];
	const char *stockfile = env_or_default(env_var, default_file);
	unsigned long magic, ep, filesize;

	/* File on a FAT block device only, NOT the "U-boot" NAND partition. */
	sprintf(cmd, "fatload %s 0:1 0x%x %s", iface, STOCK_UBOOT_LOAD_ADDR, stockfile);
	if (run_command(cmd, 0) != 0) {
		printf("[chainload] fatload of %s from %s 0:1 failed — copy it to "
		       "the %s FAT partition\n", stockfile, iface, iface);
		return 1;
	}

	magic = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x3c);
	if (magic != ARK_HEADER_MAGIC) {
		printf("[chainload] bad ARK header magic 0x%lx (expected 0x%x) — "
		       "refusing to jump into garbage\n", magic, ARK_HEADER_MAGIC);
		return 1;
	}

	filesize = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x50);
	ep = *(volatile unsigned long *)(STOCK_UBOOT_LOAD_ADDR + 0x44);
	printf("[chainload] header OK (header EP 0x%lx, unused), "
	       "flushing caches and jumping to 0x%x now (warm handoff, watch "
	       "serial closely)\n", ep, STOCK_UBOOT_LOAD_ADDR);

	flush_cache(STOCK_UBOOT_LOAD_ADDR, filesize);
	invalidate_icache_all();

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

	/* If the kernel/DTB were fatload'd from USB, U-Boot's own musb-hdrc
	 * driver is left mid-enumeration on the controller. Without stopping
	 * it here, the kernel's musb-hdrc probe hangs trying to take over a
	 * controller that was never quiesced — cleanup_before_linux() (called
	 * inside bootz below) resets CPU/cache/MMU state but knows nothing
	 * about USB hardware state. Only applies to the "usb" iface; mmc never
	 * touches the USB controller so there's nothing to stop there. */
	if (strcmp(iface, "usb") == 0)
		run_command("usb stop", 0);

	sprintf(cmd, "bootz 0x%lx - 0x%lx", kerneladdr, dtbaddr);
	return run_command(cmd, 0);
}

int do_bootmmc(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("[bootmmc] booting kernel+DTB from SD card (mmc 0:1), rootfs on %s\n",
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
	printf("[bootusb] starting USB...\n");
	if (run_command("usb start", 0) != 0) {
		printf("[bootusb] usb start failed\n");
		return 1;
	}
	printf("[bootusb] booting kernel+DTB+rootfs from USB stick (usb 0:1), root=%s\n",
	       env_or_default("usbroot", "/dev/sda2"));
	return boot_from_block_dev("usb");
}

U_BOOT_CMD(
	bootusb, 1, 0, do_bootusb,
	"boot kernel+DTB from a USB stick (same as bootmmc, rootfs unchanged)",
	"bootusb\n"
);

int do_bootstockkernel(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[192];
	unsigned long kerneladdr = env_or_default_hex("kerneladdr", 0x1000000);
	const char *kernelfile = env_or_default("stockkernelfile", "zImage_stock");
	const char *bootargs_common = env_or_default("bootargs_common",
		"console=ttyS0,115200n8 mem=180M earlyprintk=serial ubi.mtd=6 root=ubi0:rootfs rootfstype=ubifs rootwait ro screen=0 user_debug=8");
	unsigned long machid = env_or_default_hex("machid", 0x1068);

	printf("[bootstockkernel] booting stock 3.4 kernel via ATAGS from mmc 0:1 (%s)...\n", kernelfile);

	sprintf(cmd, "setenv bootargs %s", bootargs_common);
	run_command(cmd, 0);

	sprintf(cmd, "setenv machid 0x%lx", machid);
	run_command(cmd, 0);

	sprintf(cmd, "fatload mmc 0:1 0x%lx %s", kerneladdr, kernelfile);
	if (run_command(cmd, 0) != 0) {
		printf("[bootstockkernel] failed to load %s from mmc 0:1\n", kernelfile);
		return 1;
	}

	/* 2-argument bootz passes ATAGS (not DTB) to stock 3.4 kernel */
	sprintf(cmd, "bootz 0x%lx", kerneladdr);
	return run_command(cmd, 0);
}

U_BOOT_CMD(
	bootstockkernel, 1, 0, do_bootstockkernel,
	"boot stock 3.4 kernel directly from SD card using legacy ATAGS (no DTB)",
	"bootstockkernel\n"
);

