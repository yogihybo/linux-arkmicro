# local-src/

Real upstream source trees for version-bumped packages, referenced by
`../package-overrides.mk` via Buildroot's `<PKG>_OVERRIDE_SRCDIR`
mechanism (`BR2_PACKAGE_OVERRIDE_FILE`). Not committed to git (too
large -- 88MB total, same reasoning as
`prado-firmware-reconstruction/custom_ui/third_party/build_boost.sh`'s
own "not vendored, tarball redownload is the reproducible artifact"
convention) -- reproduce with the commands below.

**Real, confirmed Buildroot constraint**: `BR2_GLOBAL_PATCH_DIR`
patches are architecturally never applied to `OVERRIDE_SRCDIR`
packages in this Buildroot version (2019.05-rc1) -- confirmed by
reading `package/pkg-generic.mk` directly: the entire
patch-before-configure dependency chain only exists inside
`ifeq ($(PKG)_OVERRIDE_SRCDIR,)`. The override mechanism assumes
you've already customized the source tree yourself. So the one real
customization below (hostapd's DPP disable) is a direct edit to the
extracted source, not a patch file -- reproduce it exactly as shown.

```sh
cd local-src

# openssl 1.1.1w (verified sha256: cf3098950cb4d853ad95c0841f1f9c6d3dc102dccfcacd521d93925208b76ac8)
curl -sL -o openssl-1.1.1w.tar.gz https://www.openssl.org/source/openssl-1.1.1w.tar.gz
tar xzf openssl-1.1.1w.tar.gz && rm openssl-1.1.1w.tar.gz
for p in ../../buildroot/package/libopenssl/*.patch; do
    patch -p1 -d openssl-1.1.1w < "$p"
done

# dbus 1.12.20 (verified sha256: f77620140ecb4cdc67f37fb444f8a6bea70b5b6461f12f1cbe2cec60fa7de5fe)
curl -sL -o dbus-1.12.20.tar.gz https://dbus.freedesktop.org/releases/dbus/dbus-1.12.20.tar.gz
tar xzf dbus-1.12.20.tar.gz && rm dbus-1.12.20.tar.gz
# no existing Buildroot patches for this package -- nothing to apply

# hostapd 2.10
curl -sL -o hostapd-2.10.tar.gz https://w1.fi/releases/hostapd-2.10.tar.gz
tar xzf hostapd-2.10.tar.gz && rm hostapd-2.10.tar.gz
# Buildroot's one wpa_supplicant patch is already incorporated upstream
# in 2.10 (confirmed via `patch --dry-run` reporting "Reversed (or
# previously applied) patch detected") -- do not apply it.
#
# Real customization, direct edit (see package-overrides.mk's own
# comment for the full "why" -- DPP/Wi-Fi-Easy-Connect references
# crypto_ec_key_*() functions gated behind a CONFIG_ECC flag this
# defconfig has no line for at all; this project doesn't use DPP):
sed -i \
    -e 's/^CONFIG_DPP=y/#CONFIG_DPP=y/' \
    -e 's/^CONFIG_DPP2=y/#CONFIG_DPP2=y/' \
    hostapd-2.10/hostapd/defconfig

# wpa_supplicant 2.10 (same tarball family as hostapd, same w1.fi release)
curl -sL -o wpa_supplicant-2.10.tar.gz https://w1.fi/releases/wpa_supplicant-2.10.tar.gz
tar xzf wpa_supplicant-2.10.tar.gz && rm wpa_supplicant-2.10.tar.gz
#
# Real customization, direct edit (2026-08-22, glibc 2.30 migration):
# driver_macsec_linux.c needs linux/if_macsec.h, which doesn't exist in
# the Bootlin armv7-eabihf--glibc--stable-2020.02-1 toolchain's kernel
# headers (4.4.215, predates mainline MACsec support) -- real build
# failure, not a config gap. This project doesn't use 802.1X/MACsec.
sed -i \
    -e 's/^CONFIG_DRIVER_MACSEC_LINUX=y/#CONFIG_DRIVER_MACSEC_LINUX=y/' \
    -e 's/^CONFIG_MACSEC=y/#CONFIG_MACSEC=y/' \
    wpa_supplicant-2.10/wpa_supplicant/defconfig
```

All four build-verified (`make BR2_EXTERNAL=../buildroot-external <pkg>`)
against `ark1668_ft_dyn_defconfig` — see `merry-snacking-wirth.md`'s
version-currency-audit section for the full per-package assessment.
