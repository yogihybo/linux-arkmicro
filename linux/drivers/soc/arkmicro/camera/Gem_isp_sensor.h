// sensor.h
#ifndef  _GEM_ISP_SENSOR_H_
#define  _GEM_ISP_SENSOR_H_

typedef struct isp_sen_info_ 
{
  int sensor_type;
  int image_size;
  int bayer_mode;
} isp_sen_t; 

typedef struct isp_sen_info_ *isp_sen_ptr_t;



const char *isp_get_sensor_name (void);
#define	SENSOR_NAME		isp_get_sensor_name()

#endif