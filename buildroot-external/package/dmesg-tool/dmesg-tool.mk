################################################################################
#
# dmesg-tool
#
################################################################################

DMESG_TOOL_SITE = $(BR2_EXTERNAL_ARK_PATH)/../../prado-firmware-reconstruction/tools/dmesg
DMESG_TOOL_SITE_METHOD = local

define DMESG_TOOL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/dmesg $(TARGET_DIR)/usr/bin/dmesg
endef

$(eval $(generic-package))
