### x86 ###

%.o: %.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.dat: %.asm
	$(NASM) -f bin -o $@ $<

all: libx86.a

libx86.a: ThunkCode64.dat ThunkCode32.o ArchUtilsX86.o CompilerX86.o Thunk64.o DebuggerUtilsX86.o $(EXTRAOBJS)
	$(AR) cru $@ ThunkCode32.o ArchUtilsX86.o CompilerX86.o Thunk64.o DebuggerUtilsX86.o $(EXTRAOBJS)
	$(RANLIB) $@

clean:
	rm -f *.dat *.o *.a

