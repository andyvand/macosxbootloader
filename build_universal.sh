#!/bin/sh
#

### Universal EFI binary build script for Xcode and GNU tools ###
make clean

if [ "${HACKINTOSH}" = "1"; then
HACKMAKE=HACKINTOSH=1
else
HACKMAKE= 
fi

if [ "${CLOVERTOOLS}" = "1" ]; then
echo "Building GNU x64 boot.efi"
make CLOVERTOOLS=1 ${HACKMAKE}
else
echo "Building Apple x64 boot.efi"
make ${HACKMAKE}
fi

cp src/boot/boot.efi boot.64.efi
make clean

if [ "${CLOVERTOOLS}" = "1" ]; then
echo "Building GNU x86 boot.efi"
make CLOVERTOOLS=1 ARCH=i386 ${HACKMAKE}
else
echo "Building Apple x86 boot.efi"
make ARCH=i386 ${HACKMAKE}
fi

cp src/boot/boot.efi boot.32.efi

echo "Building universal EFI binary"
./efilipo/efilipo boot.efi boot.64.efi boot.32.efi
make clean

rm -f boot.64.efi boot.32.efi

mkdir -p bin
mv -f boot.efi bin/boot.efi

echo "Build done, result is in bin :-D"
