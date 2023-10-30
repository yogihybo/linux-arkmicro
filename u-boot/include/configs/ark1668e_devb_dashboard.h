/*
 * (C) Copyright 2012 Stephen Warren
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#define CONFIG_SYS_NONCACHED_MEMORY		SZ_1M

/* Memory layout */
#define CONFIG_NR_DRAM_BANKS			1
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_SDRAM_SIZE		SZ_64M
#define CONFIG_SYS_MALLOC_LEN		0x400000

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SYS_INIT_SP_ADDR		0x308000
#else
#define CONFIG_SYS_INIT_SP_ADDR \
	(CONFIG_SYS_SDRAM_BASE + 16 * 1024 - GENERATED_GBL_DATA_SIZE)
#endif

/* phy auto negotiation timeout */
#define PHY_ANEG_TIMEOUT    20000

/* SPL */
#define CONFIG_SPL_TEXT_BASE		0x300000
#define CONFIG_SPL_MAX_SIZE			0x8000

/* timer base address */
#define CONFIG_SYS_TIMERBASE		0xe8600000

/* watchdog base address */
#define CONFIG_WATCHDOG_BASEADDR	0xe4b00000


/*
 * NAND Flash controller Configuration
 */
#define CONFIG_SYS_USE_NANDFLASH
#define CONFIG_SYS_NAND_BASE		0xec000000
#define CONFIG_SYS_MAX_NAND_DEVICE      1
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SYS_NAND_USE_FLASH_BBT
#define CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_DEVICE
#define NANDARGS \
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0" \
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0" \
	"bootstrapupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} ubootspl.bin; " \
		"then nand erase.part bootstrap; " \
		"switchecc 1; " \
		"nand write ${loadaddr} bootstrap ${filesize}; " \
		"switchecc 0; fi\0" \
	"fdtupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"then setenv fdtsize ${filesize}; " \
		"nand erase.part fdt; " \
		"nand write ${loadaddr} fdt ${filesize}; fi\0" \
	"kernelupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} zImage; " \
		"then setenv kernelsize ${filesize}; " \
		"nand erase.part kernel; " \
		"nand write ${loadaddr} kernel ${filesize}; fi\0" \
	"bootloaderupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} u-boot.img; " \
		"then setenv bootloadersize ${filesize}; " \
		"nand erase.part bootloader; " \
		"nand write ${loadaddr} bootloader ${filesize}; fi\0" \
/*	"bootloaderupdate=Updatebootloader ${update_dev_type} ${update_dev_part}; \0" \ 
*/	"updatefromflash=UpdateFlash 0 0; \0" \
	"rootfsupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} rootfs.ubi; " \
		"then setenv rootfssize ${filesize}; " \
		"nand erase.part rootfs; " \
		"nand write ${loadaddr} rootfs ${filesize}; fi\0" \
	"bootanimationupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} bootanimation; " \
		"then setenv bootanimationsize ${filesize}; " \
		"nand erase.part bootanimation; " \
		"nand write ${loadaddr} bootanimation ${filesize}; " \
		"else setenv bootanimationsize 0; fi\0" \
	"reversingtrackupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} reversingtrack; " \
		"then setenv reversingtracksize ${filesize}; " \
		"nand erase.part reversingtrack; " \
		"nand write ${loadaddr} reversingtrack ${filesize}; " \
		"else setenv reversingtracksize 0; fi\0" \
	"bootloaderupdate_back=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} u-boot.img; " \
		"then setenv bootloadersize ${filesize}; " \
		"nand erase.part bootloader_bak; " \
		"nand write ${loadaddr} bootloader_bak ${filesize}; fi\0" \
	"nandargs=setenv bootargs console=ttyS0,115200 " \
		"earlyprintk  loglevel=3 clk_ignore_unused lpj=2285568 enable_console " \
		"${mtdparts} " \
		"root=${nandroot} " \
		"rootfstype=${nandrootfstype}\0" \
		"nandroot=ubi0:rootfs ro ubi.mtd=rootfs ubi.fm_autoconvert=1\0" \
		"nandrootfstype=ubifs rootwait\0" \
		"nandboot=echo Booting from nand ...; " \
		"run nandargs; " \
		"nand read ${fdtaddr} fdt ${fdtsize}; " \
		"nand read ${kerneladdr} kernel ${kernelsize}; " \
		"nand read ${bootanimationaddr} bootanimation ${bootanimationsize}; " \
		"nand read ${reversingtrackaddr} reversingtrack ${reversingtracksize}; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0" \
	"sdboot=echo Booting from sd...; " \
		"run nandargs; " \
		"fatload mmc ${sd_dev_part} ${fdtaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"fatload mmc ${sd_dev_part} ${kerneladdr} zImage; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0"

/*
 * SPI Flash controller Configuration
 */
#define CONFIG_SYS_SPI_BASE			0xe4f00000
#define CONFIG_ENV_SECT_SIZE 		65536
#define CONFIG_SF_DEFAULT_MODE		(SPI_MODE_0)
#define CONFIG_ENV_SPI_MODE 		CONFIG_SF_DEFAULT_MODE
#define CONFIG_SF_DEFAULT_SPEED		2000000
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED

#define SFARGS \
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0" \
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0" \
	"bootstrapupdate=fatload ${update_dev_type} ${update_dev_part} ${loadaddr} LOADSYS.BIN; " \
		"sf erase bootstrap 0; " \
		"sf write ${loadaddr} bootstrap ${filesize}\0" \
	"fdtupdate=fatload ${update_dev_type} ${update_dev_part} ${loadaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"setenv fdtsize ${filesize}; " \
		"sf erase fdt 0; " \
		"sf write ${loadaddr} fdt ${filesize}\0" \
	"kernelupdate=fatload ${update_dev_type} ${update_dev_part} ${loadaddr} zImage; " \
		"setenv kernelsize ${filesize}; " \
		"sf erase kernel 0; " \
		"sf write ${loadaddr} kernel ${filesize}\0" \
	"bootloaderupdate=fatload ${update_dev_type} ${update_dev_part} ${loadaddr} u-boot.bin; " \
		"setenv bootloadersize ${filesize}; " \
		"sf erase bootloader 0; " \
		"sf write ${loadaddr} bootloader ${filesize}\0" \
	"rootfsupdate=fatload ${update_dev_type} ${update_dev_part} ${loadaddr} rootfs.ubi; " \
		"setenv rootfssize ${filesize}; " \
		"sf erase rootfs 0; " \
		"sf write ${loadaddr} rootfs ${filesize}\0" \
	"sfargs=setenv bootargs console=ttyS0,115200 " \
		"earlyprintk  loglevel=3 clk_ignore_unused lpj=2285568 " \
		"${mtdparts} " \
		"root=${sfroot} " \
		"rootfstype=${sfrootfstype}\0" \
	"sfroot=ubi0:rootfs ro ubi.mtd=rootfs\0" \
	"sfrootfstype=ubifs rootwait\0" \
	"sfboot=echo Booting from spi flash ...; " \
		"run sfargs; " \
		"sf read ${fdtaddr} fdt ${fdtsize}; " \
		"sf read ${kerneladdr} kernel ${kernelsize}; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0" \
	"sdboot=echo Booting from sd...; " \
		"run sfargs; " \
		"fatload mmc ${sd_dev_part} ${fdtaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"fatload mmc ${sd_dev_part} ${kerneladdr} zImage; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0"

/*
 * EMMC controller Configuration
 */
#define CONFIG_SUPPORT_EMMC_BOOT
#define CONFIG_SYS_MMC_ENV_DEV	0
#define EMMCARGS \
	"emmcparts=blkdevparts=" CONFIG_EMMCPARTS_DEFAULT "\0" \
	"bootstrapupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} ubootspl.bin; " \
		"then emmc erase.part bootstrap; " \
		"emmc write ${loadaddr} bootstrap ${filesize}; fi\0" \
	"fdtupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"then setenv fdtsize ${filesize}; " \
		"emmc erase.part fdt; " \
		"emmc write ${loadaddr} fdt ${filesize}; fi\0" \
	"kernelupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} zImage; " \
		"then setenv kernelsize ${filesize}; " \
		"emmc erase.part kernel; " \
		"emmc write ${loadaddr} kernel ${filesize}; fi\0" \
	"bootloaderupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} u-boot.img; " \
		"then setenv bootloadersize ${filesize}; " \
		"emmc erase.part bootloader; " \
		"emmc write ${loadaddr} bootloader ${filesize}; fi\0" \
/*	"bootloaderupdate=Updatebootloader ${update_dev_type} ${update_dev_part}; \0" \
*/	"updatefromflash=UpdateFlash 0 0; \0" \
	"rootfsupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} rootfs.ubi; " \
		"then setenv rootfssize ${filesize}; " \
		"emmc erase.part rootfs; " \
		"emmc write ${loadaddr} rootfs ${filesize}; fi\0" \
	"bootanimationupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} bootanimation; " \
		"then setenv bootanimationsize ${filesize}; " \
		"emmc erase.part bootanimation; " \
		"emmc write ${loadaddr} bootanimation ${filesize}; " \
		"else setenv bootanimationsize 0; fi\0" \
	"reversingtrackupdate=if fatload ${update_dev_type} ${update_dev_part} ${loadaddr} reversingtrack; " \
		"then setenv reversingtracksize ${filesize}; " \
		"emmc erase.part reversingtrack; " \
		"emmc write ${loadaddr} reversingtrack ${filesize}; " \
		"else setenv reversingtracksize 0; fi\0" \
	"emmcargs=setenv bootargs console=ttyS0,115200 " \
		"earlyprintk  loglevel=3 clk_ignore_unused lpj=2285568 " \
		"${emmcparts} " \
		"root=${emmcroot} " \
		"rootfstype=${emmcrootfstype}\0" \
	"emmcroot=/dev/mmcblk0p8 rw\0" \
	"emmcrootfstype=ext2 rootwait\0" \
	"emmcboot=echo Booting from emmc ...; " \
		"run emmcargs; " \
		"mmc dev ${emmc_dev_part}; " \
		"emmc read ${fdtaddr} fdt ${fdtsize}; " \
		"emmc read ${kerneladdr} kernel ${kernelsize}; " \
		"emmc read ${bootanimationaddr} bootanimation ${bootanimationsize}; " \
		"emmc read ${reversingtrackaddr} reversingtrack ${reversingtracksize}; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0"

/* Environment */
#define CONFIG_ENV_SIZE			8192	//CONFIG_PARTITION_UBOOT_ENV_SIZE
#define CONFIG_ENV_OFFSET		0x240000
#define CONFIG_ENV_OFFSET_REDUND	0x2C0000

#define CONFIG_SYS_LOAD_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x1000000)

/* Console UART */
#define CONFIG_PL011_CLOCK		24000000
#define CONFIG_PL01x_PORTS      {(void *)CONFIG_SYS_SERIAL0, (void *)CONFIG_SYS_SERIAL1, (void *)CONFIG_SYS_SERIAL2, (void *)CONFIG_SYS_SERIAL3}
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_SERIAL0      0xe8200000
#define CONFIG_SYS_SERIAL1      0xe8300000
#define CONFIG_SYS_SERIAL2      0xe8400000
#define CONFIG_SYS_SERIAL3      0xe8500000

/* Console configuration */
#define CONFIG_SYS_CBSIZE		512
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_BOUNCE_BUFFER


#if defined(CONFIG_SD_BOOT)

#define CONFIG_EXTRA_ENV_SETTINGS \
	"need_update=yes\0" \
	"update_dev_type=mmc\0" \
	"update_dev_part=1\0" \
	"sd_dev_part="CONFIG_SD_DEV_PART"\0" \
	"emmc_dev_part="CONFIG_EMMC_DEV_PART"\0" \
	"loadaddr=0x44000000\0" \
	"fdtaddr=0x42000000\0" \
	"kerneladdr=0x42100000\0" \
	"bootanimationaddr=0x5e000000\0" \
	"bootanimationsize=0\0" \
	"reversingtrackaddr=0x5ea00000\0" \
	"reversingtracksize=0\0" \
	"ipaddr=192.168.5.66\0" \
	EMMCARGS

#define CONFIG_BOOTCOMMAND	\
	"if test ${do_update} = yes; then " \
		"echo display update progess ...; " \
		"disconfig 0; "\
		"mtdparts default; " \
		"echo update ubootspl ...; " \
		"run bootstrapupdate; " \
		"disconfig 10; "\
		"echo update bootloader ...; " \
		"run bootloaderupdate; " \
		"disconfig 15; "\
		"echo update fdt ...; " \
		"run fdtupdate; " \
		"disconfig 25; "\
		"echo update kernel ...; " \
		"run kernelupdate; " \
		"disconfig 45; "\
		"echo update rootfs ...; " \
		"run rootfsupdate; " \
		"disconfig 80; "\
		"echo update bootanimation ...; " \
		"run bootanimationupdate; " \
		"disconfig 85; "\
		"echo update reversingtrack ...; " \
		"run reversingtrackupdate; " \
		"disconfig 90; "\
		"setenv do_update no; " \
		"disconfig 95; "\
		"saveenv; " \
		"disconfig 100; "\
		"run emmcboot; " \
	"else " \
		"run emmcboot; " \
	"fi"

#elif defined(CONFIG_NAND_BOOT)

#define CONFIG_EXTRA_ENV_SETTINGS \
	"need_update=yes\0" \
	"update_dev_type=mmc\0" \
	"update_dev_part=1\0" \
	"sd_dev_part="CONFIG_SD_DEV_PART"\0" \
	"loadaddr=0x44000000\0" \
	"fdtaddr=0x42000000\0" \
	"kerneladdr=0x42100000\0" \
	"bootanimationaddr=0x5e000000\0" \
	"bootanimationsize=0\0" \
	"reversingtrackaddr=0x5ea00000\0" \
	"reversingtracksize=0\0" \
	"ipaddr=192.168.5.66\0" \
	"nandfdt="CONFIG_DEFAULT_FDT_FILE"\0" \
	NANDARGS

#define CONFIG_BOOTCOMMAND	\
	"if test ${do_update} = yes; then " \
		"echo display update progess ...; " \
		"disconfig 0; "\
		"mtdparts default; " \
		"echo update ubootspl ...; " \
		"run bootstrapupdate; " \
		"disconfig 10; "\
		"echo update bootloader ...; " \
		"run bootloaderupdate; " \
		"disconfig 15; "\
		"echo update fdt ...; " \
		"run fdtupdate; " \
		"disconfig 25; "\
		"echo update kernel ...; " \
		"run kernelupdate; " \
		"disconfig 45; "\
		"echo update rootfs ...; " \
		"run rootfsupdate; " \
		"disconfig 80; "\
		"echo update bootanimation ...; " \
		"run bootanimationupdate; " \
		"disconfig 85; "\
		"echo update reversingtrack ...; " \
		"run reversingtrackupdate; " \
		"disconfig 90; "\
		"echo update bootloader back ...; " \
		"run bootloaderupdate_back; " \
		"disconfig 95; "\
		"setenv do_update no; " \
		"echo set need_update no ...; " \
		"setenv need_update no; " \
		"disconfig 97; "\
		"saveenv; " \
		"disconfig 100; "\
		"run nandboot; " \
	"else " \
		"run nandboot; " \
	"fi"
	
#endif

#endif

