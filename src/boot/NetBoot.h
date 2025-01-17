//********************************************************************
//	created:	8:11:2009   19:41
//	filename: 	NetBoot.h
//	author:		tiamo
//	purpose:	net boot
//********************************************************************

#ifndef __NETBOOT_H__
#define __NETBOOT_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// get root match
//
CHAR8 CONST* NetGetRootMatchDict(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath);

//
// insert info into device
//
EFI_STATUS NetSetupDeviceTree(EFI_HANDLE bootDeviceHandle);

#endif /* __NETBOOT_H__ */
