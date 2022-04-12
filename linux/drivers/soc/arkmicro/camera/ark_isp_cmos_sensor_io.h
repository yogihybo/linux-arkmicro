//****************************************************************************
//
//	Copyright (C) 2015 ShenZhen ExceedSpace
//
//	Author	ZhuoYongHong
//
//	File name: arkn141_isp_cmos_sensor_io.h
//			io interface API to access the cmos sensor
//
//	Revision history
//
//		2015.08.25	ZhuoYongHong Initial version
//
//****************************************************************************
#ifndef _ARK_ISP_CMOS_SENSOR_IO_H_
#define _ARK_ISP_CMOS_SENSOR_IO_H_

#if defined (__cplusplus)
	extern "C"{
#endif

#include "ark_camera.h"

// 初始化CMOS sensor
// 返回值
//  0    success
// -1    failure 
int ark_isp_cmos_sensor_init (void);

// 复位CMOS sensor
// 返回值
//  0    success
// -1    failure 
int ark_isp_cmos_sensor_reset (void);

// 读取sensor的寄存器内容
//		读出的数值保存在data指针指向的地址. 
//		若数值不够32位, 将高位用0填充
// 返回值
//  0    success
// -1    failure 
int ark_isp_cmos_sensor_read_register  (u32_t addr, u32_t *data);


// 写入数据内容到sensor的寄存器
// 返回值
//  0    success
// -1    failure 
int ark_isp_cmos_sensor_write_register (u32_t addr, u32_t data);

// 16位地址, 8位数据模式的i2c读写函数
int i2c_reg16_write8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data);
int i2c_reg16_read8 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data);

// 16位地址, 16位数据模式的i2c读写函数
int i2c_reg16_write16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int data);
int i2c_reg16_read16 (unsigned int slvaddr, unsigned int ucDataOffset, unsigned int *data);



#if defined (__cplusplus)
	}
#endif		/* end of __cplusplus */

#endif	// _ARKN141_ISP_CMOS_SENSOR_IO_H_
