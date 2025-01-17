//********************************************************************
//	created:	20:9:2012   21:16
//	filename: 	Base64.h
//	author:		tiamo
//	purpose:	base64
//********************************************************************

#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// decode
//
UINTN Base64Decode(CHAR8 CONST* inputString, UINTN inputLength, VOID* outputBuffer, UINTN* outputLength);

#endif /* __BASE64_H__ */
