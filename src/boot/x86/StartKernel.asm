global __Z15ArchStartKernelPvS_
__Z15ArchStartKernelPvS_:
	cli
	mov edx, [esp+4]
	mov eax, [esp+8]
	call edx
	retn
