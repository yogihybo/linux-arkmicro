################################################################################
#
# libdns_sd
#
################################################################################

#LIBDNS_SD_VERSION = 1.0
LIBDNS_SD_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/libdns_sd/software
LIBDNS_SD_SITE_METHOD = local
LIBDNS_SD_INSTALL_STAGING = YES

define LIBDNS_SD_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/lib/libdns_sd.so $(TARGET_DIR)/usr/lib/libdns_sd.so
$(INSTALL) -D -m 0755 $(@D)/lib/libdns_sd.so $(STAGING_DIR)/usr/lib/libdns_sd.so
$(INSTALL) -D -m 0755 $(@D)/lib/mdnsd $(TARGET_DIR)/usr/bin/mdnsd
$(INSTALL) -D -m 0755 $(@D)/lib/mdnsd $(STAGING_DIR)/usr/bin/mdnsd
mkdir -p $(STAGING_DIR)/usr/include/
$(INSTALL) -D -m 0644 $(@D)/include/*.h  $(STAGING_DIR)/usr/include/
endef

$(eval $(generic-package))
