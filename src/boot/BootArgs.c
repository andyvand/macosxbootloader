//********************************************************************
//	created:	8:11:2009   16:38
//	filename: 	BootArgs.cpp
//	author:		tiamo
//	purpose:	boot arg
//********************************************************************

#include "StdAfx.h"

//
// ram dmg extent info
//
#if defined(_MSC_VER)
#include <pshpack1.h>
#endif

typedef struct _RAM_DMG_EXTENT_INFO
{
	//
	// start
	//
	UINT64																	Start;

	//
	// length
	//
	UINT64																	Length;
} RAM_DMG_EXTENT_INFO;

//
// ram dmg header
//
typedef struct _RAM_DMG_HEADER
{
	//
	// signature
	//
	UINT64																	Signature;

	//
	// version
	//
	UINT32																	Version;

	//
	// extent count
	//
	UINT32																	ExtentCount;

	//
	// extent
	//
	RAM_DMG_EXTENT_INFO														ExtentInfo[0xfe];

	//
	// padding
	//
	UINT64																	Reserved;

	//
	// signature 2
	//
	UINT64																	Signature2;
} RAM_DMG_HEADER;

#if defined(_MSC_VER)
#include <poppack.h>
#endif /* _MSC_VER */

//
// global
//
STATIC DEVICE_TREE_NODE* BlpMemoryMapNode									= NULL;

//
// root UUID from hfs volume header
//
STATIC CHAR8 CONST* BlpRootUUIDFromDisk(EFI_HANDLE deviceHandle, CHAR8* uuidBuffer, UINTN uuidBufferLength)
{
	CHAR8 CONST* rootUUID													= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif /* _MSC_VER */
		//
		// get block io protocol
		//
		EFI_BLOCK_IO_PROTOCOL* blockIoProtocol								= NULL;
		if(EFI_ERROR(EfiBootServices->HandleProtocol(deviceHandle, &EfiBlockIoProtocolGuid, (VOID**)(&blockIoProtocol))))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return rootUUID;
#endif /* _MSC_VER */

		//
		// get disk io protocol
		//
		EFI_DISK_IO_PROTOCOL* diskIoProtocol								= NULL;
		if(EFI_ERROR(EfiBootServices->HandleProtocol(deviceHandle, &EfiDiskIoProtocolGuid, (VOID**)(&diskIoProtocol))))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return rootUUID;
#endif /* _MSC_VER */

		//
		// read volume header
		//
		STATIC UINT8 volumeHeader[0x200]									= {0};
		if(EFI_ERROR(diskIoProtocol->ReadDisk(diskIoProtocol, blockIoProtocol->Media->MediaId, 1024, sizeof(volumeHeader), volumeHeader)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return rootUUID;
#endif /* _MSC_VER */

		//
		// hfs+ volume
		//
		if(volumeHeader[0] == 'B' && volumeHeader[1] == 'D' && volumeHeader[0x7c] == 'H' && volumeHeader[0x7d] == '+')
		{
			UINT64 volumeHeaderOffset										= ((volumeHeader[0x1d] + (volumeHeader[0x1c] << 8)) << 9) + 1024;
			UINT16 startBlock												= volumeHeader[0x7f] + (volumeHeader[0x7e] << 8);
			UINT32 blockSize												= volumeHeader[0x17] + (volumeHeader[0x16] << 8) + (volumeHeader[0x15] << 16) + (volumeHeader[0x14] << 24);
			volumeHeaderOffset												+= startBlock * blockSize;

			if(EFI_ERROR(diskIoProtocol->ReadDisk(diskIoProtocol, blockIoProtocol->Media->MediaId, volumeHeaderOffset, sizeof(volumeHeader), volumeHeader)))
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else /* !_MSC_VER */
                return rootUUID;
#endif /* _MSC_VER */
		}

		//
		// check hfs+ volume header
		//
		UINT8 volumeId[8]													= {0};
		if(volumeHeader[0] == 'H' && (volumeHeader[1] == '+' || volumeHeader[1] == 'X'))
			memcpy(volumeId, volumeHeader + 0x68, sizeof(volumeId));
		else
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return rootUUID;
#endif /* _MSC_VER */

		//
		// just like AppleFileSystemDriver
		//
		UINT8 md5Result[0x10]												= {0};
		UINT8 prefixBuffer[0x10]											= {0xb3, 0xe2, 0x0f, 0x39, 0xf2, 0x92, 0x11, 0xd6, 0x97, 0xa4, 0x00, 0x30, 0x65, 0x43, 0xec, 0xac};
		MD5_CONTEXT md5Context;
		MD5Init(&md5Context);
		MD5Update(&md5Context, prefixBuffer, sizeof(prefixBuffer));
		MD5Update(&md5Context, volumeId, sizeof(volumeId));
		MD5Final(md5Result, &md5Context);

		//
		// this UUID has been made version 3 style (i.e. via namespace)
		// see "-uuid-urn-" IETF draft (which otherwise copies byte for byte)
		//
		CHAR8* dstBuffer													= uuidBuffer;
		md5Result[6]														= 0x30 | (md5Result[6] & 0x0f);
		md5Result[8]														= 0x80 | (md5Result[8] & 0x3f);
		UINTN formatBase													= 0;
		UINT8 uuidFormat[]													= {4, 2, 2, 2, 6};
		for(UINTN formatIndex = 0; formatIndex < ARRAYSIZE(uuidFormat); formatIndex ++)
		{
			UINTN i;
			for(i = 0; i < uuidFormat[formatIndex]; i++)
			{
				UINT8 byteValue												= md5Result[formatBase + i];
				CHAR8 nibValue												= byteValue >> 4;

				*dstBuffer													= nibValue + '0';
				if(*dstBuffer > '9')
					*dstBuffer												= (nibValue - 9 + ('A' - 1));

				dstBuffer													+= 1;

				nibValue													= byteValue & 0xf;
				*dstBuffer													= nibValue + '0';
				if(*dstBuffer > '9')
					*dstBuffer												= (nibValue - 9 + ('A' - 1));

				dstBuffer													+= 1;
			}

			formatBase														+= i;
			if(formatIndex < ARRAYSIZE(uuidFormat) - 1)
			{
				*dstBuffer													= '-';
				dstBuffer													+= 1;
			}
			else
			{
				*dstBuffer													= 0;
			}
		}

		rootUUID															= uuidBuffer;
#if defined(_MSC_VER)
	}
	__finally
	{
	}
#endif /* _MSC_VER */

	return rootUUID;
}

//
// root uuid from device path
//
STATIC CHAR8 CONST* BlpRootUUIDFromDevicePath(EFI_HANDLE deviceHandle, CHAR8* uuidBuffer, UINTN uuidBufferLength)
{
	EFI_DEVICE_PATH_PROTOCOL* devicePath									= NULL;
	if(EFI_ERROR(EfiBootServices->HandleProtocol(deviceHandle, &EfiDevicePathProtocolGuid, (VOID**)(&devicePath))))
		return NULL;

	HARDDRIVE_DEVICE_PATH* hardDriverDevicePath								= _CR(DevPathGetNode(devicePath, MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP), HARDDRIVE_DEVICE_PATH, Header);
	if(!hardDriverDevicePath || hardDriverDevicePath->MBRType != MBR_TYPE_EFI_PARTITION_TABLE_HEADER || hardDriverDevicePath->SignatureType != SIGNATURE_TYPE_GUID)
		return NULL;

	EFI_GUID g;
	memcpy(&g, hardDriverDevicePath->Signature, sizeof(g));
	CHAR8 CONST* formatString												= CHAR8_CONST_STRING("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X");
	snprintf(uuidBuffer, uuidBufferLength, formatString, g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],g.Data4[4],g.Data4[5],g.Data4[6],g.Data4[7]);

	return uuidBuffer;
}

//
// add ram dmg property
//
STATIC VOID BlpAddRamDmgProperty(DEVICE_TREE_NODE* chosenNode, EFI_DEVICE_PATH_PROTOCOL* bootDevicePath)
{
#if defined(_MSC_VER)
	__try
	{
#endif /* _MSC_VER */
		//
		// get mem map device path
		//
		MEMMAP_DEVICE_PATH* memMapDevicePath								= NULL;
		MEMMAP_DEVICE_PATH* ramDmgDevicePath								= NULL;
		UINT64* ramDmgSize													= NULL;
		while(!EfiIsDevicePathEnd(bootDevicePath))
		{
			if(EfiDevicePathType(bootDevicePath) == HARDWARE_DEVICE_PATH && bootDevicePath->SubType == HW_MEMMAP_DP)
			{
				memMapDevicePath											= _CR(bootDevicePath, MEMMAP_DEVICE_PATH, Header);
			}
			else if(EfiDevicePathType(bootDevicePath) == MESSAGING_DEVICE_PATH && bootDevicePath->SubType == MSG_VENDOR_DP)
			{
				VENDOR_DEVICE_PATH* vendorDevicePath						= _CR(bootDevicePath, VENDOR_DEVICE_PATH, Header);
				if(!memcmp(&vendorDevicePath->Guid, &AppleRamDmgDevicePathGuid, sizeof(EFI_GUID)))
				{
					ramDmgSize												= (UINT64*)(vendorDevicePath + 1);
					ramDmgDevicePath										= memMapDevicePath;
					break;
				}
			}
			bootDevicePath													= EfiNextDevicePathNode(bootDevicePath);
		}
		if(!ramDmgDevicePath)
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return;
#endif /* _MSC_VER */

		//
		// check length
		//
		if(ramDmgDevicePath->EndingAddress + 1 - ramDmgDevicePath->StartingAddress < sizeof(RAM_DMG_HEADER))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return;
#endif /* _MSC_VER */

		//
		// check header
		//
		RAM_DMG_HEADER* ramDmgHeader										= ArchConvertAddressToPointer(ramDmgDevicePath->StartingAddress, RAM_DMG_HEADER*);
		if(ramDmgHeader->Signature != ramDmgHeader->Signature2 || ramDmgHeader->Signature != 0x544E5458444D4152ULL || ramDmgHeader->Version != 0x10000)
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return;
#endif /* _MSC_VER */

		//
		// check size
		//
		if(ramDmgHeader->ExtentCount > ARRAYSIZE(ramDmgHeader->ExtentInfo) || !*ramDmgSize)
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else /* !_MSC_VER */
            return;
#endif /* _MSC_VER */

		DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-ramdmg-extents"), ramDmgHeader->ExtentInfo, ramDmgHeader->ExtentCount * sizeof(RAM_DMG_EXTENT_INFO), FALSE);
		DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-ramdmg-size"), ramDmgSize, sizeof(UINT64), FALSE);
#if defined(_MSC_VER)
	}
	__finally
	{

	}
#endif /* _MSC_VER */
}

//
// add memory range
//
EFI_STATUS BlAddMemoryRangeNode(CHAR8 CONST* rangeName, UINT64 physicalAddress, UINT64 rangeLength)
{
	UINT32 propertyBuffer[2]												= {(UINT32)(physicalAddress), (UINT32)(rangeLength)};
	return DevTreeAddProperty(BlpMemoryMapNode, rangeName, propertyBuffer, sizeof(propertyBuffer), TRUE);
}

//
// init boot args
//
EFI_STATUS BlInitializeBootArgs(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath, EFI_DEVICE_PATH_PROTOCOL* bootFilePath, EFI_HANDLE bootDeviceHandle, CHAR8 CONST* kernelCommandLine, BOOT_ARGS** bootArgsP)
{
	EFI_STATUS status														= EFI_SUCCESS;
	*bootArgsP																= NULL;
	UINT64 virtualAddress													= 0;
	UINT64 physicalAddress													= 0;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// detect acpi nvs memory
		//
		AcpiDetectNVSMemory();

		//
		// allocate memory
		//
		UINTN bufferLength													= sizeof(BOOT_ARGS);
		physicalAddress														= MmAllocateKernelMemory(&bufferLength, &virtualAddress);

        if(!physicalAddress) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            return status;
#endif
        }

		//
		// fill common fileds
		//
		BOOT_ARGS* bootArgs													= ArchConvertAddressToPointer(physicalAddress, BOOT_ARGS*);
		memset(bootArgs, 0, sizeof(BOOT_ARGS));
		bootArgs->Revision													= 0;
		bootArgs->Version													= 2;
		bootArgs->EfiMode													= ArchNeedEFI64Mode() ? 64 : 32;
		bootArgs->DebugMode													= 0;
#if LEGACY_GREY_SUPPORT
		bootArgs->Flags														= 1;	// kBootArgsFlagRebootOnPanic
#else
		bootArgs->Flags														= 65;	// kBootArgsFlagRebootOnPanic + kBootArgsFlagBlackTheme
#endif
		bootArgs->PhysicalMemorySize										= BlGetMemorySize();

		for (UINT8 m = 0; m < 5; m++)
		{
			CsPrintf(CHAR8_CONST_STRING("PIKE: bootArgs->PhysicalMemorySize= 0x%llx\n"), bootArgs->PhysicalMemorySize);
		}

		bootArgs->ASLRDisplacement											= (UINT32)(LdrGetASLRDisplacement());

		//
		// read fsb frequency
		//
		DEVICE_TREE_NODE* platformNode										= DevTreeFindNode(CHAR8_CONST_STRING("/efi/platform"), FALSE);
		if(platformNode)
		{
			UINT32 length													= 0;
			VOID CONST* fsbFrequency										= DevTreeGetProperty(platformNode, CHAR8_CONST_STRING("FSBFrequency"), &length);
			if(length >= 8)
				bootArgs->FSBFrequency										= *(UINT64 CONST*)(fsbFrequency);
			else if(length >= 4)
				bootArgs->FSBFrequency										= *(UINT32 CONST*)(fsbFrequency);
		}

		//
		// get PCI config space info
		//
		AcpiGetPciConfigSpaceInfo(&bootArgs->PCIConfigSpaceBaseAddress, &bootArgs->PCIConfigSpaceStartBusNumber, &bootArgs->PCIConfigSpaceEndBusNumber);

#if (TARGET_OS == EL_CAPITAN)
		//
		// Boot P-State limit for Power Management (set to 0 = no limit)
		//
		bootArgs->Boot_SMC_plimit											= 0;
#endif
		//
		// get root node
		//
		DEVICE_TREE_NODE* rootNode											= DevTreeFindNode(CHAR8_CONST_STRING("/"), FALSE);
        if(!rootNode) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            return status;
#endif
        }

		//
		// compatible = ACPI
		//
        if(EFI_ERROR(status = DevTreeAddProperty(rootNode, CHAR8_CONST_STRING("compatible"), "ACPI", 5, FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);
            
            return status;
#endif
        }

		//
		// model = ACPI
		//
        if(EFI_ERROR(status = DevTreeAddProperty(rootNode, CHAR8_CONST_STRING("model"), "ACPI", 5, FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);
            
            return status;
#endif
        }

		//
		// get chosen node
		//
		DEVICE_TREE_NODE* chosenNode										= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);
        if(!chosenNode) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            return status;
#endif
        }

		//
		// get memory map node
		//
		BlpMemoryMapNode													= DevTreeFindNode(CHAR8_CONST_STRING("/chosen/memory-map"), TRUE);
        if(!BlpMemoryMapNode) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            return status;
#endif
        }

		//
		// get boot guid
		//
		if(!CmGetStringValueForKeyAndCommandLine(kernelCommandLine, CHAR8_CONST_STRING("rd"), NULL, FALSE))
		{
			//
			// get root match
			//
			CHAR8 CONST* rootMatchDict										= CmSerializeValueForKey(CHAR8_CONST_STRING("Root Match"), NULL);
			if(!rootMatchDict)
			{
				//
				// root match dict not found,try root UUID
				//
				CHAR8 uuidBuffer[0x41]										= {0};
				CHAR8 const* rootUUID										= CmGetStringValueForKey(NULL, CHAR8_CONST_STRING("Root UUID"), NULL);
				if(!rootUUID)
				{
					//
					// check net boot
					//
					rootMatchDict											= NetGetRootMatchDict(bootDevicePath);
					if(rootMatchDict)
					{
						//
						// set net-boot flags
						//
						DevTreeAddProperty(rootNode, CHAR8_CONST_STRING("net-boot"), NULL, 0, FALSE);
					}
					else
					{
						//
						// build uuid by md5(hfs volume header)
						//
						rootUUID											= BlpRootUUIDFromDisk(bootDeviceHandle, uuidBuffer, ARRAYSIZE(uuidBuffer) - 1);

						//
						// extract uuid from device path
						//
						if(!rootUUID)
							rootUUID										= BlpRootUUIDFromDevicePath(bootDeviceHandle, uuidBuffer, ARRAYSIZE(uuidBuffer) - 1);
					}
				}

				//
				// add root uuid
				//
				if(rootUUID)
					DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-uuid"), rootUUID, (UINT32)(strlen(rootUUID) + 1) * sizeof(CHAR8), TRUE);
			}

			//
			// add root match
			//
			if(rootMatchDict)
			{
				DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("root-matching"), rootMatchDict, (UINT32)(strlen(rootMatchDict) + 1) * sizeof(CHAR8), TRUE);
				MmFreePool((CHAR8*)(rootMatchDict));
			}
		}

		//
		// Set chosen/boot-file property.
		//
		CHAR8 CONST* bootFileName											= LdrGetKernelCachePathName();
		
		if(BlTestBootMode(BOOT_MODE_SAFE))
			bootFileName													= LdrGetKernelPathName();

		if(EFI_ERROR(status = DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-file"), bootFileName, (UINT32)(strlen(bootFileName) + 1) * sizeof(CHAR8), FALSE)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            return status;
#endif
		}

		//
		// add boot device path
		//
        if(EFI_ERROR(status = DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-device-path"), bootDevicePath, (UINT32)(DevPathGetSize(bootDevicePath)), FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);
            
            return status;
#endif
        }

		//
		// add boot file path
		//
        if(EFI_ERROR(status = DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-file-path"), bootFilePath, (UINT32)(DevPathGetSize(bootFilePath)), FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(EFI_ERROR(status))
                MmFreeKernelMemory(virtualAddress, physicalAddress);
            
            return status;
#endif
        }

		//
		// Boot != Root key for firmware's /chosen
		//
		if(BlTestBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT))
		{
			if(EFI_ERROR(status = DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("bootroot-active"), NULL, 0, FALSE)))
			{
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                if(EFI_ERROR(status))
                    MmFreeKernelMemory(virtualAddress, physicalAddress);

                return status;
#endif
			}
		}

		//
		// Add booter version information.
		//
		BlAddBooterInfo(chosenNode);

		//
		// add ram dmg info
		//
		BlpAddRamDmgProperty(chosenNode, bootDevicePath);

#if (TARGET_OS >= YOSEMITE)
		//
		// Add 'random-seed' property.
		//
		UINT8 index															= 0;
		UINT16 PMTimerValue													= 0;
		UINT32 ecx, esi, edi												= 0;
		UINT64 rcx, rdx, rsi, rdi, cpuTick									= 0;

		UINT8 seedBuffer[64]												= {0};

		ecx																	= 0;				// xor		%ecx,	%ecx
		rcx = rdx = rsi = rdi = 0;

		do																						// 0x17e55:
		{
			//
			// The RDRAND instruction is part of the Intel Secure Key Technology, which is currently only available 
			// on the Intel i5/i7 Ivy Bridge and Haswell processors. Not on processors used in the old MacPro models.
			// This is why I had to disassemble Apple's boot.efi and port their assembler code to standard C.
			//
			PMTimerValue = ARCH_READ_PORT_UINT16(ArchConvertAddressToPointer(0x408, UINT16*));	// in		(%dx),	%ax
			esi = PMTimerValue;																	// movzwl	%ax,	%esi
			
			if (esi < ecx)																		// cmp		%ecx,	%esi
			{
				continue;																		// jb		0x17e55		(retry)
			}
			
			cpuTick = ArchGetCpuTick();															// callq	0x121a7
			rcx = (cpuTick >> 8);																// mov		%rax,	%rcx
																								// shr		$0x8,	%rcx
			rdx = (cpuTick >> 10);																// mov		%rax,	%rdx
																								// shr		$0x10,	%rdx
			rdi = rsi;																			// mov		%rsi,	%rdi
			rdi = (rdi ^ cpuTick);																// xor		%rax,	%rdi
			rdi = (rdi ^ rcx);																	// xor		%rcx,	%rdi
			rdi = (rdi ^ rdx);																	// xor		%rdx,	%rdi
			
			seedBuffer[index] = (rdi & 0xff);													// mov		%dil,	(%r15,%r12,1)
			
			edi = (edi & 0x2f);																	// and		$0x2f,	%edi
			edi = (edi + esi);																	// add		%esi,	%edi
			index++;																			// inc		r12
			ecx = (edi & 0xffff);																// movzwl	%di,	%ecx
			
		} while (index < 64);																	// cmp		%r14d,	%r12d
																								// jne		0x17e55		(next)

		DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("random-seed"), seedBuffer, sizeof(seedBuffer), TRUE);
#endif // #if (TARGET_OS >= YOSEMITE)

		//
		// output
		//
		*bootArgsP															= bootArgs;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(EFI_ERROR(status))
			MmFreeKernelMemory(virtualAddress, physicalAddress);
#if defined(_MSC_VER)
	}
#endif
    
	return status;
}

//
// finalize boot args
//
EFI_STATUS BlFinalizeBootArgs(BOOT_ARGS* bootArgs, CHAR8 CONST* kernelCommandLine, EFI_HANDLE bootDeviceHandle, MACH_O_LOADED_INFO* loadedInfo)
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// save command line
		//
		strncpy(bootArgs->CommandLine, kernelCommandLine, ARRAYSIZE(bootArgs->CommandLine) - 1);

		//
		// get chosen node
		//
		DEVICE_TREE_NODE* chosenNode										= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);
        if(!chosenNode) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

		//
		// add boot-args
		//
		if(EFI_ERROR(status = DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-args"), bootArgs->CommandLine, (UINT32)(strlen(bootArgs->CommandLine) + 1) * sizeof(CHAR8), FALSE)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif
        
		//
		// net
		//
		if(EFI_ERROR(status = NetSetupDeviceTree(bootDeviceHandle)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// pe
		//
		if(EFI_ERROR(status = PeSetupDeviceTree()))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// FileVault2, key store
		//
		if(BlTestBootMode(BOOT_MODE_HAS_FILE_VAULT2_CONFIG))
		{
			UINT64 keyStorePhysicalAddress									= 0;
			UINTN keyStoreDataSize											= 0;
			if(EFI_ERROR(status = FvSetupDeviceTree(&keyStorePhysicalAddress, &keyStoreDataSize, TRUE)))
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                return status;
#endif

			bootArgs->KeyStoreDataStart										= (UINT32)(keyStorePhysicalAddress);
			bootArgs->KeyStoreDataSize										= (UINT32)(keyStoreDataSize);
		}
			

		//
		// console device tree
		//
		if(EFI_ERROR(status = CsSetupDeviceTree(bootArgs)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// get device tree size
		//
		UINT32 deviceTreeSize												= 0;
		DevTreeFlatten(NULL, &deviceTreeSize);

		//
		// allocate buffer
		//
		UINT64 virtualAddress												= 0;
		UINTN bufferLength													= deviceTreeSize;
		UINT64 physicalAddress												= MmAllocateKernelMemory(&bufferLength, &virtualAddress);
        if(!physicalAddress) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }
		//
		// flatten it
		//
		VOID* flattenBuffer													= ArchConvertAddressToPointer(physicalAddress, VOID*);
		if(EFI_ERROR(status = DevTreeFlatten(&flattenBuffer, &deviceTreeSize)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// save it
		//
		bootArgs->DeviceTree												= (UINT32)(physicalAddress);
		bootArgs->DeviceTreeLength											= deviceTreeSize;

		//
		// free device tree
		//
		DevTreeFinalize();

		//
		// get memory map
		//
		UINTN memoryMapSize													= 0;
		EFI_MEMORY_DESCRIPTOR* memoryMap									= NULL;
		UINTN memoryMapKey													= 0;
		UINTN descriptorSize												= 0;
		UINT32 descriptorVersion											= 0;
		if(EFI_ERROR(status = MmGetMemoryMap(&memoryMapSize, &memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// get runtime memory info
		//
		UINT64 runtimeMemoryPages											= 0;
		UINTN runtimeMemoryDescriptors										= MmGetRuntimeMemoryInfo(memoryMap, memoryMapSize, descriptorSize, &runtimeMemoryPages);

		//
		// free memory map
		//
		MmFreePool(memoryMap);

		//
		// allocate kernel memory for runtime area
		//
		runtimeMemoryPages													+= 1;
		runtimeMemoryDescriptors											+= 1;
		UINT64 runtimeServicesVirtualAddress								= 0;
		bufferLength														= (UINTN)(runtimeMemoryPages << EFI_PAGE_SHIFT);
		UINT64 runtimeServicesPhysicalAddress								= MmAllocateKernelMemory(&bufferLength, &runtimeServicesVirtualAddress);
        if(!runtimeServicesPhysicalAddress) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }
		//
		// allocate memory map
		//
		memoryMapSize														+= runtimeMemoryDescriptors * descriptorSize + 512;
		UINT64 memoryMapVirtualAddress										= 0;
		UINT64 memoryMapPhysicalAddress										= MmAllocateKernelMemory(&memoryMapSize, &memoryMapVirtualAddress);
        if(!memoryMapPhysicalAddress) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

		//
		// get memory map
		//
		for(UINTN i = 0; i < 5; i ++)
		{
			UINTN currentSize												= 0;
			status															= EfiBootServices->GetMemoryMap(&currentSize, 0, &memoryMapKey, &descriptorSize, &descriptorVersion);
			if(status != EFI_BUFFER_TOO_SMALL)
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                return status;
#endif

			if(currentSize > memoryMapSize)
			{
				MmFreeKernelMemory(memoryMapVirtualAddress, memoryMapPhysicalAddress);
				currentSize													+= (runtimeMemoryDescriptors + 2) * descriptorSize + 512;
				memoryMapPhysicalAddress									= MmAllocateKernelMemory(&currentSize, &memoryMapVirtualAddress);
				memoryMapSize												= currentSize;
			}

			memoryMap														= ArchConvertAddressToPointer(memoryMapPhysicalAddress, EFI_MEMORY_DESCRIPTOR*);
			status															= EfiBootServices->GetMemoryMap(&currentSize, memoryMap, &memoryMapKey, &descriptorSize, &descriptorVersion);
			if(!EFI_ERROR(status))
			{
				memoryMapSize												= currentSize;
				break;
			}
		}

		//
		// unable to get memory map
		//
		if(EFI_ERROR(status))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// save it
		//
		bootArgs->MemoryMap													= (UINT32)(memoryMapPhysicalAddress);
		bootArgs->MemoryMapDescriptorSize									= (UINT32)(descriptorSize);
		bootArgs->MemoryMapDescriptorVersion								= (UINT32)(descriptorVersion);
		bootArgs->MemoryMapSize												= (UINT32)(memoryMapSize);

		//
		// exit boot services
		//
		if(EFI_ERROR(status = EfiBootServices->ExitBootServices(EfiImageHandle, memoryMapKey)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif

		//
		// adjust memory map
		//
		memoryMapSize														= AcpiAdjustMemoryMap(memoryMap, memoryMapSize, descriptorSize);
		bootArgs->MemoryMapSize												= (UINT32)(memoryMapSize);

		//
		// sort memory map
		//
		MmSortMemoryMap(memoryMap, memoryMapSize, descriptorSize);

		//
		// convert pointer
		//
		UINT64 efiSystemTablePhysicalAddress								= ArchConvertPointerToAddress(EfiSystemTable);
		MmConvertPointers(memoryMap, &memoryMapSize, descriptorSize, descriptorVersion, runtimeServicesPhysicalAddress, runtimeMemoryPages, runtimeServicesVirtualAddress, &efiSystemTablePhysicalAddress, TRUE, loadedInfo);

		//
		// sort memory map
		//
		MmSortMemoryMap(memoryMap, memoryMapSize, descriptorSize);

		//
		// save efi services
		//
		bootArgs->MemoryMapSize												= (UINT32)(memoryMapSize);
		bootArgs->EfiSystemTable											= (UINT32)(efiSystemTablePhysicalAddress);
		bootArgs->EfiRuntimeServicesPageCount								= (UINT32)(runtimeMemoryPages);
		bootArgs->EfiRuntimeServicesPageStart								= (UINT32)(runtimeServicesPhysicalAddress >> EFI_PAGE_SHIFT);
		bootArgs->EfiRuntimeServicesVirtualPageStart						= runtimeServicesVirtualAddress >> EFI_PAGE_SHIFT;

		//
		// save kernel range
		//
		UINT64 kernelBegin													= 0;
		UINT64 kernelEnd													= 0;
		MmGetKernelPhysicalRange(&kernelBegin, &kernelEnd);
		bootArgs->KernelAddress												= (UINT32)(kernelBegin);
		bootArgs->KernelSize												= (UINT32)(kernelEnd - kernelBegin);

		//
		// clear serives pointer
		//
		EfiBootServices														= NULL;
#if defined(_MSC_VER)
	}
	__finally
	{
	}
#endif

	return status;
}

#if (TARGET_OS == EL_CAPITAN)
//
// Configure System Integrity Protection.
//
EFI_STATUS BlInitCSRState(BOOT_ARGS* bootArgs)
{
#if DEBUG_NVRAM_CALL_CSPRINTF
	UINT8 i																	= 0;
#endif
	EFI_STATUS status														= EFI_SUCCESS;
	UINT32 attributes														= (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS);
	UINT32 csrActiveConfig													= CSR_ALLOW_APPLE_INTERNAL;
	UINTN dataSize															= sizeof(UINT32);

	if(BlTestBootMode(BOOT_MODE_FROM_RECOVER_BOOT_DIRECTORY | BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE | BOOT_MODE_IS_INSTALLER))
	{
#if DEBUG_NVRAM_CALL_CSPRINTF
		if (BlTestBootMode(BOOT_MODE_IS_INSTALLER))
		{
			for (i = 0; i < 5; i++)
			{
				CsPrintf(CHAR8_CONST_STRING("PIKE: BlInitCSRState(Installer detected)!\n"));
			}
		}
		else
		{
			for (i = 0; i < 5; i++)
			{
				CsPrintf(CHAR8_CONST_STRING("PIKE: BlInitCSRState(RecoveryOS detected)!\n"));
			}
		}
#endif
		attributes															|= EFI_VARIABLE_NON_VOLATILE;
		csrActiveConfig														= CSR_ALLOW_DEVICE_CONFIGURATION;
		bootArgs->Flags														|= (kBootArgsFlagCSRActiveConfig + kBootArgsFlagCSRConfigMode + kBootArgsFlagCSRBoot);
	}
	else
	{
		bootArgs->Flags														|= (kBootArgsFlagCSRActiveConfig + kBootArgsFlagCSRBoot);
	}

	//
	// System Integrity Protection Capabilties.
	//
	bootArgs->CsrCapabilities												= CSR_VALID_FLAGS;
	
	//
	// Check 'csr-active-config' variable in NVRAM.
	//
	if(EFI_ERROR(status = EfiRuntimeServices->GetVariable(CHAR16_STRING(L"csr-active-config"), &AppleNVRAMVariableGuid, NULL, &dataSize, &csrActiveConfig)))
	{
#if DEBUG_NVRAM_CALL_CSPRINTF
		for (i = 0; i < 5; i++)
		{
			CsPrintf(CHAR8_CONST_STRING("PIKE: NVRAM csr-active-config NOT found (ERROR: %d)!\n"), status);
		}
#endif
		//
		// Not there. Add the 'csr-active-config' variable.
		//
		if(EFI_ERROR(status = EfiRuntimeServices->SetVariable(CHAR16_STRING(L"csr-active-config"), &AppleNVRAMVariableGuid, attributes, sizeof(UINT32), &csrActiveConfig)))
		{
#if DEBUG_NVRAM_CALL_CSPRINTF
			for (i = 0; i < 5; i++)
			{
				CsPrintf(CHAR8_CONST_STRING("PIKE: NVRAM csr-active-config add failed (ERROR: %d)!\n"), status);
			}
#endif
		}
		else
		{
#if DEBUG_NVRAM_CALL_CSPRINTF
			for (i = 0; i < 5; i++)
			{
				CsPrintf(CHAR8_CONST_STRING("PIKE: NVRAM csr-active-config[0x%x] set (OK)!\n"), csrActiveConfig);
			}
#endif
		}
		//
		// Set System Integrity Protection ON by default
		//
		bootArgs->CsrActiveConfig											= (csrActiveConfig & 0x6f);
#if DEBUG_NVRAM_CALL_CSPRINTF
		for (i = 0; i < 5; i++)
		{
			CsPrintf(CHAR8_CONST_STRING("PIKE: bootArgs->CsrActiveConfig[0x%x] set!\n"), csrActiveConfig);
		}
#endif
	}
	else
	{
		//
		// Set System Integrity Protection to the value found in NVRAM.
		//
		bootArgs->CsrActiveConfig											= (csrActiveConfig & 0x6f);
#if DEBUG_NVRAM_CALL_CSPRINTF
		for (i = 0; i < 5; i++)
		{
			CsPrintf(CHAR8_CONST_STRING("PIKE: NVRAM csr-active-config[0x%x/0x%x] found (OK)!\n"), csrActiveConfig, dataSize);
		}
#endif
	}

	return status;
}
#endif

//
// Mimic boot.efi and set boot.efi info properties.
//
EFI_STATUS BlAddBooterInfo(DEVICE_TREE_NODE* chosenNode)
{
	EFI_STATUS status														= EFI_SUCCESS;

	//
	// Static data for now. Should extract this from Apple's boot.efi
	//
	if (BlTestBootMode(BOOT_MODE_IS_INSTALLER))
	{
		DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("booter-name"), "bootbase.efi", 12, FALSE);
	}
	else
	{
		DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("booter-name"), "boot.efi", 8, FALSE);
	}

	DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("booter-version"), "version:307", 11, FALSE);
	DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("booter-build-time"), "Fri Sep  4 15:34:00 PDT 2015", 28, FALSE);

	return status;
}
