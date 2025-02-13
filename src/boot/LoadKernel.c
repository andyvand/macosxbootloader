//********************************************************************
//	created:	12:11:2009   19:19
//	filename: 	LoadKernel.cpp
//	author:		tiamo
//	purpose:	load kernel
//********************************************************************

#include "StdAfx.h"

#ifndef KERNEL_CACHE_MAGIC
#define KERNEL_CACHE_MAGIC													0x636f6d70
#endif /* KERNEL_CACHE_MAGIC */

#ifndef KERNEL_CACHE_LZSS
#define KERNEL_CACHE_LZSS													0x6c7a7373
#endif /* KERNEL_CACHE_LZSS */

#ifndef KERNEL_CACHE_LZVN
#if (TARGET_OS >= YOSEMITE)
#define KERNEL_CACHE_LZVN												    0x6c7a766e
#endif /* TARGET_OS >= YOSEMITE */
#endif /* KERNEL_CACHE_LZVN */

//
// compressed header
//
#if defined(_MSC_VER)
#include <pshpack1.h>
#endif

typedef struct _COMPRESSED_KERNEL_CACHE_HEADER
{
	//
	// signature
	//
	UINT32																	Signature;

	//
	// compress type
	//
	UINT32																	CompressType;

	//
	// adler32
	//
	UINT32																	Adler32Value;

	//
	// uncompressed size
	//
	UINT32																	UncompressedSize;

	//
	// compressed size
	//
	UINT32																	CompressedSize;

	//
	// support ASLR
	//
	UINT32																	SupportASLR;

	//
	// reserved
	//
	UINT32																	Reserved[10];

	//
	// platform name
	//
	CHAR8																	PlatformName[64];

	//
	// efi device path
	//
	CHAR8																	RootPath[256];
}COMPRESSED_KERNEL_CACHE_HEADER;

#if defined(_MSC_VER)
#include <poppack.h>
#endif

//
// global
//
STATIC BOOLEAN LdrpKernelCacheOverride										= 0;
STATIC UINT64 LdrpASLRDisplacement											= 0;
STATIC CHAR8* LdrpKernelPathName											= NULL;
STATIC EFI_DEVICE_PATH_PROTOCOL* LdrpKernelFilePath							= NULL;
STATIC CHAR8* LdrpKernelCachePathName										= NULL;
STATIC EFI_DEVICE_PATH_PROTOCOL* LdrpKernelCacheFilePath					= NULL;
STATIC CHAR8* LdrpRamDiskPathName											= NULL;
STATIC EFI_DEVICE_PATH_PROTOCOL* LdrpRamDiskFilePath						= NULL;

//
// compute displacement
//
STATIC UINT64 LdrpComputeASLRDisplacement(UINT8 slideValue)
{
	if (!(slideValue & 0x80))
		return (UINT64)(slideValue) << 21;

	UINT32 eaxValue															= 0;
	UINT32 ebxValue															= 0;
	UINT32 ecxValue															= 0;
	UINT32 edxValue															= 0;
	ArchCpuId(1, &eaxValue, &ebxValue, &ecxValue, &edxValue);

	//
	// family, model, ext_family, ext_model
	//	6,		e,		0,			2			= Xeon MP						(45nm) -> 0x2e
	//	6,		f,		0,			2			= Xeon MP						(32nm) -> 0x2e
	//	6,		c,		0,			2			= Core i7, Xeon					(32nm) -> 0x2c
	//	6,		a,		0,			2			= 2nd Core, Xeon E3-1200		(Sand Bridge 32nm) -> 0x2a
	//	6,		d,		0,			2			= Xeon E5xx						(Sand Bridge 32nm) -> 0x2c
	//	6,		a,		0,			3			= 3nd Core, Xeon E3-1200 v2		(Sand Bridge 22nm) -> 0x3a
	//
	UINT32 family															= (eaxValue >>  8) & 0x0f;
	UINT32 model															= ((eaxValue >>  4) & 0x0e) | ((eaxValue >> 12) & 0xf0);
	return ((UINT64)(slideValue) + (family == 6 && model >= 0x2a ? 0x81 : 0x00)) << 21;
}

//
// random
//
STATIC UINT8 LdrpRandom()
{
	UINT32 eaxValue															= 0;
	UINT32 ebxValue															= 0;
	UINT32 ecxValue															= 0;
	UINT32 edxValue															= 0;
	ArchCpuId(0x80000001, &eaxValue, &ebxValue, &ecxValue, &edxValue);
	BOOLEAN supportHardwareRandom											= (ecxValue & 0x40000000) ? TRUE : FALSE;

	while(TRUE)
	{
		UINTN randomValue													= 0;
		if (supportHardwareRandom)
		{
			randomValue														= ArchHardwareRandom();
			if (randomValue)
				return (UINT8)(randomValue);
		}

		UINT64 cpuTick														= ArchGetCpuTick();
		randomValue															= (cpuTick & 0xff) ^ ((cpuTick >> 8) & 0xff);
		if (randomValue)
			return (UINT8)(randomValue);
	}
	return 0;
}

//
// check cache valid
//
STATIC EFI_STATUS LdrpKernelCacheValid(CHAR8 CONST* cachePathName, BOOLEAN* kernelCacheValid)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE cacheFile												= {0};
	IO_FILE_HANDLE kernelFile												= {0};
	IO_FILE_HANDLE extensionsFile											= {0};
	EFI_FILE_INFO* cacheInfo												= NULL;
	EFI_FILE_INFO* kernelInfo												= NULL;
	EFI_FILE_INFO* extensionsInfo											= NULL;
	EFI_FILE_INFO* checkerInfo												= NULL;
	*kernelCacheValid														= FALSE;

#if DEBUG_LDRP_CALL_CSPRINTF
	CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(%s).\n"), cachePathName);
#endif
#if defined(_MSC_VER)
	__try
	{
#endif
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(1).\n"));
#endif
		//
		// open cache file
		//
        if (EFI_ERROR(IoOpenFile(cachePathName, NULL, &cacheFile, IO_OPEN_MODE_NORMAL))) {
#if defined(_MSC_VER)
            try_leave(LdrpKernelCachePathName ? status = EFI_NOT_FOUND : EFI_SUCCESS);
#else
            LdrpKernelCachePathName ? status = EFI_NOT_FOUND : EFI_SUCCESS;

            if (cacheInfo)
                MmFreePool(cacheInfo);

            if (kernelInfo)
                MmFreePool(kernelInfo);

            if (extensionsInfo)
                MmFreePool(extensionsInfo);

            IoCloseFile(&cacheFile);
            IoCloseFile(&kernelFile);
            IoCloseFile(&extensionsFile);

            return status;
#endif
        }
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(2).\n"));
#endif
		//
		// get cache file info
		//
        if (EFI_ERROR(status = IoGetFileInfo(&cacheFile, &cacheInfo))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (cacheInfo)
                MmFreePool(cacheInfo);

            if (kernelInfo)
                MmFreePool(kernelInfo);

            if (extensionsInfo)
                MmFreePool(extensionsInfo);

            IoCloseFile(&cacheFile);
            IoCloseFile(&kernelFile);
            IoCloseFile(&extensionsFile);

            return status;
#endif
        }

#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(3).\n"));
#endif
		//
		// check cache file info
		//
        if (!cacheInfo || (cacheInfo->Attribute & EFI_FILE_DIRECTORY)) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (cacheInfo)
                MmFreePool(cacheInfo);

            if (kernelInfo)
                MmFreePool(kernelInfo);

            if (extensionsInfo)
                MmFreePool(extensionsInfo);

            IoCloseFile(&cacheFile);
            IoCloseFile(&kernelFile);
            IoCloseFile(&extensionsFile);

            return status;
#endif
        }

#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(4).\n"));
#endif
		//
		// kernel cache override
		//
        if (LdrpKernelCacheOverride) {
#if defined(_MSC_VER)
            try_leave(*kernelCacheValid = TRUE);
#else
            *kernelCacheValid = TRUE;

            if (cacheInfo)
                MmFreePool(cacheInfo);

            if (kernelInfo)
                MmFreePool(kernelInfo);

            if (extensionsInfo)
                MmFreePool(extensionsInfo);

            IoCloseFile(&cacheFile);
            IoCloseFile(&kernelFile);
            IoCloseFile(&extensionsFile);

            return status;
#endif
        }
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(5).\n"));
#endif
		//
		// open kernel file
		//
		if (!EFI_ERROR(IoOpenFile(LdrpKernelPathName, NULL, &kernelFile, IO_OPEN_MODE_NORMAL)))
        {
#if DEBUG_LDRP_CALL_CSPRINTF
            CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(6).\n"));
#endif
            //
            // get kernel file info
            //
            if (EFI_ERROR(status = IoGetFileInfo(&kernelFile, &kernelInfo))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (cacheInfo)
                    MmFreePool(cacheInfo);

                if (kernelInfo)
                    MmFreePool(kernelInfo);

                if (extensionsInfo)
                    MmFreePool(extensionsInfo);

                IoCloseFile(&cacheFile);
                IoCloseFile(&kernelFile);
                IoCloseFile(&extensionsFile);

                return status;
#endif
            }

#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(7).\n"));
#endif
			//
			// check kernel file info
			//
            if (!kernelInfo || (kernelInfo->Attribute & EFI_FILE_DIRECTORY)) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (cacheInfo)
                    MmFreePool(cacheInfo);

                if (kernelInfo)
                    MmFreePool(kernelInfo);

                if (extensionsInfo)
                    MmFreePool(extensionsInfo);

                IoCloseFile(&cacheFile);
                IoCloseFile(&kernelFile);
                IoCloseFile(&extensionsFile);

                return status;
#endif
            }

#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(8).\n"));
#endif
			checkerInfo														= kernelInfo;
		}

		//
		// open extensions
		//
		if (!EFI_ERROR(IoOpenFile(CHAR8_CONST_STRING("System\\Library\\Extensions"), NULL, &extensionsFile, IO_OPEN_MODE_NORMAL)))
		{
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(9).\n"));
#endif
			//
			// get extensions file info
			//
            if (EFI_ERROR(status = IoGetFileInfo(&extensionsFile, &extensionsInfo))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (cacheInfo)
                    MmFreePool(cacheInfo);

                if (kernelInfo)
                    MmFreePool(kernelInfo);

                if (extensionsInfo)
                    MmFreePool(extensionsInfo);

                IoCloseFile(&cacheFile);
                IoCloseFile(&kernelFile);
                IoCloseFile(&extensionsFile);

                return status;
#endif
            }

#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(A).\n"));
#endif

			//
			// check extensions info
			//
            if (!extensionsInfo || !(extensionsInfo->Attribute & EFI_FILE_DIRECTORY)) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (cacheInfo)
                    MmFreePool(cacheInfo);

                if (kernelInfo)
                    MmFreePool(kernelInfo);

                if (extensionsInfo)
                    MmFreePool(extensionsInfo);

                IoCloseFile(&cacheFile);
                IoCloseFile(&kernelFile);
                IoCloseFile(&extensionsFile);

                return status;
#endif
            }
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(B).\n"));
#endif

			//
			// get bigger
			//
			if (!checkerInfo || BlCompareTime(&checkerInfo->ModificationTime, &extensionsInfo->ModificationTime) < 0)
				checkerInfo													= extensionsInfo;
		}
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(C).\n"));
#endif

		//
		// check time
		//
        if (!checkerInfo) {
#if defined(_MSC_VER)
            try_leave(*kernelCacheValid = TRUE);
#else
            *kernelCacheValid = TRUE;

            if (cacheInfo)
                MmFreePool(cacheInfo);

            if (kernelInfo)
                MmFreePool(kernelInfo);

            if (extensionsInfo)
                MmFreePool(extensionsInfo);

            IoCloseFile(&cacheFile);
            IoCloseFile(&kernelFile);
            IoCloseFile(&extensionsFile);

            return status;
#endif
        }

#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(D).\n"));
#endif

		//
		// add one second
		//
		EFI_TIME modifyTime													= checkerInfo->ModificationTime;
		BlAddOneSecond(&modifyTime);

		//
		// compare time
		//
		if (memcmp(&modifyTime, &cacheInfo->ModificationTime, sizeof(modifyTime)))
			status															= EFI_NOT_FOUND;
		else
		{
			*kernelCacheValid												= TRUE;
#if DEBUG_LDRP_CALL_CSPRINTF
			CsPrintf(CHAR8_CONST_STRING("PIKE: in LdrpKernelCacheValid(E).\n"));
#endif
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (cacheInfo)
			MmFreePool(cacheInfo);

		if (kernelInfo)
			MmFreePool(kernelInfo);

		if (extensionsInfo)
			MmFreePool(extensionsInfo);

		IoCloseFile(&cacheFile);
		IoCloseFile(&kernelFile);
		IoCloseFile(&extensionsFile);
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// setup ASLR
//
VOID LdrSetupASLR(BOOLEAN enableASLR, UINT8 slideValue)
{
	if (enableASLR)
	{
		if (!slideValue)
			slideValue														= LdrpRandom();

		BlSetBootMode(BOOT_MODE_ASLR, 0);
		LdrpASLRDisplacement												= LdrpComputeASLRDisplacement(slideValue);
	}
	else
	{
		BlSetBootMode(0, BOOT_MODE_ASLR);
		LdrpASLRDisplacement												= 0;
	}
}

//
// get aslr displacement
//
UINT64 LdrGetASLRDisplacement()
{
	return LdrpASLRDisplacement;
}

//
// get kernel path name
//
CHAR8 CONST* LdrGetKernelPathName()
{
	return LdrpKernelPathName;
}

//
// get kernel cache path name
//
CHAR8 CONST* LdrGetKernelCachePathName()
{
	return LdrpKernelCachePathName;
}

//
// get kernel cache override
//
BOOLEAN LdrGetKernelCacheOverride()
{
	return LdrpKernelCacheOverride;
}

//
// setup kernel cache path
//
VOID LdrSetupKernelCachePath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName, BOOLEAN cacheOverride)
{
	LdrpKernelCacheFilePath													= filePath;
	LdrpKernelCachePathName													= fileName;
	LdrpKernelCacheOverride													= cacheOverride;
}

//
// setup kernel path
//
VOID LdrSetupKernelPath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName)
{
	LdrpKernelFilePath														= filePath;
	LdrpKernelPathName														= fileName;
}

//
// setup ramdisk path
//
VOID LdrSetupRamDiskPath(EFI_DEVICE_PATH_PROTOCOL* filePath, CHAR8* fileName)
{
	LdrpRamDiskFilePath														= filePath;
	LdrpRamDiskPathName														= fileName;
}

//
// load prelinked kernel or kernel cache (fall back)
//
EFI_STATUS LdrLoadKernelCache(MACH_O_LOADED_INFO* loadedInfo, EFI_DEVICE_PATH_PROTOCOL* bootDevicePath)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};
	VOID* compressedBuffer													= NULL;
	VOID* uncompressedBuffer												= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// check mode
		//
        if (BlTestBootMode(BOOT_MODE_ALT_KERNEL)) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            return status;
#endif
        }

		//
		// get length info
		//
		UINT8 tempBuffer[0x140]												= {0};
		CHAR8 CONST* modelName												= PeGetModelName();
		UINTN modelNameLength												= strlen(modelName);
		UINTN devicePathLength												= DevPathGetSize(bootDevicePath);
		CHAR8 CONST* fileName												= LdrpKernelPathName ? LdrpKernelPathName : LdrpKernelCachePathName;
		UINTN fileNameLength												= strlen(fileName);

		if (modelNameLength > 0x40)
			modelNameLength													= 0x40;

		if (devicePathLength > 0x100)
			devicePathLength												= 0x100;

		UINTN leftLength													= sizeof(tempBuffer) - modelNameLength - devicePathLength;

		//
		// build alder32 buffer
		//
		memcpy(tempBuffer, modelName, modelNameLength);
		memcpy(tempBuffer + 0x40, bootDevicePath, devicePathLength);
		memcpy(tempBuffer + 0x40 + devicePathLength, fileName, fileNameLength > leftLength ? leftLength : fileNameLength);

		//
		// alder32
		//
		UINT32 tempValue													= BlAdler32(tempBuffer, sizeof(tempBuffer));
		tempValue															= SWAP32(tempValue);

		//
		// build cache path
		//
		BOOLEAN netBoot														= IoBootingFromNet();
		CHAR8 kernelCachePathName[1024]										= {0};

		if (netBoot)
		{
			if (LdrpKernelCachePathName)
				strncpy(kernelCachePathName, LdrpKernelCachePathName, sizeof(kernelCachePathName));
		}
		else
		{
			for(UINTN i = 0; i < 5; i ++)
			{
				//
				// build path
				//
				switch(i)
				{
					case 0:
						if (LdrpKernelCachePathName)
							strcpy(kernelCachePathName, LdrpKernelCachePathName);
						break;
					case 1:
						strcpy(kernelCachePathName, (CONST CHAR8*)"System\\Library\\PrelinkedKernels\\prelinkedkernel");
						break;
					case 2:
						strcpy(kernelCachePathName, (CONST CHAR8*)"System\\Library\\Caches\\com.apple.kext.caches\\Startup\\prelinkedkernel");
						break;
					case 3:
						strcpy(kernelCachePathName, (CONST CHAR8*)"System\\Library\\Caches\\com.apple.kext.caches\\Startup\\kernelcache");
						break;
					case 4:
						kernelCachePathName[0]								= 0;
						break;
				}

				//
				// check name
				//
				if (kernelCachePathName[0])
				{
					//
					// check valid
					//
					BOOLEAN kernelCacheValid								= FALSE;

                    if (EFI_ERROR(status = LdrpKernelCacheValid(kernelCachePathName, &kernelCacheValid))) {
#if defined(_MSC_VER)
                        try_leave(NOTHING);
#else
                        if (compressedBuffer)
                            MmFreePool(compressedBuffer);

                        if (uncompressedBuffer)
                            MmFreePool(uncompressedBuffer);

                        return status;
#endif
                    }

					if (kernelCacheValid)
						break;
				}
				status														= EFI_NOT_FOUND;
			}
		}

		//
		// unable to build file path
		//
        if (EFI_ERROR(status)) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            return status;
#endif
        }

		//
		// open file
		//
        if (EFI_ERROR(status = IoOpenFile(kernelCachePathName, LdrpKernelCacheFilePath, &fileHandle, IO_OPEN_MODE_NORMAL))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            return status;
#endif
        }

		//
		// load as thin fat file
		//
        if (EFI_ERROR(status = MachLoadThinFatFile(&fileHandle, NULL, NULL))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// read file header
		//
		STATIC COMPRESSED_KERNEL_CACHE_HEADER fileHeader					= {0};
		UINTN readLength													= 0;

        if (EFI_ERROR(status = IoReadFile(&fileHandle, &fileHeader, sizeof(fileHeader), &readLength, FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// check length
		//
        if (readLength != sizeof(fileHeader)) {
#if defined(_MSC_VER)
            try_leave(status = EFI_NOT_FOUND);
#else
            status = EFI_NOT_FOUND;

            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		//
		// check signature
		//
		if (fileHeader.Signature == SWAP_BE32_TO_HOST(KERNEL_CACHE_MAGIC))
		{
			//
			// check compressed type
			//
            if (fileHeader.CompressType != SWAP_BE32_TO_HOST(KERNEL_CACHE_LZSS) && fileHeader.CompressType != SWAP_BE32_TO_HOST(KERNEL_CACHE_LZVN)) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// disable ASLR
			//
			if (!fileHeader.SupportASLR)
				LdrSetupASLR(FALSE, 0);

			//
			// check platform name
			//
            if (fileHeader.PlatformName[0] && memcmp(tempBuffer, fileHeader.PlatformName, sizeof(fileHeader.PlatformName))) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// check root path
			//
            if (fileHeader.RootPath[0] && memcmp(tempBuffer + sizeof(fileHeader.PlatformName), fileHeader.RootPath, sizeof(fileHeader.RootPath))) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// allocate buffer
			//
			UINT32 compressedSize											= SWAP_BE32_TO_HOST(fileHeader.CompressedSize);
			compressedBuffer												= MmAllocatePool(compressedSize);

            if (!compressedBuffer) {
#if defined(_MSC_VER)
                try_leave(status = EFI_OUT_OF_RESOURCES);
#else
                status = EFI_OUT_OF_RESOURCES;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }


			//
			// read in
			//
            if (EFI_ERROR(status = IoReadFile(&fileHandle, compressedBuffer, compressedSize, &readLength, FALSE))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// check length
			//
            if (readLength != compressedSize) {
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// allocate buffer
			//
			UINT32 uncompressedSize											= SWAP_BE32_TO_HOST(fileHeader.UncompressedSize);
			uncompressedBuffer												= MmAllocatePool(uncompressedSize);

            if (!uncompressedBuffer) {
#if defined(_MSC_VER)
                try_leave(status = EFI_OUT_OF_RESOURCES);
#else
                status = EFI_OUT_OF_RESOURCES;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
            }

			//
			// decompress it
			//
			if (fileHeader.CompressType == SWAP_BE32_TO_HOST(KERNEL_CACHE_LZSS))
			{
#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: Calling BlDecompressLZSS().\n"));
#endif
                if (EFI_ERROR(status = BlDecompressLZSS(compressedBuffer, compressedSize, uncompressedBuffer, uncompressedSize, &readLength))) {
#if defined(_MSC_VER)
                    try_leave(NOTHING);
#else
                    if (compressedBuffer)
                        MmFreePool(compressedBuffer);

                    if (uncompressedBuffer)
                        MmFreePool(uncompressedBuffer);

                    IoCloseFile(&fileHandle);

                    return status;
#endif
                }
			}
#if (TARGET_OS >= YOSEMITE)
			else if (fileHeader.CompressType == SWAP_BE32_TO_HOST(KERNEL_CACHE_LZVN))
			{
	#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: Calling BlDecompressLZVN().\n"));
	#endif
				if (EFI_ERROR(status = BlDecompressLZVN(compressedBuffer, compressedSize, uncompressedBuffer, uncompressedSize, &readLength)))
				{
	#if DEBUG_LDRP_CALL_CSPRINTF
					CsPrintf(CHAR8_CONST_STRING("PIKE: BlDecompressLZVN() returned: %d!\n"), status);
	#endif
#if defined(_MSC_VER)
					try_leave(NOTHING);
#else
                    if (compressedBuffer)
                        MmFreePool(compressedBuffer);

                    if (uncompressedBuffer)
                        MmFreePool(uncompressedBuffer);

                    IoCloseFile(&fileHandle);

                    return status;
#endif
				}
			}
#endif // #if (TARGET_OS >= YOSEMITE)

			//
			// length check
			//
			if (readLength != uncompressedSize)
			{
#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: readLength(%d) != uncompressedSize(%d).\n"), readLength, uncompressedSize);
#endif
#if defined(_MSC_VER)
				try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
			}
			//
			// check adler32
			//
			tempValue														= BlAdler32(uncompressedBuffer, uncompressedSize);

			if (tempValue != SWAP_BE32_TO_HOST(fileHeader.Adler32Value))
			{
#if DEBUG_LDRP_CALL_CSPRINTF
				CsPrintf(CHAR8_CONST_STRING("PIKE: adler32(%d) != %d!\n"), SWAP_BE32_TO_HOST(fileHeader.Adler32Value), tempValue);
#endif
#if defined(_MSC_VER)
                try_leave(status = EFI_NOT_FOUND);
#else
                status = EFI_NOT_FOUND;

                if (compressedBuffer)
                    MmFreePool(compressedBuffer);

                if (uncompressedBuffer)
                    MmFreePool(uncompressedBuffer);

                IoCloseFile(&fileHandle);

                return status;
#endif
			}
			//
			// hack file handle
			//
			IoCloseFile(&fileHandle);
			fileHandle.EfiFileHandle										= NULL;
			fileHandle.EfiFilePath											= NULL;
			fileHandle.EfiLoadFileProtocol									= NULL;
			fileHandle.FileBuffer											= (UINT8*)(uncompressedBuffer);
			fileHandle.FileOffset											= 0;
			fileHandle.FileSize												= uncompressedSize;
			uncompressedBuffer												= NULL;
		}
		else
		{
			//
			// seek to beginning
			//
			IoSetFilePosition(&fileHandle, 0);
		}
#if DEBUG_LDRP_CALL_CSPRINTF
		CsPrintf(CHAR8_CONST_STRING("PIKE: Calling MachLoadMachO().\n"));
#endif
		//
		// load mach-o
		//
        if (EFI_ERROR(status = MachLoadMachO(&fileHandle, loadedInfo))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (compressedBuffer)
                MmFreePool(compressedBuffer);

            if (uncompressedBuffer)
                MmFreePool(uncompressedBuffer);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		DEVICE_TREE_NODE* chosenNode										= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);

		if (chosenNode)
			DevTreeAddProperty(chosenNode, CHAR8_CONST_STRING("boot-kernelcache-adler32"), &tempValue, sizeof(tempValue), TRUE);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (compressedBuffer)
			MmFreePool(compressedBuffer);

		if (uncompressedBuffer)
			MmFreePool(uncompressedBuffer);

		IoCloseFile(&fileHandle);
#if defined(_MSC_VER)
    }
#endif

#if DEBUG_LDRP_CALL_CSPRINTF
	CsPrintf(CHAR8_CONST_STRING("PIKE: Returning from LdrLoadKernelCache(%d).\n"), status);
#endif

	return status;
}

//
// load kernel
//
EFI_STATUS LdrLoadKernel(MACH_O_LOADED_INFO* loadedInfo)
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};

	if (EFI_ERROR(status = IoOpenFile(LdrpKernelPathName, LdrpKernelFilePath, &fileHandle, IO_OPEN_MODE_KERNEL)))
		return status;

	status																	= MachLoadMachO(&fileHandle, loadedInfo);
	IoCloseFile(&fileHandle);

	return status;
}

//
// load ramdisk
//
EFI_STATUS LdrLoadRamDisk()
{
	EFI_STATUS status														= EFI_SUCCESS;
	IO_FILE_HANDLE fileHandle												= {0};
	UINT64 physicalAddress													= 0;
	UINT64 virtualAddress													= 0;

#if defined(_MSC_VER)
	__try
	{
#endif
        if (EFI_ERROR(status = IoOpenFile(LdrpRamDiskPathName, LdrpRamDiskFilePath, &fileHandle, IO_OPEN_MODE_RAMDISK))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (physicalAddress)
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            return status;
#endif
        }

		UINTN bufferLength													= 0;

        if (EFI_ERROR(status = MachLoadThinFatFile(&fileHandle, NULL, &bufferLength))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (physicalAddress)
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		UINTN allocatedLength												= bufferLength;
		physicalAddress														= MmAllocateKernelMemory(&allocatedLength, &virtualAddress);

        if (!physicalAddress) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		UINTN readLength													= 0;

        if (EFI_ERROR(status = IoReadFile(&fileHandle, ArchConvertAddressToPointer(physicalAddress, VOID*), bufferLength, &readLength, FALSE))) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (physicalAddress)
                MmFreeKernelMemory(virtualAddress, physicalAddress);

            IoCloseFile(&fileHandle);

            return status;
#endif
        }

		if (!EFI_ERROR(status = BlAddMemoryRangeNode(CHAR8_CONST_STRING("RAMDisk"), physicalAddress, readLength)))
			physicalAddress													= 0;
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (physicalAddress)
			MmFreeKernelMemory(virtualAddress, physicalAddress);

		IoCloseFile(&fileHandle);
#if defined(_MSC_VER)
	}
#endif

	return status;
}
