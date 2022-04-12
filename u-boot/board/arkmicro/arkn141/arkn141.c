#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

#define rSYS_AHB_CLK_EN			*((volatile unsigned int *)(0x40408044))
#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0x40408058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0x4040805c))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0x40408074))
#define rSYS_PAD_CTRL03			*((volatile unsigned int *)(0x404081cc))
#define rSYS_PAD_CTRL08			*((volatile unsigned int *)(0x404081e0))
#define rSYS_PAD_CTRL09			*((volatile unsigned int *)(0x404081e4))

static void dwmci_select_pad(void)
{
	/* use sd/mmc 0 */
	rSYS_PAD_CTRL09 |= 0x7F;

	rSYS_SD_CLK_CFG = 0x00000420;

	/* use sd/mmc 1 pad sd1_0 */
	rSYS_PAD_CTRL09 |= (0x7F << 7);

	rSYS_SD1_CLK_CFG = 0x00000420;
}

static void dwmci_reset(void)
{
	rSYS_AHB_CLK_EN &= ~((1 << 4) | (1 << 3));
	rSYS_SOFT_RSTNA &= ~((1 << 31) | (1 << 12));
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1 << 31) | (1 << 12));
	rSYS_AHB_CLK_EN |= ((1 << 4) | (1 << 3));
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
	ark_dwmci_init("ARK_MMC0", 0x60000000, 4, 0);
    ark_dwmci_init("ARK_MMC1", 0x68000000, 4, 0);

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
	unsigned int tmp;

	/* SPI pad enable */
	tmp = rSYS_PAD_CTRL08;
	tmp &= ~(0xFF << 16);
	tmp |= (0xA8 << 16);
	rSYS_PAD_CTRL08 = tmp;

	/* power GPIO31 high for sdio wifi module in shangqi carcorder project */
	rSYS_PAD_CTRL03 &= ~(0x7 << 3);
	gpio_direction_output(31, 1);

    return 0;
}

int board_late_init(void)
{
	return 0;
}
