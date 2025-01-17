//********************************************************************
//	created:	12:11:2009   1:48
//	filename: 	LoadKernel.h
//	author:		tiamo
//	purpose:	load kernel
//********************************************************************

#ifndef __LOADKERNEL_H__
#define __LOADKERNEL_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

typedef enum _CPU_ARCH
{
    CPU_ARCH_NONE = 0x00000000,
    CPU_ARCH_X86  = 0x00000007,
    CPU_ARCH_X64  = 0x01000007
} CPU_ARCH;

//
// load kernel cache
//
EFI_STATUS LdrLoadKernelCache(struct _MACH_O_LOADED_INFO* loadedInfo, EFI_DEVICE_PATH_PROTOCOL* bootDevicePath);

//
// load kernel
//
EFI_STATUS LdrLoadKernel(struct _MACH_O_LOADED_INFO* loadedInfo);

//
// load ramdisk
//
EFI_STATUS LdrLoadRamDisk();

//
// setup ASLR
//
VOID LdrSetupASLR(BOOLEAN enableASLR, UINT8 slideValue);

//
// get aslr displacement
//
UINT64 LdrGetASLRDisplacement();

//
// get kernel path name
//
CHAR8 CONST* LdrGetKernelPathName();

//
// get kernel cache path name
//
CHAR8 CONST* LdrGetKernelCachePathName();

//
// get kernel cache override
//
BOOLEAN LdrGetKernelCacheOverride();

//
// setup kernel cache path
//
VOID LdrSetupKernelCachePath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName, BOOLEAN cacheOverride);

//
// setup kernel path
//
VOID LdrSetupKernelPath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName);

//
// setup ramdisk path
//
VOID LdrSetupRamDiskPath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName);

#endif /* __LOADKERNEL_H__ */
