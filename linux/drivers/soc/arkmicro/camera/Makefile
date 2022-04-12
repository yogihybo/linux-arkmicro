
ccflags-y += -DGPIO_SENSOR_RST=$(CONFIG_GPIO_SENSOR_RST)
ccflags-y += -DGPIO_SENSOR_STANDBY=$(CONFIG_GPIO_SENSOR_STANDBY)

obj-$(CONFIG_SENSOR_GC2053) += gc2053.o
obj-$(CONFIG_SENSOR_IMX322) += imx322_dvr.o
imx322_dvr-objs := imx322.o imx322_core.o

obj-$(CONFIG_ARK_CAMERA)	+= ark_camera_drv.o ark_camera_core.o ark_isp_exposure.o ark_isp_exposure_cmos.o Gem_isp_ae.o \
								Gem_isp_colors.o Gem_isp_enhance.o Gem_isp_fesp.o Gem_isp_eris.o Gem_isp_denoise.o Gem_isp_awb.o Gem_isp_sys.o


