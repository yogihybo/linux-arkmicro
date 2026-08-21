################################################################################
#
# bluetoothd-test
#
# Hardware-confirmed static bluetoothd 5.66 + dbus-policy configs from
# prado-firmware-reconstruction's tools/bluetoothd-test/ -- prebuilt
# binary reuse, same "local site, generic-package, plain $(INSTALL)"
# pattern as this tree's own libgal/libmali packages. Real from-source
# rebuild recipe is documented in that tool's own README if it ever
# needs to change; not duplicated here.
#
################################################################################

BLUETOOTHD_TEST_SITE = $(BR2_EXTERNAL_ARK_PATH)/../../prado-firmware-reconstruction/tools/bluetoothd-test
BLUETOOTHD_TEST_SITE_METHOD = local

define BLUETOOTHD_TEST_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/bluetoothd $(TARGET_DIR)/usr/sbin/bluetoothd
	$(INSTALL) -D -m 0644 $(@D)/dbus-policy/bluetooth.conf $(TARGET_DIR)/usr/etc/dbus-1/system.d/bluetooth.conf
	$(INSTALL) -D -m 0644 $(@D)/dbus-policy/system-diagnostic.conf $(TARGET_DIR)/etc/dbus-policy/system-diagnostic.conf
endef

$(eval $(generic-package))
