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

### `.config` vs `ark1668_defconfig` — a recurring trap, read before changing kernel config

`linux/.config` is gitignored and has **never** been committed, ever
(`git log --all -- linux/.config` is empty) — `arch/arm/configs/
ark1668_defconfig` is the *only* version-controlled source of truth
for kernel config. But `build_kernel.sh` (and the `make ark1668_defconfig`
step above) only actually **applies** that defconfig when `.config`
doesn't already exist yet — it's skipped on every subsequent
incremental build, by design, for speed.

That combination is a trap: if you change config interactively
(`make menuconfig`, `./scripts/config --enable X`) and it works, `.config`
now has your change — but `ark1668_defconfig` doesn't. Nothing warns
you. Every build keeps working, silently relying on a `.config` that
no longer matches the checked-in defconfig, for as long as that exact
`.config` file happens to survive on that exact machine. The moment
someone re-applies the defconfig from a clean state (a fresh checkout,
`build_kernel.sh --defconfig`, a new dev machine, CI) your change is
gone, with no diff, no error, no log — the kernel just quietly builds
without whatever that setting did, and you find out only when
something that used to work mysteriously breaks.

**This has happened for real, multiple times**, each one costing a
full debugging cycle to figure out what silently vanished:

- `CONFIG_RTC_CLASS`/`CONFIG_RTC_DRV_ARK` — properly fixed and
  committed early (`2ec1c5855`), so a good example of doing this right.
- `CONFIG_INET`/`CONFIG_IPV6`/`CONFIG_WIRELESS`/`CONFIG_WLAN` + all 4
  RTL8xxx WiFi driver configs — silently missing for who knows how
  long, only discovered 2026-07-19 when enabling `ARK1668_ITU656`
  required a defconfig re-apply for the first time in a long time and
  it took DHCP/hostapd down with it. Recovered in `5f9fde926` by
  cross-referencing stale `.ko` build artifacts still sitting in the
  tree (proof of what modules a past, lost `.config` had actually
  built) plus boot-log evidence of what was confirmed working.
- `CONFIG_USB_MUSB_HDRC`/`CONFIG_USB_MUSB_ARKMICRO` — same defconfig
  re-apply also silently reverted these from a live-patched `=y` back
  to the checked-in (wrong, since the very first commit of this file)
  `=m`, which hung the `bootusb` boot path forever at "Waiting for
  root device" (no initramfs, so a module can never load before root
  needs mounting). Found by comparing early-boot dmesg timing across
  logs — a built-in driver's init lines print synchronously before
  root-mount; a module's can't. Fixed in `db0d63877`.

**The correct workflow, every time you change kernel config:**

1. Test the change against the live `.config` first —
   `./scripts/config --enable/--module/--disable SYMBOL`, or
   `make menuconfig`, then `make ARCH=arm olddefconfig` to resolve
   dependent options.
2. Build and get it actually confirmed working (ideally on real
   hardware, not just "compiles clean" — a config change can compile
   fine and still be functionally wrong, same as any other bug).
3. **Capture it into `arch/arm/configs/ark1668_defconfig` itself,
   immediately, before moving on to anything else.** For 1-4 line
   changes, add the specific `CONFIG_X=y`/`=m` line directly near
   related existing entries, with a comment explaining *why* (see the
   RTC/WiFi/MUSB blocks in that file for the expected format — future
   readers need to know why a setting is there, not just that it is).
   Do **not** blindly copy `make savedefconfig`'s output over the
   whole file — it reorders everything and silently drops every
   hand-written explanatory comment already there (including this
   note's own examples), producing a technically-equivalent but much
   less maintainable file.
4. **Validate the merge actually captured everything**, every time:
   ```bash
   cp .config /tmp/known_good_config.txt
   make ARCH=arm ark1668_defconfig
   diff /tmp/known_good_config.txt .config
   ```
   Should show *only* `CONFIG_LOCALVERSION`/build-timestamp/
   `CONFIG_GCC_VERSION`-style metadata differences. Any real config
   line in the diff means the defconfig edit didn't fully capture the
   live `.config` state — go back to step 3. This is the exact
   technique used to validate both fixes above.
5. Before ever force-reapplying defconfig on a `.config` you haven't
   just personally hand-edited (i.e. one that might be carrying
   someone else's — or your own past-session's — undocumented live
   patches), back it up first: `cp .config /tmp/config_backup.txt`.
   A diff against that backup is a five-second check; reconstructing
   lost settings after the fact from stale `.ko` timestamps and boot
   log timing analysis (as both fixes above had to) is not.

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
