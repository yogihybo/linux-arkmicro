################################################################################
#
# ark1668ft
#
################################################################################

#ARK1668FT_VERSION = 1.0
ARK1668FT_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/ark1668ft/src
ARK1668FT_SITE_METHOD = local
ARK1668FT_INSTALL_STAGING = YES

define ARK1668FT_BUILD_CMDS
$(MAKE) CROSS_COMPILE=$(TARGET_CROSS) -C $(@D)
endef

define ARK1668FT_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/ark1668ft $(TARGET_DIR)/usr/bin/ark1668ft
$(INSTALL) -D -m 0755 $(@D)/ark1668ft $(STAGING_DIR)/usr/bin/ark1668ft
endef

$(eval $(generic-package))
