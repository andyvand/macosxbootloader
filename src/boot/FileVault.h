//********************************************************************
//	created:	19:9:2012   18:18
//	filename: 	FileVault.h
//	author:		tiamo	
//	purpose:	FileVault2
//********************************************************************

#ifndef __FILEVAULT_H__
#define __FILEVAULT_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// load and unlock core volume
//
EFI_STATUS FvLookupUnlockCoreVolumeKey(EFI_DEVICE_PATH_PROTOCOL* bootDevicePath, BOOLEAN resumeFromCoreStorage);

//
// find volume key
//
BOOLEAN FvFindCoreVolumeKey(UINT8 CONST* volumeIdent, UINT8* volumeKey, UINTN volumeKeyLength);

//
// setup device tree
//
EFI_STATUS FvSetupDeviceTree(UINT64* keyStorePhysicalAddress, UINTN* keyStoreSize, BOOLEAN setupDeviceTree);

#endif /* __FILEVAULT_H__ */
