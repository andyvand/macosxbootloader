#=-> macosxbootloader OS X Makefile <-=#

### Top level directory ###
TOPDIR=$(PWD)

### Debug ###
DEBUG =# 1
ifeq ("$(DEBUG)", "")
DEBUGFLAGS=-g0 -DNDEBUG
else
DEBUGFLAGS=-g3 -DDEBUG -D_DEBUG
endif

### Tools ###
ifeq ("$(CLOVERTOOLS)", "1")
ifeq ("$(TOOLPATH)", "")
TOOLPATH=$(HOME)/Downloads/CloverGrowerPro/toolchain/cross/bin/
endif
CC=$(TOOLPATH)x86_64-clover-linux-gnu-gcc
CXX=$(TOOLPATH)x86_64-clover-linux-gnu-g++
LD=$(TOOLPATH)x86_64-clover-linux-gnu-g++
AR=$(TOOLPATH)x86_64-clover-linux-gnu-ar
RANLIB=$(TOOLPATH)x86_64-clover-linux-gnu-ranlib
STRIP=$(TOOLPATH)x86_64-clover-linux-gnu-strip

### Architecture - Intel 32 bit / Intel 64 bit ###
ifeq ("$(ARCH)", "i386")
ARCHDIR=x86
ARCHFLAGS=-m32 -malign-double -fno-stack-protector -freorder-blocks -freorder-blocks-and-partition -mno-stack-arg-probe
NASMFLAGS=-f elf32 -DAPPLE
MTOC=$(TOOLPATH)x86_64-clover-linux-gnu-objcopy -I elf32-i386 -O pei-i386
else
ARCHDIR=x64
ARCHFLAGS=-m64 -fno-stack-protector -DNO_BUILTIN_VA_FUNCS -mno-red-zone -mno-stack-arg-probe
NASMFLAGS=-f elf64 -DAPPLE -DARCH64
MTOC=$(TOOLPATH)x86_64-clover-linux-gnu-objcopy -I elf64-x86-64 -O pei-x86-64
endif

CFLAGS = "$(DEBUGFLAGS) $(ARCHFLAGS) -fshort-wchar -fno-strict-aliasing -ffunction-sections -fdata-sections -fPIC -Os -DEFI_SPECIFICATION_VERSION=0x0001000a -DTIANO_RELEASE_VERSION=1 -I$(TOPDIR)/include -I/usr/include -DGNU -D__declspec\(x\)= -D__APPLE__"
CXXFLAGS = $(CFLAGS)

ifeq ("$(ARCH)", "i386")
ALDFLAGS = -melf_x86_64
else
ALDFLAGS = -melf_x86_64
endif

LDFLAGS = "$(DEBUGFLAGS) $(ARCHFLAGS) -nostdlib -n -Wl,--script,$(TOPDIR)/gcc4.9-ld-script -u _Z7EfiMainPvP17_EFI_SYSTEM_TABLE -e _Z7EfiMainPvP17_EFI_SYSTEM_TABLE --entry _Z7EfiMainPvP17_EFI_SYSTEM_TABLE --pie"

EXTRAOBJS=
else
CC=gcc
CXX=g++
LD=ld
AR=ar
RANLIB=ranlib
MTOC=mtoc -align 0x20

### Architecture - Intel 32 bit / Intel 64 bit ###
ifeq ("$(ARCH)", "i386")
ARCHDIR = x86
ARCHFLAGS = -arch i386
ARCHLDFLAGS = -u __Z7EfiMainPvP17_EFI_SYSTEM_TABLE -e __Z7EfiMainPvP17_EFI_SYSTEM_TABLE
NASMFLAGS = -f macho -DAPPLEUSE
STRIP = strip
ARCHCFLAGS =  -funsigned-char -fno-ms-extensions -fno-stack-protector -fno-builtin -fshort-wchar -mno-implicit-float -mms-bitfields -ftrap-function=undefined_behavior_has_been_optimized_away_by_clang -DAPPLEEXTRA
EXTRAOBJS="StartKernel.o"
else
ARCHDIR = x64
ARCHFLAGS = -arch x86_64
ARCHLDFLAGS =  -u ?EfiMain@@YA_KPEAXPEAU_EFI_SYSTEM_TABLE@@@Z -e ?EfiMain@@YA_KPEAXPEAU_EFI_SYSTEM_TABLE@@@Z
NASMFLAGS = -f macho64 -DARCH64 -DAPPLEUSE
STRIP = strip
ARCHCFLAGS = -target x86_64-pc-win32-macho -funsigned-char -fno-ms-extensions -fno-stack-protector -fno-builtin -fshort-wchar -mno-implicit-float -mms-bitfields -ftrap-function=undefined_behavior_has_been_optimized_away_by_clang
EXTRAOBJS=
endif

CFLAGS = "$(DEBUGFLAGS) $(ARCHFLAGS) -fborland-extensions $(ARCHCFLAGS) -fPIC -mno-implicit-float -mms-bitfields -msoft-float -Oz -DEFI_SPECIFICATION_VERSION=0x0001000a -DTIANO_RELEASE_VERSION=1 -I$(TOPDIR)/include -I/usr/include"
CXXFLAGS = $(CFLAGS)
LDFLAGS = "$(ARCHFLAGS) -preload -segalign 0x20 $(ARCHLDFLAGS) -pie -all_load -dead_strip -seg1addr 0x240"
endif

NASM=nasm

### Flags ###

all: rijndael $(ARCHDIR) boot efilipo

rijndael:
	cd src/rijndael && make -f Makefile CC="$(CC)" CFLAGS=$(CFLAGS) AR="$(AR)" RANLIB="$(RANLIB)" && cd ../..

x64:
	cd src/boot/x64 && make CXX="$(CXX)" CXXFLAGS=$(CXXFLAGS) NASM="$(NASM)" NASMFLAGS="$(NASMFLAGS)" AR="$(AR)" RANLIB="$(RANLIB)" && cd ../../..

x86:
	cd src/boot/x86 && make EXTRAOBJS=$(EXTRAOBJS) CXX="$(CXX)" CXXFLAGS=$(CXXFLAGS) NASM="$(NASM)" NASMFLAGS="$(NASMFLAGS)" AR="$(AR)" RANLIB="$(RANLIB)" && cd ../../..

boot:
	cd src/boot && make CC="$(CC)" CFLAGS=$(CFLAGS) CXX="$(CXX)" ARCH=$(ARCH) CXXFLAGS=$(CXXFLAGS) LD="$(LD)" LDFLAGS=$(LDFLAGS) STRIP="$(STRIP)" MTOC="$(MTOC)" && cd ../..

efilipo:
	cd efilipo && make && cd ..

clean:
	cd src/rijndael && make clean && cd ../..
	cd src/boot && make clean && cd ../..
	cd src/boot/x64 && make clean && cd ../../..
	cd src/boot/x86 && make clean && cd ../../..
	cd efilipo && make clean && cd ..
