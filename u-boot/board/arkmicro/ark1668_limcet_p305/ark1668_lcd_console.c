/*
 * lcdconsole — mirrors u-boot's console output (the same text normally
 * only seen over serial) onto the physical LCD screen, as a scrolling
 * monochrome text console on OSD2_LAYER.
 *
 * Registered as a standard stdio_dev (see include/stdio_dev.h) and added
 * to stdout/stderr via CONFIG_CONSOLE_MUX's comma-separated env fan-out
 * (see include/configs/ark1668_limcet_p305.h — "stdout=serial,lcdconsole").
 * Serial output is completely unaffected; this only adds a second
 * simultaneous output device, using the exact same OSD-layer primitives
 * (ark_set_osd_image/addr, ark_osd_en_layer) already proven by the boot
 * logo work — no new hardware register protocol here, just pixel writes
 * into a RAM framebuffer the display controller was already reading from.
 *
 * Font: 16x16 monochrome, ASCII 0x20-0x7e, rasterized from DejaVu Sans
 * Mono (make_console_font.py) at a real 16px size and thresholded for
 * thin, legible console-style strokes — rendering natively at 16x16
 * rather than pixel-doubling an 8x8 bitmap, which lost too much detail
 * under a hard threshold at that size.
 */

#include "ark1668_lcd.h"
#include <stdio_dev.h>
#include <iomux.h>
#include "ark_console_font.h"

/* Deliberately NOT in the 0xexxxxxx/0xfxxxxxx range used elsewhere on this
 * board (bootlogo/arkdata/bootlogofind) — this board's u-boot only knows
 * about 64MB of DRAM (CONFIG_SYS_SDRAM_SIZE), so 0x0-0x4000000 is the only
 * region guaranteed safe from u-boot's own perspective. The high addresses
 * happen to also be valid (the kernel uses 0xf000000 for its own LCD
 * framebuffer), but this buffer is under continuous heavy write load for
 * the entire boot (unlike a one-shot file load), so it gets the safe
 * address rather than relying on that. 0x2500000 sits clear of the
 * zImage (0x1000000) and DTB (0x2000000) staging areas. */
#define CONSOLE_FB_ADDR		0x2500000
#define CONSOLE_SCREEN_W	800
#define CONSOLE_SCREEN_H	480
#define CONSOLE_CELL		16	/* native glyph size in ark_console_font.h */
#define CONSOLE_COLS		(CONSOLE_SCREEN_W / CONSOLE_CELL)
#define CONSOLE_ROWS		(CONSOLE_SCREEN_H / CONSOLE_CELL)

#define CONSOLE_FG		0xffffffff	/* white, ARGB LE -> BGRA in memory, same convention as the rest of this board's OSD code */
#define CONSOLE_BG		0xff000000	/* black */

static int console_started;
static int cursor_col, cursor_row;

static inline unsigned int *fb_pixel(int x, int y)
{
	return (unsigned int *)(CONSOLE_FB_ADDR) + (y * CONSOLE_SCREEN_W) + x;
}

static void console_clear_row(int row)
{
	int x, y;
	for (y = row * CONSOLE_CELL; y < (row + 1) * CONSOLE_CELL; y++)
		for (x = 0; x < CONSOLE_SCREEN_W; x++)
			*fb_pixel(x, y) = CONSOLE_BG;
}

static void console_clear_all(void)
{
	int row;
	for (row = 0; row < CONSOLE_ROWS; row++)
		console_clear_row(row);
}

static void console_scroll(void)
{
	/* move rows 1..ROWS-1 up by one text row, clear the new last row */
	memmove(fb_pixel(0, 0), fb_pixel(0, CONSOLE_CELL),
		CONSOLE_SCREEN_W * (CONSOLE_SCREEN_H - CONSOLE_CELL) * 4);
	console_clear_row(CONSOLE_ROWS - 1);
}

static void console_draw_glyph(int col, int row, char c)
{
	const unsigned short *glyph;
	int gx, gy;
	int base_x = col * CONSOLE_CELL;
	int base_y = row * CONSOLE_CELL;

	if (c < CONSOLE_FONT_FIRST || c > CONSOLE_FONT_LAST)
		return;
	glyph = console_font_16x16[(unsigned char)c - CONSOLE_FONT_FIRST];

	for (gy = 0; gy < CONSOLE_CELL; gy++) {
		unsigned short bits = glyph[gy];
		for (gx = 0; gx < CONSOLE_CELL; gx++) {
			unsigned int color = (bits & (1 << (15 - gx))) ? CONSOLE_FG : CONSOLE_BG;
			*fb_pixel(base_x + gx, base_y + gy) = color;
		}
	}
}

static void console_newline(void)
{
	cursor_col = 0;
	cursor_row++;
	if (cursor_row >= CONSOLE_ROWS) {
		console_scroll();
		cursor_row = CONSOLE_ROWS - 1;
	}
}

static void lcdconsole_putc_raw(char c)
{
	if (c == '\n') {
		console_newline();
		return;
	}
	if (c == '\r') {
		cursor_col = 0;
		return;
	}
	if (c == '\t') {
		int next = (cursor_col + 8) & ~7;
		while (cursor_col < next && cursor_col < CONSOLE_COLS)
			console_draw_glyph(cursor_col++, cursor_row, ' ');
		if (cursor_col >= CONSOLE_COLS)
			console_newline();
		return;
	}

	console_draw_glyph(cursor_col, cursor_row, c);
	cursor_col++;
	if (cursor_col >= CONSOLE_COLS)
		console_newline();
}

static void lcdconsole_putc(struct stdio_dev *dev, const char c)
{
	if (!console_started)
		return;
	lcdconsole_putc_raw(c);
}

static void lcdconsole_puts(struct stdio_dev *dev, const char *s)
{
	if (!console_started)
		return;
	while (*s)
		lcdconsole_putc_raw(*s++);
}

/* Bring up OSD2 as the full-screen text layer and register the stdio
 * device. Call once, after ark_display_init() has already brought up the
 * panel (screen clocks/timing must already be live). Safe to call even if
 * the boot logo is also showing on OSD1 — OSD2 renders above it, so once
 * text starts appearing it will cover the logo; that's the intended
 * behavior (logo during the earliest boot moment, live log once things
 * start happening). */
void ark_lcd_console_init(void)
{
	struct stdio_dev dev;

	console_clear_all();
	cursor_col = 0;
	cursor_row = 0;

	/* OSD1 (bootlogo, when a bootlogo.raw was found) composites above
	 * OSD2 — confirmed on hardware: with a logo loaded, OSD1 fully
	 * covers OSD2's text; without one, OSD1 stays disabled (its default
	 * state from ark_display_init()) and text is the only thing visible.
	 * The console taking over is the intended behavior once boot
	 * proceeds past the initial splash, so turn OSD1 off explicitly
	 * here instead of relying on layer priority. */
	ark_osd_en_layer(OSD1_LAYER, 0);

	ark_set_osd_image(OSD2_LAYER, DISP_RGB_888, CONSOLE_SCREEN_W, CONSOLE_SCREEN_H);
	ark_set_osd_addr(OSD2_LAYER, CONSOLE_FB_ADDR);
	ark_disp_set_osd_layer_position(OSD2_LAYER, 0, 0);
	ark_osd_en_layer(OSD2_LAYER, 1);

	memset(&dev, 0, sizeof(dev));
	strcpy(dev.name, "lcdconsole");
	dev.flags = DEV_FLAGS_OUTPUT;
	dev.putc = lcdconsole_putc;
	dev.puts = lcdconsole_puts;

	if (stdio_register(&dev) == 0) {
		console_started = 1;
		printf("[lcdconsole] registered, %dx%d chars on OSD2 @ 0x%x\n",
		       CONSOLE_COLS, CONSOLE_ROWS, CONSOLE_FB_ADDR);

		/* console_init_r() already ran (long before board_late_init())
		 * and built stdout/stderr's device list from the env before
		 * this device existed — search_device("lcdconsole") failed
		 * then and it was silently dropped from the list. Re-run the
		 * exact same env-parsing step console_init_r() used
		 * (iomux_doenv(), CONFIG_CONSOLE_MUX) now that the device is
		 * actually registered, so it gets picked up this time. */
		if (iomux_doenv(stdout, "serial,lcdconsole"))
			printf("[lcdconsole] iomux_doenv(stdout) failed\n");
		if (iomux_doenv(stderr, "serial,lcdconsole"))
			printf("[lcdconsole] iomux_doenv(stderr) failed\n");
	} else {
		printf("[lcdconsole] stdio_register failed\n");
	}
}
