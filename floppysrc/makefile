all : tools.bin

tools.bin : build/asm.o build/c.o
	wlink @tools.lnk

build/asm.o : tools.asm | build
	wasm -0 -bt=RAW tools.asm -fo=build/asm.o

build/c.o : tools.c | build
	wcc -0 -zl -s -bt=raw -ms tools.c -fo=build/c.o

build : 
	mkdir build

clean :
	rm -r build