################################################################################
#
# rtk-hciattach-test
#
################################################################################

RTK_HCIATTACH_TEST_SITE = $(BR2_EXTERNAL_ARK_PATH)/../../prado-firmware-reconstruction/tools/rtk-hciattach-test
RTK_HCIATTACH_TEST_SITE_METHOD = local

define RTK_HCIATTACH_TEST_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/rtk_hciattach $(TARGET_DIR)/usr/bin/rtk_hciattach
	$(INSTALL) -D -m 0755 $(@D)/hci-updown $(TARGET_DIR)/usr/bin/hci-updown
	$(INSTALL) -D -m 0644 $(@D)/device-firmware/rtl8761bt_fw $(TARGET_DIR)/lib/firmware/rtlbt/rtl8761b_fw
	$(INSTALL) -D -m 0644 $(@D)/device-firmware/rtl8761bt_config $(TARGET_DIR)/lib/firmware/rtlbt/rtl8761b_config
endef

$(eval $(generic-package))
