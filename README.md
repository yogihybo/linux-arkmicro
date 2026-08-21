# linux-arkmicro — ARK1668/ARK1680 vendor SDK

ArkMicro's vendor Linux/U-Boot/Buildroot SDK (`RD_Software/linux-arkmicro`),
used by [`prado-firmware-reconstruction`](https://github.com/yogihybo/prado-firmware-reconstruction)
to build a Linux 4.19.192 kernel and U-Boot 2018.07 for the Toyota Prado
Limcet P305/P306 dashboard head unit — plain **ARK1668** SoC, Cortex-A5,
board target `ark1668_limcet_p305` (not "ARK1668E", a different,
Cortex-A7 SoC sub-variant used by unrelated boards like
`ark1668e_devb`; the two names get mixed up elsewhere in this
project's docs). See `docs/KERNEL_REFERENCE.md` and
`docs/UBOOT_BUILD_GUIDE.md` in `prado-firmware-reconstruction` for
deeper reverse-engineering reference material.

## Overview

This repo builds the shared kernel and bootloader for the board, plus
one rootfs image: a modern, dynamically-linked, general-purpose
Buildroot rootfs for the device. `custom_ui`/`androidauto-sidecar` are
its current primary workload, not its whole scope — it's not built
narrowly around them. All three artifacts boot via the same U-Boot
commands (see "Filesystem images" below).

Three real artifacts come out of this repo and the scripts around it:

- **Kernel** (`zImage.w_dtb`) — Linux 4.19.192.
- **U-Boot** (`UBOOT.BIN`) — the bootloader.
- **Modern rootfs** (config name `ark1668_ft_custom_ui_defconfig`, dynamically linked, glibc 2.25) — deployed via `prado-firmware-reconstruction/build_bootable_sdcard_custom_ui.sh`. Named after its first real workload; the rootfs itself is general-purpose.

A separate "stock" rootfs also boots on this board, but it's built and
owned entirely by the sibling `prado-firmware-reconstruction` repo
(`build_bootable_sdcard.sh`/`firmware_overlay/`) — not by anything
here. It's mentioned in "Filesystem images" only as context for why
the modern rootfs exists.

## Quick start — automated build scripts

`build_uboot.sh` and `build_kernel.sh` (this repo's root) wrap all the
manual steps below into one command each, including U-Boot's mandatory
ARK-header patch routine:

```bash
./build_uboot.sh    # -> u-boot/UBOOT.BIN (ARK header already injected)
./build_kernel.sh   # -> zImage.w_dtb, compiled_modules/
```

Both default to an incremental build (skip `defconfig` if `.config`
already exists, use `nproc` jobs). `--clean` forces `make mrproper` first,
`--defconfig` forces reapplying the board defconfig even with an existing
`.config`, `-j N` overrides the job count. `--help` on either for details.

## Source tree layout

```
linux-arkmicro/
├── linux/                      ← kernel source (4.19.192)
├── u-boot/                     ← bootloader source (2018.07)
├── buildroot/, buildroot-2021.02.2/, buildroot-external/
│   └── toolchain/              ← cross-compiler lives here
├── compiled_modules/           ← kernel module install output (INSTALL_MOD_PATH)
├── zImage.w_dtb                ← final boot image (zImage + DTB appended)
└── env.source                  ← sets up CROSS_COMPILE/ARCH/PATH, see below
```

## Toolchain

Linaro GCC `arm-linux-gnueabihf` 7.3.1 (glibc 2.25) or 7.4.1, at
`buildroot-external/toolchain/gcc-linaro-*/bin/`. Source the environment
script before any build command — it picks whichever of the two toolchain
versions is actually present and puts it on `$PATH`:

```bash
source env.source
```

Host packages needed:

```bash
sudo apt install gcc make bc bison flex libssl-dev libelf-dev \
  python3 rsync u-boot-tools device-tree-compiler lzop \
  gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf
```

(`libssl-dev` deprecation warnings about `ENGINE_ctrl_cmd` from
`scripts/extract-cert` on OpenSSL 3.x are warnings only, not build
failures.)

## Kernel

Board target is `ark1668_defconfig` (generic — board specificity comes from
the `ark1668_limcet_p305.dtb` device tree, not a separate kernel defconfig).

```bash
source env.source
cd linux

# Full clean build
make ark1668_defconfig
make olddefconfig
make -j$(nproc) zImage
make -j$(nproc) modules
make modules_install INSTALL_MOD_PATH=../compiled_modules

# Incremental rebuild (after config/source changes)
make -j$(nproc) zImage modules && make modules_install INSTALL_MOD_PATH=../compiled_modules

# Device tree blobs only
make dtbs

# Boot image: the ARK1668 bootloader expects the DTB appended directly to
# the end of the compressed zImage
cat arch/arm/boot/zImage arch/arm/boot/dts/ark1668_limcet_p305.dtb > ../zImage.w_dtb
```

`prado-firmware-reconstruction/build_bootable_sdcard.sh` auto-detects
`arch/arm/boot/zImage`/`zImage.w_dtb` and `compiled_modules/` when this
repo is a sibling directory (or at `~/Downloads/linux-arkmicro`) — no
manual copying needed after a build.

### `.config` vs `ark1668_defconfig`

`linux/.config` is gitignored and is never committed —
`arch/arm/configs/ark1668_defconfig` is the *only* version-controlled
source of truth for kernel config. `build_kernel.sh` (and
`make ark1668_defconfig` above) only applies that defconfig when
`.config` doesn't already exist; it's skipped on every subsequent
incremental build.

This means an interactive config change (`make menuconfig`,
`./scripts/config --enable X`) that isn't also captured back into
`ark1668_defconfig` will silently disappear the next time the
defconfig gets re-applied (fresh checkout, `build_kernel.sh
--defconfig`, a new dev machine, CI) — no warning, no diff, no log.

**Workflow for any kernel config change:**

1. Test against the live `.config` first —
   `./scripts/config --enable/--module/--disable SYMBOL`, or
   `make menuconfig`, then `make ARCH=arm olddefconfig` to resolve
   dependent options.
2. Confirm it actually works, ideally on real hardware — a config
   change can compile clean and still be functionally wrong.
3. Capture it into `arch/arm/configs/ark1668_defconfig` immediately.
   For small changes, add the specific `CONFIG_X=y`/`=m` line near
   related existing entries, with a comment explaining why (see the
   RTC/WiFi/MUSB blocks in that file for the expected format). Don't
   run `make savedefconfig` over the whole file — it reorders
   everything and drops hand-written comments.
4. Validate the merge captured everything:
   ```bash
   cp .config /tmp/known_good_config.txt
   make ARCH=arm ark1668_defconfig
   diff /tmp/known_good_config.txt .config
   ```
   Should show only `CONFIG_LOCALVERSION`/timestamp/`CONFIG_GCC_VERSION`
   metadata differences — any real config line in the diff means the
   defconfig edit is incomplete.
5. Before force-reapplying defconfig on a `.config` you haven't
   personally just edited, back it up first (`cp .config
   /tmp/config_backup.txt`) in case it's carrying undocumented local
   changes.

## U-Boot

Board target is `ark1668_limcet_p305_defconfig` (no-SPL flat binary,
`CONFIG_SYS_TEXT_BASE=0x30000`).

```bash
source env.source
cd u-boot

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mrproper
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- ark1668_limcet_p305_defconfig

# Sanity-check the config landed correctly:
grep -E "^CONFIG_SPL\b|CONFIG_SKIP|CONFIG_SYS_TEXT_BASE" .config
#   # CONFIG_SPL is not set
#   CONFIG_SYS_TEXT_BASE=0x30000

make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j$(nproc)
```

Outputs: `u-boot` (ELF, needed by the header-injection script below for
entry point + BSS addresses), `u-boot.bin` (flat binary, ~370KB, no ARK
header yet), `u-boot.map`.

**`u-boot.bin` is not directly bootable on this hardware** — the
board's Stepldr expects an ARK-format header (magic, entry point, load
address, checksum) at fixed offsets that a stock U-Boot build doesn't
produce. `inject_ark_header.py` (this repo's root — canonical copy is
`prado-firmware-reconstruction/build_tools/inject_ark_header.py`, keep
both in sync) adds it, run from the `u-boot/` build directory:

```bash
python3 ../inject_ark_header.py u-boot.bin UBOOT.BIN
```

`build_bootable_sdcard.sh --new-uboot` does this automatically as part
of populating an SD card image. Manual SD card population (header
injection + copying `UBOOT.BIN`/DTB/`uEnv.txt` to the boot partition)
is documented in `docs/UBOOT_BUILD_GUIDE.md` "Step 7".

## U-Boot boot commands

`board/arkmicro/ark1668_limcet_p305/ark1668_boot_cmds.c` implements
these boot paths, each runnable by hand at the serial console:

| Command | Kernel/DTB from | Rootfs from | Notes |
|---|---|---|---|
| `bootnand` | NAND | NAND | 100% stock, untouched by this repo |
| `bootusb` | USB p1 | USB p2 | default autoboot's first attempt |
| `bootmmc` | SD p1 | SD p2 | not tried automatically — see below |
| `bootstock` / `boothybrid` | — | — | chainloads the real stock U-Boot binary; manual only |
| `bootcheck` | — | — | NAND-persisted boot-count fallback logic |

**Default autoboot only tries `bootusb`, then falls back to
`bootnand`** — it does not try `bootmmc` automatically. To boot an
SD-card image, interrupt autoboot at the serial console (`Hit space
to stop autoboot:`, in the early boot banner) and run `bootmmc`
manually, or write the same image to a USB drive instead so the
default chain picks it up.

## Filesystem images

This repo builds one rootfs: a modern, dynamically-linked,
general-purpose Buildroot rootfs for the device, booting on top of the
shared kernel/U-Boot output above (`zImage.w_dtb`, `compiled_modules/`).
`custom_ui`/`androidauto-sidecar` are its current primary workload —
the config/overlay/script names below carry that name because that's
what the rootfs was built to run first, not because the rootfs itself
is scoped to just those two binaries. Nothing about it precludes
running other things.

**Context (not built here)**: a separate "stock" rootfs — the patched
original vendor rootfs, running `MsnCoreApp`/`sink`/`blueware` alongside
`custom_ui`/`androidauto-sidecar` — also boots on this board, via the
same `bootusb`/`bootmmc` commands. It's built and owned entirely by
the sibling `prado-firmware-reconstruction` repo
(`build_bootable_sdcard.sh`/`firmware_overlay/`), not by anything in
this repo. It's relevant here only as the reason this repo's own
rootfs exists: the stock rootfs ships glibc 2.27, older than this
repo's own cross-compiler targets, so `custom_ui`/`androidauto-sidecar`
had to be statically linked to run on it — and on this device (173MB
RAM, no swap), static linking left them with no library pages shared
with the rest of the system, the root cause of a real `kswapd0`
page-thrashing crash (`ECONNRESET` killing live Android Auto
sessions). The fix is dynamic linking, which needs a rootfs whose own
glibc matches the build toolchain (2.25) — hence this repo's own,
general-purpose Buildroot rootfs.

| | |
|---|---|
| Config | `buildroot-external/configs/ark1668_ft_custom_ui_defconfig` (forked from `ark1668_ft_defconfig`) |
| Overlay | `prado-firmware-reconstruction/firmware_overlay_custom_ui/` |
| glibc | 2.25 (matches this repo's own toolchain) |
| Binaries | Base rootfs is dynamically linked throughout. `custom_ui`/`androidauto-sidecar` (its current workload) are dynamic ELF; BlueZ/`rtk_hciattach`/select `tools/*` stay static, carried in via the overlay rather than rebuilt as Buildroot packages — see each tool's own `tools/*/README.md` |
| Build | `cd buildroot && make BR2_EXTERNAL=../buildroot-external` |
| Deploy | `sudo prado-firmware-reconstruction/build_bootable_sdcard_custom_ui.sh --non-interactive` (thin wrapper around the stock repo's own `build_bootable_sdcard.sh`, which it never modifies), then `dd` the resulting image to an SD card or USB drive |
| Boot | `bootusb` (default) or `bootmmc` |

Reference docs: `custom_ui/docs/BLUEZ_MIGRATION_AND_BLUEWARE_DEPRECATION_HANDOFF.md`
for the Bluetooth/BlueZ stack specifically; `~/.claude/plans/merry-snacking-wirth.md`
(this machine only) for the full build/design history behind this rootfs.

## Board-code logging convention

Everything U-Boot prints from `board/arkmicro/ark1668_limcet_p305/*.c`
(i.e. everything after the stock startup banner —
`U-Boot 2018.07-linux4ark_1.0 (...)` / `DRAM:` / `NAND:` / `MMC:` / `In:`
/ `Out:` / `Err:` / `Hit space to stop autoboot:`) is tagged
`[item-name] message`, one tag per subsystem or command (`[arkdata.ini]`,
`[bootnand]`, `[bootmmc]`, `[bootusb]`, `[bootstock]`, `[gpiotest]`,
`[jpeghw]`, `[nandoobcheck]`, `[bootlogo]`, `[regr]`/`[regw]`/`[pmem]`,
etc.) so a boot log can be grepped down to one subsystem's output.
