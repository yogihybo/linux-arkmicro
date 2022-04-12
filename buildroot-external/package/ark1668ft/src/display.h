#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#define CVBS_PAL    0
#define CVBS_NTSC   1
#define NTSC_WIDTH  720
#define NTSC_HEIGHT 480

#define  ITU601_WIDTH       800
#define  ITU601_HEIGHT      480

int ark7116Init(void);
int ark7116SignalDetect(void);

int display_init(void);
void display_uninit(void);
void ark_disp_init_tvenc_cvbs(int mode);
void ark_disp_init_itu601_out(void);
void itu656_init(void);
void itu601_init(void);
void itu_uninit(void);
int itu656_compare_data(void);
int itu601_compare_data(void);
int lcd_rgb_pad_test(void);

#endif

