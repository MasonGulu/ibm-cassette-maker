OBJS=build/main.o
WOBJS=bulid/main.wo

all : ibmwriter.out ibmwriter.exe

ibmwriter.out : build/main.o
	g++ -o build/ibmwriter.out -static build/main.o

build/main.o : src/main.cpp
	g++ -o build/main.o -c src/main.cpp

ibmwriter.exe : build/main.wo
	x86_64-w64-mingw32-g++ -o build/ibmwriter.exe -static build/main.wo

build/main.wo : src/main.cpp
	x86_64-w64-mingw32-g++ -o build/main.wo -c src/main.cpp

$(OBJS) : | build

$(WOBJS) : | build

build : 
	mkdir build

clean :
	rm -r build ibmwriter.out ibmwriter.exe