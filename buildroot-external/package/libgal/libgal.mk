################################################################################
#
# libgal
#
################################################################################

#LIBGAL_VERSION = 1.0
LIBGAL_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libgal/software
LIBGAL_SITE_METHOD = local
LIBGAL_INSTALL_STAGING = YES

define LIBGAL_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/lib/libGAL.so $(TARGET_DIR)/usr/lib/libGAL.so
$(INSTALL) -D -m 0755 $(@D)/lib/libGAL.so $(STAGING_DIR)/usr/lib/libGAL.so
$(INSTALL) -D -m 0755 $(@D)/lib/libGAL.fb.so $(TARGET_DIR)/usr/lib/libGAL.fb.so
$(INSTALL) -D -m 0755 $(@D)/lib/libGAL.fb.so $(STAGING_DIR)/usr/lib/libGAL.fb.so
$(INSTALL) -D -m 0755 $(@D)/lib/galcore.ko $(TARGET_DIR)/lib/modules/galcore.ko
mkdir -p $(STAGING_DIR)/usr/include/HAL
$(INSTALL) -D -m 0644 $(@D)/include/HAL/*.h  $(STAGING_DIR)/usr/include/HAL
endef

$(eval $(generic-package))
