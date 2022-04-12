#ifndef __ARK_DISPLAY_H__
#define __ARK_DISPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>


#define DISPLAY_DATA_ID                         0x4449
#define LAYER_FREE                              -1
#define PRIMARY_LAYER_ADDR_PHY                  0x0f000000
#define PRIMARY_LAYER_SIZE_MAX                  0x01000000

#define ATOMIC_SET_LAYER_POS                    (1 << 0)
#define ATOMIC_SET_LAYER_SIZE                   (1 << 1)
#define ATOMIC_SET_LAYER_FMT                    (1 << 2)
#define ATOMIC_SET_LAYER_ADDR                   (1 << 3)
#define ATOMIC_SET_LAYER_SCALER                 (1 << 4)


#define ARK_IOW(num, dtype)                     _IOW('O', num, dtype)
#define ARK_IOR(num, dtype)                     _IOR('O', num, dtype)
#define ARK_IOWR(num, dtype)                    _IOWR('O', num, dtype)
#define ARK_IO(num)                             _IO('O', num)

#define ARKFB_GET_VSYNC_STATUS					ARK_IOW(37, unsigned int)
#define ARKFB_WAITFORVSYNC                      ARK_IO(38)
#define ARKFB_SHOW_WINDOW                       ARK_IO(39)
#define ARKFB_HIDE_WINDOW                       ARK_IO(40)
#define ARKFB_SET_WINDOW_POS                    ARK_IOW(41, unsigned int)
#define ARKFB_SET_WINDOW_SIZE                   ARK_IOW(42, unsigned int)
#define ARKFB_SET_WINDOW_FORMAT                 ARK_IOW(43, unsigned int)
#define ARKFB_SET_WINDOW_ADDR                   ARK_IOW(44, struct ark_disp_addr)
#define ARKFB_SET_WINDOW_SCALER                 ARK_IOW(45, struct ark_disp_scaler)
#define ARKFB_SET_WINDOW_ATOMIC                 ARK_IOW(46, struct ark_disp_atomic)
#define ARKFB_SET_REG_VALUE                     ARK_IOW(47, struct ark_disp_reg)
#define ARKFB_GET_REG_VALUE                     ARK_IOR(48, struct ark_disp_reg)
#define ARKFB_GET_WINDOW_ADDR                   ARK_IOR(49, struct ark_disp_addr)
#define ARKFB_GET_SCREEN_INFO                   ARK_IOR(50, struct ark_screen)
#define ARKFB_SET_SCREEN_INFO                   ARK_IOW(51, struct ark_screen)
#define ARKFB_GET_PLATFORM_INFO                 ARK_IOR(52, struct ark_platform_info)
#define ARKFB_GET_WINDOW_FORMAT         		ARK_IOR(53, unsigned int)
#define ARKFB_SET_VP_INFO           			ARK_IOW(54, struct ark_disp_vp)
#define ARKFB_GET_VP_INFO           			ARK_IOW(55, struct ark_disp_vp)
#define ARKFB_SET_WINDOW_PRIORITY              	ARK_IOW(63, unsigned int)

#define ARK1668_LCDC_BASE                       0xe0500000
#define PRIMARY_LAYER_VP_OFFSET		        0x144
#define VIDEO_LAYER_VP_OFFSET                   0x14c
#define OVER_LAYER_VP_OFFSET                    0x154
#define TVOUT_LAYER_VP_OFFSET                   0x1d8
#define AUX_LAYER_VP_OFFSET                     0x134

#define ARKN141_LCDC_BASE                       0x70190000
#define ARKN141_VP_OFFSET                       0x20c

typedef struct ark_disp_atomic {
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
} disp_atomic;

typedef struct ark_disp_head {
	struct list_head list;
	int display_mode;
	int init;
	sem_t sem;
	disp_handle *active[MAX_LAYERS];
	int reserve[64];
} disp_head;

typedef struct ark_disp_reg {
	u32 addr;
	u32 value;
} disp_reg;



#ifdef __cplusplus
}
#endif

#endif
