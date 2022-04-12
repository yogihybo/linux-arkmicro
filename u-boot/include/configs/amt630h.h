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

/* Memory layout */
#define CONFIG_NR_DRAM_BANKS			1
#define CONFIG_SYS_SDRAM_BASE		0x20000000
#define CONFIG_SYS_UBOOT_BASE		CONFIG_SYS_TEXT_BASE

/*
 * The board really has 256M. However, the VC (VideoCore co-processor) shares
 * the RAM, and uses a configurable portion at the top. We tell U-Boot that a
 * smaller amount of RAM is present in order to avoid stomping on the area
 * the VC uses.
 */
#define CONFIG_SYS_SDRAM_SIZE		SZ_32M //(SZ_128M + SZ_32M + SZ_16M + SZ_4M)
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + CONFIG_SYS_SDRAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_MALLOC_LEN		0x80000

/* timer base address */
#define CONFIG_SYS_TIMERBASE		0x60a00000

/* watchdog base address */
#define CONFIG_WATCHDOG_BASEADDR	0x60c00000

/* usb base address */
#define CONFIG_USB_DWC2_REG_ADDR	0x70300000
#define CONFIG_DWC2_UTMI_WIDTH		16


#define CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_DEVICE

/*
 * SPI Flash controller Configuration
 */
#define CONFIG_SYS_SPI_BASE			0x60100000
#define CONFIG_ENV_SECT_SIZE 		65536
#define CONFIG_SF_DEFAULT_MODE		(SPI_MODE_0)
#define CONFIG_ENV_SPI_MODE 		CONFIG_SF_DEFAULT_MODE
#define CONFIG_SF_DEFAULT_SPEED		50000000
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED

#define SFARGS \
	"mtdids=" CONFIG_MTDIDS_DEFAULT "\0" \
	"mtdparts=" CONFIG_MTDPARTS_DEFAULT "\0" \
	"bootstrapupdatesd=fatload ${update_dev} ${dev_part} ${loadaddr} LOADSYS.BIN; " \
		"sf erase bootstrap 0; " \
		"sf write ${loadaddr} bootstrap ${filesize}\0" \
	"fdtupdatesd=fatload ${update_dev} ${dev_part} ${loadaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"setenv fdtsize ${filesize}; " \
		"sf erase fdt 0; " \
		"sf write ${loadaddr} fdt ${filesize}\0" \
	"kernelupdatesd=fatload ${update_dev} ${dev_part} ${loadaddr} zImage; " \
		"setenv kernelsize ${filesize}; " \
		"sf erase kernel 0; " \
		"sf write ${loadaddr} kernel ${filesize}\0" \
	"bootloaderupdatesd=fatload ${update_dev} ${dev_part} ${loadaddr} u-boot.bin; " \
		"setenv bootloadersize ${filesize}; " \
		"sf erase bootloader 0; " \
		"sf write ${loadaddr} bootloader ${filesize}\0" \
	"rootfsupdatesd=fatload ${update_dev} ${dev_part} ${loadaddr} rootfs.ubi; " \
		"setenv rootfssize ${filesize}; " \
		"sf erase rootfs 0; " \
		"sf write ${loadaddr} rootfs ${filesize}\0" \
	"sfargs=setenv bootargs console=ttyS0,115200 " \
		"earlyprintk  loglevel=3 clk_ignore_unused lpj=1708032 " \
		"${mtdparts} " \
		"root=${sfroot} " \
		"rootfstype=${sfrootfstype}\0" \
	"sdargs=setenv bootargs console=ttyS0,115200 " \
		"earlyprintk  loglevel=8 clk_ignore_unused lpj=1708032 2285568 enable_console root=/dev/mmcblk0p2 rw rootfstype=ext2 rootwait\0" \
	"sfroot=ubi0:rootfs rw ubi.mtd=rootfs\0" \
	"sfrootfstype=ubifs rootwait\0" \
	"sfboot=echo Booting from spi flash ...; " \
		"run sfargs; " \
		"sf read ${fdtaddr} fdt ${fdtsize}; " \
		"sf read ${kerneladdr} kernel ${kernelsize}; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0" \
	"sdboot=echo Booting from sd...; " \
		"run sdargs; " \
		"fatload mmc 0 ${fdtaddr} "CONFIG_DEFAULT_FDT_FILE"; " \
		"fatload mmc 0 ${kerneladdr} zImage; " \
		"bootz ${kerneladdr} - ${fdtaddr}\0"


/* Environment */
#define CONFIG_ENV_SIZE			4096	//CONFIG_PARTITION_UBOOT_ENV_SIZE
#define CONFIG_ENV_OFFSET		0xA0000

#define CONFIG_SYS_LOAD_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x1000000)

/* Console UART */
#define CONFIG_PL011_CLOCK		24000000
#define CONFIG_PL01x_PORTS      {(void *)CONFIG_SYS_SERIAL0, (void *)CONFIG_SYS_SERIAL1}
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_SERIAL0      0x60500000
#define CONFIG_SYS_SERIAL1      0x60600000

/* Console configuration */
#define CONFIG_SYS_CBSIZE		512
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_BOUNCE_BUFFER

#define CONFIG_EXTRA_ENV_SETTINGS \
	"need_update=no\0" \
	"update_dev=mmc\0" \
	"dev_part=0\0" \
	"loadaddr=0x21000000\0" \
	"fdtaddr=0x21000000\0" \
	"kerneladdr=0x21100000\0" \
	SFARGS

#define CONFIG_BOOTCOMMAND	\
	"if test ${need_update} = yes; then " \
		"echo Start update ...; " \
		"echo Update from ${update_dev} ...; " \
		"echo update bootstrap ...; " \
		"run bootstrapupdatesd; " \
		"echo update bootloader ...; " \
		"run bootloaderupdatesd; " \
		"echo update fdt ...; " \
		"run fdtupdatesd; " \
		"echo update kernel ...; " \
		"run kernelupdatesd; " \
		"echo update rootfs ...; " \
		"run rootfsupdatesd; " \
		"setenv need_update no; " \
		"saveenv; " \
		"run sfboot; " \
	"else " \
		"run sdboot; " \
	"fi"

#endif

