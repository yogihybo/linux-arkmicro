//*********************************************
//		iic 通讯测试库函数v22
//*********************************************
//用户需要将本程序编译为库函数调用，以便后期处理加密驱动。
#include<linux/fs.h>
 
#define XS508_DT1    1          //1ms
int     XS_flag=0;

//!!!用户需要完成以下4个函数，以便X508调用。
//XS508I2C地址为0x29,对应的写指令为0x52,读指令为0x53;通讯速度100K。
//请用户将buffer中25个数据，自XS508的0x07处连续写出，并检测每次XS508的有效应答。
extern int  HI2C_write25Byte( unsigned char *buffer);

//请用户自XS508的0x00处连续读入32个字节数据，存入buffer，
//期间在发写指令0x52、xs508内指针0x00和读指令0x53时，应检测XS508的3次有效应答。
extern int  HI2C_read32Byte(unsigned char *buffer);  

//ms延时偏差不得超过10%
extern void XS508_delay_ms(int D_TIME);                

//请用户结合主控生成随机数
extern int get_xs_srand(void);
*********************************************/
//extern int printf(char *frm,...);

/*********************************************
//   XS508 Communication Test Driver 
//  IN:  16BYTE KEY & 16BYTE ID
// OUT:  0->PASS 
//      -1->I2C WRITE OR READ ERROR
//      -2->NG
*********************************************/
int XS508_I2C_Handshake(unsigned char *XS508_16B_Ukey,unsigned char *XS508_16B_ID)
{
	unsigned char wr_data_25_byte[25];
	unsigned char rd_data_32_byte[32]={0};
	int i,check_cnt;
	volatile int xs_curr_time;
	int debugset = 0;//0xa8;
	int t1;
	
	if (XS_flag ==0) 
	{
		t1 = xs_curr_time & 0xFF;
		//xs_curr_time += get_xs_srand();
		
		//srand(xs_curr_time);
		while (t1--)
		{
			//rand();
			get_xs_srand();
		}
		XS_flag=1;
	}  
	/* 
	if (XS_flag ==0) 
	{
		SysTick->CTRL&=0xfffffffb;
		xs_curr_time+=SysTick->VAL;
		srand(xs_curr_time);
		XS_flag=1;
	}	
 
 	*/      
 	
	//--------- initilize ------------------
	if (debugset==0xa8)
		printk(KERN_ALERT "T_I2C write	BYTES =\r\n"); 

					 
	for(i=1;i<25;i++)
	{
		//wr_data_25_byte[i]=i;
		//wr_data_25_byte[i]+=(char)rand();
		wr_data_25_byte[i]+=(char)get_xs_srand();
	}
	
	for(i=0;i<16;i++)
	{
		wr_data_25_byte[i+4]+=XS508_16B_Ukey[i];
	}
	
	if ((wr_data_25_byte[23]==0x5a)|| (wr_data_25_byte[23]==0xa5) )
		wr_data_25_byte[23]=0x7a;
   
	wr_data_25_byte[0]=0xF2;
	if (debugset==0xa8)	
	{
		for(i=0;i<25;i++)
			printk(KERN_ALERT "0x%2x,",wr_data_25_byte[i]);
		printk(KERN_ALERT "\n");
	}
	//------ step1: prapare 25 bytes data wr_data_25_byte[] -----------------------

	check_cnt=0;
	while(HI2C_write25Byte(wr_data_25_byte))	//0: i2c ACK , others: NACK
	{
		XS508_delay_ms(XS508_DT1);
		check_cnt++;
		if (check_cnt>100)
		{
			if (debugset==0xa8)
				printk(KERN_ALERT "X508 I2C Write  error \n");
			return -1;  
		}
	}
	//------- step2: write 25 bytes to xs508 -----

	XS508_delay_ms(10);//delay 10ms
	//------- step3: wait xs508 dispose  -----			
				
	check_cnt=0;
	while(HI2C_read32Byte(rd_data_32_byte))	//0: i2c ACK , others: NACK
	{
		XS508_delay_ms(XS508_DT1);//
		check_cnt++;
		if (check_cnt > 100)
		{
			if (debugset==0xa8)
				printk(KERN_ALERT "XS508 I2C Read  error \n");
		 
			return -1; 
		}
	}
	//------- step4: read 32 bytes data from xs508 -----
	
	XS508_delay_ms(100);  //delay 100ms 
	
	if (debugset==0xa8)
	{
		printk(KERN_ALERT "XS508_I2C READ 32 BYTES =\r\n"); 
		for(i=0;i<32;i++)
			printk(KERN_ALERT "0x%2x,",rd_data_32_byte[i]);
		printk(KERN_ALERT "\n"); 
	}
	
	
	for(i=0;i<24;i++)
	{
		if (wr_data_25_byte[1+i]!=(rd_data_32_byte[8+i]^0xaa) )
		{
			if (debugset==0xa8)
				printk(KERN_ALERT "no(%d): 0x%2x _xor_0xaa != 0x%2x	",i,wr_data_25_byte[1+i],rd_data_32_byte[8+i]);
			if (debugset==0xa8)
				printk(KERN_ALERT "IIC_DATA_ERR\r\n"); 
			return -2; //verify error
		}
	}
			
	
	if (debugset==0xa8)
		printk(KERN_ALERT "---XS508E_I2C success!---\r\n");
	
	return 0;	
}


