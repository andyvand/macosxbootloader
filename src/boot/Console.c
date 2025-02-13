//********************************************************************
//	created:	4:11:2009   10:40
//	filename: 	Console.cpp
//	author:		tiamo
//	purpose:	Console
//********************************************************************

#include "StdAfx.h"
#include "PictData.h"

//
// global
//
STATIC EFI_CONSOLE_CONTROL_SCREEN_MODE CspConsoleMode						= EfiConsoleControlScreenGraphics;
STATIC EFI_CONSOLE_CONTROL_SCREEN_MODE CspRestoreMode						= EfiConsoleControlScreenGraphics;
STATIC EFI_CONSOLE_CONTROL_PROTOCOL* CspConsoleControlProtocol				= NULL;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL* CspGraphicsOutputProtocol				= NULL;
STATIC EFI_UGA_DRAW_PROTOCOL* CspUgaDrawProtocol							= NULL;
STATIC APPLE_DEVICE_CONTROL_PROTOCOL* CspDeviceControlProtocol				= NULL;
STATIC APPLE_GRAPH_CONFIG_PROTOCOL* CspGraphConfigProtocol					= NULL;
STATIC UINT64 CspFrameBufferAddress											= 0;
STATIC UINT64 CspFrameBufferSize											= 0;
STATIC UINT32 CspBytesPerRow												= 0;
STATIC UINT32 CspHorzRes													= 0;
STATIC UINT32 CspVertRes													= 0;
STATIC UINT32 CspColorDepth													= 0;
STATIC BOOLEAN CspScreenNeedRedraw											= TRUE;
STATIC EFI_UGA_PIXEL CspBackgroundClear										= {0, 0, 0, 0};
STATIC UINTN CspIndicatorWidth												= 0;
STATIC UINTN CspIndicatorHeight												= 0;
STATIC UINTN CspIndicatorFrameCount											= 0;
STATIC UINTN CspIndicatorOffsetY											= 0;
STATIC UINTN CspIndicatorCurrentFrame										= 0;
STATIC EFI_UGA_PIXEL* CspIndicatorImage										= NULL;
STATIC EFI_EVENT CspIndicatorRefreshTimerEvent								= NULL;
STATIC BOOLEAN CspHiDPIMode													= FALSE;
STATIC INT32 CspGfxSavedConfigRestoreStatus									= -1;

//
// convert image
//
STATIC EFI_STATUS CspConvertImage(EFI_UGA_PIXEL** imageBuffer, UINT8 CONST* imageData, UINTN width, UINTN height, UINTN frameCount, UINT8 CONST* lookupTable)
{
	UINTN dataLength														= width * height * frameCount;
	UINTN imageLength														= dataLength * sizeof(EFI_UGA_PIXEL);
	EFI_UGA_PIXEL* theImage													= (EFI_UGA_PIXEL*)(MmAllocatePool(imageLength));

	if (!theImage)
		return EFI_OUT_OF_RESOURCES;

	*imageBuffer															= theImage;

	for (UINTN i = 0; i < dataLength; i++, theImage++, imageData++)
	{
		UINTN index															= *imageData * 3;
		theImage->Red														= lookupTable[index + 0];
		theImage->Green														= lookupTable[index + 1];
		theImage->Blue														= lookupTable[index + 2];
	}

	return EFI_SUCCESS;
}

//
// scale image
//
STATIC EFI_STATUS CspScaleImage(EFI_UGA_PIXEL* inputData, UINTN inputWidth, UINTN inputHeight, EFI_UGA_PIXEL** outputData, UINTN* outputWidth, UINTN* outputHeight, INTN scaleRate)
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// compute dims
		//
		UINTN scaledWidth													= inputWidth * scaleRate / 1000;
		UINTN scaledHeight													= inputHeight * scaleRate / 1000;

		if (!scaledWidth || !scaledHeight || scaledWidth > CspHorzRes || scaledHeight > CspVertRes)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_INVALID_PARAMETER);
#else
            status = EFI_INVALID_PARAMETER;
            return status;
#endif
		}

		//
		// allocate buffer
		//
		EFI_UGA_PIXEL* scaledData											= (EFI_UGA_PIXEL*)(MmAllocatePool(scaledWidth * scaledHeight * sizeof(EFI_UGA_PIXEL)));

		if (!scaledData)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
		}

		//
		// output
		//
		*outputWidth														= scaledWidth;
		*outputHeight														= scaledHeight;
		*outputData															= scaledData;

		//
		// scale it
		//
		for (UINTN y = 0; y < scaledHeight; y++)
		{
			for (UINTN x = 0; x < scaledWidth; x++, scaledData++)
			{
				*scaledData													= *(inputData + x * inputWidth / scaledWidth + y * inputHeight / scaledHeight * inputWidth);
			}
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
// fill screen
//
STATIC EFI_STATUS CspFillRect(UINTN x, UINTN y, UINTN width, UINTN height, EFI_UGA_PIXEL pixelColor)
{
	CspScreenNeedRedraw														= TRUE;

	if (CspGraphicsOutputProtocol)
	{
		return CspGraphicsOutputProtocol->Blt(CspGraphicsOutputProtocol, &pixelColor, EfiBltVideoFill, 0, 0, x, y, width, height, 0);
	}

	if (CspUgaDrawProtocol)
	{
		return CspUgaDrawProtocol->Blt(CspUgaDrawProtocol, &pixelColor, EfiUgaVideoFill, 0, 0, x, y, width, height, 0);
	}

	return EFI_UNSUPPORTED;
}

//
// draw rect
//
STATIC EFI_STATUS CspDrawRect(UINTN x, UINTN y, UINTN width, UINTN height, EFI_UGA_PIXEL* pixelBuffer)
{
	CspScreenNeedRedraw														= TRUE;

	if (CspGraphicsOutputProtocol)
	{
		return CspGraphicsOutputProtocol->Blt(CspGraphicsOutputProtocol, pixelBuffer, EfiBltBufferToVideo, 0, 0, x, y, width, height, 0);
	}

	if (CspUgaDrawProtocol)
	{
		return CspUgaDrawProtocol->Blt(CspUgaDrawProtocol, pixelBuffer, EfiUgaBltBufferToVideo, 0, 0, x, y, width, height, 0);
	}

	return EFI_UNSUPPORTED;
}

//
// load logo file
//
EFI_STATUS CspLoadLogoFile(CHAR8 CONST* logoFileName, EFI_UGA_PIXEL** logoImage, UINTN* imageWidth, UINTN* imageHeight)
{
	EFI_STATUS status														= EFI_SUCCESS;
	CHAR8* fileBuffer														= NULL;
	UINTN fileSize															= 0;
	EFI_HANDLE* handleArray													= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// read file
		//
		if (EFI_ERROR(status = IoReadWholeFile(NULL, logoFileName, &fileBuffer, &fileSize, FALSE)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (fileBuffer)
            {
                MmFreePool(fileBuffer);
            }

            if (handleArray)
            {
                EfiBootServices->FreePool(handleArray);
            }

            return status;
#endif
		}

		//
		// get image decoders
		//
		UINTN totalHandles													= 0;

		if (EFI_ERROR(status = EfiBootServices->LocateHandleBuffer(ByProtocol, &AppleImageCodecProtocolGuid, NULL, &totalHandles, &handleArray)))
		{
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileBuffer)
            {
                MmFreePool(fileBuffer);
            }

            if (handleArray)
            {
                EfiBootServices->FreePool(handleArray);
            }

            return status;
#endif
		}

		//
		// search decoder
		//
		APPLE_IMAGE_CODEC_PROTOCOL* imageCodecProtocol						= NULL;
		status																= EFI_NOT_FOUND;

		for (UINTN i = 0; i < totalHandles; i++)
		{
			//
			// get codec protocol
			//
			EFI_HANDLE theHandle											= handleArray[i];

			if (!theHandle)
			{
				continue;
			}

			if (EFI_ERROR(status = EfiBootServices->HandleProtocol(theHandle, &AppleImageCodecProtocolGuid, (VOID**)(&imageCodecProtocol))))
			{
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                if (fileBuffer)
                {
                    MmFreePool(fileBuffer);
                }

                if (handleArray)
                {
                    EfiBootServices->FreePool(handleArray);
                }

                return status;
#endif
			}

			//
			// can decode this file
			//
			if (!EFI_ERROR(status = imageCodecProtocol->RecognizeImageData(fileBuffer, fileSize)))
			{
				break;
			}
		}

		//
		// decoder not found
		//
		if (EFI_ERROR(status))
		{
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileBuffer)
            {
                MmFreePool(fileBuffer);
            }

            if (handleArray)
            {
                EfiBootServices->FreePool(handleArray);
            }

            return status;
#endif
		}

		//
		// get image dims
		//
		if (EFI_ERROR(status = imageCodecProtocol->GetImageDims(fileBuffer, fileSize, imageWidth, imageHeight)))
		{
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileBuffer)
            {
                MmFreePool(fileBuffer);
            }

            if (handleArray)
            {
                EfiBootServices->FreePool(handleArray);
            }

            return status;
#endif
		}

		//
		// decode buffer
		//
		UINTN imageDataSize													= 0;

		if (EFI_ERROR(status = imageCodecProtocol->DecodeImageData(fileBuffer, fileSize, logoImage, &imageDataSize)))
		{
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if (fileBuffer)
            {
                MmFreePool(fileBuffer);
            }

            if (handleArray)
            {
                EfiBootServices->FreePool(handleArray);
            }

            return status;
#endif
		}

		//
		// alpha blend with background
		//
		UINTN pixelCount													= imageDataSize / sizeof(EFI_UGA_PIXEL);
		EFI_UGA_PIXEL* imageData											= *logoImage;

		for (UINTN i = 0; i < pixelCount; i++, imageData++)
		{
			UINT8 alpha														= 255 - imageData->Reserved;

			if (alpha != 255)
			{
				UINT32 red													= (imageData->Red * alpha + imageData->Reserved * CspBackgroundClear.Red) / 255;
				imageData->Red												= (UINT8)(red > 255 ? 255 : red);

				UINT32 green												= (imageData->Green * alpha + imageData->Reserved * CspBackgroundClear.Green) / 255;
				imageData->Green											= (UINT8)(green > 255 ? 255 : green);

				UINT32 blue													= (imageData->Blue * alpha + imageData->Reserved * CspBackgroundClear.Blue) / 255;
				imageData->Blue												= (UINT8)(blue > 255 ? 255 : blue);
			}
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (fileBuffer)
		{
			MmFreePool(fileBuffer);
		}

		if (handleArray)
		{
			EfiBootServices->FreePool(handleArray);
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// convert logo file
//
STATIC EFI_STATUS CspConvertLogoImage(BOOLEAN normalLogo, EFI_UGA_PIXEL** logoImage, UINTN* imageWidth, UINTN* imageHeight)
{
	//
	// get logo file name
	//
	EFI_STATUS status														= EFI_SUCCESS;
	EFI_UGA_PIXEL* efiAllocatedData											= NULL;
	UINT8* imageData														= NULL;
	*logoImage																= NULL;
	*imageWidth																= 0;
	*imageHeight															= 0;

#if defined(_MSC_VER)
	__try
	{
#endif
		CHAR8 CONST* logoFileName											= CmGetStringValueForKey(NULL, normalLogo ? CHAR8_CONST_STRING("Boot Logo") : CHAR8_CONST_STRING("Boot Fail Logo"), NULL);

		//
		// using alt image
		//
		if (logoFileName)
		{
			//
			// load file
			//
			if (!EFI_ERROR(CspLoadLogoFile(logoFileName, &efiAllocatedData, imageWidth, imageHeight)))
			{
				//
				// get scale rate
				//
				INT64 scaleRate												= 125;
				CmGetIntegerValueForKey(CHAR8_CONST_STRING("Boot Logo Scale"), &scaleRate);

				//
				// no scale
				//
				if (scaleRate <= 0 || CspVertRes >= 1080)
				{
					UINTN imageSize											= *imageWidth * *imageHeight * sizeof(EFI_UGA_PIXEL);
					*logoImage												= (EFI_UGA_PIXEL*)(MmAllocatePool(imageSize));

					if (*logoImage)
					{
						memcpy(*logoImage, efiAllocatedData, imageSize);
					}

#if defined(_MSC_VER)
					try_leave(status = *logoImage ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES);
#else
                    status = *logoImage ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;

                    if (imageData)
                    {
                        MmFreePool(imageData);
                    }

                    if (efiAllocatedData)
                    {
                        EfiBootServices->FreePool(efiAllocatedData);
                    }

                    if (EFI_ERROR(status))
                    {
                        *imageWidth                                                        = 0;
                        *imageHeight                                                    = 0;
                        *logoImage                                                        = NULL;
                    }

                    return status;
#endif
				}

				if (!EFI_ERROR(CspScaleImage(efiAllocatedData, *imageWidth, *imageHeight, logoImage, imageWidth, imageHeight, (INTN)(scaleRate))))
				{
#if defined(_MSC_VER)
					try_leave(NOTHING);
#else
                    if (imageData)
                    {
                        MmFreePool(imageData);
                    }

                    if (efiAllocatedData)
                    {
                        EfiBootServices->FreePool(efiAllocatedData);
                    }

                    if (EFI_ERROR(status))
                    {
                        *imageWidth                                                        = 0;
                        *imageHeight                                                    = 0;
                        *logoImage                                                        = NULL;
                    }

                    return status;
#endif
				}

				MmFreePool(*logoImage);
			}
		}

		//
		// then try the default one
		//
		typedef struct _builtin_image_info
		{
			UINTN															Width;
			UINTN															Height;
			UINTN															BufferSize;
			VOID*															Buffer;
			UINT8*															LookupTable;
		} builtin_image_info;

		//
		// 4 images(normal, normal@2x, failed, failed@2x)
		//
		STATIC builtin_image_info imageInfo[4] =
		{
#if (TARGET_OS >= YOSMITE)
	#if LEGACY_GREY_SUPPORT
			/* normal */		{ 84, 103, sizeof(AppleLogoPacked),			AppleLogoPacked,			AppleLogoClut},
			/* normal@2x */		{168, 206, sizeof(AppleLogo2XPacked),		AppleLogo2XPacked,			AppleLogo2XClut},
	#else
			/* normal */		{ 84, 103, sizeof(AppleLogoBlackPacked),	AppleLogoBlackPacked,		AppleLogoBlackClut},
			/* normal@2x */		{168, 206, sizeof(AppleLogoBlack2XPacked),	AppleLogoBlack2XPacked,		AppleLogoBlack2XClut},
	#endif
			/* failed */		{100, 100, sizeof(CspFailedLogo),			CspFailedLogo,				CspFailedLogoLookupTable},
			/* failed@2x */		{200, 200, sizeof(CspFailedLogo2x),			CspFailedLogo2x,			CspFailedLogoLookupTable2x},
#else // #if TARGET_OS_LEGACY
			/* normal */		{ 84, 103, sizeof(CspNormalLogo),			CspNormalLogo,				CspNormalLogoLookupTable},
			/* normal@2x */		{168, 206, sizeof(CspNormalLogo2x),			CspNormalLogo2x,			CspNormalLogoLookupTable2x},

			/* failed */		{100, 100, sizeof(CspFailedLogo),			CspFailedLogo,				CspFailedLogoLookupTable},
			/* failed@2x */		{200, 200, sizeof(CspFailedLogo2x),			CspFailedLogo2x,			CspFailedLogoLookupTable2x},
#endif // #if (TARGET_OS >= YOSMITE)
		};

		//
		// decompress it
		//
		UINTN index															= (normalLogo ? 0 : 1) * 2  + (CspHiDPIMode ? 1 : 0);
		*imageWidth															= imageInfo[index].Width;
		*imageHeight														= imageInfo[index].Height;
		UINTN imageSize														= *imageWidth * *imageHeight;
		imageData															= (UINT8*)(MmAllocatePool(imageSize));

		if (!imageData)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if (imageData)
            {
                MmFreePool(imageData);
            }

            if (efiAllocatedData)
            {
                EfiBootServices->FreePool(efiAllocatedData);
            }

            if (EFI_ERROR(status))
            {
                *imageWidth                                                        = 0;
                *imageHeight                                                    = 0;
                *logoImage                                                        = NULL;
            }

            return status;
#endif
		}

#if (TARGET_OS >= YOSMITE)
		if (index < 2)
		{
			if (EFI_ERROR(status = BlDecompressLZVN(imageInfo[index].Buffer, imageInfo[index].BufferSize, imageData, imageSize, &imageSize)))
			{
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                if (imageData)
                {
                    MmFreePool(imageData);
                }

                if (efiAllocatedData)
                {
                    EfiBootServices->FreePool(efiAllocatedData);
                }

                if (EFI_ERROR(status))
                {
                    *imageWidth                                                        = 0;
                    *imageHeight                                                    = 0;
                    *logoImage                                                        = NULL;
                }

                return status;
#endif
			}
		}
		else
		{
#endif // #if (TARGET_OS >= YOSMITE)
			if (EFI_ERROR(status = BlDecompressLZSS(imageInfo[index].Buffer, imageInfo[index].BufferSize, imageData, imageSize, &imageSize)))
			{
#if defined(_MSC_VER)
				try_leave(NOTHING);
#else
                if (imageData)
                {
                    MmFreePool(imageData);
                }

                if (efiAllocatedData)
                {
                    EfiBootServices->FreePool(efiAllocatedData);
                }

                if (EFI_ERROR(status))
                {
                    *imageWidth                                                        = 0;
                    *imageHeight                                                    = 0;
                    *logoImage                                                        = NULL;
                }

                return status;
#endif
			}
#if (TARGET_OS >= YOSMITE)
		}
#endif // #if (TARGET_OS >= YOSMITE)
		//
		// convert it
		//
		if (EFI_ERROR(status = CspConvertImage(logoImage, imageData, *imageWidth, *imageHeight, 1, imageInfo[index].LookupTable)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (imageData)
            {
                MmFreePool(imageData);
            }

            if (efiAllocatedData)
            {
                EfiBootServices->FreePool(efiAllocatedData);
            }

            if (EFI_ERROR(status))
            {
                *imageWidth                                                        = 0;
                *imageHeight                                                    = 0;
                *logoImage                                                        = NULL;
            }

            return status;
#endif
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (imageData)
		{
			MmFreePool(imageData);
		}

		if (efiAllocatedData)
		{
			EfiBootServices->FreePool(efiAllocatedData);
		}

		if (EFI_ERROR(status))
		{
			*imageWidth														= 0;
			*imageHeight													= 0;
			*logoImage														= NULL;
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// restore graph config
//
STATIC EFI_STATUS CspRestoreGraphConfig(UINT32 count, VOID* p1, VOID* p2, VOID* p3)
{
	if (ArchConvertPointerToAddress(CspGraphConfigProtocol) >= 2)
	{
		return CspGraphConfigProtocol->RestoreConfig(CspGraphConfigProtocol, 2, count, p1, p2, p3);
	}

	return EFI_SUCCESS;
}

//
// timer event
//
STATIC VOID EFIAPI CspIndicatorRefreshTimerEventNotifyRoutine(EFI_EVENT theEvent, VOID* theContext)
{
	if (!theContext)
	{
		EFI_UGA_PIXEL* curBuffer											= CspIndicatorImage + CspIndicatorHeight * CspIndicatorWidth * CspIndicatorCurrentFrame;
		CspIndicatorCurrentFrame											= (CspIndicatorCurrentFrame + 1) % CspIndicatorFrameCount;
		CspDrawRect((CspHorzRes - CspIndicatorWidth) / 2, (CspVertRes - CspIndicatorHeight) / 2 + CspIndicatorOffsetY, CspIndicatorWidth, CspIndicatorHeight, curBuffer);
	}
	else
	{
		CspFillRect((CspHorzRes - CspIndicatorWidth) / 2, (CspVertRes - CspIndicatorHeight) / 2 + CspIndicatorOffsetY, CspIndicatorWidth, CspIndicatorHeight, CspBackgroundClear);
	}
}

//
// decode and draw preview buffer
//
STATIC VOID CspDecodeAndDrawPreviewBufferColored(HIBERNATE_PREVIEW* previewBuffer, UINT32 imageIndex, UINT32 horzRes, UINT32 vertRes, UINT32 pixelPerRow, UINT64 frameBuffer, VOID* config)
{
	UINT32* row																= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		VOID* output														= ArchConvertAddressToPointer(frameBuffer, VOID*);
		UINT8* dataBuffer													= (UINT8*)(previewBuffer + 1) + previewBuffer->ImageCount * vertRes;
		row																	= (UINT32*)(MmAllocatePool(horzRes * sizeof(UINT32)));

		if (!row)
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            return;
#endif
		}

		if (config)
		{
			INT32 delta														= 0;
			UINT16* configBuffer											= (UINT16*)(config);

			for (INT32 i = 0; i < 0x400; i++, configBuffer++)
			{
				INT32 index													= (i - delta) / 4;
				delta														= index / 85 + 1;
				configBuffer[i]												= (dataBuffer[index + 0x000] << 8) | dataBuffer[index + 0x000];
				configBuffer[i + 0x400]										= (dataBuffer[index + 0x100] << 8) | dataBuffer[index + 0x100];
				configBuffer[i + 0x800]										= (dataBuffer[index + 0x200] << 8) | dataBuffer[index + 0x200];
			}
		}

		for (UINT32 j = 0; j < vertRes; j++)
		{
			UINT32* input													= (UINT32*)(previewBuffer + 1) + imageIndex * vertRes + j;
			input															= Add2Ptr(previewBuffer, *input, UINT32*);

			UINT32 count													= 0;
			UINT32 repeat													= 0;
			BOOLEAN fetch													= FALSE;
			UINT32 data														= 0;

			for (UINT32 i = 0; i < horzRes; i++)
			{
				if (!count)
				{
					count													= *input++;
					repeat													= (count & 0xff000000);
					count													^= repeat;
					fetch													= TRUE;
				}
				else
				{
					fetch													= !repeat;
				}

				count														-= 1;

				if (fetch)
				{
					data													= *input++;

					if (dataBuffer)
					{
						data												= (dataBuffer[(data  >> 16) & 0xff] << 16) | (dataBuffer[0x100 | ((data >> 8) & 0xff)] << 8) | dataBuffer[0x200 | (data & 0xff)];
					}
				}

				row[i]														= data;
			}

			EfiBootServices->CopyMem(output, row, horzRes * sizeof(UINT32));
			output															= Add2Ptr(output, pixelPerRow * sizeof(UINT32), VOID*);
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (row)
		{
			MmFreePool(row);
		}
#if defined(_MSC_VER)
	}
#endif
}

//
// decode and draw preview buffer
//
STATIC EFI_STATUS CspDecodeAndDrawPreviewBuffer(HIBERNATE_PREVIEW* previewBuffer, UINT32 imageIndex, UINT64 frameBuffer, UINT32 horzRes, UINT32 vertRes, UINT32 colorDepth, UINT32 bytesPerRow, BOOLEAN colorMode, VOID* config)
{
	EFI_STATUS status														= EFI_SUCCESS;
	UINT16* sc0																= NULL;
	UINT16* sc1																= NULL;
	UINT16* sc2																= NULL;
	UINT16* sc3																= NULL;
	UINT32* row																= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// check match
		//
		if (imageIndex >= previewBuffer->ImageCount || (colorDepth >> 3) != previewBuffer->Depth || horzRes != previewBuffer->Width || vertRes != previewBuffer->Height || colorDepth != 32)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_INVALID_PARAMETER);
#else
            status = EFI_INVALID_PARAMETER;

            if (sc0)
            {
                MmFreePool(sc0);
            }

            if (sc1)
            {
                MmFreePool(sc1);
            }

            if (sc2)
            {
                MmFreePool(sc2);
            }

            if (sc3)
            {
                MmFreePool(sc3);
            }

            if (row)
            {
                MmFreePool(row);
            }

            return status;
#endif
		}

		//
		// color mode
		//
		if (colorMode)
		{
#if defined(_MSC_VER)
			try_leave(CspDecodeAndDrawPreviewBufferColored(previewBuffer, imageIndex, horzRes, vertRes, bytesPerRow / sizeof(UINT32), frameBuffer, config));
#else
            CspDecodeAndDrawPreviewBufferColored(previewBuffer, imageIndex, horzRes, vertRes, bytesPerRow / sizeof(UINT32), frameBuffer, config);

            if (sc0)
            {
                MmFreePool(sc0);
            }

            if (sc1)
            {
                MmFreePool(sc1);
            }

            if (sc2)
            {
                MmFreePool(sc2);
            }

            if (sc3)
            {
                MmFreePool(sc3);
            }

            if (row)
            {
                MmFreePool(row);
            }

            return status;
#endif
		}

		//
		// get info
		//
		VOID* output														= ArchConvertAddressToPointer(frameBuffer, VOID*);
		UINT32 pixelPerRow													= bytesPerRow / sizeof(UINT32);
		row																	= (UINT32*)(MmAllocatePool(horzRes * sizeof(UINT32)));
		sc0																	= (UINT16*)(MmAllocatePool((horzRes + 2) * sizeof(UINT16)));
		sc1																	= (UINT16*)(MmAllocatePool((horzRes + 2) * sizeof(UINT16)));
		sc2																	= (UINT16*)(MmAllocatePool((horzRes + 2) * sizeof(UINT16)));
		sc3																	= (UINT16*)(MmAllocatePool((horzRes + 2) * sizeof(UINT16)));

		if (!sc0 || !sc1 || !sc2 || !sc3 || !row)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if (sc0)
            {
                MmFreePool(sc0);
            }

            if (sc1)
            {
                MmFreePool(sc1);
            }

            if (sc2)
            {
                MmFreePool(sc2);
            }

            if (sc3)
            {
                MmFreePool(sc3);
            }

            if (row)
            {
                MmFreePool(row);
            }

            return status;
#endif
		}

		memset(sc0, 0, (horzRes + 2) * sizeof(UINT16));
		memset(sc1, 0, (horzRes + 2) * sizeof(UINT16));
		memset(sc2, 0, (horzRes + 2) * sizeof(UINT16));
		memset(sc3, 0, (horzRes + 2) * sizeof(UINT16));

		for (UINT32 j = 0; j < vertRes + 2; j++)
		{
			UINT32* input													= (UINT32*)(previewBuffer + 1) + imageIndex * vertRes;

			if (j < vertRes)
			{
				input														+= j;
			}
			else
			{
				input														+= vertRes - 1;
			}

			input															= Add2Ptr(previewBuffer, *input, UINT32*);

			UINT32 count													= 0;
			UINT32 repeat													= 0;
			UINT32 sr0														= 0;
			UINT32 sr1														= 0;
			UINT32 sr2														= 0;
			UINT32 sr3														= 0;
			BOOLEAN fetch													= FALSE;
			UINT32 data														= 0;

			for (UINT32 i = 0; i < horzRes + 2; i++)
			{
				if (i < horzRes)
				{
					if (!count)
					{
						count												= *input++;
						repeat												= (count & 0xff000000);
						count												^= repeat;
						fetch												= TRUE;
					}
					else
					{
						fetch												= !repeat;
					}

					count													-= 1;

					if (fetch)
					{
						data												= *input++;
						data												= (((13933 * (0xff & (data >> 24)) + 46871 * (0xff & (data >> 16)) + 4732 * (0xff & data)) >> 16) * 19661 + (103 << 16)) >> 16;
					}
				}

				UINT32 tmp2													= sr0 + data;
				sr0															= data;
				UINT32 tmp1													= sr1 + tmp2;
				sr1															= tmp2;
				tmp2														= sr2 + tmp1;
				sr2															= tmp1;
				tmp1														= sr3 + tmp2;
				sr3															= tmp2;

				tmp2														= sc0[i] + tmp1;
				sc0[i]														= (UINT16)(tmp1);
				tmp1														= sc1[i] + tmp2;
				sc1[i]														= (UINT16)(tmp2);
				tmp2														= sc2[i] + tmp1;
				sc2[i]														= (UINT16)(tmp1);
				UINT32 out													= ((128 + sc3[i] + tmp2) >> 8) & 0xff;
				sc3[i]														= (UINT16)(tmp2);

				if (i > 1 && j > 1)
				{
					row[i - 2]												= out | (out << 8) | (out << 16);
				}
			}

			if (j > 1)
			{
				EfiBootServices->CopyMem(output, row, horzRes * sizeof(UINT32));
				output														= Add2Ptr(output, pixelPerRow * sizeof(UINT32), VOID*);
			}
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (sc0)
		{
			MmFreePool(sc0);
		}

		if (sc1)
		{
			MmFreePool(sc1);
		}

		if (sc2)
		{
			MmFreePool(sc2);
		}

		if (sc3)
		{
			MmFreePool(sc3);
		}

		if (row)
		{
			MmFreePool(row);
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// initialize
//
EFI_STATUS CsInitialize()
{
	//
	// get background clear
	//
	UINTN dataSize															= sizeof(CspBackgroundClear);

	if (!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"BackgroundClear"), &AppleFirmwareVariableGuid, NULL, &dataSize, &CspBackgroundClear)))
	{
		CspScreenNeedRedraw													= FALSE;
	}
	else
	{
		CspScreenNeedRedraw													= TRUE;
	}

	//
	// get console control protocol
	//
	EFI_STATUS status														= EFI_SUCCESS;

	if (EFI_ERROR(status = EfiBootServices->LocateProtocol(&EfiConsoleControlProtocolGuid, NULL, (VOID**)(&CspConsoleControlProtocol))))
	{
		return status;
	}

	//
	// get console mode
	//
	if (EFI_ERROR(status = CspConsoleControlProtocol->GetMode(CspConsoleControlProtocol, &CspConsoleMode, NULL, NULL)))
	{
		return status;
	}

	//
	// read UIScale
	//
	UINT8 uiScale															= 0;
	dataSize																= sizeof(uiScale);

	if (!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"UIScale"), &AppleFirmwareVariableGuid, NULL, &dataSize, &uiScale)))
	{
		UINT16 actualDensity												= 0;
		dataSize															= sizeof(actualDensity);
		EfiRuntimeServices->GetVariable(CHAR16_STRING(L"ActualDensity"), &AppleFirmwareVariableGuid, NULL, &dataSize, &actualDensity);

		UINT16 densityThreshold												= 0;
		dataSize															= sizeof(densityThreshold);
		EfiRuntimeServices->GetVariable(CHAR16_STRING(L"DensityThreshold"), &AppleFirmwareVariableGuid, NULL, &dataSize, &densityThreshold);

		CspHiDPIMode														= uiScale >= 2;
	}

	//
	// save old mode
	//
	CspRestoreMode															= CspConsoleMode;

	return EFI_SUCCESS;
}

//
// set text mode
//
EFI_STATUS CsSetConsoleMode(BOOLEAN textMode, BOOLEAN force)
{
	EFI_CONSOLE_CONTROL_SCREEN_MODE screenMode								= textMode ? EfiConsoleControlScreenText : EfiConsoleControlScreenGraphics;

	if (!force && screenMode == CspConsoleMode)
	{
		return EFI_SUCCESS;
	}

	EFI_STATUS status														= CspConsoleControlProtocol->SetMode(CspConsoleControlProtocol, screenMode);

	if (EFI_ERROR(status))
	{
		return status;
	}

	CspConsoleMode															= screenMode;
	CspScreenNeedRedraw														= TRUE;

	return EFI_SUCCESS;
}

//
// setup graph mode
//
EFI_STATUS CsInitializeGraphMode()
{
	EFI_STATUS status;

	if (EFI_ERROR(status = CsSetConsoleMode(FALSE, FALSE)))
	{
		return status;
	}

	if (EFI_ERROR(status = EfiBootServices->HandleProtocol(EfiSystemTable->ConsoleOutHandle, &EfiGraphicsOutputProtocolGuid, (VOID**)(&CspGraphicsOutputProtocol))))
	{
		if (EFI_ERROR(status = EfiBootServices->HandleProtocol(EfiSystemTable->ConsoleOutHandle, &EfiUgaDrawProtocolGuid, (VOID**)(&CspUgaDrawProtocol))))
		{
			return status;
		}
	}

	BlSetBootMode(BOOT_MODE_GRAPH, 0);
#if LEGACY_GREY_SUPPORT
	INT64 clearColor														= 0xffbfbfbf;
#else
	INT64 clearColor														= 0xff030000;
#endif
	CmGetIntegerValueForKey(CHAR8_CONST_STRING("Background Color"), &clearColor);

	EFI_UGA_PIXEL clearPixel;
	clearPixel.Red															= (UINT8)((clearColor >>  0) & 0xff);
	clearPixel.Green														= (UINT8)((clearColor >>  8) & 0xff);
	clearPixel.Blue															= (UINT8)((clearColor >> 16) & 0xff);

	if (clearPixel.Red != CspBackgroundClear.Red || clearPixel.Green != CspBackgroundClear.Green || clearPixel.Blue != CspBackgroundClear.Blue)
	{
		CspScreenNeedRedraw													= TRUE;
	}

	CspBackgroundClear														= clearPixel;
	CsInitializeBootVideo(NULL);

	return EFI_SUCCESS;
}

//
// setup boot video
//
EFI_STATUS CsInitializeBootVideo(BOOT_VIDEO* bootVideo)
{
	//
	// get info
	//
	if (!CspFrameBufferAddress)
	{
		//
		// check graphics output protocol
		//
		EFI_GRAPHICS_OUTPUT_PROTOCOL* graphicsOutputProtocol				= NULL;

		if (!EFI_ERROR(EfiBootServices->HandleProtocol(EfiSystemTable->ConsoleOutHandle, &EfiGraphicsOutputProtocolGuid, (VOID**)(&graphicsOutputProtocol))))
		{
			CspFrameBufferAddress											= graphicsOutputProtocol->Mode->FrameBufferBase;

			if (CspFrameBufferAddress)
			{
				CspColorDepth												= 0;
				CspFrameBufferSize											= graphicsOutputProtocol->Mode->FrameBufferSize;
				CspHorzRes													= graphicsOutputProtocol->Mode->Info->HorizontalResolution;
				CspVertRes													= graphicsOutputProtocol->Mode->Info->VerticalResolution;

				if (graphicsOutputProtocol->Mode->Info->PixelFormat == PixelBitMask)
				{
					UINT32 colorMask										= graphicsOutputProtocol->Mode->Info->PixelInformation.BlueMask;
					colorMask												|= graphicsOutputProtocol->Mode->Info->PixelInformation.GreenMask;
					colorMask												|= graphicsOutputProtocol->Mode->Info->PixelInformation.RedMask;

					for (UINTN i = 0; i < 32; i++, colorMask >>= 1)
					{
						if (colorMask & 1)
							CspColorDepth									+= 1;
					}

					CspColorDepth											= (CspColorDepth + 7) & ~7;
				}
				else if (graphicsOutputProtocol->Mode->Info->PixelFormat != PixelBltOnly)
				{
					CspColorDepth											= 32;
				}

				if (!CspColorDepth)
				{
					CspFrameBufferSize										= 0;
					CspFrameBufferAddress									= 0;
					CspHorzRes												= 0;
					CspVertRes												= 0;
				}
				else
				{
					CspBytesPerRow											= graphicsOutputProtocol->Mode->Info->PixelsPerScanLine * (CspColorDepth >> 3);
				}
			}
		}

		//
		// check device protocol?
		//
		if (!CspFrameBufferAddress)
		{
			APPLE_GRAPH_INFO_PROTOCOL* graphInfoProtocol					= NULL;

			if (!EFI_ERROR(EfiBootServices->LocateProtocol(&AppleGraphInfoProtocolGuid, 0, (VOID**)(&graphInfoProtocol))))
			{
				if (EFI_ERROR(graphInfoProtocol->GetInfo(graphInfoProtocol, &CspFrameBufferAddress, &CspFrameBufferSize, &CspBytesPerRow, &CspHorzRes, &CspVertRes, &CspColorDepth)))
				{
					CspFrameBufferAddress									= 0;
				}
			}
		}
	}

	if (bootVideo)
	{
		bootVideo->BaseAddress												= (UINT32)(CspFrameBufferAddress);
		bootVideo->BytesPerRow												= CspBytesPerRow;
		bootVideo->ColorDepth												= CspColorDepth;
		bootVideo->HorzRes													= CspHorzRes;
		bootVideo->VertRes													= CspVertRes;
	}

	return CspFrameBufferAddress ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

EFI_STATUS CsInitializeBootVideoV1(BOOT_VIDEO_V1* bootVideo)
{
    //
    // get info
    //
    if (!CspFrameBufferAddress)
    {
        //
        // check graphics output protocol
        //
        EFI_GRAPHICS_OUTPUT_PROTOCOL* graphicsOutputProtocol                = NULL;

        if (!EFI_ERROR(EfiBootServices->HandleProtocol(EfiSystemTable->ConsoleOutHandle, &EfiGraphicsOutputProtocolGuid, (VOID**)(&graphicsOutputProtocol))))
        {
            CspFrameBufferAddress                                            = graphicsOutputProtocol->Mode->FrameBufferBase;

            if (CspFrameBufferAddress)
            {
                CspColorDepth                                                = 0;
                CspFrameBufferSize                                            = graphicsOutputProtocol->Mode->FrameBufferSize;
                CspHorzRes                                                    = graphicsOutputProtocol->Mode->Info->HorizontalResolution;
                CspVertRes                                                    = graphicsOutputProtocol->Mode->Info->VerticalResolution;

                if (graphicsOutputProtocol->Mode->Info->PixelFormat == PixelBitMask)
                {
                    UINT32 colorMask                                        = graphicsOutputProtocol->Mode->Info->PixelInformation.BlueMask;
                    colorMask                                                |= graphicsOutputProtocol->Mode->Info->PixelInformation.GreenMask;
                    colorMask                                                |= graphicsOutputProtocol->Mode->Info->PixelInformation.RedMask;

                    for (UINTN i = 0; i < 32; i++, colorMask >>= 1)
                    {
                        if (colorMask & 1)
                            CspColorDepth                                    += 1;
                    }

                    CspColorDepth                                            = (CspColorDepth + 7) & ~7;
                }
                else if (graphicsOutputProtocol->Mode->Info->PixelFormat != PixelBltOnly)
                {
                    CspColorDepth                                            = 32;
                }

                if (!CspColorDepth)
                {
                    CspFrameBufferSize                                        = 0;
                    CspFrameBufferAddress                                    = 0;
                    CspHorzRes                                                = 0;
                    CspVertRes                                                = 0;
                }
                else
                {
                    CspBytesPerRow                                            = graphicsOutputProtocol->Mode->Info->PixelsPerScanLine * (CspColorDepth >> 3);
                }
            }
        }

        //
        // check device protocol?
        //
        if (!CspFrameBufferAddress)
        {
            APPLE_GRAPH_INFO_PROTOCOL* graphInfoProtocol                    = NULL;

            if (!EFI_ERROR(EfiBootServices->LocateProtocol(&AppleGraphInfoProtocolGuid, 0, (VOID**)(&graphInfoProtocol))))
            {
                if (EFI_ERROR(graphInfoProtocol->GetInfo(graphInfoProtocol, &CspFrameBufferAddress, &CspFrameBufferSize, &CspBytesPerRow, &CspHorzRes, &CspVertRes, &CspColorDepth)))
                {
                    CspFrameBufferAddress                                    = 0;
                }
            }
        }
    }

    if (bootVideo)
    {
        bootVideo->BaseAddress                                                = (UINT32)(CspFrameBufferAddress);
        bootVideo->BytesPerRow                                                = CspBytesPerRow;
        bootVideo->ColorDepth                                                = CspColorDepth;
        bootVideo->HorzRes                                                    = CspHorzRes;
        bootVideo->VertRes                                                    = CspVertRes;
    }

    return CspFrameBufferAddress ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}
//
// print string
//
VOID CsPrintf(CHAR8 CONST* printForamt, ...)
{
	if (EfiSystemTable && EfiSystemTable->ConOut && EfiSystemTable->ConsoleOutHandle)
	{
		STATIC CHAR8 utf8Buffer[1024]										= {0};
		STATIC CHAR16 unicodeBuffer[1024]									= {0};
		VA_LIST list;
		VA_START(list, printForamt);
		vsnprintf(utf8Buffer, ARRAYSIZE(utf8Buffer) - 1, printForamt, list);
		VA_END(list);

		CHAR16 tempBuffer[2]												= {0};
		BlUtf8ToUnicode(utf8Buffer, strlen(utf8Buffer), unicodeBuffer, ARRAYSIZE(unicodeBuffer) - 1);

		for (UINTN i = 0; i < ARRAYSIZE(unicodeBuffer) && unicodeBuffer[i]; i++)
		{
			if (unicodeBuffer[i] == L'\n')
			{
				EfiSystemTable->ConOut->OutputString(EfiSystemTable->ConOut, CHAR16_STRING(L"\r\n"));
			}
			else if (unicodeBuffer[i] == L'\t')
			{
				EfiSystemTable->ConOut->OutputString(EfiSystemTable->ConOut, CHAR16_STRING(L"    "));
			}
			else
			{
				tempBuffer[0]												= unicodeBuffer[i];
				EfiSystemTable->ConOut->OutputString(EfiSystemTable->ConOut, tempBuffer);
			}
		}
	}
}

//
// connect device
//
EFI_STATUS CsConnectDevice(BOOLEAN connectAll, BOOLEAN connectDisplay)
{
	EFI_STATUS status														= EFI_SUCCESS;

	if (!CspDeviceControlProtocol)
	{
		if (EFI_ERROR(EfiBootServices->LocateProtocol(&AppleDeviceControlProtocolGuid, NULL, (VOID**)(&CspDeviceControlProtocol))))
		{
			CspDeviceControlProtocol										= ArchConvertAddressToPointer(1, APPLE_DEVICE_CONTROL_PROTOCOL*);
		}
	}

	if (ArchConvertPointerToAddress(CspDeviceControlProtocol) >= 2)
	{
		if (connectAll)
		{
			status															= CspDeviceControlProtocol->ConnectAll();
		}
		if (connectDisplay)
		{
			status															= CspDeviceControlProtocol->ConnectDisplay();
		}
	}

	if (!CspGraphConfigProtocol)
	{
		if (EFI_ERROR(EfiBootServices->LocateProtocol(&AppleGraphConfigProtocolGuid, NULL, (VOID**)(&CspGraphConfigProtocol))))
		{
			CspGraphConfigProtocol											= ArchConvertAddressToPointer(1, APPLE_GRAPH_CONFIG_PROTOCOL*);
		}
	}

	if (connectAll && connectDisplay)
	{
		CspRestoreGraphConfig(0, NULL, NULL, NULL);
	}

	UINT32 result[sizeof(INTN) / sizeof(UINT32)]							= {0};
	UINTN dataSize															= sizeof(result);

	if (!EFI_ERROR(EfiRuntimeServices->GetVariable(CHAR16_STRING(L"gfx-saved-config-restore-status"), &AppleFirmwareVariableGuid, NULL, &dataSize, result)))
	{
		CspGfxSavedConfigRestoreStatus										= (INT32)(result[0] | (result[ARRAYSIZE(result) - 1] & 0x80000000));
	}

	return status;
}

//
// draw boot image
//
EFI_STATUS CsDrawBootImage(BOOLEAN normalLogo)
{
	EFI_STATUS status														= EFI_SUCCESS;
	UINT8* dataBuffer														= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// check graph mode
		//
		if (!CspFrameBufferAddress || CspConsoleMode != EfiConsoleControlScreenGraphics)
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		CsClearScreen();
		//
		// convert logo image
		//
		EFI_UGA_PIXEL* logoImage											= NULL;
		UINTN imageWidth													= 0;
		UINTN imageHeight													= 0;

		if (EFI_ERROR(status = CspConvertLogoImage(normalLogo, &logoImage, &imageWidth, &imageHeight)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		//
		// draw it
		//
		status																= CspDrawRect((CspHorzRes - imageWidth) / 2, (CspVertRes - imageHeight) / 2, imageWidth, imageHeight, logoImage);
		MmFreePool(logoImage);

		//
		// show indicator only in netboot mode
		//
		if (EFI_ERROR(status) || !normalLogo || !BlTestBootMode(BOOT_MODE_NET))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		//
		// setup indicator info
		//
		CspIndicatorWidth													= CspHiDPIMode ? 64 : 32;
		CspIndicatorHeight													= CspIndicatorWidth;
		CspIndicatorOffsetY													= CspHiDPIMode ? 400 : 200;
		CspIndicatorFrameCount												= 18;
		UINTN imageSize														= CspIndicatorWidth * CspIndicatorHeight * CspIndicatorFrameCount;
		dataBuffer															= (UINT8*)(MmAllocatePool(imageSize));

		if (!dataBuffer)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;

            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		//
		// decompress data
		//
		if (EFI_ERROR(status = BlDecompressLZSS(CspHiDPIMode ? CspIndicator2x : CspIndicator, CspHiDPIMode ? sizeof(CspIndicator2x) : sizeof(CspIndicator), dataBuffer, imageSize, &imageSize)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		//
		// convert image
		//
		if (EFI_ERROR(status = CspConvertImage(&CspIndicatorImage, dataBuffer, CspIndicatorWidth, CspIndicatorHeight, CspIndicatorFrameCount, CspHiDPIMode ? CspIndicatorLookupTable2x : CspIndicatorLookupTable)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		//
		// draw the 1st frame
		//
		CspIndicatorRefreshTimerEventNotifyRoutine(CspIndicatorRefreshTimerEvent, NULL);

		//
		// create timer event
		//
		if (EFI_ERROR(status = EfiBootServices->CreateEvent(EFI_EVENT_TIMER | EFI_EVENT_NOTIFY_SIGNAL, EFI_TPL_NOTIFY, &CspIndicatorRefreshTimerEventNotifyRoutine, NULL, &CspIndicatorRefreshTimerEvent)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (dataBuffer)
            {
                MmFreePool(dataBuffer);
            }

            return status;
#endif
		}

		//
		// setup 100ms timer
		//
		status																= EfiBootServices->SetTimer(CspIndicatorRefreshTimerEvent, TimerPeriodic, 1000000);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (dataBuffer)
		{
			MmFreePool(dataBuffer);
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// draw panic image
//
EFI_STATUS CsDrawPanicImage()
{
	EFI_STATUS status														= EFI_SUCCESS;
	UINT8* imageData														= NULL;
	EFI_UGA_PIXEL* panicImage												= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// check graph mode
		//
		if (!CspFrameBufferAddress || CspConsoleMode != EfiConsoleControlScreenGraphics)
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (imageData)
            {
                MmFreePool(imageData);
            }

            if (panicImage)
            {
                MmFreePool(panicImage);
            }

            return status;
#endif
		}

		//
		// decompress data
		//
		UINTN imageWidth													= CspHiDPIMode ? 920 : 460;
		UINTN imageHeight													= CspHiDPIMode ? 570 : 285;
		UINTN imageSize														= imageWidth * imageHeight;

#if (TARGET_OS >= YOSEMITE)
		if (EFI_ERROR(status = BlDecompressLZVN(CspHiDPIMode ? ApplePanicDialog2X : ApplePanicDialog, CspHiDPIMode ? sizeof(ApplePanicDialog2X) : sizeof(ApplePanicDialog), imageData, imageSize, &imageSize)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (imageData)
            {
                MmFreePool(imageData);
            }

            if (panicImage)
            {
                MmFreePool(panicImage);
            }

            return status;
#endif
		}
#else
		if (EFI_ERROR(status = BlDecompressLZSS(CspHiDPIMode ? CspPanicDialog2x : CspPanicDialog, CspHiDPIMode ? sizeof(CspPanicDialog2x) : sizeof(CspPanicDialog), imageData, imageSize, &imageSize)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (imageData)
            {
                MmFreePool(imageData);
            }

            if (panicImage)
            {
                MmFreePool(panicImage);
            }

            return status;
#endif
		}
#endif // #if (TARGET_OD => YOSEMITE)

		//
		// convert it
		//
		if (EFI_ERROR(status = CspConvertImage(&panicImage, imageData, imageWidth, imageHeight, 1, CspHiDPIMode ? ApplePanicDialog2XClut : ApplePanicDialogClut)))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (imageData)
            {
                MmFreePool(imageData);
            }

            if (panicImage)
            {
                MmFreePool(panicImage);
            }

            return status;
#endif
		}

		//
		// draw it
		//
		status																= CspDrawRect((CspHorzRes - imageWidth) / 2, (CspVertRes - imageHeight) / 2, imageWidth, imageHeight, panicImage);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (imageData)
		{
			MmFreePool(imageData);
		}

		if (panicImage)
		{
			MmFreePool(panicImage);
		}
#if defined(_MSC_VER)
	}
#endif

	return status;
}

//
// setup device tree
//
EFI_STATUS CsSetupDeviceTree(BOOT_ARGS* bootArgs)
{
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// boot video
		//
		bootArgs->BootVideo.BaseAddress										= 0;
		CsInitializeBootVideo(&bootArgs->BootVideo);

		if (!bootArgs->BootVideo.BaseAddress)
		{
			memset(&bootArgs->BootVideo, 0, sizeof(bootArgs->BootVideo));
		}

		//
		// setup display mode
		//
		bootArgs->BootVideo.DisplayMode										= BlTestBootMode(BOOT_MODE_GRAPH) ? 1 : 2;

        CsInitializeBootVideoV1(&bootArgs->BootVideoV1);

        if (!bootArgs->BootVideoV1.BaseAddress)
        {
            memset(&bootArgs->BootVideoV1, 0, sizeof(bootArgs->BootVideoV1));
        }

        bootArgs->BootVideoV1.DisplayMode                                        = BlTestBootMode(BOOT_MODE_GRAPH) ? 1 : 2;

		//
		// hi-dpi mode
		//
		if (CspHiDPIMode)
		{
			bootArgs->Flags													|= 2;	// kBootArgsFlagHiDPI
		}

		//
		// allocate FailedCLUT
		//
		UINTN bufferLength													= CspHiDPIMode ? sizeof(CspFailedLogoLookupTable2x) : sizeof(CspFailedLogoLookupTable);
		UINTN allocatedLength												= bufferLength;
		UINT64 physicalAddress												= MmAllocateKernelMemory(&allocatedLength, NULL);

		if (!physicalAddress)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
		}

		//
		// add memory node
		//
		BlAddMemoryRangeNode(CHAR8_CONST_STRING("FailedCLUT"), physicalAddress, bufferLength);

		//
		// copy FailedCLUT
		//
		memcpy(ArchConvertAddressToPointer(physicalAddress, VOID*), CspHiDPIMode ? CspFailedLogoLookupTable2x : CspFailedLogoLookupTable, bufferLength);

		//
		// boot progress info
		//
		typedef struct _BOOT_PROGRESS_ELEMENT
		{
			//
			// width
			//
			UINT32															Width;

			//
			// heigth
			//
			UINT32															Height;

			//
			// y offset
			//
			UINT32															OffsetY;

			//
			// reserved
			//
			UINT32															Reserved[5];
		}BOOT_PROGRESS_ELEMENT;

		//
		// allocate FailedImage
		//
		bufferLength														= (CspHiDPIMode ? sizeof(CspFailedLogo2x) : sizeof(CspFailedLogo)) + sizeof(BOOT_PROGRESS_ELEMENT);
		allocatedLength														= bufferLength;
		physicalAddress														= MmAllocateKernelMemory(&allocatedLength, 0);

		if (!physicalAddress)
		{
#if defined(_MSC_VER)
			try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
		}

		//
		// add memory node
		//
		BlAddMemoryRangeNode(CHAR8_CONST_STRING("FailedImage"), physicalAddress, bufferLength);

		//
		// setup info and copy FailedImage
		//
		BOOT_PROGRESS_ELEMENT* element										= ArchConvertAddressToPointer(physicalAddress, BOOT_PROGRESS_ELEMENT*);
		element->Height														= CspHiDPIMode ? 200 : 100;
		element->Width														= element->Height;
		element->OffsetY													= 0;
		memcpy(element + 1, CspHiDPIMode ? CspFailedLogo2x : CspFailedLogo, bufferLength - sizeof(BOOT_PROGRESS_ELEMENT));
#if defined(_MSC_VER)
	}
	__finally
	{
	}
#endif

	return status;
}

//
// clear screen
//
VOID CsClearScreen()
{
	CspScreenNeedRedraw														= FALSE;
	CspFillRect(0, 0, CspHorzRes, CspVertRes, CspBackgroundClear);
}

//
// finalize()
//
EFI_STATUS CsFinalize()
{
	if (!CspIndicatorRefreshTimerEvent)
	{
		return EFI_SUCCESS;
	}

	EfiBootServices->CloseEvent(CspIndicatorRefreshTimerEvent);
	CspIndicatorRefreshTimerEvent											= NULL;
	CspIndicatorRefreshTimerEventNotifyRoutine(CspIndicatorRefreshTimerEvent, &CspIndicatorRefreshTimerEvent);

	return EFI_SUCCESS;
}

//
// draw preview
//
VOID CsDrawPreview(HIBERNATE_PREVIEW* previewBuffer, UINT32 imageIndex, UINT8 progressSaveUnder[HIBERNATE_PROGRESS_COUNT][HIBERNATE_PROGRESS_SAVE_UNDER_SIZE], BOOLEAN colorMode, BOOLEAN fromFV2, INT32* gfxRestoreStatus)
{
	VOID* configBuffer														= NULL;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// setup graph
		//
		if (!fromFV2 && (EFI_ERROR(CsConnectDevice(FALSE, TRUE)) || EFI_ERROR(CsInitializeGraphMode())))
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (configBuffer)
            {
                MmFreePool(configBuffer);
            }

            return;
#endif
		}

		//
		// get graph info
		//
		if (EFI_ERROR(CsInitializeBootVideo(NULL)) || !CspFrameBufferAddress)
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (configBuffer)
            {
                MmFreePool(configBuffer);
            }

            return;
#endif
		}

		//
		// save gfx restore status
		//
		if (gfxRestoreStatus)
		{
			*gfxRestoreStatus												= CspGfxSavedConfigRestoreStatus;
		}

		//
		// allocate config buffer
		//
		if (ArchConvertPointerToAddress(CspGraphConfigProtocol) >= 2)
		{
			configBuffer													= MmAllocatePool(0x1800);
		}

		//
		// error gfx status
		//
		if (CspGfxSavedConfigRestoreStatus < -1)
		{
			previewBuffer													= NULL;
		}

		//
		// show preview buffer or boot image
		//
		if (previewBuffer && !EFI_ERROR(CspDecodeAndDrawPreviewBuffer(previewBuffer, imageIndex, CspFrameBufferAddress, CspHorzRes, CspVertRes, CspColorDepth, CspBytesPerRow, colorMode, configBuffer)))
		{
			if (ArchConvertPointerToAddress(CspGraphConfigProtocol) >= 2)
			{
				if (colorMode)
				{
					CspRestoreGraphConfig(0x400, configBuffer, Add2Ptr(configBuffer, 0x800, VOID*), Add2Ptr(configBuffer, 0x1000, VOID*));
				}
				else
				{
					CspRestoreGraphConfig(0, NULL, NULL, NULL);
				}
				
			}
		}
		else if (!colorMode)
		{
			CsDrawBootImage(TRUE);
			CspRestoreGraphConfig(0, NULL, NULL, NULL);
		}

		//
		// calc pixel shift (16 = 1, 32 = 2)
		//
		UINT32 pixelShift													= CspColorDepth >> 4;

		if (!progressSaveUnder || pixelShift < 1)
		{
#if defined(_MSC_VER)
			try_leave(NOTHING);
#else
            if (configBuffer)
            {
                MmFreePool(configBuffer);
            }

            return;
#endif
		}

		//
		// calc screen buffer
		//
		UINT8* screenBuffer													= ArchConvertAddressToPointer(CspFrameBufferAddress, UINT8*);
		screenBuffer														+= (CspHorzRes - HIBERNATE_PROGRESS_COUNT * (HIBERNATE_PROGRESS_WIDTH + HIBERNATE_PROGRESS_SPACING)) << (pixelShift - 1);
		screenBuffer														+= (CspVertRes - HIBERNATE_PROGRESS_ORIGINY - HIBERNATE_PROGRESS_HEIGHT) * CspBytesPerRow;

		UINTN index[HIBERNATE_PROGRESS_COUNT]								= {0};

		for (UINTN y = 0; y < HIBERNATE_PROGRESS_HEIGHT; y++)
		{
			VOID* outputBuffer												= screenBuffer + y * CspBytesPerRow;

			for (UINTN blob = 0; blob < HIBERNATE_PROGRESS_COUNT; blob++)
			{
				UINT32 color												= blob ? HIBERNATE_PROGRESS_DARK_GRAY : HIBERNATE_PROGRESS_MID_GRAY;

				for (UINTN x = 0; x < HIBERNATE_PROGRESS_WIDTH; x++)
				{
					UINT8 alpha												= HbpProgressAlpha[y][x];
					UINT32 result											= color;

					if (alpha)
					{
						if (0xff != alpha)
						{
							UINT8 dstColor									= *(UINT8*)(outputBuffer);
							if (pixelShift == 1)
								dstColor									= ((dstColor & 0x1f) << 3) | ((dstColor & 0x1f) >> 2);

							progressSaveUnder[blob][index[blob]]			= dstColor;
							index[blob]										+= 1;
							result											= ((255 - alpha) * dstColor + alpha * result) / 255;
						}

						if (pixelShift == 1)
						{
							result											>>= 3;
							*(UINT16*)(outputBuffer)				= (UINT16)((result << 10) | (result << 5) | result);
						}
						else
						{
							*(UINT32*)(outputBuffer)				= (result << 16) | (result << 8) | result;
						}
					}

					outputBuffer											= Add2Ptr(outputBuffer, (UINT32)(1 << pixelShift), VOID*);
				}

				outputBuffer												= Add2Ptr(outputBuffer, HIBERNATE_PROGRESS_SPACING << pixelShift, VOID*);
			}
		}
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if (configBuffer)
		{
			MmFreePool(configBuffer);
		}
#if defined(_MSC_VER)
	}
#endif
}

//
// update progress
//
VOID CsUpdateProgress(UINT8 progressSaveUnder[HIBERNATE_PROGRESS_COUNT][HIBERNATE_PROGRESS_SAVE_UNDER_SIZE], UINTN prevBlob, UINTN currentBlob)
{
	//
	// calc pixel shift (16 = 1, 32 = 2)
	//
	UINT32 pixelShift														= CspColorDepth >> 4;

	if (pixelShift < 1 || !CspFrameBufferAddress)
	{
		return;
	}

	//
	// calc screen buffer
	//
	UINT8* screenBuffer														= ArchConvertAddressToPointer(CspFrameBufferAddress, UINT8*);
	screenBuffer															+= (CspHorzRes - HIBERNATE_PROGRESS_COUNT * (HIBERNATE_PROGRESS_WIDTH + HIBERNATE_PROGRESS_SPACING)) << (pixelShift - 1);
	screenBuffer															+= (CspVertRes - HIBERNATE_PROGRESS_ORIGINY - HIBERNATE_PROGRESS_HEIGHT) * CspBytesPerRow;

	UINTN lastBlob															= currentBlob < HIBERNATE_PROGRESS_COUNT ? currentBlob : HIBERNATE_PROGRESS_COUNT - 1;
	screenBuffer															+= (prevBlob * (HIBERNATE_PROGRESS_WIDTH + HIBERNATE_PROGRESS_SPACING)) << pixelShift;
	UINTN index[HIBERNATE_PROGRESS_COUNT]									= {0};

	for (UINTN y = 0; y < HIBERNATE_PROGRESS_HEIGHT; y++)
	{
		VOID* outputBuffer													= screenBuffer + y * CspBytesPerRow;

		for (UINTN blob = prevBlob; blob <= lastBlob; blob++)
		{
			UINT32 color													= blob < currentBlob ? HIBERNATE_PROGRESS_LIGHT_GRAY : HIBERNATE_PROGRESS_MID_GRAY;
			for (UINTN x = 0; x < HIBERNATE_PROGRESS_WIDTH; x++)
			{
				UINT8 alpha													= HbpProgressAlpha[y][x];
				UINT32 result												= color;

				if (alpha)
				{
					if (0xff != alpha)
					{
						UINT8 dstColor										= progressSaveUnder[blob][index[blob]];
						index[blob]											+= 1;
						result												= ((255 - alpha) * dstColor + alpha * result) / 255;
					}

					if (pixelShift == 1)
					{
						result												>>= 3;
						*(UINT16*)(outputBuffer)					= (UINT16)((result << 10) | (result << 5) | result);
					}
					else
					{
						*(UINT32*)(outputBuffer)					= (result << 16) | (result << 8) | result;
					}
				}

				outputBuffer												= Add2Ptr(outputBuffer, (UINT32)(1 << pixelShift), VOID*);
			}

			outputBuffer													= Add2Ptr(outputBuffer, HIBERNATE_PROGRESS_SPACING << pixelShift, VOID*);
		}
	}
}
