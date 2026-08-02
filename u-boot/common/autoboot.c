// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */
#include <common.h>
#include <autoboot.h>
#include <bootretry.h>
#include <cli.h>
#include <console.h>
#include <fdtdec.h>
#include <menu.h>
#include <post.h>
#include <u-boot/sha256.h>
#include <bootcount.h>
DECLARE_GLOBAL_DATA_PTR;
#define MAX_DELAY_STOP_STR 32
#ifndef DEBUG_BOOTKEYS
#define DEBUG_BOOTKEYS 0
#endif
#define debug_bootkeys(fmt, args...)		\
	debug_cond(DEBUG_BOOTKEYS, fmt, ##args)
/* Stored value of bootdelay, used by autoboot_command() */
static int stored_bootdelay;
#if defined(CONFIG_AUTOBOOT_KEYED)
#if defined(CONFIG_AUTOBOOT_STOP_STR_SHA256)
/*
 * Use a "constant-length" time compare function for this
 * hash compare:
 *
 * https://crackstation.net/hashing-security.htm
 */
static int slow_equals(u8 *a, u8 *b, int len)
{
	int diff = 0;
	int i;
	for (i = 0; i < len; i++)
		diff |= a[i] ^ b[i];
	return diff == 0;
}
static int passwd_abort(uint64_t etime)
{
	const char *sha_env_str = env_get("bootstopkeysha256");
	u8 sha_env[SHA256_SUM_LEN];
	u8 sha[SHA256_SUM_LEN];
	char presskey[MAX_DELAY_STOP_STR];
	const char *algo_name = "sha256";
	u_int presskey_len = 0;
	int abort = 0;
	int size = sizeof(sha);
	int ret;
	if (sha_env_str == NULL)
		sha_env_str = CONFIG_AUTOBOOT_STOP_STR_SHA256;
	/*
	 * Generate the binary value from the environment hash value
	 * so that we can compare this value with the computed hash
	 * from the user input
	 */
	ret = hash_parse_string(algo_name, sha_env_str, sha_env);
	if (ret) {
		printf("Hash %s not supported!\n", algo_name);
		return 0;
	}
	/*
	 * We don't know how long the stop-string is, so we need to
	 * generate the sha256 hash upon each input character and
	 * compare the value with the one saved in the environment
	 */
	do {
		if (tstc()) {
			/* Check for input string overflow */
			if (presskey_len >= MAX_DELAY_STOP_STR)
				return 0;
			presskey[presskey_len++] = getc();
			/* Calculate sha256 upon each new char */
			hash_block(algo_name, (const void *)presskey,
				   presskey_len, sha, &size);
			/* And check if sha matches saved value in env */
			if (slow_equals(sha, sha_env, SHA256_SUM_LEN))
				abort = 1;
		}
	} while (!abort && get_ticks() <= etime);
	return abort;
}
#else
static int passwd_abort(uint64_t etime)
{
	int abort = 0;
	struct {
		char *str;
		u_int len;
		int retry;
	}
	delaykey[] = {
		{ .str = env_get("bootdelaykey"),  .retry = 1 },
		{ .str = env_get("bootstopkey"),   .retry = 0 },
	};
	char presskey[MAX_DELAY_STOP_STR];
	u_int presskey_len = 0;
	u_int presskey_max = 0;
	u_int i;
#  ifdef CONFIG_AUTOBOOT_DELAY_STR
	if (delaykey[0].str == NULL)
		delaykey[0].str = CONFIG_AUTOBOOT_DELAY_STR;
#  endif
#  ifdef CONFIG_AUTOBOOT_STOP_STR
	if (delaykey[1].str == NULL)
		delaykey[1].str = CONFIG_AUTOBOOT_STOP_STR;
#  endif
	for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
		delaykey[i].len = delaykey[i].str == NULL ?
				    0 : strlen(delaykey[i].str);
		delaykey[i].len = delaykey[i].len > MAX_DELAY_STOP_STR ?
				    MAX_DELAY_STOP_STR : delaykey[i].len;
		presskey_max = presskey_max > delaykey[i].len ?
				    presskey_max : delaykey[i].len;
		debug_bootkeys("%s key:<%s>\n",
			       delaykey[i].retry ? "delay" : "stop",
			       delaykey[i].str ? delaykey[i].str : "NULL");
	}
	/* In order to keep up with incoming data, check timeout only
	 * when catch up.
	 */
	do {
		if (tstc()) {
			if (presskey_len < presskey_max) {
				presskey[presskey_len++] = getc();
			} else {
				for (i = 0; i < presskey_max - 1; i++)
					presskey[i] = presskey[i + 1];
				presskey[i] = getc();
			}
		}
		for (i = 0; i < sizeof(delaykey) / sizeof(delaykey[0]); i++) {
			if (delaykey[i].len > 0 &&
			    presskey_len >= delaykey[i].len &&
				memcmp(presskey + presskey_len -
					delaykey[i].len, delaykey[i].str,
					delaykey[i].len) == 0) {
					debug_bootkeys("got %skey\n",
						delaykey[i].retry ? "delay" :
						"stop");
				/* don't retry auto boot */
				if (!delaykey[i].retry)
					bootretry_dont_retry();
				abort = 1;
			}
		}
	} while (!abort && get_ticks() <= etime);
	return abort;
}
#endif
/***************************************************************************
 * Watch for 'delay' seconds for autoboot stop or autoboot delay string.
 * returns: 0 -  no key string, allow autoboot 1 - got key string, abort
 */
static int __abortboot(int bootdelay)
{
	int abort;
	uint64_t etime = endtick(bootdelay);
#  ifdef CONFIG_AUTOBOOT_PROMPT
	/*
	 * CONFIG_AUTOBOOT_PROMPT includes the %d for all boards.
	 * To print the bootdelay value upon bootup.
	 */
	printf(CONFIG_AUTOBOOT_PROMPT, bootdelay);
#  endif
	abort = passwd_abort(etime);
	if (!abort)
		debug_bootkeys("key timeout\n");
	return abort;
}
# else	/* !defined(CONFIG_AUTOBOOT_KEYED) */
#ifdef CONFIG_MENUKEY
static int menukey;
#endif
/* arkdata.ini reader (board/arkmicro/ark1668_limcet_p305/ark1668_arkdata_ini.c)
 * -- lazy-loads and caches arkdata.ini from the SD card, already used for LCD
 * timing/RgbMode overrides. No board-specific header pulled into this
 * generic file; matches how ark_watchdog_stop() below is already called
 * from here without one. */
extern int arkdata_ini_get_int(const char *key, int base, int *out);

static int __abortboot(int bootdelay)
{
	int abort = 0;
	unsigned long ts;
	int boot_interrupt = 0;	/* default: prompt skipped, see below */

	/*
	 * 2026-08-03: SD-card-toggleable kill switch for the whole abort
	 * mechanism, same idea as disabling a serial console via
	 * cmdline.txt -- lets a "production" SD card close the noise-abort
	 * exposure investigated above entirely (no prompt printed, no
	 * tstc() polling at all, so there's genuinely nothing listening
	 * to abort on).
	 *
	 * 2026-08-03: default flipped to OFF (skip the prompt) -- an
	 * arkdata.ini that's missing/unreadable, or simply doesn't have a
	 * BootInterrupt key yet (every card built before this change),
	 * now behaves the same as BootInterrupt=0. This is a deliberate
	 * departure from this reader's usual fail-safe-to-compiled-
	 * default contract everywhere else it's used (LCD timing/RgbMode
	 * missing key = keep the old value) -- here the "old value" IS
	 * the noise-exposed behavior this toggle exists to get away from,
	 * so failing toward it would defeat the point. Set
	 * BootInterrupt=1 explicitly in arkdata.ini to get the prompt
	 * back (e.g. for active development/debugging).
	 */
	arkdata_ini_get_int("BootInterrupt", 10, &boot_interrupt);

	if (!boot_interrupt) {
		debug_bootkeys("BootInterrupt=0 in arkdata.ini -- autoboot abort disabled\n");
		return 0;
	}

#ifdef CONFIG_MENUPROMPT
	printf(CONFIG_MENUPROMPT);
#else
	printf("Press spacebar to stop autoboot: %2d ", bootdelay);
#endif
	/*
	 * Check if key already pressed
	 *
	 * 2026-08-02: tstc() only means "a byte is pending" -- it says
	 * nothing about which byte. With no console cable attached, the
	 * console UART's RX pin is undriven, and real hardware testing
	 * showed a genuinely cold boot (never a warm reset, never with a
	 * console attached) can get stuck sitting at this prompt --
	 * consistent with electrical noise during power-up being
	 * misread by the UART receiver as a spurious byte and aborting
	 * autoboot outright. A real console holds RX at a defined idle
	 * level, which is why attaching one to investigate always made
	 * the problem disappear. Fixed by requiring the actual byte to be
	 * space or Ctrl-C (0x03) before treating it as a real abort
	 * request -- any other byte (i.e. noise) is consumed and ignored,
	 * autoboot continues. Checked against the real factory-dumped
	 * U-Boot log (docs/logs/archived/uboot original dumped log_260715.txt)
	 * first: stock has this identical tstc()-driven prompt too, so this
	 * isn't a design stock omits -- it's a real gap on both, just
	 * tightened here to only the two keys that were ever intended to
	 * work.
	 */
	if (tstc()) {	/* we got a key press	*/
		int ch = getc();  /* consume input	*/
		if (ch == ' ' || ch == 0x03) {
			puts("\b\b\b 0");
			abort = 1;	/* don't auto boot	*/
		}
	}
	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = get_timer(0);
		do {
			if (tstc()) {	/* we got a key press	*/
				int ch = getc();  /* consume input, always */
				if (ch == ' ' || ch == 0x03) {
					abort  = 1;	/* don't auto boot	*/
					bootdelay = 0;	/* no more delay	*/
# ifdef CONFIG_MENUKEY
					menukey = ch;
# endif
					break;
				}
				/* spurious byte -- ignore, keep counting down */
			}
			udelay(10000);
		} while (!abort && get_timer(ts) < 1000);
		printf("\b\b\b%2d ", bootdelay);
	}
	putc('\n');

	/* 2026-08-02: moved from right after the immediate-keypress check
	 * above (where it only covered the narrow "space pressed before the
	 * countdown loop even started" case) to here, covering every abort
	 * path uniformly -- including any key pressed during the actual
	 * countdown loop, and Ctrl-C specifically, neither of which
	 * triggered ark_watchdog_stop() before. Real gap found on real
	 * hardware: user connected a terminal mid-hang, pressed Ctrl-C,
	 * successfully dropped to the interactive prompt -- but with a
	 * watchdog armed early enough to cover this whole window (which
	 * this project's own board code doesn't currently do, precisely
	 * because of this gap, see ark1668.c's board_late_init() history),
	 * an incomplete disarm here would reset the board out from under
	 * that same manual investigation, the exact regression already
	 * found and fixed once this session (checklist section 100a). */
	if (abort)
		ark_watchdog_stop();

	return abort;
}
# endif	/* CONFIG_AUTOBOOT_KEYED */
static int abortboot(int bootdelay)
{
	int abort = 0;
	if (bootdelay >= 0)
		abort = __abortboot(bootdelay);
#ifdef CONFIG_SILENT_CONSOLE
	if (abort)
		gd->flags &= ~GD_FLG_SILENT;
#endif
	return abort;
}
static void process_fdt_options(const void *blob)
{
#if defined(CONFIG_OF_CONTROL) && defined(CONFIG_SYS_TEXT_BASE)
	ulong addr;
	/* Add an env variable to point to a kernel payload, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "kernel-offset", 0);
	if (addr)
		env_set_addr("kernaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));
	/* Add an env variable to point to a root disk, if available */
	addr = fdtdec_get_config_int(gd->fdt_blob, "rootdisk-offset", 0);
	if (addr)
		env_set_addr("rootaddr", (void *)(CONFIG_SYS_TEXT_BASE + addr));
#endif /* CONFIG_OF_CONTROL && CONFIG_SYS_TEXT_BASE */
}
const char *bootdelay_process(void)
{
	char *s;
	int bootdelay;
	bootcount_inc();
	s = env_get("bootdelay");
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;
#ifdef CONFIG_OF_CONTROL
	bootdelay = fdtdec_get_config_int(gd->fdt_blob, "bootdelay",
			bootdelay);
#endif
	debug("### main_loop entered: bootdelay=%d\n\n", bootdelay);
#if defined(CONFIG_MENU_SHOW)
	bootdelay = menu_show(bootdelay);
#endif
	bootretry_init_cmd_timeout();
#ifdef CONFIG_POST
	if (gd->flags & GD_FLG_POSTFAIL) {
		s = env_get("failbootcmd");
	} else
#endif /* CONFIG_POST */
	if (bootcount_error())
		s = env_get("altbootcmd");
	else
		s = env_get("bootcmd");
	process_fdt_options(gd->fdt_blob);
	stored_bootdelay = bootdelay;
	return s;
}
void autoboot_command(const char *s)
{
	debug("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");
	if (stored_bootdelay != -1 && s && !abortboot(stored_bootdelay)) {
#if defined(CONFIG_AUTOBOOT_KEYED) && !defined(CONFIG_AUTOBOOT_KEYED_CTRLC)
		int prev = disable_ctrlc(1);	/* disable Control C checking */
#endif
		run_command_list(s, -1, 0);
#if defined(CONFIG_AUTOBOOT_KEYED) && !defined(CONFIG_AUTOBOOT_KEYED_CTRLC)
		disable_ctrlc(prev);	/* restore Control C checking */
#endif
	}
#ifdef CONFIG_MENUKEY
	if (menukey == CONFIG_MENUKEY) {
		s = env_get("menucmd");
		if (s)
			run_command_list(s, -1, 0);
	}
#endif	/* CONFIG_MENUKEY */
}
