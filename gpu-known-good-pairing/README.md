# GPU driver: known-good matched pairing (6.2.4.p1.8)

This folder preserves the one GPU driver combination that has actually gotten
furthest on real hardware, so it doesn't get lost/overwritten again by later
struct-RE work in `../gpu-vivante-6.2.4/`.

## What's here

- `galcore.ko` — kernel module, built `2026-07-16` (from `backup_working_no_fbcon/`),
  pre-dates all of this session's `gcsHAL_INTERFACE` struct/enum reconstruction work.
- `libGAL.so` — matching userspace driver, recovered from git history
  (`prado-firmware-reconstruction` commit `fd8cb33`, the state right before
  `bcf8bba` restored stock's original `libGAL.so`).
- `nxp-source-6.2.4.p1.8/` — pristine upstream source both binaries were built
  from: `Freescale/kernel-module-imx-gpu-viv.git`, tag `upstream/6.2.4.p1.8`
  (commit `1477635709772965b302e97aedee8c2b50a955d5`), confirmed by exact
  version-string match (`gcvVERSION_STRING "6.2.4.p1.150331"` in both the
  binaries' `strings` output and this tag's `gc_hal_version.h`). Plain files,
  no `.git` — re-clone the tag if history is ever needed, same reasoning as
  the `gpu-vivante-6.2.4/` import earlier this session (avoids nested-repo
  confusion).

Both `galcore.ko` and `libGAL.so` are a self-consistent pair (same version,
mismatched against stock's real driver, but matched to *each other*) — do not
mix either file with the struct-RE'd `gpu-vivante-6.2.4/` build or with
stock's original `libGAL.so`.

## Hardware test status

- **MsnCoreApp**: this pairing avoids the `ENOTTY`/struct-mismatch crash that
  stock's original `libGAL.so` hit against our from-source `galcore.ko`.
- **EffectWatch**: reaches real `/dev/galcore` ioctls (confirmed via `strace`,
  successful `IOCTL_GCHAL_INTERFACE` calls) — furthest any pairing has gotten.
  Hits the submenu black-screen bug instead of crashing. See
  `project_effectwatch_black_screen.md` (memory) — leading theory is that
  `EffectWatch`'s crossfade `IDirectFBSurface::StretchBlit` (GPU-accelerated,
  goes through this exact galcore/libGAL pair) hangs or silently no-ops under
  the ABI mismatch, leaving the pre-blit `Clear(black)` as the last frame ever
  drawn — i.e. permanently stuck black, not just a brief flash.
- **NOT yet tested**: whether this pairing also avoids the newer
  `libdirectfb_fbdev.so` `system_initialize()` SIGSEGV found later in the
  session (crash happens before any galcore ioctl, so it's suspected
  unrelated to which GAL pairing is loaded — but unconfirmed).

## Why this matters

The struct-RE'd `gpu-vivante-6.2.4/` + stock's original `libGAL.so` (today's
default state) is byte-exact-correct on paper, but has never been verified to
actually run `EffectWatch`'s GPU-accelerated compose path — it's been blocked
by the separate `fbdev.so` crash every time. This pairing is the fallback to
restore for testing anything GPU/EffectWatch-related when that's blocking:

```
cp gpu-known-good-pairing/galcore.ko compiled_modules/lib/modules/4.19.192/galcore.ko
cp gpu-known-good-pairing/libGAL.so /media/sf_GitHub/prado-firmware-reconstruction/firmware_source/prado_reconstructed/mtd6_rootfs/rootfs/usr/lib/libGAL.so
cp gpu-known-good-pairing/libGAL.so /media/sf_GitHub/prado-firmware-reconstruction/firmware_overlay/prado/usr/lib/libGAL.so
```

(This is exactly what's staged as of 2026-07-20, uncommitted, in the main repo.)
