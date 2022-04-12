#ifndef _ARK7116_H_
#define _ARK7116_H_



#define ARK7116_AV0  0
#define ARK7116_AV1  1
#define ARK7116_AV2  2
///////////////////////////

typedef enum _InputSourceIDType
{
	INPUT_AV1,
	INPUT_AV2,
	INPUT_TV,
	INPUT_CAMERA_DoorBell,
	INPUT_CAMERA_Car,
	INPUT_SVIDEO,
	INPUT_ITU656,
	INPUT_FM,	
	INPUT_YPBPR,
	INPUT_VGA,
	MAX_VIDEO_CHANNEL ,
	ALL_INPUT_SOURCE = 0XFF,
}InputSourceType;


typedef enum _ConfigDisplayMode
{
     DISP_16_9= 0 ,
     DISP_4_3,
}ConfigDisplayMode;



typedef enum _ColorSysType
{
    PAL = 0,
	PAL_N,
	PAL_M,
    NTSC,
    SECAM,   
    PAL60,
    AUTO,
	NULL_SYS = -1,
}ColorSysType;



//MCU CFG Addr
#define MCU_CFG_ADDR 				0xC6

/************************Global ***********************/
#define RSTN                           	0XFD00
#define ENH_PLL                        0XFD0E


//BUS Addr
#define BUS_STATUS_ADDR         	0xAF


typedef struct _PanlstaticPara
{
    unsigned int addr;
    unsigned char dat;
}PanlstaticPara;

typedef struct _PanlPosDynPara
{
    unsigned int addr;
    unsigned char dat_posDyn[6];
}PanlPosDynPara;

typedef struct _PanlSysDynPara
{
    unsigned int addr;
    unsigned char dat_sysDyn[8];
}PanlSysDynPara;


typedef enum _VdeOutputType
{
	VDE_CLOSE = 0,             
	VDE_RED,          
	VDE_GREEN,          
	VDE_BLUE ,                    
	VDE_GRAY,     
	VDE_WHITE,     
	VDE_BLACK,
	MAX_VDECOLOR = VDE_BLACK,
} VdeOutputTyp;

/*************************************VP CONTROL REG*********************************/
#define BRIGHT_REG               		0XFFD4
#define CONTRAST_REG            		0XFFD3 
#define SATURATION_REG          		0XFFD6
#define TINT_REG                        0XFFD5
#define VDE_REG                    		0XFFD2
 

/*==============start===============*/
/*AV1
[VideoChannel]
AV1
[VideoType]
CVBS
[VideoPI]
VIDEO_P
[VideoPicSys]
PAL
[VideoData]
13500000
 690
 280
 864
 312

Update date:Monday, November 24, 2014
Update time:11:05:45
*/

/*屏参参数相关的结构体*/
typedef struct _PannelPara
{
   PanlstaticPara  *pVideoStaicPara;
   PanlPosDynPara *pVideoPosDynPara;
   PanlSysDynPara  *pVideoSysDynPara;
}PannelPara;
typedef struct _VideoChannel
{
   unsigned char INPUT_ID;
   PannelPara    VideoPara;
}VideoChannel;

#define I2C_ACCESS_LOOP_TIME   		5


//#define I2C_DEBUG
#ifdef I2C_DEBUG
#define	gpioi2c_print(x...) printk(KERN_ALERT x)
#else
#define	gpioi2c_print(x...)
#endif

#define ARK7116_RESET 0


struct ark7116_private_data{
        struct i2c_client *client;
        int gpio_reset;
        int config_finish;
};


int ark7116_config(void);

#endif

