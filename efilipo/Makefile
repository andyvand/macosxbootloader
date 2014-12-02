### efilipo makefile ###
CC = gcc
LD = $(CC)
CFLAGS = -g0 -arch i386 -arch x86_64 -Ofast
LDFLAGS = -g0 -arch i386 -arch x86_64
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

efilipo: efilipo.o swap.o
	$(LD) $(LDFLAGS) -o $@ efilipo.o swap.o
	if [ $(SIGNCERT) != "" ]; then $(CODESIGN) -s $(SIGNCERT) $@; fi

efilipo.o: efilipo.c
	$(CC) $(CFLAGS) -o $@ -c $<

swap.o: swap.asm
	$(NASM) -fmacho64 -o $@.64 $<
	$(NASM) -fmacho -o $@.32 $<
	$(LIPO) -create -output $@ $@.64 $@.32
	rm -f $@.64 $@.32

clean:
	rm -f *.o efilipo *.pkg

installer:  efilipo
	sudo mkdir -p efilipoinst/usr/bin
	mkdir -p efilipopkg
	mkdir -p efilipocombopkg
	sudo $(INSTALL) $< efilipoinst/usr/bin/$<
	sudo chown -R root:wheel efilipoinst
	cd efilipoinst && sudo rm -f .DS_Store */.DS_Store */*/.DS_Store && sudo cpio -o < ../efilipo_pkg.txt > ../efilipopkg/Payload && sudo rm -f .DS_Store */.DS_Store */*/.DS_Store && sudo mkbom . ../efilipopkg/Bom && cd ..
	sudo rm -Rf efilipoinst
	sudo cp -Rf Installer/PackageInfo efilipopkg/PackageInfo
	cd efilipopkg && sudo rm -Rf .DS_Store && sudo xar -cjf ../efilipocombopkg/efilipo-1.0.pkg . && cd ..
	sudo rm  -Rf efilipopkg Payload Bom
	if [ $(PKGSIGNCERT) != "" ]; then sudo productsign --sign $(PKGSIGNCERT) efilipocombopkg/efilipo-1.0.pkg efilipocombopkg/efilipo-1.0-apple.pkg && sudo rm -Rf efilipocombopkg/efilipo-1.0.pkg; else mv efilipocombopkg/efilipo-1.0.pkg efilipocombopkg/efilipo-1.0-apple.pkg; fi
	sudo cp -Rf Installer/Resources efilipocombopkg/Resources
	sudo cp -f Installer/Distribution efilipocombopkg/Distribution
	cd efilipocombopkg &&  sudo rm -Rf .DS_Store */.DS_Store */*/.DS_Store && sudo productbuild --distribution Distribution --resources Resources --package-path $(PWD) ../efilipo-apple.pkg && cd ..
	sudo rm -Rf efilipocombopkg
	if [ $(PKGSIGNCERT) != "" ]; then sudo productsign --sign $(PKGSIGNCERT) efilipo-apple.pkg efilipo.pkg && sudo rm -Rf efilipo-apple.pkg; else mv efilipo-apple.pkg efilipo.pkg; fi

