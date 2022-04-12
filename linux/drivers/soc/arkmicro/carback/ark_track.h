#ifndef __ARK_TRACK_H__
#define __ARK_TRACK_H__ 

#define TRACK_MAX    100
#define CAR_MAX      10
#define RADAR_MAX    200
#define SIGNAL_MAX   10
 
//#define ARK_TRACK_DBG

#ifdef ARK_TRACK_DBG
#define ARKTRACK_DBGPRTK(...) printk(KERN_ALERT __VA_ARGS__)
#else
#define ARKTRACK_DBGPRTK(...)
#endif

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))
#define ARK_IDENTITY  MKTAG('R', 'S', 'T', 'K')//0xa1a2a3a4

#define FL_00        0xaa000000
#define FL_01        0x01000000
#define FL_02        0x02000000
#define FL_03        0x03000000
#define FL_04        0x04000000
#define FL_05        0x05000000
#define FL_06        0x06000000
#define FL_10        0x10000000
#define FL_11        0x11000000
#define FL_12        0x12000000
#define FL_13        0x13000000
#define FL_14        0x14000000
#define FL_15        0x15000000
#define FL_16        0x16000000
#define FL_20        0x20000000
#define FL_21        0x21000000
#define FL_22        0x22000000
#define FL_23        0x23000000
#define FL_24        0x24000000
#define FL_25        0x25000000
#define FL_26        0x26000000
#define FL_30        0x30000000
#define FL_31        0x31000000
#define FL_32        0x32000000
#define FL_33        0x33000000
#define FL_34        0x34000000
#define FL_35        0x35000000
#define FL_36        0x36000000
#define FL_40        0x40000000
#define FL_41        0x41000000
#define FL_42        0x42000000
#define FL_43        0x43000000
#define FL_44        0x44000000
#define FL_45        0x45000000
#define FL_46        0x46000000
#define FL_50        0x50000000
#define FL_51        0x51000000
#define FL_52        0x52000000
#define FL_53        0x53000000
#define FL_54        0x54000000
#define FL_55        0x55000000
#define FL_56        0x56000000
#define FL_60        0x60000000
#define FL_61        0x61000000
#define FL_62        0x62000000
#define FL_63        0x63000000
#define FL_64        0x64000000
#define FL_65        0x65000000
#define FL_66        0x66000000

#define FR_00        0x00aa0000
#define FR_01        0x00010000
#define FR_02        0x00020000
#define FR_03        0x00030000
#define FR_04        0x00040000
#define FR_05        0x00050000
#define FR_06        0x00060000
#define FR_10        0x00100000
#define FR_11        0x00110000
#define FR_12        0x00120000
#define FR_13        0x00130000
#define FR_14        0x00140000
#define FR_15        0x00150000
#define FR_16        0x00160000
#define FR_20        0x00200000
#define FR_21        0x00210000
#define FR_22        0x00220000
#define FR_23        0x00230000
#define FR_24        0x00240000
#define FR_25        0x00250000
#define FR_26        0x00260000
#define FR_30        0x00300000
#define FR_31        0x00310000
#define FR_32        0x00320000
#define FR_33        0x00330000
#define FR_34        0x00340000
#define FR_35        0x00350000
#define FR_36        0x00360000
#define FR_40        0x00400000
#define FR_41        0x00410000
#define FR_42        0x00420000
#define FR_43        0x00430000
#define FR_44        0x00440000
#define FR_45        0x00450000
#define FR_46        0x00460000
#define FR_50        0x00500000
#define FR_51        0x00510000
#define FR_52        0x00520000
#define FR_53        0x00530000
#define FR_54        0x00540000
#define FR_55        0x00550000
#define FR_56        0x00560000
#define FR_60        0x00600000
#define FR_61        0x00610000
#define FR_62        0x00620000
#define FR_63        0x00630000
#define FR_64        0x00640000
#define FR_65        0x00650000
#define FR_66        0x00660000

#define RL_00        0x0000aa00
#define RL_01        0x00000100
#define RL_02        0x00000200
#define RL_03        0x00000300
#define RL_04        0x00000400
#define RL_05        0x00000500
#define RL_06        0x00000600
#define RL_10        0x00001000
#define RL_11        0x00001100
#define RL_12        0x00001200
#define RL_13        0x00001300
#define RL_14        0x00001400
#define RL_15        0x00001500
#define RL_16        0x00001600
#define RL_20        0x00002000
#define RL_21        0x00002100
#define RL_22        0x00002200
#define RL_23        0x00002300
#define RL_24        0x00002400
#define RL_25        0x00002500
#define RL_26        0x00002600
#define RL_30        0x00003000
#define RL_31        0x00003100
#define RL_32        0x00003200
#define RL_33        0x00003300
#define RL_34        0x00003400
#define RL_35        0x00003500
#define RL_36        0x00003600
#define RL_40        0x00004000
#define RL_41        0x00004100
#define RL_42        0x00004200
#define RL_43        0x00004300
#define RL_44        0x00004400
#define RL_45        0x00004500
#define RL_46        0x00004600
#define RL_50        0x00005000
#define RL_51        0x00005100
#define RL_52        0x00005200
#define RL_53        0x00005300
#define RL_54        0x00005400
#define RL_55        0x00005500
#define RL_56        0x00005600
#define RL_60        0x00006000
#define RL_61        0x00006100
#define RL_62        0x00006200
#define RL_63        0x00006300
#define RL_64        0x00006400
#define RL_65        0x00006500
#define RL_66        0x00006600

#define RR_00        0x000000aa
#define RR_01        0x00000001
#define RR_02        0x00000002
#define RR_03        0x00000003
#define RR_04        0x00000004
#define RR_05        0x00000005
#define RR_06        0x00000006
#define RR_10        0x00000010
#define RR_11        0x00000011
#define RR_12        0x00000012
#define RR_13        0x00000013
#define RR_14        0x00000014
#define RR_15        0x00000015
#define RR_16        0x00000016
#define RR_20        0x00000020
#define RR_21        0x00000021
#define RR_22        0x00000022
#define RR_23        0x00000023
#define RR_24        0x00000024
#define RR_25        0x00000025
#define RR_26        0x00000026
#define RR_30        0x00000030
#define RR_31        0x00000031
#define RR_32        0x00000032
#define RR_33        0x00000033
#define RR_34        0x00000034
#define RR_35        0x00000035
#define RR_36        0x00000036
#define RR_40        0x00000040
#define RR_41        0x00000041
#define RR_42        0x00000042
#define RR_43        0x00000043
#define RR_44        0x00000044
#define RR_45        0x00000045
#define RR_46        0x00000046
#define RR_50        0x00000050
#define RR_51        0x00000051
#define RR_52        0x00000052
#define RR_53        0x00000053
#define RR_54        0x00000054
#define RR_55        0x00000055
#define RR_56        0x00000056
#define RR_60        0x00000060
#define RR_61        0x00000061
#define RR_62        0x00000062
#define RR_63        0x00000063
#define RR_64        0x00000064
#define RR_65        0x00000065
#define RR_66        0x00000066

#define IMAGE_ID_NONE       0xFFFFFFFF
#define PIXEL_DATA_NONE		0x00FFFFFF
#define PIXEL_DATA_ZERO		0x0
#define PIXEL_ALPHA_ZERO	0x0
#define PIXEL_DATA_SIZE     4


#define RADAR_CHANNEL_FL  	0
#define RADAR_CHANNEL_FR  	1
#define RADAR_CHANNEL_RL  	2
#define RADAR_CHANNEL_RR  	3

#define TRACK_STRAIGHT_FORWARD_ID   40
#define CAR_NORMAL_STATUS_ID		0
#define SIGNAL_NORMAL_STATUS_ID		0
#define RADAR_NORMAL_STATUS_ID		0x23322332		
#define TRACK2_STRAIGHT_FORWARD_ID  35

enum image_type{
	IMAGE_TYPE_TRACK = 1,
	IMAGE_TYPE_CAR,
	IMAGE_TYPE_RADAR,
	IMAGE_TYPE_SIGNAL,
	IMAGE_TYPE_TRACK2,
	IMAGE_TYPE_MRADAR
};
enum cover_type{
	SRC_COVER_DST = 0,
	SRC_COVER_DST2,
	DST_COVER_SRC2,
};

typedef struct {
	unsigned int image_type;
	unsigned int image_store_id;
	unsigned int image_id;
	unsigned int image_offset;
	unsigned int image_size;
}item_info;


typedef struct {
	unsigned int pos_x;
	unsigned int pos_y;
	unsigned int width;
	unsigned int height;
}rect_info;

typedef struct {
	unsigned int identity;
	unsigned int file_size;
	unsigned int file_type;
	unsigned int track_total;
	unsigned int car_total;
	unsigned int radar_total;
	unsigned int signal_total;
	unsigned int track2_total;

	rect_info track_rect;
	rect_info car_rect;
	rect_info radar_rect[4];
	rect_info signal_rect;
	rect_info track2_rect;

	item_info track[TRACK_MAX];
	item_info car[CAR_MAX];
	item_info radar[RADAR_MAX];
	item_info signal[SIGNAL_MAX];
	item_info track2[TRACK_MAX];

}user_header;

#define MRADAR_MAX    16
#define MRADAR_ITEM_MAX    16
#define HEADER2_FILE_FLAG    (1 << 31)

typedef struct {
	unsigned int identity;
	unsigned int file_size;
	unsigned int file_type;//resolve
	
	unsigned int mradar_total[MRADAR_MAX];
	rect_info mradar_rect[MRADAR_MAX];
	item_info mradar[MRADAR_MAX][MRADAR_ITEM_MAX];

	unsigned int reserve[256];
	
}user2_header;


typedef struct {
	char image_name[10];
	unsigned int image_id;
	int dis_channel;
}idmap_info;

typedef struct {
	rect_info track_rect;
	rect_info car_rect;
	rect_info radar_rect[4];
	rect_info signal_rect;
	rect_info track2_rect;
	//rect_info mradar_rect[MRADAR_MAX];

}track_param_context;

typedef struct {
	rect_info mradar_rect[MRADAR_MAX];
}mradar_param_context;

typedef struct {
	user_header* pheader_buf;
	unsigned int disp_xpos;
	unsigned int disp_ypos;
	unsigned int disp_width;
	unsigned int disp_height;
	
	unsigned int disp_track_id;
	unsigned int disp_car_id;
	unsigned int disp_radar_id;
	unsigned int disp_signal_id;
	unsigned int disp_track2_id;

	unsigned char* disp_track_buf;
	unsigned char* disp_car_buf;
	unsigned char* disp_radar_buf[4];
	unsigned char* disp_signal_buf;
	unsigned char* disp_track2_buf;

	unsigned int disp_track_size;
	unsigned int disp_car_size;
	unsigned int disp_radar_size;
	unsigned int disp_signal_size;
	unsigned int disp_track2_size;

	track_param_context track_param;
	unsigned int file_type;;

}track_context;

typedef struct {
	user2_header* pheader2_buf;
	unsigned int file_type;;
	mradar_param_context mradar_param;
	
	unsigned char disp_mradar_id[MRADAR_MAX];
	unsigned int disp_mradar_id_change;

	unsigned char* disp_mradar_buf[MRADAR_MAX];//0-7: fontrear  8-15: center
	unsigned int disp_mradar_size[MRADAR_MAX];//0-7: fontrear  8-15: center
	

}track_context2;


extern int track_paint_init(void);
extern unsigned int track_paint_fill(void *dest, unsigned int width, unsigned int height);
extern void track_paint_deinit(void);
extern int set_disp_track_id(unsigned int image_id);
extern int set_disp_car_id(unsigned int image_id);
extern int set_disp_radar_id(unsigned int image_id);
extern int set_disp_signal_id(unsigned int image_id);
extern int set_disp_track2_id(unsigned int image_id);
extern int set_disp_mradar_id(unsigned char *pimage_id);

#endif
