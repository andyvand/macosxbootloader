;;;*=->
;
; Segment definition (32 bit swap)
;
;;;
;
; Standard SEG32 (also 64), possible for SEG16 => 64/32 bit or 16 bit
;
;;;*=->

%ifdef SEG16
USE16

segment .text

align 2
%else
USE32

segment .text

align 4
%endif

;;;
;
; Optimal 32 bit swap function
;
;;;
global ___OSSwapInt32
___OSSwapInt32:

;;;
;
; Microsoft (cdecl) spec
;
;;;
%ifdef MSSTYLE ;=-> CDecl
%ifdef SEG16
	mov	dx, dx
	mov	ax, cx
%else
	mov	eax, edx
%endif

%else ;=-> SysV
;;;
;
; Unix (System V) spec
;
;;;
%ifdef SEG16
	mov	dx, di
	mov	ax, si
	xchg	ah, al
	xchg	dh, dl
%else
	mov	eax, edi
	bswap	eax
%endif
%endif

	ret
