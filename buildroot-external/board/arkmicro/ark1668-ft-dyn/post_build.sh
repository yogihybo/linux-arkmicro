#!/bin/bash
# Forked from board/arkmicro/ark1668-ft/post_build.sh (untouched) for the
# ark1668_ft_dyn_defconfig -- see merry-snacking-wirth.md's
# 2026-08-21 "target-finalize" fix note for the two real bugs in the
# original this works around, rather than editing a script shared with
# every other board defconfig in this tree:
#
#   1. It `cd`s into "${BR2_EXTERNAL_ARK_PATH}/../output/board/ark1668-ft/linux"
#      and runs `make modules_install` there -- that path has never
#      existed in this project's real kernel build flow (build_kernel.sh
#      builds directly in linux-arkmicro/linux/, and its real output is
#      linux-arkmicro/compiled_modules/lib/modules/<kver>/, already a
#      complete, depmod-processed module tree: modules.dep/.alias/.softdep
#      etc. all present, confirmed by direct inspection). Nobody has ever
#      completed a full Buildroot rootfs build for this SoC family in
#      this tree (see plan Context), so this bug was never exercised.
#   2. It references ${TARGET_DIR} as a bare env var, but Buildroot's own
#      Makefile passes the target dir as $1 (positional), not an env var
#      (EXTRA_ENV is empty in this tree) -- would have been unbound/empty
#      even if bug #1 didn't fail first.
#
# Real fix: just copy the already-built, already-depmod'd module tree
# directly -- no kernel `make modules_install`/depmod invocation needed
# at all, consistent with this whole rootfs's "same shared kernel, no
# second kernel build" architecture.
set -e
TARGET_DIR="$1"
source "${BR2_EXTERNAL_ARK_PATH}/../env.source"
KVER="$(ls "${BR2_EXTERNAL_ARK_PATH}/../compiled_modules/lib/modules")"
mkdir -p "${TARGET_DIR}/lib/modules"
cp -a "${BR2_EXTERNAL_ARK_PATH}/../compiled_modules/lib/modules/${KVER}" "${TARGET_DIR}/lib/modules/"
