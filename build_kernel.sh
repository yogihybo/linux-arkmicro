#!/bin/bash
# build_kernel.sh — build the 4.19.192 kernel + modules + DTB for the
# ark1668_limcet_p305 board and assemble the final zImage.w_dtb boot image.
#
# Wraps the manual steps in README.md's "Kernel" section: defconfig, zImage,
# modules, modules_install, dtbs, then appending the DTB to the zImage (the
# ARK1668 bootloader expects the DTB appended directly to the compressed
# zImage, not loaded as a separate file in this boot mode). No source patch
# routine currently applies to the kernel build (unlike U-Boot's mandatory
# ARK-header injection) — this script still checks for and runs one at
# patches/kernel/apply.sh if it ever gets added, so this stays the one place
# that needs updating if that changes.
#
# Usage:
#   ./build_kernel.sh [options]
#
# Options:
#   --clean         make mrproper before building (full clean rebuild)
#   --defconfig     Re-apply ark1668_defconfig even if .config already
#                   exists (default: only applied if .config is missing)
#   -j N            Parallel jobs (default: nproc)
#   --help          Show this help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LINUX_DIR="$SCRIPT_DIR/linux"
MODULES_OUT="$SCRIPT_DIR/compiled_modules"
DEFCONFIG="ark1668_defconfig"
DTB_NAME="ark1668_limcet_p305.dtb"

RED='\033[0;31m'; YELLOW='\033[1;33m'; GREEN='\033[0;32m'
CYAN='\033[0;36m'; BOLD='\033[1m'; RESET='\033[0m'
info()    { echo -e "${CYAN}==> $*${RESET}"; }
success() { echo -e "${GREEN}✔ $*${RESET}"; }
warn()    { echo -e "${YELLOW}⚠ $*${RESET}"; }
die()     { echo -e "${RED}ERROR: $*${RESET}" >&2; exit 1; }

CLEAN=false
FORCE_DEFCONFIG=false
JOBS="$(nproc)"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)      CLEAN=true; shift ;;
        --defconfig)  FORCE_DEFCONFIG=true; shift ;;
        -j)           JOBS="$2"; shift 2 ;;
        --help|-h)    grep '^#' "$0" | grep -v '^#!/' | sed 's/^# \?//'; exit 0 ;;
        *) die "Unknown option: $1 (see --help)" ;;
    esac
done

[[ -d "$LINUX_DIR" ]] || die "linux/ not found at $LINUX_DIR"
[[ -f "$SCRIPT_DIR/env.source" ]] || die "env.source not found at $SCRIPT_DIR — needed for CROSS_COMPILE/ARCH/PATH"

# shellcheck source=/dev/null
source "$SCRIPT_DIR/env.source"
command -v "${CROSS_COMPILE}gcc" &>/dev/null || die "${CROSS_COMPILE}gcc not found on PATH after sourcing env.source — check buildroot-external/toolchain/"

cd "$LINUX_DIR"

if $CLEAN; then
    info "Cleaning (make mrproper)..."
    make mrproper
fi

if $FORCE_DEFCONFIG || [[ ! -f .config ]]; then
    info "Applying $DEFCONFIG + olddefconfig..."
    make "$DEFCONFIG"
    make olddefconfig
else
    info ".config already exists — skipping defconfig (pass --defconfig to force)"
fi

# --- Patch routine (kernel) ---
# No source patches currently apply to this kernel build — the project's
# fixes (e.g. the OSD1 enable-latch fix, logging cleanup — see
# prado-firmware-reconstruction/docs/DISPLAY_SUBSYSTEM.md) are committed
# directly into linux/, not applied as a separate step. Kept as an explicit
# checked-for hook so a future patch set has one obvious place to wire in,
# consistent with build_uboot.sh's inject_ark_header.py step.
KERNEL_PATCH_SCRIPT="$SCRIPT_DIR/patches/kernel/apply.sh"
if [[ -f "$KERNEL_PATCH_SCRIPT" ]]; then
    info "Running patch routine: $KERNEL_PATCH_SCRIPT..."
    bash "$KERNEL_PATCH_SCRIPT" "$LINUX_DIR"
else
    info "No kernel patch routine present (patches/kernel/apply.sh not found) — skipping"
fi

info "Building zImage (-j$JOBS)..."
make -j"$JOBS" zImage 2>&1 | tee build_kernel.log
[[ -f arch/arm/boot/zImage ]] || die "Build did not produce arch/arm/boot/zImage — check build_kernel.log"
success "zImage built"

info "Building modules (-j$JOBS)..."
make -j"$JOBS" modules 2>&1 | tee -a build_kernel.log
success "Modules built"

info "Installing modules to $MODULES_OUT..."
mkdir -p "$MODULES_OUT"
make modules_install INSTALL_MOD_PATH="$MODULES_OUT" 2>&1 | tail -5
success "Modules installed"

info "Building device tree blobs..."
make dtbs 2>&1 | tail -10
[[ -f "arch/arm/boot/dts/$DTB_NAME" ]] || die "dtbs build did not produce arch/arm/boot/dts/$DTB_NAME"
success "DTB built: arch/arm/boot/dts/$DTB_NAME"

info "Assembling zImage.w_dtb (zImage + DTB appended)..."
cat arch/arm/boot/zImage "arch/arm/boot/dts/$DTB_NAME" > "$SCRIPT_DIR/zImage.w_dtb"
success "zImage.w_dtb written: $SCRIPT_DIR/zImage.w_dtb"

echo ""
success "Done. Outputs:"
ls -lh arch/arm/boot/zImage "arch/arm/boot/dts/$DTB_NAME" "$SCRIPT_DIR/zImage.w_dtb"
KVER="$(ls "$MODULES_OUT/lib/modules/" 2>/dev/null | head -1)"
[[ -n "$KVER" ]] && echo "  Modules: $MODULES_OUT/lib/modules/$KVER/ ($(find "$MODULES_OUT/lib/modules/$KVER" -name '*.ko' 2>/dev/null | wc -l) .ko files)"
echo ""
info "prado-firmware-reconstruction/build_bootable_sdcard.sh auto-detects this"
info "  tree (as a sibling dir, or ~/Downloads/linux-arkmicro) when --new-kernel is on."
