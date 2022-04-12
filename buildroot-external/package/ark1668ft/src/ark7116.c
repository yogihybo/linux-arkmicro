#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>

#include "ftcfg.h"
#include "ftypes.h"
#include "utils.h"

#define BUS_STATUS_ADDR     0xAF
#define ENH_PLL             0XFD0E

typedef enum _ConfigDisplayMode
{
     DISP_16_9= 0 ,
     DISP_4_3,
}ConfigDisplayMode;

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

static int debug_flag = 0;
int fd_i2c = -1;

PanlstaticPara AV1_staticPara[]= {
//GLOBAL
    {0XFD0A,0X30},
    {0XFD0B,0X27},
    {0XFD0D,0XF0},
    {0XFD0F,0X03},
    {0XFD10,0X04},
    {0XFD11,0XFF},
    {0XFD12,0XFF},
    {0XFD13,0XFF},
    {0XFD14,0X02},
    {0XFD15,0X02},
    {0XFD16,0X0A},
    {0XFD1A,0X40},
//DECODER
	{0XFE00,0X90},
    {0XFE83,0X7F},
    {0XFE26,0X0E},
    {0XFE27,0X00},
    {0XFE28,0X00},
    {0XFE2A,0X01},
    {0XFE42,0X00},
	{0XFE44,0X20},
    {0XFE83,0X7F},
    {0XFEAB,0X3E},
    {0XFEAC,0X77},
    {0XFEB1,0X01},
    {0XFEC9,0X00},
    {0XFED0,0X41},
    {0XFED7,0XF7},
    {0XFEE0,0X2E},
    {0XFEE1,0X94},
    {0XFEE2,0X01},
//VP
    {0XFFB0,0X7E},
    {0XFFB1,0X0F},
    {0XFFB2,0X08},
    {0XFFB3,0X08},
    {0XFFB4,0X08},
    {0XFFB7,0X90},
    {0XFFB8,0X10},
    {0XFFB9,0X62},
    {0XFFBA,0X20},
    {0XFFBB,0XAA},
    {0XFFBC,0X20},
	{0XFFBD,0X20},
    {0XFFC7,0X31},
    {0XFFC8,0X06},
    {0XFFC9,0X30},
    {0XFFCB,0XC0},
    {0XFFCC,0X80},
    {0XFFCD,0X2D},
    {0XFFCE,0X10},
    {0XFFCF,0X80},
    {0XFFD0,0X80},
    {0XFFD2,0X4F},
    {0XFFD3,0X80},
    {0XFFD4,0X80},
    {0XFFD7,0X1A},
    {0XFFD8,0X80},
    {0XFFE7,0X50},
    {0XFFE8,0XFF},
    {0XFFE9,0X22},
    {0XFFEA,0X20},
    {0XFFF0,0X18},
    {0XFFF1,0XF5},
    {0XFFF2,0XF3},
    {0XFFF3,0XEA},
    {0XFFF4,0XFD},
    {0XFFF5,0X0C},
    {0XFFF6,0XFF},
    {0XFFF7,0XF1},
    {0XFFF8,0XF8},
    {0XFFF9,0XFD},
    {0XFFFA,0X2E},
    {0XFFFB,0X81},
    {0XFFD5,0X00},
    {0XFFD6,0X50},
//TCON
    {0XFC00,0X40},
//SCALE
    {0XFC90,0X02},
    {0XFC91,0X01},
    {0XFC92,0X00},
    {0XFC93,0X0C},
    {0XFC94,0X00},
    {0XFC95,0X00},
    {0XFC98,0X00},
    {0XFC99,0X04},
    {0XFC9A,0X59},
    {0XFC9B,0X03},
    {0XFC9C,0X01},
    {0XFC9D,0X00},
    {0XFC9E,0X06},
    {0XFC9F,0X00},
    {0XFCA0,0X23},
    {0XFCA1,0X00},
    {0XFCA2,0XF5},
    {0XFCA3,0X02},
    {0XFCA4,0X03},
    {0XFCA5,0X00},
    {0XFCA6,0X05},
    {0XFCA7,0X00},
    {0XFCA8,0X0E},
    {0XFCA9,0X00},
    {0XFCAA,0X06},
    {0XFCAB,0X01},
    {0XFCB1,0X14},
    {0XFCB2,0X00},
    {0XFCB3,0X00},
    {0XFCB4,0X00},
    {0XFCB5,0X00},
    {0XFCB7,0X07},
    {0XFCB8,0X01},
    {0XFCBB,0X37},
    {0XFCBC,0X01},
    {0XFCBD,0X01},
    {0XFCBE,0X00},
    {0XFCBF,0X0C},
    {0XFCC0,0X00},
    {0XFCC1,0X00},
    {0XFCC4,0X00},
    {0XFCC5,0X04},
    {0XFCC6,0X62},
    {0XFCC7,0X03},
    {0XFCC8,0X01},
    {0XFCC9,0X00},
    {0XFCCA,0X06},
    {0XFCCB,0X00},
    {0XFCCC,0X20},
    {0XFCCD,0X00},
    {0XFCCE,0XF2},
    {0XFCCF,0X02},
    {0XFCD1,0X00},
    {0XFCD2,0X08},
    {0XFCD3,0X00},
    {0XFCD4,0X08},
    {0XFCD5,0X00},
    {0XFCD6,0X28},
    {0XFCD7,0X01},
    {0XFCDD,0X14},
    {0XFCDE,0X00},
    {0XFCDF,0X00},
    {0XFCE0,0X00},
    {0XFCE1,0X00},
    {0XFCD0,0X03},
    {0XFCE2,0X00},
    {0XFCB6,0X00},
    {0XFB35,0X00},
    {0XFB89,0X00},
//GAMMA
    {0XFF00,0X03},
    {0XFF01,0X12},
    {0XFF02,0X1F},
    {0XFF03,0X29},
    {0XFF04,0X32},
    {0XFF05,0X3B},
    {0XFF06,0X44},
    {0XFF07,0X4D},
    {0XFF08,0X55},
    {0XFF09,0X5E},
    {0XFF0A,0X66},
    {0XFF0B,0X6E},
    {0XFF0C,0X76},
    {0XFF0D,0X7E},
    {0XFF0E,0X86},
    {0XFF0F,0X8E},
    {0XFF10,0X95},
    {0XFF11,0X9D},
    {0XFF12,0XA5},
    {0XFF13,0XAC},
    {0XFF14,0XB3},
    {0XFF15,0XBA},
    {0XFF16,0XC1},
    {0XFF17,0XC8},
    {0XFF18,0XCF},
    {0XFF19,0XD6},
    {0XFF1A,0XDC},
    {0XFF1B,0XE3},
    {0XFF1C,0XE8},
    {0XFF1D,0XEE},
    {0XFF1E,0XF4},
    {0XFF1F,0XF9},
    {0XFF20,0X12},
    {0XFF21,0X1F},
    {0XFF22,0X29},
    {0XFF23,0X32},
    {0XFF24,0X3B},
    {0XFF25,0X44},
    {0XFF26,0X4D},
    {0XFF27,0X55},
    {0XFF28,0X5E},
    {0XFF29,0X66},
    {0XFF2A,0X6E},
    {0XFF2B,0X76},
    {0XFF2C,0X7E},
    {0XFF2D,0X86},
    {0XFF2E,0X8E},
    {0XFF2F,0X95},
    {0XFF30,0X9D},
    {0XFF31,0XA5},
    {0XFF32,0XAC},
    {0XFF33,0XB3},
    {0XFF34,0XBA},
    {0XFF35,0XC1},
    {0XFF36,0XC8},
    {0XFF37,0XCF},
    {0XFF38,0XD6},
    {0XFF39,0XDC},
    {0XFF3A,0XE3},
    {0XFF3B,0XE8},
    {0XFF3C,0XEE},
    {0XFF3D,0XF4},
    {0XFF3E,0XF9},
    {0XFF3F,0X12},
    {0XFF40,0X1F},
    {0XFF41,0X29},
    {0XFF42,0X32},
    {0XFF43,0X3B},
    {0XFF44,0X44},
    {0XFF45,0X4D},
    {0XFF46,0X55},
    {0XFF47,0X5E},
    {0XFF48,0X66},
    {0XFF49,0X6E},
    {0XFF4A,0X76},
    {0XFF4B,0X7E},
    {0XFF4C,0X86},
    {0XFF4D,0X8E},
    {0XFF4E,0X95},
    {0XFF4F,0X9D},
    {0XFF50,0XA5},
    {0XFF51,0XAC},
    {0XFF52,0XB3},
    {0XFF53,0XBA},
    {0XFF54,0XC1},
    {0XFF55,0XC8},
    {0XFF56,0XCF},
    {0XFF57,0XD6},
    {0XFF58,0XDC},
    {0XFF59,0XE3},
    {0XFF5A,0XE8},
    {0XFF5B,0XEE},
    {0XFF5C,0XF4},
    {0XFF5D,0XF9},
    {0XFF5E,0XFF},
    {0XFF5F,0XFF},
    {0XFF60,0XFF},
};

PanlPosDynPara  AV1_posDynPara[]= {
//dispmode:  16:9  4:3  DM_EX0  DM_EX1  DM_EX2  DM_EX3
//GLOBAL
//PAD MUX
//DECODER
//VP
//TCON
//SCALE
    {0XFC96,{0XDE,0XD4,0XD4,0XD4,0XD4,0XD4}},
    {0XFC97,{0X03,0X03,0X03,0X03,0X03,0X03}},
    {0XFCAC,{0X1E,0X20,0X20,0X20,0X20,0X20}},
    {0XFCAD,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCAE,{0X02,0X02,0X02,0X02,0X02,0X02}},
    {0XFCAF,{0X04,0X04,0X04,0X04,0X04,0X04}},
    {0XFCB0,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCC2,{0XE0,0XE0,0XE0,0XE0,0XE0,0XE0}},
    {0XFCC3,{0X03,0X03,0X03,0X03,0X03,0X03}},
    {0XFCD8,{0X0F,0X0F,0X0F,0X0F,0X0F,0X0F}},
    {0XFCD9,{0X00,0X00,0X00,0X00,0X00,0X00}},
    {0XFCDA,{0X0A,0X0A,0X0A,0X0A,0X0A,0X0A}},
    {0XFCDB,{0X05,0X05,0X05,0X05,0X05,0X05}},
    {0XFCDC,{0X00,0X00,0X00,0X00,0X00,0X00}},
};

PanlstaticPara  AMT_PadMuxStaticPara[]= {
//PAD MUX
    {0XFD32,0X11},
    {0XFD33,0X11},
    {0XFD34,0X00},
    {0XFD35,0X40},
    {0XFD36,0X44},
    {0XFD37,0X44},
    {0XFD38,0X44},
    {0XFD39,0X44},
    {0XFD3A,0X00},
    {0XFD3B,0X00},
    {0XFD3C,0X00},
    {0XFD3D,0X00},
    {0XFD3E,0X00},
    {0XFD3F,0X00},
    {0XFD40,0X00},
    {0XFD41,0X00},
    {0XFD44,0X01},
    {0XFD45,0X00},
    {0XFD46,0X00},
    {0XFD47,0X00},
    {0XFD48,0X00},
    {0XFD49,0X00},
    {0XFD4A,0X00},
    {0XFD4B,0X00},
    {0XFD50,0X09},
};

int ark7116_i2c_write_byte(int fd, uint8_t addr, uint8_t reg, uint8_t val)
{
	return ark_i2c_write(fd, addr, reg, &val, 1);
}

int ark7116_i2c_read_byte(int fd, uint8_t addr, uint8_t reg, uint8_t *val)
{
	return ark_i2c_read(fd, addr, reg, val, 1);
}

void ark7116Reset(void)
{
	//Reset ARK7116
    gpio_export(0);
    gpio_set_dir(0, "out");
    gpio_set_value(0, 0);
    usleep(1000);
    gpio_set_value(0, 1);
    usleep(1000);
    gpio_set_value(0, 0);
    usleep(15000);
}

void  AMT_WriteReg(unsigned short RegAddr,unsigned char RegVal)
{
    unsigned char uctmpDeviceAddr;
    uint8_t addr, reg;

    uctmpDeviceAddr = (unsigned char)((RegAddr>>8)&0XFF);
    reg = (unsigned char)(RegAddr&0XFF);

	switch(uctmpDeviceAddr) {
    case 0XF9:
    case 0XFD:
        addr= 0XB0;
        break;
    case 0XFA:
        addr= 0XBE;
        break;
    case 0XFB:
        addr= 0XB6;
        break;
    case 0XFC:
        addr= 0XB8;
        break;
    case 0XFE:
        addr= 0XB2;
        break;
    case 0XFF:
        addr= 0XB4;
        break;
    case 0X00:
        addr = 0XBE;
        break;
    default:
        addr= 0XB0;
        break;
	}

    ark7116_i2c_write_byte(fd_i2c, addr, reg, RegVal);

 	if(debug_flag) {
        unsigned char valReg;
        ark7116_i2c_read_byte(fd_i2c, addr, reg, &valReg);
        if(valReg != RegVal)
		    printf("valReg =0x%x,RegVal =0x%x \n",valReg,RegVal);
	}
}

unsigned char AMT_ReadReg(unsigned int RegAddr)
{
    unsigned char uctmpDeviceAddr;
    uint8_t addr, reg, regval;

    uctmpDeviceAddr = (unsigned char)((RegAddr>>8)&0XFF);
    reg = (unsigned char)(RegAddr&0XFF);

	switch(uctmpDeviceAddr) {
    case 0XF9:
    case 0XFD:
        addr= 0XB0;
        break;
    case 0XFA:
        addr= 0XBE;
        break;
    case 0XFB:
        addr= 0XB6;
        break;
    case 0XFC:
        addr= 0XB8;
        break;
    case 0XFE:
        addr= 0XB2;
        break;
    case 0XFF:
        addr= 0XB4;
        break;
    case 0X00:
        addr = 0XBE;
        break;
    default:
        addr= 0XB0;
        break;
	}

    ark7116_i2c_read_byte(fd_i2c, addr, reg, &regval);

	return regval;
}

void ConfigSlaveMode(void)
{
    unsigned char AddrBuff[6] = {0xa1,0xa2,0xa3,0xa4,0xa5,0xa6};
    unsigned char DataBuff[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
	unsigned char i;

	printf("+++ConfigSlaveMode! \r\n");

    DataBuff[0] = 0X55;
    DataBuff[1] = 0xAA;
    DataBuff[2] = 0X03;
    DataBuff[3] = 0X50;  //slave mode
    DataBuff[4] = 0;     // crc val
    DataBuff[5] = DataBuff[2]^DataBuff[3]^DataBuff[4];

	AMT_WriteReg(BUS_STATUS_ADDR, 0x00);  //I2c Write Start

	for(i =0;i < 6;i++)
	   AMT_WriteReg(AddrBuff[i], DataBuff[i]);

	AMT_WriteReg(BUS_STATUS_ADDR, 0x11);  //I2c Write End
	usleep(10);

	AMT_WriteReg(0xFAC6, 0x20);

	printf("---ConfigSlaveMode!\r\n");
}

void ConfigStaticPara(void)
{
    int i;

    for (i = 0; i < sizeof(AV1_staticPara) / sizeof(AV1_staticPara[0]); i++)
        AMT_WriteReg(AV1_staticPara[i].addr, AV1_staticPara[i].dat);
}

void ConfigDispZoomDynPara(int currentmode)
{
    int i;

    for (i = 0; i < sizeof(AV1_posDynPara) / sizeof(AV1_posDynPara[0]); i++)
        AMT_WriteReg(AV1_posDynPara[i].addr, AV1_posDynPara[i].dat_posDyn[currentmode]);
}

void ConfigPadMuxPara(void)
{
    int i;

    for (i = 0; i < sizeof(AMT_PadMuxStaticPara) / sizeof(AMT_PadMuxStaticPara[0]); i++)
        AMT_WriteReg(AMT_PadMuxStaticPara[i].addr, AMT_PadMuxStaticPara[i].dat);
}

void InitGlobalPara(void)
{
	printf("InitGlobalPara! \r\n");

	AMT_WriteReg(ENH_PLL,0X20);
	ConfigStaticPara();
	ConfigDispZoomDynPara(DISP_16_9);
	ConfigPadMuxPara();
	AMT_WriteReg(ENH_PLL,0X2C);
}

int ark7116SignalDetect(void)
{
    return ((AMT_ReadReg(0xFE26) & 0x6) == 0x6);
}

int ark7116Init(void)
{
	unsigned char val;

    fd_i2c = open("/dev/i2c-0", O_RDWR);
    if (fd_i2c < 0) {
        printf("open i2c device fail.\n");
        return -1;
    }

    ark7116Reset();

    ConfigSlaveMode();
	//soft reset 7116
	AMT_WriteReg(0xFD00, 0x5A);
	usleep(15000);

	ConfigSlaveMode();

    InitGlobalPara();

	//soft reset decoder
	val = AMT_ReadReg(0xFEA0);
	val |= 1;
	AMT_WriteReg(0xFEA0, val);
	usleep(30000);
	val &= ~1;
	AMT_WriteReg(0xFEA0, val);
	usleep(1000);

    return 0;
}