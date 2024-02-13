//********************************************************************
//	created:	5:11:2009   13:45
//	filename: 	ArchUtils.cpp
//	author:		tiamo
//	purpose:	arch utils
//********************************************************************

#include "../StdAfx.h"
#include "ArchDefine.h"

//
// global
//
typedef VOID (BOOTAPI *ArchTransferRoutine)(VOID* kernelEntry, VOID* bootArgs);
ArchTransferRoutine	ArchpTransferRoutine									/*= nullptr*/;

#ifdef __cplusplus
extern "C"
{
#endif
    extern VOID ArchTransferRoutineBegin();
    extern VOID ArchTransferRoutineEnd();
#ifdef __cplusplus
}
#endif

//
// init phase 0
//
EFI_STATUS BOOTAPI ArchInitialize0()
{
	return EFI_SUCCESS;
}

//
// init phase 1
//
EFI_STATUS BOOTAPI ArchInitialize1()
{
	UINTN bytesCount														= ArchConvertPointerToAddress(&ArchTransferRoutineEnd) - ArchConvertPointerToAddress(&ArchTransferRoutineBegin);
	UINT64 physicalAddress													= 4 * 1024 * 1024 * 1024ULL - 1;
	VOID* transferRoutineBuffer												= MmAllocatePages(AllocateMaxAddress, EfiLoaderCode, EFI_SIZE_TO_PAGES(bytesCount), &physicalAddress);
	if(!transferRoutineBuffer)
		return EFI_OUT_OF_RESOURCES;

	memcpy(transferRoutineBuffer, (const void *)&ArchTransferRoutineBegin, bytesCount);
	ArchpTransferRoutine													= (ArchTransferRoutine)(transferRoutineBuffer);
	return EFI_SUCCESS;
}

//
// check 64bit cpu
//
EFI_STATUS BOOTAPI ArchCheck64BitCpu()
{
	return EFI_SUCCESS;
}

//
// set idt entry
//
VOID BOOTAPI ArchSetIdtEntry(UINT64 base, UINT32 index, UINT32 segCs, VOID* offset,UINT32 access)
{
	KIDTENTRY* idtEntry														= Add2Ptr(base, index * sizeof(KIDTENTRY), KIDTENTRY*);
	idtEntry->Selector														= (UINT16)(segCs);
	idtEntry->Access														= (UINT16)(access);
	idtEntry->Offset														= (UINT16)(ArchConvertPointerToAddress(offset) & 0xffff);
	idtEntry->ExtendedOffset												= (UINT16)(ArchConvertPointerToAddress(offset) >> 16);
	idtEntry->HighOffset													= (UINT32)(ArchConvertPointerToAddress(offset) >> 32);
	idtEntry->Reserved														= 0;
}

//
// sweep instruction cache
//
VOID BOOTAPI ArchSweepIcacheRange(VOID* startAddress, UINT32 bytesCount)
{
}

//
// transfer to kernel
//
VOID BOOTAPI ArchStartKernel(VOID* kernelEntry, VOID* bootArgs)
{
	ArchpTransferRoutine(kernelEntry, bootArgs);
}

//
// setup thunk code
//
VOID BOOTAPI ArchSetupThunkCode0(UINT64 thunkOffset, struct _MACH_O_LOADED_INFO* loadedInfo)
{

}

//
// setup thunk code
//
VOID BOOTAPI ArchSetupThunkCode1(UINT64* efiSystemTablePhysicalAddress, UINT64 thunkOffset)
{
}
