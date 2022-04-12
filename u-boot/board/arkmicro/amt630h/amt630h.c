#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define AMT630H_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"

#define rSYS_SOFT_RST		*((volatile unsigned int *)(0x6000005c))
#define rSYS_ANA_CFG		*((volatile unsigned int *)(0x60000080))


static void dwmci_select_pad(void)
{

}

static void dwmci_reset(void)
{

}

#define ARK_MMC_CLK     	24000000
int ark_dwmci_init(char *name,u32 regbase, int bus_width, int index)
{
    struct dwmci_host *host = NULL;
    host = malloc(sizeof(struct dwmci_host));
    if (!host) {
    	printf("dwmci_host malloc fail!\n");
        return 1;
    }
	memset(host, 0, sizeof(struct dwmci_host));

	dwmci_select_pad();
	dwmci_reset();

    host->name = name;
    host->ioaddr = (void *)regbase;
    host->buswidth = bus_width;
    host->dev_index = index;
    host->bus_hz = ARK_MMC_CLK;
	host->fifo_mode = 1;

    add_dwmci(host, host->bus_hz, 400000);

    return 0;
}

int board_mmc_init(bd_t *bis)
{
	ark_dwmci_init("ARK_MMC0", 0x70400000, 4, 0);

	return 0;
}

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)CONFIG_SYS_SDRAM_BASE,
				    CONFIG_SYS_SDRAM_SIZE);

	return 0;
}

int board_init(void)
{
    return 0;
}

int board_late_init(void)
{
	/* set usb0 host mode */
	rSYS_ANA_CFG &= ~(1 << 25);
	rSYS_ANA_CFG |= (1 << 24);
	udelay(100);

	/* soft reset phy */
	rSYS_SOFT_RST &= ~(1 << 30);
	udelay(100);
	rSYS_SOFT_RST |= (1 << 30);
	udelay(1000);

	/* soft reset controller */
	rSYS_SOFT_RST &= ~(1 << 3);
	udelay(100);
	rSYS_SOFT_RST |= (1 << 3);
	udelay(100);
	
#if 0
	char cmd[128];
	char *need_update;
	unsigned int loadaddr;

	run_command("sf probe", 0);

	gpio_direction_input(32);
	if (!gpio_get_value(32))
		env_set("need_update", "yes");

	need_update = env_get("need_update");
	if (!strcmp(need_update, "yes")) {
		sprintf(cmd, "fatload %s %s %s update-magic", env_get("update_dev"),
			env_get("dev_part"), env_get("loadaddr"));
		run_command(cmd, 0);
		loadaddr = env_get_hex("loadaddr", 0);
		if (loadaddr && memcmp((void*)loadaddr, ARKN141_UPDATE_MAGIC, 
				strlen(ARKN141_UPDATE_MAGIC))) {
			printf("Wrong update magic, do not update.\n");
			env_set("need_update", "no");
		} else {
			run_command("sf erase usrdata 0", 0);
			run_command("env default -f -a", 0);	
		}
	}
#endif
	return 0;
}
