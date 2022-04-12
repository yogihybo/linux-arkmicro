################################################################################
#
# DashBoard
#
################################################################################

#DASHBOARD_VERSION = 1.0
DASHBOARD_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/DashBoard
DASHBOARD_SITE_METHOD = local
DASHBOARD_DEPENDENCIES = qt5base libarkapi hx170dec carlink
DASHBOARD_INSTALL_STAGING = YES

define DASHBOARD_BUILD_CMDS
#$(TARGET_DIR)/../host/bin/qmake
$(HOST_DIR)/bin/qmake $(@D)/DashBoard.pro -o $(@D)/Makefile
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)
endef

define DASHBOARD_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/out/bin/DashBoard $(TARGET_DIR)/usr/bin/DashBoard
endef

$(eval $(generic-package))
