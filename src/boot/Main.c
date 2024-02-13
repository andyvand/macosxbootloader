//********************************************************************
//	created:	4:11:2009   10:04
//	filename: 	boot.cpp
//	author:		tiamo
//	purpose:	main
//********************************************************************

#include "StdAfx.h"
#include "DebugUsb.h"

//
// Read debug options.
//
STATIC EFI_STATUS BlpReadDebugOptions(CHAR8** debugOptions)
{
	*debugOptions															= NULL;
	UINTN variableSize														= 0;
	EFI_STATUS status														= EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-windbg-args"), &AppleNVRAMVariableGuid, NULL, &variableSize, NULL);

	if (status != EFI_BUFFER_TOO_SMALL)
		return status;

	CHAR8* variableBuffer													= (CHAR8*)(MmAllocatePool(variableSize + sizeof(CHAR8)));

	if (!variableBuffer)
		return EFI_OUT_OF_RESOURCES;

	variableBuffer[variableSize]											= 0;
	status																	= EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-windbg-args"), &AppleNVRAMVariableGuid, NULL, &variableSize, variableBuffer);

	if (!EFI_ERROR(status))
		*debugOptions														= variableBuffer;
	else
		MmFreePool(variableBuffer);

	return status;
}

//
// Setup ROM variable.
//
STATIC EFI_STATUS BlpSetupRomVariable()
{
#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// ROM = [0xffffff01, 0xffffff07).
		//
		UINT32 attribute													= EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
		UINT8 romBuffer[6]													= {0};
		UINTN dataSize														= sizeof(romBuffer);

		if (EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"ROM"), &AppleFirmwareVariableGuid, NULL, &dataSize, romBuffer)))
			EfiRuntimeServices->SetVariable(CHAR16_STRING(L"ROM"), &AppleFirmwareVariableGuid, attribute, sizeof(romBuffer), ArchConvertAddressToPointer(0xffffff01, VOID*));

		//
		// Check MLB.
		//
		UINT8 mlbBuffer[0x80]												= {0};
		dataSize															= sizeof(mlbBuffer);

		if (!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"MLB"), &AppleFirmwareVariableGuid, NULL, &dataSize, mlbBuffer)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return EFI_SUCCESS;
#endif

		//
		// Search [0xffffff08, 0xffffff50).
		//
		UINT8 tempBuffer[0x48];
		memset(tempBuffer, 0xff, sizeof(tempBuffer));
		EfiBootServices->CopyMem(tempBuffer, ArchConvertAddressToPointer(0xffffff08, VOID*), sizeof(tempBuffer));

		for (UINTN i = 0; i < 4; i ++)
		{
			if (tempBuffer[i * 0x12] == 0xff)
			{
				if (i)
				{
					dataSize												= 0;
					UINT8* mlb_buffer										= tempBuffer + i * 0x12 - 0x12;

					while (mlb_buffer[dataSize] != ' ')
						dataSize											+= 1;

					EfiRuntimeServices->SetVariable(CHAR16_STRING(L"MLB"), &AppleFirmwareVariableGuid, attribute, dataSize, mlb_buffer);
				}

				break;
			}
		}
#if defined(_MSC_VER)
	}
	__finally
	{

	}
#endif

	return EFI_SUCCESS;
}

//
// Check board-id.
//
STATIC EFI_STATUS BlpCheckBoardId(CHAR8 CONST* boardId, EFI_DEVICE_PATH_PROTOCOL* bootFilePath)
{
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_DEVICE_PATH_PROTOCOL* fileDevPath									= NULL;
	CHAR8* fileName															= NULL;
	CHAR8* fileBuffer														= NULL;
	XML_TAG* rootTag														= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// VMM ok.
		//
        if (!strnicmp(boardId, CHAR8_CONST_STRING("VMM"), 3)) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileDevPath)
                MmFreePool(fileDevPath);

            if (fileName)
                MmFreePool(fileName);

            if (fileBuffer)
                MmFreePool(fileBuffer);

            if (rootTag)
                CmFreeTag(rootTag);

            return status;
#endif
        }

		//
		// Check current directory.
		//
		fileDevPath															= DevPathAppendLastComponent(bootFilePath, CHAR8_CONST_STRING("PlatformSupport.plist"), TRUE);

		if (fileDevPath)
		{
			fileName														= DevPathExtractFilePathName(fileDevPath, TRUE);

			if (fileName)
				status														= IoReadWholeFile(bootFilePath, fileName, &fileBuffer, NULL, TRUE);
			else
				status														= EFI_NOT_FOUND;
		}
		else
		{
			//
			// Check root directory.
			//
			status															= IoReadWholeFile(bootFilePath, CHAR8_CONST_STRING("PlatformSupport.plist"), &fileBuffer, NULL, TRUE);
		}

		//
		// Check default file.
		//
        if (EFI_ERROR(status) && EFI_ERROR(status = IoReadWholeFile(bootFilePath, CHAR8_CONST_STRING("System\\Library\\CoreServices\\PlatformSupport.plist"), &fileBuffer, NULL, TRUE))) {
#if defined(_MSC_VER)
            try_leave(status = EFI_SUCCESS);
#else
            status = EFI_SUCCESS;

            if (fileDevPath)
                MmFreePool(fileDevPath);

            if (fileName)
                MmFreePool(fileName);

            if (fileBuffer)
                MmFreePool(fileBuffer);

            if (rootTag)
                CmFreeTag(rootTag);

            return status;
#endif
        }

		//
		// Parse the file.
		//
        if (EFI_ERROR(status = CmParseXmlFile(fileBuffer, &rootTag))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileDevPath)
                MmFreePool(fileDevPath);

            if (fileName)
                MmFreePool(fileName);

            if (fileBuffer)
                MmFreePool(fileBuffer);

            if (rootTag)
                CmFreeTag(rootTag);

            return status;
#endif
        }

		//
		// Get tag value.
		//
		XML_TAG* supportedIds												= CmGetTagValueForKey(rootTag, CHAR8_CONST_STRING("SupportedBoardIds"));
		UINTN count															= CmGetListTagElementsCount(supportedIds);

		for (UINTN i = 0; i < count; i ++)
		{
			XML_TAG* supportedId											= CmGetListTagElementByIndex(supportedIds, i);

			if (!supportedId || supportedId->Type != XML_TAG_STRING)
				continue;

            if (!strcmp(supportedId->StringValue, boardId)) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (fileDevPath)
                    MmFreePool(fileDevPath);

                if (fileName)
                    MmFreePool(fileName);

                if (fileBuffer)
                    MmFreePool(fileBuffer);

                if (rootTag)
                    CmFreeTag(rootTag);

                return status;
#endif
            }
		}
		status																= EFI_UNSUPPORTED;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (fileDevPath)
			MmFreePool(fileDevPath);

		if (fileName)
			MmFreePool(fileName);

		if (fileBuffer)
			MmFreePool(fileBuffer);

		if (rootTag)
			CmFreeTag(rootTag);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// Run recovery booter.
//
STATIC EFI_STATUS BlpRunRecoveryEfi(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath, EFI_DEVICE_PATH_PROTOCOL* bootFilePath)
{
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_DEVICE_PATH_PROTOCOL* recoveryFilePath								= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// Get current partition number.
		//
		UINT32 partitionNumber												= DevPathGetPartitionNumber(bootDevicePath);

        if (partitionNumber == -1) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (recoveryFilePath)
                MmFreePool(recoveryFilePath);

            return status;
#endif
        }

		//
		// Root partition is followed by recovery partition.
		//
		if (!BlTestBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT))
			partitionNumber													+= 1;

		//
		// Get recovery partition handle.
		//
		EFI_HANDLE recoveryPartitionHandle									= DevPathGetPartitionHandleByNumber(bootDevicePath, partitionNumber);

        if (!recoveryPartitionHandle) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (recoveryFilePath)
                MmFreePool(recoveryFilePath);

            return status;
#endif
        }

		//
		// Get recovery partition device path.
		//
		EFI_DEVICE_PATH_PROTOCOL* recoveryPartitionDevicePath				= DevPathGetDevicePathProtocol(recoveryPartitionHandle);

        if (!recoveryPartitionDevicePath) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (recoveryFilePath)
                MmFreePool(recoveryFilePath);

            return status;
#endif
        }

		//
		// Get recovery file path.
		//
		recoveryFilePath													= DevPathAppendFilePath(recoveryPartitionDevicePath, CHAR16_CONST_STRING(L"\\com.apple.recovery.boot\\boot.efi"));

        if (!recoveryFilePath) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (recoveryFilePath)
                MmFreePool(recoveryFilePath);

            return status;
#endif
        }

		//
		// Load image.
		//
		EFI_HANDLE imageHandle												= NULL;

		if (EFI_ERROR(status = EfiBootServices->LoadImage(FALSE, EfiImageHandle, recoveryFilePath, NULL, 0, &imageHandle)))
		{
            if (!BlTestBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT)) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (recoveryFilePath)
                    MmFreePool(recoveryFilePath);

                return status;
#endif
            }

			//
			// Get root UUID.
			//
			CHAR8 CONST* rootUUID											= CmGetStringValueForKey(NULL, CHAR8_CONST_STRING("Root UUID"), NULL);

            if (!rootUUID) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
            	status = EFI_NOT_FOUND;

                if (recoveryFilePath)
                    MmFreePool(recoveryFilePath);

                return status;
#endif
            }

			//
			// Load booter.
			//
            if (EFI_ERROR(status = IoLoadBooterWithRootUUID(bootFilePath, rootUUID, &imageHandle))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (recoveryFilePath)
                    MmFreePool(recoveryFilePath);

                return status;
#endif
            }
		}

		//
		// Start it.
		//
		status																= EfiBootServices->StartImage(imageHandle, NULL, NULL);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (recoveryFilePath)
			MmFreePool(recoveryFilePath);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// Run Apple boot.
//
STATIC EFI_STATUS BlpRunAppleBoot(CHAR8 CONST* bootFileName)
{
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_HANDLE* handleArray													= NULL;
	CHAR16* fileName														= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// Convert file name.
		//
		fileName															= BlAllocateUnicodeFromUtf8(bootFileName, strlen(bootFileName));
        if (!fileName) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

		//
		// Locate file system protocol.
		//
		UINTN totalHandles													= 0;

        if (EFI_ERROR(status = EfiBootServices->LocateHandleBuffer(ByProtocol, &EfiSimpleFileSystemProtocolGuid, NULL, &totalHandles, &handleArray))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileName)
                MmFreePool(fileName);

            return status;
#endif
        }

		for (UINTN i = 0; i < totalHandles; i ++)
		{
			EFI_HANDLE theHandle											= handleArray[i];

			if (!theHandle)
				continue;

			//
			// Get file system protocol.
			//
			EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystemProtocol				= NULL;

			if (EFI_ERROR(EfiBootServices->HandleProtocol(theHandle, &EfiSimpleFileSystemProtocolGuid, (VOID**)(&fileSystemProtocol))))
				continue;

			//
			// Open root directory.
			//
			EFI_FILE_HANDLE rootFile										= NULL;

			if (EFI_ERROR(fileSystemProtocol->OpenVolume(fileSystemProtocol, &rootFile)))
				continue;

			//
			// Open boot.efi
			//
			EFI_FILE_HANDLE bootFile										= NULL;
			EFI_STATUS openStatus											= rootFile->Open(rootFile, &bootFile, fileName, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
			rootFile->Close(rootFile);

			if (EFI_ERROR(openStatus))
				continue;

			//
			// Close it.
			//
			bootFile->Close(bootFile);

			//
			// Get device path.
			//
			EFI_DEVICE_PATH_PROTOCOL* rootDevicePath						= NULL;

            if (EFI_ERROR(status = EfiBootServices->HandleProtocol(theHandle, &EfiDevicePathProtocolGuid, (VOID**)(&rootDevicePath)))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (fileName)
                    MmFreePool(fileName);

                return status;
#endif
            }

			//
			// Load it.
			//
			EFI_DEVICE_PATH_PROTOCOL* bootFilePath							= DevPathAppendFilePath(rootDevicePath, fileName);
			EFI_HANDLE imageHandle											= NULL;

            if (EFI_ERROR(status = EfiBootServices->LoadImage(TRUE, EfiImageHandle, bootFilePath, NULL, 0, &imageHandle))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (fileName)
                    MmFreePool(fileName);

                return status;
#endif
            }

			//
			// Free file path.
			//
			MmFreePool(bootFilePath);

			//
			// Get loaded image protocol.
			//
			EFI_LOADED_IMAGE_PROTOCOL* loadedImage							= NULL;

            if (EFI_ERROR(status = EfiBootServices->HandleProtocol(imageHandle, &EfiLoadedImageProtocolGuid, (VOID**)(&loadedImage)))) {
#if defined(_MSC_VER)
                try_leave(EfiBootServices->UnloadImage(imageHandle));
#else
                EfiBootServices->UnloadImage(imageHandle);

                if (fileName)
                    MmFreePool(fileName);

                return status;
#endif
            }

			//
			// Run it.
			//
			UINTN exitDataSize												= 0;
#if defined(_MSC_VER)
			try_leave(status = EfiBootServices->StartImage(imageHandle, &exitDataSize, NULL));
#else
            status = EfiBootServices->StartImage(imageHandle, &exitDataSize, NULL);
            return status;
#endif
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (fileName)
			MmFreePool(fileName);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// Main entry point.
//
EFI_STATUS EFIAPI EfiMain(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* systemTable)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE installationFolder										= {0};
	EFI_FILE_INFO* installationFolderInfo									= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// Save handle.
		//
		EfiImageHandle														= imageHandle;
		EfiSystemTable														= systemTable;
		EfiBootServices														= systemTable->BootServices;
		EfiRuntimeServices													= systemTable->RuntimeServices;

		//
		// Stop watchdog timer.
		//
		EfiBootServices->SetWatchdogTimer(0, 0, 0, NULL);

		//
		// Memory initialisation.
		//
        if (EFI_ERROR(status = MmInitialize())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Initialise arch phase 0.
		//
        if (EFI_ERROR(status = ArchInitialize0())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Get debug options.
		//
		CHAR8* debugOptions													= NULL;
		BlpReadDebugOptions(&debugOptions);

		//
		// Init boot debugger.
		//
		//debugOptions														= CHAR8_STRING("/debug=1394 /channel=12 /break /connectall /runapple=/System/Library/CoreServices/boot.apple");
		//debugOptions														= CHAR8_STRING("/debug=1394 /channel=12 /break /connectall /connectwait=5");

        if (EFI_ERROR(status = BdInitialize(debugOptions))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Run Apple's boot.efi
		//
		CHAR8 CONST* appleBootFileName										= debugOptions ? strstr(debugOptions, CHAR8_CONST_STRING("/runapple=")) : NULL;

        if (appleBootFileName) {
#if defined(_MSC_VER)
            try_leave(status = BlpRunAppleBoot(appleBootFileName + 10));
#else
            status = BlpRunAppleBoot(appleBootFileName + 10);
            return status;
#endif
        }

		//
		// Initialise arch parse 1.
		//
        if (EFI_ERROR(status = ArchInitialize1())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Initialize console.
		//
        if (EFI_ERROR(status = CsInitialize())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Fix ROM variable.
		//
        if (EFI_ERROR(status = BlpSetupRomVariable())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Initialize device tree.
		//
        if (EFI_ERROR(status = DevTreeInitialize())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Detect memory size.
		//
        if (EFI_ERROR(status = BlDetectMemorySize())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Init platform expert.
		//
        if (EFI_ERROR(status = PeInitialize())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Check hibernate.
		//
		UINT8 coreStorageVolumeKeyIdent[16]									= {0};
		BOOLEAN resumeFromCoreStorage										= HbStartResumeFromHibernate(coreStorageVolumeKeyIdent);

		if (resumeFromCoreStorage)
			BlSetBootMode(BOOT_MODE_HIBER_FROM_FV, 0);

		//
		// Enable ASLR.
		//
		LdrSetupASLR(TRUE, 0);

		//
		// Detect hot key.
		//
        if (EFI_ERROR(status = BlDetectHotKey())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Get loaded image protocol.
		//
		EFI_LOADED_IMAGE_PROTOCOL* loadedBootImage							= NULL;

        if (EFI_ERROR(status = EfiBootServices->HandleProtocol(EfiImageHandle, &EfiLoadedImageProtocolGuid, (VOID**)(&loadedBootImage)))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Allocate buffer.
		//
		UINTN loaderOptionsSize												= (loadedBootImage->LoadOptionsSize / sizeof(CHAR16) + 1) * sizeof(CHAR8);
		CHAR8* loaderOptions												= (CHAR8*)(MmAllocatePool(loaderOptionsSize));

        if (!loaderOptions) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

		//
		// Convert unicode to UTF8.
		//
        if (EFI_ERROR(status = BlUnicodeToUtf8((CHAR16*)(loadedBootImage->LoadOptions), loadedBootImage->LoadOptionsSize / sizeof(CHAR16), loaderOptions, loaderOptionsSize / sizeof(CHAR8)))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Detect root device.
		//
		EFI_HANDLE bootDeviceHandle											= loadedBootImage->DeviceHandle;
		EFI_DEVICE_PATH_PROTOCOL* bootFilePath								= loadedBootImage->FilePath;

        if (EFI_ERROR(status = IoDetectRoot(&bootDeviceHandle, &bootFilePath, debugOptions && strstr(debugOptions, CHAR8_CONST_STRING("/detectboot"))))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Get boot device path.
		//
		EFI_DEVICE_PATH_PROTOCOL* bootDevicePath							= DevPathGetDevicePathProtocol(bootDeviceHandle);

        if (!bootDevicePath) {
#if defined(_MSC_VER)
            try_leave(status = EFI_DEVICE_ERROR);
#else
            status = EFI_DEVICE_ERROR;
            return status;
#endif
        }

		//
		// Process option.
		//
		CHAR8* kernelCommandLine											= NULL;

        if (EFI_ERROR(status = BlProcessOptions(loaderOptions, &kernelCommandLine, bootDevicePath, bootFilePath))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Check 64-bit CPU.
		//
        if (EFI_ERROR(status = ArchCheck64BitCpu())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Compact check
		//
        if (!BlTestBootMode(BOOT_MODE_SKIP_BOARD_ID_CHECK) && EFI_ERROR(status = BlpCheckBoardId(BlGetBoardId(), bootFilePath))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Createmedia installer and recovery boot detection.
		//
		//
		CHAR8* filePath														= DevPathExtractFilePathName(bootFilePath, TRUE);

		if (filePath)
		{
			if (strstr(filePath, CHAR8_CONST_STRING("\\.IABootFiles")) || strstr(filePath, CHAR8_CONST_STRING("\\OS X Install Data")) )
			{
				BlSetBootMode(BOOT_MODE_IS_INSTALLER, 0);
			}
			else if (strstr(filePath, CHAR8_CONST_STRING("com.apple.recovery.boot")))
			{
				BlSetBootMode(BOOT_MODE_FROM_RECOVER_BOOT_DIRECTORY, BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE | BOOT_MODE_BOOT_IS_NOT_ROOT);
			}

			MmFreePool(filePath);

			if (!BlTestBootMode(BOOT_MODE_IS_INSTALLER))
			{
				//
				// Legacy installer detection.
				//
				if (!EFI_ERROR(IoOpenFile(CHAR8_CONST_STRING("System\\Installation\\CDIS"), NULL, &installationFolder, IO_OPEN_MODE_NORMAL)))
				{
					//
					// Get CDIS file info.
					//
					if (!EFI_ERROR(IoGetFileInfo(&installationFolder, &installationFolderInfo)))
					{
						//
						// Check CDIS info (must be a directory).
						//
						if (installationFolderInfo)
						{
							if (installationFolderInfo->Attribute & EFI_FILE_DIRECTORY)
								BlSetBootMode(BOOT_MODE_IS_INSTALLER, 0);
						
							MmFreePool(installationFolderInfo);
						}
					}

					IoCloseFile(&installationFolder);
				}
			}
		}

		//
		// Show panic dialog.
		//
		if (!BlTestBootMode(BOOT_MODE_SKIP_PANIC_DIALOG | BOOT_MODE_FROM_RECOVER_BOOT_DIRECTORY | BOOT_MODE_HIBER_FROM_FV | BOOT_MODE_SAFE))
			BlShowPanicDialog(&kernelCommandLine);

		//
		// Run recovery.efi
		//
		if (BlTestBootMode(BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE))
			BlpRunRecoveryEfi(bootDevicePath, bootFilePath);

		//
		// Check FileVault2.
		//
		if (BlTestBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT))
			FvLookupUnlockCoreVolumeKey(bootDevicePath, resumeFromCoreStorage);

		//
		// Restore graph config.
		//
		if (!BlTestBootMode(BOOT_MODE_HAS_FILE_VAULT2_CONFIG))
			CsConnectDevice(FALSE, FALSE);

		//
		// Setup console mode.
		//
		if (BlTestBootMode(BOOT_MODE_VERBOSE))
		{
			CsSetConsoleMode(TRUE, FALSE);
		}
		else
		{
			if (!EFI_ERROR(CsInitializeGraphMode()))
				CsDrawBootImage(TRUE);
		}

		//
		// Continue hibernate.
		//
		if (resumeFromCoreStorage)
		{
			UINT8 coreStorageVolumeKey[16]									= {0};

			if (FvFindCoreVolumeKey(coreStorageVolumeKeyIdent, coreStorageVolumeKey, sizeof(coreStorageVolumeKey)))
				HbContinueResumeFromHibernate(coreStorageVolumeKey, sizeof(coreStorageVolumeKey));
		}

		//
		// Load prelinkedkernel/kernel cache.
		//
		MACH_O_LOADED_INFO kernelInfo										= {0};
		status																= LdrLoadKernelCache(&kernelInfo, bootDevicePath);
		BOOLEAN usingKernelCache											= !EFI_ERROR(status);

        if (!usingKernelCache && LdrGetKernelCacheOverride()) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Load kernel.
		//
        if (!usingKernelCache && EFI_ERROR(status = LdrLoadKernel(&kernelInfo))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Initialize boot args.
		//
		BOOT_ARGS* bootArgs													= NULL;

        if (EFI_ERROR(status = BlInitializeBootArgs(bootDevicePath, bootFilePath, bootDeviceHandle, kernelCommandLine, &bootArgs))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Load driver.
		//
        if (!usingKernelCache && EFI_ERROR(status = LdrLoadDrivers())) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Load ramdisk.
		//
		LdrLoadRamDisk();

		//
		// Console finalize.
		//
		CsFinalize();

#if (TARGET_OS == EL_CAPITAN)
		//
		// SIP configuration.
		//
        if (EFI_ERROR(BlInitCSRState(bootArgs))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }
#endif
		//
		// Finish boot args.
		//
        if (EFI_ERROR(status = BlFinalizeBootArgs(bootArgs, kernelCommandLine, bootDeviceHandle, &kernelInfo))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// Stop debugger.
		//
		BdFinalize();

		//
		// Start kernel.
		//
		ArchStartKernel(ArchConvertAddressToPointer(kernelInfo.EntryPointPhysicalAddress, VOID*), bootArgs);
#if defined(_MSC_VER)
	}
	__finally
	{
	}
#endif

	return status;
}
