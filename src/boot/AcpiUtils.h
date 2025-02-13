//********************************************************************
//	created:	6:11:2009   15:28
//	filename: 	AcpiUtils.h
//	author:		tiamo
//	purpose:	acpi utils
//********************************************************************

#ifndef __ACPIUTILS_H__
#define __ACPIUTILS_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// get fadt
//
VOID CONST* AcpiGetFixedAcpiDescriptionTable();


// get machine signature
//
EFI_STATUS AcpiGetMachineSignature(UINT32* machineSignature);

//
// get pci config space info
//
EFI_STATUS AcpiGetPciConfigSpaceInfo(UINT64* baseAddress, UINT32* startBus, UINT32* endBus);

//
// detect acpi nvs memory
//
VOID AcpiDetectNVSMemory();

//
// adjust memory map for acpi nvs memory
//
UINTN AcpiAdjustMemoryMap(EFI_MEMORY_DESCRIPTOR* memoryMap, UINTN memoryMapSize, UINTN descriptorSize);

//
// checksum8 from AppleSMBIOS.kext
//
UINT8 Checksum8(VOID * start, unsigned int length);

#endif /* __ACPIUTILS_H__ */
