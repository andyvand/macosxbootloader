//********************************************************************
//	created:	20:9:2012   18:17
//	filename: 	SHA256.h
//	author:		tiamo	
//	purpose:	sha256
//********************************************************************

#ifndef __SHA256_H__
#define __SHA256_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#include "macros.h"

#if defined(_MSC_VER)
#include <pshpack1.h>
#endif /* _MSC_VER */

//
// context
//
typedef struct _SHA256_CONTEXT
{
	//
	// length
	//
	UINT32																	TotalLength[2];

	//
	// state
	//
	UINT32																	State[8];

	//
	// buffer
	//
	UINT8																	Buffer[64];
} SHA256_CONTEXT GNUPACK;

ASSERT_TRUE(sizeof(SHA256_CONTEXT) == 104);

#if defined(_MSC_VER)
#include <poppack.h>
#endif /* _MSC_VER */

//
// init
//
VOID SHA256_Init(SHA256_CONTEXT* sha256Context);

//
// update
//
VOID SHA256_Update(VOID CONST* dataBuffer, UINTN dataLength, SHA256_CONTEXT* sha256Context);

//
// final
//
VOID SHA256_Final(UINT8* resultBuffer, SHA256_CONTEXT* sha256Context);

//
// sha256 buffer
//
VOID SHA256(VOID CONST* dataBuffer, UINTN dataLength, UINT8* resultBuffer);

#endif /* __SHA256_H__ */
