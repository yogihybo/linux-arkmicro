################################################################################
#
# libarkapi
#
################################################################################

#LIBARKAPI_VERSION = 1.0
LIBARKAPI_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libarkapi/software
LIBARKAPI_SITE_METHOD = local
LIBARKAPI_INSTALL_STAGING = YES
LIBARKAPI_DEPENDENCIES += hx170dec
ifeq ($(BR2_LIBARKAPI_ARK1668),y)
LIBARKAPI_PLATFORM = LIBARKAPI_ARK1668
LIBARKAPI_DEPENDENCIES += libgal
else ifeq ($(BR2_LIBARKAPI_ARKN141),y)
LIBARKAPI_PLATFORM = LIBARKAPI_ARKN141
else ifeq ($(BR2_LIBARKAPI_ARK1668E),y)
LIBARKAPI_PLATFORM = LIBARKAPI_ARK1668E
endif

define LIBARKAPI_BUILD_CMDS
$(MAKE) LIBARKAPI_PLATFORM=$(LIBARKAPI_PLATFORM) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)
endef

define LIBARKAPI_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/libarkapi.so $(TARGET_DIR)/usr/lib/libarkapi.so
$(INSTALL) -D -m 0755 $(@D)/libarkapi.so $(STAGING_DIR)/usr/lib/libarkapi.so
$(INSTALL) -D -m 0644 $(@D)/include/ark_list.h $(@D)/include/ark_api.h $(STAGING_DIR)/usr/include/
endef

$(eval $(generic-package))
