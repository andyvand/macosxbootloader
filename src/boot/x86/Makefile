### x86 ###

TOPDIR2 = ../../..
PREBUILT2 = ../../../Prebuilt
CXXC = $(PREBUILT2)/clang
NASM2 = $(PREBUILT2)/nasm

ARCH2CFLAGS = -arch i386 -target i386-pc-win32-macho -funsigned-char -fno-ms-extensions -fno-stack-protector -fno-builtin -fshort-wchar -mno-implicit-float -mms-bitfields -ftrap-function=undefined_behavior_has_been_optimized_away_by_clang -DAPPLEEXTRA -Duint_8t=unsigned\ char -Duint_16t=unsigned\ short -Duint_32t=unsigned\ int -Duint_64t=unsigned\ long\ long -DBRG_UI8=1 -DBRG_UI16=1 -DBRG_UI32=1 -DBRG_UI64=1 -D__i386__=1 -DARCH32=1 -D__APPLE__=1 -D__i386__=1

C2FLAGS = -Wall -Werror $(DEBUGFLAGS) $(ARCHFLAGS) $(ARCH2CFLAGS) $(HACKFLAGS) -I../../../include -fborland-extensions -nostdinc $(ARCHCFLAGS) -std=gnu11 -Oz -DEFI_SPECIFICATION_VERSION=0x0001000a -DTIANO_RELEASE_VERSION=1 -fno-exceptions

CXX2FLAGS = -Wall -Werror $(DEBUGFLAGS) $(HACKFLAGS) -fborland-extensions -nostdinc $(ARCH2CFLAGS) -Oz -DEFI_SPECIFICATION_VERSION=0x0001000a -DTIANO_RELEASE_VERSION=1 -I../../../include -fno-exceptions -std=gnu++11

%.o: %.asm
	$(NASM2) $(NASMFLAGS) -o $@ $<

%.o: %.c
	$(CXXC) $(C2FLAGS) -c $< -o $@

%.o: %.cpp
	$(CXXC) $(CXX2FLAGS) -c $< -o $@

%.dat: %.asm
	$(NASM2) -f bin -o $@ $<

all: libx86.a

libx86.a: ThunkCode64.dat ThunkCode32.o ArchUtilsX86.o CompilerX86.o Thunk64.o DebuggerUtilsX86.o $(EXTRAOBJS)
	$(AR) cru $@ ThunkCode32.o ArchUtilsX86.o CompilerX86.o Thunk64.o DebuggerUtilsX86.o $(EXTRAOBJS)
	$(RANLIB) $@

clean:
	rm -f *.dat *.o *.a

