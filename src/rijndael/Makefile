TOPDIR=../..
PREBUILT=$(TOPDIR)/Prebuilt
CC3=../../Prebuilt/clang
CXX3=../../Prebuilt/clang
NASM3=../../Prebuilt/nasm

### Rijndael Mac OS X makefile ###

%.o: %.c
	$(CC3) $(CFLAGS) $(ASMCOMPFLAGS) -c $< -o $@

%.o: %.asm
	$(NASM3) $(NASMFLAGS) $(ASMCOMPFLAGS) $(NASMCOMPFLAGS) -o $@ $<

librijndael.a: aes_modes.o aescrypt.o aeskey.o aestab.o aesxts.o $(EXTRAOBJS)
	$(AR) cru $@ aes_modes.o aescrypt.o aeskey.o aestab.o aesxts.o $(EXTRAOBJS)
	$(RANLIB) $@

clean:
	rm -f *.a *.o

