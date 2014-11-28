### x86 ###

%.o: %.asm
	$(NASM) $(NASMFLAGS) -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.dat: %.asm
	$(NASM) -f bin -o $@ $<

all: libx86.a

libx86.a: StartKernel.o ThunkCode64.dat ThunkCode32.o ThunkCode64.o ArchUtilsX86.o CompilerX86.o Thunk64.o # DebuggerUtilsX86.o
	$(AR) cru $@ StartKernel.o ThunkCode32.o ThunkCode64.o ArchUtilsX86.o CompilerX86.o Thunk64.o # DebuggerUtilsX86.o
	$(RANLIB) $@

clean:
	rm -f *.dat *.o *.a

