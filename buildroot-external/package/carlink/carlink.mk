################################################################################
#
# carlink libs and demo
#
################################################################################

#CARLINK_VERSION = 1.0
CARLINK_SITE = $(BR2_EXTERNAL_ARK_PATH)/package/carlink
CARLINK_SITE_METHOD = local
CARLINK_INSTALL_STAGING = YES

define CARLINK_INSTALL_STAGING_CMDS
mkdir -p $(STAGING_DIR)/usr/lib/carlink
$(INSTALL) -D -m 0755 $(@D)/lib/auto/libAndroidAuto.so $(TARGET_DIR)/usr/lib/libAndroidAuto.so
$(INSTALL) -D -m 0755 $(@D)/lib/auto/libarkCarlinkUtils.so $(TARGET_DIR)/usr/lib/libarkCarlinkUtils.so

$(INSTALL) -D -m 0755 $(@D)/lib/carlife/libcarlifeplayer.so $(TARGET_DIR)/usr/lib/libcarlifeplayer.so
$(INSTALL) -D -m 0755 $(@D)/lib/carlife/libcarlifevehicle.so $(TARGET_DIR)/usr/lib/libcarlifevehicle.so

$(INSTALL) -D -m 0755 $(@D)/lib/eclink/libeclinkplayer.so $(TARGET_DIR)/usr/lib/libeclinkplayer.so
$(INSTALL) -D -m 0755 $(@D)/lib/eclink/libECSDK.so $(TARGET_DIR)/usr/lib/libECSDK.so
$(INSTALL) -D -m 0755 $(@D)/lib/eclink/libECSDKFramework.so $(TARGET_DIR)/usr/lib/libECSDKFramework.so

$(INSTALL) -D -m 0755 $(@D)/lib/eclink/libECSDKFramework.so $(TARGET_DIR)/usr/lib/libmirrorplayer.so

$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libAirPlay.so $(TARGET_DIR)/usr/lib/libAirPlay.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libAirPlaySupport.so $(TARGET_DIR)/usr/lib/libAirPlaySupport.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libAudioConverter.so $(TARGET_DIR)/usr/lib/libAudioConverter.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libAudioConverter_dummy.so $(TARGET_DIR)/usr/lib/libAudioConverter_dummy.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libAudioStream.so $(TARGET_DIR)/usr/lib/libAudioStream.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libcarplaymisc.so $(TARGET_DIR)/usr/lib/libcarplaymisc.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libCarplayWrapper.so $(TARGET_DIR)/usr/lib/libCarplayWrapper.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libCoreUtils.so $(TARGET_DIR)/usr/lib/libCoreUtils.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libiap2link.so $(TARGET_DIR)/usr/lib/libiap2link.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libmyutils.so $(TARGET_DIR)/usr/lib/libmyutils.so
$(INSTALL) -D -m 0755 $(@D)/lib/carplay/libScreenStream.so $(TARGET_DIR)/usr/lib/libScreenStream.so

$(INSTALL) -D -m 0755 $(@D)/carlink $(TARGET_DIR)/usr/bin
$(INSTALL) -D -m 0755 $(@D)/demo/demo $(TARGET_DIR)/usr/bin
$(INSTALL) -D -m 0755 $(@D)/PhoneLink.ini $(TARGET_DIR)/etc


mkdir -p $(STAGING_DIR)/usr/include/carlink
mkdir -p $(STAGING_DIR)/usr/include/carlink/auto
$(INSTALL) -D -m 0644 $(@D)/include/auto/*.h  $(STAGING_DIR)/usr/include/carlink/auto


mkdir -p $(STAGING_DIR)/usr/include/carlink/carlife
$(INSTALL) -D -m 0644 $(@D)/include/carlife/*.h  $(STAGING_DIR)/usr/include/carlink/carlife


mkdir -p $(STAGING_DIR)/usr/include/carlink/carplay
$(INSTALL) -D -m 0644 $(@D)/include/carplay/*.h  $(STAGING_DIR)/usr/include/carlink/carplay

mkdir -p $(STAGING_DIR)/usr/include/carlink/eclink
$(INSTALL) -D -m 0644 $(@D)/include/eclink/*.h  $(STAGING_DIR)/usr/include/carlink/eclink

mkdir -p $(STAGING_DIR)/usr/include/carlink/mirror
$(INSTALL) -D -m 0644 $(@D)/include/mirror/*.h  $(STAGING_DIR)/usr/include/carlink/mirror

endef

$(eval $(generic-package))

