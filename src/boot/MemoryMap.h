//********************************************************************
//	created:	9:11:2009   0:30
//	filename: 	MemoryMap.h
//	author:		tiamo
//	purpose:	memory map
//********************************************************************

#ifndef __MEMORYMAP_H__
#define __MEMORYMAP_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// get memory map
//
EFI_STATUS MmGetMemoryMap(UINTN* memoryMapSize, EFI_MEMORY_DESCRIPTOR** memoryMap, UINTN* memoryMapKey, UINTN* descriptorSize, UINT32* descriptorVersion);

//
// get runtime memory info
//
UINTN MmGetRuntimeMemoryInfo(EFI_MEMORY_DESCRIPTOR* memoryMap, UINTN memoryMapSize, UINTN descriptorSize, UINT64* totalPages);

//
// remove non runtime descriptors
//
EFI_STATUS MmRemoveNonRuntimeDescriptors(EFI_MEMORY_DESCRIPTOR* memoryMap, UINTN* memoryMapSize, UINTN descriptorSize);

//
// sort
//
VOID MmSortMemoryMap(EFI_MEMORY_DESCRIPTOR* memoryMap, UINTN memoryMapSize, UINTN descriptorSize);

//
// convert pointers
//
EFI_STATUS MmConvertPointers(EFI_MEMORY_DESCRIPTOR* memoryMap, UINTN* memmapSize, UINTN descSize, UINT32 descVersion, UINT64 rtPhysical, UINT64 runtimePages, UINT64 rtVirtual, UINT64* efiSysTablePhy, BOOLEAN createSubRegion, struct _MACH_O_LOADED_INFO* loadedInfo);

#endif /* __MEMORYMAP_H__ */
