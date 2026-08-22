# Real version bumps for in-tree Buildroot packages, applied via
# Buildroot's own <PKG>_OVERRIDE_SRCDIR mechanism (BR2_PACKAGE_OVERRIDE_FILE,
# see Config.in) rather than editing buildroot/package/*/*.mk directly --
# same "don't touch the vendored Buildroot tree" convention as the
# fakeroot/m4 patches in buildroot-external/patches/, just a different
# real mechanism since those were source patches (BR2_GLOBAL_PATCH_DIR)
# and these are version bumps (OVERRIDE_SRCDIR skips the normal
# download/hash-check step and rsyncs from a local source dir instead).
#
# IMPORTANT, confirmed by reading package/pkg-generic.mk directly:
# BR2_GLOBAL_PATCH_DIR patches are architecturally NEVER applied to
# OVERRIDE_SRCDIR packages in this Buildroot version (2019.05-rc1) --
# the whole patch-before-configure dependency chain only exists inside
# `ifeq ($(PKG)_OVERRIDE_SRCDIR,)`. Real error hit trying the opposite
# assumption: a hostapd patch silently never applied (no "Patching"
# step in the build log at all), the built .config still had the
# un-patched defaults. Buildroot's OWN existing package patches (e.g.
# libopenssl's 3) still get applied normally, because those are
# baked into the .mk's own $(PKG)_PATCH mechanism (line ~205 of
# pkg-generic.mk), separate from the GLOBAL_PATCH_DIR step -- only
# GLOBAL_PATCH_DIR is skipped for override builds. Any NEW
# customization needed on top of an override'd source tree (see
# hostapd's DPP disable below) has to be a direct edit to the
# extracted source in local-src/, not a patch file.
#
# local-src/ itself is gitignored (88MB, too large to vendor, same
# "tarball redownload is the reproducible artifact" convention as
# custom_ui/third_party/build_boost.sh) -- see local-src/README.md for
# the exact, real fetch+extract+patch commands to reproduce every
# directory referenced below, including the one direct-edit
# customization. See merry-snacking-wirth.md's version-currency-audit
# section for the full per-package assessment.

# openssl: 1.1.1b -> 1.1.1w (same-branch final release of the 1.1.1
# LTS series -- EOL Sept 2023, but the last point release, fixing
# real CVEs across the branch's life). All 3 of Buildroot's own
# libopenssl patches confirmed to still apply cleanly (checked via
# `patch --dry-run` before committing to this bump).
LIBOPENSSL_OVERRIDE_SRCDIR = $(BR2_EXTERNAL_ARK_PATH)/local-src/openssl-1.1.1w

# dbus: 1.12.10 -> 1.12.20 (same-branch, fixes real CVE-2020-12049 --
# a NULL-deref DoS via a malformed message). No existing Buildroot
# patches for this package, nothing to re-verify compatibility for.
DBUS_OVERRIDE_SRCDIR = $(BR2_EXTERNAL_ARK_PATH)/local-src/dbus-1.12.20

# hostapd/wpa_supplicant: 2.7 -> 2.10 (includes the FragAttacks fixes
# and other real WiFi-security hardening from 2.9/2.10). Buildroot's
# one existing wpa_supplicant patch (a Dec-2018 upstream commit,
# CONFIG_IEEE80211X build fix) is already incorporated upstream in
# 2.10 -- confirmed via `patch --dry-run` reporting "Reversed (or
# previously applied) patch detected," not assumed. This project's own
# hostapd-custom_ui.conf directives (ssid/hw_mode/channel/driver/
# wpa_*/access_network_type/venue_*/vendor_elements) all checked
# against 2.10's real config syntax -- none renamed or deprecated in
# this range. One real, non-blocking note: wpa_pairwise=TKIP is a
# legacy cipher, may log a deprecation warning on 2.10 but doesn't
# break config parsing.
#
# Real build-system gap found and fixed: 2.10's own hostapd/defconfig
# enables CONFIG_DPP=y/CONFIG_DPP2=y by default (Wi-Fi Easy Connect --
# didn't exist as an option in 2.7, so Buildroot's own sed-based
# HOSTAPD_CONFIG_ENABLE list never needed to touch it). DPP's dpp.c
# needs crypto_ec_key_*() wrapper functions gated behind a SEPARATE
# CONFIG_ECC flag that 2.10's defconfig has no line for at all (nothing
# to uncomment) -- real link failure (`undefined reference to
# crypto_ec_key_deinit` etc.), not a source patch issue. This project
# doesn't use DPP provisioning at all (plain WPA-PSK), so fixed by
# commenting out CONFIG_DPP/CONFIG_DPP2 directly in this repo's own
# staged local-src/hostapd-2.10/hostapd/defconfig copy (not Buildroot's
# own hostapd.mk) -- lower risk than adding CONFIG_ECC support neither
# this project nor Buildroot's existing config-generation logic has
# ever exercised. Real build-verified after the fix (`hostapd`/
# `wpa_supplicant` both link and install cleanly, both report real
# version 2.10 via `strings`).
HOSTAPD_OVERRIDE_SRCDIR = $(BR2_EXTERNAL_ARK_PATH)/local-src/hostapd-2.10
WPA_SUPPLICANT_OVERRIDE_SRCDIR = $(BR2_EXTERNAL_ARK_PATH)/local-src/wpa_supplicant-2.10

# host-fakeroot: 1.20.2 -> 1.31 -- real BUG fix, not just a version
# refresh. 1.20.2's chown-faking is broken on this host: an isolated
# test (fakeroot session: `chown -h -R 0:0 file`, then `stat` the SAME
# file in the SAME fakeroot session) reports the real build-user
# uid/gid unchanged, not 0:0 -- confirmed with and without
# FAKEROOTDONTTRYCHOWN, confirmed chmod-faking (setuid bits etc.) DOES
# work correctly via the same test, so this is specific to chown, not
# a general fakeroot/LD_PRELOAD failure. The system's own installed
# fakeroot (1.31) passes the identical test cleanly -- this bump
# targets that same known-good version.
#
# This host-only bug directly affects fs/common.mk's real rootfs-image
# generation (BR2_TARGET_ROOTFS_TAR and any other format): its
# fakeroot pass's `chown -h -R 0:0 $(TARGET_DIR)` never actually took
# effect, so every generated image (tar, ext2/ext4 via mke2fs -d, etc.)
# shipped with the build user's own uid/gid baked in instead of real
# root:root ownership -- confirmed via `tar -tvf` on a real built
# rootfs.tar before this fix.
#
# Real, NOT just cosmetic: 1.31 upstream already carries its own
# per-arch _STAT_VER fallback (libfakeroot.c, same x86_64 value the
# 1.20.2 host-glibc compat patch in buildroot-external/patches/fakeroot/
# used) -- confirmed by direct inspection of the 1.31 source, so that
# patch is now OBSOLETE for this version, not reapplied. It's kept in
# place (harmless, unused -- BR2_GLOBAL_PATCH_DIR patches never apply
# to OVERRIDE_SRCDIR packages anyway, per this file's own header note)
# rather than deleted, in case a future version regresses this again.
#
# Build-verified: the isolated chown/stat repro now passes (0:0
# reported back correctly), and a real `make rootfs-tar` rebuild with
# this fakeroot produced a rootfs.tar with genuine root:root ownership
# throughout (`tar -tvf` confirms 0/0, not the build user's uid/gid).
HOST_FAKEROOT_OVERRIDE_SRCDIR = $(BR2_EXTERNAL_ARK_PATH)/local-src/fakeroot-1.31
