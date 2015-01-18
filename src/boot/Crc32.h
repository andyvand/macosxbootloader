//********************************************************************
//	created:	11:11:2009   0:16
//	filename: 	Crc32.h
//	author:		tiamo
//	purpose:	crc32
//********************************************************************

#pragma once

#ifndef _CRC32_H_
#define _CRC32_H_

//
// calc crc32
//
UINT32 BlCrc32(UINT32 crcValue, VOID CONST* inputBuffer, UINTN bufferLength);

#endif
