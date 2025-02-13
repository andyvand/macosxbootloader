/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GraphicsOutput.h

Abstract:

  Graphics Output Protocol from the UEFI 2.0 specification.

  Abstraction of a very simple graphics device.

--*/

#ifndef __GRAPHICS_OUTPUT_H__
#define __GRAPHICS_OUTPUT_H__

#include EFI_PROTOCOL_DEFINITION2 (UgaDraw)

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID { 0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a} }

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
  UINT32            RedMask;
  UINT32            GreenMask;
  UINT32            BlueMask;
  UINT32            ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
  PixelRedGreenBlueReserved8BitPerColor,
  PixelBlueGreenRedReserved8BitPerColor,
  PixelBitMask,
  PixelBltOnly,
  PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
  UINT32                     Version;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
  EFI_PIXEL_BITMASK          PixelInformation;
  UINT32                     PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
/*++

  Routine Description:
    Return the current video mode information.

  Arguments:
    This                  - Protocol instance pointer.
    ModeNumber            - The mode number to return information on.
    SizeOfInfo            - A pointer to the size, in bytes, of the Info buffer.
    Info                  - A pointer to callee allocated buffer that returns information about ModeNumber.

  Returns:
    EFI_SUCCESS           - Mode information returned.
    EFI_DEVICE_ERROR      - A hardware error occurred trying to retrieve the video mode.
    EFI_NOT_STARTED       - Video display is not initialized. Call SetMode () 
    EFI_INVALID_PARAMETER - One of the input args was NULL.

--*/
;

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL * This,
  IN  UINT32                       ModeNumber
  )
/*++

  Routine Description:
    Return the current video mode information.

  Arguments:
    This             - Protocol instance pointer.
    ModeNumber       - The mode number to be set.

  Returns:
    EFI_SUCCESS      - Graphics mode was changed.
    EFI_DEVICE_ERROR - The device had an error and could not complete the request.
    EFI_UNSUPPORTED  - ModeNumber is not supported by this device.

--*/
;

typedef EFI_UGA_PIXEL EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef union {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Pixel;
  UINT32                        Raw;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION;

typedef enum {
  EfiBltVideoFill,
  EfiBltVideoToBltBuffer,
  EfiBltBufferToVideo, 
  EfiBltVideoToVideo,
  EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            * This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           * BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  );

/*++

  Routine Description:
    The following table defines actions for BltOperations:
    EfiBltVideoFill - Write data from the  BltBuffer pixel (SourceX, SourceY) 
      directly to every pixel of the video display rectangle 
      (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height). 
      Only one pixel will be used from the BltBuffer. Delta is NOT used.
    EfiBltVideoToBltBuffer - Read data from the video display rectangle 
      (SourceX, SourceY) (SourceX + Width, SourceY + Height) and place it in 
      the BltBuffer rectangle (DestinationX, DestinationY ) 
      (DestinationX + Width, DestinationY + Height). If DestinationX or 
      DestinationY is not zero then Delta must be set to the length in bytes 
      of a row in the BltBuffer.
    EfiBltBufferToVideo - Write data from the  BltBuffer rectangle 
      (SourceX, SourceY) (SourceX + Width, SourceY + Height) directly to the 
      video display rectangle (DestinationX, DestinationY) 
      (DestinationX + Width, DestinationY + Height). If SourceX or SourceY is 
      not zero then Delta must be set to the length in bytes of a row in the 
      BltBuffer.
    EfiBltVideoToVideo - Copy from the video display rectangle (SourceX, SourceY)
     (SourceX + Width, SourceY + Height) .to the video display rectangle 
     (DestinationX, DestinationY) (DestinationX + Width, DestinationY + Height). 
     The BltBuffer and Delta  are not used in this mode.

  Arguments:
    This          - Protocol instance pointer.
    BltBuffer     - Buffer containing data to blit into video buffer. This 
                    buffer has a size of Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
    BltOperation  - Operation to perform on BlitBuffer and video memory
    SourceX       - X coordinate of source for the BltBuffer.
    SourceY       - Y coordinate of source for the BltBuffer.
    DestinationX  - X coordinate of destination for the BltBuffer.
    DestinationY  - Y coordinate of destination for the BltBuffer.
    Width         - Width of rectangle in BltBuffer in pixels.
    Height        - Hight of rectangle in BltBuffer in pixels.
    Delta         -
  
  Returns:
    EFI_SUCCESS           - The Blt operation completed.
    EFI_INVALID_PARAMETER - BltOperation is not valid.
    EFI_DEVICE_ERROR      - A hardware error occured writting to the video 
                             buffer.

--*/
;

typedef struct {
  UINT32                                 MaxMode;
  UINT32                                 Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
  UINTN                                  SizeOfInfo;
  EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
  UINTN                                  FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT         Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

extern EFI_GUID gEfiGraphicsOutputProtocolGuid;

#endif
