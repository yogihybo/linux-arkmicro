#ifndef __FTCFG_H__
#define __FTCFG_H__

#define TEST_TIMEOUT  10000   //ms

#define TEST_ITEMS_NUM      13

#define USB_TESTFILE_NAME       "usbdata.bin"
#define USB_TESTFILE_SIZE       0x100000
#define SDMMC_TESTFILE_NAME     "sdmmcdata.bin"
#define SDMMC_TESTFILE_SIZE     0x100000

#define SPINOR_TESTFILE_PATH    "/usr/share/spinor.bin"
#define SPINOR_TESTFILE_SIZE    0x20000

#define VDEC_STREAM_PATH        "/usr/share/mp4.bin"
#define VDEC_YUVDATA_PATH       "/usr/share/mp4.yuv"

#define GPU_SRCFILE_PATH        "/usr/share/mp4.yuv"
#define GPU_SRC_WIDTH           720
#define GPU_SRC_HEIGHT          480
#define GPU_DSTFILE_PATH        "/usr/share/mp4.rgb"
#define GPU_DST_WIDTH           320
#define GPU_DST_HEIGHT          240

#define JPG_FILE_PATH           "/usr/share/jpg.jpg"
#define JPG_YUVDATA_PATH        "/usr/share/jpg.yuv"
#define JPG_OUT_WIDTH           640
#define JPG_OUT_HEIGHT          480

#define SERIAL_DATA_PATH        "/usr/share/serial.bin"

#define PLAYBACK_AUDIO_PATH     "/usr/share/sin1k.wav"
#define SOUND_MATCH             128

#define LCD_VIDEO_DATA_PATH     "/usr/share/video.yuv"
#define LCD_VIDEO_WIDTH         320
#define LCD_VIDEO_HEIGHT        240
#define LCD_OSD_DATA_PATH       "/usr/share/osd.rgb"
#define LCD_OSD_WIDTH           320
#define LCD_OSD_HEIGHT          240

#define ITU656_DIFF_MAX         128
#define ITU601_DIFF_MAX         256
#define ITU656_DATA_PATH        "/usr/share/itu656.yuv"
#define ITU601_DATA_PATH        "/usr/share/itu601.yuv"
#define ITU601_DATA2_PATH       "/usr/share/itu601_2.yuv"

#define DEINT_FIELD_DATA_PATH  "/usr/share/field.yuv"
#define DEINT_FRAME_DATA_PATH  "/usr/share/frame.yuv"

#define PWM1_GPIO               94
#define PWM2_GPIO               93
#define PWM3_GPIO               96

#define PANX_GPIO               117
#define PANY_GPIO               118

#define TEST_DONE_GPIO          4
#define RET0_GPIO               5
#define RET1_GPIO               9
#define RET2_GPIO               76
#define RET3_GPIO               90
#define RET4_GPIO               91
#define RET5_GPIO               92
#define RET6_GPIO               96
#define RET7_GPIO               110

#endif
