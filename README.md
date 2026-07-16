# linux-arkmicro — ARK1668/ARK1680 vendor SDK

ArkMicro's vendor Linux/U-Boot/Buildroot SDK (`RD_Software/linux-arkmicro`),
used by [`prado-firmware-reconstruction`](https://github.com/yogihybo/prado-firmware-reconstruction)
to build a Linux 4.19.192 kernel and U-Boot 2018.07 for the Toyota Prado
Limcet P305/P306 dashboard head unit (ARK1668E SoC, board target
`ark1668_limcet_p305`). This README only covers building the kernel and
U-Boot for that specific board — see that project's `docs/KERNEL_REFERENCE.md`
and `docs/UBOOT_BUILD_GUIDE.md` for the full reverse-engineering context,
board-specific config deltas, and known gotchas.

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

Linaro GCC `arm-linux-gnueabihf` 7.3.1 or 7.4.1, at
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

**The raw `u-boot.bin` isn't directly bootable on this hardware** — the
board's Stepldr expects an ARK-format header (magic, entry point, load
address, checksum) at fixed offsets that a stock U-Boot build doesn't
produce. `inject_ark_header.py` (at this repo's root — canonical copy is
`prado-firmware-reconstruction/build_tools/inject_ark_header.py`, keep
both in sync) adds it, run from the `u-boot/` build directory:

```bash
python3 ../inject_ark_header.py u-boot.bin UBOOT.BIN
```

`build_bootable_sdcard.sh --new-uboot` does this automatically as part of
populating an SD card image — see that project's README for the full SD
boot workflow. Manual SD card population (header injection + copying
`UBOOT.BIN`/DTB/`uEnv.txt` to the boot partition) is documented in
`docs/UBOOT_BUILD_GUIDE.md` "Step 7" if you need to do it by hand.

## Board-code logging convention

Everything U-Boot prints from `board/arkmicro/ark1668_limcet_p305/*.c`
(i.e. everything after the stock startup banner —
`U-Boot 2018.07-linux4ark_1.0 (...)` / `DRAM:` / `NAND:` / `MMC:` / `In:`
/ `Out:` / `Err:` / `Hit space to stop autoboot:`) is tagged
`[item-name] message`, one tag per subsystem or command (`[arkdata.ini]`,
`[bootnand]`, `[bootmmc]`, `[bootusb]`, `[bootstock]`, `[gpiotest]`,
`[jpeghw]`, `[nandoobcheck]`, `[bootlogo]`, `[regr]`/`[regw]`/`[pmem]`,
etc.) so a boot log can be grepped down to one subsystem's output.
