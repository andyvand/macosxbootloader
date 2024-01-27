;*********************************************************************
;	created:	6:10:2009   16:24
;	filename: 	PortRegisterIo.asm
;	author:		tiamo
;	purpose:	io
;*********************************************************************

									default rel
									[bits 64]

%include "Common.inc"

PUBLIC_ROUTINE ARCH_READ_PORT_UINT8
									mov					dx, cx
									in					al, dx
									retn

PUBLIC_ROUTINE ARCH_READ_PORT_UINT16
									mov					dx, cx
									in					ax, dx
									retn

PUBLIC_ROUTINE ARCH_READ_PORT_UINT32
									mov					dx, cx
									in					eax, dx
									retn

PUBLIC_ROUTINE ARCH_WRITE_PORT_UINT8
									mov					al, dl
									mov					dx, cx
									out					dx, al
									retn

PUBLIC_ROUTINE ARCH_WRITE_PORT_UINT16
									mov					ax, dx
									mov					dx, cx
									out					dx, ax
									retn

PUBLIC_ROUTINE ARCH_WRITE_PORT_UINT32
									mov					eax, edx
									mov					dx, cx
									out					dx, eax
									retn

PUBLIC_ROUTINE ARCH_READ_REGISTER_UINT8
									mov					al, [rcx]
									retn

PUBLIC_ROUTINE ARCH_READ_REGISTER_UINT16
									mov					ax, [rcx]
									retn

PUBLIC_ROUTINE ARCH_READ_REGISTER_UINT32
									mov					eax, [rcx]
									retn

PUBLIC_ROUTINE ARCH_WRITE_REGISTER_UINT8
									xor					eax, eax
									mov					[rcx], dl
							lock	or					[rsp], eax
									retn

PUBLIC_ROUTINE ARCH_WRITE_REGISTER_UINT16
									xor					eax, eax
									mov					[rcx], dx
							lock	or					[rsp], eax
									retn

PUBLIC_ROUTINE ARCH_WRITE_REGISTER_UINT32
									xor					eax, eax
									mov					[rcx], rdx
							lock	or					[rsp], eax
									retn
