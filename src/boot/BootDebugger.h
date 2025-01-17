//********************************************************************
//	created:	6:11:2009   21:34
//	filename: 	BootDebugger.h
//	author:		tiamo
//	purpose:	boot debugger
//********************************************************************

#ifndef __BOOTDEBUGGER_H__
#define __BOOTDEBUGGER_H__

#if defined(_MSC_VER)
#pragma once
#endif /* _MSC_VER */

//
// initialize boot debugger
//
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

EFI_STATUS BdInitialize(CHAR8 CONST* loaderOptions);

//
// debugger is enabled
//
BOOLEAN BdDebuggerEnabled();

//
// poll break in
//
BOOLEAN BdPollBreakIn();

//
// poll connection
//
VOID BdPollConnection();

//
// dbg breakpoint
//
VOID BOOTAPI DbgBreakPoint();

//
// dbg print
//
UINT32 DbgPrint(CHAR8 CONST* printFormat, ...);

//
// destroy debugger
//
EFI_STATUS BdFinalize();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOOTDEBUGGER_H__ */
