//********************************************************************
//	created:	11:11:2009   21:33
//	filename: 	LoadDrivers.cpp
//	author:		tiamo
//	purpose:	load drivers
//********************************************************************

#include "StdAfx.h"

#ifndef MKEXT_MAGIC
#define MKEXT_MAGIC															0x4d4b5854
#endif /*  MKEXT_MAGIC */

#ifndef MKEXT_SIGN
#define MKEXT_SIGN															0x4d4f5358
#endif /* MKEXT_SIGN */

//
// module
//
#if defined(_MSC_VER)
#include <pshpack1.h>
#endif

typedef struct _LDR_EXTENSION_MODULE
{
	//
	// next
	//
	struct _LDR_EXTENSION_MODULE*											NextModule;

	//
	// load
	//
	INTN																	WillLoad;

	//
	// info.plist tag
	//
	XML_TAG*																InfoPlistTag;

	//
	// plist buffer
	//
	CHAR8*																	InfoPlistBuffer;

	//
	// length
	//
	UINTN																	InfoPlistLength;

	//
	// driver path
	//
	CHAR8*																	DriverPath;

	//
	// driver path length
	//
	UINTN																	DriverPathLength;
}LDR_EXTENSION_MODULE;

//
// driver info
//
typedef struct _LDR_DRIVER_INFO
{
	//
	// plist buffer
	//
	UINT32																	InfoPlistAddress;

	//
	// length
	//
	UINT32																	InfoPlistLength;

	//
	// module address
	//
	UINT32																	ModuleAddress;

	//
	// module length
	//
	UINT32																	ModuleLength;

	//
	// driver path
	//
	UINT32																	DriverPath;

	//
	// path length
	//
	UINT32																	DriverPathLength;
}LDR_DRIVER_INFO;

#if defined(_MSC_VER)
#include <poppack.h>
#endif

//
// global
//
STATIC LDR_EXTENSION_MODULE* LdrpExtensionModulesHead						= NULL;
STATIC LDR_EXTENSION_MODULE* LdrpExtensionModulesTail						= NULL;

//
// parse driver info plist
//
STATIC EFI_STATUS LdrpParseDriverInfoPlist(CHAR8* fileBuffer, LDR_EXTENSION_MODULE** theModuleP)
{
	XML_TAG* moduleDict														= NULL;
	EFI_STATUS status														= EFI_SUCCESS;
#if defined(_MSC_VER)
	__try
	{
#endif
        if(EFI_ERROR(status = CmParseXmlFile(fileBuffer, &moduleDict))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(moduleDict)
                CmFreeTag(moduleDict);

            return status;
#endif
        }

		XML_TAG* requiredProperty											= CmGetTagValueForKey(moduleDict, CHAR8_CONST_STRING("OSBundleRequired"));
        if(!requiredProperty || requiredProperty->Type != XML_TAG_STRING || !strcmp(requiredProperty->StringValue, CHAR8_CONST_STRING("Safe Boot"))) {
#if defined(_MSC_VER)
            try_leave(status = EFI_INVALID_PARAMETER);
#else
            status = EFI_INVALID_PARAMETER;

            if(moduleDict)
                CmFreeTag(moduleDict);

            return status;
#endif
        }

		LDR_EXTENSION_MODULE* theModule										= (LDR_EXTENSION_MODULE*)(MmAllocatePool(sizeof(LDR_EXTENSION_MODULE)));
        if(!theModule) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(moduleDict)
                CmFreeTag(moduleDict);

            return status;
#endif
        }

		theModule->InfoPlistTag												= moduleDict;
		theModule->WillLoad													= 1;
		*theModuleP															= theModule;
		moduleDict															= NULL;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(moduleDict)
			CmFreeTag(moduleDict);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// load plist
//
STATIC EFI_STATUS LdrpLoadDriverPList(CHAR8 CONST* extensionDir, CHAR8 CONST* fileName, BOOLEAN inContentsFolder)
{
	EFI_STATUS status														= EFI_SUCCESS;
	CHAR8* driverDirectory													= NULL;
	CHAR8* fileBuffer														= NULL;
	CHAR8* fileBuffer2														= NULL;
	IO_FILE_HANDLE fileHandle												= {0};

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// allocate dir buffer
		//
		STATIC CHAR8 CONST contentsMacOSDir[]								= "Contents\\MacOS\\";
		STATIC CHAR8 CONST contentsDir[]									= "Contents\\";
		UINTN filePathLength												= (strlen(extensionDir) + 1 + strlen(fileName) + 1 + (inContentsFolder ? ARRAYSIZE(contentsMacOSDir) : 1)) * sizeof(CHAR8);
		driverDirectory														= (CHAR8*)(MmAllocatePool(filePathLength));
        if(!driverDirectory) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            return status;
#endif
        }

		//
		// build directory
		//
		snprintf(driverDirectory, filePathLength / sizeof(CHAR8), CHAR8_CONST_STRING("%s\\%s\\%s"), extensionDir, fileName, inContentsFolder ? contentsMacOSDir : CHAR8_CONST_STRING(""));

		//
		// build file name
		//
		STATIC CHAR8 fullFileName[1025]										= {0};
		snprintf(fullFileName, ARRAYSIZE(fullFileName), CHAR8_CONST_STRING("%s\\%s\\%sInfo.plist"), extensionDir, fileName, inContentsFolder ? contentsDir : CHAR8_CONST_STRING(""));

		//
		// open file
		//
        if(EFI_ERROR(status = IoOpenFile(fullFileName, NULL, &fileHandle, IO_OPEN_MODE_NORMAL))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// get file length
		//
		UINT64 fileLength													= 0;
        if(EFI_ERROR(status = IoGetFileSize(&fileHandle, &fileLength))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// allocate buffer
		//
		fileBuffer															= (CHAR8*)(MmAllocatePool((UINTN)(fileLength) + sizeof(CHAR8)));
        if(!fileBuffer) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// read file
		//
		UINTN readLength													= 0;
        if(EFI_ERROR(status = IoReadFile(&fileHandle, fileBuffer, (UINTN)(fileLength), &readLength, FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// length check
		//
        if(readLength != fileLength) {
#if defined(_MSC_VER)
            try_leave(status = EFI_DEVICE_ERROR);
#else
            status = EFI_DEVICE_ERROR;

            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// set NULL-terminated
		//
		fileBuffer[fileLength]												= 0;

		//
		// allocate buffer
		//
		fileBuffer2															= (CHAR8*)(MmAllocatePool((UINTN)(fileLength) + sizeof(CHAR8)));

        if(!fileBuffer2) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		memcpy(fileBuffer2, fileBuffer, (UINTN)(fileLength) + sizeof(CHAR8));

		//
		// parse the file
		//
		LDR_EXTENSION_MODULE* theModule										= NULL;
        if(EFI_ERROR(status = LdrpParseDriverInfoPlist(fileBuffer, &theModule))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(fileBuffer)
                MmFreePool(fileBuffer);

            if(fileBuffer2)
                MmFreePool(fileBuffer2);

            if(driverDirectory)
                MmFreePool(driverDirectory);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// save buffers
		//
		theModule->DriverPath												= driverDirectory;
		theModule->DriverPathLength											= strlen(theModule->DriverPath) + 1;
		theModule->InfoPlistBuffer											= fileBuffer2;
		theModule->InfoPlistLength											= (UINTN)(fileLength) + sizeof(CHAR8);
		driverDirectory														= NULL;
		fileBuffer2															= NULL;

		//
		// insert into list
		//
		if(!LdrpExtensionModulesHead)
			LdrpExtensionModulesHead										= theModule;
		else
			LdrpExtensionModulesTail->NextModule							= theModule;

		LdrpExtensionModulesTail											= theModule;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(fileBuffer)
			MmFreePool(fileBuffer);

		if(fileBuffer2)
			MmFreePool(fileBuffer2);

		if(driverDirectory)
			MmFreePool(driverDirectory);

		IoCloseFile(&fileHandle);
#if defined(_MSC_VER)
    }
#endif

	return status;
}

//
// load drivers
//
STATIC EFI_STATUS LdrpLoadDrivers(CHAR8 CONST* extensionsDirectory, BOOLEAN isPluginModule)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// open directory
		//
        if(EFI_ERROR(status = IoOpenFile(extensionsDirectory, NULL, &fileHandle, IO_OPEN_MODE_NORMAL))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		while(TRUE)
		{
			//
			// read dir entry
			//
			STATIC CHAR8 localBuffer[sizeof(EFI_FILE_INFO) + 0x400]			= {0};
			EFI_FILE_INFO* fileInfo											= (EFI_FILE_INFO*)((VOID*)(localBuffer));
			UINTN readLength												= 0;
			status															= IoReadFile(&fileHandle, fileInfo, sizeof(localBuffer), &readLength, TRUE);
            if(EFI_ERROR(status) || !readLength) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// direcotry attribute
			//
			if(!(fileInfo->Attribute & EFI_FILE_DIRECTORY))
				continue;

			//
			// check .kext
			//
			UINTN fileNameLength											= wcslen(fileInfo->FileName);
			if( fileNameLength < 5 || fileInfo->FileName[fileNameLength - 5] != L'.' || fileInfo->FileName[fileNameLength - 4] != L'k' ||
				fileInfo->FileName[fileNameLength - 3] != L'e' || fileInfo->FileName[fileNameLength - 2] != L'x' || fileInfo->FileName[fileNameLength - 1] != L't')
			{
				continue;
			}

			//
			// convert to ansi
			//
			STATIC CHAR8 utf8FileName[1024]									= {0};
            if(EFI_ERROR(status = BlUnicodeToUtf8(fileInfo->FileName, fileNameLength, utf8FileName, ARRAYSIZE(utf8FileName) - 1))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// check content folder
			//
			IO_FILE_HANDLE tempFileHandle									= {0};
			snprintf(localBuffer, ARRAYSIZE(localBuffer) - 1, CHAR8_CONST_STRING("%s\\%s\\Contents"), extensionsDirectory, utf8FileName);
			BOOLEAN hasContentsFolder										= EFI_ERROR(IoOpenFile(localBuffer, NULL, &tempFileHandle, IO_OPEN_MODE_NORMAL)) ? FALSE : TRUE;
			IoCloseFile(&tempFileHandle);

			//
			// build plugin directory
			//
			STATIC CHAR8 pluginBuffer[1024]									= {0};
			if(!isPluginModule)
				snprintf(pluginBuffer, ARRAYSIZE(pluginBuffer) - 1, CHAR8_CONST_STRING("%s\\%s\\%sPlugIns"), extensionsDirectory, utf8FileName, hasContentsFolder ? "Contents\\" : "");

			//
			// load plist
			//
			LdrpLoadDriverPList(extensionsDirectory, utf8FileName, hasContentsFolder);

			//
			// load plugins
			//
			if(!isPluginModule)
				 LdrpLoadDrivers(pluginBuffer, TRUE);
		}
#if defined(_MSC_VER)
    }
	__finally
	{
#endif
		IoCloseFile(&fileHandle);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// match libraries
//
STATIC VOID LdrpMatchLibraries()
{
	BOOLEAN finished														= TRUE;

	do
	{
		finished															= TRUE;
		LDR_EXTENSION_MODULE* theModule										= LdrpExtensionModulesHead;

		while(theModule)
		{
			if(theModule->WillLoad == 1)
			{
				XML_TAG* theProperty										= CmGetTagValueForKey(theModule->InfoPlistTag, CHAR8_CONST_STRING("OSBundleLibraries"));
				if(theProperty)
				{
					theProperty												= theProperty->TagValue;
					while(theProperty)
					{
						LDR_EXTENSION_MODULE* theModule2					= LdrpExtensionModulesHead;
						while(theModule2)
						{
							XML_TAG* theProperty2							= CmGetTagValueForKey(theModule2->InfoPlistTag, CHAR8_CONST_STRING("CFBundleIdentifier"));
							if(theProperty2 && !strcmp(theProperty->StringValue, theProperty2->StringValue))
							{
								if(!theModule2->WillLoad)
									theModule2->WillLoad					= 1;

								break;
							}
							theModule2										= theModule2->NextModule;
						}
						theProperty											= theProperty->NextTag;
					}
				}
				theModule->WillLoad											= 2;
				finished													= FALSE;
			}
			theModule														= theModule->NextModule;
		}
	}while(!finished);
}

//
// load matched modules
//
STATIC EFI_STATUS LdrpLoadMatchedModules()
{
	LDR_EXTENSION_MODULE* theModule											= LdrpExtensionModulesHead;
	STATIC CHAR8 segName[0x40]												= {0};
	STATIC CHAR8 fileFullPath[1024]											= {0};

	while(theModule)
	{
		if(theModule->WillLoad)
		{
			IO_FILE_HANDLE fileHandle										= {0};
			UINT64 physicalAddress											= 0;
			UINT64 virtualAddress											= 0;

#if defined(_MSC_VER)
			__try
			{
#endif
				XML_TAG* theProperty										= CmGetTagValueForKey(theModule->InfoPlistTag, CHAR8_CONST_STRING("CFBundleExecutable"));
				UINTN moduleLength											= 0;

				if(theProperty)
				{
					CHAR8* fileName											= theProperty->StringValue;
					snprintf(fileFullPath, ARRAYSIZE(fileFullPath) - 1, CHAR8_CONST_STRING("%s%s"), theModule->DriverPath, fileName);
                    if(EFI_ERROR(IoOpenFile(fileFullPath, NULL, &fileHandle, IO_OPEN_MODE_NORMAL))) {
#if defined(_MSC_VER)
                        try_leave(NOTHING);
#else
                        if(physicalAddress)
                            MmFreeKernelMemory(virtualAddress, physicalAddress);

                        return EFI_SUCCESS;
#endif
                    }

                    if(EFI_ERROR(MachLoadThinFatFile(&fileHandle, NULL, &moduleLength))) {
#if defined(_MSC_VER)
                        try_leave(NOTHING);
#else
                        if(physicalAddress)
                            MmFreeKernelMemory(virtualAddress, physicalAddress);

                        IoCloseFile(&fileHandle);

                        return EFI_SUCCESS;
#endif
                    }
				}

				UINTN driverLength											= sizeof(LDR_DRIVER_INFO) + theModule->InfoPlistLength + moduleLength + theModule->DriverPathLength;
				UINTN allocatedLength										= driverLength;
				physicalAddress												= MmAllocateKernelMemory(&allocatedLength, &virtualAddress);
				UINT32 driverAddress										= (UINT32)(physicalAddress);
                if(!driverAddress) {
#if defined(_MSC_VER)
                    try_leave(NOTHING);
#else
                    if(physicalAddress)
                        MmFreeKernelMemory(virtualAddress, physicalAddress);

                    IoCloseFile(&fileHandle);

                    return EFI_SUCCESS;
#endif
                }

				LDR_DRIVER_INFO* driverInfo									= ArchConvertAddressToPointer(driverAddress, LDR_DRIVER_INFO*);
				driverInfo->InfoPlistAddress								= driverAddress + sizeof(LDR_DRIVER_INFO);
				driverInfo->InfoPlistLength									= (UINT32)(theModule->InfoPlistLength);
				driverInfo->DriverPath										= (UINT32)(driverAddress + sizeof(LDR_DRIVER_INFO) + driverInfo->InfoPlistLength);
				driverInfo->DriverPathLength								= (UINT32)(theModule->DriverPathLength);
				driverInfo->ModuleLength									= (UINT32)(moduleLength);
				if(moduleLength)
					driverInfo->ModuleAddress								= driverInfo->InfoPlistAddress + driverInfo->InfoPlistLength + driverInfo->DriverPathLength;
				else
					driverInfo->ModuleAddress								= 0;

                if(moduleLength && EFI_ERROR(IoReadFile(&fileHandle, ArchConvertAddressToPointer(driverInfo->ModuleAddress, VOID*), moduleLength, &moduleLength, FALSE))) {
#if defined(_MSC_VER)
                    try_leave(NOTHING);
#else
                    if(physicalAddress)
                        MmFreeKernelMemory(virtualAddress, physicalAddress);

                    IoCloseFile(&fileHandle);

                    return EFI_SUCCESS;
#endif
                }

				memcpy(ArchConvertAddressToPointer(driverInfo->InfoPlistAddress, CHAR8*), theModule->InfoPlistBuffer, theModule->InfoPlistLength);
				memcpy(ArchConvertAddressToPointer(driverInfo->DriverPath, CHAR8*), theModule->DriverPath, theModule->DriverPathLength);
				BlConvertPathSeparator(ArchConvertAddressToPointer(driverInfo->DriverPath, CHAR8*), '\\', '/');
				snprintf(segName, ARRAYSIZE(segName) - 1, CHAR8_CONST_STRING("Driver-%llx"), physicalAddress);
				if(!EFI_ERROR(BlAddMemoryRangeNode(segName, driverAddress, driverLength)))
					physicalAddress											= 0;
#if defined(_MSC_VER)
			}
			__finally
			{
#endif
				if(physicalAddress)
					MmFreeKernelMemory(virtualAddress, physicalAddress);

				IoCloseFile(&fileHandle);
#if defined(_MSC_VER)
			}
#endif
		}
		theModule															= theModule->NextModule;
	}

	return EFI_SUCCESS;
}

//
// load drivers
//
EFI_STATUS LdrLoadDrivers()
{
	IO_FILE_HANDLE fileHandle												= {0};
	EFI_STATUS status														= EFI_SUCCESS; 

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// check library directory
		//
		BOOLEAN loadLibrary													= !EFI_ERROR(IoOpenFile(CHAR8_CONST_STRING(""), NULL, &fileHandle, IO_OPEN_MODE_NORMAL));
		
		//
		// load from system directory
		//
		LdrpLoadDrivers(CHAR8_CONST_STRING("System\\Library\\Extensions"), FALSE);

		//
		// load from library directory
		//
		if(loadLibrary)
			LdrpLoadDrivers(CHAR8_CONST_STRING("Library\\Extensions"), FALSE);

		//
		// match libraries
		//
		LdrpMatchLibraries();

		//
		// load matched modules
		//
		status																= LdrpLoadMatchedModules();
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		IoCloseFile(&fileHandle);
#if defined(_MSC_VER)
	}
#endif

	return status;
}
