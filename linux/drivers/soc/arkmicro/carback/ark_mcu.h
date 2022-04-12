#ifndef	ARK_MCU_H
#define	ARK_MCU_H


typedef struct
{
        unsigned int Baud;
        unsigned char Databit;
        unsigned char Parity;
        unsigned char Stopbit;
}UART_PARA;

enum uart_user{
        MCU,
        DASHBOARD,
        USER_END,
};


extern bool is_uartx_app_used(void);
extern void kernel_read_mcu_data(void);
extern void register_mcu_interface(void);
extern void unregister_mcu_interface(void);

#endif
