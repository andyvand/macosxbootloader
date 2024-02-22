%ifdef ARCH32
global ArchStartKernel
ArchStartKernel:
%else
global _ArchStartKernel
_ArchStartKernel:
%endif
	cli
	mov edx, [esp+4]
	mov eax, [esp+8]
	call edx
	retn
