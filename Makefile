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
TOOLPATH=/Users/andyvand/Downloads/CloverGrowerPro/toolchain2/cross/bin/
endif
CC=$(TOOLPATH)x86_64-clover-linux-gnu-gcc
CXX=$(TOOLPATH)x86_64-clover-linux-gnu-g++
LD=$(TOOLPATH)x86_64-clover-linux-gnu-g++
AR=$(TOOLPATH)x86_64-clover-linux-gnu-ar
RANLIB=$(TOOLPATH)x86_64-clover-linux-gnu-ranlib

### Architecture - Intel 32 bit / Intel 64 bit ###
ifeq ("$(ARCH)", "i386")
ARCHFLAGS=-m32 -malign-double -fno-stack-protector -freorder-blocks -freorder-blocks-and-partition -mno-stack-arg-probe
NASMFLAGS=-f elf32 -DAPPLE
MTOC=$(TOOLPATH)x86_64-clover-linux-gnu-objcopy -I elf32-i386 -O pei-i386
else
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
else
CC=gcc
CXX=g++
LD=ld
AR=ar
RANLIB=ranlib
MTOC=mtoc -align 0x20

### Architecture - Intel 32 bit / Intel 64 bit ###
ifeq ("$(ARCH)", "i386")
ARCHFLAGS = -arch i386
NASMFLAGS = -f macho -DAPPLE
ARCHCFLAGS = -funsigned-char -fno-stack-protector -fno-builtin -fshort-wchar -fasm-blocks -ftrap-function=undefined_behavior_has_been_optimized_away_by_clang
else
ARCHFLAGS = -arch x86_64
NASMFLAGS = -f macho64 -DARCH64 -DAPPLEUSE
ARCHCFLAGS = -target x86_64-pc-win32-macho -funsigned-char -fno-ms-extensions -fno-stack-protector -fno-builtin -fshort-wchar -mno-implicit-float -mms-bitfields -ftrap-function=undefined_behavior_has_been_optimized_away_by_clang
endif

CFLAGS = "$(DEBUGFLAGS) $(ARCHFLAGS) -fborland-extensions $(ARCHCFLAGS) -fPIC -mno-implicit-float -mms-bitfields -msoft-float -Oz -DEFI_SPECIFICATION_VERSION=0x0001000a -DTIANO_RELEASE_VERSION=1 -I$(TOPDIR)/include -I/usr/include"
CXXFLAGS = $(CFLAGS)
LDFLAGS = "$(ARCHFLAGS) -preload -segalign 0x20 -u ?EfiMain@@YA_KPEAXPEAU_EFI_SYSTEM_TABLE@@@Z -e ?EfiMain@@YA_KPEAXPEAU_EFI_SYSTEM_TABLE@@@Z -pie -all_load -dead_strip -seg1addr 0x240 suppress"
endif

NASM=nasm

### Flags ###

all: rijndael x64 x86 boot

rijndael:
	cd src/rijndael && make -f Makefile CC="$(CC)" AR="$(AR)" RANLIB="$(RANLIB)" CFLAGS=$(CFLAGS) && cd ../..

x64:
	cd src/boot/x64 && make CXX="$(CXX)" CXXFLAGS=$(CXXFLAGS) NASM="$(NASM)" NASMFLAGS="$(NASMFLAGS)" AR="$(AR)" RANLIB="$(RANLIB)" && cd ../../..

x86:
	cd src/boot/x86 && make CXX="$(CXX)" CXXFLAGS=$(CXXFLAGS) NASM="$(NASM)" NASMFLAGS="$(NASMFLAGS)" AR="$(AR)" RANLIB="$(RANLIB)" && cd ../../..

boot:
	cd src/boot && make CXX="$(CXX)" CXXFLAGS=$(CXXFLAGS) LD="$(LD)" LDFLAGS=$(LDFLAGS) MTOC="$(MTOC)" && cd ../..

clean:
	cd src/rijndael && make -f Makefile clean && cd ../..
	cd src/boot && make -f Makefile clean && cd ../..
	cd src/boot/x64 && make -f Makefile clean && cd ../../..
	cd src/boot/x86 && make -f Makefile clean && cd ../../..

