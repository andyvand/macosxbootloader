;********************************************************************
;	created:	25:10:2009   16:53
;	filename: 	All_X86.asm
;	author:		tiamo
;	purpose:	all
;********************************************************************

%macro PUBLIC_SYMBOL 1
									global %1
									%1:
%endmacro

%ifdef APPLEUSE
section .data
%else
section .rdata
%endif
									align				16
%ifndef APPLE
%ifdef ARCH64
PUBLIC_SYMBOL ?ArchThunk64BufferStart@@3PAEA
%else
PUBLIC_SYMBOL _ArchThunk64BufferStart
%endif
%else
PUBLIC_SYMBOL ArchThunk64BufferStart
%endif
									incbin				"ThunkCode64.dat"
%ifndef APPLE
%ifdef ARCH64
PUBLIC_SYMBOL ?ArchThunk64BufferEnd@@3PAEA
%else
PUBLIC_SYMBOL _ArchThunk64BufferEnd
%endif
%else
PUBLIC_SYMBOL ArchThunk64BufferEnd
%endif
