global _ArchStartKernel
%ifdef ARCH64
_ArchStartKernel:
	cli
	mov rdx, [esp+8]
	mov rax, [esp+16]
	call rdx
	retn
%else
_ArchStartKernel:
	cli
	mov edx, [esp+4]
	mov eax, [esp+8]
	call edx
	retn
%endif

