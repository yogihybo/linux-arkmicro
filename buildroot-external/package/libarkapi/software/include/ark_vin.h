#ifndef __ARK_VIN_H__
#define __ARK_VIN_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ARK_DVR_IOC_MAGIC			'n'

#define VIN_UPDATE_WINDOW		_IOWR(ARK_DVR_IOC_MAGIC, 50, struct vin_screen)
#define VIN_START			_IO(ARK_DVR_IOC_MAGIC, 51)
#define VIN_STOP			_IO(ARK_DVR_IOC_MAGIC, 52)
#define VIN_SWITCH_CHANNEL		_IOWR(ARK_DVR_IOC_MAGIC, 53, int)
#define VIN_CONFIG			_IOWR(ARK_DVR_IOC_MAGIC, 54, struct vin_para)

#ifdef __cplusplus
}
#endif

#endif

