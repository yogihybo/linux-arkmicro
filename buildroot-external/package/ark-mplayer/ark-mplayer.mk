################################################################################
#
# ark-mplayer
#
################################################################################

#ARK_MPLAYER_VERSION = 1.0
ARK_MPLAYER_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/ark-mplayer/bin
ARK_MPLAYER_SITE_METHOD = local
ARK_MPLAYER_INSTALL_STAGING = YES

define ARK_MPLAYER_INSTALL_STAGING_CMDS
$(INSTALL) -D -m 0755 $(@D)/mplayer $(TARGET_DIR)/usr/bin/mplayer
$(INSTALL) -D -m 0755 $(@D)/mplayer $(STAGING_DIR)/usr/bin/mplayer
endef

$(eval $(generic-package))
