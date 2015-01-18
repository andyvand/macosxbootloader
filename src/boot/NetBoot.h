//********************************************************************
//	created:	8:11:2009   19:41
//	filename: 	NetBoot.h
//	author:		tiamo
//	purpose:	net boot
//********************************************************************

#pragma once

#ifndef _NETBOOT_H_
#define _NETBOOT_H_

//
// get root match
//
CHAR8 CONST* NetGetRootMatchDict(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath);

//
// insert info into device
//
EFI_STATUS NetSetupDeviceTree(EFI_HANDLE bootDeviceHandle);

#endif
