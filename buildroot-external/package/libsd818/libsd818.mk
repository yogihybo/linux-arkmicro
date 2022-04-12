################################################################################
#
# libsd818
#
################################################################################

#LIBSD818_VERSION = 1.0
LIBSD818_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libsd818/software
LIBSD818_SITE_METHOD = local
LIBSD818_INSTALL_STAGING = YES

define LIBSD818_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/lib/libbt_stack.so $(TARGET_DIR)/usr/lib/libbt_stack.so
$(INSTALL) -D -m 0755 $(@D)/lib/libwapm.so $(TARGET_DIR)/usr/lib/libwapm.so
$(INSTALL) -D -m 0755 $(@D)/lib/gocsdk $(TARGET_DIR)/usr/bin/gocsdk
$(INSTALL) -D -m 0755 $(@D)/lib/config.ini $(TARGET_DIR)/usr/config.ini

endef

$(eval $(generic-package))
