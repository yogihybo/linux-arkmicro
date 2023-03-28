################################################################################
#
# ARK1668EAPP
#
################################################################################

#ARK1668EAPP_VERSION = 1.0
ARK1668EAPP_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/ark1668eapp
ARK1668EAPP_SITE_METHOD = local
ARK1668EAPP_DEPENDENCIES = qt5base libarkapi hx170dec carlink
ARK1668EAPP_INSTALL_STAGING = YES

define ARK1668EAPP_BUILD_CMDS
#$(TARGET_DIR)/../host/bin/qmake
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/UserInterface/UserInterface.pro -o $(@D)/ArkSdk/UserInterface/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/UserInterface
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/Utility/Utility.pro -o $(@D)/ArkSdk/Utility/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/Utility
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/RunnableThread/RunnableThread.pro -o $(@D)/ArkSdk/RunnableThread/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/RunnableThread
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/ArkApplication/ArkApplication.pro -o $(@D)/ArkSdk/ArkApplication/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/ArkApplication
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/AudioService/AudioService.pro -o $(@D)/ArkSdk/AudioService/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/AudioService
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/AutoConnect/AutoConnect.pro -o $(@D)/ArkSdk/AutoConnect/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/AutoConnect
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/DbusService/DbusService.pro -o $(@D)/ArkSdk/DbusService/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/DbusService
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/MultimediaService/MultimediaService.pro -o $(@D)/ArkSdk/MultimediaService/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/MultimediaService
$(HOST_DIR)/bin/qmake $(@D)/ArkSdk/1668eArk-IVI/1668eArk-IVI.pro -o $(@D)/ArkSdk/1668eArk-IVI/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/ArkSdk/1668eArk-IVI
endef

define ARK1668EAPP_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/ArkSdk/Package/1668eArk-IVI/out/Launcher $(TARGET_DIR)/usr/bin/Launcher
$(INSTALL) -D -m 0755 $(@D)/ArkSdk/1668eArk-IVI/Recource/Devb/WSVGA.rcc $(TARGET_DIR)/usr/share/WSVGA.rcc
$(INSTALL) -D -m 0755 $(@D)/lib/libConvert.so $(TARGET_DIR)/usr/lib/libConvert.so
$(INSTALL) -D -m 0755 $(@D)/lib/libCoreUtils.so $(TARGET_DIR)/usr/lib/libCoreUtils.so
$(INSTALL) -D -m 0755 $(@D)/lib/librtkvnd.so $(TARGET_DIR)/usr/lib/librtkvnd.so
$(INSTALL) -D -m 0755 $(@D)/lib/libtag.so.1  $(TARGET_DIR)/usr/lib/libtag.so.1
$(INSTALL) -D -m 0755 $(@D)/lib/libtag.so  $(TARGET_DIR)/usr/lib/libtag.so
#$(INSTALL) -D -m 0755 $(@D)/lib/qt/plugins/imageformats/libqgif.so  $(TARGET_DIR)/usr/lib/qt/plugins/imageformats/libqgif.so
#$(INSTALL) -D -m 0755 $(@D)/lib/qt/plugins/imageformats/libqico.so  $(TARGET_DIR)/usr/lib/qt/plugins/imageformats/libqico.so
#$(INSTALL) -D -m 0755 $(@D)/lib/qt/plugins/imageformats/libqjpeg.so  $(TARGET_DIR)/usr/lib/qt/plugins/imageformats/libqjpeg.so
$(INSTALL) -D -m 0755 $(@D)/lib/fonts/wqy-microhei.ttc  $(TARGET_DIR)/usr/lib/fonts/wqy-microhei.ttc
endef


$(eval $(generic-package))
