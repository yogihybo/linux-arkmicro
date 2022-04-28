################################################################################
#
# libmali
#
################################################################################

#LIBMALI_VERSION = 1.0
LIBMALI_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libmali
LIBMALI_SITE_METHOD = local
LIBMALI_INSTALL_STAGING = YES

LIBMALI_PROVIDES = libegl libgles

define LIBMALI_INSTALL_STAGING_CMDS
	mkdir -p $(STAGING_DIR)/usr/lib $(STAGING_DIR)/usr/include
	cp -rf $(@D)/lib/*.so* $(STAGING_DIR)/usr/lib/
	cp -rf $(@D)/include/* $(STAGING_DIR)/usr/include/
	$(INSTALL) -D -m 0644 $(@D)/egl.pc $(STAGING_DIR)/usr/lib/pkgconfig/egl.pc
	$(INSTALL) -D -m 0644 $(@D)/glesv2.pc $(STAGING_DIR)/usr/lib/pkgconfig/glesv2.pc
endef

define LIBMALI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib
	cp -rf $(@D)/lib/*.so* $(TARGET_DIR)/usr/lib/
endef


$(eval $(generic-package))
