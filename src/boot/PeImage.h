//********************************************************************
//	created:	6:11:2009   23:43
//	filename: 	PeImage.h
//	author:		tiamo
//	purpose:	pe image
//********************************************************************

#ifndef __PEIMAGE_H__
#define __PEIMAGE_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

//
// get image nt header
//
EFI_IMAGE_NT_HEADERS* PeImageNtHeader(VOID* imageBase);

//
// get image size
//
UINT32 PeImageGetSize(VOID* ntHeaders);

//
// get checksum
//
UINT32 PeImageGetChecksum(VOID* ntHeaders);

//
// get entry point
//
VOID* PeImageGetEntryPoint(VOID* imageBase);

#endif /* __PEIMAGE_H__ */
