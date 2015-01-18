//********************************************************************
//	created:	20:9:2012   21:16
//	filename: 	Base64.h
//	author:		tiamo
//	purpose:	base64
//********************************************************************

#pragma once

#ifndef _BASE64_H_
#define _BASE64_H_

//
// decode
//
UINTN Base64Decode(CHAR8 CONST* inputString, UINTN inputLength, VOID* outputBuffer, UINTN* outputLength);

#endif
