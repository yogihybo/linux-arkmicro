# Changelog

All commit messages have been translated into English for improved maintainability and international readability.

## [v1.0] - 2022-04-12
*   Initial commit (v1.0).

## [Previous Versions & Bug Fixes]

### **Bug Fixes & Stability**
*   Fix rare failure in normal recording and playback (`d81ebad64`).
*   Support setting GPU memory size and buffer frame count via Device Tree to improve UI refresh rate (`0306116f8`).
*   Fix mobile interconnect connection failure caused by previous USB driver modifications (`a460dc3ca`).
*   Fix screen flicker during display switching. Added reverse mirror function. Fixed issues related to rear display and trajectory lines (`1dbfdda57`).
*   Added support for 512MB NAND flash TC58NVG2S0HTAI0 and trimmed U-Boot size (`b76a1c8e6`).
*   Optimized 1668e v4l2 demo (`a7aff1fc1`).
*   Enhanced USB pin driver capability and fixed abnormal USB reset (`ef542e339`).
*   Optimize audio driver and fix sound stuttering issues (`38bb05259`).
*   Fix media playback freezing issues. Fixed random recording stutter and empty recordings on dev board. Changed EMMC root FS from ext2 to ext4, resolving space shrinking during data copy. Fixed rare issue of premature animation hiding or false detection of card upgrade when no SD card is inserted (`bf2b5edf8`).
*   Fixes related to USB handling: fixed kernel exception caused by frequent USB plugging/unplugging (`2ec5d9e19`), optimized USB driver for dual unplugging/reset (`ded26c573`), and resolved dual USB port usage issues (`5312ec0dc`).
*   Added notification interface to enable application-level USB recovery mechanism, resolving non-recognition errors upon USB connection/disconnection (`163cc8e40`).

### **Feature Additions & Enhancements**
*   Optimized audio driver and added I2S2 support (`44fa36f82`).
*   Added board type `ark1668e_devb_dashboard` for instrumentation (`92bfa59a9`).
*   Modify ADC driver to support simultaneous sampling on multiple channels (`d6c7cd6a2`).
*   Modified libmfc to support JPEG decoding up to 8192x8192, and retrieve actual resolution before H264 stream non-16 alignment (`fca8941da`).
*   Added missing compilation files for ark1668eapp (`1ced649af`).
*   Added `ark1668eapp` sample demo, adding NTFS and exFAT file system support (`e4e4c97c6`).

### **System & Bootloader (U-Boot/Kernel)**
*   Fixed kernel exception issues potentially caused by USB in some platforms (`40d1d4e50`).
*   Fixes related to U-Boot boot sequence: `bootusb` now mounts root from USB, not SD card (`125d497e1`), and the autoboot order was reordered for better stability (`0026096fa`). Added necessary board ports and logic refactors for USB/MMC handling (`ee0895568`, `ad658cea7`, `ed10b730d`).
*   Fixed CRC checks missing during EMMC OTA upgrades and kernel updates (`a931daba6`) and added upgrade documentation (`ff129a5b6`).

### **Media & Interconnect**
*   Phone Interconnect: Added hicar library, fixed reconnection issues, added C call interface, initialization notification, networking modes, and fixed CarPlay/Android Auto connection errors (`e8755f50f`, `554bbea57`).
*   Updated mplayer to support soft decoding (`cc3600c0f`) and pipe support (`152e6be25`).

### **Networking & Hardware Configuration**
*   Modify clock frequency and DDR parameters to balance module performance load (`8e8668444`).
*   Fixed bug in `add-board.sh` script and added 2-stage division for LCD clock (`85529e666`).
*   Modify SPI1 pin and clock configuration, and modify high-speed serial DMA configuration (`e7c454a7c`).

### **Recent Updates (July 2026)**
*   Remove trailing white space (`c8e16a348`).
*   Add log output and reset additional NAND/BCH registers before warm handoff (`55b3d5d36`).
*   DTS: Override LCD pinctrl group with custom group `lcd_prado` to free I2C pins (`1600c9321`).
*   Zero `rBCH_CR` and `rNAND_CR` before stock U-Boot chainload jump to fix warm handoff stale NAND ECC state (`262030bc1`).
*   Kernel: Don't let the USB recovery watchdog disconnect an already-working port (`07db9a9c3`).

---
***Note:** This document is a translation log. It does not rewrite your Git history.*