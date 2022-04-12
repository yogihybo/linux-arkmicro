#ifndef __ARK_CARBACK_H__
#define __ARK_CARBACK_H__

#ifdef __cplusplus
extern "C" {
#endif



#define CARBACK_IOCTL_BASE                                      0x9A
#define CARBACK_IOCTL_SET_APP_READY                             _IO(CARBACK_IOCTL_BASE, 0)
#define CARBACK_IOCTL_APP_ENTER_DONE                            _IO(CARBACK_IOCTL_BASE, 1)
#define CARBACK_IOCTL_APP_EXIT_DONE                             _IO(CARBACK_IOCTL_BASE, 2)	
#define CARBACK_IOCTL_GET_STATUS                                _IOR(CARBACK_IOCTL_BASE, 3, int)	
#define CARBACK_IOCTL_DETECT_SIGNAL                             _IOR(CARBACK_IOCTL_BASE, 4, int)	



#ifdef __cplusplus
}
#endif

#endif
