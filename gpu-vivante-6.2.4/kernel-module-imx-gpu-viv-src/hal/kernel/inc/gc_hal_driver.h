/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2014 - 2018 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2014 - 2018 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/


#ifndef __gc_hal_driver_h_
#define __gc_hal_driver_h_

#include "gc_hal_enum.h"
#include "gc_hal_types.h"

#if gcdENABLE_VG
#include "gc_hal_driver_vg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
******************************* I/O Control Codes ******************************
\******************************************************************************/

#define gcvHAL_CLASS                    "galcore"
#define IOCTL_GCHAL_INTERFACE           30000
#define IOCTL_GCHAL_KERNEL_INTERFACE    30001
#define IOCTL_GCHAL_TERMINATE           30002

#undef CONFIG_ANDROID_RESERVED_MEMORY_ACCOUNT
/******************************************************************************\
********************************* Command Codes ********************************
\******************************************************************************/

/*
 * Positions 0-63 below are pinned with explicit "= N" values to exactly
 * match stock's real, original 5.0.11.28018 galcore.ko, decompiled from
 * /lib/modules/3.4.0/galcore.ko's gckKERNEL_Dispatch jump table (64 entries,
 * indices 0-63, `cmp r3, #63` bound check). This is on-the-wire ABI with
 * libGAL.so -- getting any of these numbers wrong risks the same kind of
 * crash the ATTACH/DETACH miscount caused before (see
 * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b). Do not renumber by
 * inserting/removing entries without updating the explicit values.
 *
 * Confidence key (see 2026-07-20 session report for full evidence per
 * entry):
 *   [C]  confirmed -- direct decompiled function-call match, or exact
 *        struct-field-layout match, or (rare) a hardcoded constant found
 *        in stock's own compiled code.
 *   [P]  probable -- struct-field pattern match, current source's C
 *        implementation matches the decompiled behavior, but no direct
 *        function-name/constant proof.
 *   [U]  genuinely uncertain -- best available guess, flagged explicitly.
 *
 * Values 64+ are commands with NO stock equivalent at all (verified: no
 * matching entry anywhere in stock's 0-63 jump table) -- newer,
 * multi-GPU-core/DRM-era additions from the imx-gpu-viv 6.2.4 tree. Their
 * exact numbering amongst themselves doesn't matter (never sent over the
 * wire to stock hardware/HAL), but gc_hal_kernel_driver.c's ioctl entry
 * point must reject anything >= gcvHAL_COMMAND_CODE_COUNT since stock's
 * `cmp r3, #63` bound check has no equivalent in our code otherwise.
 */
typedef enum _gceHAL_COMMAND_CODES
{
    /* Generic query. */
    gcvHAL_QUERY_VIDEO_MEMORY = 0,                 /* [C] */
    gcvHAL_QUERY_CHIP_IDENTITY,                    /* [C] */

    /* Contiguous memory. */
    gcvHAL_ALLOCATE_NON_PAGED_MEMORY,               /* [C] */
    gcvHAL_FREE_NON_PAGED_MEMORY,                   /* [C] */
    gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY,              /* [C] */
    gcvHAL_FREE_CONTIGUOUS_MEMORY,                  /* [C] */

    /* Video memory allocation. */
    gcvHAL_ALLOCATE_VIDEO_MEMORY,           /* [C] Enforced alignment; deprecated in stock (always returns error). */
    gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY,    /* [C] No alignment. */
    gcvHAL_RELEASE_VIDEO_MEMORY,            /* [C] */

    /* Physical-to-logical mapping. */
    gcvHAL_MAP_MEMORY,                              /* [C] */
    gcvHAL_UNMAP_MEMORY,                            /* [C] */

    /* Logical-to-physical mapping. */
    gcvHAL_MAP_USER_MEMORY,                         /* [C] */
    gcvHAL_UNMAP_USER_MEMORY,                       /* [C] */

    /* Surface lock/unlock. */
    gcvHAL_LOCK_VIDEO_MEMORY,                       /* [C] */
    gcvHAL_UNLOCK_VIDEO_MEMORY,                     /* [C] */

    /* Event queue. */
    gcvHAL_EVENT_COMMIT,                            /* [C] */

    gcvHAL_USER_SIGNAL,                             /* [C] */

    /* [C] NOT a dispatch gap despite stock's jump table pointing at the
     * shared c4d0 fallback for index 17 -- userspace never sends this
     * command directly (hence no dispatch case), but stock's OWN compiled
     * gckEVENT_Compose (and at least 3 other internal helpers) hardcode
     * the literal 17 when synthesizing an internal event record's
     * info.command field (e.g. `mov sl, #17` / `str sl, [r6, #8]` at
     * decompiled address 0x132a0, matching gcsEVENT.info.command's
     * compiled offset). This directly contradicts an earlier assumption
     * in this project that index 17 was a genuine reserved gap -- it is
     * not; the enum value is real, just unreachable from the public
     * ioctl. Kept at its already-correct position 17. */
    gcvHAL_SIGNAL,

    /* [C] Genuine gap -- stock's jump table entry 18 is the shared c4d0
     * fallback, AND (unlike index 17) no hardcoded use of the literal 18
     * was found anywhere in stock's disassembly as an internal event
     * command value either. gcvHAL_WRITE_DATA has no confirmed stock
     * equivalent at any position; moved to the 64+ pool below. */
    gcvHAL_RESERVED_18, /* reserved, unhandled in stock */

    gcvHAL_COMMIT,                                  /* [C] */
    gcvHAL_STALL,                                   /* [C] */

    gcvHAL_READ_REGISTER,                           /* [C] struct-field + mutex-guarded gckOS_ReadRegisterEx call match */
    gcvHAL_WRITE_REGISTER,                          /* [C] struct-field + mutex-guarded gckOS_WriteRegisterEx call match */

    gcvHAL_GET_PROFILE_SETTING,                     /* [C] exact match: reads Kernel->profileEnable into Interface.GetProfileSetting.enable, identical to current gc_hal_kernel.c case body */
    gcvHAL_SET_PROFILE_SETTING,                     /* [C] exact match: writes Interface.SetProfileSetting.enable into Kernel->profileEnable behind the same capability guard as current gc_hal_kernel.c case body */

    /* [U] Genuinely uncertain. Decompiled index 25 calls
     * gckHARDWARE_QueryContextProfile (looks up a context by name from
     * Interface+32, writes an OUT value to Interface+36) -- this does not
     * cleanly match this tree's _gcsHAL_PROFILE_REGISTERS_2D struct
     * (single OUT UINT64 field, no IN context-name field). No switch case
     * for gcvHAL_PROFILE_REGISTERS_2D exists in gc_hal_kernel.c at all
     * (falls to default/NOT_SUPPORTED), so a wrong guess here is lower
     * risk than ATTACH/DETACH was, but it IS still a guess. Kept at its
     * existing position by elimination/ordering continuity only. */
    gcvHAL_PROFILE_REGISTERS_2D,
    /* gcvHAL_READ_ALL_PROFILE_REGISTERS_PART1/PART2 removed here (not just
     * their union fields, see below) -- these VIVANTE_PROFILER/multi-core-
     * era commands don't exist in stock's real enum at all. See
     * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b. */
    gcvHAL_READ_PROFILER_REGISTER_SETTING,          /* [P] decompiled index 26 calls gckHARDWARE_ProfileEngine2D with a single input value from Interface+32 and no OUT field -- matches this struct's single IN gctBOOL bclear shape, though stock routes it through a different (HW-call) mechanism than this tree's direct Kernel->profileCleanRegister field write */

    /* Power management. */
    gcvHAL_SET_POWER_MANAGEMENT_STATE,              /* [C] */
    gcvHAL_QUERY_POWER_MANAGEMENT_STATE,            /* [C] */

    gcvHAL_GET_BASE_ADDRESS,                        /* [C] */

    gcvHAL_SET_IDLE, /* [C] reserved -- genuine gap, matches stock's c4d0 fallback */

    /* Queries. */
    gcvHAL_QUERY_KERNEL_SETTINGS,                   /* [C] */

    /* Reset. */
    gcvHAL_RESET,                                   /* [C] */

    /* [C] Genuine gap -- stock's jump table entry 33 is the shared c4d0
     * fallback. gcvHAL_MAP_PHYSICAL (previously assigned here) has no
     * confirmed stock implementation at any position; moved to the 64+
     * pool below. */
    gcvHAL_RESERVED_33, /* reserved, unhandled in stock */

    /* Debugger stuff. */
    gcvHAL_DEBUG,                                   /* [C] */

    /* Cache stuff. */
    gcvHAL_CACHE,                                   /* [C] */

    /* TimeStamp */
    gcvHAL_TIMESTAMP,                               /* [C] exact struct-field match: timer@+32 (index<=7 select), request@+36 (==2 selects delta calc), timeDelta OUT@+40 -- matches _gcsHAL_TIMESTAMP exactly */

    /* Database. */
    gcvHAL_DATABASE,                                /* [C] */

    /* Version. */
    gcvHAL_VERSION,                                 /* [C] very high confidence: decompiled code hardcodes major=5,minor=0,patch=11,build=28018 into the exact _gcsHAL_VERSION field offsets -- matches the known stock driver version 5.0.11.28018 exactly */

    /* Chip info */
    gcvHAL_CHIP_INFO,                               /* [C] count=1 + types[0] field pattern match */

    /* Process attaching/detaching. */
    gcvHAL_ATTACH,                                  /* [C] load-bearing -- this exact number previously caused a real hardware crash when wrong */
    gcvHAL_DETACH,                                  /* [C] */

    /* [C] NEW -- gcvHAL_COMPOSE does not exist anywhere in this source
     * tree (neither the enum value nor the underlying gckEVENT_Compose /
     * gckHARDWARE_Compose kernel functions), but stock's real dispatch
     * table has a distinct, non-gap handler at this exact position that
     * calls gckEVENT_Compose. See the struct comment on ComposeInfo below
     * and the union member note for what is/isn't implemented. */
    gcvHAL_COMPOSE,

    /* Set timeOut value */
    gcvHAL_SET_TIMEOUT,                             /* [C] writes Interface+32 (timeOut) into *(Kernel+0xc0), matches _gcsHAL_SET_TIMEOUT.timeOut */

    /* Frame database. */
    gcvHAL_GET_FRAME_INFO,                          /* [C] calls gckHARDWARE_GetFrameInfo */

    /* [C] Genuine gap -- stock's jump table entry 45 is the shared c4d0
     * fallback. gcvHAL_DUMP_GPU_PROFILE (previously assigned here) has no
     * confirmed stock implementation at any position; moved to the 64+
     * pool below. */
    gcvHAL_RESERVED_45, /* reserved, unhandled in stock */

    /* [C] Genuine gap -- stock's jump table entry 46 is the shared c4d0
     * fallback. gcvHAL_COMMIT_DONE (previously assigned here) has no
     * confirmed stock implementation at any position; moved to the 64+
     * pool below. */
    gcvHAL_RESERVED_46, /* reserved, unhandled in stock */

    /* GPU and event dump */
    gcvHAL_DUMP_GPU_STATE,                          /* [C] calls _DumpDriverConfigure then _DumpState, exact function-name match */

    /* [U] Genuinely uncertain. Decompiled index 48 jumps straight to the
     * dispatcher's generic "return success" trampoline with NO function
     * call at all -- a true no-op. Kept at this position only because it
     * sits exactly between the confirmed DUMP_GPU_STATE (47) and
     * ALLOCATE_VIRTUAL_COMMAND_BUFFER (49), which is also where this
     * source's enum already places gcvHAL_DUMP_EVENT. Plausible (a
     * disabled-in-this-build debug feature trivially succeeding) but not
     * independently confirmed by any call or struct-field evidence. */
    gcvHAL_DUMP_EVENT,

    /* Virtual command buffer. */
    gcvHAL_ALLOCATE_VIRTUAL_COMMAND_BUFFER,         /* [C] calls gckKERNEL_AllocateVirtualCommandBuffer */
    gcvHAL_FREE_VIRTUAL_COMMAND_BUFFER,             /* [C] calls gckOS_DestroyUserVirtualMapping + gckKERNEL_DestroyVirtualCommandBuffer */

    /* FSCALE_VAL. */
    gcvHAL_SET_FSCALE_VALUE,                        /* [C] calls gckHARDWARE_SetFscaleValue */
    gcvHAL_GET_FSCALE_VALUE,                        /* [C] calls gckHARDWARE_GetFscaleValue */

    /* [C] MOVED from this source's previous position 54 -- decompiled
     * index 53 calls gckVIDMEM_NODE_Name (assign a name to a node),
     * unambiguous function-name match. */
    gcvHAL_NAME_VIDEO_MEMORY,
    /* [C] MOVED from this source's previous position 55 -- decompiled
     * index 54 calls gckVIDMEM_NODE_Import, unambiguous function-name
     * match. */
    gcvHAL_IMPORT_VIDEO_MEMORY,

    /* [U] MOVED from this source's previous position 56 -- genuinely
     * uncertain. Decompiled index 55 does a plain 8-byte field copy from
     * *(Kernel+56) into Interface+32, no function call. This tree's
     * current gcvHAL_QUERY_RESET_TIME_STAMP case populates TWO 64-bit
     * fields (timeStamp AND contextID from two separate Kernel fields),
     * but stock's decompiled code here only ever touches ONE 64-bit
     * value. Best guess by elimination (no other unclaimed candidate
     * fits a bare "return a cached timestamp" shape), but the mismatch
     * with this tree's two-field implementation is real and unresolved. */
    gcvHAL_QUERY_RESET_TIME_STAMP,

    /* [C] Genuine gaps -- stock's jump table entries 56-60 (5 consecutive
     * slots) are all the shared c4d0 fallback. gcvHAL_READ_REGISTER_EX,
     * gcvHAL_WRITE_REGISTER_EX, gcvHAL_CREATE_NATIVE_FENCE,
     * gcvHAL_WAIT_NATIVE_FENCE, and gcvHAL_DESTROY_MMU (previously
     * assigned across roughly this range) have no confirmed stock
     * implementation at any position; moved to the 64+ pool below. (The
     * non-EX gcvHAL_READ_REGISTER/WRITE_REGISTER at 21/22 already cover
     * the real register-access commands -- their *_Ex OS-layer helper
     * functions are just stock's internal implementation detail, not a
     * separate exposed HAL command, consistent with this single-core
     * SoC having no multi-core register-select need.) */
    gcvHAL_RESERVED_56, /* reserved, unhandled in stock */
    gcvHAL_RESERVED_57, /* reserved, unhandled in stock */
    gcvHAL_RESERVED_58, /* reserved, unhandled in stock */
    gcvHAL_RESERVED_59, /* reserved, unhandled in stock */
    gcvHAL_RESERVED_60, /* reserved, unhandled in stock */

    /* Shared buffer. */
    gcvHAL_SHBUF,                                   /* [C] 5-way sub-switch matches gckKERNEL_CreateShBuffer/DestroyShBuffer/MapShBuffer/WriteShBuffer/ReadShBuffer exactly, unchanged position */

    /* [C] MOVED from this source's previous position 67 -- decompiled
     * index 62 calls gckKERNEL_ConfigPowerManagement, the only call site
     * to that function anywhere in stock's module. */
    gcvHAL_CONFIG_POWER_MANAGEMENT,

    /* [C] MOVED from this source's previous position 53 -- decompiled
     * index 63 calls gckVIDMEM_NODE_GetFd, the only call site to that
     * function anywhere in stock's module. Stock has NO
     * gckVIDMEM_NODE_Export function at all (searched the full
     * disassembly), so this is the only plausible candidate for
     * "export video memory as an fd". gcvHAL_GET_GRAPHIC_BUFFER_FD
     * (previously at this source's position 63) has no confirmed stock
     * equivalent and moves to the 64+ pool below. */
    gcvHAL_EXPORT_VIDEO_MEMORY,

    /*
     * ------------------------------------------------------------------
     * Everything below this line (64+) has NO confirmed stock equivalent
     * anywhere in the 0-63 range above -- verified by full accounting of
     * all 64 stock jump-table slots (53 real handlers + 11 genuine c4d0
     * gaps = 64). These are newer, multi-GPU-core/DRM-era additions from
     * the imx-gpu-viv 6.2.4 source tree. Stock's real hardware/libGAL.so
     * will never send these values, and our own ioctl entry point must
     * reject anything >= gcvHAL_COMMAND_CODE_COUNT (see
     * gc_hal_kernel_driver.c) since stock's `cmp r3, #63` bound check has
     * no equivalent here otherwise. Ordering among these doesn't matter.
     * ------------------------------------------------------------------
     */
    gcvHAL_WRITE_DATA = 64,

    /* Map physical address into handle. */
    gcvHAL_MAP_PHYSICAL,

    /* GPU profile dump */
    gcvHAL_DUMP_GPU_PROFILE,

    gcvHAL_QUERY_COMMAND_BUFFER,

    gcvHAL_COMMIT_DONE,

    /* Multi-GPU read/write. */
    gcvHAL_READ_REGISTER_EX,
    gcvHAL_WRITE_REGISTER_EX,

    /* Create native fence and return its fd. */
    gcvHAL_CREATE_NATIVE_FENCE,

    /* Let GPU wait on native fence. */
    gcvHAL_WAIT_NATIVE_FENCE,

    /* Destory MMU. */
    gcvHAL_DESTROY_MMU,

    /*
     * Fd representation of android graphic buffer contents.
     * Currently, it is only to reference video nodes, signal, etc to avoid being
     * destroyed when trasfering across processes.
     */
    gcvHAL_GET_GRAPHIC_BUFFER_FD,

    gcvHAL_SET_VIDEO_MEMORY_METADATA,

    /* Connect a video node to an OS native fd. */
    gcvHAL_GET_VIDEO_MEMORY_FD,

    /* Wrap a user memory into a video memory node. */
    gcvHAL_WRAP_USER_MEMORY,

    /* Wait until GPU finishes access to a resource. */
    gcvHAL_WAIT_FENCE,

#if gcdDEC_ENABLE_AHB
    gcvHAL_DEC300_READ,
    gcvHAL_DEC300_WRITE,
    gcvHAL_DEC300_FLUSH,
    gcvHAL_DEC300_FLUSH_WAIT,
#endif

    gcvHAL_BOTTOM_HALF_UNLOCK_VIDEO_MEMORY,
    gcvHAL_QUERY_CHIP_OPTION,

    /* Sentinel -- always keep last. Used for ioctl command-value bound
     * checks (see gc_hal_kernel_driver.c) and to size _DispatchText[]
     * (see gc_hal_kernel.c). */
    gcvHAL_COMMAND_CODE_COUNT

}
gceHAL_COMMAND_CODES;

/******************************************************************************\
****************************** Interface Structure *****************************
\******************************************************************************/

#define gcdMAX_PROFILE_FILE_NAME    128

/* Kernel settings. */
typedef struct _gcsKERNEL_SETTINGS
{
    /* Used RealTime signal between kernel and user. */
    gctINT signal;
}
gcsKERNEL_SETTINGS;

typedef struct _gcsUSER_MEMORY_DESC
{
    /* Import flag. */
    gctUINT32                  flag;

    /* gcvALLOC_FLAG_DMABUF */
    gctUINT32                  handle;
    gctUINT64                  dmabuf;

    /* gcvALLOC_FLAG_USERMEMORY */
    gctUINT64                  logical;
    gctUINT32                  physical;
    gctUINT32                  size;

    /* gcvALLOC_FLAG_EXTERNAL_MEMORY */
    gcsEXTERNAL_MEMORY_INFO    externalMemoryInfo;
}
gcsUSER_MEMORY_DESC;


#define gcdMAX_FLAT_MAPPING_COUNT           16

typedef struct _gcsFLAT_MAPPING_RANGE
{
    gctUINT64 start;
    gctUINT64 end;
}
gcsFLAT_MAPPING_RANGE;

/* gcvHAL_QUERY_CHIP_IDENTITY */
typedef struct _gcsHAL_QUERY_CHIP_IDENTITY * gcsHAL_QUERY_CHIP_IDENTITY_PTR;
typedef struct _gcsHAL_QUERY_CHIP_IDENTITY
{

    /* Chip model. */
    gceCHIPMODEL                chipModel;

    /* Revision value.*/
    gctUINT32                   chipRevision;

    /* Chip date. */
    gctUINT32                   chipDate;

#if gcdENABLE_VG
    /* Supported feature fields. */
    gctUINT32                   chipFeatures;

    /* Supported minor feature fields. */
    gctUINT32                   chipMinorFeatures;

    /* Supported minor feature 1 fields. */
    gctUINT32                   chipMinorFeatures1;

    /* Supported minor feature 2 fields. */
    gctUINT32                   chipMinorFeatures2;

    /* Supported minor feature 3 fields. */
    gctUINT32                   chipMinorFeatures3;

    /* Supported minor feature 4 fields. */
    gctUINT32                   chipMinorFeatures4;

    /* Supported minor feature 5 fields. */
    gctUINT32                   chipMinorFeatures5;

    /* Supported minor feature 6 fields. */
    gctUINT32                   chipMinorFeatures6;
#endif

    /* Number of streams supported. */
    gctUINT32                   streamCount;

    /* Number of pixel pipes. */
    gctUINT32                   pixelPipes;

    /* Number of resolve pipes. */
    gctUINT32                   resolvePipes;

    /* Number of instructions. */
    gctUINT32                   instructionCount;

    /* Number of constants. */
    gctUINT32                   numConstants;

    /* Number of varyings */
    gctUINT32                   varyingsCount;

    /* Number of 3D GPUs */
    gctUINT32                   gpuCoreCount;

    /* Product ID */
    gctUINT32                   productID;

    /* Special chip flag bits */
    gceCHIP_FLAG                chipFlags;

    /* ECO ID. */
    gctUINT32                   ecoID;

    /* Customer ID. */
    gctUINT32                   customerID;
}
gcsHAL_QUERY_CHIP_IDENTITY;

typedef struct _gcsHAL_QUERY_CHIP_OPTIONS * gcsHAL_QUERY_CHIP_OPTIONS_PTR;
typedef struct _gcsHAL_QUERY_CHIP_OPTIONS
{
    gctBOOL     gpuProfiler;
    gctBOOL     allowFastClear;
    gctBOOL     powerManagement;
    /* Whether use new MMU. It is meaningless
    ** for old MMU since old MMU is always enabled.
    */
    gctBOOL     enableMMU;
    gceCOMPRESSION_OPTION     allowCompression;
    gctUINT     uscL1CacheRatio;
    gceSECURE_MODE    secureMode;

}
gcsHAL_QUERY_CHIP_OPTIONS;

typedef struct _gcsHAL_INTERFACE
{
    /* Command code. */
    gceHAL_COMMAND_CODES        command;

    /* Reserved -- stock's real galcore.ko reads status at byte offset 8
     * (confirmed via decompile: param_3[2] in gckKERNEL_Dispatch), meaning
     * a 4-byte field/gap exists here between command and status in stock's
     * real struct. Its actual identity is unknown (never read/written in
     * any decompiled code path examined), so this is a plain reserved
     * placeholder purely to preserve the byte offset. */
    gctUINT32                   _reserved0;

    /* Status value. */
    gceSTATUS                   status;

    /* ARK1668 is a single-core, single-engine SoC -- hardwareType/coreIndex/
     * handle/pid/engine/ignoreTLS (multi-GPU-core support added in later
     * Vivante driver generations) were removed here to restore this
     * struct's on-the-wire layout to match stock's real, original
     * 5.0.11.28018 driver (which this device's real hardware and userspace
     * libGAL.so were built against). See
     * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b.
     *
     * Cross-validated via decompile of stock's real galcore.ko against TWO
     * independent internal functions (gckKERNEL_LockVideoMemory's node
     * field at Interface+0x20, and the MAP_USER_MEMORY dispatch case's use
     * of Interface+0x20 for the "memory" field) that stock's union starts
     * at absolute byte offset 32 -- not immediately after status(offset 8)
     * as first assumed. 20 bytes of stock's real header content between
     * status and the union are still unidentified (never read/written in
     * any decompiled path examined so far); reserved as padding here to
     * preserve the byte offset until/unless identified. */
    gctUINT8                    _reserved1[20];

    /* Union of command structures. */
    union _u
    {
        /* gcvHAL_GET_BASE_ADDRESS */
        struct _gcsHAL_GET_BASE_ADDRESS
        {
            /* Physical memory address of internal memory. */
            OUT gctUINT32               baseAddress;

            /* flatMappingRangeCount/flatMappingRanges[16] (256 bytes) removed
             * here -- stock's real GET_BASE_ADDRESS handler
             * (gckOS_GetBaseAddress) only ever wrote a single baseAddress
             * value, no flat-mapping-range array; this field is unused by
             * this chip's actual driver logic (Kernel->mmu->flatMappingRangeCount
             * is 0 whenever flat mapping isn't in use). See
             * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b. */
        }
        GetBaseAddress;

        /* gcvHAL_QUERY_VIDEO_MEMORY */
        struct _gcsHAL_QUERY_VIDEO_MEMORY
        {
            /* Physical memory address of internal memory. Just a name. */
            OUT gctUINT32               internalPhysical;

            /* Size in bytes of internal memory. */
            OUT gctUINT64               internalSize;

            /* Physical memory address of external memory. Just a name. */
            OUT gctUINT32               externalPhysical;

            /* Size in bytes of external memory.*/
            OUT gctUINT64               externalSize;

            /* Physical memory address of contiguous memory. Just a name. */
            OUT gctUINT32               contiguousPhysical;

            /* Size in bytes of contiguous memory.*/
            OUT gctUINT64               contiguousSize;
        }
        QueryVideoMemory;

        /* gcvHAL_QUERY_CHIP_IDENTITY */
        gcsHAL_QUERY_CHIP_IDENTITY      QueryChipIdentity;

        /* gcvHAL_MAP_MEMORY */
        struct _gcsHAL_MAP_MEMORY
        {
            /* Physical memory address to map. Just a name on Linux/Qnx. */
            IN gctUINT32                physical;

            /* Number of bytes in physical memory to map. */
            IN gctUINT64                bytes;

            /* Address of mapped memory. */
            OUT gctUINT64               logical;
        }
        MapMemory;

        /* gcvHAL_UNMAP_MEMORY */
        struct _gcsHAL_UNMAP_MEMORY
        {
            /* Physical memory address to unmap. Just a name on Linux/Qnx. */
            IN gctUINT32                physical;

            /* Number of bytes in physical memory to unmap. */
            IN gctUINT64                bytes;

            /* Address of mapped memory to unmap. */
            IN gctUINT64                logical;
        }
        UnmapMemory;

        /* gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY */
        struct _gcsHAL_ALLOCATE_LINEAR_VIDEO_MEMORY
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT              bytes;

            /* Buffer alignment. */
            IN gctUINT                  alignment;

            /* Type of allocation. */
            IN gceSURF_TYPE             type;

            /* Flag of allocation. */
            IN gctUINT32                flag;

            /* Memory pool to allocate from. */
            IN OUT gcePOOL              pool;

            /* Allocated video memory. */
            OUT gctUINT32               node;
        }
        AllocateLinearVideoMemory;

        /* gcvHAL_ALLOCATE_VIDEO_MEMORY */
        struct _gcsHAL_ALLOCATE_VIDEO_MEMORY
        {
            /* Width of rectangle to allocate. */
            IN OUT gctUINT              width;

            /* Height of rectangle to allocate. */
            IN OUT gctUINT              height;

            /* Depth of rectangle to allocate. */
            IN gctUINT                  depth;

            /* Format rectangle to allocate in gceSURF_FORMAT. */
            IN gceSURF_FORMAT           format;

            /* Type of allocation. */
            IN gceSURF_TYPE             type;

            /* Memory pool to allocate from. */
            IN OUT gcePOOL              pool;

            /* Allocated video memory. */
            OUT gctUINT32               node;
        }
        AllocateVideoMemory;

        /* gcvHAL_RELEASE_VIDEO_MEMORY */
        struct _gcsHAL_RELEASE_VIDEO_MEMORY
        {
            /* Allocated video memory. */
            IN gctUINT32                node;

#ifdef __QNXNTO__
            /* Mapped logical address to unmap in user space. */
            OUT gctUINT64               memory;

            /* Number of bytes to allocated. */
            OUT gctUINT64               bytes;
#endif
        }
        ReleaseVideoMemory;

        /* gcvHAL_LOCK_VIDEO_MEMORY */
        struct _gcsHAL_LOCK_VIDEO_MEMORY
        {
            /* Allocated video memory. */
            IN gctUINT32                node;

            /* Cache configuration. */
            /* Only gcvPOOL_CONTIGUOUS and gcvPOOL_VIRUTAL
            ** can be configured */
            IN gctBOOL                  cacheable;

            /* Hardware specific address. */
            OUT gctUINT32               address;

            /* Mapped logical address. */
            OUT gctUINT64               memory;

            /* Customer priviate handle*/
            OUT gctUINT32               gid;

            /* Bus address of a contiguous video node. */
            OUT gctUINT64               physicalAddress;
        }
        LockVideoMemory;

        /* gcvHAL_UNLOCK_VIDEO_MEMORY */
        struct _gcsHAL_UNLOCK_VIDEO_MEMORY
        {
            /* Allocated video memory. */
            IN gctUINT64                node;

            /* Type of surface. */
            IN gceSURF_TYPE             type;

            /* Pool of the unlock node */
            OUT gcePOOL                 pool;

            /* Bytes of the unlock node */
            OUT gctUINT                 bytes;

            /* Flag to unlock surface asynchroneously. */
            IN OUT gctBOOL              asynchroneous;
        }
        UnlockVideoMemory;

        /* gcvHAL_ALLOCATE_NON_PAGED_MEMORY */
        struct _gcsHAL_ALLOCATE_NON_PAGED_MEMORY
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT64        bytes;

            /* Physical address of allocation. Just a name. */
            OUT gctUINT32           physical;

            /* Logical address of allocation. */
            OUT gctUINT64           logical;
        }
        AllocateNonPagedMemory;

        /* gcvHAL_FREE_NON_PAGED_MEMORY */
        struct _gcsHAL_FREE_NON_PAGED_MEMORY
        {
            /* Number of bytes allocated. */
            IN gctUINT64            bytes;

            /* Physical address of allocation. Just a name. */
            IN gctUINT32            physical;

            /* Logical address of allocation. */
            IN gctUINT64            logical;
        }
        FreeNonPagedMemory;

        /* gcvHAL_ALLOCATE_NON_PAGED_MEMORY */
        struct _gcsHAL_ALLOCATE_VIRTUAL_COMMAND_BUFFER
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT64        bytes;

            /* Physical address of allocation. Just a name. */
            OUT gctUINT32           physical;

            /* Logical address of allocation. */
            OUT gctUINT64           logical;
        }
        AllocateVirtualCommandBuffer;

        /* gcvHAL_FREE_NON_PAGED_MEMORY */
        struct _gcsHAL_FREE_VIRTUAL_COMMAND_BUFFER
        {
            /* Number of bytes allocated. */
            IN gctUINT64            bytes;

            /* Physical address of allocation. Just a name. */
            IN gctUINT32            physical;

            /* Logical address of allocation. */
            IN gctUINT64            logical;
        }
        FreeVirtualCommandBuffer;

        /* gcvHAL_EVENT_COMMIT. */
        struct _gcsHAL_EVENT_COMMIT
        {
            /* Event queue in gcsQUEUE. */
            IN gctUINT64            queue;
        }
        Event;

        /* gcvHAL_COMMIT */
        struct _gcsHAL_COMMIT
        {
            /* Context buffer object gckCONTEXT. */
            IN gctUINT64            context;

            /* Command buffer gcoCMDBUF. */
            IN gctUINT64            commandBuffer;

            /* State delta buffer in gcsSTATE_DELTA. */
            gctUINT64               delta;

            /* deltas[]/contexts[]/commandBuffers[gcvCORE_COUNT] (240 bytes)
             * removed here -- stock's real single-core COMMIT handler
             * (decompiled) uses only the singular context/commandBuffer/
             * delta fields above, confirmed via exact byte-offset match
             * (context@0, commandBuffer@8, delta@16, queue@24 relative to
             * the union). These arrays are multi-core-only, unused by this
             * chip. See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b. */

            /* Event queue in gcsQUEUE. */
            IN gctUINT64            queue;

            /* Used to distinguish different FE. */
            IN gceENGINE            engine1;

            /* The command buffer is linked to multiple command queue. */
            IN gctBOOL              shared;

            /* Index of command queue. */
            IN gctUINT32            index;

            /* Count of gpu core. */
            IN gctUINT32            count;

            /* Commit stamp of this commit. */
            OUT gctUINT64           commitStamp;

            /* If context switch for this commit */
            OUT gctBOOL             contextSwitched;
        }
        Commit;

        /* gcvHAL_MAP_USER_MEMORY */
        struct _gcsHAL_MAP_USER_MEMORY
        {
            /* Base address of user memory to map. */
            IN gctUINT64                memory;

            /* Physical address of user memory to map. */
            IN gctUINT32                physical;

            /* Size of user memory in bytes to map. */
            IN gctUINT64                size;

            /* Info record required by gcvHAL_UNMAP_USER_MEMORY. Just a name. */
            OUT gctUINT32               info;

            /* Physical address of mapped memory. */
            OUT gctUINT32               address;
        }
        MapUserMemory;

        /* gcvHAL_UNMAP_USER_MEMORY */
        struct _gcsHAL_UNMAP_USER_MEMORY
        {
            /* Base address of user memory to unmap. */
            IN gctUINT64                memory;

            /* Size of user memory in bytes to unmap. */
            IN gctUINT64                size;

            /* Info record returned by gcvHAL_MAP_USER_MEMORY. Just a name. */
            IN gctUINT32                info;

            /* Physical address of mapped memory as returned by
               gcvHAL_MAP_USER_MEMORY. */
            IN gctUINT32                address;
        }
        UnmapUserMemory;
#if !USE_NEW_LINUX_SIGNAL
        /* gcsHAL_USER_SIGNAL  */
        struct _gcsHAL_USER_SIGNAL
        {
            /* Command. */
            gceUSER_SIGNAL_COMMAND_CODES command;

            /* Signal ID. */
            IN OUT gctINT               id;

            /* Reset mode. */
            IN gctBOOL                  manualReset;

            /* Wait timedout. */
            IN gctUINT32                wait;

            /* State. */
            IN gctBOOL                  state;
        }
        UserSignal;
#endif

        /* gcvHAL_SIGNAL. */
        struct _gcsHAL_SIGNAL
        {
            /* Signal handle to signal gctSIGNAL. */
            IN gctUINT64                signal;

            /* Reserved gctSIGNAL. */
            IN gctUINT64                auxSignal;

            /* Process owning the signal gctHANDLE. */
            IN gctUINT64                process;

#if defined(__QNXNTO__)
            /* Client pulse side-channel connection ID. Set by client in gcoOS_CreateSignal. */
            IN gctINT32                 coid;

            /* Set by server. */
            IN gctINT32                 rcvid;
#endif
            /* Event generated from where of pipeline */
            IN gceKERNEL_WHERE          fromWhere;
        }
        Signal;

        /* gcvHAL_WRITE_DATA. */
        struct _gcsHAL_WRITE_DATA
        {
            /* Address to write data to. */
            IN gctUINT32                address;

            /* Data to write. */
            IN gctUINT32                data;
        }
        WriteData;

        /* gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY */
        struct _gcsHAL_ALLOCATE_CONTIGUOUS_MEMORY
        {
            /* Number of bytes to allocate. */
            IN OUT gctUINT64            bytes;

            /* Hardware address of allocation. */
            OUT gctUINT32               address;

            /* Physical address of allocation. Just a name. */
            OUT gctUINT32               physical;

            /* Logical address of allocation. */
            OUT gctUINT64               logical;
        }
        AllocateContiguousMemory;

        /* gcvHAL_FREE_CONTIGUOUS_MEMORY */
        struct _gcsHAL_FREE_CONTIGUOUS_MEMORY
        {
            /* Number of bytes allocated. */
            IN gctUINT64                bytes;

            /* Physical address of allocation. Just a name. */
            IN gctUINT32                physical;

            /* Logical address of allocation. */
            IN gctUINT64                logical;
        }
        FreeContiguousMemory;

        /* gcvHAL_READ_REGISTER */
        struct _gcsHAL_READ_REGISTER
        {
            /* Logical address of memory to write data to. */
            IN gctUINT32            address;

            /* Data read. */
            OUT gctUINT32           data;
        }
        ReadRegisterData;

        /* gcvHAL_WRITE_REGISTER */
        struct _gcsHAL_WRITE_REGISTER
        {
            /* Logical address of memory to write data to. */
            IN gctUINT32            address;

            /* Data read. */
            IN gctUINT32            data;
        }
        WriteRegisterData;

        /* gcvHAL_READ_REGISTER_EX */
        struct _gcsHAL_READ_REGISTER_EX
        {
            /* Logical address of memory to write data to. */
            IN gctUINT32            address;

            IN gctUINT32            coreSelect;

            /* Data read. */
            OUT gctUINT32           data[4];
        }
        ReadRegisterDataEx;

        /* gcvHAL_WRITE_REGISTER_EX */
        struct _gcsHAL_WRITE_REGISTER_EX
        {
            /* Logical address of memory to write data to. */
            IN gctUINT32            address;

            IN gctUINT32            coreSelect;

            /* Data read. */
            IN gctUINT32            data[4];
        }
        WriteRegisterDataEx;

#if VIVANTE_PROFILER
        /* gcvHAL_GET_PROFILE_SETTING */
        struct _gcsHAL_GET_PROFILE_SETTING
        {
            /* Enable profiling */
            OUT gctBOOL             enable;
        }
        GetProfileSetting;

        /* gcvHAL_SET_PROFILE_SETTING */
        struct _gcsHAL_SET_PROFILE_SETTING
        {
            /* Enable profiling */
            IN gctBOOL              enable;
        }
        SetProfileSetting;

        /* gcvHAL_READ_PROFILER_REGISTER_SETTING */
        struct _gcsHAL_READ_PROFILER_REGISTER_SETTING
        {
            /*Should Clear Register*/
            IN gctBOOL               bclear;
        }
        SetProfilerRegisterClear;

        /* RegisterProfileData_part1/part2 (gcsPROFILER_COUNTERS_PART1/2,
         * ~360 bytes) removed here -- stock's real, production
         * 5.0.11.28018 driver doesn't have these commands at all (not in
         * its decompiled dispatch table), and this GPU-perf-counter-dump
         * feature is unused by any actual rendering/compositing path.
         * They inflated gcsHAL_INTERFACE's union past stock's real
         * on-the-wire size. See
         * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b. The
         * corresponding gckKERNEL_Dispatch cases now unconditionally
         * return gcvSTATUS_NOT_SUPPORTED instead. */

        /* gcvHAL_PROFILE_REGISTERS_2D */
        struct _gcsHAL_PROFILE_REGISTERS_2D
        {
            /* Data read in gcs2D_PROFILE. */
            OUT gctUINT64       hwProfile2D;
        }
        RegisterProfileData2D;
#endif

        /* Power management. */
        /* gcvHAL_SET_POWER_MANAGEMENT_STATE */
        struct _gcsHAL_SET_POWER_MANAGEMENT
        {
            /* Data read. */
            IN gceCHIPPOWERSTATE        state;
        }
        SetPowerManagement;

        /* gcvHAL_QUERY_POWER_MANAGEMENT_STATE */
        struct _gcsHAL_QUERY_POWER_MANAGEMENT
        {
            /* Data read. */
            OUT gceCHIPPOWERSTATE       state;

            /* Idle query. */
            OUT gctBOOL                 isIdle;
        }
        QueryPowerManagement;

        /* gcvHAL_QUERY_KERNEL_SETTINGS */
        struct _gcsHAL_QUERY_KERNEL_SETTINGS
        {
            /* Settings.*/
            OUT gcsKERNEL_SETTINGS      settings;
        }
        QueryKernelSettings;

        /* gcvHAL_MAP_PHYSICAL */
        struct _gcsHAL_MAP_PHYSICAL
        {
            /* gcvTRUE to map, gcvFALSE to unmap. */
            IN gctBOOL                  map;

            /* Physical address. */
            IN OUT gctUINT64            physical;
        }
        MapPhysical;

        /* gcvHAL_DEBUG */
        struct _gcsHAL_DEBUG
        {
            /* If gcvTRUE, set the debug information. */
            IN gctBOOL                  set;
            IN gctUINT32                level;
            IN gctUINT32                zones;
            IN gctBOOL                  enable;

            IN gceDEBUG_MESSAGE_TYPE    type;
            IN gctUINT32                messageSize;

            /* Message to print if not empty. */
            IN gctCHAR                  message[80];

        }
        Debug;

        /* gcvHAL_CACHE */
        struct _gcsHAL_CACHE
        {
            IN gceCACHEOPERATION        operation;
            IN gctUINT64                process;
            IN gctUINT64                logical;
            IN gctUINT64                bytes;
            IN gctUINT32                node;
        }
        Cache;

        /* gcvHAL_TIMESTAMP */
        struct _gcsHAL_TIMESTAMP
        {
            /* Timer select. */
            IN gctUINT32                timer;

            /* Timer request type (0-stop, 1-start, 2-send delta). */
            IN gctUINT32                request;

            /* Result of delta time in microseconds. */
            OUT gctINT32                timeDelta;
        }
        TimeStamp;

        /* gcvHAL_DATABASE */
        struct _gcsHAL_DATABASE
        {
            /* Set to gcvTRUE if you want to query a particular process ID.
            ** Set to gcvFALSE to query the last detached process. */
            IN gctBOOL                  validProcessID;

            /* Process ID to query. */
            IN gctUINT32                processID;

            /* Information. */
            OUT gcuDATABASE_INFO        vidMem;
            OUT gcuDATABASE_INFO        nonPaged;
            OUT gcuDATABASE_INFO        contiguous;
            OUT gcuDATABASE_INFO        gpuIdle;

            /* Detail information about video memory. Shrunk from [3] to [1]
             * 2026-07-20 -- part of restoring gcsHAL_INTERFACE's on-the-wire
             * size to match stock's real driver (see
             * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b).
             * gckKERNEL_QueryDatabase's loop bound was reduced to match
             * (only pool type 0's stats are reported now instead of all 3)
             * -- this command isn't in stock's real command set at all, and
             * isn't used by any rendering/compositing path either way. */
            OUT gcuDATABASE_INFO        vidMemPool[1];
        }
        Database;

        /* gcvHAL_VERSION */
        struct _gcsHAL_VERSION
        {
            /* Major version: N.n.n. */
            OUT gctINT32                major;

            /* Minor version: n.N.n. */
            OUT gctINT32                minor;

            /* Patch version: n.n.N. */
            OUT gctINT32                patch;

            /* Build version. */
            OUT gctUINT32               build;
        }
        Version;

        /* gcvHAL_CHIP_INFO */
        struct _gcsHAL_CHIP_INFO
        {
            /* Chip count. */
            OUT gctINT32                count;

            /* Chip types. */
            OUT gceHARDWARE_TYPE        types[gcdCHIP_COUNT];

            /* Chip IDs. */
            OUT gctUINT32               ids[gcvCORE_COUNT];
        }
        ChipInfo;

        /* gcvHAL_ATTACH */
        struct _gcsHAL_ATTACH
        {
            /* Handle of context buffer object. */
            OUT gctUINT32               context;

            /* Unidentified 4-byte gap -- decompiled stock's real ATTACH
             * dispatch case writes its single maxState/numStates-equivalent
             * value at union-relative offset 8, not 4, meaning something
             * unaccounted-for sits here (same pattern as the outer struct's
             * header gap). See docs/DEVICE_TEST_CHECKLIST_2026-07-18.md
             * section 1b. */
            gctUINT32                   _reserved0;

            /* Maximum state in the buffer -- confirmed 4 bytes (a single
             * 32-bit value written via gckCOMMAND_Attach's 3rd arg), not
             * gctUINT64 as originally declared here; numStates as a
             * separate field doesn't appear to exist in stock's real
             * struct at all. */
            OUT gctUINT32               maxState;

            /* Unidentified 4-byte gap before "map" (confirmed at
             * union-relative offset 16 in stock's decompiled code). */
            gctUINT32                   _reserved1;

            /* Map context buffer to user or not. */
            IN gctBOOL                  map;

            /* Physical of context buffer. */
            OUT gctUINT32               physicals[2];

            /* Physical of context buffer. */
            OUT gctUINT64               logicals[2];

            /* Bytes of context buffer. */
            OUT gctUINT32               bytes;
        }
        Attach;

        /* gcvHAL_DETACH */
        struct _gcsHAL_DETACH
        {
            /* Context buffer object gckCONTEXT. Just a name. */
            IN gctUINT32                context;
        }
        Detach;

        /* gcvHAL_COMPOSE.
         *
         * NOT YET IMPLEMENTED -- this struct's shape is reconstructed
         * purely from decompiling stock's dispatch call site and the body
         * of stock's gckEVENT_Compose (address 0x1323c in stock's real
         * galcore.ko), NOT from any source in this tree (neither
         * gckEVENT_Compose nor gckHARDWARE_Compose exist here at all).
         *
         * Stock's dispatch case does:
         *   1. gckKERNEL_QueryPointerFromName(Kernel, Interface+32) to
         *      resolve a name into a pointer, then OVERWRITES Interface+32
         *      with that resolved pointer and zeroes Interface+36 before
         *      calling gckEVENT_Compose(Kernel-><field at Kernel+20>,
         *      &Interface+32).
         *   2. gckEVENT_Compose itself then reads 8/4-byte fields from its
         *      second argument (i.e. from this union, byte-relative to its
         *      own start) at relative offsets 0x00, 0x08, 0x10, 0x14,
         *      0x18, 0x20, 0x28, and 0x38 -- the highest access (0x38 + 8
         *      bytes) means up to 64 bytes of this union region are
         *      touched. Field purposes were not identified (would require
         *      decompiling gckHARDWARE_Compose too, which doesn't exist in
         *      this tree either -- that's a separate, larger reverse-
         *      engineering task, not done here).
         *
         * This struct exists ONLY to reserve the correct 64-byte budget in
         * the union (well within the union's 232-byte pad budget) so that
         * a future implementation has a properly-sized home and doesn't
         * silently grow gcsHAL_INTERFACE past its exact 264-byte on-wire
         * size. The corresponding gc_hal_kernel.c case intentionally
         * returns gcvSTATUS_NOT_SUPPORTED rather than fabricating logic
         * for functions that don't exist in this source tree.
         */
        struct _gcsHAL_COMPOSE
        {
            gctUINT8                    _unidentified[64];
        }
        ComposeInfo;

        /* gcvHAL_GET_FRAME_INFO. */
        struct _gcsHAL_GET_FRAME_INFO
        {
            /* gcsHAL_FRAME_INFO* */
            OUT gctUINT64     frameInfo;
        }
        GetFrameInfo;

        /* gcvHAL_SET_TIME_OUT. */
        struct _gcsHAL_SET_TIMEOUT
        {
            gctUINT32                   timeOut;
        }
        SetTimeOut;

#if gcdENABLE_VG
        /* gcvHAL_COMMIT */
        struct _gcsHAL_VGCOMMIT
        {
            /* Context buffer. gcsVGCONTEXT_PTR */
            IN gctUINT64                context;

            /* Command queue. gcsVGCMDQUEUE_PTR */
            IN gctUINT64                queue;

            /* Number of entries in the queue. */
            IN gctUINT                  entryCount;

            /* Task table. gcsTASK_MASTER_TABLE_PTR */
            IN gctUINT64                taskTable;
        }
        VGCommit;

        /* gcvHAL_QUERY_COMMAND_BUFFER */
        struct _gcsHAL_QUERY_COMMAND_BUFFER
        {
            /* Command buffer attributes. */
            OUT gcsCOMMAND_BUFFER_INFO    information;
        }
        QueryCommandBuffer;

#endif

        struct _gcsHAL_SET_FSCALE_VALUE
        {
            IN gctUINT              value;
        }
        SetFscaleValue;

        struct _gcsHAL_GET_FSCALE_VALUE
        {
            OUT gctUINT             value;
            OUT gctUINT             minValue;
            OUT gctUINT             maxValue;
        }
        GetFscaleValue;

        /* gcvHAL_EXPORT_VIDEO_MEMORY */
        struct _gcsHAL_EXPORT_VIDEO_MEMORY
        {
            /* Allocated video memory. */
            IN gctUINT32                node;

            /* Export flags */
            IN gctUINT32                flags;

            /* Exported dma_buf fd */
            OUT gctINT32                fd;
        }
        ExportVideoMemory;

        struct _gcsHAL_NAME_VIDEO_MEMORY
        {
            IN gctUINT32            handle;
            OUT gctUINT32           name;
        }
        NameVideoMemory;

        struct _gcsHAL_IMPORT_VIDEO_MEMORY
        {
            IN gctUINT32            name;
            OUT gctUINT32           handle;
        }
        ImportVideoMemory;

        struct _gcsHAL_QUERY_RESET_TIME_STAMP
        {
            OUT gctUINT64           timeStamp;
            OUT gctUINT64           contextID;
        }
        QueryResetTimeStamp;

        struct _gcsHAL_CREATE_NATIVE_FENCE
        {
            /* Signal id. */
            IN gctUINT64                signal;

            /* Native fence file descriptor. */
            OUT gctINT                  fenceFD;

        }
        CreateNativeFence;

        struct _gcsHAL_WAIT_NATIVE_FENCE
        {
            /* Native fence file descriptor. */
            IN gctINT                   fenceFD;

            /* Wait timeout. */
            IN gctUINT32                timeout;
        }
        WaitNativeFence;

        struct _gcsHAL_DESTROY_MMU
        {
            /* Mmu object. */
            IN gctUINT64                mmu;
        }
        DestroyMmu;

        struct _gcsHAL_SHBUF
        {
            gceSHBUF_COMMAND_CODES      command;

            /* Shared buffer. */
            IN OUT gctUINT64            id;

            /* User data to be shared. */
            IN gctUINT64                data;

            /* Data size. */
            IN OUT gctUINT32            bytes;
        }
        ShBuf;

        struct _gcsHAL_GET_GRAPHIC_BUFFER_FD
        {
            /* Max 3 video nodes, node handle here. */
            IN gctUINT32                node[3];

            /* A shBuf. */
            IN gctUINT64                shBuf;

            /* A signal. */
            IN gctUINT32                signal;

            OUT gctINT32                fd;
        }
        GetGraphicBufferFd;

        struct _gcsHAL_VIDEO_MEMORY_METADATA
        {
            /* Allocated video memory. */
            IN gctUINT32            node;

            IN gctUINT32            readback;

            INOUT gctINT32          ts_fd;
            INOUT gctUINT32         fc_enabled;
            INOUT gctUINT32         fc_value;
            INOUT gctUINT32         fc_value_upper;

            INOUT gctUINT32         compressed;
            INOUT gctUINT32         compress_format;
        }
        SetVidMemMetadata;

        struct _gcsHAL_GET_VIDEO_MEMORY_FD
        {
            IN gctUINT32            handle;
            OUT gctINT              fd;
        }
        GetVideoMemoryFd;

        struct _gcsHAL_CONFIG_POWER_MANAGEMENT
        {
            IN gctBOOL                  enable;
        }
        ConfigPowerManagement;

        struct _gcsHAL_WRAP_USER_MEMORY
        {
            /* Description of user memory. */
            IN gcsUSER_MEMORY_DESC      desc;

            /* Output video mmory node. */
            OUT gctUINT32               node;

            /* size of the node in bytes */
            OUT gctUINT64               bytes;
        }
        WrapUserMemory;

        struct _gcsHAL_WAIT_FENCE
        {
            IN gctUINT32                handle;
            IN gctUINT32                timeOut;
        }
        WaitFence;

        struct _gcsHAL_COMMIT_DONE
        {
            IN gctUINT64                context;
        }
        CommitDone;

#if gcdDEC_ENABLE_AHB
        struct _gcsHAL_DEC300_READ
        {
            gctUINT32      enable;
            gctUINT32      readId;
            gctUINT32      format;
            gctUINT32      strides[3];
            gctUINT32      is3D;
            gctUINT32      isMSAA;
            gctUINT32      clearValue;
            gctUINT32      isTPC;
            gctUINT32      isTPCCompressed;
            gctUINT32      surfAddrs[3];
            gctUINT32      tileAddrs[3];
        }
        DEC300Read;

        struct _gcsHAL_DEC300_WRITE
        {
            gctUINT32       enable;
            gctUINT32       readId;
            gctUINT32       writeId;
            gctUINT32       format;
            gctUINT32       surfAddr;
            gctUINT32       tileAddr;
        }
        DEC300Write;

        struct _gcsHAL_DEC300_FLUSH
        {
            IN gctUINT8     useless;
        }
        DEC300Flush;

        struct _gcsHAL_DEC300_FLUSH_WAIT
        {
            IN gctUINT32    done;
        }
        DEC300FlushWait;
#endif
        /* gcvHAL_BOTTOM_HALF_UNLOCK_VIDEO_MEMORY: */
        struct _gcsHAL_BOTTOM_HALF_UNLOCK_VIDEO_MEMORY
        {
            /* Allocated video memory. */
            IN gctUINT32                node;

            /* Type of surface. */
            IN gceSURF_TYPE             type;
        }
        BottomHalfUnlockVideoMemory;

        gcsHAL_QUERY_CHIP_OPTIONS QueryChipOptions;

        /* Padding-only member -- forces this union to exactly 232 bytes
         * (232 + 32-byte header = 264 total), matching stock's real,
         * decompiled 5.0.11.28018 gcsHAL_INTERFACE size exactly. Sized
         * empirically: after removing all the multi-core/profiler bloat
         * above, the union's real largest member came out to 208 bytes,
         * 24 short of target. See
         * docs/DEVICE_TEST_CHECKLIST_2026-07-18.md section 1b. */
        gctUINT8 _unionSizePad[232];
    }
    u;
}
gcsHAL_INTERFACE;


#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_driver_h_ */
