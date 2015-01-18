;*********************************************************************
;	created:	6:10:2009   16:24
;	filename: 	PortRegisterIo.asm
;	author:		tiamo
;	purpose:	io
;*********************************************************************

									default rel
									[bits 64]

%include "Common.inc"

%ifdef APPLE
PUBLIC_ROUTINE _Z20ARCH_READ_PORT_UINT8Ph
%else
PUBLIC_ROUTINE ?ARCH_READ_PORT_UINT8@@YAEPEAE@Z
%endif
									mov					dx, cx
									in					al, dx
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z21ARCH_READ_PORT_UINT16Pt
%else
PUBLIC_ROUTINE ?ARCH_READ_PORT_UINT16@@YAGPEAG@Z
%endif
									mov					dx, cx
									in					ax, dx
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z21ARCH_READ_PORT_UINT32Pj
%else
PUBLIC_ROUTINE ?ARCH_READ_PORT_UINT32@@YAIPEAI@Z
%endif
									mov					dx, cx
									in					eax, dx
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z21ARCH_WRITE_PORT_UINT8Phh
%else
PUBLIC_ROUTINE ?ARCH_WRITE_PORT_UINT8@@YAXPEAEE@Z
%endif
									mov					al, dl
									mov					dx, cx
									out					dx, al
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z22ARCH_WRITE_PORT_UINT16Ptt
%else
PUBLIC_ROUTINE ?ARCH_WRITE_PORT_UINT16@@YAXPEAGG@Z
%endif
									mov					ax, dx
									mov					dx, cx
									out					dx, ax
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z22ARCH_WRITE_PORT_UINT32Pjj
%else
PUBLIC_ROUTINE ?ARCH_WRITE_PORT_UINT32@@YAXPEAII@Z
%endif
									mov					eax, edx
									mov					dx, cx
									out					dx, eax
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z24ARCH_READ_REGISTER_UINT8Ph
%else
PUBLIC_ROUTINE ?ARCH_READ_REGISTER_UINT8@@YAEPEAE@Z
%endif
									mov					al, [rcx]
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z25ARCH_READ_REGISTER_UINT16Pt
%else
PUBLIC_ROUTINE ?ARCH_READ_REGISTER_UINT16@@YAGPEAG@Z
%endif
									mov					ax, [rcx]
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z25ARCH_READ_REGISTER_UINT32Pj
%else
PUBLIC_ROUTINE ?ARCH_READ_REGISTER_UINT32@@YAIPEAI@Z
%endif
									mov					eax, [rcx]
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z25ARCH_WRITE_REGISTER_UINT8Phh
%else
PUBLIC_ROUTINE ?ARCH_WRITE_REGISTER_UINT8@@YAXPEAEE@Z
%endif
									xor					eax, eax
									mov					[rcx], dl
							lock	or					[rsp], eax
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z26ARCH_WRITE_REGISTER_UINT16Ptt
%else
PUBLIC_ROUTINE ?ARCH_WRITE_REGISTER_UINT16@@YAXPEAGG@Z
%endif
									xor					eax, eax
									mov					[rcx], dx
							lock	or					[rsp], eax
									retn

%ifdef APPLE
PUBLIC_ROUTINE _Z26ARCH_WRITE_REGISTER_UINT32Pjj
%else
PUBLIC_ROUTINE ?ARCH_WRITE_REGISTER_UINT32@@YAXPEAII@Z
%endif
									xor					eax, eax
									mov					[rcx], rdx
							lock	or					[rsp], eax
									retn
