//********************************************************************
//	created:	4:11:2009   10:03
//	filename: 	StdAfx.h
//	author:		tiamo
//	purpose:	stdafx
//********************************************************************

#ifndef __STDAFX_H__
#define __STDAFX_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#include "macros.h"

#ifndef DEBUG_LDRP_CALL_CSPRINTF
#define DEBUG_LDRP_CALL_CSPRINTF											0
#endif /* DEBUG_LDRP_CALL_CSPRINTF */

#ifndef DEBUG_NVRAM_CALL_CSPRINTF
#define DEBUG_NVRAM_CALL_CSPRINTF											0
#endif /* DEBUG_NVRAM_CALL_CSPRINTF */

#ifndef DEBUG_KERNEL_PATCHER
#define DEBUG_KERNEL_PATCHER												0
#endif /*  DEBUG_KERNEL_PATCHER */

#ifndef OS_LEGACY
#define OS_LEGACY															6
#endif /*  OS_LEGACY */

#ifndef YOSEMITE
#define YOSEMITE															10
#endif /*  YOSEMITE */

#ifndef EL_CAPITAN
#define EL_CAPITAN															11
#endif /*  EL_CAPITAN */

#ifndef TARGET_OS
#define TARGET_OS															EL_CAPITAN
#endif /* TARGET_OS */
#if (TARGET_OS >= YOSEMITE)
#ifndef LEGACY_GREY_SUPPORT
#define LEGACY_GREY_SUPPORT												    1
#endif /* LEGACY_GREY_SUPPORT */
#else
#ifndef LEGACY_GREY_SUPPORT
#define LEGACY_GREY_SUPPORT												    0
#endif /* LEGACY_GREY_SUPPORT */
#endif /* TARGET_OS */

#ifndef PATCH_LOAD_EXECUTABLE
#define PATCH_LOAD_EXECUTABLE												1
#endif /* PATCH_LOAD_EXECUTABLE */

//
// grep '\x48\x89\xc3\x48\x85\xdb\x74\x70' /S*/L*/Kernels/kernel
//
#define LOAD_EXECUTABLE_TARGET_UINT64										0x487074db8548c389ULL
#define LOAD_EXECUTABLE_PATCH_UINT64										0x4812ebdb8548c389ULL

#define PATCH_READ_STARTUP_EXTENSIONS										0
//
// grep '\xe8\x25\x00\x00\x00\xeb\x05\xe8' /S*/L*/Kernels/kernel
//
#define READ_STARTUP_EXTENSIONS_TARGET_UINT64								0xe805eb00000025e8ULL
#define READ_STARTUP_EXTENSIONS_PATCH_UINT64								0xe8909000000025e8ULL

#ifndef DO_REPLACE_BOARD_ID
#define DO_REPLACE_BOARD_ID 1
#endif /* DO_REPLACE_BOARD_ID */

#if DO_REPLACE_BOARD_ID
#define MACPRO_31														    "Mac-F42C88C8"
#define MACBOOKPRO_31													    "Mac-F4238BC8"
#define DEBUG_BOARD_ID_CSPRINTF											    0
#endif /* DO_REPLACE_BOARD_ID */

#define NOTHING
#define BOOTAPI																__cdecl
#define CHAR8_CONST_STRING(S)												(CHAR8 CONST*)(S)
#define CHAR16_CONST_STRING(S)												(CHAR16 CONST*)(S)
#define CHAR8_STRING(S)														(CHAR8*)(S)
#define CHAR16_STRING(S)													(CHAR16*)(S)
#define try_leave(S)														do{S;__leave;}while(0)
#define ARRAYSIZE(A)														(sizeof((A)) / sizeof((A)[0]))
#define ArchConvertAddressToPointer(P,T)									((T)((UINTN)(P)))
#define ArchConvertPointerToAddress(A)										((UINTN)(A))
#define ArchNeedEFI64Mode() (MmGetKernelVirtualStart() > (UINT32)(-1) || sizeof(UINTN) == sizeof(UINT64))
#define LdrStaticVirtualToPhysical(V)										((V) & (1 * 1024 * 1024 * 1024 - 1))
#define Add2Ptr(P, O, T)													ArchConvertAddressToPointer(ArchConvertPointerToAddress(P) + (O), T)
#ifndef PAGE_ALIGN
#define PAGE_ALIGN(A)														((A) & ~EFI_PAGE_MASK)
#endif /*  PAGE_ALIGN */

#ifndef BYTE_OFFSET
#define BYTE_OFFSET(A)														((UINT32)((UINT64)(A) & 0xFFFFFFFF) & EFI_PAGE_MASK)
#endif

#ifndef SWAP32
#define SWAP32(V)															((((UINT32)(V) & 0xff) << 24) | (((UINT32)(V) & 0xff00) << 8) | (((UINT32)(V) & 0xff0000) >> 8) |  (((UINT32)(V) & 0xff000000) >> 24))
#endif /* SWAP32 */
#ifndef SWAP_BE32_TO_HOST
#define SWAP_BE32_TO_HOST													SWAP32
#endif /* SWAP_BE32_TO_HOST */

#if (TARGET_OS == EL_CAPITAN)
typedef enum _CSR_ALLOW_FLAGS
{
    CSR_ALLOW_NONE                 = (     0),    // 0
    CSR_ALLOW_UNTRUSTED_KEXTS      = (1 << 0),    // 1
    CSR_ALLOW_UNRESTRICTED_FS      = (1 << 1),    // 2
    CSR_ALLOW_TASK_FOR_PID         = (1 << 2),    // 4
    CSR_ALLOW_KERNEL_DEBUGGER      = (1 << 3),    // 8
    CSR_ALLOW_APPLE_INTERNAL       = (1 << 4),    // 16
    CSR_ALLOW_UNRESTRICTED_DTRACE  = (1 << 5),    // 32
    CSR_ALLOW_UNRESTRICTED_NVRAM   = (1 << 6),    // 64
    CSR_ALLOW_DEVICE_CONFIGURATION = (1 << 7),    // 128
    CSR_VALID_FLAGS                = (CSR_ALLOW_UNTRUSTED_KEXTS | CSR_ALLOW_UNRESTRICTED_FS | CSR_ALLOW_TASK_FOR_PID | CSR_ALLOW_KERNEL_DEBUGGER | CSR_ALLOW_APPLE_INTERNAL | CSR_ALLOW_UNRESTRICTED_DTRACE | CSR_ALLOW_UNRESTRICTED_NVRAM | CSR_ALLOW_DEVICE_CONFIGURATION)
} CSR_ALLOW_FLAGS;
#endif // #if (TARGET_OS == YOSMITE)

#include "../../sdk/include/EfiCommon.h"
#include "../../sdk/include/EfiApi.h"
#include "../../sdk/include/EfiImage.h"
#include "../../sdk/include/EfiDevicePath.h"
#include "../../sdk/include/IndustryStandard/Acpi.h"
#include "../../sdk/include/IndustryStandard/pci.h"
#include "../../sdk/include/IndustryStandard/SmBios.h"

#include "GuidDefine.h"
#include "RuntimeLib.h"
#include "ArchUtils.h"
#include "Memory.h"
#include "MiscUtils.h"
#include "AcpiUtils.h"
#include "PeImage.h"
#include "BootDebugger.h"
#include "Base64.h"
#include "Crc32.h"
#include "MD5.h"
#include "SHA256.h"
#include "DeviceTree.h"
#include "DevicePath.h"
#include "Config.h"
#include "FileIo.h"
#include "MachO.h"
#include "PlatformExpert.h"
#include "NetBoot.h"
#include "Hibernate.h"
#include "Console.h"
#include "Options.h"
#include "LoadKernel.h"
#include "LoadDrivers.h"
#include "BootArgs.h"
#include "MemoryMap.h"
#include "PanicDialog.h"
#include "FileVault.h"

#endif /* __STDAFX_H__ */
