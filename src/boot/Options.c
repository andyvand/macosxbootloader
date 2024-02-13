//********************************************************************
//	created:	8:11:2009   18:33
//	filename: 	Options.cpp
//	author:		tiamo
//	purpose:	process option
//********************************************************************

#include "StdAfx.h"

//
// global
//
STATIC UINT32 BlpBootMode													= BOOT_MODE_NORMAL | BOOT_MODE_SKIP_BOARD_ID_CHECK;
STATIC BOOLEAN BlpPasswordUIEfiRun											= FALSE;

//
// extract options
//
STATIC CHAR8 CONST* BlpExtractOptions(CHAR8 CONST* commandLine)
{
	while(isspace(*commandLine))
		commandLine															+= 1;

	CHAR8 CONST* retValue													= commandLine;
	CHAR8 c																	= *commandLine;

	if((c < 'a' || c > 'z') && c != '/' && c != '\\' && (c < 'A' || c > 'Z'))
		return retValue;

	while(*commandLine && *commandLine != '=' && !isspace(*commandLine))
		commandLine															+= 1;

	if(*commandLine == '=')
		return retValue;

	while(isspace(*commandLine))
		commandLine															+= 1;

	return commandLine;
}

//
// check temporary boot
//
STATIC BOOLEAN BlpIsTemporaryBoot()
{
	UINT32 attribute														= 0;
	UINT32 value															= 0;
	UINTN dataSize															= sizeof(value);
	if(!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"PickerEntryReason"), &AppleFirmwareVariableGuid, &attribute, &dataSize, &value)) && !(attribute & EFI_VARIABLE_NON_VOLATILE))
		return TRUE;

	attribute																= 0;
	dataSize																= sizeof(value);
	if(!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"BootCurrent"), &AppleFirmwareVariableGuid, &attribute, &dataSize, &value)) && !(attribute & EFI_VARIABLE_NON_VOLATILE) && !value)
		return TRUE;

	return FALSE;
}

//
// run PasswordUI.efi
//
STATIC EFI_STATUS BlpRunPasswordUIEfi()
{
	EFI_STATUS status														= EFI_SUCCESS;
	UINTN handleCount														= 0;
	EFI_HANDLE* handleArray													= NULL;
#if defined(_MSC_VER)
	__try
	{
#endif
        if(!BlTestBootMode(BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE) || BlpIsTemporaryBoot() || BlpPasswordUIEfiRun) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(handleArray)
                MmFreePool(handleArray);

            return status;
#endif
        }

		CsConnectDevice(TRUE, FALSE);
		EfiBootServices->LocateHandleBuffer(ByProtocol, &EfiFirmwareVolumeProtocolGuid, NULL, &handleCount, &handleArray);
		for(UINTN i = 0; i < handleCount; i ++)
		{
			EFI_HANDLE theHandle											= handleArray[i];
			EFI_FIRMWARE_VOLUME_PROTOCOL* firwareVolumeProtocol				= NULL;
			if(EFI_ERROR(EfiBootServices->HandleProtocol(theHandle, &EfiFirmwareVolumeDispatchProtocolGuid, (VOID**)(&firwareVolumeProtocol))))
				continue;
			if(EFI_ERROR(EfiBootServices->HandleProtocol(theHandle, &EfiFirmwareVolumeProtocolGuid, (VOID**)(&firwareVolumeProtocol))))
				continue;

			UINTN fileSize													= 0;
			EFI_FV_FILETYPE fileType										= 0;
			EFI_FV_FILE_ATTRIBUTES fileAttribute							= 0;
			UINT32 authStatus												= 0;
			if(EFI_ERROR(firwareVolumeProtocol->ReadFile(firwareVolumeProtocol, &ApplePasswordUIEfiFileNameGuid, NULL, &fileSize, &fileType, &fileAttribute, &authStatus)))
				continue;

			EFI_DEVICE_PATH_PROTOCOL* devPath								= DevPathGetDevicePathProtocol(theHandle);
			if(devPath)
			{
				UINT8 buffer[sizeof(MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + END_DEVICE_PATH_LENGTH];
				MEDIA_FW_VOL_FILEPATH_DEVICE_PATH* fvFileDevPathNode		= (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH*)(buffer);
				fvFileDevPathNode->Header.Type								= MEDIA_DEVICE_PATH;
				fvFileDevPathNode->Header.SubType							= MEDIA_FV_FILEPATH_DP;
				memcpy(&fvFileDevPathNode->NameGuid, &ApplePasswordUIEfiFileNameGuid, sizeof(ApplePasswordUIEfiFileNameGuid));
				SetDevicePathNodeLength(&fvFileDevPathNode->Header, sizeof(MEDIA_FW_VOL_FILEPATH_DEVICE_PATH));
				EFI_DEVICE_PATH_PROTOCOL* endOfPath							= NextDevicePathNode(&fvFileDevPathNode->Header);
				SetDevicePathEndNode(endOfPath);
				EFI_DEVICE_PATH_PROTOCOL* fileDevPath						= DevPathAppendDevicePath(devPath, &fvFileDevPathNode->Header);

				EFI_HANDLE imageHandle										= NULL;
				if(!EFI_ERROR(EfiBootServices->LoadImage(FALSE, EfiImageHandle, fileDevPath, NULL, 0, &imageHandle)))
				{
                    if(!EFI_ERROR(EfiBootServices->StartImage(imageHandle, NULL, NULL))) {
#if defined(_MSC_VER)
                        try_leave(BlpPasswordUIEfiRun = TRUE);
#else
                        BlpPasswordUIEfiRun = TRUE;

                        if(handleArray)
                            MmFreePool(handleArray);

                        return status;
#endif
                    }
				}
			}
			break;
		}
		BlSetBootMode(0, BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(handleArray)
			MmFreePool(handleArray);
#if defined(_MSC_VER)
    }
#endif

	return status;
}

//
// read kernel flags
//
STATIC EFI_STATUS BlpReadKernelFlags(EFI_DEVICE_PATH_PROTOCOL* bootFilePath, CHAR8 CONST* fileName, CHAR8 CONST** kernelFlags)
{
	EFI_STATUS status														= EFI_SUCCESS;
	CHAR8* fileBuffer														= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// read config file
		//
        if(EFI_ERROR(status	= IoReadWholeFile(bootFilePath, fileName, &fileBuffer, NULL, TRUE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(fileBuffer)
                MmFreePool(fileBuffer);

            return status;
#endif
        }

		//
		// parse file
		//
        if(EFI_ERROR(status = CmParseXmlFile(fileBuffer, NULL))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(fileBuffer)
                MmFreePool(fileBuffer);

            return status;
#endif
        }

		*kernelFlags														= CmGetStringValueForKey(NULL, CHAR8_CONST_STRING("Kernel Flags"), NULL);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(fileBuffer)
			MmFreePool(fileBuffer);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// load config file
//
STATIC CHAR8 CONST* BlpLoadConfigFile(CHAR8 CONST* bootOptions, EFI_DEVICE_PATH_PROTOCOL* bootFilePath)
{
	CHAR8 CONST* retValue													= NULL;
	EFI_DEVICE_PATH_PROTOCOL* bootPlistDevPath								= NULL;
	CHAR8* bootPlistPathName												= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// try com.apple.Boot.plist in current directory
		//
		bootPlistDevPath													= DevPathAppendLastComponent(bootFilePath, CHAR8_CONST_STRING("com.apple.Boot.plist"), TRUE);
		if(bootPlistDevPath)
		{
			bootPlistPathName												= DevPathExtractFilePathName(bootPlistDevPath, TRUE);
            if(bootPlistPathName && !EFI_ERROR(BlpReadKernelFlags(bootFilePath, bootPlistPathName, &retValue))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if(bootPlistPathName)
                    MmFreePool(bootPlistPathName);
                if(bootPlistDevPath)
                    MmFreePool(bootPlistDevPath);

                return retValue;
#endif
            }
		}

		//
		// read config from command line
		//
		STATIC CHAR8 fileName[1024]											= {0};
		UINTN valueLength													= 0;
		CHAR8 CONST* configFileName											= CmGetStringValueForKeyAndCommandLine(bootOptions, CHAR8_CONST_STRING("config"), &valueLength, FALSE);
		if(!configFileName)
		{
			//
			// then default file
			//
			configFileName													= CHAR8_CONST_STRING("com.apple.Boot");
			valueLength														= 14;
		}

		//
		// build full path name
		//
		if(!IoBootingFromNet() && configFileName[0] != '/' && configFileName[0] != '\\')
			strcpy(fileName, CHAR8_CONST_STRING("Library\\Preferences\\SystemConfiguration\\"));

		UINTN length														= strlen(fileName);
		memcpy(fileName + length, configFileName, valueLength);
		fileName[length + valueLength]										= 0;
		strcat(fileName, CHAR8_CONST_STRING(".plist"));
		BlpReadKernelFlags(bootFilePath, fileName, &retValue);
#if defined(_MSC_VER)
    }
	__finally
	{
#endif
		if(bootPlistPathName)
			MmFreePool(bootPlistPathName);
		if(bootPlistDevPath)
			MmFreePool(bootPlistDevPath);
#if defined(_MSC_VER)
	}
#endif

	return retValue;
}

//
// read device path variable
//
STATIC EFI_DEVICE_PATH_PROTOCOL* BlpReadDevicePathVariable(CHAR16 CONST* variableName, BOOLEAN macAddressNode)
{
	EFI_DEVICE_PATH_PROTOCOL* devicePath									= NULL;
	EFI_DEVICE_PATH_PROTOCOL* retValue										= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		UINTN variableLength												= 0;
        if(EfiRuntimeServices->GetVariable((CHAR16*)(variableName), &AppleNVRAMVariableGuid, NULL, &variableLength, NULL) != EFI_BUFFER_TOO_SMALL) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return retValue;
#endif
        }

		devicePath															= (EFI_DEVICE_PATH_PROTOCOL*)(MmAllocatePool(variableLength));
        if(!devicePath) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return retValue;
#endif
        }

        if(EFI_ERROR(EfiRuntimeServices->GetVariable((CHAR16*)(variableName), &AppleNVRAMVariableGuid, NULL, &variableLength, devicePath))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(devicePath)
                MmFreePool(devicePath);

            return retValue;
#endif
        }

        if(macAddressNode != DevPathHasMacAddressNode(devicePath)) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(devicePath)
                MmFreePool(devicePath);

            return retValue;
#endif
        }

		retValue															= devicePath;
		devicePath															= NULL;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(devicePath)
			MmFreePool(devicePath);
#if defined(_MSC_VER)
	}
#endif

	return retValue;
}

//
// setup path from variable
//
STATIC VOID BlpSetupPathFromVariable(EFI_DEVICE_PATH_PROTOCOL** filePath, CHAR8** pathName, CHAR16 CONST* variableName)
{
	EFI_DEVICE_PATH_PROTOCOL* devicePath									= BlpReadDevicePathVariable(variableName, BlTestBootMode(BOOT_MODE_NET) ? TRUE : FALSE);
	*filePath																= devicePath;
	if(devicePath && pathName)
		*pathName															= DevPathExtractFilePathName(devicePath, FALSE);
}

//
// setup path from command line
//
STATIC EFI_STATUS BlpSetupPathFromCommandLine(EFI_DEVICE_PATH_PROTOCOL** filePath, CHAR8** pathName, CHAR8 CONST* kernelCommandLine, CHAR8 CONST* keyName, EFI_DEVICE_PATH_PROTOCOL* bootFilePath, BOOLEAN* keyFound)
{
	UINTN valueLength														= 0;
	CHAR8 CONST* theValue													= CmGetStringValueForKeyAndCommandLine(kernelCommandLine, keyName, &valueLength, TRUE);
	if(theValue && valueLength)
	{
		//
		// free prev one
		//
		if(*pathName)
			MmFreePool(*pathName);

		//
		// allocate and copy path name
		//
		*pathName															= (CHAR8*)(MmAllocatePool(valueLength + 1));
		if(!*pathName)
			return EFI_OUT_OF_RESOURCES;

		memcpy(*pathName, theValue, valueLength);
		(*pathName)[valueLength]											= 0;
		BlConvertPathSeparator(*pathName, '/', '\\');

		//
		// build device path
		//
		if(IoBootingFromNet() && filePath)
		{
			if(*filePath)
				MmFreePool(*filePath);

			*filePath														= DevPathAppendLastComponent(bootFilePath, *pathName, TRUE);
			if(!*filePath)
				return EFI_OUT_OF_RESOURCES;
		}

		if(keyFound)
			*keyFound														= TRUE;
	}
	else if(keyFound)
	{
		*keyFound															= FALSE;
	}
	return EFI_SUCCESS;
}

//
// get key and value
//
STATIC CHAR8 CONST* BlpGetKeyAndValue(CHAR8 CONST* inputBuffer, CHAR8 CONST** keyBuffer, UINTN* totalLength)
{
	*totalLength															= 0;
	while(isspace(*inputBuffer))
		inputBuffer															+= 1;

	*keyBuffer																= inputBuffer;
	while(*inputBuffer && !isspace(*inputBuffer))
	{
		inputBuffer															+= 1;
		*totalLength														+= 1;
	}
	return inputBuffer;
}

//
// copy args
//
STATIC VOID BlpCopyArgs(CHAR8* dstBuffer, CHAR8 CONST* srcBuffer)
{
	while(srcBuffer && *srcBuffer)
	{
		CHAR8 CONST* keyBuffer												= NULL;
		UINTN totalLength													= 0;
		srcBuffer															= BlpGetKeyAndValue(srcBuffer, &keyBuffer, &totalLength);
		if(!totalLength)
			continue;

		if(CmGetStringValueForKeyAndCommandLine(dstBuffer, keyBuffer, NULL, FALSE))
			continue;

		strcat(dstBuffer, CHAR8_CONST_STRING(" "));
		UINTN dstLength														= strlen(dstBuffer);
		memcpy(dstBuffer + dstLength, keyBuffer, totalLength);
		dstBuffer[dstLength + totalLength]									= 0;
	}
}

//
// setup kernel command line
//
CHAR8* BlSetupKernelCommandLine(CHAR8 CONST* bootOptions, CHAR8 CONST* bootArgsVariable, CHAR8 CONST* kernelFlags)
{
	UINTN totalLength														= 0x80;
	totalLength																+= bootOptions ? strlen(bootOptions) : 0;
	totalLength																+= bootArgsVariable ? strlen(bootArgsVariable) : 0;
	totalLength																+= kernelFlags ? strlen(kernelFlags) : 0;

	//
	// allocate kernel command line
	//
	CHAR8* retValue															= (CHAR8*)(MmAllocatePool(totalLength));
	if(retValue)
	{
		retValue[0]															= 0;
		BlpCopyArgs(retValue, bootOptions);
		BlpCopyArgs(retValue, bootArgsVariable);
		BlpCopyArgs(retValue, kernelFlags);

		if(BlTestBootMode(BOOT_MODE_SAFE))
			BlpCopyArgs(retValue, CHAR8_CONST_STRING("-x"));
		if(BlTestBootMode(BOOT_MODE_SINGLE_USER))
			BlpCopyArgs(retValue, CHAR8_CONST_STRING("-s"));
		if(BlTestBootMode(BOOT_MODE_VERBOSE))
			BlpCopyArgs(retValue, CHAR8_CONST_STRING("-v"));
		if(BlTestBootMode(BOOT_MODE_FLUSH_CACHES))
			BlpCopyArgs(retValue, CHAR8_CONST_STRING("-f"));
		if(BlTestBootMode(BOOT_MODE_NET))
			BlpCopyArgs(retValue, CHAR8_CONST_STRING("srv=1"));
	}
	return retValue;
}

//
// detect hot key
//
EFI_STATUS BlDetectHotKey()
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// locate firmware password protocol
		//
		APPLE_FIRMWARE_PASSWORD_PROTOCOL* fwPwdProtocol						= NULL;
		if(!EFI_ERROR(EfiBootServices->LocateProtocol(&AppleFirmwarePasswordProtocolGuid, NULL, (VOID**)(&fwPwdProtocol))))
		{
			UINTN checkResult												= FALSE;
			fwPwdProtocol->Check(fwPwdProtocol, &checkResult);
			if(checkResult)
				BlSetBootMode(BOOT_MODE_FIRMWARE_PASSWORD, 0);
		}

		//
		// skip hiber
		//
        if(BlTestBootMode(BOOT_MODE_HIBER_FROM_FV)) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// locate key press protocol
		//
		APPLE_KEY_STATE_PROTOCOL* keyStateProtocol							= NULL;
        if(EFI_ERROR(status = EfiBootServices->LocateProtocol(&AppleKeyStateProtocolGuid, 0, (VOID**)(&keyStateProtocol)))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// read state
		//
		UINT16 modifyFlags													= 0;
		CHAR16 pressedKeys[32]												= {0};
		UINTN statesCount													= ARRAYSIZE(pressedKeys);
        if(EFI_ERROR(status = keyStateProtocol->ReadKeyState(keyStateProtocol, &modifyFlags, &statesCount, pressedKeys))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// check keys
		//
		BOOLEAN pressedV													= FALSE;	// al
		BOOLEAN pressedR													= FALSE;	// cl
		BOOLEAN pressedC													= FALSE;	// r8b
		BOOLEAN pressedMinus												= FALSE;	// r9b
		BOOLEAN pressedX													= FALSE;	// r11b
		BOOLEAN pressedS													= FALSE;	// r14b
		BOOLEAN pressedCommand												= (modifyFlags & (APPLE_KEY_STATE_MODIFY_LEFT_COMMAND | APPLE_KEY_STATE_MODIFY_RIGHT_COMMAND)) ? TRUE : FALSE; // !dl
		BOOLEAN pressedShift												= (modifyFlags & (APPLE_KEY_STATE_MODIFY_LEFT_SHIFT | APPLE_KEY_STATE_MODIFY_RIGHT_SHIFT)) ? TRUE : FALSE;
		BOOLEAN pressedOption												= (modifyFlags & (APPLE_KEY_STATE_MODIFY_LEFT_OPTION | APPLE_KEY_STATE_MODIFY_RIGHT_OPTION)) ? TRUE : FALSE;
		BOOLEAN pressedControl												= (modifyFlags & (APPLE_KEY_STATE_MODIFY_LEFT_CONTROL | APPLE_KEY_STATE_MODIFY_RIGHT_CONTROL)) ? TRUE : FALSE;
		
		for(UINTN i = 0; i < statesCount; i ++)
		{
			switch(pressedKeys[i])
			{
			case APPLE_KEY_STATE_C:
				pressedC													= TRUE;
				break;

			case APPLE_KEY_STATE_MINUS:
				pressedMinus												= TRUE;
				break;

			case APPLE_KEY_STATE_R:
				pressedR													= TRUE;
				break;

			case APPLE_KEY_STATE_S:
				pressedS													= TRUE;
				break;

			case APPLE_KEY_STATE_V:
				pressedV													= TRUE;
				break;

			case APPLE_KEY_STATE_X:
				pressedX													= TRUE;
				break;
			}
		}

		//
		// Command, R = recovery
		//
		if(pressedR && pressedCommand)
			BlSetBootMode(BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE, 0);

		//
		// run PasswordUI.efi
		//
        if(BlTestBootMode(BOOT_MODE_FIRMWARE_PASSWORD)) {
#if defined(_MSC_VER)
            try_leave(BlpRunPasswordUIEfi());
#else
            BlpRunPasswordUIEfi();
            return status;
#endif
        }

		//
		// SHIFT
		//
		if(pressedShift && !pressedCommand && !pressedControl && !pressedOption)
		{
			BlSetBootMode(BOOT_MODE_SAFE, 0);
			LdrSetupASLR(FALSE, 0);
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return status;
#endif
		}

		if(pressedCommand)
		{
			if(pressedV)
				BlSetBootMode(BOOT_MODE_VERBOSE, 0);

			if(pressedS)
			{
				if(pressedMinus)
					LdrSetupASLR(FALSE, 0);
				else
					BlSetBootMode(BOOT_MODE_SINGLE_USER | BOOT_MODE_VERBOSE, 0);
			}

			if(pressedX)
				BlSetBootMode(BOOT_MODE_X, 0);

			if(pressedC && pressedMinus)
				BlSetBootMode(BOOT_MODE_SKIP_BOARD_ID_CHECK, 0);
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
// process option
//
EFI_STATUS BlProcessOptions(CHAR8 CONST* bootCommandLine, CHAR8** kernelCommandLine, EFI_DEVICE_PATH_PROTOCOL* bootDevicePath, EFI_DEVICE_PATH_PROTOCOL* bootFilePath)
{
	EFI_STATUS status														= EFI_SUCCESS;
	CHAR8* bootArgsVariable													= NULL;
	CHAR8* bootOptions														= NULL;
	CHAR8 CONST* kernelFlags												= NULL;
	CHAR8* kernelCachePathName												= NULL;
	CHAR8* kernelPathName													= NULL;
	CHAR8* ramDiskPathName													= NULL;
	EFI_DEVICE_PATH_PROTOCOL* kernelCacheFilePath							= NULL;
	EFI_DEVICE_PATH_PROTOCOL* kernelFilePath								= NULL;
	EFI_DEVICE_PATH_PROTOCOL* ramDiskFilePath								= NULL;
	BOOLEAN kernelCacheOverride												= FALSE;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// extract kernel name and build boot options
		//
		bootCommandLine														= BlpExtractOptions(bootCommandLine);
		bootOptions															= (CHAR8*)(MmAllocatePool(strlen(bootCommandLine) + 1));
        if(!bootOptions) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }
		strcpy(bootOptions, bootCommandLine);

		//
		// check safe mode
		//
		UINTN valueLength													= 0;
		if(CmGetStringValueForKeyAndCommandLine(bootOptions, CHAR8_CONST_STRING("-x"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_SAFE, 0);

		//
		// check net boot device path
		//
		if(DevPathHasMacAddressNode(bootDevicePath))
			BlSetBootMode(BOOT_MODE_NET, 0);

		//
		// check recovery mode
		//
		if(!BlTestBootMode(BOOT_MODE_NET | BOOT_MODE_HIBER_FROM_FV))
		{
			UINT8 dataBuffer[10]											= {0};
			UINTN dataSize													= sizeof(dataBuffer);
			UINT32 attribute												= 0;
			status															= EfiRuntimeServices->GetVariable(CHAR16_STRING(L"recovery-boot-mode"), &AppleNVRAMVariableGuid, &attribute, &dataSize, dataBuffer);
			if(!EFI_ERROR(status) || status == EFI_BUFFER_TOO_SMALL)
				BlSetBootMode(BOOT_MODE_EFI_NVRAM_RECOVERY_BOOT_MODE, 0);

			status															= EFI_SUCCESS;
		}

		//
		// read boot-args
		//
		if(!BlTestBootMode(BOOT_MODE_SAFE))
		{
			UINTN dataSize													= 0;
			UINT32 attribute												= 0;
			if(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-args"), &AppleNVRAMVariableGuid, &attribute, &dataSize, NULL) == EFI_BUFFER_TOO_SMALL)
			{
				bootArgsVariable											= (CHAR8*)(MmAllocatePool(dataSize + sizeof(CHAR8)));
				if(bootArgsVariable)
				{
					if(!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"boot-args"), &AppleNVRAMVariableGuid, &attribute, &dataSize, bootArgsVariable)))
						bootArgsVariable[dataSize]							= 0;
					else
						MmFreePool(bootArgsVariable), bootArgsVariable = NULL;
				}
			}
		}

		//
		// load kernel cache device path
		//
		if(!BlTestBootMode(BOOT_MODE_SAFE))
			BlpSetupPathFromVariable(&kernelCacheFilePath, &kernelCachePathName, CHAR16_CONST_STRING(L"efi-boot-kernelcache-data"));

		//
		// load kernel device path
		//
		if(!BlTestBootMode(BOOT_MODE_SAFE) && !kernelCacheFilePath)
			BlpSetupPathFromVariable(&kernelFilePath, &kernelPathName, CHAR16_CONST_STRING(L"efi-boot-file-data"));

		//
		// setup default kernel cache name
		//
		if(BlTestBootMode(BOOT_MODE_NET) && !kernelCacheFilePath && !kernelFilePath)
		{
			kernelCachePathName												= BlAllocateString(CHAR8_CONST_STRING("x86_64\\kernelcache"));
			kernelCacheFilePath												= DevPathAppendLastComponent(bootFilePath, kernelCachePathName, TRUE);
		}

		//
		// setup override flags
		//
		if(BlTestBootMode(BOOT_MODE_NET) && kernelCachePathName)
			kernelCacheOverride												= TRUE;

		//
		// load config file
		//
		if(!BlTestBootMode(BOOT_MODE_SAFE) || BlTestBootMode(BOOT_MODE_BOOT_IS_NOT_ROOT))
			kernelFlags														= BlpLoadConfigFile(bootOptions, bootFilePath);

		//
		// setup kernel command line
		//
		*kernelCommandLine													= BlSetupKernelCommandLine(bootOptions, bootArgsVariable, kernelFlags);

		//
		// check kernel
		//
		BOOLEAN keyFound													= FALSE;
		if(!EFI_ERROR(BlpSetupPathFromCommandLine(&kernelFilePath, &kernelPathName, *kernelCommandLine, CHAR8_CONST_STRING("Kernel"), bootFilePath, &keyFound)) && keyFound)
		{
			if(strcmp(kernelPathName[0] == '/' || kernelPathName[0] == '\\' ? kernelPathName + 1 : kernelPathName, CHAR8_CONST_STRING("kernel")))
				BlSetBootMode(BOOT_MODE_ALT_KERNEL, 0);
		}

		//
		// check kernel cache
		//
		if(!EFI_ERROR(BlpSetupPathFromCommandLine(&kernelCacheFilePath, &kernelCachePathName, *kernelCommandLine, CHAR8_CONST_STRING("Kernel Cache"), bootFilePath, &keyFound)) && keyFound)
			kernelCacheOverride												= TRUE;

		//
		// check ramdisk
		//
		BlpSetupPathFromCommandLine(&ramDiskFilePath, &ramDiskPathName, *kernelCommandLine, CHAR8_CONST_STRING("RAM Disk"), bootFilePath, NULL);

		//
		// verbose mode
		//
		if(CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("-v"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_VERBOSE, 0);

		//
		// safe mode
		//
		if(CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("-x"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_SAFE, 0);

		//
		// single user mode
		//
		if(CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("-s"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_SINGLE_USER | BOOT_MODE_VERBOSE, 0);

		//
		// compact check
		//
		if(CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("-no_compat_check"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_SKIP_BOARD_ID_CHECK, 0);

		//
		// show panic dialog
		//
		if(CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("-no_panic_dialog"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_SKIP_PANIC_DIALOG, 0);

		//
		// debug
		//
		if(CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("-debug"), &valueLength, FALSE))
			BlSetBootMode(BOOT_MODE_DEBUG, 0);

		//
		// disable ASLR for safe mode
		//
		if(BlTestBootMode(BOOT_MODE_SAFE) && BlTestBootMode(BOOT_MODE_ASLR))
			LdrSetupASLR(FALSE, 0);

		//
		// check slide
		//
		if(BlTestBootMode(BOOT_MODE_ASLR))
		{
			CHAR8 CONST* slideString										= CmGetStringValueForKeyAndCommandLine(*kernelCommandLine, CHAR8_CONST_STRING("slide"), &valueLength, TRUE);
			if(slideString)
			{
				INTN slideValue												= valueLength ? atoi(slideString) : 0;
				if(slideValue >= 0 && slideValue <= 0xff)
					LdrSetupASLR(!valueLength || slideValue, (UINT8)(slideValue));
			}
		}

		//
		// setup default value
		//
		if(!IoBootingFromNet())
		{
			if(!kernelPathName)
				kernelPathName												= BlAllocateString(CHAR8_CONST_STRING("System\\Library\\Kernels\\kernel"));
		}

		//
		// save those
		//
		LdrSetupKernelCachePath(kernelCacheFilePath, kernelCachePathName, kernelCacheOverride);
		LdrSetupKernelPath(kernelFilePath, kernelPathName);
		LdrSetupRamDiskPath(ramDiskFilePath, ramDiskPathName);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(bootOptions)
			MmFreePool(bootOptions);

		if(bootArgsVariable)
			MmFreePool(bootArgsVariable);
#if defined(_MSC_VER)
    }
#endif

	return status;
}

//
// test boot mode
//
UINT32 BlTestBootMode(UINT32 bootMode)
{
	return BlpBootMode & bootMode;
}

//
// set boot mode
//
VOID BlSetBootMode(UINT32 setValue, UINT32 clearValue)
{
	BlpBootMode																|= setValue;
	BlpBootMode																&= ~clearValue;
}
