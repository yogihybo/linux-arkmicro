#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/setup.h>
#include <linux/cdev.h>
#include "ark_mcu.h"
#include "ark_track.h"
#include "ark1668e_carback.h"
#include <linux/serial.h>

extern void track_serial_register_rev_handler(void (*handler)(unsigned char ch));
extern void track_serial_unregister_rev_handler(void);


extern struct carback_context* g_carback_context;

extern unsigned int track_pic_id;
extern unsigned int radar_pic_id;
extern unsigned int car_pic_id;
extern unsigned int track2_pic_id;//small track for vbox

extern int vbox_track_paint;

static u8 get_rx_crc(u8 *buf, u8 len)
{
	u8 i,crc;
	crc=0;
	for(i=1;i<len;i++){
		crc ^= buf[i];
	}
	return crc;
}

static void get_mcu_carback_data(unsigned char ch)
{
	if(!g_carback_context->carback_status || vbox_track_paint) return;

	//for uatr test
	set_disp_track_id(ch);
#if 0
	if(g_carback_context && g_carback_context->get_wheel_angle)
		g_carback_context->get_wheel_angle(ch);
	
	if(g_carback_context && g_carback_context->get_radar_info)
		g_carback_context->get_radar_info(ch);
#endif
	
}

void demo_parse_mcu_data(unsigned char *buf,unsigned char size)
{
	unsigned char wheel_angle, wheel_anglesign;
	unsigned int pic_id_fl, pic_id_fr, pic_id_rl, pic_id_rr;
	unsigned char cmd;
	unsigned char data[20];
	int i;
	
	memset(data,0,20);
	memcpy(data,buf,size);
	
	cmd = data[0];
	if(cmd == 0x01){
		//demo: parse to track id, and set track_pic_id
		wheel_anglesign = data[1];
		wheel_angle     = data[2];

		if(wheel_anglesign == 0)
			track_pic_id = TRACK_STRAIGHT_FORWARD_ID + wheel_angle;
		else
			track_pic_id = TRACK_STRAIGHT_FORWARD_ID - wheel_angle;
		
		set_disp_track_id(track_pic_id);
	}
	else if(cmd == 0x03){
		
		//demo: parse data to radar id,and set radar_pic_id
		pic_id_fl = (data[1]&0xf) << 4 | (data[2]&0xf);
		if(pic_id_fl == 0) pic_id_fl = 0xaa;
		pic_id_fl <<= 24;
		
		pic_id_fr = (data[3]&0xf) << 4 | (data[4]&0xf);
		if(pic_id_fr == 0) pic_id_fr = 0xaa;
		pic_id_fr <<= 16;
		
		pic_id_rl = (data[5]&0xf) << 4 | (data[6]&0xf);
		if(pic_id_rl == 0) pic_id_rl = 0xaa;
		pic_id_rl <<= 8;
		
		pic_id_rr = (data[7]&0xf) << 4 | (data[8]&0xf);
		if(pic_id_rr == 0) pic_id_rr = 0xaa;
		pic_id_rr <<= 0;
		
		radar_pic_id = pic_id_fl | pic_id_fr | pic_id_rl| pic_id_rr;
		set_disp_radar_id(radar_pic_id);
	}

#if 0//debug
	{
		static char count = 0;
		if(is_uartx_app_used() == true){
			if(count++ < 100) return;

		}

		if(is_uartx_app_used() == true) {
			ARKTRACK_DBGPRTK("app==>mcu data:track_pic_id=0x%0x, radar_pic_id=0x%0x\n",track_pic_id,radar_pic_id);
		}
		else {
			ARKTRACK_DBGPRTK("kernel==>mcu data:track_pic_id=0x%0x, radar_pic_id=0x%0x\n",track_pic_id,radar_pic_id);
		}
	
		for(i = 0;i < size; i++){
			//ARKTRACK_DBGPRTK("%0x ",data[i]);
		}
		//ARKTRACK_DBGPRTK("\n");

		count = 0;
	}		
#endif

}

void demo_get_wheel_angle(unsigned char ch)
{
        /*******************************************************************************
                * demo radar info format:
                * [0x02] [0x81] [cmd] [len]               ----->fixdata
                * [subcmd] [anglesign] [angle]          ----->vardata
                * [checksum]
              ********************************************************************************/
        static u8 mcu_data[20] = {0x02,0x81,0xee,0x03,    
                                  0x01,0x00,0x05,    
                                  0x68};
        
        static bool  app_check_first = false;
        static unsigned int i = 0;
        char vardata_len = 0x03;//mcu_data[3];
        char fixdata_len = 4;

        //ARKTRACK_DBGPRTK("0x%0x\n",ch);	
	if(app_check_first == false ){
		app_check_first = true;
		i = 0;
		return;
	}

	if(i < fixdata_len){
		if(mcu_data[i] == ch)i++;
		else i = 0;
	}
	else if(i >= fixdata_len && i < (fixdata_len + vardata_len)){
		mcu_data[i] = ch;
		i++;
	}
	else if(i == (fixdata_len + vardata_len) && get_rx_crc(mcu_data,i) == ch){
                
		if(g_carback_context && g_carback_context->parse_mcu_data)
			g_carback_context->parse_mcu_data(&mcu_data[fixdata_len],vardata_len);

		memset(&mcu_data[fixdata_len], 0, vardata_len);
		i = 0;
	}
	else{
		i = 0;
	}
	
}

void demo_get_radar_info(unsigned char ch)
{        
        /*******************************************************************************
                * demo radar info format:
                * [0x02] [0x81] [cmd] [len]                ----->fixdata                                
                * [subcmd] [radar_fl] [radar_cfl]  [radar_cfr] [radar_fr][radar_rl] [radar_crl][radar_crr]  [radar_rr]   ----->vardata
                * [checksum]
              ********************************************************************************/
        static u8 mcu_data[20] = {0x02,0x81,0xee,0x09,  
                                  0x03,0x02,0x03,0x03,0x02,0x02,0x03,0x03,0x02,  
                                  0x65};//demo
        static bool  app_check_first = false;
        static unsigned int i = 0;
        char vardata_len = 0x09;//mcu_data[3];
        char fixdata_len = 4;
        
	if(app_check_first == false){
		app_check_first = true;
		i = 0;
		return;
	}
        
	if(i < fixdata_len){
		if(mcu_data[i] == ch)i++;
		else i = 0;
	}
	else if(i >= fixdata_len && i < (fixdata_len + vardata_len)){
		mcu_data[i] = ch;
		i++;
	}
	else if(i == (fixdata_len + vardata_len) && get_rx_crc(mcu_data,i) == ch){
                
		if(g_carback_context && g_carback_context->parse_mcu_data)
			g_carback_context->parse_mcu_data(&mcu_data[fixdata_len], vardata_len);
                
		memset(&mcu_data[fixdata_len], 0, vardata_len);
		i = 0;
	}
	else{
		i = 0;
	}
	
}

void register_mcu_interface(void)
{
	if(g_carback_context == NULL){
		printk(KERN_ALERT "register mcu interface error.\n");
		return;
	}

	track_serial_register_rev_handler(get_mcu_carback_data);

#ifdef CONFIG_REVERSING_TRACK
#ifdef CONFIG_DISABLE_GET_MCU_DATA
	printk("disable get mcu data.\n");
#else
	g_carback_context->get_mcu_carback_data = get_mcu_carback_data;
	
	//user need change interface
	g_carback_context->parse_mcu_data       = demo_parse_mcu_data;
	g_carback_context->get_wheel_angle      = demo_get_wheel_angle;
	g_carback_context->get_radar_info       = demo_get_radar_info;
#endif
#endif
}

void unregister_mcu_interface(void)
{
	if(g_carback_context == NULL){
		printk(KERN_ALERT "unregister mcu interface error.\n");
		return;
	}

	track_serial_unregister_rev_handler();

#ifdef CONFIG_REVERSING_TRACK
	g_carback_context->get_mcu_carback_data = NULL;
	
	//user need change interface
	g_carback_context->parse_mcu_data       = NULL;
	g_carback_context->get_wheel_angle      = NULL;
	g_carback_context->get_radar_info       = NULL;
#endif
}


