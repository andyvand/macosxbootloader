/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiBind.h

Abstract:

  Processor or Compiler specific defines and types for IA-32.
  We are using the ANSI C 2000 _t type definitions for basic types.
  This it technically a violation of the coding standard, but they
  are used to make EfiTypes.h portable. Code other than EfiTypes.h
  should never use any ANSI C 2000 _t integer types.

--*/

#ifndef _EFI_BIND_H_
#define _EFI_BIND_H_

#ifdef EFI_DEBUG

#ifdef EFI_NT_EMULATOR

#define EFI_DRIVER_ENTRY_POINT(InitFunction)                  \
          EFI_STATUS                                          \
          EFIAPI                                              \
          InitFunction (                                      \
            EFI_HANDLE        ImageHandle,                    \
            EFI_SYSTEM_TABLE  *SystemTable                    \
            );                                                \
                                                              \
          UINTN                                               \
          __stdcall                                           \
          _DllMainCRTStartup (                                \
              UINTN    Inst,                                  \
              UINTN    reason_for_call,                       \
              VOID    *rserved                                \
              )                                               \
          {                                                   \
              return 1;                                       \
          }                                                   \
                                                              \
          EFI_STATUS                                          \
          __declspec( dllexport  )                            \
          __cdecl                                             \
          InitializeDriver (                                  \
              EFI_HANDLE       ImageHandle,                   \
              EFI_SYSTEM_TABLE *SystemTable                   \
              )                                               \
          {                                                   \
              return InitFunction(ImageHandle, SystemTable);  \
          }

#define EFI_APPLICATION_ENTRY_POINT EFI_DRIVER_ENTRY_POINT

#else

#define EFI_DRIVER_ENTRY_POINT(InitFunction)                  
#define EFI_APPLICATION_ENTRY_POINT EFI_DRIVER_ENTRY_POINT

#endif

#else

#define EFI_DRIVER_ENTRY_POINT(InitFunction)                  
#define EFI_APPLICATION_ENTRY_POINT EFI_DRIVER_ENTRY_POINT

#endif


//
// Make sure we are using the correct packing rules per EFI specification
//
#pragma pack(8)

#if _MSC_EXTENSIONS

//
// Disable warning that make it impossible to compile at /W4
// This only works for Microsoft* tools
//

//
// Disabling bitfield type checking warnings.
//
#pragma warning ( disable : 4214 )

//
// Disabling the unreferenced formal parameter warnings.
//
#pragma warning ( disable : 4100 )

//
// Disable slightly different base types warning as CHAR8 * can not be set
// to a constant string.
//
#pragma warning ( disable : 4057 )

//
// ASSERT(FALSE) or while (TRUE) are legal constructes so supress this warning
//
#pragma warning ( disable : 4127 )

//
// Int64ShllMod32 unreferenced inline function
//
#pragma warning ( disable : 4514 )

//
// Unreferenced formal parameter - We are object oriented, so we pass This even
//  if we  don't need them.
//
#pragma warning ( disable : 4100 )

//
// This warning is caused by empty (after preprocessing) souce file.
//
#pragma warning ( disable : 4206 )


#endif


#if (__STDC_VERSION__ < 199901L)
  //
  // No ANSI C 2000 stdint.h integer width declarations, so define equivalents
  //

  #if _MSC_EXTENSIONS
    //
    // use Microsoft* C complier dependent interger width types
    //
    typedef unsigned __int64    uint64_t;
    typedef __int64             int64_t;
    typedef unsigned __int32    uint32_t;
    typedef __int32             int32_t;
    typedef unsigned short      uint16_t;
    typedef short               int16_t;
    typedef unsigned char       uint8_t;
    #ifdef __GNUC__
      typedef __signed char       int8_t;
    #else
      typedef char                int8_t;
    #endif
  #else

    //
    // Assume standard IA-32 alignment.
    // BugBug: Need to check portability of long long
    //
    typedef unsigned long long  uint64_t;
    typedef long long           int64_t;
    typedef unsigned int        uint32_t;
    typedef int                 int32_t;
    typedef unsigned short      uint16_t;
    typedef short               int16_t;
    typedef unsigned char       uint8_t;
    #ifdef __GNUC__
      typedef __signed char       int8_t;
    #else
      typedef char                int8_t;
    #endif
  #endif
#else
  //
  // Use ANSI C 2000 stdint.h integer width declarations
  //
  #include <stdint.h>
#endif

//
// Native integer size in stdint.h
//
typedef uint32_t  uintn_t;
typedef int32_t   intn_t;

//
// Processor specific defines
//
#define EFI_MAX_BIT       0x80000000
#define MAX_2_BITS        0xC0000000

//
// Maximum legal IA-32 address
//
#define EFI_MAX_ADDRESS   0xFFFFFFFF

//
//  Bad pointer value to use in check builds.
//  if you see this value you are using uninitialized or free'ed data
//
#define EFI_BAD_POINTER          0xAFAFAFAF
#define EFI_BAD_POINTER_AS_BYTE  0xAF

//
// Inject a break point in the code to assist debugging for NT Emulation Environment
// For real hardware, just put in a halt loop. Don't do a while(1) because the
// compiler will optimize away the rest of the function following, so that you run out in
// the weeds if you skip over it with a debugger.
//
#define EFI_BREAKPOINT()  __asm { int 3 }
#define EFI_DEADLOOP()    { volatile UINTN __iii; __iii = 1; while (__iii); }

//
// Memory Fence forces serialization, and is needed to support out of order
//  memory transactions. The Memory Fence is mainly used to make sure IO
//  transactions complete in a deterministic sequence, and to syncronize locks
//  an other MP code. Currently no memory fencing is required.
//
#define MEMORY_FENCE()

//
// Some compilers don't support the forward reference construct:
//  typedef struct XXXXX. The forward reference is required for 
//  ANSI compatibility.
//
// The following macro provide a workaround for such cases.
//


#ifdef EFI_NO_INTERFACE_DECL
  #define EFI_FORWARD_DECLARATION(x)
#else
  #define EFI_FORWARD_DECLARATION(x) typedef struct _##x x
#endif


//
// Some C compilers optimize the calling conventions to increase performance.
// _EFIAPI is used to make all public APIs follow the standard C calling 
// convention.
//
#if _MSC_EXTENSIONS || defined(__clang__)
  //
  // Microsoft* compiler requires _EFIAPI useage, __cdecl is Microsoft* specific C.
  // 

  #define _EFIAPI __cdecl  
#else
  #define _EFIAPI       
#endif


#ifdef _EFI_WINNT

  #define EFI_SUPPRESS_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( disable : 4142 )

  #define EFI_DEFAULT_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( default : 4142 )
#else

  #define EFI_SUPPRESS_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( disable : 4068 )

  #define EFI_DEFAULT_BENIGN_REDEFINITION_OF_TYPE_WARNING()  \
           warning ( default : 4068 )

#endif



#endif

