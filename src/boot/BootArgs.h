//********************************************************************
//	created:	8:11:2009   16:39
//	filename: 	BootArgs.h
//	author:		tiamo
//	purpose:	boot arg
//********************************************************************

#ifndef __BOOTARGS_H__
#define __BOOTARGS_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#include "macros.h"

//
// video
//
#if defined(_MSC_VER)
#include <pshpack1.h>
#endif /* _MSC_VER */

#ifndef BOOT_LINE_LENGTH
#define BOOT_LINE_LENGTH 1024
#endif /* BOOT_LINE_LENGTH */

#ifndef BOOT_STRING_LEN
#define BOOT_STRING_LEN  BOOT_LINE_LENGTH
#endif /* BOOT_STRING_LEN */

typedef enum _CONSOLE_MODE
{
    GRAPHICS_NONE = 0,
    GRAPHICS_MODE = 1,
    FB_TEXT_MODE  = 2
} CONSOLE_MODE;

/* Boot argument structure - passed into Mach kernel at boot time.
 * "Revision" can be incremented for compatible changes
 */
/* Snapshot constants of previous revisions that are supported */
typedef enum _BOOTARGS_VERSION
{
    kBootArgsRevision      = 0,
    kBootArgsRevision2_0   = 0,
    kBootArgsRevision1     = 1,
    kBootArgsVersion1      = 1,
    kBootArgsVersion2      = 2,
    kBootArgsVersion       = 2
} BOOTARGS_VERSION;

/*
 * Types of boot driver that may be loaded by the booter.
 */
typedef enum _BOOT_DRIVER_TYPE {
    kBootDriverTypeInvalid = 0,
    kBootDriverTypeKEXT    = 1,
    kBootDriverTypeMKEXT   = 2
} BOOT_DRIVER_TYPE;

typedef enum _EFI_TYPES {
    kEfiReservedMemoryType      = 0,
    kEfiLoaderCode              = 1,
    kEfiLoaderData              = 2,
    kEfiBootServicesCode        = 3,
    kEfiBootServicesData        = 4,
    kEfiRuntimeServicesCode     = 5,
    kEfiRuntimeServicesData     = 6,
    kEfiConventionalMemory      = 7,
    kEfiUnusableMemory          = 8,
    kEfiACPIReclaimMemory       = 9,
    kEfiACPIMemoryNVS           = 10,
    kEfiMemoryMappedIO          = 11,
    kEfiMemoryMappedIOPortSpace = 12,
    kEfiPalCode                 = 13,
    kEfiMaxMemoryType           = 14
} EFI_TYPES;

typedef enum _BOOTARGS_EFI_MODE
{
    kBootArgsEfiModeNone  = 0,
    kBootArgsEfiMode16    = 16,
    kBootArgsEfiMode32    = 32,
    kBootArgsEfiMode64    = 64
} BOOTARGS_EFI_MODE;

/* Bitfields for boot_args->flags */
typedef enum _BOOTARGS_FLAGS
{
    kBootArgsFlagNone               =       0,      // 0
    kBootArgsFlagRebootOnPanic      = (1 << 0),     // 1
    kBootArgsFlagHiDPI              = (1 << 1),     // 2
    kBootArgsFlagBlack              = (1 << 2),     // 4
    kBootArgsFlagCSRActiveConfig    = (1 << 3),     // 8
    kBootArgsFlagCSRConfigMode      = (1 << 4),     // 16
    kBootArgsFlagCSRBoot            = (1 << 5),     // 32
    kBootArgsFlagBlackBg            = (1 << 6),     // 64
    kBootArgsFlagLoginUI            = (1 << 7),     // 128
    kBootArgsFlagInstallUI          = (1 << 8),     // 256
    kBootArgsFlagRecoveryBoot       = (1 << 10),    // 1024
    kBootArgsFlagAll                = (kBootArgsFlagRebootOnPanic | kBootArgsFlagHiDPI | kBootArgsFlagBlack | kBootArgsFlagCSRActiveConfig | kBootArgsFlagCSRConfigMode | kBootArgsFlagCSRBoot | kBootArgsFlagBlackBg | kBootArgsFlagLoginUI | kBootArgsFlagInstallUI | kBootArgsFlagRecoveryBoot)
} BOOTARGS_FLAGS;

/* Struct describing an image passed in by the booter */
typedef struct _BOOT_ICON_ELEMENT {
    UINT32    width;
    UINT32    height;
    INT32     y_offset_from_center;
    UINT32    data_size;
    UINT32    __reserved1[4];
    UINT8     data[0];
} BOOT_ICON_ELEMENT GNUPACK;

typedef struct _BOOT_VIDEO  {
    UINT32    DisplayMode;    /* Display Code (if Applicable */
    UINT32    BytesPerRow;    /* Number of bytes per pixel row */
    UINT32    HorzRes;    /* Width */
    UINT32    VertRes;    /* Height */
    UINT32    ColorDepth;    /* Pixel Depth */
    UINT32    Reserved[7];    /* Reserved */
    UINT64    BaseAddress;    /* Base address of video memory */
} BOOT_VIDEO GNUPACK;

ASSERT_TRUE(sizeof(BOOT_VIDEO) == 56);

typedef struct _BOOT_VIDEO_V1 {
    UINT32    BaseAddress;    /* Base address of video memory */
    UINT32    DisplayMode;    /* Display Code (if Applicable */
    UINT32    BytesPerRow;    /* Number of bytes per pixel row */
    UINT32    HorzRes;    /* Width */
    UINT32    VertRes;    /* Height */
    UINT32    ColorDepth;    /* Pixel Depth */
} BOOT_VIDEO_V1 GNUPACK;

ASSERT_TRUE(sizeof(BOOT_VIDEO_V1) == 24);

//
// boot arg
//
typedef struct _BOOT_ARGS {
    UINT16        Revision;    /* Revision of boot_args structure */
    UINT16        Version;    /* Version of boot_args structure */

    UINT8         EfiMode;    /* 32 = 32-bit, 64 = 64-bit */
    UINT8         DebugMode;  /* Bit field with behavior changes */
    UINT16        Flags;

    CHAR8         CommandLine[BOOT_LINE_LENGTH];    /* Passed in command line */

    UINT32        MemoryMap;  /* Physical address of memory map */
    UINT32        MemoryMapSize;
    UINT32        MemoryMapDescriptorSize;
    UINT32        MemoryMapDescriptorVersion;

    BOOT_VIDEO_V1 BootVideoV1;    /* Video Information */

    UINT32        DeviceTree;      /* Physical address of flattened device tree */
    UINT32        DeviceTreeLength; /* Length of flattened tree */

    UINT32        KernelAddress;            /* Physical address of beginning of kernel text */
    UINT32        KernelSize;            /* Size of combined kernel text+data+efi */

    UINT32        EfiRuntimeServicesPageStart; /* physical address of defragmented runtime pages */
    UINT32        EfiRuntimeServicesPageCount;
    UINT64        EfiRuntimeServicesVirtualPageStart; /* virtual address of defragmented runtime pages */

    UINT32        EfiSystemTable;   /* physical address of system table in runtime area */
    UINT32        ASLRDisplacement;

    UINT32        PerformanceDataStart; /* physical address of log */
    UINT32        PerformanceDataSize;

    UINT32        KeyStoreDataStart; /* physical address of key store data */
    UINT32        KeyStoreDataSize;
    UINT64        BootMemStart;
    UINT64        BootMemSize;
    UINT64        PhysicalMemorySize;
    UINT64        FSBFrequency;
    UINT64        PCIConfigSpaceBaseAddress;
    UINT32        PCIConfigSpaceStartBusNumber;
    UINT32        PCIConfigSpaceEndBusNumber;
    UINT32        CsrActiveConfig;
    UINT32        CsrCapabilities;
    UINT32        Boot_SMC_plimit;
    UINT16        BootProgressMeterStart;
    UINT16        BootProgressMeterEnd;

    BOOT_VIDEO    BootVideo;        /* Video Information */

    UINT32        ApfsDataStart; /* Physical address of apfs volume key structure */
    UINT32        ApfsDataSize;

    UINT32        __reserved4[710];
} BOOT_ARGS GNUPACK;

/* BOOT_ARGS Assert */
ASSERT_TRUE(sizeof(BOOT_ARGS) == 4096);

#if defined(_MSC_VER)
#include <poppack.h>
#endif /* _MSC_VER */

//
// add memory range
//
EFI_STATUS BlAddMemoryRangeNode(CHAR8 CONST* rangeName, UINT64 physicalAddress, UINT64 rangeLength);

//
// init boot args
//
EFI_STATUS BlInitializeBootArgs(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath, EFI_DEVICE_PATH_PROTOCOL* bootFilePath, EFI_HANDLE kernelDeviceHandle, CHAR8 CONST* bootCommandLine, BOOT_ARGS** bootArgsP);

//
// finalize boot args
//
EFI_STATUS BlFinalizeBootArgs(BOOT_ARGS* bootArgs, CHAR8 CONST* kernelCommandLine, EFI_HANDLE bootDeviceHandle, struct _MACH_O_LOADED_INFO* loadedInfo);

#if (TARGET_OS == EL_CAPITAN)
//
// Read csr-active-config from NVRAM
//
EFI_STATUS BlInitCSRState(BOOT_ARGS* bootArgs);
#endif /* (TARGET_OS == EL_CAPITAN) */

//
// Mimic boot.efi and set boot.efi info properties.
//
EFI_STATUS BlAddBooterInfo(DEVICE_TREE_NODE* chosenNode);

#endif /* __BOOTARGS_H__ */
