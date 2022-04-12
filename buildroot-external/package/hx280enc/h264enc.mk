################################################################################
#
# hx280enc
#
################################################################################

#HX280ENC_VERSION = 1.0
HX280ENC_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/hx280enc/software
HX280ENC_SITE_METHOD = local
HX280ENC_INSTALL_STAGING = YES

define HX280ENC_BUILD_CMDS
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)/linux_reference versatile
endef

define HX280ENC_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/linux_reference/lib8290enc.a $(STAGING_DIR)/usr/lib/lib8290enc.a
$(INSTALL) -D -m 0644 $(@D)/inc/*.h $(STAGING_DIR)/usr/include/
endef

$(eval $(generic-package))
