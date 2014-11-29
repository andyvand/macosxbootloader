### efilipo makefile ###
CC = gcc
CFLAGS = -g0 -arch x86_64 -Ofast
NASM = nasm
LIPO = lipo

all: efilipo

efilipo: efilipo.o swap.o

efilipo.o: efilipo.c
	$(CC) $(CFLAGS) -o $@ -c $<

swap.o: swap.asm
	$(NASM) -fmacho64 -o $@.64 $<
	$(NASM) -fmacho -o $@.32 $<
	$(LIPO) -create -output $@ $@.64 $@.32
	rm -f $@.64 $@.32

clean:
	rm -f *.o efilipo

