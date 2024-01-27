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
PUBLIC_SYMBOL _ArchThunk64BufferStart
									incbin				"ThunkCode64.dat"
PUBLIC_SYMBOL _ArchThunk64BufferEnd
