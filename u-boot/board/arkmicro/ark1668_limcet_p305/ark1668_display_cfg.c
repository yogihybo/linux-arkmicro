#include "ark1668_lcd.h"
#include <asm-generic/gpio.h>
#include <asm/setup.h>
#include <version.h>


#define  UPDATING_WIDTH      200
#define  UPDATING_HEIGHT     20

#define  PROGRESS_WIDTH     200
#define  PROGRESS_HEIGHT     32

#define  UPDATE_LOGO_WIDTH   PROGRESS_WIDTH
#define  UPDATE_LOGO_HEIGHT  (UPDATING_HEIGHT+PROGRESS_HEIGHT)

#if 1
//chinese
#define  UPDATING_POINT_COUNT  397
static unsigned short updating[UPDATING_POINT_COUNT] =
{
        810, 821, 828, 840, 866, 878, 883, 894, 900, 1008, 1009, 1010, 1021, 1024, 1025, 1026, 
        1027, 1028, 1040, 1060, 1066, 1076, 1077, 1078, 1079, 1080, 1083, 1086, 1093, 1094, 1096, 1097, 
        1098, 1099, 1100, 1206, 1207, 1210, 1220, 1223, 1224, 1225, 1227, 1228, 1240, 1260, 1261, 1263, 
        1264, 1265, 1266, 1267, 1268, 1269, 1275, 1276, 1277, 1280, 1281, 1283, 1285, 1293, 1299, 1404, 
        1405, 1406, 1407, 1410, 1420, 1422, 1425, 1427, 1436, 1440, 1441, 1442, 1443, 1444, 1445, 1461, 
        1466, 1467, 1468, 1477, 1481, 1483, 1484, 1492, 1494, 1498, 1499, 1500, 1501, 1502, 1603, 1606, 
        1610, 1619, 1621, 1622, 1624, 1627, 1628, 1629, 1636, 1637, 1638, 1639, 1640, 1641, 1644, 1645, 
        1664, 1665, 1666, 1667, 1669, 1677, 1678, 1679, 1680, 1683, 1684, 1685, 1686, 1692, 1694, 1695, 
        1696, 1697, 1698, 1702, 1806, 1810, 1812, 1813, 1814, 1818, 1819, 1820, 1821, 1824, 1825, 1826, 
        1827, 1828, 1829, 1836, 1840, 1844, 1859, 1860, 1861, 1862, 1863, 1864, 1865, 1866, 1867, 1868, 
        1869, 1870, 1874, 1875, 1876, 1877, 1880, 1881, 1882, 1883, 1884, 1885, 1891, 1892, 1894, 1896, 
        1897, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2014, 2020, 2024, 2025, 2028, 
        2036, 2040, 2044, 2058, 2059, 2060, 2063, 2068, 2076, 2077, 2078, 2080, 2085, 2090, 2092, 2094, 
        2096, 2097, 2098, 2099, 2100, 2101, 2206, 2210, 2219, 2220, 2221, 2222, 2223, 2224, 2226, 2227, 
        2228, 2236, 2237, 2238, 2239, 2240, 2241, 2242, 2243, 2244, 2245, 2260, 2264, 2265, 2266, 2267, 
        2268, 2276, 2277, 2279, 2280, 2281, 2282, 2283, 2284, 2285, 2292, 2294, 2295, 2296, 2298, 2406, 
        2410, 2419, 2420, 2423, 2426, 2427, 2436, 2440, 2460, 2464, 2465, 2466, 2467, 2468, 2475, 2477, 
        2480, 2485, 2492, 2494, 2498, 2499, 2500, 2501, 2502, 2606, 2610, 2621, 2622, 2623, 2626, 2627, 
        2640, 2660, 2662, 2663, 2664, 2668, 2674, 2675, 2677, 2680, 2681, 2682, 2683, 2684, 2685, 2692, 
        2694, 2695, 2696, 2697, 2698, 2805, 2810, 2820, 2821, 2822, 2825, 2826, 2827, 2840, 2851, 2852, 
        2860, 2861, 2862, 2864, 2865, 2866, 2867, 2868, 2874, 2877, 2880, 2885, 2892, 2897, 2899, 3004, 
        3005, 3010, 3018, 3019, 3021, 3022, 3024, 3025, 3028, 3040, 3051, 3052, 3060, 3061, 3064, 3068, 
        3077, 3080, 3083, 3085, 3092, 3096, 3100, 3106, 3107, 3114, 3115, 3122, 3123, 3203, 3204, 3210, 
        3221, 3223, 3224, 3228, 3229, 3230, 3240, 3252, 3264, 3266, 3267, 3268, 3277, 3280, 3284, 3285, 
        3292, 3295, 3296, 3300, 3301, 3403, 3422, 3423, 3451, 3464, 3494, 3495, 3502, 
};

#else
//english
#define  UPDATING_POINT_COUNT  237
static unsigned short updating[UPDATING_POINT_COUNT] =
{
        1021, 1022, 1023, 1044, 1045, 1222, 1223, 1235, 1236, 1422, 1423, 1435, 1436, 1601, 1602, 1603, 
        1605, 1606, 1607, 1609, 1610, 1611, 1612, 1613, 1614, 1619, 1620, 1621, 1622, 1623, 1627, 1628, 
        1629, 1630, 1633, 1634, 1635, 1636, 1637, 1638, 1642, 1643, 1644, 1645, 1649, 1650, 1652, 1653, 
        1654, 1659, 1660, 1661, 1662, 1663, 1802, 1803, 1806, 1807, 1810, 1811, 1814, 1815, 1818, 1819, 
        1822, 1823, 1826, 1827, 1830, 1831, 1835, 1836, 1844, 1845, 1850, 1851, 1852, 1854, 1855, 1858, 
        1859, 1861, 1862, 2002, 2003, 2006, 2007, 2010, 2011, 2014, 2015, 2018, 2019, 2022, 2023, 2028, 
        2029, 2030, 2031, 2035, 2036, 2044, 2045, 2050, 2054, 2055, 2058, 2059, 2061, 2062, 2202, 2203, 
        2206, 2207, 2210, 2211, 2214, 2215, 2218, 2219, 2222, 2223, 2227, 2228, 2230, 2231, 2235, 2236, 
        2244, 2245, 2250, 2254, 2255, 2259, 2260, 2261, 2402, 2403, 2406, 2407, 2410, 2411, 2414, 2415, 
        2418, 2419, 2422, 2423, 2426, 2427, 2430, 2431, 2435, 2436, 2444, 2445, 2450, 2454, 2455, 2458, 
        2459, 2466, 2467, 2468, 2474, 2475, 2476, 2482, 2483, 2484, 2603, 2604, 2605, 2606, 2607, 2608, 
        2610, 2611, 2612, 2613, 2614, 2619, 2620, 2621, 2622, 2623, 2624, 2627, 2628, 2629, 2630, 2631, 
        2632, 2636, 2637, 2638, 2642, 2643, 2644, 2645, 2646, 2647, 2649, 2650, 2651, 2652, 2654, 2655, 
        2656, 2658, 2659, 2660, 2661, 2662, 2666, 2667, 2668, 2674, 2675, 2676, 2682, 2683, 2684, 2810, 
        2811, 2858, 2859, 2862, 2863, 3009, 3010, 3011, 3012, 3059, 3060, 3061, 3062,
};
#endif
static struct screen_info screens [] = {
	/* vbp/vfp/hfp/hsw/clk_freq/clk_div1 corrected to match the real
	 * calibration in arkdata.ini [LCD_TIMMING]/[LCD_CLOCK] (the compiled
	 * defaults were generic/reference values, not this panel's actual
	 * timing — see conversation history / docs/uboot_build.md).
	 * Original (pre-correction) values, kept for revert if the new
	 * timing doesn't sync on real hardware:
	 * {SCREEN_QUN700, SCREEN_TYPE_RGB, LVDS_FORMAT_RGB0888, 800, 480, 40, 36, 16, 32, 32, 41, 0, 1, 1, 0, 0x8, 12,  SCREEN_CLKSEL_SYSPLL,0,13,1,  DISP_RGB_888,RGB_MODE_BGR,32,0,0, 0,0,0,0},
	 */
	{SCREEN_QUN700, SCREEN_TYPE_RGB, LVDS_FORMAT_RGB0888, 800, 480, 29, 25, 16, 32, 25, 54, 0, 1, 1, 0, 0x8, 12,  SCREEN_CLKSEL_SYSPLL,330000000,11,1,  DISP_RGB_888,RGB_MODE_BGR,32,0,0, 0,0,0,0},
	{SCREEN_CVBS_NTSC, SCREEN_TYPE_CVBS, CVBS_FORMAT_NTSC, 720, 480, 0, 0, 21, 0, 0, 135, 0, 0, 0, 0, 0x1F,0,     0,0,0,0,  DISP_RGB_888,RGB_MODE_BGR,32,0,1, 0,0,0,0},
	{SCREEN_CVBS_PAL, SCREEN_TYPE_CVBS, CVBS_FORMAT_PAL, 720, 576, 0, 0, 23, 0, 0, 141, 0, 0, 0, 0, 0x1F,0,       0,0,0,0,  DISP_RGB_888,RGB_MODE_BGR,32,0,1, 0,0,0,0},
	{SCREEN_VGA8060, SCREEN_TYPE_VGA, VGA_FORMAT_800x600, 800, 600, 23, 1, 3, 79, 39, 127, 0, 0, 0, 0, 0x1F,0,    SCREEN_CLKSEL_DDSCLK,200,5,0, DISP_RGB_888,RGB_MODE_BGR,32,0,0, 0,0,0,0},
	{SCREEN_TYPE_YPBPR720P, SCREEN_TYPE_YPBPR, YPBPR_FORMAT_720P60HZ, 1280, 720, 0, 0, 29, 0, 0, 369, 0, 0, 0, 0, 0x1F,0,  SCREEN_CLKSEL_DDSCLK,297,4,0, DISP_RGB_888,RGB_MODE_BGR,32,0,0, 0,0,0,0},
	{SCREEN_C101EAN, SCREEN_TYPE_LVDS, LVDS_FORMAT_RGB0888, 1280, 720, 14, 14, 10, 60, 60, 46, 0, 1, 1, 0, 0x8, 12,        SCREEN_CLKSEL_SYSPLL,0,6,1,  DISP_RGB_888,RGB_MODE_BGR,32,0xE0EC,0, 0,0,0,0},
	{SCREEN_CLAA101, SCREEN_TYPE_LVDS, LVDS_FORMAT_RGB0888, 1024, 600, 8, 8, 10, 50, 50, 60, 0, 1, 1, 0, 0x8, 12,     SCREEN_CLKSEL_SYSPLL,0,7,1,  DISP_RGB_888,RGB_MODE_BGR,32,0x160FD,0, 0,0,0,0},
	{0},
	{0},//SCREEN_USER_TYPE
};

#define UPDATELOGO_DISPLAY_ADDR	        0xE200000


static int g_screen_id = 0;
struct screen_info *g_screen_info;
display_updatepara g_display_para;




static void  screen_info_show(struct screen_info *p_info)
{
        printf("[screen_info] screen_id=%d\n",p_info->screen_id);
        printf("[screen_info] ScreenType:%d, subType:%d,Width:%d, Height:%d,Format:%d, TvoutType:%d,screen_id:%d,interlace:%d\r\n", 
        		p_info->screen_type,p_info->format,p_info->width, p_info->height, p_info->src_format,p_info->tvout_format,p_info->screen_id,p_info->interlace); 
        printf("[screen_info] RgbMode:%d ,BPP:%d ,pad_unset:%d\r\n", p_info->rgb_mode,p_info->bpp,p_info->pad_unset); 
        printf("[screen_info] HFP:%d,HBP:%d,HSW:%d,VBP:%d,VFP:%d,VSW:%d,HSYNCPolarity:0x%0x,VSYNCPolarity:0x%0x ,DEPolarity:%d\r\n", 
        		p_info->hfp,p_info->hbp,p_info->hsw,p_info->vbp,p_info->vfp,p_info->vsw,p_info->hsync_active,p_info->vsync_active,p_info->de_active);
        printf("[screen_info] CLKSource:%d,CLKFreq:%d,CLKDIV1:%d,CLKDIV2:%d,CLKPolarity:%d,LVDSCfg:0x%0x,FrameRate:%d,\r\n", 
        		p_info->clk_source,p_info->clk_freq,p_info->clk_div1,p_info->clk_div2,p_info->vclk_active,p_info->lvds_cfg,p_info->frame_rate);
}

static void updating_init(void)
{
        int i;
        unsigned int *p = (unsigned int *)(UPDATELOGO_DISPLAY_ADDR);
        unsigned int color = 0xffffffff;
        int pos = 0;

        for(i = 0; i < UPDATING_WIDTH*UPDATING_HEIGHT; i++){
                if(updating[pos] == i){
                        *p = color;
                        pos++;
                }
                
                p++;
        }
}

static void update_progress_init(void)
{
        int i, j;
        unsigned int *p = (unsigned int *)(UPDATELOGO_DISPLAY_ADDR + UPDATING_HEIGHT*PROGRESS_WIDTH*4);
        int height = UPDATE_LOGO_HEIGHT;
        unsigned int color = 0xffffffff;

        for(j = UPDATING_HEIGHT; j < height; j++){
                for(i = 0; i < PROGRESS_WIDTH; i++){
                        if(j < (UPDATING_HEIGHT + 2) || j > (height -3) || \
                                i < 2 || i > (PROGRESS_WIDTH -3)){
                                *p = color;
                        }
                        
                        p++;
                }
        }
}

static void update_progress_set(int percent)
{
        int i, j, end;;
        unsigned int *p = (unsigned int *)(UPDATELOGO_DISPLAY_ADDR + UPDATING_HEIGHT*PROGRESS_WIDTH*4);
        int height = UPDATE_LOGO_HEIGHT;
        unsigned int color = 0xff00ff00;

        if(percent < 0 || percent > 100){
                printf("[update_progress] percent=%d, error.",percent);
                return;
        }

        end = percent * (PROGRESS_WIDTH-3)/100;

        for(j = UPDATING_HEIGHT; j < height; j++){
                for(i = 0; i < PROGRESS_WIDTH; i++){
                        if(j > (UPDATING_HEIGHT + 2) && j < (height -3) \
                                && i > 2 && i < end){
                                *p = color;
                        }
                        p++;
                }
        }

        if(percent >= 90){
                udelay(200000);
        }
        else if(percent == 100){
                udelay(500000);
                ark_osd_en_layer(OSD2_LAYER, 0);
        }

}

static void display_updatelogo(void)
{
	struct screen_info *screen = &screens[g_screen_id];
	int x, y, height;
	unsigned int src_width = UPDATE_LOGO_WIDTH;
        unsigned int src_height = UPDATE_LOGO_HEIGHT;

        memset((void *)UPDATELOGO_DISPLAY_ADDR, 0 , src_width*src_height*4);
        updating_init();
        update_progress_init();

	if (!IS_TVENC_SCREEN(screen)) 
		ark_disp_wait_lcd_frame_int();
	else
		ark_disp_wait_tvenc_frame_int();

	x = (screen->width - src_width) / 2;
	y = (screen->height - src_height) / 2;
	height = src_height;
	if (is_interlace_tvenc(screen)) {
		y /= 2;
		height /= 2;
	}
        
	ark_set_osd_image(OSD2_LAYER, DISP_RGB_888, src_width, height);
	ark_set_osd_addr(OSD2_LAYER, UPDATELOGO_DISPLAY_ADDR);
	ark_disp_set_osd_layer_position(OSD2_LAYER, x, y);
        //printf("---->%d  %d  %d  %d.\n",x, y,src_width,height);

}


/* Bootlogo NAND+SD search/validate, ported from the stock binary's
 * FUN_0006bf68 (see docs/uboot_build.md RE trail). Stock's actual behavior:
 * try the "bootlogo" NAND partition first, check the loaded data for a
 * JPEG SOI marker (0xFFD8); if that's missing/absent, fall back to
 * `fatload mmc <0/1/2>:x bootlogo` on the SD card, trying each partition
 * in turn. We have no JPEG decoder (see jpeghw's port notes), so this
 * can't feed our display pipeline directly — it's kept as a diagnostic:
 * confirms whether valid bootlogo JPEG bytes are present and where, which
 * is useful when comparing against the pre-converted bootlogo.raw path
 * ark_show_bootlogo() actually uses. */
/* Was 0xe000000 — moved for the same reason as BOOTLOGO_SD_ADDR above
 * (outside U-Boot's declared 64MB DRAM, real hardware showed this class
 * of address causing memory corruption elsewhere in U-Boot). */
#define BOOTLOGO_SCRATCH_ADDR	0x2a00000

static int bootlogo_has_jpeg_soi(unsigned int addr)
{
	unsigned char *p = (unsigned char *)addr;
	return (p[0] == 0xff && p[1] == 0xd8);
}

int do_bootlogofind(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[64];
	int part;

	if (run_command("nand read " __stringify(BOOTLOGO_SCRATCH_ADDR)
			 " bootlogo 0x140000", 0) == 0 &&
	    bootlogo_has_jpeg_soi(BOOTLOGO_SCRATCH_ADDR)) {
		printf("[bootlogofind] valid JPEG in NAND bootlogo partition "
		       "(loaded at 0x%x)\n", BOOTLOGO_SCRATCH_ADDR);
		return 0;
	}
	printf("[bootlogofind] no valid JPEG in NAND, trying SD card...\n");

	for (part = 0; part < 3; part++) {
		sprintf(cmd, "fatload mmc %d:1 0x%x bootlogo", part,
			BOOTLOGO_SCRATCH_ADDR);
		if (run_command(cmd, 0) == 0 &&
		    bootlogo_has_jpeg_soi(BOOTLOGO_SCRATCH_ADDR)) {
			printf("[bootlogofind] valid JPEG on mmc %d (loaded at 0x%x)\n",
			       part, BOOTLOGO_SCRATCH_ADDR);
			return 0;
		}
	}

	printf("[bootlogofind] no valid bootlogo JPEG found in NAND or SD 0-2\n");
	return 1;
}

U_BOOT_CMD(
	bootlogofind, 1, 0, do_bootlogofind,
	"search NAND/SD for a valid bootlogo JPEG (diagnostic only)",
	"bootlogofind\n"
);

void ark_display_init(int screen_id)
{
	struct screen_info *screen = &screens[screen_id];
	int interlace;
	
	//lcd clk enable
	rSYS_AHB_CLK_EN |= 1 << 8;
	rSYS_AXI_CLK_EN |= 1 << 1;
	rSYS_PER_CLK_EN |= 1 << 4;

	arkdata_apply_lcd_timing(screen);
        screen_info_show(screen);

	g_screen_info = screen;
        memset(&g_display_para, 0 ,sizeof(g_display_para));
        memcpy(&g_display_para.screeninfo, screen, sizeof(struct screen_info));
#if ARK_DISPLAY_ALL_MODE
        arkdata_apply_vpinfo(&g_display_para.vpinfo);
#endif

	ark_backlight_config_f(screen->screen_id);
	ark_display_initialize_port(screen);
	interlace = is_interlace_tvenc(screen);        

	/* display_updatelogo() (stock's small "updating..." progress-bar
	 * overlay, drawn via UPDATELOGO_DISPLAY_ADDR = 0x0E200000) is
	 * deliberately not called here — we don't use it, it briefly showed
	 * on screen on every boot for no reason, and that address is in the
	 * same outside-U-Boot's-64MB-DRAM category that caused real memory
	 * corruption elsewhere (see docs/UBOOT_BOOTLOGO_AND_RE_PORTS.md).
	 * Removing the call removes that write entirely rather than just
	 * relocating it. */
        ark_osd_en_layer(OSD1_LAYER, 0);

	/* OSD2 — unlike OSD1 (bootlogo, transient, managed by
	 * ark_show_bootlogo() below), the real stock U-Boot always leaves
	 * OSD2 fully configured and enabled here, pointing at a fixed
	 * framebuffer address ABOVE the kernel's own "mem=180M" ceiling
	 * (0x0be00000, ~190MB) — i.e. memory the kernel's own allocator
	 * never manages, so its display driver can build on a
	 * pre-reserved OSD2 layer instead of allocating one itself.
	 *
	 * This build previously left OSD2 fully disabled/zeroed
	 * (SIZE/ADDR/CTL all 0, OSD2 bit clear in rLCD_CONTROL) — the
	 * likely root cause of the real stock kernel's ark_disp driver
	 * failing to register /dev/ark_display (and MsnCoreApp/DirectFB
	 * failing right after) when booted via `bootnand` directly,
	 * instead of chainloading the real stock U-Boot via `bootstock`.
	 * Confirmed via a live md.l register comparison against a working
	 * stock unit — see
	 * docs/historical/HANDOFF_nand_ecc_uboot_vs_kernel.md §5.
	 *
	 * Values below are the exact ones read live from stock, not
	 * derived from ark_set_osd_image()'s partial-bitfield helper (which
	 * only touches a subset of CTL's bits), to guarantee a byte-for-byte
	 * match rather than an approximation. */
	rLCD_OSD2_SIZE = 0x001e0320;   /* 800x480, same format as OSD1 */
	rLCD_OSD2_ADDR = 0x0be00000;   /* fixed, outside kernel-managed RAM */
	rLCD_OSD2_CTL  = 0x002320ff;   /* enabled + format/order bits matching stock */
        ark_osd_en_layer(OSD2_LAYER, 1);
	if (IS_TVENC_SCREEN(screen) && interlace) {
		ark_set_osd_frame_mode(OSD2_LAYER, 1);
	} 

	ark_backlight_config(screen->screen_id);
   
}


/* Boot logo shown from U-Boot, decoded offline and stored raw on the SD card
 * (see convert_bootlogo.py) since we have no JPEG decoder available here.
 *
 * Was 0xfc00000 (reusing bootanimationaddr) — moved after real hardware
 * testing showed memory corruption (garbled `help` output, corrupted
 * command-table entries) traced to buffers living outside U-Boot's own
 * declared 64MB DRAM (CONFIG_SYS_SDRAM_SIZE). 0xfc00000 is valid video
 * memory from the kernel's perspective (it has its own full memory map),
 * but U-Boot's own allocator/relocation logic knows nothing about
 * anything past 0x4000000, so writes there aren't guaranteed safe from
 * U-Boot's side. See docs/UBOOT_BOOTLOGO_AND_RE_PORTS.md. */
#define BOOTLOGO_SD_ADDR	0x0b400000
#define BOOTLOGO_WIDTH		800
#define BOOTLOGO_HEIGHT		480

static int display_bootlogo_file(const char *filename)
{
	char cmd[64];
	unsigned long filesize;
	int ret;

	/* Disable OSD1 before touching its live framebuffer. On the very
	 * first call (ark_show_bootlogo()) this is a no-op -- OSD1 starts
	 * disabled, from ark_display_init()'s default state. On a later
	 * swap (bootlogofile), OSD1 is already enabled and pointed at this
	 * same address from the previous load, and the LCDC composites from
	 * it continuously at the panel refresh rate -- without disabling it
	 * here first, the memset+fatload below overwrites a buffer the
	 * display hardware is actively reading, and the screen shows
	 * whatever partially-written garbage happens to be there at each
	 * refresh (zeroed regions, half-loaded new bytes, stale old bytes)
	 * for as long as the load takes -- the "multicolour striped screen
	 * between each image" seen on real hardware (2026-07-29). */
	ark_osd_en_layer(OSD1_LAYER, 0);

	/* Zero the buffer before loading — if bootlogo.raw is undersized
	 * (wrong resolution, truncated/corrupt conversion), fatload only
	 * overwrites however many bytes the file actually has, leaving
	 * whatever was previously in this RAM in the remainder. In a
	 * row-major framebuffer that tail is the bottom rows of the image,
	 * which is exactly where colored-pixel artifacts were seen on real
	 * hardware. Failing to black there instead of showing stale memory
	 * is a lot easier to diagnose. */
	memset((void *)BOOTLOGO_SD_ADDR, 0, BOOTLOGO_WIDTH * BOOTLOGO_HEIGHT * 4);

	sprintf(cmd, "fatload mmc 0:1 0x%x %s", BOOTLOGO_SD_ADDR, filename);
	printf("[bootlogo] loading -> `%s`\n", cmd);
	ret = run_command(cmd, 0);
	if (ret != 0) {
		printf("[bootlogo] fatload of %s failed (ret=%d) — file "
		       "missing from SD card FAT partition, or mmc 0:1 not "
		       "accessible; skipping splash\n", filename, ret);
		return -1;
	}

	filesize = env_get_hex("filesize", 0);
	printf("[bootlogo] fatload reported filesize=0x%lx (%lu bytes), "
	       "expected 0x%x (%dx%dx32bpp)\n",
	       filesize, filesize, BOOTLOGO_WIDTH * BOOTLOGO_HEIGHT * 4,
	       BOOTLOGO_WIDTH, BOOTLOGO_HEIGHT);
	if (filesize != (unsigned long)(BOOTLOGO_WIDTH * BOOTLOGO_HEIGHT * 4)) {
		printf("[bootlogo] WARNING — size mismatch, bootlogo.raw may be the "
		       "wrong resolution or corrupt; showing it anyway\n");
	}

	printf("[bootlogo] pushing OSD1 image %dx%d @ 0x%x (DISP_RGB_888)\n",
	       BOOTLOGO_WIDTH, BOOTLOGO_HEIGHT, BOOTLOGO_SD_ADDR);
	ark_set_osd_image(OSD1_LAYER, DISP_RGB_888, BOOTLOGO_WIDTH, BOOTLOGO_HEIGHT);
	ark_set_osd_addr(OSD1_LAYER, BOOTLOGO_SD_ADDR);
	ark_disp_set_osd_layer_position(OSD1_LAYER, 0, 0);
	ark_osd_en_layer(OSD1_LAYER, 1);
	printf("[bootlogo] OSD1 layer enabled, splash should be visible now\n");

	return 0;
}

void ark_show_bootlogo(void)
{
	printf("[bootlogo] ark_show_bootlogo() starting, screen_id=%d\n", SCREEN_QUN700);

	g_screen_id = SCREEN_QUN700;
	ark_display_init(g_screen_id);
	printf("[bootlogo] ark_display_init() done\n");

	/* Keep OSD2 layer enabled at 0x0be00000 (190MB) to match stock U-Boot state */
	ark_osd_en_layer(OSD2_LAYER, 1);
	if (display_bootlogo_file("bootlogo.raw") != 0)
		printf("[bootlogo] no splash shown this boot (see reason above)\n");
}

/* bootlogofile <name> — swap the OSD1 splash to a different pre-rendered
 * variant mid-boot (display is already initialized by ark_show_bootlogo()
 * at this point, so this only needs to redo the fatload + OSD1 addr/enable
 * steps, not the full ark_display_init()). Used by CONFIG_BOOTCOMMAND to
 * show "Loading USB"/"Loading NAND" as the boot sequence actually reaches
 * each stage -- see build_tools/generate_boot_status_logos.py for how the
 * variant files are generated (same gradient/emblem/wordmark/font as
 * bootlogo.raw, only the status line differs). Silently no-ops (keeps
 * whatever was showing) if the named file isn't present on the SD card,
 * same fail-safe behavior as the initial bootlogo.raw load. */
int do_bootlogofile(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return cmd_usage(cmdtp);

	display_bootlogo_file(argv[1]);
	return 0;
}

U_BOOT_CMD(
	bootlogofile, 2, 0, do_bootlogofile,
	"swap the OSD1 boot splash to a different raw framebuffer file from the SD card",
	"bootlogofile <filename.raw>\n"
);

int do_disconfig (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long percent = 0;
	int ret;
	if(argc < 2)
		return cmd_usage(cmdtp);
	ret = strict_strtoul(argv[1], 10, &percent);
	if(ret < 0){
		printf("[disconfig] error format screen id\n");
	}
	
	if(percent > 100)
	{
		printf("[disconfig] invalid param.\n");
		return cmd_usage(cmdtp);
	}
                
        if(percent == 0){
                g_screen_id = SCREEN_QUN700;//SCREEN_CLAA101;
	        ark_display_init(g_screen_id);
                printf("[disconfig] ark display init g_screen_id=%d.\n",g_screen_id);
        }else{
                update_progress_set(percent);
                printf("[disconfig] update progress set percent=%ld.\n",percent);
        }

	return 0;
}

/***************************************************/

U_BOOT_CMD(
	disconfig,	CONFIG_SYS_MAXARGS,	0,	do_disconfig,
	"config display",
	"disconfig percent\n"
	"percent(0:) \n"
);

/* backcarcheck: reads the reverse-gear GPIO and configures the ITU656
 * camera-input controller accordingly, matching stock U-Boot's own
 * backcar routine -- recovered via Ghidra decompilation of the raw
 * mtd1_uboot.bin binary (no symbols; confirmed present, byte-identical,
 * in every vendor U-Boot dump we have -- Holden, CarSyncTech Toyota,
 * P306 2025, and all 3 copies of this unit's own U-Boot). GPIO 5 is
 * the real backcar-detect pin (active-low, confirmed via the stock
 * kernel's own matching gpio_request(..., "backcar") label). Register
 * base 0xe0800000 is ITU656IN_BASE, confirmed via ark1668.dtsi's
 * itu656in@e0800000 node -- the reverse camera's video CAPTURE INPUT
 * controller, not the LCDC (0xe0500000). Does NOT touch GPIO 81 (the
 * shared LCD backlight enable pin, already correctly managed by
 * ark_backlight_config()/ark_backlight_config_f() elsewhere in this
 * file's boot path) -- stock's own routine briefly toggles it off/on
 * around this same check, but duplicating that here risks a new
 * flicker/race against the existing backlight logic for no confirmed
 * benefit; see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 30.
 *
 * Stock's own MCU UART notification (two fixed frames sent whenever
 * this GPIO is read): an earlier session found the decompiled protocol
 * bytes but could find no caller anywhere in stock's binary after an
 * exhaustive search (direct branch, literal pool, movw/movt, thumb
 * interworking) and left it unported as possibly-dead code. 2026-07-31:
 * ported anyway per explicit instruction, despite that reachability
 * doubt -- see ark_mcu_notify_backcar() below and
 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md for the full reasoning and
 * caveats (the onoff-value polarity in particular is an unconfirmed
 * inference, not disassembly-proven). */
#define ARK_BACKCAR_GPIO	5

int do_backcarcheck(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int in_reverse;

	gpio_direction_input(ARK_BACKCAR_GPIO);
	in_reverse = (gpio_get_value(ARK_BACKCAR_GPIO) == 0);

	rITU656IN_INPUT_SEL |= 1;
	if (in_reverse) {
		rITU656IN_MODULE_EN |= 6;
		PIX_LINE_NUM_DELTA = 0x1e0a;
	} else {
		rITU656IN_IMR = 0;
	}

	printf("[backcarcheck] gpio%d=%d, in_reverse=%d.\n",
	       ARK_BACKCAR_GPIO, gpio_get_value(ARK_BACKCAR_GPIO), in_reverse);

	return 0;
}

U_BOOT_CMD(
	backcarcheck,	1,	0,	do_backcarcheck,
	"read reverse-gear GPIO and configure ITU656 camera input",
	""
);

/* 2026-07-31: stock's own U-Boot->MCU backcar-onoff UART notification,
 * ported from FUN_0006ede4 in the real mtd1_uboot.bin. UART2
 * (0xE8000000, CONFIG_SYS_SERIAL2) is a separate physical port from
 * whatever's the active console -- not necessarily clocked/pinmuxed/
 * baud-configured already, so this replicates stock's own one-time init
 * sequence (FUN_0006ed64) before transmitting, rather than assuming the
 * port is ready. Raw register pokes, matching this project's existing
 * style for this class of hardware access (see ark_itu656_camera_bypass_enable()
 * in ark1668_debug_cmds.c) rather than going through U-Boot's own
 * multi-instance serial framework (drivers/serial/serial_pl01x.c),
 * which has no clean API for "write to a specific non-console port".
 *
 * Frame bytes and the SYS_BASE+0x1e0 pinmux/init register values below
 * are extracted directly from stock's rodata/instruction stream, not
 * guessed -- see the plan file / docs for the full disassembly trail.
 * The onoff-value polarity (does nonzero mean entering or leaving
 * reverse?) is an unconfirmed inference: no direct caller of
 * FUN_0006ede4 was found, so the exact call convention couldn't be
 * checked against a real call site. Implemented here as nonzero =
 * camera turning ON (send both frames), zero = OFF (send only the
 * first frame) -- flip this if hardware testing shows the MCU reacting
 * backwards. */
#define ARK_UART2_BASE		0xE8000000
#define ARK_UART2_DR		(ARK_UART2_BASE + 0x00)
#define ARK_UART2_FR		(ARK_UART2_BASE + 0x18)
#define ARK_UART2_FR_TXFF	(1 << 5)
#define ARK_UART2_IBRD		(ARK_UART2_BASE + 0x24)
#define ARK_UART2_FBRD		(ARK_UART2_BASE + 0x28)
#define ARK_UART2_LCRH		(ARK_UART2_BASE + 0x2c)
#define ARK_UART2_VENDOR34	(ARK_UART2_BASE + 0x34) /* meaning unknown, replicated verbatim */
#define ARK_UART2_CR		(ARK_UART2_BASE + 0x30)
#define ARK_UART2_VENDOR38	(ARK_UART2_BASE + 0x38) /* meaning unknown, replicated verbatim */

static void ark_uart2_init_once(void)
{
	static int inited;
	volatile unsigned int *pinmux = (volatile unsigned int *)(SYS_BASE + 0x1e0);

	if (inited)
		return;

	*pinmux = (*pinmux & ~0xf00) | 0x500;

	*(volatile unsigned int *)ARK_UART2_IBRD = 13;
	*(volatile unsigned int *)ARK_UART2_FBRD = 1;
	*(volatile unsigned int *)ARK_UART2_LCRH = 0x70;   /* 8N, FIFO enable */
	*(volatile unsigned int *)ARK_UART2_VENDOR34 = 0x1d;
	*(volatile unsigned int *)ARK_UART2_CR = 0x301;    /* UARTEN | TXE | RXE */
	*(volatile unsigned int *)ARK_UART2_VENDOR38 = 0x50;

	inited = 1;
}

static void ark_uart2_putc(unsigned char c)
{
	ark_uart2_init_once();
	while (*(volatile unsigned int *)ARK_UART2_FR & ARK_UART2_FR_TXFF)
		;
	*(volatile unsigned int *)ARK_UART2_DR = c;
}

void ark_mcu_notify_backcar(int onoff)
{
	static const unsigned char frame1[7] = {0x0d, 0x24, 0x03, 0x00, 0x01, 0xff, 0x02};
	static const unsigned char frame2[6] = {0x0d, 0x24, 0x02, 0x02, 0x04, 0xfb};
	unsigned int i;

	for (i = 0; i < sizeof(frame1); i++)
		ark_uart2_putc(frame1[i]);
	if (onoff) {
		for (i = 0; i < sizeof(frame2); i++)
			ark_uart2_putc(frame2[i]);
	}

	printf("[carback] notified mcu backcar onoff=%d\n", onoff);
}

/* 2026-07-31: automatic boot-time gate -- calls both the ITU656 camera
 * bypass (ark1668_debug_cmds.c) and the MCU notify above when the
 * reverse-gear GPIO is asserted, mirroring stock's own early-boot
 * behavior (subagent disassembly found this reached "early in board
 * init," independent of which boot medium ends up chosen). Invoked as
 * the very first thing in CONFIG_BOOTCOMMAND via the carbackcamcheck
 * command below, before uEnv.txt import / bootcheck / medium selection,
 * so it fires as fast as possible after power-on. */
void ark_carback_camera_check(void)
{
	const char *mode = env_get("carback_camera_mode");
	int in_reverse;

	if (mode && strcmp(mode, "0") == 0)
		return; /* explicitly disabled, mirrors stock's env gate */

	gpio_direction_input(ARK_BACKCAR_GPIO);
	in_reverse = (gpio_get_value(ARK_BACKCAR_GPIO) == 0); /* active-low */

	if (in_reverse) {
		printf("[carback] reverse detected at boot -- enabling instant camera preview\n");
		ark_itu656_camera_bypass_enable();
		ark_mcu_notify_backcar(1);
	}
}

int do_carbackcamcheck(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ark_carback_camera_check();
	return 0;
}

U_BOOT_CMD(
	carbackcamcheck,	1,	0,	do_carbackcamcheck,
	"instant boot-time reverse-camera preview if the reverse-gear GPIO is asserted",
	""
);

/* setup_board_tags() -- resolves the "track paint init out width or
 * height fail!" bug seen on bootnand (stock 3.4 kernel direct boot),
 * plus two related vendor ATAGs ported for consistency with stock.
 * See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 40/43/44 for
 * the full trace.
 *
 * Root cause of the track-paint bug: the stock kernel's
 * track_paint_init() (carback/reverse-camera overlay) validates the
 * reservingtrack blob's width/height against a global struct at a
 * fixed kernel address, populated ONLY by parse_tag_display_param() --
 * one of three custom ATAG parsers stock's kernel registers. U-Boot's
 * mainline arch/arm/lib/bootm.c calls a __weak setup_board_tags() hook
 * right before ATAG_NONE, specifically so boards can inject vendor
 * tags; stock's board file overrides it to emit these three, ours
 * never defined the override at all, so the weak no-op ran instead --
 * the kernel's struct stayed zeroed from its __memzero(), and
 * track_paint_init()'s width/height check failed against 0. This was
 * previously investigated (section 40) and parked as an apparent
 * "dynamic struct, no static global" dead end -- that read was wrong:
 * it *is* a real static global, just one populated via ATAG at boot
 * rather than by any function call, which is why an exhaustive
 * call-graph/xref sweep of the kernel binary alone couldn't find a
 * writer.
 *
 * All three tags recovered from stock's uboot.bin via Ghidra (function
 * chain at 0x31b18 calling 0x319e8/0x31a20/0x31a7c, matching bootm.c's
 * setup_board_tags() call site exactly, confirmed via the same
 * ATAG_CORE/ATAG_CMDLINE/ATAG_MEM/ATAG_INITRD2 literal-pool values as
 * our own unmodified bootm.c). Tag-ID-to-handler mapping was
 * cross-checked against the kernel's own (unstripped, real symbol
 * names) parse_tag_uboot_version()/parse_tag_backcar()/
 * parse_tag_display_param(), matching each handler's payload size
 * against each U-Boot sender's declared size -- not just assumed from
 * declaration order:
 *
 * - ATAG_BACKCAR (0x41000403, U-Boot @ 0x319e8): 1-word payload,
 *   exactly matching parse_tag_backcar()'s single-word read into a
 *   kernel global later checked once in ark_disp_dev_init() (gates a
 *   display-layer init default for one specific OSD/video sub-layer).
 *   Stock reads this from a fixed U-Boot global, populated somewhere
 *   in its own boot sequence -- not re-derived here since it wasn't
 *   possible to reliably recover the same build-specific value from a
 *   binary compiled for a different customer/board revision than this
 *   exact unit (later per-build data offsets diverge even though the
 *   code itself is shared). Rather than guess a magic constant, this
 *   sends the SAME live GPIO 5 read already used by backcarcheck
 *   (do_backcarcheck() above) -- a well-justified, verifiable value
 *   (real reverse-gear state at boot) even if it doesn't reproduce
 *   whatever static flag stock's specific build happened to compile
 *   in.
 * - ATAG_UBOOT_VERSION (0x41000404, U-Boot @ 0x31a20): 64-byte payload,
 *   exactly matching parse_tag_uboot_version()'s 0x40-byte copy into a
 *   kernel-side version-string buffer. Stock sends its own build
 *   string (a "root<date>"-style tag, differs per customer build and
 *   isn't meaningful to copy); this sends U-Boot's own version_string
 *   (include/version.h) instead -- informational only, no kernel
 *   behavior found to depend on its content, just presence/format.
 * - ATAG_DISPLAY_PARAM (0x41000405, U-Boot @ 0x31a7c): 536-byte (0x218)
 *   payload, four separate memcpy's from a single vendor struct,
 *   byte-identical in size to this file's own display_updatepara:
 *   0x78 bytes (screeninfo) at payload offset 0, 0x98 bytes at +0x80,
 *   0xb8 bytes at +0x118, 0x50 bytes at +0x1d0. Only the first
 *   (screeninfo, holding width/height at struct offsets 0xc/0x10) is
 *   confirmed to matter -- track_paint_init() and every other reader
 *   found in the kernel's call-graph sweep (section 41) only ever
 *   reads those two fields from this tag. The other three sections'
 *   exact field-for-field mapping into vpinfo/gammainfo/
 *   itu656bypinfo/spec_info/touch_info was not re-derived (would need
 *   further stock decompilation to confirm padding/order) -- sent
 *   zeroed, matching this tag's previous fully-absent state for those
 *   fields, so this is a strict improvement with no new regression
 *   risk for the parts left unmapped.
 *
 * All three tag IDs sit immediately after mainline's own
 * ATAG_MEMCLK=0x41000402 (arch/arm/include/asm/setup.h) -- the vendor
 * extended the same numbering block mainline already reserved,
 * corroborating this reading rather than being a coincidence. */
#define ATAG_BACKCAR		0x41000403
#define ATAG_UBOOT_VERSION	0x41000404
#define ATAG_DISPLAY_PARAM	0x41000405
#define ATAG_DISPLAY_PARAM_PAYLOAD_SIZE	0x218
#define ATAG_UBOOT_VERSION_PAYLOAD_SIZE	0x40

void setup_board_tags(struct tag **in_params)
{
	struct tag *params = *in_params;
	u32 backcar_word;

	/* ATAG_BACKCAR: same live GPIO 5 read as do_backcarcheck() above. */
	gpio_direction_input(ARK_BACKCAR_GPIO);
	backcar_word = (gpio_get_value(ARK_BACKCAR_GPIO) == 0) ? 1 : 0;

	params->hdr.tag = ATAG_BACKCAR;
	params->hdr.size = (sizeof(struct tag_header) +
			     sizeof(backcar_word) + 3) >> 2;
	memcpy(&params->u, &backcar_word, sizeof(backcar_word));
	params = tag_next(params);

	/* ATAG_UBOOT_VERSION: our own build's version banner. */
	params->hdr.tag = ATAG_UBOOT_VERSION;
	params->hdr.size = (sizeof(struct tag_header) +
			     ATAG_UBOOT_VERSION_PAYLOAD_SIZE + 3) >> 2;
	memset(&params->u, 0, ATAG_UBOOT_VERSION_PAYLOAD_SIZE);
	strncpy((char *)&params->u, version_string,
		ATAG_UBOOT_VERSION_PAYLOAD_SIZE - 1);
	params = tag_next(params);

	/* ATAG_DISPLAY_PARAM: only the screeninfo section (width/height)
	 * is populated -- see comment block above. */
	params->hdr.tag = ATAG_DISPLAY_PARAM;
	params->hdr.size = (sizeof(struct tag_header) +
			     ATAG_DISPLAY_PARAM_PAYLOAD_SIZE + 3) >> 2;
	memset(&params->u, 0, ATAG_DISPLAY_PARAM_PAYLOAD_SIZE);
	memcpy(&params->u, &g_display_para.screeninfo,
	       sizeof(struct screen_info));
	params = tag_next(params);

	*in_params = params;
}

/* regr/regw/pmem — ported from the stock binary's debug command table
 * (recovered via Ghidra decompilation of mtd1_uboot.bin @ 0x6c0d8/0x6c244/
 * 0x6c404). Generic register peek/poke across the SoC's display-adjacent
 * blocks, keyed by an opcode selecting the base address:
 *   0=LCD_BASE  1=SYS_BASE(offset<=0x1f0)  2=ITU656_BASE
 *   3=VICL_BASE  4=VICH_BASE  5=JPEG_BASE  6=PWM_BASE
 * op=6 (PWM_BASE) has no stock counterpart — added locally so the already
 * source-confirmed backlight PWM path (ark_backlight_config() in
 * ark1668_lcd.c, rPWM_ENA1/CNTR1/DUTY1 @ PWM_BASE+0x10/0x14/0x18 for
 * SCREEN_QUN700) can be probed the same way as everything else.
 * Useful for exactly the kind of "what does the compiled build touch that
 * stock doesn't" register comparison this board needed for the white-screen
 * bring-up issue. */

static int reg_base_for_op(int op, unsigned long *base_out, unsigned long max_off)
{
	switch (op) {
	case 0: *base_out = LCD_BASE;    return 0;
	case 1: *base_out = SYS_BASE;    return (max_off > 0x1f0) ? -1 : 0;
	case 2: *base_out = ITU656_BASE; return 0;
	case 3: *base_out = VICL_BASE;   return 0;
	case 4: *base_out = VICH_BASE;   return 0;
	case 5: *base_out = JPEG_BASE;   return 0;
	case 6: *base_out = PWM_BASE;    return 0;
	default: return -2;
	}
}

int do_regr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long op, off, base;
	int ret;

	if (argc < 3)
		return cmd_usage(cmdtp);

	if (strict_strtoul(argv[1], 16, &op) < 0 ||
	    strict_strtoul(argv[2], 16, &off) < 0) {
		printf("[regr] error format op_code/register\n");
		return 1;
	}
	if (off & 3) {
		printf("[regr] invalid reg offset 0x%02lx, should be 4-byte aligned\n", off);
		return 1;
	}
	ret = reg_base_for_op(op, &base, off);
	if (ret == -2) {
		printf("[regr] invalid op_code %ld\n", op);
		return 0;
	}
	if (ret == -1 || off > 0x800) {
		printf("[regr] invalid reg offset 0x%02lx, exceeds range for op_code %ld\n", off, op);
		return 1;
	}

	printf("[regr] op=%ld reg 0x%02lx = 0x%08x\n", op, off,
	       *(volatile unsigned int *)(base + off));
	return 0;
}

int do_regw(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long op, off, val, base;
	int ret;

	if (argc < 4)
		return cmd_usage(cmdtp);

	if (strict_strtoul(argv[1], 16, &op) < 0 ||
	    strict_strtoul(argv[2], 16, &off) < 0 ||
	    strict_strtoul(argv[3], 16, &val) < 0) {
		printf("[regw] error format op_code/register/value\n");
		return 1;
	}
	if (off & 3) {
		printf("[regw] invalid reg offset 0x%02lx, should be 4-byte aligned\n", off);
		return 1;
	}
	ret = reg_base_for_op(op, &base, off);
	if (ret == -2) {
		printf("[regw] invalid op_code %ld\n", op);
		return 0;
	}
	if (ret == -1 || off > 0x800) {
		printf("[regw] invalid reg offset 0x%02lx, exceeds range for op_code %ld\n", off, op);
		return 1;
	}

	*(volatile unsigned int *)(base + off) = val;
	printf("[regw] op=%ld reg 0x%02lx <- 0x%08x (readback 0x%08x)\n", op, off, (unsigned int)val,
	       *(volatile unsigned int *)(base + off));
	return 0;
}

int do_pmem(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long addr, len, i;
	unsigned char *p;

	if (argc < 3)
		return cmd_usage(cmdtp);

	if (strict_strtoul(argv[1], 16, &addr) < 0 ||
	    strict_strtoul(argv[2], 16, &len) < 0) {
		printf("[pmem] error format addr/len\n");
		return 1;
	}

	p = (unsigned char *)addr;
	for (i = 0; i < len; i++) {
		printf("0x%02x ", p[i]);
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	printf("\n");
	return 0;
}

U_BOOT_CMD(
	regr, 4, 0, do_regr,
	"read register (LCD/SYS/ITU656/VICL/VICH/JPEG/PWM)",
	"regr op_code reg_offset\n"
	"  op_code: 0=LCD 1=SYS(<=0x1f0) 2=ITU656 3=VICL 4=VICH 5=JPEG 6=PWM\n"
);

U_BOOT_CMD(
	regw, 4, 0, do_regw,
	"write register (LCD/SYS/ITU656/VICL/VICH/JPEG/PWM)",
	"regw op_code reg_offset value\n"
	"  op_code: 0=LCD 1=SYS(<=0x1f0) 2=ITU656 3=VICL 4=VICH 5=JPEG\n"
);

U_BOOT_CMD(
	pmem, 3, 0, do_pmem,
	"hex dump of a memory region",
	"pmem addr len\n"
);

