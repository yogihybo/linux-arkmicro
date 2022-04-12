#ifndef __ARK_CARBACK_H__
#define __ARK_CARBACK_H__

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))  
  
//#define ARK_CARBACK_DBG
 
#ifdef ARK_CARBACK_DBG 
#define CARBACK_DBGPRTK(...) printk(KERN_ALERT __VA_ARGS__)
#else
#define CARBACK_DBGPRTK(...)
#endif

#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	480

struct carback_context {
		int gpio_id;
        struct device *dev;
		struct gpio_desc	*detect;
		void __iomem *sys_base;
		void __iomem *uart_base;
        unsigned char carback_status;
        int carback_changed;
        int app_ready;
        int app_enter_done;
        int app_exit_done;
		int screen_width;
		int screen_height;
		int carback_signal;
		int carback_count;
		int track_setting;
		int track_data_status;
		int track_frame_delay;
        wait_queue_head_t carback_waiq;
        wait_queue_head_t app_enter_waiq;
        wait_queue_head_t app_exit_waiq;
        struct work_struct track_work;
        struct workqueue_struct *track_queue;
		struct timer_list track_timer;
		struct timer_list uartx_timer;
		struct timer_list carback_filter_timer;	
		struct work_struct carback_work;
		struct workqueue_struct *carback_queue;
        struct fasync_struct *async_queue_cb;
		unsigned int track_data_phyaddr;
		unsigned int track_data_virtaddr;
		unsigned int track_display_phyaddr;
		unsigned int track_display_virtaddr;
		unsigned int track_display_size;
		void (*parse_mcu_data)(unsigned char *buf,unsigned char size);
		void (*get_wheel_angle)(unsigned char ch);
		void (*get_radar_info)(unsigned char ch);
		void (*send_mcu_carback)(bool en);
		void (*get_mcu_carback_data)(unsigned char ch);
		int track_disp_width;
		int track_disp_height;
		int track_disp_xpos;
		int track_disp_ypos;
		void *ptrack_param;
	    unsigned int file_type;
		void *pmradar_param;
        spinlock_t spin_lock;
        int itu656_init;
};

struct ark_carback {
        int irq;
        int debounce_detect;
        struct platform_device *pdev;
        const char *driver_name;
        const char *name;
        int major;
        int minor_start;
        int minor_num;
        int num;
        struct cdev cdev;
        struct class *carback_class;
        struct device *carback_device;
        struct carback_context context;
};

enum ui_scaler_type_id {
	UI_SCALER_NONE,//normal mode: no caler ,cannt set posx posy
	UI_POSITION_CVBS,//no scaler, can set posx posy throught arkdata.ini	
	UI_SCALER_VGA, //scaler mode, can set posx posy throught arkdata.ini
	UI_SCALER_CVBS, //scaler mode, can set posx posy throught arkdata.ini
	UI_SCALER_END,
};

/*************************************************************************
 * Ioctl command definition
 *************************************************************************/
#define CARBACK_IOCTL_BASE							0x9A
#define CARBACK_IOCTL_SET_APP_READY					_IO(CARBACK_IOCTL_BASE, 0)
#define CARBACK_IOCTL_APP_ENTER_DONE				_IO(CARBACK_IOCTL_BASE, 1)
#define CARBACK_IOCTL_APP_EXIT_DONE					_IO(CARBACK_IOCTL_BASE, 2)	
#define CARBACK_IOCTL_GET_STATUS					_IOR(CARBACK_IOCTL_BASE, 3, int)	
#define CARBACK_IOCTL_DETECT_SIGNAL					_IOR(CARBACK_IOCTL_BASE, 4, int)
#define CARBACK_IOCTL_GET_HASTRACK					_IOR(CARBACK_IOCTL_BASE, 5, int)
#define CARBACK_IOCTL_STRACK_INIT					_IO(CARBACK_IOCTL_BASE, 6)
#define CARBACK_IOCTL_STRACK_START					_IO(CARBACK_IOCTL_BASE, 7)
#define CARBACK_IOCTL_STRACK_STOP					_IO(CARBACK_IOCTL_BASE, 8)	
#define CARBACK_IOCTL_SET_STRACKID					_IOW(CARBACK_IOCTL_BASE, 9, int)	
#define CARBACK_IOCTL_STRACK_SHOW					_IO(CARBACK_IOCTL_BASE, 10)
#define CARBACK_IOCTL_STRACK_CLOSE					_IO(CARBACK_IOCTL_BASE, 11)
#define CARBACK_IOCTL_STRACK_SETTING				_IOW(CARBACK_IOCTL_BASE, 12, int)
#define CARBACK_IOCTL_STRACK_FRAME_RATE				_IOW(CARBACK_IOCTL_BASE, 13, int)
#define CARBACK_IOCTL_STRACK_SET_PARAM				_IOW(CARBACK_IOCTL_BASE, 14, int)
#define CARBACK_IOCTL_STRACK_GET_PARAM				_IOW(CARBACK_IOCTL_BASE, 15, int)
#define CARBACK_IOCTL_STRACK_GET_FILETYPE			_IOW(CARBACK_IOCTL_BASE, 16, int)
#define CARBACK_IOCTL_STRACK_GET_IDENTITY			_IOW(CARBACK_IOCTL_BASE, 17, int)
#define CARBACK_IOCTL_MRADAR_SET_PARAM				_IOW(CARBACK_IOCTL_BASE, 18, int)
#define CARBACK_IOCTL_MRADAR_GET_PARAM				_IOW(CARBACK_IOCTL_BASE, 19, int)
#define CARBACK_IOCTL_MRADAR_SET_ID				    _IOW(CARBACK_IOCTL_BASE, 20, int)
#define CARBACK_IOCTL_GET_DATA_STATUS				 _IOW(CARBACK_IOCTL_BASE, 21, int)

extern int ark_track_display_init(int width,int height);
extern int ark_track_set_display_addr(unsigned int addr);
extern int ark_track_alpha_blend(void);
extern int ark_track_get_screen_info(int* width,int* height);

#endif
