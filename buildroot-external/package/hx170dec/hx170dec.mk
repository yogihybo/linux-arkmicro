################################################################################
#
# hx170dec
#
################################################################################

#HX170DEC_VERSION = 1.0
HX170DEC_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/hx170dec/software
HX170DEC_SITE_METHOD = local
HX170DEC_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_HX280ENC),y)
HX170DEC_CFLAG = -DDEC_WITH_ENC
endif

define HX170DEC_BUILD_CMDS
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D) CFLAG="$(HX170DEC_CFLAG)"
endef

define HX170DEC_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/libmfc.so $(TARGET_DIR)/usr/lib/libmfc.so
$(INSTALL) -D -m 0755 $(@D)/libmfc.so $(STAGING_DIR)/usr/lib/libmfc.so
$(INSTALL) -D -m 0644 $(@D)/include/basetype.h $(@D)/include/decapicommon.h $(@D)/include/dwl.h \
	$(@D)/include/memalloc.h $(@D)/include/mfcapi.h $(@D)/include/ppapi.h $(STAGING_DIR)/usr/include/
endef

$(eval $(generic-package))
