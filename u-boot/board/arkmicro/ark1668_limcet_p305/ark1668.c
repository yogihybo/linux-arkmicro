#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <debug_uart.h>
#include <asm-generic/gpio.h>
#include <asm/arch/ark-common.h>
#include "ark1668_lcd.h"

DECLARE_GLOBAL_DATA_PTR;

#define ARK1668_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"

#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0xe4900058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0xe490005c))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0xe4900074))
#define rSYS_SOFT_RSTNB			*((volatile unsigned int *)(0xe4900078))
#define rSYS_DDR_STATUS			*((volatile unsigned int *)(0xe4900180))
#define rSYS_PAD_CTRL05			*((volatile unsigned int *)(0xe49001d4))
#define rSYS_PAD_CTRL06			*((volatile unsigned int *)(0xe49001d8))
#define rSYS_PAD_CTRL08			*((volatile unsigned int *)(0xe49001e0))
#define rSYS_PAD_CTRL0B			*((volatile unsigned int *)(0xe49001ec))

#define rREMAP					(*(volatile unsigned int *)(0xe4400020))

static void dwmci_select_pad(void)
{
	unsigned int val;
	/* use sd/mmc 0 */
	val = rSYS_PAD_CTRL0B;
	val &= ~((0xF << 0) | (0x1 << 4));
	val |= ((0xF << 0) | (0x1 << 4));
	rSYS_PAD_CTRL0B = val;

	val = rSYS_PAD_CTRL05;
	val &= ~((0xF << 24) | (0xF << 28));
	val |= ((0x2 << 24) | (0x2 << 28));
	rSYS_PAD_CTRL05 = val;

	rSYS_SD_CLK_CFG = 0x00000420;

	/* use sd/mmc 1 */
	val = rSYS_PAD_CTRL06;
	val &= ~0x2222222;
	val |= 0x2222222;
	rSYS_PAD_CTRL06 = val;

	rSYS_SD1_CLK_CFG = 0x00000420;
}

static void dwmci_reset(void)
{
	rSYS_SOFT_RSTNA &= ~((1 << 29) | (1 << 16));
	rSYS_SOFT_RSTNB &= ~(1 << 3);
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1 << 29) | (1 << 16));
	rSYS_SOFT_RSTNB |= (1 << 3);
}

#define ARK_MMC_CLK     	24000000
int ark_dwmci_init(char *name, u32 regbase, int bus_width, int index)
{
	struct dwmci_host *host = NULL;
	host = malloc(sizeof(struct dwmci_host));
	if (!host) {
		printf("[dwmci] host malloc fail!\n");
		return 1;
	}
	memset(host, 0, sizeof(struct dwmci_host));

	dwmci_select_pad();
	dwmci_reset();

	host->name = name;
	host->ioaddr = (void *)regbase;
	host->buswidth = bus_width;
	host->dev_index = index;
	host->bus_hz = ARK_MMC_CLK;
	host->fifo_mode = 1;

	add_dwmci(host, host->bus_hz, 400000);

	return 0;
}

int board_mmc_init(bd_t * bis)
{
	ark_dwmci_init("ARK_MMC0", 0xec400000, 4, 0);
//	ark_dwmci_init("ARK_MMC1", 0xec800000, 4, 0);
	//ark_dwmci_init("ARK_MMC2",SDHC2_BASE, 4, 2);

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE, CONFIG_SYS_SDRAM_SIZE);

	return 0;
}

int board_init(void)
{
	/* gpio64 ouput high */
	rSYS_PAD_CTRL08 &= ~(0x3 << 4);
	gpio_direction_output(64, 1);

	return 0;
}

#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
#ifdef CONFIG_DEBUG_UART
	debug_uart_init();
#endif
	return 0;
}
#endif

/* Build-time A/B toggle for the LCD console (mirrors serial output onto
 * the screen — see ark1668_lcd_console.c).
 *
 * DISABLED (0) — confirmed by A/B testing on real hardware to cause
 * genuine data corruption in the serial output (not just a display
 * artifact): after enabling lcdconsole, `help`/other long command output
 * came back with garbled command names/text, in some cases containing
 * intact fragments of unrelated strings (e.g. part of the boot banner
 * showing up inside another command's help text) — a classic signature
 * of memory corruption, not a rendering bug.
 *
 * Ruled out during investigation (kept here so this isn't re-litigated
 * from scratch later):
 *   - The framebuffer address itself: confirmed via `bdinfo` that
 *     CONSOLE_FB_ADDR (0x2500000) sits ~18.7MB clear of U-Boot's actual
 *     relocation address/malloc arena/stack (all ~62-64MB), so it isn't
 *     colliding with U-Boot's own runtime memory.
 *   - stdio_register() storing a dangling pointer to a stack-local
 *     struct: checked common/stdio.c — stdio_clone() does a proper
 *     calloc()+memcpy(), so the registered stdio_dev is a real heap copy,
 *     not a stale stack reference.
 *   - Pixel/scroll bounds math in ark1668_lcd_console.c: manually
 *     verified — CONSOLE_COLS/ROWS divide the screen evenly (50x30 @
 *     16px), and console_scroll()'s memmove source/dest ranges stay
 *     exactly within the framebuffer.
 *   - CONFIG_SYS_PBSIZE (U-Boot's own printf line buffer): uses
 *     vscnprintf, which truncates safely rather than overflowing, so an
 *     undersized buffer there can't be the cause either.
 *
 * Leading unconfirmed theory: timing, not memory safety. Each character
 * is rendered as 256 individual pixel writes (16x16), done synchronously
 * inside the same dispatch loop that also calls the serial putc for that
 * character — on high-volume output (help's 60+ commands, USB retry
 * messages) this adds real per-character latency that could be
 * disrupting something timing-sensitive downstream (UART FIFO, a
 * watchdog) and corrupting the serial byte stream itself, independent of
 * any RAM address. Not yet tested — see docs/UBOOT_BOOTLOGO_AND_RE_PORTS.md
 * before re-enabling this.
 */
#define ENABLE_LCDCONSOLE	0

extern void ark_wdt_arm(unsigned int timeout_ms);

/* 2026-08-01: real hardware reports occasional hangs with no serial
 * console attached (in-vehicle use) -- splash visible (ark_show_bootlogo()
 * completed) but boot never reaches bootusb/nandboot at all. Root cause:
 * everything from here through the autoboot countdown and
 * CONFIG_BOOTCOMMAND's own uEnv.txt fatload + bootcheck runs with ZERO
 * watchdog protection -- earlier and separate from the two fatload
 * sequences already covered (do_bootnand()'s NANDBOOT_WDT_MS arm,
 * boot_from_block_dev()'s KERNEL_HANDOFF_WDT_MS arm, both in
 * ark1668_boot_cmds.c). This is the earliest point in the whole
 * automatic-boot sequence it's safe to arm from -- before it,
 * ark_show_bootlogo() hasn't even run yet. 20s comfortably covers
 * ark_show_bootlogo()'s own 2 fatloads + the autoboot countdown +
 * uEnv.txt's fatload + bootcheck (every boot log shows this whole
 * window finishing in a couple of seconds); bootusb/nandboot then
 * re-arm with their own fresh timeouts before this one could expire on
 * a legitimately slow but successful boot. */
#define BOARD_LATE_INIT_WDT_MS	20000

int board_late_init(void)
{
	char cmd[128];
	char *need_update,*update_flash;
	unsigned int loadaddr;
	int do_update = 0, update_from_mmc = 1;

	ark_wdt_arm(BOARD_LATE_INIT_WDT_MS);

	ark_show_bootlogo();
#if ENABLE_LCDCONSOLE
	ark_lcd_console_init();
#else
	printf("[lcdconsole] disabled at build time (ENABLE_LCDCONSOLE=0), serial-only\n");
#endif

	update_flash = env_get("update_from_flash");
	if (update_flash)
		printf("[update] update_from_flash=%s\n", update_flash);
	need_update = env_get("need_update");
	if (need_update && !strcmp(need_update, "yes")) {
		loadaddr = env_get_hex("loadaddr", 0);

		char *sd_dev_part = env_get("sd_dev_part");
		char *loadaddr_str = env_get("loadaddr");
		if (sd_dev_part && loadaddr_str) {
			sprintf(cmd, "fatload %s %s %s update-magic", "mmc", sd_dev_part, loadaddr_str);
			run_command(cmd, 0);
		}
		if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
			do_update = 1;
			run_command("env default -f -a", 0);
			goto update_done;
		} else {
			printf("[update] wrong update magic, do not update from mmc.\n");
		}

#ifdef CONFIG_USB_MUSB_HCD
		//use old musb driver
		run_command("usb start", 0);
#endif
		if (loadaddr_str) {
			sprintf(cmd, "fatload %s %s %s update-magic", "usb", "0", loadaddr_str);
			run_command(cmd, 0);
		}
		if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
			do_update = 1;
			run_command("env default -f -a", 0);
			update_from_mmc = 0;
			goto update_done;
		} else {
			printf("[update] wrong update magic, do not update from usb.\n");
		}
	}	
	else if (update_flash && !strcmp(update_flash, "yes")){

		run_command("env default -f -a", 0);
		mdelay(100);		
 		env_set("update_from_flash", "yes");	
		mdelay(100);	
	    sprintf(cmd, "run updatefromflash");
		printf("[update] cmd=%s\n", cmd);	
		run_command(cmd, 0);
 
	}

update_done:
	if (do_update) {

		env_set("need_update", "no");
		env_set("do_update", "yes");
		if (update_from_mmc) {
			printf("[update] update form mmc...\n");
			env_set("update_dev_type", "mmc");
			env_set("update_dev_part", env_get("sd_dev_part"));
		} else {
			printf("[update] update form usb...\n");
			env_set("update_dev_type", "usb");
			env_set("update_dev_part", "0");
		}
	} else {
		env_set("do_update", "no");
	}

	return 0;
}

#ifdef CONFIG_SPL_BUILD
void mem_init(void)
{
	int ret;
	gpio_direction_output(8, 1);
	udelay(10);

reset:
	/* controller reset */
	rSYS_SOFT_RSTNA |= 1 << 8;
	rSYS_SOFT_RSTNB |= 0x3 << 10;
	udelay(1);
	rSYS_SOFT_RSTNA &= ~(1 << 8);
	rSYS_SOFT_RSTNB &= ~(0x3 << 10);
	udelay(10);
	rSYS_SOFT_RSTNA |= 1 << 8;
	rSYS_SOFT_RSTNB |= 0x3 << 10;
	udelay(10);

	ddr3_sdramc_init();
	udelay(1500);
	if (rSYS_DDR_STATUS & (1 << 2))	//train error
		goto reset;
	udelay(1);
	ret = ddr3_data_training(0);
	if (ret) {
		udelay(1);
		goto reset;
	}
	udelay(1);

	rREMAP = 1;
	udelay(10);
	printf("[mem_init] remap...\n");
}
#endif
