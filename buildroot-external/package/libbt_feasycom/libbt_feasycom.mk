################################################################################
#
# libbt_feasy
#
################################################################################
LIBBT_FEASYCOM_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libbt_feasycom/software
LIBBT_FEASYCOM_SITE_METHOD = local
LIBBT_FEASYCOM_INSTALL_STAGING = YES

define LIBBT_FEASYCOM_INSTALL_STAGING_CMDS
mkdir -p $(TARGET_DIR)/etc/bluetooth
$(INSTALL) -D -m 0755 $(@D)/etc/bluetooth/blueware.properties $(TARGET_DIR)/etc/bluetooth/
$(INSTALL) -D -m 0755 $(@D)/usr/bin/gocsdk $(TARGET_DIR)/usr/bin/gocsdk

endef

$(eval $(generic-package))
