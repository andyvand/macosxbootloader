TOPDIR=../..
PREBUILT=$(TOPDIR)/Prebuilt
CC=../../Prebuilt/clang
CXX=../../Prebuilt/clang

### Rijndael Mac OS X makefile ###

%.o: %.c
	$(CC) $(CFLAGS) $(ASMCOMPFLAGS) -c $< -o $@

%.o: %.asm
	$(NASM) $(NASMFLAGS) $(ASMCOMPFLAGS) $(NASMCOMPFLAGS) -o $@ $<

librijndael.a: aes_modes.o aescrypt.o aeskey.o aestab.o aesxts.o $(EXTRAOBJS)
	$(AR) cru $@ aes_modes.o aescrypt.o aeskey.o aestab.o aesxts.o $(EXTRAOBJS)
	$(RANLIB) $@

clean:
	rm -f *.a *.o

