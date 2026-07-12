#include <common.h>
#include "musb_core.h"
#include "ark_musb.h"
#include <asm/gpio.h>

/* MUSB platform configuration */
struct musb_config musb_cfg = {
#if 0
#if CONFIG_USB_DEV_PART == 0
	.regs       = (struct musb_regs *)MUSB_ARK_USB_BASE,
#elif CONFIG_USB_DEV_PART == 1
	.regs       = (struct musb_regs *)MUSB_ARK_USB1_BASE,
#endif
#endif
	.regs       = (struct musb_regs *)MUSB_ARK_USB_BASE,
	.timeout    = 400000,//0x3FFFFFF,
	.musb_speed = 0,
};

#if 0
static void set_usb_power(unsigned int Enable)
{
	unsigned int reg;

	//GPIO126
	reg = (*(unsigned int *)(MUSB_ARK_SYS_BASE+0x1f0));
	reg &= ~(0x1<<(126-117));
	(*(unsigned int *)(MUSB_ARK_SYS_BASE+0x1f0)) = reg;

	//Set GPIO OutPut
	(*(unsigned int *)(MUSB_ARK_GPIO_BASE+0x60)) &= ~(1<<(126-96));

	//Set GPIO Data
	if(Enable)
	{
		(*(unsigned int *)(MUSB_ARK_GPIO_BASE+0x64)) |= (1<<(126-96));
	}
	else
	{
		(*(unsigned int *)(MUSB_ARK_GPIO_BASE+0x64)) &= ~(1<<(126-96));
	}	
}
#endif

static void reset_usb_phy(int index)
{
	unsigned int reg;

	//set_usb_power(0);
	//mdelay(20);
	//set_usb_power(1);
	
	if(index == 0) {
		reg = (*(unsigned int *)(MUSB_ARK_SYS_BASE+0x74));
		reg &= ~(0x3<<5);
		(*(unsigned int *)(MUSB_ARK_SYS_BASE+0x74)) = reg;
		
		udelay(20);
		
		reg |= (0x3<<5);
		(*(unsigned int *)(MUSB_ARK_SYS_BASE+0x74)) = reg;
	}else if(index == 1) {
		reg = (*(unsigned int *)(MUSB_ARK_SYS_BASE+0x78));
		reg &= ~(0x3<<6);
		(*(unsigned int *)(MUSB_ARK_SYS_BASE+0x78)) = reg;
		
		udelay(20);
		
		reg |= (0x3<<6);
		(*(unsigned int *)(MUSB_ARK_SYS_BASE+0x78)) = reg;	
	}
}

/*
 * Port-specific GPIO numbers. usb0 (index 0) uses the Kconfig-configured
 * values (CONFIG_USB_GPIO_*, currently PWR=126/ID=76/SW=-1) since that's
 * the port originally wired up here. usb1 (index 1) uses PWR=117/ID=1,
 * confirmed against the Linux kernel device tree (ark1668.dtsi: usb1's
 * gpio-pwr/gpio-id) — this is the port the WiFi module is actually on,
 * per the kernel boot log ("usb 2-1: ..." on musb-hdrc.1 == index 1).
 * No corresponding "SW" GPIO documented for usb1 in the DTS, so it's
 * left unconfigured (matching how usb0's SW is disabled by default too).
 */
#define USB1_GPIO_PWR	117
#define USB1_GPIO_ID	1

/*
 * Configure and reset one MUSB port (0 or 1): selects its register base
 * in musb_cfg, drives its power/id GPIOs, and resets its PHY. Callable
 * for either port — used both for the normal (index 0) bring-up and for
 * the index-1 retry in usb_lowlevel_init() (musb_hcd.c) when index 0
 * finds nothing attached.
 */
int musb_ark_configure_port(int index)
{
	if (index == 1) {
		musb_cfg.regs = (struct musb_regs *)MUSB_ARK_USB1_BASE;
		gpio_direction_output(USB1_GPIO_PWR, 1);
		gpio_direction_output(USB1_GPIO_ID, 0);
	} else if (index == 0) {
		musb_cfg.regs = (struct musb_regs *)MUSB_ARK_USB_BASE;
		if (CONFIG_USB_GPIO_SW >= 0)
			gpio_direction_output(CONFIG_USB_GPIO_SW, 0);
		if (CONFIG_USB_GPIO_PWR >= 0)
			gpio_direction_output(CONFIG_USB_GPIO_PWR, 1);
		if (CONFIG_USB_GPIO_ID >= 0)
			gpio_direction_output(CONFIG_USB_GPIO_ID, 0);
	} else {
		return -1;
	}
	udelay(20000);

	reset_usb_phy(index);
	return 0;
}

/*
 * CPU and board-specific MUSB initializations.  Aliased function
 * signals caller to move on.
 */
static void musb_ark_init(void)
{
	char *ch = CONFIG_USB_DEV_PART;
	int index = ch ? (*ch - '0') : 0;

	musb_ark_configure_port(index);
}

void board_musb_init(void) __attribute__((weak, alias("musb_ark_init")));

int musb_platform_init(void)
{
	/* board specific initialization */
	board_musb_init();

	return 0;
}

/*
 * This function performs platform specific deinitialization for usb.
*/
void musb_platform_deinit(void)
{
}
