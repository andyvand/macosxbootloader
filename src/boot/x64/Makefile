### x64 specific ###
TOPDIR2 = ../../..
CXXC = $(TOPDIR2)/Prebuilt/clang

ARCH2CFLAGS = -arch x86_64 -target x86_64-pc-win32-macho -funsigned-char -fno-ms-extensions -fno-stack-protector -fno-builtin -fshort-wchar -mno-implicit-float -msoft-float -mms-bitfields -ftrap-function=undefined_behavior_has_been_optimized_away_by_clang -D__x86_64__=1

CXX2FLAGS = -Wall $(DEBUGFLAGS) $(HACKFLAGS) -fborland-extensions -nostdinc $(ARCH2CFLAGS) -Oz -DEFI_SPECIFICATION_VERSION=0x0001000a -DTIANO_RELEASE_VERSION=1 -I../../../include -fno-exceptions -std=gnu++11

%.o: %.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

%.o: %.cpp
	$(CXXC) $(CXX2FLAGS) -o $@ -c $<

all: libx64.a

libx64.a: DebugTrap.o CompilerX64.o ArchUtilsX64.o DebuggerUtilsX64.o PortRegisterIo.o TransferRoutine.o
	$(AR) cru $@ DebugTrap.o CompilerX64.o ArchUtilsX64.o DebuggerUtilsX64.o PortRegisterIo.o TransferRoutine.o
	$(RANLIB) $@

clean:
	rm -f *.a *.o
