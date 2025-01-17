//********************************************************************
//	created:	14:9:2012   20:29
//	filename: 	Hibernate.h
//	author:		tiamo
//	purpose:	hibernate
//********************************************************************

#ifndef __HIBERNATE_H__
#define __HIBERNATE_H__

#ifdef _MSC_VER
#pragma once
#endif /* _MSC_VER */

#include "macros.h"

#define HIBERNATE_PROGRESS_COUNT											19
#define HIBERNATE_PROGRESS_SAVE_UNDER_SIZE									38
#define HIBERNATE_PROGRESS_WIDTH											7
#define HIBERNATE_PROGRESS_HEIGHT											16
#define HIBERNATE_PROGRESS_SPACING											3
#define HIBERNATE_PROGRESS_ORIGINY											81
#define HIBERNATE_PROGRESS_DARK_GRAY										92
#define HIBERNATE_PROGRESS_MID_GRAY											174
#define HIBERNATE_PROGRESS_LIGHT_GRAY										230

#ifdef _MSC_VER
#include <pshpack1.h>
#endif /* MSC_VER */

//
// preview
//
typedef struct _HIBERNATE_PREVIEW
{
	//
	// image count
	//
	UINT32																	ImageCount;

	//
	// width
	//
	UINT32																	Width;

	//
	// height
	//
	UINT32																	Height;

	//
	// depth
	//
	UINT32																	Depth;

	//
	// lock time
	//
	UINT32																	LockTime;

	//
	// reserved
	//
	UINT32																	ReservedG[8];

	//
	// reserved
	//
	UINT32																	ReservedK[8];
} HIBERNATE_PREVIEW GNUPACK;

ASSERT_TRUE(sizeof(HIBERNATE_PREVIEW) == 84);

#ifdef _MSC_VER
#include <poppack.h>
#endif /* MSC_VER */

//
// start resume from hiberate
//
BOOLEAN HbStartResumeFromHibernate(UINT8* coreStorageVolumeKeyIdent);

//
// continue resume
//
VOID HbContinueResumeFromHibernate(UINT8* coreStorageVolumeKey, UINTN coreStorageVolumeKeyLength);

#endif /* __HIBERNATE_H__ */
