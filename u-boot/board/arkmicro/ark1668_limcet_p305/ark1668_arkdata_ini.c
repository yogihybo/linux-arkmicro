/*
 * arkdata.ini reader — lets LCD timing (and, incrementally, other
 * calibration) be overridden at boot from an external arkdata.ini on the
 * SD card, instead of being baked into screens[] at compile time.
 *
 * This does NOT replicate the stock binary's ini-parser object (see
 * docs/uboot_build.md RE trail — FUN_0006f97c/f910/f6e0 with BOM
 * detection and hash-table key lookup). That was a deliberate call: the
 * observable behavior of a "Key=Value" reader is trivial to match, and
 * matching the hash-table internals byte-for-byte would have been a lot
 * of extra work for zero functional difference to anything that calls it.
 *
 * Format assumptions, confirmed against the real dumped arkdata.ini:
 *   - one "Key=Value" pair per line
 *   - ';' starts a comment (rest of line ignored)
 *   - '[Section]' header lines are skipped (lookups are flat/global —
 *     the dumped file has no key name reused across sections)
 *   - blank Value ("SrcWidth=") is treated as "key not present"
 *
 * Fails safe: if arkdata.ini isn't present or the key isn't found,
 * callers keep whatever default they already had. Nothing here is
 * required for boot — it's purely an optional override.
 */

/* Enables debug()-level tracing in this file only (per-key parse results,
 * cache-hit notices) on top of the always-on printf() logging below. Kept
 * file-scoped rather than a global build option since turning DEBUG on
 * tree-wide would flood the console with unrelated subsystem noise. */
#define DEBUG

#include "ark1668_lcd.h"

/* Was 0xfe00000 — moved for the same reason as BOOTLOGO_SD_ADDR in
 * ark1668_display_cfg.c (outside U-Boot's declared 64MB DRAM; real
 * hardware testing traced memory corruption in U-Boot's own command
 * table to buffers living in that address range). */
#define ARKDATA_BUF_ADDR	0x2900000
#define ARKDATA_BUF_MAXLEN	0x8000		/* arkdata partition is 256K; the actual file is a few KB */

static char *arkdata_buf;
static unsigned int arkdata_len;
static int arkdata_loaded;	/* 0 = not tried, 1 = loaded ok, -1 = tried and failed */

static int arkdata_ini_load(void)
{
	char cmd[64];
	unsigned long filesize;
	int ret;

	if (arkdata_loaded != 0) {
		debug("[arkdata.ini] load() called again, using cached result (%s)\n",
		      arkdata_loaded == 1 ? "loaded" : "failed");
		return arkdata_loaded == 1 ? 0 : -1;
	}

	sprintf(cmd, "fatload mmc 0:1 0x%x arkdata.ini", ARKDATA_BUF_ADDR);
	printf("[arkdata.ini] loading -> `%s`\n", cmd);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("[arkdata.ini] fatload failed (ret=%d) — file missing from SD "
		       "card FAT partition, or mmc 0:1 not accessible; using compiled "
		       "defaults\n", ret);
		arkdata_loaded = -1;
		return -1;
	}

	filesize = env_get_hex("filesize", 0);
	printf("[arkdata.ini] fatload reported filesize=0x%lx (%lu bytes)\n",
	       filesize, filesize);
	if (filesize == 0 || filesize >= ARKDATA_BUF_MAXLEN) {
		printf("[arkdata.ini] size 0x%lx out of expected range (1..0x%x), "
		       "ignoring file and using compiled defaults\n",
		       filesize, ARKDATA_BUF_MAXLEN);
		arkdata_loaded = -1;
		return -1;
	}

	arkdata_buf = (char *)ARKDATA_BUF_ADDR;
	arkdata_len = filesize;
	arkdata_loaded = 1;
	printf("[arkdata.ini] loaded %u bytes from SD into RAM @ 0x%x\n",
	       arkdata_len, ARKDATA_BUF_ADDR);
	return 0;
}

/* Find "key=" at the start of a line (after optional leading whitespace)
 * and return a pointer to the value (after '='), or NULL if not found or
 * the value is empty. *value_len is set to the value's length (up to but
 * not including the newline). */
static const char *arkdata_find_key(const char *key, unsigned int *value_len)
{
	const char *p = arkdata_buf;
	const char *end = arkdata_buf + arkdata_len;
	unsigned int keylen = strlen(key);

	while (p < end) {
		const char *line_start = p;
		const char *line_end = p;
		while (line_end < end && *line_end != '\n')
			line_end++;

		/* skip leading whitespace */
		while (line_start < line_end && (*line_start == ' ' || *line_start == '\t'))
			line_start++;

		if (line_start + keylen < line_end && line_start[keylen] == '=' &&
		    !strncmp(line_start, key, keylen)) {
			const char *val = line_start + keylen + 1;
			const char *val_end = line_end;
			/* trim trailing \r if present */
			if (val_end > val && val_end[-1] == '\r')
				val_end--;
			if (val_end == val) {
				/* "Key=" with no value */
				p = line_end + 1;
				continue;
			}
			*value_len = val_end - val;
			return val;
		}

		p = line_end + 1;
	}
	return NULL;
}

/* Parse an arkdata.ini value as an integer. base=10 or 16 (0x prefix is
 * also honored regardless of base, matching how the dumped file writes
 * both plain-decimal and 0x-prefixed values in different sections). */
int arkdata_ini_get_int(const char *key, int base, int *out)
{
	const char *val;
	unsigned int len;
	char tmp[32];

	if (arkdata_ini_load() != 0)
		return -1;

	val = arkdata_find_key(key, &len);
	if (!val) {
		debug("[arkdata.ini] key '%s' not found\n", key);
		return -1;
	}
	if (len == 0 || len >= sizeof(tmp)) {
		printf("[arkdata.ini] key '%s' has an unusable value (len=%u), skipping\n",
		       key, len);
		return -1;
	}

	memcpy(tmp, val, len);
	tmp[len] = '\0';

	*out = (int)simple_strtoul(tmp, NULL, base);
	debug("[arkdata.ini] %s='%s' -> %d (0x%x)\n", key, tmp, *out, *out);
	return 0;
}

/* One field: look up `key`, and if found, log old->new and write it into
 * *field. Returns 1 if overridden, 0 if left alone (key missing/invalid). */
static int apply_field(const char *key, unsigned int *field)
{
	int v;

	if (arkdata_ini_get_int(key, 10, &v) != 0) {
		printf("[arkdata.ini]   %-8s not found, keeping compiled default (%u)\n",
		       key, *field);
		return 0;
	}
	if ((unsigned int)v != *field)
		printf("[arkdata.ini]   %-8s %u -> %u\n", key, *field, (unsigned int)v);
	else
		printf("[arkdata.ini]   %-8s %u (unchanged)\n", key, *field);
	*field = (unsigned int)v;
	return 1;
}

/* Override an already-populated screen_info's LCD timing fields from
 * arkdata.ini, if present. Leaves the struct untouched on any failure
 * (missing file, missing keys) — always safe to call unconditionally. */
void arkdata_apply_lcd_timing(struct screen_info *screen)
{
	int overridden = 0;

	printf("[arkdata.ini] applying LCD timing overrides for screen_id=%d\n",
	       screen->screen_id);

	if (arkdata_ini_load() != 0) {
		printf("[arkdata.ini] not available, screen_id=%d keeps compiled "
		       "timing (vbp=%u vfp=%u vsw=%u hbp=%u hfp=%u hsw=%u)\n",
		       screen->screen_id, screen->vbp, screen->vfp, screen->vsw,
		       screen->hbp, screen->hfp, screen->hsw);
		return;
	}

	overridden += apply_field("VBP", &screen->vbp);
	overridden += apply_field("VFP", &screen->vfp);
	overridden += apply_field("VSW", &screen->vsw);
	overridden += apply_field("HBP", &screen->hbp);
	overridden += apply_field("HFP", &screen->hfp);
	overridden += apply_field("HSW", &screen->hsw);
	overridden += apply_field("IHS", &screen->hsync_active);
	overridden += apply_field("IVS", &screen->vsync_active);
	overridden += apply_field("IOE", &screen->de_active);
	overridden += apply_field("CLKFreq", &screen->clk_freq);
	overridden += apply_field("CLKDIV1", &screen->clk_div1);
	overridden += apply_field("CLKDIV2", &screen->clk_div2);

	printf("[arkdata.ini] done — %d/12 fields overridden from SD card, "
	       "final timing (vbp=%u vfp=%u vsw=%u hbp=%u hfp=%u hsw=%u "
	       "clk_freq=%u clk_div1=%u clk_div2=%u)\n",
	       overridden, screen->vbp, screen->vfp, screen->vsw, screen->hbp,
	       screen->hfp, screen->hsw, screen->clk_freq, screen->clk_div1,
	       screen->clk_div2);
}

int do_arkdatatest(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int v;

	if (argc < 2)
		return cmd_usage(cmdtp);

	if (arkdata_ini_get_int(argv[1], 10, &v) == 0)
		printf("[arkdatatest] %s = %d (0x%x)\n", argv[1], v, v);
	else
		printf("[arkdatatest] %s: not found or arkdata.ini not loaded\n", argv[1]);

	return 0;
}

U_BOOT_CMD(
	arkdatatest, 2, 0, do_arkdatatest,
	"look up a key in arkdata.ini (SD card) for testing",
	"arkdatatest key\n"
);
