%ifdef ARCH32
global ?ArchStartKernel@@YAXPAX0@Z
?ArchStartKernel@@YAXPAX0@Z:
%else
global __Z15ArchStartKernelPvS_
__Z15ArchStartKernelPvS_:
%endif
	cli
	mov edx, [esp+4]
	mov eax, [esp+8]
	call edx
	retn
