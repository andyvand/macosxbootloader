//********************************************************************
//	created:	11:11:2009   23:24
//	filename: 	FileIo.cpp
//	author:		tiamo
//	purpose:	file io
//********************************************************************

#include "StdAfx.h"

//
// booting from net
//
STATIC EFI_LOAD_FILE_PROTOCOL* IopLoadFileProtocol							= nullptr;
STATIC EFI_FILE_HANDLE IopRootFile											= nullptr;

//
// find boot device
//
STATIC EFI_STATUS IopFindBootDevice(EFI_HANDLE* bootDeviceHandle, EFI_DEVICE_PATH_PROTOCOL** bootFilePath)
{
	STATIC CHAR16* checkFileName[] = 
	{
		CHAR16_STRING(L"\\.IABootFiles"),
		CHAR16_STRING(L"\\OS X Install Data"),
		CHAR16_STRING(L"\\com.apple.recovery.boot"),
		CHAR16_STRING(L"\\System\\Library\\Kernels\\kernel"),
		CHAR16_STRING(L"\\com.apple.boot.R"),
		CHAR16_STRING(L"\\com.apple.boot.P"),
		CHAR16_STRING(L"\\com.apple.boot.S")
	};

	STATIC CHAR16* booterName[] = 
	{
		CHAR16_STRING(L"\\.IABootFiles\\boot.efi"),
		CHAR16_STRING(L"\\OS X Install Data\\boot.efi"),
		CHAR16_STRING(L"\\com.apple.recovery.boot\\boot.efi"),
		CHAR16_STRING(L"\\System\\Library\\CoreServices\\boot.efi"),
		CHAR16_STRING(L"\\System\\Library\\CoreServices\\boot.efi"),
		CHAR16_STRING(L"\\System\\Library\\CoreServices\\boot.efi"),
		CHAR16_STRING(L"\\System\\Library\\CoreServices\\boot.efi")
	};

	STATIC UINT8 bootFilePathBuffer[256]									= {0};
	EFI_HANDLE foundHandle[ARRAYSIZE(checkFileName)]						= {0};
	EFI_HANDLE* handleArray													= nullptr;
	EFI_STATUS status														= EFI_NOT_FOUND;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// search all block device
		//
		UINTN handleCount													= 0;
		EfiBootServices->LocateHandleBuffer(ByProtocol, &EfiSimpleFileSystemProtocolGuid, nullptr, &handleCount, &handleArray);

		for (UINTN i = 0; i < handleCount; i ++)
		{
			EFI_HANDLE theHandle											= handleArray[i];

			if (!theHandle)
				continue;

			//
			// get file system protocol
			//
			EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystemProtocol				= nullptr;

			if (EFI_ERROR(EfiBootServices->HandleProtocol(theHandle, &EfiSimpleFileSystemProtocolGuid, reinterpret_cast<VOID**>(&fileSystemProtocol))))
				continue;

			//
			// open root directory
			//
			EFI_FILE_HANDLE rootFile										= nullptr;

			if (EFI_ERROR(fileSystemProtocol->OpenVolume(fileSystemProtocol, &rootFile)))
				continue;

			//
			// check file exist
			//
			for (UINTN j = 0; j < ARRAYSIZE(checkFileName); j ++)
			{
				EFI_FILE_HANDLE checkFile									= nullptr;
				BOOLEAN fileExist											= !EFI_ERROR(rootFile->Open(rootFile, &checkFile, checkFileName[j], EFI_FILE_MODE_READ, 0));

				if (!fileExist)
					continue;

				foundHandle[j]												= theHandle;
				checkFile->Close(checkFile);
			}
			rootFile->Close(rootFile);
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (handleArray)
			MmFreePool(handleArray);

		for (UINTN i = 0; i < ARRAYSIZE(foundHandle); i ++)
		{
			if (!foundHandle[i])
				continue;

			FILEPATH_DEVICE_PATH* filePath									= reinterpret_cast<FILEPATH_DEVICE_PATH*>(bootFilePathBuffer);
			*bootFilePath													= &filePath->Header;
			filePath->Header.Type											= MEDIA_DEVICE_PATH;
			filePath->Header.SubType										= MEDIA_FILEPATH_DP;
			status															= EFI_SUCCESS;
			*bootDeviceHandle												= foundHandle[i];
			UINTN size														= (wcslen(booterName[i]) + 1) * sizeof(CHAR16);
			SetDevicePathNodeLength(&filePath->Header, size + SIZE_OF_FILEPATH_DEVICE_PATH);
			EFI_DEVICE_PATH_PROTOCOL* endOfPath								= NextDevicePathNode(&filePath->Header);
			memcpy(filePath->PathName, booterName[i], size);
			SetDevicePathEndNode(endOfPath);
			break;
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// check PRS
//
STATIC EFI_STATUS IopCheckRPS(EFI_FILE_HANDLE rootFile, EFI_FILE_HANDLE* realRootFile)
{
	EFI_STATUS retValue														= EFI_SUCCESS;
	*realRootFile															= rootFile;

	EFI_FILE_HANDLE fileR													= nullptr;
	EFI_STATUS startR														= rootFile->Open(rootFile, &fileR, CHAR16_STRING(L"com.apple.boot.R"), EFI_FILE_MODE_READ, 0);

	EFI_FILE_HANDLE fileP													= nullptr;
	EFI_STATUS startP														= rootFile->Open(rootFile, &fileP, CHAR16_STRING(L"com.apple.boot.P"), EFI_FILE_MODE_READ, 0);

	EFI_FILE_HANDLE fileS													= nullptr;
	EFI_STATUS startS														= rootFile->Open(rootFile, &fileS, CHAR16_STRING(L"com.apple.boot.S"), EFI_FILE_MODE_READ, 0);

	if (!EFI_ERROR(startR) && !EFI_ERROR(startP) && !EFI_ERROR(startS))
		*realRootFile														= fileR;
	else if (!EFI_ERROR(startR) && !EFI_ERROR(startP))
		*realRootFile														= fileP;
	else if (!EFI_ERROR(startR) && !EFI_ERROR(startS))
		*realRootFile														= fileR;
	else if (!EFI_ERROR(startS) && !EFI_ERROR(startP))
		*realRootFile														= fileS;
	else if (!EFI_ERROR(startR))
		*realRootFile														= fileR;
	else if (!EFI_ERROR(startP))
		*realRootFile														= fileP;
	else if (!EFI_ERROR(startS))
		*realRootFile														= fileS;
	else
		retValue															= EFI_NOT_FOUND;

	if (!EFI_ERROR(startR) && *realRootFile != fileR)
		fileR->Close(fileR);
	if (!EFI_ERROR(startP) && *realRootFile != fileP)
		fileP->Close(fileP);
	if (!EFI_ERROR(startS) && *realRootFile != fileS)
		fileS->Close(fileS);

	return retValue;
}

//
// detect root
//
STATIC EFI_STATUS IopDetectRoot(EFI_HANDLE deviceHandle, EFI_DEVICE_PATH_PROTOCOL* bootFilePath, BOOLEAN allowBootDirectory)
{
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_FILE_HANDLE kernelFile												= nullptr;
	EFI_FILE_HANDLE realRootFile											= nullptr;
	CHAR8* bootFullPath														= nullptr;
	CHAR16* bootFullPath16													= nullptr;
	IopRootFile																= nullptr;
	IopLoadFileProtocol														= nullptr;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// check simple file system protocol or load file protocol
		//
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystemProtocol					= nullptr;

		if (EFI_ERROR(status = EfiBootServices->HandleProtocol(deviceHandle, &EfiSimpleFileSystemProtocolGuid, reinterpret_cast<VOID**>(&fileSystemProtocol))))
#if defined(_MSC_VER)
			try_leave(status = EfiBootServices->HandleProtocol(deviceHandle, &EfiLoadFileProtocolGuid, reinterpret_cast<VOID**>(&IopLoadFileProtocol)));
#else
        status = EfiBootServices->HandleProtocol(deviceHandle, &EfiLoadFileProtocolGuid, reinterpret_cast<VOID**>(&IopLoadFileProtocol));
        return status;
#endif

		//
		// open root directory
		//
		if (EFI_ERROR(status = fileSystemProtocol->OpenVolume(fileSystemProtocol, &IopRootFile)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// check kernel in Kernels directory
		//
		if (!EFI_ERROR(status = IopRootFile->Open(IopRootFile, &kernelFile, CHAR16_STRING(L"System\\Library\\Kernels\\kernel"), EFI_FILE_MODE_READ, 0)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// detect RPS
		//
        if (!EFI_ERROR(status = IopCheckRPS(IopRootFile, &realRootFile))) {
#if defined(_MSC_VER)
            try_leave(BlSetBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT, 0));
#else
            BlSetBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT, 0);
            return -1;
#endif
        }

		//
		// check boot directory
		//
        if (!allowBootDirectory) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;
            return status;
#endif
        }
		//
		// get booter's full path
		//
		status																= EFI_SUCCESS;
		bootFullPath														= DevPathExtractFilePathName(bootFilePath, TRUE);

		if (!bootFullPath || (bootFullPath[0] != '/' && bootFullPath[0] != '\\') || strlen(bootFullPath) == 1)
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// get current directory
		//
		CHAR8* lastComponent												= nullptr;

		for (UINTN i = 1; bootFullPath[i]; i ++)
		{
			if (bootFullPath[i] == '\\' || bootFullPath[i] == '/')
				lastComponent												= bootFullPath + i;
		}

		if (lastComponent)
			*lastComponent													= 0;
		else
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// open it
		//
		bootFullPath16														= BlAllocateUnicodeFromUtf8(bootFullPath, lastComponent - bootFullPath);

        if (!bootFullPath16) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

        if (EFI_ERROR(status = IopRootFile->Open(IopRootFile, &realRootFile, bootFullPath16, EFI_FILE_MODE_READ, 0))) {
#if defined(_MSC_VER)
            try_leave(realRootFile = nullptr);
#else
            realRootFile = nullptr;
            return -1;
#endif
        }

		//
		// check EncryptedRoot.plist.wipekey
		//
		if (!EFI_ERROR(realRootFile->Open(realRootFile, &kernelFile, CHAR16_STRING(L"EncryptedRoot.plist.wipekey"), EFI_FILE_MODE_READ, 0)))
			BlSetBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT, 0);
		else
			realRootFile->Close(realRootFile), realRootFile = nullptr;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (bootFullPath)
			MmFreePool(bootFullPath);

		if (bootFullPath16)
			MmFreePool(bootFullPath16);

		if (kernelFile)
			kernelFile->Close(kernelFile);

		if (realRootFile && realRootFile != IopRootFile)
			IopRootFile->Close(IopRootFile), IopRootFile = realRootFile;

		if (IopRootFile && EFI_ERROR(status))
			IopRootFile->Close(IopRootFile), IopRootFile = nullptr;
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// load booter with root uuid
//
STATIC EFI_STATUS IopLoadBooterWithRootUUID(EFI_DEVICE_PATH_PROTOCOL* bootFilePath, EFI_HANDLE theHandle, CHAR8 CONST* rootUUID, EFI_HANDLE* imageHandle)
{
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_FILE_HANDLE savedRootFile											= IopRootFile;
	EFI_DEVICE_PATH_PROTOCOL* recoveryFilePath								= nullptr;
	CHAR8* fileBuffer														= nullptr;
	XML_TAG* rootTag														= nullptr;
	EFI_FILE_HANDLE rootFile												= nullptr;
	*imageHandle															= nullptr;
	IopRootFile																= nullptr;

#if defined(_MSC_VER)
    __try
	{
#endif
		//
		// get device path
		//
		EFI_DEVICE_PATH_PROTOCOL* devicePath								= DevPathGetDevicePathProtocol(theHandle);

        if (!devicePath) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;
            return status;
#endif
        }

		//
		// build recovery file path
		//
		recoveryFilePath													= DevPathAppendFilePath(devicePath, CHAR16_CONST_STRING(L"\\com.apple.recovery.boot\\boot.efi"));

        if (!recoveryFilePath) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;
            return status;
#endif
        }

		//
		// check file exist
		//
		if (EFI_ERROR(status = EfiBootServices->LoadImage(FALSE, EfiImageHandle, recoveryFilePath, nullptr, 0, imageHandle)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// get file system protocol
		//
		EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystemProtocol					= nullptr;

		if (EFI_ERROR(status = EfiBootServices->HandleProtocol(theHandle, &EfiSimpleFileSystemProtocolGuid, reinterpret_cast<VOID**>(&fileSystemProtocol))))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// open root directory
		//
		if (EFI_ERROR(status = fileSystemProtocol->OpenVolume(fileSystemProtocol, &rootFile)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// detect real root
		//
		if (EFI_ERROR(status = IopCheckRPS(rootFile, &IopRootFile)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// load boot file
		//
		if (EFI_ERROR(status = IoReadWholeFile(bootFilePath, CHAR8_CONST_STRING("Library\\Preferences\\SystemConfiguration\\com.apple.Boot.plist"), &fileBuffer, nullptr, TRUE)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// parse it
		//
		if (EFI_ERROR(status = CmParseXmlFile(fileBuffer, &rootTag)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// read root UUID
		//
		CHAR8 CONST* theRootUUID											= CmGetStringValueForKey(rootTag, CHAR8_CONST_STRING("Root UUID"), nullptr);

        if (!theRootUUID) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;
            return status;
#endif
        }

		//
		// check is the same
		//
		status																= strcmp(theRootUUID, rootUUID) ? EFI_NOT_FOUND : EFI_SUCCESS;
#if defined(_MSC_VER)
    }
	__finally
	{
#endif
		if (recoveryFilePath)
			MmFreePool(recoveryFilePath);
		if (fileBuffer)
			MmFreePool(fileBuffer);
		if (rootTag)
			CmFreeTag(rootTag);

		if (EFI_ERROR(status) && *imageHandle)
			EfiBootServices->UnloadImage(*imageHandle);

		if (IopRootFile != rootFile)
			IopRootFile->Close(IopRootFile);

		if (rootFile)
			rootFile->Close(rootFile);

		IopRootFile															= savedRootFile;
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// detect root
//
EFI_STATUS IoDetectRoot(EFI_HANDLE* deviceHandle, EFI_DEVICE_PATH_PROTOCOL** bootFilePath, BOOLEAN detectBoot)
{
	if (detectBoot && (!EFI_ERROR(IopDetectRoot(*deviceHandle, *bootFilePath, FALSE)) || !EFI_ERROR(IopFindBootDevice(deviceHandle, bootFilePath))))
		return EFI_SUCCESS;

	return IopDetectRoot(*deviceHandle, *bootFilePath, TRUE);
}

//
// load booter
//
EFI_STATUS IoLoadBooterWithRootUUID(EFI_DEVICE_PATH_PROTOCOL* bootFilePath, CHAR8 CONST* rootUUID, EFI_HANDLE* imageHandle)
{
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_HANDLE* handleArray													= nullptr;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// search all block device
		//
		CsConnectDevice(TRUE, FALSE);
		UINTN handleCount													= 0;
		EfiBootServices->LocateHandleBuffer(ByProtocol, &EfiBlockIoProtocolGuid, nullptr, &handleCount, &handleArray);

		for (UINTN i = 0; i < handleCount; i ++)
		{
			EFI_HANDLE theHandle											= handleArray[i];

			if (!theHandle)
				continue;

			if (!EFI_ERROR(status = IopLoadBooterWithRootUUID(bootFilePath, theHandle, rootUUID, imageHandle)))
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                return -1;
#endif
		}
		status																= EFI_NOT_FOUND;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (handleArray)
			MmFreePool(handleArray);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// booting from net
//
BOOLEAN IoBootingFromNet()
{
	return !IopRootFile && IopLoadFileProtocol;
}

//
// open file
//
EFI_STATUS IoOpenFile(CHAR8 CONST* filePathName, EFI_DEVICE_PATH_PROTOCOL* filePath, IO_FILE_HANDLE* fileHandle, UINTN openMode)
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		memset(fileHandle, 0, sizeof(IO_FILE_HANDLE));

		if (IopLoadFileProtocol)
		{
			if (filePath)
			{
				fileHandle->EfiLoadFileProtocol								= IopLoadFileProtocol;
				fileHandle->EfiFilePath										= DevPathDuplicate(filePath);
#if defined(_MSC_VER)
				try_leave(status = fileHandle->EfiFilePath ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES);
#else
                status = (fileHandle->EfiFilePath) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
                return status;
#endif
			}

            if (!filePathName || !filePathName[0]) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;
                return status;
#endif
            }

            if (openMode == IO_OPEN_MODE_RAMDISK) {
#if defined(_MSC_VER)
                try_leave(status = EFI_UNSUPPORTED);
#else
                status = EFI_UNSUPPORTED;
                return status;
#endif
            }
		}
		else if (filePath)
		{
            if (openMode == IO_OPEN_MODE_KERNEL) {
#if defined(_MSC_VER)
                try_leave(status = EFI_UNSUPPORTED);
#else
                status = EFI_UNSUPPORTED;
                return status;
#endif
            }
		}
		else if ((!filePathName || !filePathName[0] || !IopRootFile))
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;
            return status;
#endif
		}

		STATIC CHAR16 unicodeFilePathName[1024]								= {0};
		UINTN nameLength													= filePathName ? strlen(filePathName) : 0;
		BlUtf8ToUnicode(filePathName, nameLength, unicodeFilePathName, ARRAYSIZE(unicodeFilePathName) - 1);

		if (IopLoadFileProtocol)
		{
			UINTN filePathNodeSize											= (nameLength + 1) * sizeof(CHAR16) + SIZE_OF_FILEPATH_DEVICE_PATH;
			FILEPATH_DEVICE_PATH* fileDevicePath							= static_cast<FILEPATH_DEVICE_PATH*>(MmAllocatePool(filePathNodeSize + END_DEVICE_PATH_LENGTH));
            if (!fileDevicePath) {
#if defined(_MSC_VER)
                try_leave(status = EFI_OUT_OF_RESOURCES);
#else
                status = EFI_OUT_OF_RESOURCES;
                return status;
#endif
            }

			fileDevicePath->Header.Type										= MEDIA_DEVICE_PATH;
			fileDevicePath->Header.SubType									= MEDIA_FILEPATH_DP;
			SetDevicePathNodeLength(&fileDevicePath->Header, filePathNodeSize);
			memcpy(fileDevicePath->PathName, unicodeFilePathName, (nameLength + 1) * sizeof(CHAR16));

			EFI_DEVICE_PATH_PROTOCOL* endNode								= EfiNextDevicePathNode(&fileDevicePath->Header);
			SetDevicePathEndNode(endNode);

			fileHandle->EfiLoadFileProtocol									= IopLoadFileProtocol;
			fileHandle->EfiFilePath											= &fileDevicePath->Header;
		}
		else
		{
			status															= IopRootFile->Open(IopRootFile, &fileHandle->EfiFileHandle, unicodeFilePathName, EFI_FILE_MODE_READ, 0);
		}
#if defined(_MSC_VER)
	}
	__finally
	{
	}
#endif

	return status;
}

//
// set position
//
EFI_STATUS IoSetFilePosition(IO_FILE_HANDLE* fileHandle, UINT64 filePosition)
{
	if (fileHandle->EfiFileHandle)
		return fileHandle->EfiFileHandle->SetPosition(fileHandle->EfiFileHandle, filePosition);

	fileHandle->FileOffset													= static_cast<UINTN>(filePosition);

	return EFI_SUCCESS;
}

//
// get file size
//
EFI_STATUS IoGetFileSize(IO_FILE_HANDLE* fileHandle, UINT64* fileSize)
{
	*fileSize																= 0;

	if (fileHandle->EfiLoadFileProtocol)
	{
		if (!fileHandle->FileSize)
		{
			EFI_STATUS status												= fileHandle->EfiLoadFileProtocol->LoadFile(fileHandle->EfiLoadFileProtocol, fileHandle->EfiFilePath, FALSE, &fileHandle->FileSize, nullptr);

			if (status != EFI_BUFFER_TOO_SMALL)
				return status;
		}

		*fileSize															= fileHandle->FileSize;

		return EFI_SUCCESS;
	}

	if (fileHandle->FileBuffer)
	{
		*fileSize															= fileHandle->FileSize;

		return EFI_SUCCESS;
	}

	if (fileHandle->EfiFileHandle)
	{
		EFI_FILE_INFO* fileInfo												= nullptr;
		EFI_STATUS status													= IoGetFileInfo(fileHandle, &fileInfo);

		if (EFI_ERROR(status) || !fileInfo)
			return status;

		*fileSize															= fileInfo->FileSize;
		MmFreePool(fileInfo);

		return EFI_SUCCESS;
	}

	return EFI_INVALID_PARAMETER;
}

//
// get file info
//
EFI_STATUS IoGetFileInfo(IO_FILE_HANDLE* fileHandle, EFI_FILE_INFO** fileInfo)
{
	*fileInfo																= nullptr;

	if (!fileHandle->EfiFileHandle)
		return EFI_INVALID_PARAMETER;

	UINTN infoSize															= SIZE_OF_EFI_FILE_INFO + sizeof(CHAR16) * 64;
	EFI_FILE_INFO* infoBuffer												= static_cast<EFI_FILE_INFO*>(MmAllocatePool(infoSize));

	if (!infoBuffer)
		return EFI_OUT_OF_RESOURCES;

	EFI_STATUS status														= fileHandle->EfiFileHandle->GetInfo(fileHandle->EfiFileHandle, &EfiFileInfoGuid, &infoSize, infoBuffer);

	if (status == EFI_BUFFER_TOO_SMALL)
	{
		MmFreePool(infoBuffer);
		infoBuffer															= static_cast<EFI_FILE_INFO*>(MmAllocatePool(infoSize));
		status																= fileHandle->EfiFileHandle->GetInfo(fileHandle->EfiFileHandle, &EfiFileInfoGuid, &infoSize, infoBuffer);
	}

	if (EFI_ERROR(status))
		MmFreePool(infoBuffer);
	else
		*fileInfo															= infoBuffer;

	return status;
}

//
// read file
//
EFI_STATUS IoReadFile(IO_FILE_HANDLE* fileHandle, VOID* readBuffer, UINTN bufferSize, UINTN* readLength, BOOLEAN directoryFile)
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
    __try
	{
#endif
		*readLength															= 0;

		if (fileHandle->EfiLoadFileProtocol)
		{
			UINT64 fileSize													= 0;

			if (EFI_ERROR(status = IoGetFileSize(fileHandle, &fileSize)))
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                return -1;
#endif

            if (!fileHandle->FileBuffer && !fileHandle->FileOffset && bufferSize == fileSize) {
#if defined(_MSC_VER)
                try_leave(*readLength = bufferSize; status = fileHandle->EfiLoadFileProtocol->LoadFile(fileHandle->EfiLoadFileProtocol, fileHandle->EfiFilePath, FALSE, readLength, readBuffer));
#else
                *readLength = bufferSize;
                status = fileHandle->EfiLoadFileProtocol->LoadFile(fileHandle->EfiLoadFileProtocol, fileHandle->EfiFilePath, FALSE, readLength, readBuffer);
                return status;
#endif
            }

			if (fileSize <= fileHandle->FileOffset)
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                return -1;
#endif

			fileHandle->FileSize											= static_cast<UINTN>(fileSize);

			if (!fileHandle->FileBuffer)
			{
				fileHandle->FileBuffer										= static_cast<UINT8*>(MmAllocatePool(static_cast<UINTN>(fileSize)));

                if (!fileHandle->FileBuffer) {
#if defined(_MSC_VER)
                    try_leave(status = EFI_OUT_OF_RESOURCES);
#else
                    status = EFI_OUT_OF_RESOURCES;
                    return status;
#endif
                }

				UINTN read_length											= static_cast<UINTN>(fileSize);
				status														= fileHandle->EfiLoadFileProtocol->LoadFile(fileHandle->EfiLoadFileProtocol, fileHandle->EfiFilePath, FALSE, &read_length, fileHandle->FileBuffer);

				if (EFI_ERROR(status))
#if defined(_MSC_VER)
					try_leave(NOTHING);
#else
                    return -1;
#endif

                if (read_length != fileHandle->FileSize) {
#if defined(_MSC_VER)
                    try_leave(MmFreePool(fileHandle->FileBuffer); fileHandle->FileBuffer = nullptr; status = EFI_DEVICE_ERROR);
#else
                    MmFreePool(fileHandle->FileBuffer);
                    fileHandle->FileBuffer = nullptr;
                    status = EFI_DEVICE_ERROR;
                    return status;
#endif
                }
			}
		}

		if (fileHandle->FileBuffer)
		{
			UINTN copyLength												= fileHandle->FileOffset >= fileHandle->FileSize ? 0 : fileHandle->FileSize - fileHandle->FileOffset;

			if (copyLength > bufferSize)
				copyLength													= bufferSize;

			memcpy(readBuffer, fileHandle->FileBuffer + fileHandle->FileOffset, copyLength);
			fileHandle->FileOffset											+= copyLength;
			*readLength														= copyLength;
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif
		}

        if (!fileHandle->EfiFileHandle) {
#if defined(_MSC_VER)
            try_leave(status = EFI_INVALID_PARAMETER);
#else
            status = EFI_INVALID_PARAMETER;
            return status;
#endif
        }

		UINT8* curBuffer													= static_cast<UINT8*>(readBuffer);
		while(bufferSize)
		{
			UINTN lengthThisRun												= bufferSize > 1024 * 1024 ? 1024 * 1024 : bufferSize;

			if (EFI_ERROR(status = fileHandle->EfiFileHandle->Read(fileHandle->EfiFileHandle, &lengthThisRun, curBuffer)) || !lengthThisRun)
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                return -1;
#endif

			bufferSize														-= lengthThisRun;
			curBuffer														+= lengthThisRun;
			*readLength														+= lengthThisRun;

			if (directoryFile)
				break;
		}
#if defined(_MSC_VER)
	}
	__finally
	{
	}
#endif

	return status;
}

//
// close file
//
VOID IoCloseFile(IO_FILE_HANDLE* fileHandle)
{
	if (fileHandle->FileBuffer)
		MmFreePool(fileHandle->FileBuffer);

	if (fileHandle->EfiFilePath)
		MmFreePool(fileHandle->EfiFilePath);

	if (fileHandle->EfiFileHandle)
		fileHandle->EfiFileHandle->Close(fileHandle->EfiFileHandle);

	memset(fileHandle, 0, sizeof(IO_FILE_HANDLE));
}

//
// read whole file
//
EFI_STATUS IoReadWholeFile(EFI_DEVICE_PATH_PROTOCOL* bootFilePath, CHAR8 CONST* fileName, CHAR8** fileBuffer, UINTN* fileSize, BOOLEAN asTextFile)
{
	EFI_STATUS status														= EFI_SUCCESS;
	*fileBuffer																= nullptr;
	IO_FILE_HANDLE fileHandle												= {0};
	EFI_DEVICE_PATH_PROTOCOL* filePath										= nullptr;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// build file path
		//
		if (IoBootingFromNet() && bootFilePath)
			filePath														= DevPathAppendLastComponent(bootFilePath, fileName, TRUE);

		//
		// open file
		//
		if (EFI_ERROR(status = IoOpenFile(fileName, filePath, &fileHandle, IO_OPEN_MODE_NORMAL)))
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// get file size
		//
		UINT64 localFileSize												= 0;
		if (EFI_ERROR(status = IoGetFileSize(&fileHandle, &localFileSize)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// allocate buffer
		//
		UINTN totalSize														= static_cast<UINTN>(localFileSize) + (asTextFile ? sizeof(CHAR8) : 0);
		*fileBuffer															= static_cast<CHAR8*>(MmAllocatePool(totalSize));

        if (!*fileBuffer) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

		//
		// read file
		//
		UINTN readLength													= 0;

		if (EFI_ERROR(status = IoReadFile(&fileHandle, *fileBuffer, static_cast<UINTN>(localFileSize), &readLength, FALSE)))
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return -1;
#endif

		//
		// append NULL
		//
		if (asTextFile)
			(*fileBuffer)[readLength / sizeof(CHAR8)]						= 0;

		if (fileSize)
			*fileSize														= static_cast<UINTN>(localFileSize);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
        IoCloseFile(&fileHandle);

		if (filePath)
			MmFreePool(filePath);

		if (EFI_ERROR(status))
		{
			if (*fileBuffer)
				MmFreePool(*fileBuffer);

			*fileBuffer														= nullptr;

			if (fileSize)
				*fileSize													= 0;
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}
