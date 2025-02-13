//********************************************************************
//	created:	14:9:2012   20:30
//	filename: 	Hibernate.cpp
//	author:		tiamo
//	purpose:	hibernate
//********************************************************************

#include "StdAfx.h"
#include "Hibernate.h"

#include "../rijndael/aes.h"

#define HIBERNATE_HEADER_SIGNATURE											0x73696d65
#define HIBERNATE_HANDOFF_TYPE_END											0x686f0000
#define HIBERNATE_HANDOFF_TYPE_GRAPHICS										0x686f0001
#define HIBERNATE_HANDOFF_TYPE_CRYPT_VARS									0x686f0002
#define HIBERNATE_HANDOFF_TYPE_MEMORY_MAP									0x686f0003
#define HIBERNATE_HANDOFF_TYPE_DEVICE_TREE									0x686f0004
#define HIBERNATE_HANDOFF_TYPE_DEVICE_PROPERTIES							0x686f0005
#define HIBERNATE_HANDOFF_TYPE_KEY_STORE									0x686f0006

//
// rtc hibernate vars
//
#if defined(_MSC_VER)
#include <pshpack1.h>
#endif

typedef struct _RTC_HIBERNATE_VARS
{
	//
	// signature
	//
	UINT32																	Signature;

	//
	// revision
	//
	UINT32																	Revision;

	//
	// boot signature
	//
	UINT8																	BootSignature[20];

	//
	// wired crypt key
	//
	UINT8																	WiredCryptKey[16];
}RTC_HIBERNATE_VARS;

//
// file extent
//
typedef struct _POLLED_FILE_EXTENT
{
	//
	// offset
	//
	UINT64																	Offset;

	//
	// length
	//
	UINT64																	Length;
}POLLED_FILE_EXTENT;

//
// hibernate image header
//
typedef struct _HIBERNATE_IMAGE_HEADER
{
	//
	// image size
	//
	UINT64																	ImageSize;

	//
	// image1 size
	//
	UINT64																	Image1Size;

	//
	// restore1 code physical page
	//
	UINT32																	Restore1CodePhysicalPage;

	//
	// padding
	//
	UINT32																	Reserved1;

	//
	// restore1 code virtual address
	//
	UINT64																	Restore1CodeVirtual;

	//
	// restore1 page count
	//
	UINT32																	Restore1PageCount;

	//
	// restore1 code offset
	//
	UINT32																	Restore1CodeOffset;

	//
	// restore1 stack offset
	//
	UINT32																	Restore1StackOffset;

	//
	// page count
	//
	UINT32																	PageCount;

	//
	// bitmap size
	//
	UINT32																	BitmapSize;

	//
	// restore1 sum
	//
	UINT32																	Restore1Sum;

	//
	// image1 sum
	//
	UINT32																	Image1Sum;

	//
	// image2 sum
	//
	UINT32																	Image2Sum;

	//
	// actual restore1 sum
	//
	UINT32																	ActualRestore1Sum;

	//
	// actual image1 sum
	//
	UINT32																	ActualImage1Sum;

	//
	// actual image2 sum
	//
	UINT32																	ActualImage2Sum;

	//
	// actual uncompressed pages
	//
	UINT32																	ActualUncompressedPages;

	//
	// conflict count
	//
	UINT32																	ConflictCount;

	//
	// next free
	//
	UINT32																	NextFree;

	//
	// signature
	//
	UINT32																	Signature;

	//
	// processor flags
	//
	UINT32																	ProcessorFlags;

	//
	// runtime pages
	//
	UINT32																	RuntimePages;

	//
	// runtime page count
	//
	UINT32																	RuntimePageCount;

	//
	// runtime virtual pages
	//
	UINT64																	RuntimeVirtualPages;

	//
	// performance data start
	//
	UINT32																	PerformanceDataStart;

	//
	// performance data size
	//
	UINT32																	PerformanceDataSize;

	//
	// encrypt start
	//
	UINT64																	EncryptStart;

	//
	// machine signature
	//
	UINT64																	MachineSignature;

	//
	// preview size
	//
	UINT32																	PreviewSize;

	//
	// preview page list size
	//
	UINT32																	PreviewPageListSize;

	//
	// diag
	//
	UINT32																	Diag[4];

	//
	// handoff pages
	//
	UINT32																	HandoffPages;

	//
	// handoff page count
	//
	UINT32																	HandoffPageCount;

	//
	// system table offset
	//
	UINT32																	SystemTableOffset;

	//
	// debug flags
	//
	UINT32																	DebugFlags;

	//
	// options
	//
	UINT32																	Options;

	//
	// sleep time
	//
	UINT32																	SleepTime;

	//
	// reserved
	//
	UINT32																	Reserved2[69];

	//
	// encrypt end
	//
	UINT64																	EncryptEnd;

	//
	// device base
	//
	UINT64																	DeviceBase;

	//
	// file extent map size
	//
	UINT32																	FileExtentMapSize;

	//
	// file extent map
	//
	POLLED_FILE_EXTENT														FileExtentMap[2];
}HIBERNATE_IMAGE_HEADER;

//
// handoff
//
typedef struct _HIBERNATE_HANDOFF
{
	//
	// type
	//
	UINT32																	Type;

	//
	// size
	//
	UINT32																	Size;
}HIBERNATE_HANDOFF;

//
// graphics handoff
//
typedef struct _HIBERNATE_HANDOFF_GRAPHICS
{
	//
	// base address
	//
	UINT32																	BaseAddress;

	//
	// efi config restore status
	//
	INT32																	GfxRestoreStatus;

	//
	// bytes per row
	//
	UINT32																	BytesPerRow;

	//
	// width
	//
	UINT32																	HorzRes;

	//
	// height
	//
	UINT32																	VertRes;

	//
	// color depth
	//
	UINT32																	ColorDepth;

	//
	// progress
	//
	UINT8																	ProgressSaveUnder[HIBERNATE_PROGRESS_COUNT][HIBERNATE_PROGRESS_SAVE_UNDER_SIZE];

	//
	// padding
	//
	UINT8																	Reserved[2];
}HIBERNATE_HANDOFF_GRAPHICS;

//
// crypt wake vars
//
typedef struct _HIBERNATE_HANDOFF_CRYPT_VARS
{
	//
	// init vector
	//
	UINT8																	InitVector[16];
}HIBERNATE_HANDOFF_CRYPT_VARS;

#if defined(_MSC_VER)
#include <poppack.h>
#endif

STATIC RTC_HIBERNATE_VARS HbpHibernateVars									= {0};
STATIC UINT8* HbpBootImageKey												= NULL;
STATIC UINTN HbpBootImageKeyLength											= 0;
STATIC UINT32 HbpCurrentTime												= 0;
STATIC EFI_DISK_IO_PROTOCOL* HbpDiskIoProtocol								= NULL;
STATIC EFI_BLOCK_IO_PROTOCOL* HbpBlockIoProtocol							= NULL;
STATIC UINT64 HbpDiskOffset													= 0;
STATIC APPLE_DISK_IO_PROTOCOL* HbpAppleDiskIoProtocol						= NULL;
STATIC APPLE_SMC_PROTOCOL* HbpAppleSMCProtocol								= NULL;

//
// continue resume from hibernate
//
VOID HbpContinueResumeFromHibernate(UINT8 CONST* imageKey, UINTN imageKeyLength, BOOLEAN resumeFromCoreStorage)
{
#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// read image head
		//
		STATIC HIBERNATE_IMAGE_HEADER localHeader							= {0};
		if(EFI_ERROR(HbpDiskIoProtocol->ReadDisk(HbpDiskIoProtocol, HbpBlockIoProtocol->Media->MediaId, HbpDiskOffset, sizeof(localHeader), &localHeader)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return;
#endif

		//
		// check signature
		//
		if(localHeader.Signature != HIBERNATE_HEADER_SIGNATURE)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// check machine signature
		//
		UINT32 machineSignature												= 0;
		if(!EFI_ERROR(AcpiGetMachineSignature(&machineSignature)) && machineSignature != localHeader.MachineSignature)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// get options
		//
		UINT32 diag3														= 0;
		UINT32 options														= 0;
		UINT8 keyMSWr														= 0;
		UINT16 keyMSPS														= 0;
		if(HbpAppleSMCProtocol && !EFI_ERROR(HbpAppleSMCProtocol->ReadData(HbpAppleSMCProtocol, 0x4d535772, sizeof(keyMSWr), &keyMSWr)))
		{
			diag3															= keyMSWr;
			if(keyMSWr != 25 && keyMSWr != 112 && keyMSWr != 114 && !resumeFromCoreStorage && !EFI_ERROR(HbpAppleSMCProtocol->ReadData(HbpAppleSMCProtocol, 0x4d535053, sizeof(keyMSPS), &keyMSPS)) && keyMSPS <= 2)
				options														= 8;
		}

		//
		// get memory map
		//
		UINTN memoryMapSize													= 0;
		EFI_MEMORY_DESCRIPTOR* memoryMap									= NULL;
		UINTN memoryMapKey													= 0;
		UINTN descriptorSize												= 0;
		UINT32 descriptorVersion											= 0;
		if(EFI_ERROR(MmGetMemoryMap(&memoryMapSize, &memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// remove non-runtime pages
		//
		if(EFI_ERROR(MmRemoveNonRuntimeDescriptors(memoryMap, &memoryMapSize, descriptorSize)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// allocate restore1 pages
		//
		UINT64 restore1Size													= EFI_PAGES_TO_SIZE((UINT64)(localHeader.Restore1PageCount));
		UINT64 restore1PhysicalAddress										= EFI_PAGES_TO_SIZE((UINT64)(localHeader.Restore1CodePhysicalPage));
		VOID* restore1Code													= MmAllocatePages(AllocateAddress, EfiLoaderData, localHeader.Restore1PageCount, &restore1PhysicalAddress);
		if(!restore1Code)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// allocate runtime pages
		//
		UINT64 runtimePagesPhysicalAddress									= EFI_PAGES_TO_SIZE((UINT64)(localHeader.RuntimePages));
		VOID* runtimePages													= MmAllocatePages(AllocateAddress, EfiLoaderData, localHeader.RuntimePageCount, &runtimePagesPhysicalAddress);
		if(!runtimePages)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// allocate image1 buffer
		//
		UINT64 image1Size													= localHeader.Image1Size;
		UINT64 image1PhysicalAddress										= 0;
		VOID* image1Buffer													= MmAllocatePages(AllocateAnyPages, EfiLoaderData, (UINTN)(EFI_SIZE_TO_PAGES(image1Size)), &image1PhysicalAddress);
		if(!image1Buffer)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// copy image header
		//
		memcpy(image1Buffer, &localHeader, sizeof(localHeader));
		HIBERNATE_IMAGE_HEADER* imageHeader									= (HIBERNATE_IMAGE_HEADER*)(image1Buffer);

		//
		// fix sleep time
		//
		if(HbpCurrentTime < imageHeader->SleepTime)
			HbpCurrentTime													= 0;

		//
		// setup
		//
		UINT8 progressSaveUnder[HIBERNATE_PROGRESS_COUNT][HIBERNATE_PROGRESS_SAVE_UNDER_SIZE];
		UINT8 initVector[16]												= {0};
		aes_decrypt_ctx aesContext											= {{0}};
		UINT64 imageTotalLength												= image1Size + 1 * 1024 * 1024;
		UINT64 leftLength													= image1Size - sizeof(HIBERNATE_IMAGE_HEADER);
		UINT64 readOffset													= imageHeader->FileExtentMap[0].Offset + sizeof(HIBERNATE_IMAGE_HEADER);
		UINT64 extentLength													= imageHeader->FileExtentMap[0].Length - sizeof(HIBERNATE_IMAGE_HEADER);
		POLLED_FILE_EXTENT* nextExtent										= imageHeader->FileExtentMap + 1;
		VOID* readBuffer													= imageHeader + 1;
		BOOLEAN hasPreview													= imageHeader->PreviewSize ? TRUE : FALSE;
		BOOLEAN showProgress												= imageHeader->Options & 4 ? TRUE : FALSE;		// kIOHibernateOptionProgress = 4
		BOOLEAN colorMode													= imageHeader->Options & 2 ? TRUE : FALSE;		// kIOHibernateOptionColor = 2
		INT32 gfxRestoreStatus												= -1;
		VOID* startOfPreviewBuffer											= hasPreview ? readBuffer : NULL;
		UINT64 previewTotalSize												= restore1Size + imageHeader->FileExtentMapSize - sizeof(POLLED_FILE_EXTENT) * 2 + imageHeader->PreviewSize;
		previewTotalSize													= (UINT32)(hasPreview ? (previewTotalSize + HbpBlockIoProtocol->Media->BlockSize - 1) & ~(HbpBlockIoProtocol->Media->BlockSize - 1) : 0);
		VOID* endOfPreviewBuffer											= Add2Ptr(startOfPreviewBuffer, previewTotalSize, VOID*);
		VOID* startOfEncryptBuffer											= NULL;
		VOID* endOfEncryptBuffer											= NULL;

		//
		// draw logo
		//
		if(!hasPreview)
			CsDrawPreview(NULL, 1, showProgress ? progressSaveUnder : NULL, colorMode, resumeFromCoreStorage, &gfxRestoreStatus);

		//
		// setup aes
		//
		if(imageHeader->EncryptStart)
		{
			if(!imageKeyLength || !imageKey)
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                return;
#endif

			//
			// setup key
			//
			EFI_TPL savedTPL												= HbpAppleDiskIoProtocol ? 0 : EfiBootServices->RaiseTPL(EFI_TPL_HIGH_LEVEL);
			aes_decrypt_key(imageKey, (INT32)(imageKeyLength), &aesContext);
			if(!HbpAppleDiskIoProtocol)
				EfiBootServices->RestoreTPL(savedTPL);

			//
			// setup init vector
			//
			STATIC UINT8 firstInitVector[0x10]								= {0xa3, 0x63, 0x65, 0xa9, 0x0b, 0x71, 0x7b, 0x1c, 0xdf, 0x9e, 0x5f, 0x32, 0xd7, 0x61, 0x63, 0xda};
			memcpy(initVector, firstInitVector, sizeof(initVector));

			//
			// setup pointer
			//
			startOfEncryptBuffer											= Add2Ptr(image1Buffer, imageHeader->EncryptStart, VOID*);
			endOfEncryptBuffer												= Add2Ptr(image1Buffer, imageHeader->EncryptEnd, VOID*);
		}

		//
		// loop read/decrypt
		//
		BOOLEAN readSomething												= FALSE;
		BOOLEAN needRead													= TRUE;
		BOOLEAN needFlush													= TRUE;
		BOOLEAN useDiskIoProtocol											= !HbpAppleDiskIoProtocol;
		UINT64 totalReadLength												= 0;
		UINT64 lengthThisRun												= 0;
		UINTN currentBlob													= 0;
		UINTN prevBlob														= 0;
		UINT32 screenLockState												= 1;		// kIOScreenLockNoLock
		while(startOfEncryptBuffer || leftLength)
		{
			//
			// update read info
			//
			if(readSomething && (!needFlush || !EFI_ERROR(HbpAppleDiskIoProtocol->Flush(HbpAppleDiskIoProtocol))))
			{
				readSomething												= FALSE;
				leftLength													-= lengthThisRun;
				extentLength												-= lengthThisRun;
				readBuffer													= Add2Ptr(readBuffer, lengthThisRun, VOID*);
				readOffset													+= lengthThisRun;
				totalReadLength												+= lengthThisRun;
				needRead													= leftLength ? TRUE : FALSE;
				needFlush													= TRUE;
				if(!hasPreview)
					currentBlob												= (UINTN)((totalReadLength * HIBERNATE_PROGRESS_COUNT) / imageTotalLength);
			}

			//
			// read in 1MB block
			//
			if(needRead)
			{
				//
				// goto the next extent
				//
				if(!extentLength)
				{
					extentLength											= nextExtent->Length;
					readOffset												= nextExtent->Offset;
					nextExtent												+= 1;
				}

				//
				// compute read length
				//
				needRead													= FALSE;
				lengthThisRun												= leftLength > extentLength ? extentLength : leftLength;
				if(readBuffer < endOfPreviewBuffer && lengthThisRun > ArchConvertPointerToAddress(endOfPreviewBuffer) - ArchConvertPointerToAddress(readBuffer))
					lengthThisRun											= ArchConvertPointerToAddress(endOfPreviewBuffer) - ArchConvertPointerToAddress(readBuffer);

				if(lengthThisRun > 1 * 1024 * 1024)
					lengthThisRun											= 1 * 1024 * 1024;

				//
				// read it
				//
				if(useDiskIoProtocol)
				{
					if(EFI_ERROR(HbpDiskIoProtocol->ReadDisk(HbpDiskIoProtocol, HbpBlockIoProtocol->Media->MediaId, readOffset, (UINTN)(lengthThisRun), readBuffer)))
#if defined(_MSC_VER)
                        try_leave(NOTHING);
#else
                        return;
#endif
					readSomething											= TRUE;
					needFlush												= FALSE;
				}
				else
				{
					readSomething											= TRUE;
					UINT64 offset											= imageHeader->DeviceBase + readOffset;
					if((offset % HbpBlockIoProtocol->Media->BlockSize) || (lengthThisRun & (HbpBlockIoProtocol->Media->BlockSize - 1)))
#if defined(_MSC_VER)
                        try_leave(NOTHING);
#else
                        return;
#endif

					if(EFI_ERROR(HbpAppleDiskIoProtocol->ReadDisk(HbpAppleDiskIoProtocol, HbpBlockIoProtocol->Media->MediaId, offset / HbpBlockIoProtocol->Media->BlockSize, (UINTN)(lengthThisRun), readBuffer)))
#if defined(_MSC_VER)
						try_leave(NOTHING);
#else
                        return;
#endif
				}
			}

			//
			// update progress
			//
			if(showProgress && prevBlob != currentBlob)
			{
				CsUpdateProgress(progressSaveUnder, prevBlob, currentBlob);
				prevBlob													= currentBlob;
			}

			//
			// decrypt in 128k block
			//
			if(startOfEncryptBuffer && readBuffer > startOfEncryptBuffer)
			{
				//
				// compute decrypt length
				//
				UINT64 encryptLength										= ArchConvertPointerToAddress(readBuffer > endOfEncryptBuffer ? endOfEncryptBuffer : readBuffer) - ArchConvertPointerToAddress(startOfEncryptBuffer);
				if(encryptLength > 128 * 1024)
					encryptLength											= 128 * 1024;

				//
				// setup init vector
				//
				UINT8 tempKey[16]											= {0};
				memcpy(tempKey, initVector, sizeof(tempKey));
				memcpy(initVector, Add2Ptr(startOfEncryptBuffer, encryptLength - 0x10, VOID*), sizeof(initVector));

				//
				// decrypt
				//
				EFI_TPL savedTPL											= HbpAppleDiskIoProtocol ? 0 : EfiBootServices->RaiseTPL(EFI_TPL_HIGH_LEVEL);
				aes_cbc_decrypt((unsigned char*)(startOfEncryptBuffer), (unsigned char*)(startOfEncryptBuffer), (INT32)(encryptLength), tempKey, &aesContext);
				if(!HbpAppleDiskIoProtocol)
					EfiBootServices->RestoreTPL(savedTPL);

				//
				// check preview buffer
				//
				startOfEncryptBuffer										= Add2Ptr(startOfEncryptBuffer, encryptLength, VOID*);
				if(hasPreview && startOfEncryptBuffer >= endOfPreviewBuffer)
				{
					//
					// compute lock state
					//
					HIBERNATE_PREVIEW* previewBuffer						= Add2Ptr(imageHeader->FileExtentMap, restore1Size + imageHeader->FileExtentMapSize + imageHeader->PreviewPageListSize, HIBERNATE_PREVIEW*);
					if(resumeFromCoreStorage)
						screenLockState										= 4;		// kIOScreenLockFileVaultDialog
					else if(previewBuffer->LockTime)
						screenLockState										= HbpCurrentTime && HbpCurrentTime <= previewBuffer->LockTime ? 2 : 3;	// kIOScreenLockLocked = 3, kIOScreenLockUnlocked = 2

					//
					// draw preview
					//
					hasPreview												= FALSE;
					if(!options)
						CsDrawPreview(previewBuffer, screenLockState != 2 && screenLockState != 1 ? 1 : 0, showProgress ? progressSaveUnder : NULL, colorMode, resumeFromCoreStorage, &gfxRestoreStatus);
				}

				//
				// check end of encrypt buffer
				//
				if(startOfEncryptBuffer >= endOfEncryptBuffer)
					startOfEncryptBuffer									= NULL;
			}
		}

		//
		// compute handoff size
		//
		UINTN handoffSize													= sizeof(HIBERNATE_HANDOFF_GRAPHICS) + sizeof(HIBERNATE_HANDOFF_CRYPT_VARS) + sizeof(HIBERNATE_HANDOFF) * 5;

		//
		// count FileVault2 key store length
		//
		UINTN keyStoreDataSize												= 0;
		if(BlTestBootMode(BOOT_MODE_HAS_FILE_VAULT2_CONFIG))
		{
			FvSetupDeviceTree(NULL, &keyStoreDataSize, TRUE);
			handoffSize														+= keyStoreDataSize + sizeof(HIBERNATE_HANDOFF);
		}
		
		//
		// add screen lock state
		//
		DEVICE_TREE_NODE* chosenNode										= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);
		if(chosenNode)
			DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("IOScreenLockState"), &screenLockState, sizeof(screenLockState), TRUE);

		//
		// get device tree length
		//
		UINT32 deviceTreeLength												= 0;
		DevTreeFlatten(NULL, &deviceTreeLength);

		//
		// allocate handoff pages
		//
		handoffSize															+= deviceTreeLength + memoryMapSize;
		UINT64 handoffPhysicalAddress										= 0;
		UINTN handoffPageCount												= (handoffSize >> EFI_PAGE_SHIFT) + 1;
		VOID* handoffPages													= MmAllocatePages(AllocateAnyPages, EfiLoaderData, handoffPageCount, &handoffPhysicalAddress);
		if(!handoffPages)
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return;
#endif

		//
		// save handoff page info
		//
		imageHeader->HandoffPageCount										= (UINT32)(handoffPageCount);
		imageHeader->HandoffPages											= (UINT32)(handoffPhysicalAddress >> EFI_PAGE_SHIFT);

		//
		// setup graph handoff
		//
		HIBERNATE_HANDOFF* handoffHeader									= (HIBERNATE_HANDOFF*)(handoffPages);
		handoffHeader->Type													= HIBERNATE_HANDOFF_TYPE_GRAPHICS;
		handoffHeader->Size													= sizeof(HIBERNATE_HANDOFF_GRAPHICS);
		HIBERNATE_HANDOFF_GRAPHICS* graphics								= (HIBERNATE_HANDOFF_GRAPHICS*)(handoffHeader + 1);
		handoffHeader														= (HIBERNATE_HANDOFF*)(graphics + 1);
		if(options)
		{
			memset(graphics, 0, sizeof(HIBERNATE_HANDOFF_GRAPHICS));
		}
		else
		{
			BOOT_VIDEO bootVideo											= {0};
			CsInitializeBootVideo(&bootVideo);
			graphics->BaseAddress											= bootVideo.BaseAddress;
			graphics->BytesPerRow											= bootVideo.BytesPerRow;
			graphics->ColorDepth											= bootVideo.ColorDepth;
			graphics->GfxRestoreStatus										= gfxRestoreStatus;
			graphics->HorzRes												= bootVideo.HorzRes;
			graphics->VertRes												= bootVideo.VertRes;
			options															= bootVideo.BaseAddress ? 0 : 8;		// kIOHibernateOptionDarkWake
			memcpy(graphics->ProgressSaveUnder, progressSaveUnder, sizeof(progressSaveUnder));
		}

		//
		// setup crypt handoff
		//
		handoffHeader->Type													= HIBERNATE_HANDOFF_TYPE_CRYPT_VARS;
		handoffHeader->Size													= sizeof(HIBERNATE_HANDOFF_CRYPT_VARS);
		HIBERNATE_HANDOFF_CRYPT_VARS* cryptVars								= (HIBERNATE_HANDOFF_CRYPT_VARS*)(handoffHeader + 1);
		handoffHeader														= (HIBERNATE_HANDOFF*)(cryptVars + 1);
		memcpy(cryptVars->InitVector, initVector, sizeof(initVector));

		//
		// FileVault2, key store
		//
		if(BlTestBootMode(BOOT_MODE_HAS_FILE_VAULT2_CONFIG))
		{
			handoffHeader->Type												= HIBERNATE_HANDOFF_TYPE_KEY_STORE;
			handoffHeader->Size												= (UINT32)(keyStoreDataSize);
			UINT64 keyStorePhysicalAddress									= ArchConvertPointerToAddress(handoffHeader + 1);
			FvSetupDeviceTree(&keyStorePhysicalAddress, &keyStoreDataSize, FALSE);
			handoffHeader													= Add2Ptr(handoffHeader, keyStoreDataSize, HIBERNATE_HANDOFF*);
		}

		//
		// setup device tree handoff
		//
		handoffHeader->Type													= HIBERNATE_HANDOFF_TYPE_DEVICE_TREE;
		handoffHeader->Size													= deviceTreeLength;
		VOID* deviceTreeBuffer												= (VOID*)(handoffHeader + 1);
		handoffHeader														= Add2Ptr(deviceTreeBuffer, deviceTreeLength, HIBERNATE_HANDOFF*);
		DevTreeFlatten(&deviceTreeBuffer, &deviceTreeLength);

		//
		// setup memory map handoff
		//
		handoffHeader->Type													= HIBERNATE_HANDOFF_TYPE_MEMORY_MAP;
		UINTN oldSize														= memoryMapSize;
		if(EFI_ERROR(MmGetMemoryMap(&memoryMapSize, &memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion)) || EFI_ERROR(MmRemoveNonRuntimeDescriptors(memoryMap, &memoryMapSize, descriptorSize)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif
		if(memoryMapSize > oldSize)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif
		memcpy(handoffHeader + 1, memoryMap, memoryMapSize);

		//
		// Diag[3] ?
		//
		imageHeader->Options												= options;
		imageHeader->Diag[3]												= 0x5300 | diag3;

		//
		// exit boot service
		//
		if(EFI_ERROR(EfiBootServices->ExitBootServices(EfiImageHandle, memoryMapKey)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// convert efi structure
		//
		UINT64 sysTablePhysicalAddress										= ArchConvertPointerToAddress(EfiSystemTable);
		UINT64 rtVirtual													= EFI_PAGES_TO_SIZE(imageHeader->RuntimeVirtualPages);
		memoryMap															= (EFI_MEMORY_DESCRIPTOR*)(handoffHeader + 1);
		if(EFI_ERROR(MmConvertPointers(memoryMap, &memoryMapSize, descriptorSize, descriptorVersion, runtimePagesPhysicalAddress, imageHeader->RuntimePageCount, rtVirtual, &sysTablePhysicalAddress, FALSE, NULL)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return;
#endif

		//
		// clear serives pointer
		//
		EfiBootServices														= NULL;
		EfiRuntimeServices													= EfiSystemTable->RuntimeServices;

		//
		// setup end handoff
		//
		handoffHeader->Size													= (UINT32)(memoryMapSize);
		handoffHeader														= Add2Ptr(handoffHeader + 1, memoryMapSize, HIBERNATE_HANDOFF*);
		handoffHeader->Type													= HIBERNATE_HANDOFF_TYPE_END;
		handoffHeader->Size													= 0;
		imageHeader->SystemTableOffset										= (UINT32)(sysTablePhysicalAddress - runtimePagesPhysicalAddress);

		//
		// compute restore1 sum
		//
		UINT32 restore1Sum													= 0;
		UINT32* dstBuffer													= (UINT32*)(restore1Code);
		UINT32* srcBuffer													= Add2Ptr(imageHeader->FileExtentMap, imageHeader->FileExtentMapSize, UINT32*);
		for(UINTN i = 0; i < imageHeader->Restore1PageCount; i ++, dstBuffer = Add2Ptr(dstBuffer, EFI_PAGE_SIZE, UINT32*), srcBuffer = Add2Ptr(srcBuffer, EFI_PAGE_SIZE, UINT32*))
		{
			restore1Sum														+= srcBuffer[(i + imageHeader->Restore1CodeVirtual) & 0x3ff];
			memcpy(dstBuffer, srcBuffer, EFI_PAGE_SIZE);
		}
		imageHeader->ActualRestore1Sum										= restore1Sum;

		//
		// close debuger
		//
		BdFinalize();

		//
		// transfer to kernel
		//
		ArchStartKernel(Add2Ptr(restore1Code, imageHeader->Restore1CodeOffset, VOID*), ArchConvertAddressToPointer((ArchConvertPointerToAddress(imageHeader) >> EFI_PAGE_SHIFT), VOID*));
#if defined(_MSC_VER)

	}
	__finally
	{
	}
#endif
}

//
// start resume from hiberate
//
BOOLEAN HbStartResumeFromHibernate(UINT8* coreStorageVolumeKeyIdent)
{
	BOOLEAN resumeFromCoreStorage											= FALSE;
	BOOLEAN needReset														= FALSE;
	EFI_DEVICE_PATH_PROTOCOL* bootImagePath									= NULL;
	CHAR8* filePath															= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// skip safe mode
		//
        if(BlTestBootMode(BOOT_MODE_SAFE)) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// read boot-switch-vars
		//
		UINTN dataSize														= sizeof(HbpHibernateVars);
		if(EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-switch-vars"), &AppleNVRAMVariableGuid, NULL, &dataSize, &HbpHibernateVars)))
		{
			//
			// read boot-signature
			//
			dataSize														= sizeof(HbpHibernateVars.BootSignature);
            if(EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-signature"), &AppleNVRAMVariableGuid, NULL, &dataSize, HbpHibernateVars.BootSignature))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if(bootImagePath)
                    MmFreePool(bootImagePath);

                if(filePath)
                    MmFreePool(filePath);

                if(needReset)
                    EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

                return resumeFromCoreStorage;
#endif
            }

			EfiRuntimeServices->SetVariable(CHAR16_STRING(L"boot-signature"), &AppleNVRAMVariableGuid, 0, 0, NULL);

			//
			// read boot-image-key
			//
			dataSize														= sizeof(HbpHibernateVars.WiredCryptKey);
            if(EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-image-key"), &AppleNVRAMVariableGuid, NULL, &dataSize, HbpHibernateVars.WiredCryptKey))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if(bootImagePath)
                    MmFreePool(bootImagePath);

                if(filePath)
                    MmFreePool(filePath);

                if(needReset)
                    EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

                return resumeFromCoreStorage;
#endif
            }

			EfiRuntimeServices->SetVariable(CHAR16_STRING(L"boot-image-key"), &AppleNVRAMVariableGuid, 0, 0, NULL);

			//
			// setup default value
			//
			HbpHibernateVars.Signature										= 0x4c504141;
			HbpHibernateVars.Revision										= 1;
		}
		else
		{
			EfiRuntimeServices->SetVariable(CHAR16_STRING(L"boot-switch-vars"), &AppleNVRAMVariableGuid, 0, 0, NULL);
		}

		//
		// save boot image key
		//
		needReset															= TRUE;
		HbpBootImageKeyLength												= sizeof(HbpHibernateVars.WiredCryptKey);
		HbpBootImageKey														= (UINT8*)(MmAllocatePool(HbpBootImageKeyLength));
        if(!HbpBootImageKey) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		memcpy(HbpBootImageKey, HbpHibernateVars.WiredCryptKey, sizeof(HbpHibernateVars.WiredCryptKey));

		//
		// get current time
		//
		HbpCurrentTime														= BlGetCurrentUnixTime();

		//
		// get boot image size
		//
		dataSize															= 0;
        if(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-image"), &AppleNVRAMVariableGuid, NULL, &dataSize, NULL) != EFI_BUFFER_TOO_SMALL || !dataSize) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// allocate boot image buffer
		//
		bootImagePath														= (EFI_DEVICE_PATH_PROTOCOL*)(MmAllocatePool(dataSize));
        if(!bootImagePath) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// read boot image
		//
        if(EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-image"), &AppleNVRAMVariableGuid, NULL, &dataSize, bootImagePath))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// locate block io protocol
		//
		EFI_DEVICE_PATH_PROTOCOL* blockIoDevicePath							= bootImagePath;
		EFI_HANDLE deviceHandle												= NULL;
        if(EFI_ERROR(EfiBootServices->LocateDevicePath(&EfiBlockIoProtocolGuid, &blockIoDevicePath, &deviceHandle))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// get disk io protocol
		//
        if(EFI_ERROR(EfiBootServices->HandleProtocol(deviceHandle, &EfiDiskIoProtocolGuid, (VOID**)(&HbpDiskIoProtocol)))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// get block io protocol
		//
        if(EFI_ERROR(EfiBootServices->HandleProtocol(deviceHandle, &EfiBlockIoProtocolGuid, (VOID**)(&HbpBlockIoProtocol)))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// get apple disk io protocol
		//
		EFI_DEVICE_PATH_PROTOCOL* appleDiskIoDevicePath						= bootImagePath;
		deviceHandle														= NULL;
		HbpAppleDiskIoProtocol												= NULL;
		if(!EFI_ERROR(EfiBootServices->LocateDevicePath(&AppleDiskIoProtocolGuid, &appleDiskIoDevicePath, &deviceHandle)))
		{
			if(EFI_ERROR(EfiBootServices->HandleProtocol(deviceHandle, &AppleDiskIoProtocolGuid, (VOID**)(&HbpAppleDiskIoProtocol))))
				HbpAppleDiskIoProtocol										= NULL;
		}

		//
		// get apple smc protocol
		//
		if(EFI_ERROR(EfiBootServices->LocateProtocol(&AppleSMCProtocolGuid, NULL, (VOID**)(&HbpAppleSMCProtocol))))
			HbpAppleSMCProtocol												= NULL;

		//
		// get file path node
		//
		FILEPATH_DEVICE_PATH* filePathNode									= _CR(DevPathGetNode(bootImagePath, MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP), FILEPATH_DEVICE_PATH, Header);
        if(!filePathNode) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// get file path
		//
		filePath															= BlAllocateUtf8FromUnicode(filePathNode->PathName, (UINTN)(-1));
        if(!filePath) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(bootImagePath)
                MmFreePool(bootImagePath);

            if(filePath)
                MmFreePool(filePath);

            if(needReset)
                EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

            return resumeFromCoreStorage;
#endif
        }

		//
		// get disk offset, hibernate key
		//
		CHAR8 CONST* nextString												= strtoul64_base16(filePath, &HbpDiskOffset);
		if(nextString && *nextString == ':' && BlUUIDStringToBuffer(nextString + 1, coreStorageVolumeKeyIdent))
		{
			resumeFromCoreStorage											= TRUE;
			UINT8 tempKey[0x20]												= {0};
			if(HbpAppleSMCProtocol && !EFI_ERROR(HbpAppleSMCProtocol->ReadData(HbpAppleSMCProtocol, 0x48424B50, sizeof(tempKey), tempKey)))
			{
				for(UINTN i = 0; i < ARRAYSIZE(tempKey) && i < HbpBootImageKeyLength; i ++)
				{
					HbpBootImageKey[i]										^= tempKey[i];
					if(tempKey[0])
						resumeFromCoreStorage								= FALSE;
				}
			}

            if(resumeFromCoreStorage) {
#if defined(_MSC_VER)
                try_leave(needReset = FALSE);
#else
                needReset = FALSE;

                if(bootImagePath)
                    MmFreePool(bootImagePath);

                if(filePath)
                    MmFreePool(filePath);

                if(needReset)
                    EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);

                return resumeFromCoreStorage;
#endif
            }
		}

		//
		// continue resume from hibernate
		//
		HbpContinueResumeFromHibernate(HbpBootImageKey, HbpBootImageKeyLength, FALSE);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(bootImagePath)
			MmFreePool(bootImagePath);

		if(filePath)
			MmFreePool(filePath);

		if(needReset)
			EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
#if defined(_MSC_VER)
    }
#endif

	return resumeFromCoreStorage;
}

//
// continue resume
//
VOID HbContinueResumeFromHibernate(UINT8* coreStorageVolumeKey, UINTN coreStorageVolumeKeyLength)
{
	if(coreStorageVolumeKey && HbpBootImageKeyLength == coreStorageVolumeKeyLength && coreStorageVolumeKeyLength)
	{
		for(UINTN i = 0 ; i < coreStorageVolumeKeyLength ; i ++)
			coreStorageVolumeKey[i]											^= HbpBootImageKey[i];

		HbpContinueResumeFromHibernate(coreStorageVolumeKey, coreStorageVolumeKeyLength, TRUE);
	}

	EfiRuntimeServices->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
}
