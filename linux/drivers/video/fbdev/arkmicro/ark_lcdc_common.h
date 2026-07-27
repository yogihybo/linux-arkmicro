/*
 *  Header file for ARKN141 LCD Controller
 *
 */

#ifndef __ARKN_LCDC_COMMON_H__
#define __ARKN_LCDC_COMMON_H__

#include <linux/workqueue.h>
#include <linux/pwm.h>

/* Way LCD wires are connected to the chip:
 * A swapped wiring onboard can bring to RGB mode.
 */
#define ATOMIC_SET_LAYER_POS                    (1 << 0)
#define ATOMIC_SET_LAYER_SIZE                   (1 << 1)
#define ATOMIC_SET_LAYER_FMT                    (1 << 2)
#define ATOMIC_SET_LAYER_ADDR                   (1 << 3)
#define ATOMIC_SET_LAYER_SCALER                 (1 << 4)

/* Way LCD wires are connected to the chip:
 * A swapped wiring onboard can bring to RGB mode.
 */
#define ARK_LCDC_WIRING_BGR	0
#define ARK_LCDC_WIRING_GBR	1
#define ARK_LCDC_WIRING_RBG	2
#define ARK_LCDC_WIRING_BRG	3
#define ARK_LCDC_WIRING_GRB	4
#define ARK_LCDC_WIRING_RGB	5

/* DO NOT translate ARK_LCDC_WIRING_* through this enum before writing
 * it into the LCDC's rgb_order hardware field (OSD1_CTL/OSD2_CTL/
 * OSD3_CTL bits 18-20, and ARK1668_LCDC_CONTROL's copy of the same
 * bits) -- a same-day fix (2026-07-24) briefly did exactly that, based
 * on a debug string found elsewhere in stock's vmlinux.elf ("rgb_order:
 * 0=rgb, 1=rbg, 2=grb, 3=gbr, 4=brg, 5=bgr") that looked like it should
 * describe this field's real encoding. It was reverted the same day:
 * stock's real ark_disp_fb_set_par() (vmlinux.elf @ 0x802e2a40,
 * checklist section 33's original decompile) derives its hardware
 * rgb_order value as exactly the wiring-mode enum's own raw value (0
 * for BGR, 5 for RGB) -- i.e. lcd_wiring_mode should be written
 * directly, unchanged. The translation produced the opposite value for
 * both wiring modes actually in use on this board, causing a real,
 * hardware-confirmed red/blue channel swap in start_msn. See checklist
 * sections 33, 52, and 58 for the full history -- 52 is the wrong
 * fix, 58 is the revert. This enum's real, correct use (if any) was
 * never established; kept here only because ARK1668_LCDC_OSD1_CTL's
 * comment in ark1668_lcdfb.c cites the same debug string as unresolved
 * context, not because anything in this tree should construct one. */
enum ark_lcdc_rgb_order {
	ARK_LCDC_ORDER_RGB,
	ARK_LCDC_ORDER_RBG,
	ARK_LCDC_ORDER_GRB,
	ARK_LCDC_ORDER_GBR,
	ARK_LCDC_ORDER_BRG,
	ARK_LCDC_ORDER_BGR,
};

enum ark_platform_type {
        ARK_PLATFORM_ARK1668,
        ARK_PLATFORM_ARKN141,
        ARK_PLATFORM_ARK1668E,
        ARK_PLATFORM_MAX
};

enum ark_disp_format {
	ARK_LCDC_FORMAT_YUV422,
	ARK_LCDC_FORMAT_YUV420,
	ARK_LCDC_FORMAT_VYUY,
	ARK_LCDC_FORMAT_YUV,
	ARK_LCDC_FORMAT_RGBI555,
	ARK_LCDC_FORMAT_R5G6B5,
	ARK_LCDC_FORMAT_RGBA888,
	ARK_LCDC_FORMAT_RGB888,

	ARK_LCDC_FORMAT_Y_UV422 = 0x10,
	ARK_LCDC_FORMAT_Y_UV420 = 0x11,
};

enum ark_lcdc_yuv_order {
	ARK_LCDC_ORDER_VYUY,
	ARK_LCDC_ORDER_UYVY,
	ARK_LCDC_ORDER_YVYU,
	ARK_LCDC_ORDER_YUYV,
};


struct ark_disp_scaler {
	int src_w;
	int src_h;
	int win_x;
	int win_y;
	int win_w;
	int win_h;
	int out_x;
	int out_y;
	int out_w;
	int out_h;
	int cut_top;
	int cut_bottom;
	int cut_left;
	int cut_right;
};

struct ark_disp_addr {
	unsigned int  yaddr;
	unsigned int  cbaddr;
	unsigned int  craddr;
	unsigned int  wait_vsync;
};

struct ark_disp_reg {
	unsigned int  addr;
	unsigned int  value;
};

struct ark_screen {
	unsigned int  type;
	unsigned int  width;
	unsigned int  height;
	unsigned int  disp_x;
	unsigned int  disp_y;
	unsigned int  disp_width;
	unsigned int  disp_height;
};

struct ark_platform_info {
	int type;
	int reserve[32];
};

struct ark_disp_color {
	u8 contrast;
	u8 brightness;
	u8 saturation;
	u8 hue;
};

struct ark_disp_vp {
	struct ark_disp_color color;
	int reg[6];
};

/* Real vendor payload used by libarkcmn.so's arkapi_init_fb_display() and
 * arkapi_init_fb_video_display() (raw ioctls 0x403c4f27 / 0x403c4f37,
 * confirmed via ARM disassembly of both functions -- identical 60-byte
 * layout for both). x/y is layer position; screen_width/screen_height is
 * the rounded full framebuffer size; param6/param7 are validated non-zero
 * by the caller and are the actual rendered window width/height; the two
 * hal_field_* members are values userspace itself read back from an
 * earlier kernel query and is echoing here (kernel only consumes this
 * struct, never writes it back), so we don't need to interpret them.
 */
struct ark_fb_init_display {
	unsigned int x;
	unsigned int y;
	unsigned int right_margin;
	unsigned int bottom_margin;
	unsigned int screen_width;
	unsigned int screen_height;
	/* Confirmed via Ghidra decompile cross-reference (2026-07-27,
	 * arkapi_init_fb_video_display()'s sibling arkapi_init_fb_display_internal()
	 * call passes 0x11 == ARK_LCDC_FORMAT_Y_UV420 in this exact
	 * positional slot): this is the video layer's pixel format, not
	 * an opaque param. Previously never read by our ARKFB_INIT_DISPLAY/
	 * ARKFB_INIT_VIDEO_DISPLAY handler at all -- VIDEO2_CTL's format
	 * bits were left at whatever stale/POR value was there, producing
	 * severe green/color-cast corruption once video actually started
	 * rendering. See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 74.
	 */
	unsigned int format;
	unsigned int _reserved0;
	unsigned int _reserved1;
	unsigned int param4;
	unsigned int param5;
	unsigned int win_width;
	unsigned int win_height;
	unsigned int hal_field_1b0;
	unsigned int hal_field_1a0;
};

/* Real vendor payload used by libarkcmn.so's arkapi_set_fb_video_addr()
 * (raw ioctl 0x40104f38, confirmed via ARM disassembly) -- per-frame
 * video buffer address update for CarPlay/phone-mirroring video.
 */
struct ark_fb_set_video_addr {
	unsigned int y_addr;
	unsigned int cb_addr;
	unsigned int cr_addr;
	unsigned int param5;
};

/* Real vendor payload for the ARKFB_SET_BLEND ioctl (raw ioctl
 * 0x40104f29, confirmed via ARM disassembly of stock's real
 * ark_fb_set_blend() in vmlinux.elf -- see
 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 65). Target layer is
 * whichever /dev/fbN the fd was opened against, same as every other
 * ioctl here -- not part of this struct. alpha is a single byte; the
 * remaining 3 bytes are implicit padding from the preceding int fields
 * (struct size is exactly 16 bytes both ways, confirmed against the
 * ioctl's own encoded size field).
 */
struct ark_fb_blend {
	int alpha_blend_en;
	int per_pix_alpha_blend_en;
	int blend_mode;
	unsigned char alpha;
};

struct ark_disp_atomic {
	 int layer;
	 int atomic_stat;
	 int pos_x;
	 int pos_y;
	 int width;
	 int height;
	 int format;
	 int yuyv_order;
	 int rgb_order;
	 struct ark_disp_addr addr;
	 struct ark_disp_scaler scaler;
};

#define ARK_IOW(num, dtype)				_IOW('O', num, dtype)
#define ARK_IOR(num, dtype)				_IOR('O', num, dtype)
#define ARK_IOWR(num, dtype)			_IOWR('O', num, dtype)
#define ARK_IO(num)						_IO('O', num)

#define ARKFB_GET_VSYNC_STATUS			ARK_IOW(37, unsigned int)
#define ARKFB_WAITFORVSYNC				ARK_IO(38)
#define ARKFB_SHOW_WINDOW	        	ARK_IO(39)
#define ARKFB_HIDE_WINDOW	        	ARK_IO(40)
#define ARKFB_SET_WINDOW_POS			ARK_IOW(41, unsigned int)
#define ARKFB_SET_WINDOW_SIZE			ARK_IOW(42, unsigned int)
#define ARKFB_SET_WINDOW_FORMAT			ARK_IOW(43, unsigned int)
#define ARKFB_SET_WINDOW_ADDR           ARK_IOW(44, struct ark_disp_addr)
#define ARKFB_SET_WINDOW_SCALER         ARK_IOW(45, struct ark_disp_scaler)
#define ARKFB_SET_WINDOW_ATOMIC         ARK_IOW(46, struct ark_disp_atomic)
#define ARKFB_SET_REG_VALUE             ARK_IOW(47, struct ark_disp_reg)
#define ARKFB_GET_REG_VALUE             ARK_IOR(48, struct ark_disp_reg)
#define ARKFB_GET_WINDOW_ADDR           ARK_IOR(49, struct ark_disp_addr)
#define ARKFB_GET_SCREEN_INFO           ARK_IOR(50, struct ark_screen)
#define ARKFB_SET_SCREEN_INFO           ARK_IOW(51, struct ark_screen)
#define ARKFB_GET_PLATFORM_INFO         ARK_IOR(52, struct ark_platform_info)
#define ARKFB_GET_WINDOW_FORMAT			ARK_IOR(53, unsigned int)
#define ARKFB_SET_VP_INFO				ARK_IOW(54, struct ark_disp_vp)
#define ARKFB_GET_VP_INFO				ARK_IOW(55, struct ark_disp_vp)
#define ARKFB_SET_WINDOW_PRIORITY		ARK_IOW(63, unsigned int)

/* Stock's real vendor ioctl numbering (confirmed via vmlinux.elf disassembly,
 * see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md) assigns bare ARK_IO(44) as
 * ARKFB_HIDE_WINDOW -- not our reconstructed ARK_IO(40). This collides in
 * name only with ARKFB_SET_WINDOW_ADDR above (that's an _IOW encoding, a
 * different 32-bit value, so no actual numeric collision). Userspace
 * (MsnCoreApp/libarkcmn.so) was built against the real headers and calls
 * this number, so we must also handle it.
 */
#define ARKFB_HIDE_WINDOW_REAL			ARK_IO(44)

/* Same story for SHOW_WINDOW: stock's real ark_disp_fb_ioctl dispatches
 * bare ARK_IO(43) (0x4f2b) to ark_fb_show_window, not our reconstructed
 * ARK_IO(39). libarkcmn.so's arkapi_show_fb() (used by both the CarPlay
 * DirectFB path and the AndroidAuto `sink` daemon) calls this number.
 */
#define ARKFB_SHOW_WINDOW_REAL			ARK_IO(43)

/* Real vendor ioctls issued by libarkcmn.so's arkapi_init_fb_display()
 * (PRIMARY_LAYER) / arkapi_init_fb_video_display() (VIDEO_LAYER), and
 * arkapi_set_fb_video_addr() -- confirmed via ARM disassembly, see
 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md. These share nr with existing
 * macros above (39/55/56) but differ in dir/size, so the resulting
 * 32-bit ioctl numbers do not collide.
 */
#define ARKFB_INIT_DISPLAY				ARK_IOW(39, struct ark_fb_init_display)
#define ARKFB_INIT_VIDEO_DISPLAY		ARK_IOW(55, struct ark_fb_init_display)
#define ARKFB_SET_VIDEO_ADDR_RAW		ARK_IOW(56, struct ark_fb_set_video_addr)

/* Real vendor ioctl for dynamically configuring a layer's blend
 * parameters (alpha_blend_en, per_pix_alpha_blend_en, blend_mode,
 * layer alpha), confirmed 0x40104f29 via ARM disassembly of stock's
 * real ark_fb_set_blend() -- see docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
 * section 65. Shares nr with ARKFB_SET_WINDOW_POS (41) above, but
 * differs in size (16 bytes vs 4), so the resulting 32-bit ioctl
 * numbers do not collide -- same pattern as ARKFB_INIT_DISPLAY etc.
 */
#define ARKFB_SET_BLEND					ARK_IOW(41, struct ark_fb_blend)

/* Real vendor ioctl, confirmed via its own "ARKFB_GET_LAYER_ID fail."
 * error string in libarkcmn.so's rodata (0x80044f39 = _IOR('O', 57,
 * int)) -- queries which layer a given /dev/fbN fd corresponds to.
 * Called by arkapi_init_fb_display_internal() during video-layer
 * init; non-fatal on failure (caller just logs and continues,
 * confirmed via disassembly), but was previously entirely
 * unimplemented ("unknown ioctl 80044f39" in dmesg). Shares nr with
 * VIN_SET_WINDOW_POS (57) below, but differs in dir/size (bare _IO
 * vs _IOR with a 4-byte payload), so the resulting 32-bit ioctl
 * numbers do not collide -- same pattern as ARKFB_SET_BLEND/
 * ARKFB_SET_WINDOW_POS above. See
 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 67.
 */
#define ARKFB_GET_LAYER_ID				ARK_IOR(57, int)

/* Real vendor ioctls issued by libarkcmn.so's arkapi_set_fb_addr()/
 * arkapi_get_fb_addr() -- confirmed 0x40104f2a/0x80104f36 via Ghidra
 * decompile of the real deployed libarkcmn.so. This is a SEPARATE,
 * generic address-update path from ARKFB_SET_VIDEO_ADDR_RAW (nr 56)
 * above -- observed live on hardware (2026-07-27): `sink`'s video
 * layer (fd for the video-layer /dev/fbN, layer id 4) calls this one
 * in a tight per-frame loop (arkapi_get_fb_addr x2 as a "did the HW
 * latch the previous address yet" poll, then arkapi_set_fb_addr's own
 * ioctl once), not the nr-56 path. Same 4-word {y,cb,cr,param5} shape
 * as ark_fb_set_video_addr, so it reuses that struct. Shares nr with
 * ARKFB_SET_WINDOW_SIZE (42)/ARKFB_SET_VP_INFO (54) above, but differs
 * in size/dir, so the resulting 32-bit ioctl numbers do not collide.
 * See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 73.
 */
#define ARKFB_SET_FB_ADDR				ARK_IOW(42, struct ark_fb_set_video_addr)
#define ARKFB_GET_FB_ADDR				ARK_IOR(54, struct ark_fb_set_video_addr)

#define VIN_SHOW_WINDOW	        			ARK_IO(55)
#define VIN_HIDE_WINDOW	        			ARK_IO(56)
#define VIN_SET_WINDOW_POS					ARK_IO(57)
#define VIN_SET_SOURCE_SIZE					ARK_IO(58)
#define VIN_SET_WINDOW_FORMAT					ARK_IO(59)
#define VIN_SET_WINDOW_ADDR        		    ARK_IO(60)
#define VIN_SET_WINDOW_SCALER         		ARK_IO(61)
#define VIN_SET_LAYER_SIZE					ARK_IO(62)
#endif /* __ARKN141_LCDC_H__ */
