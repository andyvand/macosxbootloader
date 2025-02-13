//********************************************************************
//	created:	4:11:2009   17:42
//	filename: 	memory.cpp
//	author:		tiamo
//	purpose:	memory
//********************************************************************

#include "StdAfx.h"

//
// memory tag
//
typedef struct _MEMORY_TAG
{
	//
	// physical address
	//
	UINT64																	PhysicalAddress;

	//
	// length
	//
	UINT64																	BufferLength;

	//
	// address
	//
	VOID*																	VirtualAddress;

	//
	// next
	//
	struct _MEMORY_TAG*														Next;
}MEMORY_TAG;

//
// global
//
STATIC MEMORY_TAG*	MmpNextFreeMemoryTag									= NULL;
STATIC MEMORY_TAG*	MmpAllocatedMemoryTagPoolHead							= NULL;
STATIC MEMORY_TAG*	MmpAllocatedMemoryTagHead								= NULL;
STATIC UINT64 MmpKernelVirtualBegin											= (UINT64)(-1);
STATIC UINT64 MmpKernelVirtualEnd											= 0;
STATIC UINT64 MmpKernelPhysicalBegin										= (UINT64)(-1);
STATIC UINT64 MmpKernelPhysicalEnd											= 0;

//
// get memory tag
//
STATIC MEMORY_TAG* MmpGetMemoryTag()
{
	if (!MmpNextFreeMemoryTag)
	{
		if (!EfiBootServices)
			return NULL;

		VOID* memoryTagPool													= NULL;

		if (EFI_ERROR(EfiBootServices->AllocatePool(EfiBootServicesData, EFI_PAGE_SIZE, &memoryTagPool)))
			return NULL;

		MEMORY_TAG* allocatedMemoryTagPool									= (MEMORY_TAG*)(memoryTagPool);
		allocatedMemoryTagPool->BufferLength								= EFI_PAGE_SIZE;
		allocatedMemoryTagPool->VirtualAddress								= memoryTagPool;
		allocatedMemoryTagPool->Next										= MmpAllocatedMemoryTagPoolHead;
		MmpAllocatedMemoryTagPoolHead										= allocatedMemoryTagPool;

		allocatedMemoryTagPool												+= 1;

		for (UINTN i = 1; i < EFI_PAGE_SIZE / sizeof(MEMORY_TAG); i ++, allocatedMemoryTagPool ++)
		{
			allocatedMemoryTagPool->Next									= MmpNextFreeMemoryTag;
			MmpNextFreeMemoryTag											= allocatedMemoryTagPool;
		}
	}

	MEMORY_TAG* theTag														= MmpNextFreeMemoryTag;
	MmpNextFreeMemoryTag													= theTag->Next;
	memset(theTag, 0, sizeof(MEMORY_TAG));

	return theTag;
}

//
// free memory tag
//
STATIC VOID MmpFreeMemoryTag(MEMORY_TAG* theTag)
{
	theTag->Next															= MmpNextFreeMemoryTag;
	MmpNextFreeMemoryTag													= theTag;
}

//
// find memory tag
//
STATIC MEMORY_TAG* MmpFindMemoryTag(VOID* virtualAddress, UINT64 physicalAddress, MEMORY_TAG** prevTag)
{
	if (prevTag)
		*prevTag															= NULL;

	for (MEMORY_TAG* theTag = MmpAllocatedMemoryTagHead; theTag; theTag = theTag->Next)
	{
		if (theTag->VirtualAddress == virtualAddress && theTag->PhysicalAddress == physicalAddress)
			return theTag;

		if (prevTag)
			*prevTag														= theTag;
	}

	return NULL;
}

//
// insert memory tag
//
STATIC VOID MmpInsertMemoryTag(VOID* virtualAddress, UINT64 bufferLength, UINT64 physicalAddress)
{
	MEMORY_TAG* theTag														= MmpGetMemoryTag();

	if (!theTag)
		return;

	theTag->BufferLength													= bufferLength;
	theTag->PhysicalAddress													= physicalAddress;
	theTag->VirtualAddress													= virtualAddress;
	theTag->Next															= MmpAllocatedMemoryTagHead;
	MmpAllocatedMemoryTagHead												= theTag;
}

//
// remove memory tag
//
STATIC VOID MmpRemoveMemoryTag(VOID* virtualAddress, UINT64 physicalAddress)
{
	MEMORY_TAG* prevTag														= NULL;
	MEMORY_TAG* theTag														= MmpFindMemoryTag(virtualAddress, physicalAddress, &prevTag);

	if (!theTag)
		return;

	if (prevTag)
		prevTag->Next														= theTag->Next;
	else
		MmpAllocatedMemoryTagHead											= theTag->Next;

	MmpFreeMemoryTag(theTag);
}

//
// malloc
//
VOID* MmAllocatePool(UINTN bufferLength)
{
	if (!EfiBootServices)
		return NULL;

	VOID* allocatedBuffer													= NULL;

	if (EFI_ERROR(EfiBootServices->AllocatePool(EfiBootServicesData, bufferLength, &allocatedBuffer)))
		return NULL;

	MmpInsertMemoryTag(allocatedBuffer, bufferLength, 0);

	return allocatedBuffer;
}

//
// free
//
VOID MmFreePool(VOID* freeBuffer)
{
	if (!freeBuffer)
		return;

	if (EfiBootServices)
		EfiBootServices->FreePool(freeBuffer);

	MmpRemoveMemoryTag(freeBuffer, 0);
}

//
// allocate page
//
VOID* MmAllocatePages(EFI_ALLOCATE_TYPE allocateType, EFI_MEMORY_TYPE memoryType, UINTN pagesCount, UINT64* physicalAddress)
{
	if (!EfiBootServices)
		return NULL;

	if (EFI_ERROR(EfiBootServices->AllocatePages(allocateType, memoryType, pagesCount, physicalAddress)))
		return NULL;

	MmpInsertMemoryTag(NULL, EFI_PAGES_TO_SIZE((UINT64)(pagesCount)), *physicalAddress);

	return ArchConvertAddressToPointer(*physicalAddress, VOID*);
}

//
// free pages
//
VOID MmFreePages(UINT64 phyAddress)
{
	MEMORY_TAG* memoryTag													= MmpFindMemoryTag(NULL, phyAddress, NULL);
	if (!memoryTag)
		return;

	if (EfiBootServices)
		EfiBootServices->FreePages(phyAddress, (UINTN)(EFI_SIZE_TO_PAGES(memoryTag->BufferLength)));

	MmpRemoveMemoryTag(NULL, phyAddress);
}

//
// initialize
//
EFI_STATUS MmInitialize()
{
	return EFI_SUCCESS;
}

//
// finalize
//
VOID MmFinalize()
{
	if (EfiBootServices)
	{
		for (MEMORY_TAG* allocatedMemoryTag = MmpAllocatedMemoryTagHead; allocatedMemoryTag; allocatedMemoryTag = allocatedMemoryTag->Next)
		{
			if (allocatedMemoryTag->PhysicalAddress)
				EfiBootServices->FreePages(allocatedMemoryTag->PhysicalAddress, (UINTN)(EFI_SIZE_TO_PAGES(allocatedMemoryTag->BufferLength)));
			else if (allocatedMemoryTag->VirtualAddress)
				EfiBootServices->FreePool(allocatedMemoryTag->VirtualAddress);
		}

		for (MEMORY_TAG* allocatedMemoryTagPool = MmpAllocatedMemoryTagPoolHead; allocatedMemoryTagPool; allocatedMemoryTagPool = allocatedMemoryTagPool->Next)
			EfiBootServices->FreePool(allocatedMemoryTagPool->VirtualAddress);
	}

	MmpAllocatedMemoryTagHead												= NULL;
	MmpAllocatedMemoryTagPoolHead											= NULL;
	MmpNextFreeMemoryTag													= NULL;
}

//
// allocate kernel memory
//
UINT64 MmAllocateKernelMemory(UINTN* bufferLength, UINT64* virtualAddress)
{
	if (!bufferLength || !EfiBootServices)
		return 0;

	UINTN pagesCount														= EFI_SIZE_TO_PAGES(*bufferLength);
	UINT64 kernelVirtualAddress												= 0;

	if (!virtualAddress || *virtualAddress == 0)
	{
		if (MmpKernelVirtualBegin == -1)
			return 0;

		kernelVirtualAddress												= MmpKernelVirtualEnd;
	}
	else
	{
		kernelVirtualAddress												= *virtualAddress;
	}

	UINT64 physicalAddress													= LdrStaticVirtualToPhysical(kernelVirtualAddress);

	if (!MmAllocatePages(AllocateAddress, EfiLoaderData, pagesCount, &physicalAddress))
		return 0;

	*bufferLength															= pagesCount << EFI_PAGE_SHIFT;

	if (kernelVirtualAddress < MmpKernelVirtualBegin)
		MmpKernelVirtualBegin												= kernelVirtualAddress;

	if (kernelVirtualAddress + *bufferLength > MmpKernelVirtualEnd)
		MmpKernelVirtualEnd													= kernelVirtualAddress + *bufferLength;

	if (physicalAddress < MmpKernelPhysicalBegin)
		MmpKernelPhysicalBegin												= physicalAddress;

	if (physicalAddress + *bufferLength > MmpKernelPhysicalEnd)
		MmpKernelPhysicalEnd												= physicalAddress + *bufferLength;

	if (virtualAddress)
		*virtualAddress														= kernelVirtualAddress;

	return physicalAddress;
}

//
// free kernel memory
//
VOID MmFreeKernelMemory(UINT64 virtualAddress, UINT64 physicalAddress)
{
	MEMORY_TAG* theTag														= MmpFindMemoryTag(NULL, physicalAddress, NULL);

	if (!theTag)
		return;

	if (virtualAddress + theTag->BufferLength != MmpKernelVirtualEnd && physicalAddress + theTag->BufferLength != MmpKernelPhysicalEnd)
		return;

	MmpKernelPhysicalEnd													-= theTag->BufferLength;
	MmpKernelVirtualEnd														-= theTag->BufferLength;

	MmFreePages(physicalAddress);
}

//
// allocate loader data memory
//
UINT64 MmAllocateLoaderData(UINTN* bufferLength, UINT64* virtualAddress)
{
	UINT64 physicalAddress													= LdrStaticVirtualToPhysical(*virtualAddress);
	UINTN pagesCount														= EFI_SIZE_TO_PAGES(*bufferLength);

	if (!MmAllocatePages(AllocateAddress, EfiLoaderData, pagesCount, &physicalAddress))
		return 0;

	*bufferLength															= pagesCount << EFI_PAGE_SHIFT;

	return physicalAddress;
}

//
// free loader data memory
//
VOID MmFreeLoaderData(UINT64 virtualAddress, UINT64 physicalAddress)
{
	MmFreePages(physicalAddress);
}

//
// get kernel memory range
//
VOID MmGetKernelPhysicalRange(UINT64* lowerAddress, UINT64* upperAddress)
{
	*lowerAddress															= MmpKernelPhysicalBegin;
	*upperAddress															= MmpKernelPhysicalEnd;
}

//
// get kernel virtual start
//
UINT64 MmGetKernelVirtualStart()
{
	return MmpKernelVirtualBegin;
}

#ifndef    GUARD_MAX
#define    GUARD_MAX 8
#endif  /* GUARD_MAX */


long __stack_chk_guard[GUARD_MAX] = {0, 0, 0, 0, 0, 0, 0, 0};

void
__stack_chk_fail()
{
}

