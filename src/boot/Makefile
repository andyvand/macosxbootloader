### Boot ###

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

ifeq ("%(ARCH)", "i386")
ARCHDEP = x86/libx86.a
else
ARCHDEP = x64/libx64.a
endif

all: boot.efi
boot.efi: Base64.o BootArgs.o BootDebugger.o Config.o Console.o Crc32.o Debug1394.o DebugUsb.o DevicePath.o DeviceTree.o FileIo.o FileVault.o GuidDefine.o Hibernate.o LoadDrivers.o LoadKernel.o MachO.o Main.o MD5.o Memory.o MemoryMap.o MiscUtils.o NetBoot.o Options.o PanicDialog.o PeImage.o PlatformExpert.o RuntimeLib.o SHA256.o StdAfx.o AcpiUtils.o ../rijndael/librijndael.a $(ARCHDEP)
	$(LD) $(LDFLAGS) -o $@.tmp Base64.o BootArgs.o BootDebugger.o Config.o Console.o Crc32.o Debug1394.o DebugUsb.o DevicePath.o DeviceTree.o FileIo.o FileVault.o GuidDefine.o Hibernate.o LoadDrivers.o LoadKernel.o MachO.o Main.o MD5.o Memory.o MemoryMap.o MiscUtils.o NetBoot.o Options.o PanicDialog.o PeImage.o PlatformExpert.o RuntimeLib.o SHA256.o AcpiUtils.o StdAfx.o ../rijndael/librijndael.a $(ARCHDEP)
	$(MTOC) $@.tmp $@
	rm -f $@.tmp

clean:
	rm -f *.o *.efi

