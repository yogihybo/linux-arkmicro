################################################################################
#
# ark1668eft
#
################################################################################

#ARK1668EFT_VERSION = 1.0
ARK1668EFT_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/ark1668eft/src
ARK1668EFT_SITE_METHOD = local
ARK1668EFT_INSTALL_STAGING = YES
ARK1668EFT_DEPENDENCIES += hx170dec

define ARK1668EFT_BUILD_CMDS
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)
endef

define ARK1668EFT_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/ark1668eft $(TARGET_DIR)/usr/bin/ark1668eft
$(INSTALL) -D -m 0755 $(@D)/ark1668eft $(STAGING_DIR)/usr/bin/ark1668eft
endef

$(eval $(generic-package))
