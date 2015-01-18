//********************************************************************
//	created:	7:11:2009   12:49
//	filename: 	PlatformExpert.h
//	author:		tiamo
//	purpose:	platform expert
//********************************************************************

#pragma once

#ifndef _PLATFORMEXPERT_H_
#define _PLATFORMEXPERT_H_

//
// init platform node
//
EFI_STATUS PeInitialize();

//
// get model name
//
CHAR8 CONST* PeGetModelName();

//
// setup device tree
//
EFI_STATUS PeSetupDeviceTree();

#endif
