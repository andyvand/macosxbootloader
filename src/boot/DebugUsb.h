//********************************************************************
//	created:	5:8:2012   15:03
//	filename: 	DebugUsb.h
//	author:		tiamo
//	purpose:	debug over usb
//********************************************************************

#ifndef __DEBUGUSB_H__
#define __DEBUGUSB_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#include "BootDebuggerPrivate.h"

//
// setup debug device
//
EFI_STATUS BdUsbConfigureDebuggerDevice(CHAR8 CONST* loaderOptions);

//
// send packet
//
VOID BdUsbSendPacket(UINT32 packetType, STRING* messageHeader, STRING* messageData);

//
// receive packet
//
UINT32 BdUsbReceivePacket(UINT32 packetType, STRING* messageHeader, STRING* messageData, UINT32* dataLength);

//
// close debug device
//
EFI_STATUS BdUsbCloseDebuggerDevice();

#endif /* __DEBUGUSB_H__ */
