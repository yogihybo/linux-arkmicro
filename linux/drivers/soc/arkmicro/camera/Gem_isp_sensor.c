// Gem_sensor.c

#include "Gem_isp_sensor.h"
#include <stdio.h>
#include <rtos.h>

int isp_sen_id_check(void)
{
    int id; 
    id = i2c_reg_read (SEN_I2C_ADDR, ID_H_REG);
    id = (id << 8) | i2c_reg_read(SEN_I2C_ADDR, ID_L_REG);
    printf("\r\nFound Sensor id：0x%04x\n", id);
    if (id == OV9712_ID)
    {
       printf("Senosr OV9712... \n");	
    }
    else if (id == OV2643_ID)
    {
       printf("Senosr OV2643... \n");	
    }
    else if (id == JX_H22_ID)
    {
       printf("Senosr JX_H22... \n");	
    }
    else if (id == PP1210K_ID)
    {
       printf("\r\nSenosr PixPlus1210K...\n"); 	
    }
    else
    {
       printf("Senosr Unknown... \n"); 	
    }
    
    return 0;
}

// sensor ov9712 API
// -----------------

int isp_sen_config_OV9712 (void)
{
  int i,j;
  unsigned char val;
  int times = 20;

  if (isp_sen_id_check() != 0)
  {
    return -1;
  }

  //640x480--------------------------------------
/*  
  i2c_reg_write(SEN_I2C_ADDR,0x12, 0x80); //Reset
  i2c_reg_write(SEN_I2C_ADDR,0x09, 0x10); //disable sleep mode    
  i2c_reg_write(SEN_I2C_ADDR,0x1e, 0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x5f, 0x18);
  i2c_reg_write(SEN_I2C_ADDR,0x69, 0x04);
  i2c_reg_write(SEN_I2C_ADDR,0x65, 0x2a);
  i2c_reg_write(SEN_I2C_ADDR,0x68, 0x0a);
  i2c_reg_write(SEN_I2C_ADDR,0x39, 0x28);
  i2c_reg_write(SEN_I2C_ADDR,0x4d, 0x90);
  i2c_reg_write(SEN_I2C_ADDR,0xc1, 0x80);
  i2c_reg_write(SEN_I2C_ADDR,0x0c, 0x30);
  i2c_reg_write(SEN_I2C_ADDR,0x6d, 0x02);    
  i2c_reg_write(SEN_I2C_ADDR,0x96, 0x01);	
  i2c_reg_write(SEN_I2C_ADDR,0xbc, 0x68);
  i2c_reg_write(SEN_I2C_ADDR,0x12, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x3b, 0x00);	
  i2c_reg_write(SEN_I2C_ADDR,0x97, 0x80);	  // 0x88:bar 0x80:normal  
  i2c_reg_write(SEN_I2C_ADDR,0x17, 0x25);
  i2c_reg_write(SEN_I2C_ADDR,0x18, 0xA2);
  i2c_reg_write(SEN_I2C_ADDR,0x19, 0x01);
  i2c_reg_write(SEN_I2C_ADDR,0x1a, 0xCA);
  i2c_reg_write(SEN_I2C_ADDR,0x03, 0x0A);
  i2c_reg_write(SEN_I2C_ADDR,0x32, 0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x98, 0x40);
  i2c_reg_write(SEN_I2C_ADDR,0x99, 0xA0);
  i2c_reg_write(SEN_I2C_ADDR,0x9a, 0x01);
  i2c_reg_write(SEN_I2C_ADDR,0x57, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x58, 0x78);
  i2c_reg_write(SEN_I2C_ADDR,0x59, 0x50);
  i2c_reg_write(SEN_I2C_ADDR,0x4c, 0x13);
  i2c_reg_write(SEN_I2C_ADDR,0x4b, 0x36);
  i2c_reg_write(SEN_I2C_ADDR,0x3d, 0x3c);
  i2c_reg_write(SEN_I2C_ADDR,0x3e, 0x03);
  i2c_reg_write(SEN_I2C_ADDR,0xbd, 0x50);
  i2c_reg_write(SEN_I2C_ADDR,0xbe, 0x78);  
  i2c_reg_write(SEN_I2C_ADDR,0x4e, 0x55);	
  i2c_reg_write(SEN_I2C_ADDR,0x4f, 0x55);			
  i2c_reg_write(SEN_I2C_ADDR,0x50, 0x55);	
  i2c_reg_write(SEN_I2C_ADDR,0x51, 0x55);	
  i2c_reg_write(SEN_I2C_ADDR,0x24, 0x55);	
  i2c_reg_write(SEN_I2C_ADDR,0x25, 0x40);
  i2c_reg_write(SEN_I2C_ADDR,0x26, 0xa1);    
  i2c_reg_write(SEN_I2C_ADDR,0x5c, 0x1c);   // bit [4:0] multiplier
  i2c_reg_write(SEN_I2C_ADDR,0x5d, 0x12);   // bit [5:4] driver capability
  i2c_reg_write(SEN_I2C_ADDR,0x11, 0x01);
  i2c_reg_write(SEN_I2C_ADDR,0x2a, 0x8c);	
  i2c_reg_write(SEN_I2C_ADDR,0x2b, 0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x2d, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x2e, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x13, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x0e, 0x40); // [7] AEC OPEN
  i2c_reg_write(SEN_I2C_ADDR,0x03, 0x0A); // [7:5] 鏈�浣嶧PS
  i2c_reg_write(SEN_I2C_ADDR,0x14, 0x00); // Gain Ceiling 8X
  i2c_reg_write(SEN_I2C_ADDR,0x13, 0x20);
  i2c_reg_write(SEN_I2C_ADDR,0x10, 0x40);
  i2c_reg_write(SEN_I2C_ADDR,0x16, 0x01);
  i2c_reg_write(SEN_I2C_ADDR,0x49, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x4a, 0x02);
  //i2c_reg_write(SEN_I2C_ADDR,0x00, 0x00);
  //i2c_reg_write(SEN_I2C_ADDR,0x04, 0x80); //flip
*/  
                
  //1280x800 ArkMicro   OV9712
  //reset 
  i2c_reg_write(SEN_I2C_ADDR,0x12,0x80);
  OS_Delay(1);
  i2c_reg_write(SEN_I2C_ADDR,0x09,0x10);  
  OS_Delay(1);
  //Core Settings     OV9712
  i2c_reg_write(SEN_I2C_ADDR,0x1e,0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x5f,0x18);
  i2c_reg_write(SEN_I2C_ADDR,0x69,0x04);
  i2c_reg_write(SEN_I2C_ADDR,0x65,0x2a);
  i2c_reg_write(SEN_I2C_ADDR,0x68,0x0a);
  i2c_reg_write(SEN_I2C_ADDR,0x39,0x28);
  i2c_reg_write(SEN_I2C_ADDR,0x4d,0x90);
  i2c_reg_write(SEN_I2C_ADDR,0xc1,0x80);
  i2c_reg_write(SEN_I2C_ADDR,0x6d,0x02);
  //i2c_reg_write(SEN_I2C_ADDR,0x96,0xf1); // DSP options enable
  i2c_reg_write(SEN_I2C_ADDR,0xbc,0x68);
       
  i2c_reg_write(SEN_I2C_ADDR,0x12,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x3b,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x97,0x80); // color bar
  i2c_reg_write(SEN_I2C_ADDR,0x17,0x25);
  i2c_reg_write(SEN_I2C_ADDR,0x18,0xA2);
  i2c_reg_write(SEN_I2C_ADDR,0x19,0x01);
  i2c_reg_write(SEN_I2C_ADDR,0x1a,0xCA);
  i2c_reg_write(SEN_I2C_ADDR,0x32,0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x98,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x99,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x9a,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x57,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x58,0xC8);//0xC8
  i2c_reg_write(SEN_I2C_ADDR,0x59,0xA0);
  i2c_reg_write(SEN_I2C_ADDR,0x4c,0x13);
  i2c_reg_write(SEN_I2C_ADDR,0x4b,0x36);
  i2c_reg_write(SEN_I2C_ADDR,0x3d,0x3C);
  i2c_reg_write(SEN_I2C_ADDR,0x3e,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0xbd,0xA0);
  i2c_reg_write(SEN_I2C_ADDR,0xbe,0xC8);
  
  i2c_reg_write(SEN_I2C_ADDR,0x10,0xff);   
  i2c_reg_write(SEN_I2C_ADDR,0x4e,0x55);// AVERAGE
  i2c_reg_write(SEN_I2C_ADDR,0x4f,0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x50,0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x51,0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x24,0x30);// Exposure windows
  i2c_reg_write(SEN_I2C_ADDR,0x25,0x20);
  i2c_reg_write(SEN_I2C_ADDR,0x26,0xa1);
  
  //Clock    OV9712
  //i2c_reg_write(SEN_I2C_ADDR,0x5c,0x5c);//   pclk
  i2c_reg_write(SEN_I2C_ADDR,0x5c,0x1c);// *M pclk
  // 
  
  
#if 1
  i2c_reg_write(SEN_I2C_ADDR,0x5d,0x10);// bit [5:4] driver capability      pclk=1/2主时钟
#else
  i2c_reg_write(SEN_I2C_ADDR,0x5d,0x40);// bit.6 BYPASS PLL               pclk=mclk
#endif
  i2c_reg_write(SEN_I2C_ADDR,0x11,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x2a,0x8c);
  i2c_reg_write(SEN_I2C_ADDR,0x2b,0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x2d,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x2e,0x00);

  //General     OV9712
  i2c_reg_write(SEN_I2C_ADDR,0x13,0xa5);// 自动曝光
  //i2c_reg_write(SEN_I2C_ADDR,0x13,0x80);// 手动曝光  
  i2c_reg_write(SEN_I2C_ADDR,0x14,0x40);// Gain Ceiling 8X

  //Banding  OV9712
  i2c_reg_write(SEN_I2C_ADDR,0x4a,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x49,0xce);
  i2c_reg_write(SEN_I2C_ADDR,0x22,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x09,0x00);
  
	//close AE_AWB  OV9712
#if 0
  i2c_reg_write(SEN_I2C_ADDR,0x13, 0x80);//打开手动曝光
  i2c_reg_write(SEN_I2C_ADDR,0x16, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x10, 0xf0);
  i2c_reg_write(SEN_I2C_ADDR,0x00, 0x3f);
  i2c_reg_write(SEN_I2C_ADDR,0x38, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x01, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x02, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x05, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x06, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x07, 0x00);

#else
  i2c_reg_write(SEN_I2C_ADDR,0x13, 0x8d);//打开自动曝光
  i2c_reg_write(SEN_I2C_ADDR,0x16, 0x00);//v
  i2c_reg_write(SEN_I2C_ADDR,0x10, 0xff);// v
  //i2c_reg_write(SEN_I2C_ADDR,0x00, 0x3f);
  //i2c_reg_write(SEN_I2C_ADDR,0x38, 0x00);
  //i2c_reg_write(SEN_I2C_ADDR,0x01, 0x40);
  //i2c_reg_write(SEN_I2C_ADDR,0x02, 0x40);
  i2c_reg_write(SEN_I2C_ADDR,0x05, 0x40);
  i2c_reg_write(SEN_I2C_ADDR,0x06, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x07, 0x00);

#endif
  //BLC    OV9712
   
  i2c_reg_write(SEN_I2C_ADDR,0x41, 0x84);

  /*
  // Novatek
  i2c_reg_write(SEN_I2C_ADDR,0x96, 0x41);
  i2c_reg_write(SEN_I2C_ADDR,0xBC, 0x68);
  i2c_reg_write(SEN_I2C_ADDR,0x12, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x3B, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x97, 0x80);
  i2c_reg_write(SEN_I2C_ADDR,0x15, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x17, 0x25);
  i2c_reg_write(SEN_I2C_ADDR,0x18, 0xA2);
  i2c_reg_write(SEN_I2C_ADDR,0x19, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x1A, 0xCA);
  i2c_reg_write(SEN_I2C_ADDR,0x03, 0x0A);
  i2c_reg_write(SEN_I2C_ADDR,0x32, 0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x98, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x99, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x9A, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x57, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x58, 0xC8);
  i2c_reg_write(SEN_I2C_ADDR,0x59, 0xA2);
  i2c_reg_write(SEN_I2C_ADDR,0x4C, 0x13);
  i2c_reg_write(SEN_I2C_ADDR,0x4B, 0x36);
  i2c_reg_write(SEN_I2C_ADDR,0x3D, 0x3C);
  i2c_reg_write(SEN_I2C_ADDR,0x3E, 0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x4E, 0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x4F, 0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x50, 0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x51, 0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x24, 0x48);
  i2c_reg_write(SEN_I2C_ADDR,0x25, 0x38);
  i2c_reg_write(SEN_I2C_ADDR,0x26, 0x92);
  
  i2c_reg_write(SEN_I2C_ADDR,0x5c,0x1c);// 48m pclk
  i2c_reg_write(SEN_I2C_ADDR,0x5d,0x10);// bit [5:4] driver capability
  i2c_reg_write(SEN_I2C_ADDR,0x11,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x2a,0x8c);
  i2c_reg_write(SEN_I2C_ADDR,0x2b,0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x2d,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x2e,0x00);
  
  i2c_reg_write(SEN_I2C_ADDR,0x41, 0x81);
  i2c_reg_write(SEN_I2C_ADDR,0x13, 0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x14, 0x00);//0x40                        
  // General setting                         
  i2c_reg_write(SEN_I2C_ADDR,0x1E, 0x07);
  i2c_reg_write(SEN_I2C_ADDR,0x5F, 0x18);
  i2c_reg_write(SEN_I2C_ADDR,0x69, 0x04);
  i2c_reg_write(SEN_I2C_ADDR,0x65, 0x2A);
  i2c_reg_write(SEN_I2C_ADDR,0x68, 0x0A);
  i2c_reg_write(SEN_I2C_ADDR,0x39, 0x28);
  i2c_reg_write(SEN_I2C_ADDR,0x4D, 0x90);
  i2c_reg_write(SEN_I2C_ADDR,0xC1, 0x80);
  i2c_reg_write(SEN_I2C_ADDR,0x2A, 0x8c);
  i2c_reg_write(SEN_I2C_ADDR,0x96, 0x01); //{0x96, 0xC1},
  i2c_reg_write(SEN_I2C_ADDR,0x6D, 0x02);
  */

  return 0;

}

int isp_sen_config_PP1210K (void)
{
  int temp;
  unsigned long count = 0;

  if (isp_sen_id_check() != 0)
  {
    return -1;
  }
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300// select GROUP A
  i2c_reg_write(SEN_I2C_ADDR,0x24,0x01);//W0300// soft reset
  OS_Delay(1);
  //#################### start up ####################
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300
  i2c_reg_write(SEN_I2C_ADDR,0x29,0x98);//W2998	# output Hi-z release
  
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x00);//W0300
  i2c_reg_write(SEN_I2C_ADDR,0x05,0x00);//W0503	# mirror/flip
  //i2c_reg_write(SEN_I2C_ADDR,0x05,0x02 );//W0503	# mirror/flip
  i2c_reg_write(SEN_I2C_ADDR,0x06,0x08);//W0608	# framewidth_h        (08)
  i2c_reg_write(SEN_I2C_ADDR,0x07,0x97);//W0797	# framewidth_l        (97)
	i2c_reg_write(SEN_I2C_ADDR,0x08,0x04);
	i2c_reg_write(SEN_I2C_ADDR,0x09,0x74);

i2c_reg_write(SEN_I2C_ADDR,0x0c,0x00);//W0C01	# windowx1_h					(00)
i2c_reg_write(SEN_I2C_ADDR,0x0d,0x0d);//W0D4E	# windowx1_l					(01)
i2c_reg_write(SEN_I2C_ADDR,0x0e,0x00);//W0E00	# windowy1_h					(00)
i2c_reg_write(SEN_I2C_ADDR,0x0f,0x0e);//W0FC2   # windowy1_l					(02)
i2c_reg_write(SEN_I2C_ADDR,0x10,0x07);//W1006	# windowx2_h					(07)
i2c_reg_write(SEN_I2C_ADDR,0x11,0x8c);//W114D	# windowx2_l					(8C)
i2c_reg_write(SEN_I2C_ADDR,0x12,0x04);//W1203	# windowy2_h					(04)
i2c_reg_write(SEN_I2C_ADDR,0x13,0x45);//W1391	# windowy2_l					(45)

i2c_reg_write(SEN_I2C_ADDR,0x14,0x00);//W1400	# vsyncstartrow_f0_h	(00)
i2c_reg_write(SEN_I2C_ADDR,0x15,0x19);//W1519	# vsyncstartrow_f0_l	(0D)
i2c_reg_write(SEN_I2C_ADDR,0x15,0x04);//W1604	# vsyncstoprow_f0_h		(04)
i2c_reg_write(SEN_I2C_ADDR,0x17,0x53);//W1753	# vsyncstoprow_f0_l		(53)

i2c_reg_write(SEN_I2C_ADDR,0x25,0x13);//W2510 # CLK DIV1

i2c_reg_write(SEN_I2C_ADDR,0x33,0x01);//W3301 # pixelbias
i2c_reg_write(SEN_I2C_ADDR,0x34,0x02);//W3402 # compbias

i2c_reg_write(SEN_I2C_ADDR,0x36,0xc8);//W36C8 # TX_Bias; DCDC 4.96 V, LDO 4.37 V
i2c_reg_write(SEN_I2C_ADDR,0x38,0x48);//W3848 # black_bias, range_sel 0.4 V
i2c_reg_write(SEN_I2C_ADDR,0x3a,0x22);//W3A22 # main regulator output

//i2c_reg_write(SEN_I2C_ADDR,0x41,0x06);//W4121 # pll_m_cnt (21)
i2c_reg_write(SEN_I2C_ADDR,0x41,0x08);//0x41:08=3.66f/s//W4121 # pll_m_cnt (21)
i2c_reg_write(SEN_I2C_ADDR,0x42,0x01);//W4208 # pll_r_cnt (04)

i2c_reg_write(SEN_I2C_ADDR,0x40,0x10);//W4010 # pll_control
//for(count=0;count<100000;count++);      //$0500	# Delay ------- 500ms here
 OS_Delay(20);                                     
i2c_reg_write(SEN_I2C_ADDR,0x40,0x00);//W4000 # pll_control on

i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
i2c_reg_write(SEN_I2C_ADDR,0x26,0x03);//W2603 # blacksun_th_h

i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
//i2c_reg_write(SEN_I2C_ADDR,0xc0,0x01);//WC004 # inttime_h  
//i2c_reg_write(SEN_I2C_ADDR,0xc1,0xb8);//WC15F # inttime_m  
//i2c_reg_write(SEN_I2C_ADDR,0xc2,0x00);//WC200 # inttime_l  
i2c_reg_write(SEN_I2C_ADDR,0xc0,0x00);//WC004 # inttime_h  
i2c_reg_write(SEN_I2C_ADDR,0xc1,0x03);//WC15F # inttime_m  
i2c_reg_write(SEN_I2C_ADDR,0xc2,0x60);//WC200 # inttime_l  

i2c_reg_write(SEN_I2C_ADDR,0xc3,0x00);//WC300 # globalgain 
i2c_reg_write(SEN_I2C_ADDR,0xc4,0x40);//WC440 # digitalgain
//for(count=0;count<500000;count++);  
 OS_Delay(20);   
return 0;
}


int isp_sen_config_OV2643 (void)
{
  if (isp_sen_id_check() != 0)
  {
    return -1;
  }

  return 0;
}

int isp_sen_config_MT9P031 (void)
{
  if (isp_sen_id_check() != 0)
  {
    return -1;
  }

  return 0;
}

int isp_sen_config_JXH22 (void)
{
  if (isp_sen_id_check() != 0)
  {
    return -1;
  }

  i2c_reg_write(SEN_I2C_ADDR,0x12,0x40);
  i2c_reg_write(SEN_I2C_ADDR,0x00,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x01,0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x02,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x0E,0x10); //0x19
  i2c_reg_write(SEN_I2C_ADDR,0x0F,0x02);
  i2c_reg_write(SEN_I2C_ADDR,0x10,0x01);
  i2c_reg_write(SEN_I2C_ADDR,0x11,0x80); //0x01
  i2c_reg_write(SEN_I2C_ADDR,0x18,0xD5);
  i2c_reg_write(SEN_I2C_ADDR,0x19,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x1D,0xFF);
  i2c_reg_write(SEN_I2C_ADDR,0x1E,0xbF);
  i2c_reg_write(SEN_I2C_ADDR,0x1F,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x20,0xDC);
  i2c_reg_write(SEN_I2C_ADDR,0x21,0x05);
  i2c_reg_write(SEN_I2C_ADDR,0x22,0x55);
  i2c_reg_write(SEN_I2C_ADDR,0x23,0x03);
  i2c_reg_write(SEN_I2C_ADDR,0x24,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x25,0xD0);
  i2c_reg_write(SEN_I2C_ADDR,0x26,0x25);
  i2c_reg_write(SEN_I2C_ADDR,0x27,0xEa); // 0xe9  
  i2c_reg_write(SEN_I2C_ADDR,0x28,0x0c); // 0x0d
  i2c_reg_write(SEN_I2C_ADDR,0x29,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x2A,0xD4);
  i2c_reg_write(SEN_I2C_ADDR,0x2B,0x10);
  i2c_reg_write(SEN_I2C_ADDR,0x2C,0x00);
  i2c_reg_write(SEN_I2C_ADDR,0x2D,0x0A);
  i2c_reg_write(SEN_I2C_ADDR,0x2E,0xC2);
  i2c_reg_write(SEN_I2C_ADDR,0x2F,0x20);
  i2c_reg_write(SEN_I2C_ADDR,0x37,0x36);
  i2c_reg_write(SEN_I2C_ADDR,0x38,0x98);
  i2c_reg_write(SEN_I2C_ADDR,0x12,0x30);
	
  return 0; 
    
}

int isp_sen_config_AR0130 (void)
{
  int rdata;
/*  
  if (isp_sen_id_check() != 0)
  {
    return -1;
  }
*/  
  i2c_reg_read(0x21,0x301A);
  printf("\r\nrdata 0x%08x", rdata);
  
  //[720p30]
/*	
	i2c_reg_write(SEN_I2C_ADDR,0x301A, 0x0001); 	// RESET_REGISTER
	i2c_reg_write(SEN_I2C_ADDR,0x301A, 0x10D8); 	// RESET_REGISTER
	
	//delay_ms(200);	//DELAY= 200
	
	i2c_reg_write(SEN_I2C_ADDR,0x3088, 0x8000); 	// SEQ_CTRL_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0225); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x5050); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2D26); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0828); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0D17); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0926); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0028); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0526); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0xA728); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0725); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x8080); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2917); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0525); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0040); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2702); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1616); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2706); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1736); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x26A6); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1703); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x26A4); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x171F); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2805); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2620); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2804); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2520); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2027); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0017); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1E25); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0020); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2117); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1028); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x051B); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1703); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2706); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1703); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1741); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2660); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x17AE); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2500); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x9027); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0026); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1828); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x002E); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2A28); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x081E); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0831); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1440); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x4014); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2020); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1410); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1034); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1400); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1014); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0020); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1400); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x4013); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1802); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1470); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x7004); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1470); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x7003); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1470); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x7017); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2002); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1400); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2002); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1400); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x5004); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1400); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2004); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x1400); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x5022); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0314); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0020); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0314); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x0050); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2C2C); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x3086, 0x2C2C); 	// SEQ_DATA_PORT
	i2c_reg_write(SEN_I2C_ADDR,0x309E, 0x0000); 	// ERS_PROG_START_ADDR
	
	//delay_ms(200);	//DELAY= 200
	
	i2c_reg_write(SEN_I2C_ADDR,0x30E4, 0x6372); 	// ADC_BITS_6_7
	i2c_reg_write(SEN_I2C_ADDR,0x30E2, 0x7253); 	// ADC_BITS_4_5
	i2c_reg_write(SEN_I2C_ADDR,0x30E0, 0x5470); 	// ADC_BITS_2_3
	i2c_reg_write(SEN_I2C_ADDR,0x30E6, 0xC4CC); 	// ADC_CONFIG1
	i2c_reg_write(SEN_I2C_ADDR,0x30E8, 0x8050); 	// ADC_CONFIG2
	i2c_reg_write(SEN_I2C_ADDR,0x3082, 0x0029); 	// OPERATION_MODE_CTRL
	i2c_reg_write(SEN_I2C_ADDR,0x30B0, 0x1300); 	// DIGITAL_TEST
	i2c_reg_write(SEN_I2C_ADDR,0x30D4, 0xE007); 	// COLUMN_CORRECTION
	i2c_reg_write(SEN_I2C_ADDR,0x301A, 0x10DC); 	// RESET_REGISTER
	i2c_reg_write(SEN_I2C_ADDR,0x301A, 0x10D8); 	// RESET_REGISTER
	i2c_reg_write(SEN_I2C_ADDR,0x3044, 0x0400); 	// DARK_CONTROL
	i2c_reg_write(SEN_I2C_ADDR,0x3EDA, 0x0F03); 	// DAC_LD_14_15
	i2c_reg_write(SEN_I2C_ADDR,0x3ED8, 0x01EF); 	// DAC_LD_12_13
	i2c_reg_write(SEN_I2C_ADDR,0x3012, 0x02A0); 	// COARSE_INTEGRATION_TIME
	i2c_reg_write(SEN_I2C_ADDR,0x3032, 0x0000); 	// DIGITAL_BINNING
	i2c_reg_write(SEN_I2C_ADDR,0x3002, 0x003e); 	// Y_ADDR_START
	i2c_reg_write(SEN_I2C_ADDR,0x3004, 0x0004); 	// X_ADDR_START
	i2c_reg_write(SEN_I2C_ADDR,0x3006, 0x030d); 	// Y_ADDR_END
	i2c_reg_write(SEN_I2C_ADDR,0x3008, 0x0503); 	// X_ADDR_END
	i2c_reg_write(SEN_I2C_ADDR,0x300A, 0x02EE); 	// FRAME_LENGTH_LINES
	i2c_reg_write(SEN_I2C_ADDR,0x300C, 0x0CE4); 	// LINE_LENGTH_PCK
	i2c_reg_write(SEN_I2C_ADDR,0x301A, 0x10D8); 	// RESET_REGISTER
	i2c_reg_write(SEN_I2C_ADDR,0x31D0, 0x0001); 	// HDR_COMP

	//Load = PLL Enabled 27Mhz to 74.25Mhz
	i2c_reg_write(SEN_I2C_ADDR,0x302C, 0x0002); 	// VT_SYS_CLK_DIV
	i2c_reg_write(SEN_I2C_ADDR,0x302A, 0x0004); 	// VT_PIX_CLK_DIV
	i2c_reg_write(SEN_I2C_ADDR,0x302E, 0x0002); 	// PRE_PLL_CLK_DIV
	i2c_reg_write(SEN_I2C_ADDR,0x3030, 0x002C); 	// PLL_MULTIPLIER
	i2c_reg_write(SEN_I2C_ADDR,0x30B0, 0x0000); 	// DIGITAL_TEST	
	//delay_ms(100);	//DELAY= 100

	//LOAD= Disable Embedded Data and Stats
	i2c_reg_write(SEN_I2C_ADDR,0x3064, 0x1802); 	// SMIA_TEST, EMBEDDED_STATS_EN, 0x0000
	i2c_reg_write(SEN_I2C_ADDR,0x3064, 0x1802); 	// SMIA_TEST, EMBEDDED_DATA, 0x0000 
	i2c_reg_write(SEN_I2C_ADDR,0x30BA, 0x0008);       //20120502
    i2c_reg_write(SEN_I2C_ADDR,0x3EE4, 0xD308);     // Enable 1.25x analog gain 
	i2c_reg_write(SEN_I2C_ADDR,0x301A, 0x10DC); 	// RESET_REGISTER

	//delay_ms(200);	//DELAY= 200
  */
  return 0;
}

#ifdef PP1210K_SEN
// exposure setting for Pixel-Plus cmos
void isp_sen_exposure_set(isp_ae_ptr_t p_ae)
{
  unsigned int count;
  unsigned int etime_H8 = (p_ae->etimeUpd>>16) & 0xff;
  unsigned int etime_M8 = (p_ae->etimeUpd>>8) & 0xff;
  unsigned int etime_L8 = (p_ae->etimeUpd>>0) & 0xff;
  unsigned int again8 = p_ae->againUpd;
  unsigned int dgain8 = p_ae->dgainUpd;
  
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
  if (p_ae->etimeComp == 1)
  {
  	  i2c_reg_write(SEN_I2C_ADDR,0xc0,etime_H8);//WC004 # inttime_h  
     i2c_reg_write(SEN_I2C_ADDR,0xc1,etime_M8);//WC15F # inttime_m  
     i2c_reg_write(SEN_I2C_ADDR,0xc2,etime_L8);//WC200 # inttime_l  
  }
  if (p_ae->againComp == 1)
  {
  	 i2c_reg_write(SEN_I2C_ADDR,0xc3,again8);//WC300 # globalgain
  	 printf ("again8 = 0x%08x\n", again8);
  }
  if (p_ae->dgainComp == 1) 
  {
  	 i2c_reg_write(SEN_I2C_ADDR,0xc4,dgain8);//WC440 # digitalgain
  }
  printf("ae:etime_H8:0x%x etime_M8:0x%x etime_L8:0x%x again8:0x%x dgain8:0x%x\n",etime_H8,etime_M8,etime_L8,again8,dgain8);
  //for(count=0;count<10000;count++);
  
  return;
}

// exposure reading for Pixel-Plus cmos
int isp_sen_exposure_read(void)
{
  int etime, etime_H, etime_M, etime_L, again, dgain, yavg;
  
  i2c_reg_write(SEN_I2C_ADDR,0x03,0x01);//W0301
  etime_H = i2c_reg_read(SEN_I2C_ADDR,0xc0);
  etime_M = i2c_reg_read(SEN_I2C_ADDR,0xc1);
  etime_L = i2c_reg_read(SEN_I2C_ADDR,0xc2);
  etime = (etime_H<<16) | (etime_M<<8) | etime_L;
  again = i2c_reg_read(SEN_I2C_ADDR,0xc3);
  dgain = i2c_reg_read(SEN_I2C_ADDR,0xc4);
  /*
  printf ("\n");
  printf ("etime = 0x%08x\n", etime);
  printf ("again = 0x%08x\n", again);
  printf ("dgain = 0x%08x\n", dgain);*/
  
  return etime;
}
#endif
 
void isp_sen_init(isp_sen_ptr_t p_sen)
{

  p_sen->sensor_type = SENSOR_TYPE;
  //p_sen->image_size =  SENSOR_SIZE;
//  p_sen->bayer_mode = 2;
  

  if (p_sen->sensor_type == OV2643)
  {
      if (isp_sen_config_OV2643() == -1)
         printf( "\r\nOV2643 config: Failure ...");
      else
         printf( "\r\nOV2643 config: Successful ...");
  }
  else if (p_sen->sensor_type == OV9712) 
  {
      if (isp_sen_config_OV9712() == -1)
         printf( "\r\nOV9712 config: Failure ...");
      else
         printf( "\r\nOV9712 config: Successful ...");
  }
  else if (p_sen->sensor_type == JXH22)
  {
      if (isp_sen_config_JXH22() == -1)
         printf( "\r\nJXH22 config: Failure ...");
      else
         printf( "\r\nJXH22 config: Successful ...");
  }
  else if (p_sen->sensor_type == MT9P031)
  {
      if (isp_sen_config_MT9P031() == -1)
         printf( "\r\nMT9P031 config: Failure ...");
      else
         printf( "\r\nMT9P031 config: Successful ...");
  }
  else if (p_sen->sensor_type == AR0130)
  {
      isp_sen_config_AR0130();
      return;
  }
  else if (p_sen->sensor_type == PP1210K)
  {
  	  isp_sen_config_PP1210K();
  	  return;
  }
  else
  {
      printf( "\r\nSensor config: Failure ...\n");
  } 

  return;
}





