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

$(INSTALL) -D -m 0755 $(@D)/lib/eclink/TYW02/libeclinkplayer.so $(TARGET_DIR)/usr/lib/libeclinkplayer.so
$(INSTALL) -D -m 0755 $(@D)/lib/eclink/TYW02/libECSDK.so $(TARGET_DIR)/usr/lib/libECSDK.so
$(INSTALL) -D -m 0755 $(@D)/lib/eclink/TYW02/libECSDKFramework.so $(TARGET_DIR)/usr/lib/libECSDKFramework.so

$(INSTALL) -D -m 0755 $(@D)/lib/mirror/libmirrorplayer.so $(TARGET_DIR)/usr/lib/libmirrorplayer.so

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

$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libarkadapt.so $(TARGET_DIR)/usr/lib/libarkadapt.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libauthagent.so $(TARGET_DIR)/usr/lib/libauthagent.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdp.so $(TARGET_DIR)/usr/lib/libdmsdp.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpaudiohandler.so $(TARGET_DIR)/usr/lib/libdmsdpaudiohandler.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpcamerahandler.so $(TARGET_DIR)/usr/lib/libdmsdpcamerahandler.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpcrypto.so $(TARGET_DIR)/usr/lib/libdmsdpcrypto.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpdvaudio.so $(TARGET_DIR)/usr/lib/libdmsdpdvaudio.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpdvcamera.so $(TARGET_DIR)/usr/lib/libdmsdpdvcamera.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpdvdevice.so $(TARGET_DIR)/usr/lib/libdmsdpdvdevice.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpdvgps.so $(TARGET_DIR)/usr/lib/libdmsdpdvgps.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpdvinterface.so $(TARGET_DIR)/usr/lib/libdmsdpdvinterface.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdphisight.so $(TARGET_DIR)/usr/lib/libdmsdphisight.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpplatform.so $(TARGET_DIR)/usr/lib/libdmsdpplatform.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libdmsdpsec.so $(TARGET_DIR)/usr/lib/libdmsdpsec.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libhicar.so $(TARGET_DIR)/usr/lib/libhicar.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libhievent.so $(TARGET_DIR)/usr/lib/libhievent.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libhilog.so $(TARGET_DIR)/usr/lib/libhilog.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libHisightSink.so $(TARGET_DIR)/usr/lib/libHisightSink.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libhitrace.so $(TARGET_DIR)/usr/lib/libhitrace.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libhiviewlite.so $(TARGET_DIR)/usr/lib/libhiviewlite.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libHwDeviceAuthSDK.so $(TARGET_DIR)/usr/lib/libHwDeviceAuthSDK.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libHwKeystoreSDK.so $(TARGET_DIR)/usr/lib/libHwKeystoreSDK.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libmanagement.so $(TARGET_DIR)/usr/lib/libmanagement.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libnearby.so $(TARGET_DIR)/usr/lib/libnearby.so
$(INSTALL) -D -m 0755 $(@D)/lib/hicar/libsecurec.so $(TARGET_DIR)/usr/lib/libsecurec.so

$(INSTALL) -D -m 0755 $(@D)/lib/user/libUserInterface.so $(TARGET_DIR)/usr/lib/libUserInterface.so

$(INSTALL) -D -m 0755 $(@D)/bin/carlink $(TARGET_DIR)/usr/bin
$(INSTALL) -D -m 0755 $(@D)/bin/demo $(TARGET_DIR)/usr/bin
$(INSTALL) -D -m 0755 $(@D)/bin/demo_dbus $(TARGET_DIR)/usr/bin
$(INSTALL) -D -m 0755 $(@D)/cmd/carlink_cmd $(TARGET_DIR)/usr/bin

$(INSTALL) -D -m 0755 $(@D)/PhoneLink.ini $(TARGET_DIR)/etc


mkdir -p $(STAGING_DIR)/usr/include/carlink


endef

$(eval $(generic-package))

