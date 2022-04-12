#include<linux/gpio.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "ark7116.h"

extern struct ark7116_private_data *g_ark7116_pdata;
static int i2c_debug = 0;

PanlstaticPara AV1_staticPara[]=
{
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
    {0XFE44,0X1C},
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
    {0XFFB0,0X23}, 
    {0XFFB1,0X0F}, 
    {0XFFB2,0X12}, 
    {0XFFB3,0X15}, 
    {0XFFB4,0X15}, 
    {0XFFB7,0X90}, 
    {0XFFB8,0X10}, 
    {0XFFB9,0X62}, 
    {0XFFBA,0X20}, 
    {0XFFBB,0XAA}, 
    {0XFFBC,0X20}, 
    {0XFFBD,0X20}, 
    {0XFFC7,0X31}, 
    {0XFFC8,0X06}, 
    {0XFFC9,0X08}, 
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
    {0XFFF0,0X4C}, 
    {0XFFF1,0XE8}, 
    {0XFFF2,0XE8}, 
    {0XFFF3,0XD7}, 
    {0XFFF4,0XFD}, 
    {0XFFF5,0X44}, 
    {0XFFF6,0XFA}, 
    {0XFFF7,0XE4}, 
    {0XFFF8,0XED}, 
    {0XFFF9,0XFD}, 
    {0XFFFA,0X4C}, 
    {0XFFFB,0X81}, 
    {0XFFD5,0X00}, 
    {0XFFD6,0X40}, 

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
    {0XFF01,0X2A}, 
    {0XFF02,0X3C}, 
    {0XFF03,0X4A}, 
    {0XFF04,0X57}, 
    {0XFF05,0X63}, 
    {0XFF06,0X6E}, 
    {0XFF07,0X78}, 
    {0XFF08,0X80}, 
    {0XFF09,0X88}, 
    {0XFF0A,0X91}, 
    {0XFF0B,0X99}, 
    {0XFF0C,0XA1}, 
    {0XFF0D,0XA9}, 
    {0XFF0E,0XB0}, 
    {0XFF0F,0XB6}, 
    {0XFF10,0XBC}, 
    {0XFF11,0XC2}, 
    {0XFF12,0XC7}, 
    {0XFF13,0XCC}, 
    {0XFF14,0XD0}, 
    {0XFF15,0XD4}, 
    {0XFF16,0XD8}, 
    {0XFF17,0XDC}, 
    {0XFF18,0XE1}, 
    {0XFF19,0XE5}, 
    {0XFF1A,0XE9}, 
    {0XFF1B,0XEC}, 
    {0XFF1C,0XF0}, 
    {0XFF1D,0XF4}, 
    {0XFF1E,0XF8}, 
    {0XFF1F,0XFB}, 
    {0XFF20,0X2A}, 
    {0XFF21,0X3C}, 
    {0XFF22,0X4A}, 
    {0XFF23,0X57}, 
    {0XFF24,0X63}, 
    {0XFF25,0X6E}, 
    {0XFF26,0X78}, 
    {0XFF27,0X80}, 
    {0XFF28,0X88}, 
    {0XFF29,0X91}, 
    {0XFF2A,0X99}, 
    {0XFF2B,0XA1}, 
    {0XFF2C,0XA9}, 
    {0XFF2D,0XB0}, 
    {0XFF2E,0XB6}, 
    {0XFF2F,0XBC}, 
    {0XFF30,0XC2}, 
    {0XFF31,0XC7}, 
    {0XFF32,0XCC}, 
    {0XFF33,0XD0}, 
    {0XFF34,0XD4}, 
    {0XFF35,0XD8}, 
    {0XFF36,0XDC}, 
    {0XFF37,0XE1}, 
    {0XFF38,0XE5}, 
    {0XFF39,0XE9}, 
    {0XFF3A,0XEC}, 
    {0XFF3B,0XF0}, 
    {0XFF3C,0XF4}, 
    {0XFF3D,0XF8}, 
    {0XFF3E,0XFB}, 
    {0XFF3F,0X2A}, 
    {0XFF40,0X3C}, 
    {0XFF41,0X4A}, 
    {0XFF42,0X57}, 
    {0XFF43,0X63}, 
    {0XFF44,0X6E}, 
    {0XFF45,0X78}, 
    {0XFF46,0X80}, 
    {0XFF47,0X88}, 
    {0XFF48,0X91}, 
    {0XFF49,0X99}, 
    {0XFF4A,0XA1}, 
    {0XFF4B,0XA9}, 
    {0XFF4C,0XB0}, 
    {0XFF4D,0XB6}, 
    {0XFF4E,0XBC}, 
    {0XFF4F,0XC2}, 
    {0XFF50,0XC7}, 
    {0XFF51,0XCC}, 
    {0XFF52,0XD0}, 
    {0XFF53,0XD4}, 
    {0XFF54,0XD8}, 
    {0XFF55,0XDC}, 
    {0XFF56,0XE1}, 
    {0XFF57,0XE5}, 
    {0XFF58,0XE9}, 
    {0XFF59,0XEC}, 
    {0XFF5A,0XF0}, 
    {0XFF5B,0XF4}, 
    {0XFF5C,0XF8}, 
    {0XFF5D,0XFB}, 
    {0XFF5E,0XFF}, 
    {0XFF5F,0XFF}, 
    {0XFF60,0XFF}, 
};
PanlPosDynPara  AV1_posDynPara[]=
{

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
PanlSysDynPara  AV1_sysDynPara[]=
{

//picSys:   PAL  PAL-N  PAL-M  NTSC SECAM PAL-60 NTSC-J NTSC-4.43
//GLOBAL
//PAD MUX
//DECODER
//VP
    {0XFFD3,{0X82, 0X80, 0X80, 0X82, 0X80, 0X80, 0X80, 0X80}},
    {0XFFD4,{0X77, 0X80, 0X80, 0X77, 0X80, 0X80, 0X80, 0X80}},
    {0XFFD5,{0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00}},
    {0XFFD6,{0X40, 0X36, 0X36, 0X40, 0X36, 0X36, 0X36, 0X36}},
//TCON
//SCALE
//GAMMA
};

/*µãÆÁ PAD MUX ²ÎÊý*/
PanlstaticPara  AMT_PadMuxStaticPara[]=
{
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


unsigned char device_addr_convert(unsigned char device_addr)
{
	 unsigned char addr;

	 switch(device_addr)
	 {
	    case 0XF9:
	    case 0XFD:
			 addr = 0XB0;
			 break;

		case 0XFA:
			 addr = 0XBE;
			 break;
			 	
		case 0XFB:
			 addr = 0XB6;
			 break;

	    case 0XFC:
			 addr = 0XB8;
			 break;

		case 0XFE:
			 addr = 0XB2;
			 break;

		case 0XFF:
			 addr = 0XB4;
			 break;

	   	case 0X00:
			 addr = 0XBE;
			 break;
			
		default:
			 addr = 0XB0;
			 break;			
	 }	
   
	return addr;
}

static unsigned char amt_read_reg(unsigned int device_reg)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg read_msgs[2];
	s32 ret = -1;
	s32 retries = 0;
	u8 reg_val = 0x00;
        u8 device_addr;
        u8 reg_addr;

	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;

        device_addr = device_addr_convert(device_reg>>8 & 0XFF);
        reg_addr = (u8)(device_reg & 0XFF);

        read_msgs[0].flags = !I2C_M_RD;
        //read_msgs[0].addr  = 0xB2>>1;
        read_msgs[0].addr  = device_addr >> 1;//client->addr;
        read_msgs[0].len   = 1;
        read_msgs[0].buf   = &reg_addr;

        read_msgs[1].flags = I2C_M_RD;
        //read_msgs[1].addr  = 0xB2>>1;
        read_msgs[1].addr  = device_addr >> 1;//client->addr;
        read_msgs[1].len   = 1;
        read_msgs[1].buf   = &reg_val;//low byte

        while(retries < 5){
                ret = i2c_transfer(client->adapter, read_msgs, 2);
                if(ret == 2)break;
                        retries++;
        }
        //printk("regValue=%d.\n", regValue);
        if (reg_val == 0xFF) reg_val = 0;
        if((retries >= 5)){
        	printk("ark7116_read_byte_data write error\n");
        }
    
    //gpioi2c_print("device_reg=0x%x-->0x%x,, regValue=0x%x \n",device_reg, device_addr,reg_val);

	return reg_val;

}

static int amt_write_reg(unsigned short device_reg,unsigned char reg_val)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
        u8 device_addr;
        u8 reg_addr;
	
	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;

        device_addr = device_addr_convert(device_reg>>8 & 0XFF);
        reg_addr = (u8)(device_reg & 0XFF);

	buf[0] = reg_addr;
	buf[1] = reg_val;

	msg.flags = 0;
	//msg.addr  = 0xB2>>1;
	msg.addr  = device_addr >> 1;//client->addr;
	msg.len   = 2;
	msg.buf   = buf;

	while(retries < 5){
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)break;
		retries++;
	}
	if((retries >= 5)){
		printk("ark7116_write_byte  write error\n");
	}

        if(i2c_debug){
		unsigned char val; 
	 	val = amt_read_reg(device_reg);
		if(val != reg_val)
		   gpioi2c_print("device_reg=0x%x-->0x%x,, val=0x%x, regval=0x%x \n",device_reg, device_addr,val,reg_val);
	}
    
	return ret;
}


static int amt_write_static_para(PanlstaticPara * dataPt,unsigned int num)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
        u8 device_addr;
        u8 reg_addr;
	
	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;
    
        while(num--){
                device_addr = device_addr_convert((((*dataPt).addr)>>8)&0XFF);
                reg_addr = (u8)(((*dataPt).addr)&0XFF);

            	buf[0] = reg_addr;
            	buf[1] = (*dataPt).dat;

            	msg.flags = 0;
            	//msg.addr  = 0xB2>>1;
            	msg.addr  = device_addr >> 1;//client->addr;
            	msg.len   = 2;
            	msg.buf   = buf;

            	while(retries < 5){
            		ret = i2c_transfer(client->adapter, &msg, 1);
            		if (ret == 1)break;
            		retries++;
            	}
            	if((retries >= 5)){
            		printk("ark7116_write_byte  write error\n");
            	}

                if(i2c_debug){
            		unsigned char val; 
            	 	val = amt_read_reg((*dataPt).addr);
            		if(val != (*dataPt).dat)
            		   gpioi2c_print("device_reg=0x%x-->0x%x,, val=0x%x, regval=0x%x \n",(*dataPt).addr, device_addr,val,(*dataPt).dat);
            	}
                
                dataPt++;
        }
	
	return ret;
}

static int amt_write_disp_zoom_dyn_para(PanlPosDynPara *dataPt,unsigned int num,unsigned char currentmode)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
        u8 device_addr;
        u8 reg_addr;
	
	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;
    
        while(num--){
                device_addr = device_addr_convert((((*dataPt).addr)>>8)&0XFF);
                reg_addr = (u8)(((*dataPt).addr)&0XFF);

        	buf[0] = reg_addr;
        	buf[1] = (*dataPt).dat_posDyn[currentmode];

        	msg.flags = 0;
        	//msg.addr  = 0xB2>>1;
        	msg.addr  = device_addr >> 1;//client->addr;
        	msg.len   = 2;
        	msg.buf   = buf;

        	while(retries < 5){
        		ret = i2c_transfer(client->adapter, &msg, 1);
        		if (ret == 1)break;
        		retries++;
        	}
        	if(retries >= 5){
        		printk("ark7116_write_byte  write error\n");
        	}

                if(i2c_debug){
        		unsigned char val; 
        	 	val = amt_read_reg((*dataPt).addr);
        		if(val != (*dataPt).dat_posDyn[currentmode])
        		   gpioi2c_print("device_reg=0x%x-->0x%x, val=0x%x, regval=0x%x \n",(*dataPt).addr, device_addr,val,(*dataPt).dat_posDyn[currentmode]);
        	}
                dataPt++;
        }
	
	return ret;
}

static int amt_write_color_sys_dyn_para(PanlSysDynPara * dataPt,unsigned int num,unsigned char currentSys)
{
	struct ark7116_private_data *ark7116_pdata = g_ark7116_pdata;
	struct i2c_client *client;
	struct i2c_msg msg;
	s32 ret = -1;
	s32 retries = 0;
	u8 buf[2] = {0};
        u8 device_addr;
        u8 reg_addr;
	
	if(!ark7116_pdata)
		return -ENODEV;
	client = ark7116_pdata->client;
	if(!client)
		return -ENODEV;
    
        while(num--){
                device_addr = device_addr_convert((((*dataPt).addr)>>8)&0XFF);
                reg_addr = (u8)(((*dataPt).addr)&0XFF);

        	buf[0] = reg_addr;
        	buf[1] = (*dataPt).dat_sysDyn[currentSys];
        	
        	msg.flags = 0;
        	//msg.addr  = 0xB2>>1;
        	msg.addr  = device_addr >> 1;//client->addr;
        	msg.len   = 2;
        	msg.buf   = buf;

        	while(retries < 5){
        		ret = i2c_transfer(client->adapter, &msg, 1);
        		if (ret == 1)break;
        		retries++;
        	}
        	if(retries >= 5){
        		printk("ark7116_write_byte  write error\n");
        	}

                if(i2c_debug){
        		unsigned char val; 
        	 	val = amt_read_reg((*dataPt).addr);
        		if(val != (*dataPt).dat_sysDyn[currentSys])
        		   gpioi2c_print("device_reg=0x%x-->0x%x, val=0x%x, regval=0x%x \n",(*dataPt).addr, device_addr,val,(*dataPt).dat_sysDyn[currentSys]);
        	}
                        
                dataPt++;
        }
	
	return ret;
}

static void config_slave_mode(void)
{   
        unsigned char AddrBuff[6] = {0xa1,0xa2,0xa3,0xa4,0xa5,0xa6};
        unsigned char DataBuff[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
        unsigned char i;

        gpioi2c_print("+++%s.\n",__FUNCTION__);

        DataBuff[0] = 0X55;
        DataBuff[1] = 0xAA;
        DataBuff[2] = 0X03;
        DataBuff[3] = 0X50;  //slave mode
        DataBuff[4] = 0;     // crc val
        DataBuff[5] = DataBuff[2]^DataBuff[3]^DataBuff[4];

        amt_write_reg(BUS_STATUS_ADDR, 0x00);  //I2c Write Start

        for(i =0;i < 6;i++)
        {
                amt_write_reg(AddrBuff[i], DataBuff[i]);
        }
        amt_write_reg(BUS_STATUS_ADDR, 0x11);  //I2c Write End
        udelay(10);

        amt_write_reg(0xFAC6, 0x20);

        gpioi2c_print("---%s.\n",__FUNCTION__);
    
}

static void config_static_para(unsigned char curret_source)
{    
        gpioi2c_print("%s.\n",__FUNCTION__);
        amt_write_static_para(AV1_staticPara, sizeof(AV1_staticPara) / sizeof(AV1_staticPara[0]));
}


static void config_pad_mux_para(void)
{   
        gpioi2c_print("%s.\n",__FUNCTION__);
        amt_write_static_para(AMT_PadMuxStaticPara, sizeof(AMT_PadMuxStaticPara) / sizeof(AMT_PadMuxStaticPara[0]));
}

#if 0
static void config_color_sys_dyn_para(unsigned char current_sys) 
{    
        gpioi2c_print("%s.\n",__FUNCTION__);

        amt_write_color_sys_dyn_para(AV1_sysDynPara, sizeof(AV1_sysDynPara) / sizeof(AV1_sysDynPara[0]), current_sys);
}
#endif

static void config_disp_zoom_dyn_para(unsigned char currentmode) 
{    
        gpioi2c_print("%s.\n",__FUNCTION__);

        amt_write_disp_zoom_dyn_para(AV1_posDynPara, sizeof(AV1_posDynPara) / sizeof(AV1_posDynPara[0]), currentmode);
}


static void init_global_para(void)
{	
        gpioi2c_print("%s.\n",__FUNCTION__);

        amt_write_reg(ENH_PLL,0X20);
        config_static_para(INPUT_AV1); 
        config_disp_zoom_dyn_para(DISP_16_9);
        //config_color_sys_dyn_para(PAL);
        config_pad_mux_para(); 
        amt_write_reg(ENH_PLL,0X2C);
}

static void ark7116_reset(void)
{
        int reset_gpio = g_ark7116_pdata->gpio_reset;

        gpio_direction_output(reset_gpio, 0);
        gpio_set_value(reset_gpio, 0);
        mdelay(1);
        gpio_set_value(reset_gpio, 1);
        mdelay(100);//mdelay(1);
        gpio_set_value(reset_gpio, 0);
        mdelay(15);
}

int  ark7116_config(void)
{     
        unsigned char val,slave_mode,rom_sel;
        int times = 10;
        
        gpioi2c_print("ark169 %s.\n",__FUNCTION__);
        if(g_ark7116_pdata->config_finish){
                return 0;
        }

reset_ark7116:
        
        ark7116_reset();
        config_slave_mode();
        //soft reset 7116
        amt_write_reg(0xFD00, 0x5A);
        mdelay(15);

        config_slave_mode();
        val = amt_read_reg(0xFAC6);
        slave_mode = (val & 0x80)?0:1;
        rom_sel    = (val & 0x02)?1:0;
        
        printk(KERN_ALERT "\n ark7116 slave status=0x%0x.slave_mode=%d,rom_sel=%d----->\n",val,slave_mode,rom_sel);
        if(rom_sel || !slave_mode){
                if(times-- > 0){
                        goto reset_ark7116;
                }else{
                        printk(KERN_ALERT "ark7116 slave config error.\n");
                        return -1;
                }
        }

        init_global_para();

        //soft reset decoder
        val = amt_read_reg(0xFEA0);
        val |= 1;
        amt_write_reg(0xFEA0, val);
        mdelay(30);
        val &= ~1;
        amt_write_reg(0xFEA0, val);
        mdelay(1);
        
        ///////////////////////////////////////
        amt_write_reg(ENH_PLL,0X28);
        mdelay(200);
        amt_write_reg(ENH_PLL,0X2C);
        g_ark7116_pdata->config_finish = 1; 
        return 0;
}

/* int amt_detect_signal(void)
{
        return ((amt_read_reg(0xFE26) & 0x6) == 0x6);
}*/

