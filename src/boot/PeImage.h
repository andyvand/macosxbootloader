//********************************************************************
//	created:	6:11:2009   23:43
//	filename: 	PeImage.h
//	author:		tiamo
//	purpose:	pe image
//********************************************************************

#pragma once

#ifndef _PEIMAGE_H_
#define _PEIMAGE_H_

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

#endif
