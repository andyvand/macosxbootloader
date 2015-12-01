### efilipo makefile ###
CC = gcc
LD = $(CC)
CFLAGS = -g0 -arch i386 -O3
LDFLAGS = -g0 -arch i386
INSTALL = install
NASM = nasm
LIPO = lipo
CODESIGN = codesign

### Change to "" for no code signing or to your Apple developer certificate ###
SIGNCERT = ""
PKGSIGNCERT = ""
#SIGNCERT = "Developer ID Application: Andy Vandijck (GSF3NR4NQ5)"
#PKGSIGNCERT = "Developer ID Installer: Andy Vandijck (GSF3NR4NQ5)"

all: efilipo

efilipo: efilipo.o
	$(LD) $(LDFLAGS) -o $@ efilipo.o -L/usr/lib
	if [ $(SIGNCERT) != "" ]; then $(CODESIGN) -s $(SIGNCERT) $@; fi

efilipo.o: efilipo.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -Rf *.o efilipo *.pkg

newinstaller: efilipo
	sudo mkdir -p efilipoinst/usr/bin
	mkdir -p efilipopkg
	mkdir -p efilipocombopkg
	sudo $(INSTALL) $< efilipoinst/usr/bin/$<
	sudo chown -R root:wheel efilipoinst
	cd efilipoinst && sudo rm -f .DS_Store */.DS_Store */*/.DS_Store && sudo cpio -o < ../efilipo_pkg.txt > ../efilipopkg/Payload && sudo rm -f .DS_Store */.DS_Store */*/.DS_Store && sudo mkbom . ../efilipopkg/Bom && cd ..
	sudo rm -Rf efilipoinst
	sudo cp -Rf Installer/PackageInfo efilipopkg/PackageInfo
	cd efilipopkg && sudo rm -Rf .DS_Store && sudo xar -cjf ../efilipocombopkg/efilipo-1.1.pkg . && cd ..
	sudo rm  -Rf efilipopkg Payload Bom
	if [ $(PKGSIGNCERT) != "" ]; then sudo productsign --sign $(PKGSIGNCERT) efilipocombopkg/efilipo-1.1.pkg efilipocombopkg/efilipo-1.1-apple.pkg && sudo rm -Rf efilipocombopkg/efilipo-1.1.pkg; else mv efilipocombopkg/efilipo-1.1.pkg efilipocombopkg/efilipo-1.1-apple.pkg; fi
	sudo cp -Rf Installer/Resources efilipocombopkg/Resources
	sudo cp -f Installer/Distribution efilipocombopkg/Distribution
	cd efilipocombopkg &&  sudo rm -Rf .DS_Store */.DS_Store */*/.DS_Store && sudo productbuild --distribution Distribution --resources Resources --package-path $(PWD) ../efilipo-apple.pkg && cd ..
	sudo rm -Rf efilipocombopkg
	if [ $(PKGSIGNCERT) != "" ]; then sudo productsign --sign $(PKGSIGNCERT) efilipo-apple.pkg efilipo.pkg && sudo rm -Rf efilipo-apple.pkg; else mv efilipo-apple.pkg efilipo.pkg; fi

legacy-installer: efilipo
	sudo mkdir -p efilipoinst/usr/bin
	mkdir -p efilipopkg/Contents
	sudo $(INSTALL) $< efilipoinst/usr/bin/$<
	sudo chown -R root:wheel efilipoinst
	cd efilipoinst && sudo rm -f .DS_Store */.DS_Store */*/.DS_Store && sudo cpio -o < ../efilipo_pkg.txt > ../efilipopkg/Contents/Archive.pax && gzip -v9 ../efilipopkg/Contents/Archive.pax && sudo rm -f .DS_Store */.DS_Store */*/.DS_Store && sudo mkbom . ../efilipopkg/Contents/Archive.bom && cd ..
	sudo rm -Rf efilipoinst
	sudo cp -Rf LegacyInstaller/* efilipopkg/Contents/
	sudo sudo rm -Rf efilipo-1.1.pkg
	sudo mv -f efilipopkg efilipo-1.1.pkg
	sudo chown -R $(USER):staff efilipo-1.1.pkg
	sudo rm -Rf efilipo-legacy.pkg
	if [ $(SIGNCERT) != "" ]; then productsign --sign $(SIGNCERT) efilipo-1.1.pkg efilipo-legacy.pkg; else mv efilipo-1.1.pkg efilipo-legacy.pkg; fi
	rm -Rf efilipo-1.1.pkg

