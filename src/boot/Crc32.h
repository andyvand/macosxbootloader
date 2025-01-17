//********************************************************************
//	created:	11:11:2009   0:16
//	filename: 	Crc32.h
//	author:		tiamo
//	purpose:	crc32
//********************************************************************

#ifndef __CRC32_H__
#define __CRC32_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// calc crc32
//
UINT32 BlCrc32(UINT32 crcValue, VOID CONST* inputBuffer, UINTN bufferLength);

#endif /* __CRC32_H__ */
