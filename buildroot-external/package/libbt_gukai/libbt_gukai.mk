################################################################################
#
# libbt_gukai
#
################################################################################

LIBBT_GUKAI_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libbt_gukai/software
LIBBT_GUKAI_SITE_METHOD = local
LIBBT_GUKAI_INSTALL_STAGING = YES

define LIBBT_GUKAI_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/lib/libGbtsTask.so $(TARGET_DIR)/usr/lib/libGbtsTask.so
$(INSTALL) -D -m 0755 $(@D)/lib/gocsdk $(TARGET_DIR)/usr/bin/gocsdk

endef

$(eval $(generic-package))
