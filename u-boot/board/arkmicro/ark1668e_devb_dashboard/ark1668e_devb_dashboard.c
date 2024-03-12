#include <common.h>
#include <dwmmc.h>
#include <malloc.h>
#include <debug_uart.h>
#include <asm-generic/gpio.h>
#include <asm/arch/ark-common.h>
#include <linux/usb/musb.h>
DECLARE_GLOBAL_DATA_PTR;
#define ARK1668_UPDATE_MAGIC	"ada7f0c6-7c86-11e9-8f9e-2a86e4085a59"
#define rSYS_BOOT_SAMPLE		*((volatile unsigned int *)(0xe4900000))
#define rSYS_SD_CLK_CFG			*((volatile unsigned int *)(0xe4900058))
#define rSYS_SD1_CLK_CFG		*((volatile unsigned int *)(0xe490005c))
#define rSYS_SOFT_RSTNA			*((volatile unsigned int *)(0xe4900074))
#define rSYS_SOFT_RSTNB			*((volatile unsigned int *)(0xe4900078))
#define rSYS_DDR_STATUS			*((volatile unsigned int *)(0xe4900180))
#define rSYS_DDR_IO_CFG			*((volatile unsigned int *)(0xe490019C))
#define rSYS_PAD_CTRL00			*((volatile unsigned int *)(0xe49001c0))
#define rSYS_PAD_CTRL01			*((volatile unsigned int *)(0xe49001c4))
#define rSYS_PAD_CTRL02			*((volatile unsigned int *)(0xe49001c8))
#define rSYS_PAD_CTRL04			*((volatile unsigned int *)(0xe49001d0))
#define rSYS_PAD_CTRL05			*((volatile unsigned int *)(0xe49001d4))
#define rSYS_PAD_CTRL06			*((volatile unsigned int *)(0xe49001d8))
#define rSYS_PAD_CTRL07			*((volatile unsigned int *)(0xe49001dc))
#define rSYS_PAD_CTRL08			*((volatile unsigned int *)(0xe49001e0))
#define rSYS_PAD_CTRL09			*((volatile unsigned int *)(0xe49001e4))
#define rSYS_PAD_CTRL0A			*((volatile unsigned int *)(0xe49001e8))
#define rSYS_PAD_CTRL0B			*((volatile unsigned int *)(0xe49001ec))
#define rSYS_PAD_CTRL0C			*((volatile unsigned int *)(0xe49001f0))
#define rSYS_PAD_CTRL0D			*((volatile unsigned int *)(0xe49001f4))
#define rSYS_PAD_CTRL0E			*((volatile unsigned int *)(0xe49001f8))
#define rSYS_PAD_CTRL38			*((volatile unsigned int *)(0xe49001fc))
#define rSYS_PAD_CTRL3E			*((volatile unsigned int *)(0xe4900200))
#define rSYS_PAD_CTRL0F			*((volatile unsigned int *)(0xe4900204))
#define rSYS_CPU_CTL            *((volatile unsigned int *)(0xe4900208))
#define rSYS_MFC_GMAC_CTL		*((volatile unsigned int *)(0xe490020c))
#define rSYS_DEVICE_CLK_CFG7	*((volatile unsigned int *)(0xe4900230))
#define rSYS_DEVICE_CLK_CFG8	*((volatile unsigned int *)(0xe4900234))
#define rSYS_USB_CFG			*((volatile unsigned int *)(0xe4900260))
#define rSYS_USB1_CFG			*((volatile unsigned int *)(0xe490026C))
#define rWDT_CR		*((volatile unsigned int *)(0xe4b00000))
#define MUSB_BASE				0xe0100000
extern struct SYS_INFO sys_info;
extern int ark_check_data_from_partition(char *part_name,unsigned int crc_data);
extern int ark_check_data_from_devide(char *file_name,unsigned int crc_data);
extern int get_crc_data_from_device(char *file_name);
extern int get_data_from_media(char *file_name);
extern const struct musb_platform_ops ark_musb_ops;
static struct musb_hdrc_config musb_config = {
	.multipoint = 1,
	.dyn_fifo = 0,
	.num_eps = 6,
	.ram_bits = 12
};
static struct musb_hdrc_platform_data musb_platform_data = {
	.mode = MUSB_HOST,
	.config = &musb_config,
	.power = 100,
	.platform_ops = &ark_musb_ops,
};
static void dwmci_select_pad(void)
{
	unsigned int val;
	/* use sd/mmc 0 */
	val = rSYS_PAD_CTRL00;
	val &= ~((0x7<<18)|(0x7<<15)|(0x7<<12)|(0x7<<9)|(0x7<<6)|(0x7<<3)|(0x7<<0));
	val |= ((0x1<<18)|(0x1<<15)|(0x1<<12)|(0x1<<9)|(0x1<<6)|(0x1<<3)|(0x1<<0));
	rSYS_PAD_CTRL00 = val;
	val = rSYS_PAD_CTRL0E;
	val &= ~((0x7<<27)|(0x7<<24));
	val |= ((0x1<<27)|(0x1<<24));
	rSYS_PAD_CTRL0E = val;
	val = rSYS_PAD_CTRL38;
	val &= ~((0x7<<9)|(0x7<<6)|(0x7<<3)|(0x7<<0));
	val |= ((0x1<<9)|(0x1<<6)|(0x1<<3)|(0x1<<0));
	rSYS_PAD_CTRL38 = val;
	/* use sd/mmc 1 */
    val = rSYS_PAD_CTRL00;
    val &= ~((0x7<<27)|(0x7<<24)|(0x7<<21));
    val |= ((0x1<<27)|(0x1<<24)|(0x1<<21));
    rSYS_PAD_CTRL00 = val;
    val = rSYS_PAD_CTRL01;
    val &= ~((0x7<<9)|(0x7<<6)|(0x7<<3)|(0x7<<0));
    val |= ((0x1<<9)|(0x1<<6)|(0x1<<3)|(0x1<<0));
    rSYS_PAD_CTRL01 = val;
	/* use sdio wifi/mmc 2*/
    val = rSYS_PAD_CTRL01;
    val &= ~((0x7<<27)|(0x7<<24)|(0x7<<21)|(0x7<<18)|(0x7<<15)|(0x7<<12));
    val |= ((0x1<<27)|(0x1<<24)|(0x1<<21)|(0x1<<18)|(0x1<<15)|(0x1<<12));
    rSYS_PAD_CTRL01 = val;
    val = rSYS_PAD_CTRL02;
    val &= ~(0x7<<0);
    val |= (0x1<<0);
    rSYS_PAD_CTRL02 = val;
}
/* static void dwmci_reset(void)
{
	rSYS_SOFT_RSTNA &= ~((1<<29)|(1<<16));
	rSYS_SOFT_RSTNB &= ~(1<<3);
	udelay(100);
	rSYS_SOFT_RSTNA |= ((1<<29)|(1<<16));
	rSYS_SOFT_RSTNB |= (1<<3);
} */
static void usb_controller_reset(void)
{
	rSYS_SOFT_RSTNA &= ~(3 << 5);
	udelay(100);
	rSYS_SOFT_RSTNA |= 3 << 5;
	udelay(10);
	/* enhance usb driving capability */
	rSYS_USB_CFG = 0x3c2e0020;
	udelay(10);
	rSYS_USB1_CFG = 0x3c2e0020;
}
#define ARK_MMC_CLK     	48000000
#define ARK_MMC_NUM			3
struct dwmci_host dwmcihost[ARK_MMC_NUM];
static int ark_dwmci_init(char *name,u32 regbase, int bus_width, int index)
{
    struct dwmci_host *host = NULL;
    host = &dwmcihost[index];
	memset(host, 0, sizeof(struct dwmci_host));
	dwmci_select_pad();
	//dwmci_reset();
	/* config clk in sample delay */
	//rSYS_SD_CLK_CFG &= ~(0x7f << 13);
	//rSYS_SD_CLK_CFG |= 52 << 13;
	//rSYS_SD1_CLK_CFG &= ~(0x7f << 13);
	//rSYS_SD1_CLK_CFG |= 52 << 13;
	/* mmc clk axipll(480M) / ((4 + 1) * 2) */
	rSYS_SD_CLK_CFG &= ~0xfff;
	rSYS_SD_CLK_CFG |= (1 << 8) | (1 << 7) | (1 << 5) | 4;
	rSYS_SD1_CLK_CFG &= ~0xfff;
	rSYS_SD1_CLK_CFG |= (1 << 8) | (1 << 7) | (1 << 5) | 4;
    host->name = name;
    host->ioaddr = (void *)regbase;
    host->buswidth = bus_width;
    host->dev_index = index;
    host->bus_hz = ARK_MMC_CLK;
    host->fifoth_val = 64;
	host->fifo_mode = 1;
    add_dwmci(host, host->bus_hz, 400000);
    return 0;
}
int board_mmc_init(bd_t *bis)
{
	ark_dwmci_init("ARK_MMC0", 0xec400000, 8, 0);
#ifndef CONFIG_SPL_BUILD
    ark_dwmci_init("ARK_MMC1", 0xec800000, 4, 1);
#endif
	ark_dwmci_init("ARK_MMC2", 0xecc00000, 4, 2);
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
	unsigned int val;
	/* watchdog disable */
	rWDT_CR = 0;
	/* cpu1 disable */
	rSYS_CPU_CTL &= ~(1 << 7);
	/* cpu cnt clk enable */
	rSYS_CPU_CTL |= 1 << 23;
	/* nand pad enable */
	val = rSYS_PAD_CTRL08;
	val &= ~((0x7<<27) | (0x7<<24) | (0x7<<21) | (0x7<<18) | (0x7<<15) | (0x7<<12) | (0x7<<9) | (0x7<<6));
	val |= (0x1<<27) | (0x1<<24) | (0x1<<21) | (0x1<<18) | (0x1<<15) | (0x1<<12) | (0x1<<9) | (0x1<<6);
	rSYS_PAD_CTRL08 = val;
	val = rSYS_PAD_CTRL09;
	val &= ~((0x7<<15) | (0x7<<12) | (0x7<<9) | (0x7<<6) | (0x7<<3) | (0x7<<0));
	val |= (1<<15) | (1<<12) | (1<<9) | (1<<6) | (1<<3) | (1<<0);//enable nand cle, ale,ren,wen
	rSYS_PAD_CTRL09 = val;
	/* spi pad enable */
	val = rSYS_PAD_CTRL09;
	val &= ~((0x7<<27) | (0x7<<24));
	val |= (0x2<<27) | (0x2<<24);
	rSYS_PAD_CTRL09 = val;
	val = rSYS_PAD_CTRL0A;
	val &= ~((0x7<<3) | (0x7<<0));
	val |= (0x2<<0);
	rSYS_PAD_CTRL0A = val;
	/* gmac pad enable */
	rSYS_PAD_CTRL0C = (1 << 27) | (1 << 24) | (1 << 21) | (1 << 18) | (1 << 15) | (1 << 12) | (1 << 9) |
						(1 << 6) | (1 << 3) | (1 << 0);
	//rSYS_PAD_CTRL0D = (1 << 24) | (1 << 9) | (1 << 6) | (1 << 3) | (1 << 0);
	rSYS_PAD_CTRL0D = (1 << 24) | (1 << 21) | (1 << 18) | (1 << 15) | (1 << 12) | (1 << 9) |
						(1 << 6) | (1 << 3) | (1 << 0);
	/* gmac tx clk out */
	rSYS_PAD_CTRL0F |= (1 << 31);
	/* select rgmii interface */
	rSYS_MFC_GMAC_CTL &= ~(7 << 1);
	rSYS_MFC_GMAC_CTL |= (1 << 1);
	/* mac rx clk inv */
	rSYS_DEVICE_CLK_CFG7 |= (1 << 1);
	/* mac tx clk inv */
	rSYS_DEVICE_CLK_CFG8 |= (1 << 7);
	/* i2s0 sadata in */
	rSYS_PAD_CTRL0F &= ~(1 << 28);
	/* i2s1 sadata out */
	rSYS_PAD_CTRL0F |= (1 << 29);
	/* select pwm0 pad */
	rSYS_PAD_CTRL05 &= ~0x7;
	rSYS_PAD_CTRL05 |= 1;
    return 0;
}
#ifdef CONFIG_BOARD_EARLY_INIT_F
int board_early_init_f(void)
{
#ifdef CONFIG_DEBUG_UART
	debug_uart_init();
#endif
	return 0;
}
#endif
int burn_data_to_partition(char *partition_name)
{
	char cmd[128] = { 0 };
	int ret = -1;
	int file_size = 0;
	unsigned int crc_des = 0;
	unsigned int *dstdata = (unsigned int *)(env_get_hex("cmploadaddr", 0));
	sprintf(cmd, "nand erase.part %s", partition_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		return ret;
	file_size = env_get_ulong("filesize", 16, 0x2000);
	sprintf(cmd, "nand write %s %s 0x%x", env_get("loadaddr"), partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if(ret)
	{
		printf("Nand write %s Error\n",partition_name);
		return 1;
	}
	mdelay(50);
	sprintf(cmd, "nand read %s %s 0x%x",env_get("cmploadaddr"),partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if(ret)
	{
		printf("Nand read %s Error\n",partition_name);
		return 1;
	}
	crc_des = crc32(0, (const unsigned char *)dstdata, file_size);
	ret  = ark_check_data_from_partition(partition_name,crc_des);
	if(ret)
		return 1;
	return 0;
}
static int ark_update_partition(char *partition_name, char *file_name)
{
	int ret;
	char *update_device = env_get("update_dev_type");
	printf("\r\n****** update %s from %s:%s .....\r\n",partition_name,update_device,file_name);
	ret = get_data_from_media(file_name);
	if (!ret) {
		 return burn_data_to_partition(partition_name);
	}
	return ret;
}
static int ark_burn_bootstrap(char *partition_name)
{
	char cmd[128] = { 0 };
	int ret = -1;
	int file_size = 0;
	unsigned int crc_des = 0;
	unsigned int *dstdata = (unsigned int *)(env_get_hex("cmploadaddr", 0));
	sprintf(cmd, "nand erase.part %s", partition_name);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if (ret)
		return ret;
	file_size = env_get_ulong("filesize", 16, 0x2000);
	run_command("switchecc 1", 0);
	mdelay(10);
	sprintf(cmd, "nand write %s %s 0x%x", env_get("loadaddr"), partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if(ret)
	{
		printf("Nand write %s Error\n",partition_name);
		return 1;
	}
	mdelay(50);
	sprintf(cmd, "nand read %s %s 0x%x",env_get("cmploadaddr"),partition_name, file_size);
	printf("cmd=%s\n", cmd);
	ret = run_command(cmd, 0);
	if(ret)
	{
		printf("Nand read %s Error\n",partition_name);
		return 1;
	}
	run_command("switchecc 0", 0);
	mdelay(10);
	crc_des = crc32(0, (const unsigned char *)dstdata, file_size);
	printf("crc_des 0x%x,sys_info.ubootspl_crc 0x%x\n",crc_des,sys_info.ubootspl_crc);
	ret  = ark_check_data_from_partition(partition_name,crc_des);
	if(ret)
		return 1;
	return 0;
}
static int ark_update_bootstrap(char *partition_name, char *file_name)
{
	int ret;
	char *update_device = env_get("update_dev_type");
	printf("\r\n****** update %s from %s:%s .....\n",partition_name,update_device,file_name);
	ret = get_data_from_media(file_name);
	if (!ret) {
		 return ark_burn_bootstrap(partition_name);
	}
	return ret;
}
static int ark_update_from_media(unsigned int updateDevice)
{
	char cmd[32];
	unsigned int ret = 0;
	char *update_fdt = NULL;
	unsigned int dis_pos_x;
	unsigned int dis_pos_y;
	unsigned int resetuboot = 0;
	resetuboot = env_get_hex("ubootreset", 0);
	printf("resetuboot = %d \n",resetuboot);
	run_command("env default -f -a", 0);
	mdelay(100);
	if (updateDevice) {
		printf("update form mmc...\n");
		env_set("update_dev_type", "mmc");
		mdelay(10);
		env_set("update_dev_part", env_get("sd_dev_part"));
	} else {
		printf("update form usb...\n");
		env_set("update_dev_type", "usb");
		mdelay(10);
		env_set("update_dev_part", "0");
		mdelay(10);
		env_set("updata_status", "usb");
	}
	mdelay(10);
	if(resetuboot)
	{
		sprintf((char *)cmd,"disconfig 0");
		run_command(cmd, 0);
	}
	update_fdt = env_get("boardfdt");
	printf("update_fdt:%s\n", update_fdt);
	get_crc_data_from_device("crcdata.bin");
	dis_pos_x = 6;
	dis_pos_y = 20;
	ret = ark_update_bootstrap("bootstrap", "ubootspl.bin");
	mdelay(30);
	if(!ret)
	{
		sprintf(cmd, "setenv bootstrapsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 5");
		run_command(cmd, 0);
		mdelay(30);
		if(resetuboot)
		{
			sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootstrap",0);
			printf("cmd=%s\n", cmd);
			run_command(cmd, 0);
		}
	}
	else
	{
		mdelay(30);
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootstrap",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("bootloader", "u-boot.img");
	if(!ret)
	{
		sprintf(cmd, "setenv bootloadersize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 10");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		if(resetuboot)
		{
			sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader",0);
			printf("cmd=%s\n", cmd);
			run_command(cmd, 0);
		}
		if(resetuboot == 0)
		{
			env_set("ubootreset", "1");
			run_command("saveenv", 0);
			mdelay(20);
			run_command("reset", 0);
			while(1);
		}
		env_set("ubootreset", "0");
		run_command("saveenv", 0);
		mdelay(20);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("fdt", update_fdt);
	if(!ret)
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf(cmd, "setenv fdtsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 15");
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"fdt",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"fdt",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("kernel", "zImage");
	if(!ret)
	{
		sprintf(cmd, "setenv kernelsize %s",env_get("filesize"));
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 25");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"kernel",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"kernel",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
#if 0
	ret = ark_update_partition("bootanimation", "bootanimation");
	if(!ret)
	{
		sprintf(cmd, "setenv bootanimationsize %s",env_get("filesize"));
		run_command(cmd, 0);
		printf("cmd=%s\n", cmd);
		mdelay(30);
		sprintf((char *)cmd,"disconfig 30");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootanimation",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootanimation",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
#endif
	ret = ark_update_partition("bootloader_bak", "u-boot.img");
	if(!ret)
	{
		sprintf((char *)cmd,"disconfig 40");
		run_command(cmd, 0);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader_bak",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"bootloader_bak",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	ret = ark_update_partition("rootfs","rootfs.ubi");
	if(!ret)
	{
		sprintf((char *)cmd,"disconfig 95");
		run_command(cmd, 0);
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"rootfs",0);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
	else
	{
		mdelay(30);
		dis_pos_y = dis_pos_y + 20;
		sprintf((char *)cmd,"disstring %d %d %s %d",dis_pos_x,dis_pos_y,"rootfs",1);
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
		goto bootoldsys;
	}
	env_set("need_update", "no");
	mdelay(5);
	env_set("updata_status", "none");
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(30);
	sprintf((char *)cmd,"disconfig 100");
	run_command(cmd, 0);
	printf("\r\n **** update the device over! .....\r\n");
	return 0;
bootoldsys:
	env_set("updata_status", "error");
	sprintf(cmd, "saveenv");
	printf("cmd=%s\n", cmd);
	run_command(cmd, 0);
	mdelay(20);
	sprintf((char *)cmd,"disconfig 100");
	run_command(cmd, 0);
	return 1;
}
int board_late_init(void)
{
	char cmd[128];
	char *need_update,*update_flash;
	unsigned int loadaddr;
	int do_update = 0, update_from_mmc = 1;
	int update_detect = 0;//0:no update 1:need_update env 2:usb update boot 3:sd update boot
	run_command("sf probe", 0);
#ifdef CONFIG_USB_MUSB_HOST
	usb_controller_reset();
	musb_register(&musb_platform_data, NULL, (void *)MUSB_BASE);
#endif
	update_flash = env_get("update_from_ota");
	need_update = env_get("need_update");
	if (!strcmp(need_update, "yes"))
		update_detect = 1;
	else if (((rSYS_BOOT_SAMPLE >> 2) & 3) == 1)
		update_detect = 2;
	else if (((rSYS_BOOT_SAMPLE >> 2) & 3) == 2)
		update_detect = 3;
//由于usb升级检测的时候，会需要一段时间所以这个地方需要时间长一些,设置3s
	ark_watchdog_start(3000);
	printf("update_detect=%d.\n", update_detect);
	if (update_detect) {
		loadaddr = env_get_hex("loadaddr", 0);
		if (loadaddr)
			memset((void*)loadaddr, 0, strlen(ARK1668_UPDATE_MAGIC));
		if (update_detect == 1 || update_detect == 3) {
			sprintf(cmd, "fatload %s %s %s update-magic", "mmc", env_get("sd_dev_part"), env_get("loadaddr"));
			run_command(cmd, 0);
			if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
				do_update = 1;
				goto update_done;
			} else {
				printf("Wrong update magic, do not update from mmc.\n");
			}
		}
#ifdef CONFIG_USB_MUSB_HOST
		if (update_detect == 1 || update_detect == 2) {
			run_command("usb start", 0);
			sprintf(cmd, "fatload %s %s %s update-magic", "usb", "0", env_get("loadaddr"));
			run_command(cmd, 0);
			if (loadaddr && !memcmp((void *)loadaddr, ARK1668_UPDATE_MAGIC, strlen(ARK1668_UPDATE_MAGIC))) {
				do_update = 1;
				update_from_mmc = 0;
				goto update_done;
			} else {
				printf("Wrong update magic, do not update from usb.\n");
			}
		}
#endif
	}
	if (!strcmp(update_flash, "yes")){
//升级的时候关闭看门够功能
		ark_watchdog_stop();
	    sprintf(cmd, "run updatefromflash");
		printf("cmd=%s\n", cmd);
		run_command(cmd, 0);
	}
update_done:
	if (do_update) {
		run_command("nand erase.part userdata", 0);
		env_set("do_update", "yes");
		mdelay(10);
//升级的时候关闭看门够功能
		ark_watchdog_stop();
		ark_update_from_media(update_from_mmc);
	} else {
		env_set("do_update", "no");
	}
	return 0;
}

