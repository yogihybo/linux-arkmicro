#!/bin/bash
# build_uboot.sh — build U-Boot for the ark1668_limcet_p305 board and
# produce a bootable UBOOT.BIN.
#
# Wraps the manual steps in README.md's "U-Boot" section: defconfig,
# build, then the mandatory ARK-header injection patch routine
# (inject_ark_header.py) — the raw u-boot.bin isn't bootable on this
# hardware without it (Stepldr expects a proprietary header at fixed
# offsets that a stock U-Boot build doesn't produce).
#
# Usage:
#   ./build_uboot.sh [options]
#
# Options:
#   --clean         make mrproper before building (full clean rebuild)
#   --defconfig     Re-apply ark1668_limcet_p305_defconfig even if .config
#                   already exists (default: only applied if .config is missing)
#   -j N            Parallel jobs (default: nproc)
#   --help          Show this help

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UBOOT_DIR="$SCRIPT_DIR/u-boot"
DEFCONFIG="ark1668_limcet_p305_defconfig"

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

[[ -d "$UBOOT_DIR" ]] || die "u-boot/ not found at $UBOOT_DIR"
[[ -f "$SCRIPT_DIR/env.source" ]] || die "env.source not found at $SCRIPT_DIR — needed for CROSS_COMPILE/ARCH/PATH"

# shellcheck source=/dev/null
source "$SCRIPT_DIR/env.source"
command -v "${CROSS_COMPILE}gcc" &>/dev/null || die "${CROSS_COMPILE}gcc not found on PATH after sourcing env.source — check buildroot-external/toolchain/"

cd "$UBOOT_DIR"

if $CLEAN; then
    info "Cleaning (make mrproper)..."
    make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" mrproper
fi

if $FORCE_DEFCONFIG || [[ ! -f .config ]]; then
    info "Applying $DEFCONFIG..."
    make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" "$DEFCONFIG"
else
    info ".config already exists — skipping defconfig (pass --defconfig to force)"
fi

info "Sanity-checking config (no-SPL flat binary expected)..."
if ! grep -q "^# CONFIG_SPL is not set" .config; then
    warn "CONFIG_SPL appears to be enabled — this board expects a no-SPL flat binary. Check .config."
fi
if ! grep -q "^CONFIG_SYS_TEXT_BASE=0x30000" .config; then
    warn "CONFIG_SYS_TEXT_BASE isn't 0x30000 — unexpected for this board. Check .config."
fi

info "Building U-Boot (-j$JOBS)..."
make ARCH=arm CROSS_COMPILE="$CROSS_COMPILE" -j"$JOBS" 2>&1 | tee build_uboot.log

[[ -f u-boot.bin ]] || die "Build did not produce u-boot.bin — check build_uboot.log"
[[ -f u-boot ]]     || die "Build did not produce the u-boot ELF (needed for header injection) — check build_uboot.log"
success "u-boot.bin + u-boot ELF built"

# --- Patch routine: inject the ARK header ---
info "Running patch routine: inject_ark_header.py (ARK header injection)..."
[[ -f "$SCRIPT_DIR/inject_ark_header.py" ]] || die "inject_ark_header.py not found at $SCRIPT_DIR"
python3 "$SCRIPT_DIR/inject_ark_header.py" u-boot.bin UBOOT.BIN
[[ -f UBOOT.BIN ]] || die "inject_ark_header.py did not produce UBOOT.BIN"
success "UBOOT.BIN written: $UBOOT_DIR/UBOOT.BIN"

echo ""
success "Done. Outputs in $UBOOT_DIR:"
ls -lh u-boot.bin u-boot UBOOT.BIN
echo ""
info "prado-firmware-reconstruction/build_bootable_sdcard.sh auto-detects this"
info "  tree (as a sibling dir, or ~/Downloads/linux-arkmicro) when --new-uboot is on."
