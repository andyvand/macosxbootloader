//********************************************************************
//	created:	8:11:2009   19:39
//	filename: 	NetBoot.cpp
//	author:		tiamo
//	purpose:	net boot
//********************************************************************

#include "StdAfx.h"

//
// get root match dict
//
CHAR8 CONST* NetGetRootMatchDict(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath)
{
	STATIC CHAR8 CONST part1Text[0xac]										= "<dict><key>IOProviderClass</key><string>IONetworkInterface</string><key>IOParentMatch</key>"
																			  "<dict><key>IOPropertyMatch</key><dict><key>IOMACAddress</key><data format=\"hex\">";
	STATIC CHAR8 CONST part2Text[0x1d]										= "</data></dict></dict></dict>";
	MAC_ADDR_DEVICE_PATH* macAddrDevicePath									= _CR(DevPathGetNode(bootDevicePath, MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP), MAC_ADDR_DEVICE_PATH, Header);
	if(!macAddrDevicePath)
		return NULL;

	UINTN macAddressLength													= macAddrDevicePath->IfType < 2 ? 6 : sizeof(macAddrDevicePath->MacAddress);
	UINTN bufferLength														= (ARRAYSIZE(part1Text) + macAddressLength * 2 + ARRAYSIZE(part2Text)) * sizeof(CHAR8);
	CHAR8* dictBuffer														= (CHAR8*)(MmAllocatePool(bufferLength));
	if(!dictBuffer)
		return NULL;

	strcpy(dictBuffer, part1Text);
	CHAR8* writeBuffer														= dictBuffer + ARRAYSIZE(part1Text) - 1;
	for(UINTN i = 0; i < macAddressLength; i ++, writeBuffer += 2)
		snprintf(writeBuffer, bufferLength / sizeof(CHAR8), CHAR8_CONST_STRING("%02x"), macAddrDevicePath->MacAddress.Addr[i]);
	
	strcpy(writeBuffer, part2Text);
	return dictBuffer;
}

//
// insert info into device
//
EFI_STATUS NetSetupDeviceTree(EFI_HANDLE bootDeviceHandle)
{
	VOID* dhcpResponse														= 0;
	VOID* bsdpResponse														= 0;
	EFI_STATUS status														= EFI_SUCCESS;

#if defined(_MSC_VER)
	__try
	{
#endif
		//
		// get protocol from device handle
		//
		APPLE_NET_BOOT_PROTOCOL* netBootProtocol							= NULL;
		if(EFI_ERROR(EfiBootServices->HandleProtocol(bootDeviceHandle, &AppleNetBootProtocolGuid, (VOID**)(&netBootProtocol))))
		{
            if(EFI_ERROR(EfiBootServices->LocateProtocol(&AppleNetBootProtocolGuid, NULL, (VOID**)(&netBootProtocol)))) {
#if defined(_MSC_VER)
                try_leave(NOTHING);
#else
                return status;
#endif
            }
        }

		//
		// get chosen node
		//
		DEVICE_TREE_NODE* theNode											= DevTreeFindNode(CHAR8_CONST_STRING("/chosen"), TRUE);
        if(!theNode) {
#if defined(_MSC_VER)
            try_leave(status = EFI_OUT_OF_RESOURCES);
#else
            status = EFI_OUT_OF_RESOURCES;
            return status;
#endif
        }

		//
		// get length
		//
		UINTN bufferSize													= 0;
        if(netBootProtocol->GetDhcpResponse(netBootProtocol, &bufferSize, NULL) != EFI_BUFFER_TOO_SMALL) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// allocate buffer
		//
		dhcpResponse														= MmAllocatePool(bufferSize);
        if(!dhcpResponse) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            return status;
#endif
        }

		//
		// get and add it
		//
		if(!EFI_ERROR(netBootProtocol->GetDhcpResponse(netBootProtocol, &bufferSize, dhcpResponse)))
			DevTreeAddProperty(theNode, CHAR8_CONST_STRING("dhcp-response"), dhcpResponse, (UINT32)(bufferSize), TRUE);

		//
		// get length
		//
		bufferSize															= 0;
        if(netBootProtocol->GetBsdpResponse(netBootProtocol, &bufferSize, NULL) != EFI_BUFFER_TOO_SMALL) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(dhcpResponse)
                MmFreePool(dhcpResponse);

            return status;
#endif
        }

		//
		// allocate buffer
		//
		bsdpResponse														= MmAllocatePool(bufferSize);
        if(!bsdpResponse) {
#if defined(_MSC_VER)
            try_leave(NOTHING);
#else
            if(dhcpResponse)
                MmFreePool(dhcpResponse);

            return status;
#endif
        }

		//
		// get and add it
		//
		if(!EFI_ERROR(netBootProtocol->GetBsdpResponse(netBootProtocol, &bufferSize, bsdpResponse)))
			DevTreeAddProperty(theNode, CHAR8_CONST_STRING("bsdp-response"), bsdpResponse, (UINT32)(bufferSize), TRUE);
#if defined(_MSC_VER)
	}
	__finally
	{
#endif
		if(dhcpResponse)
			MmFreePool(dhcpResponse);

		if(bsdpResponse)
			MmFreePool(bsdpResponse);
#if defined(_MSC_VER)
	}
#endif

	return status;
}
