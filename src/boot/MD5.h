//********************************************************************
//	created:	8:11:2009   20:04
//	filename: 	MD5.h
//	author:		tiamo
//	purpose:	md5
//********************************************************************

#ifndef __MD5_H__
#define __MD5_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#include "macros.h"

#ifdef _MSC_VER
#include <pshpack1.h>
#endif /* _MSC_VER */

//
// context
//
typedef struct _MD5_CONTEXT
{
	//
	// state
	//
	UINT32																	State[4];

	//
	// count
	//
	UINT32																	Count[2];

	//
	// input buffer
	//
	UINT8																	InputBuffer[64];
} MD5_CONTEXT GNUPACK;

#ifdef _MSC_VER
#include <poppack.h>
#endif /* _MSC_VER */

//
// init
//
VOID MD5Init(MD5_CONTEXT* md5Context);

//
// update
//
VOID MD5Update(MD5_CONTEXT* md5Context, VOID CONST* byteBuffer, UINT32 bufferLength);

//
// finish
//
VOID MD5Final(UINT8* md5Result, MD5_CONTEXT* md5Context);

#endif /* __MD5_H__ */
