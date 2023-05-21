SOURCES = $(wildcard VirtualH89/Src/*.cpp)

CHECK = `which scan-build`
UNCRUSTIFY = uncrustify

OBJECTS = $(subst .cpp,.o,$(SOURCES))

.PHONY: clean check uncrust

all: v89

v89: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) -lpthread -lGL -lglut

CXXFLAGS = -g -std=c++11

clean:
	rm -f *.o *.orig

check:
	$(CHECK) -stats -load-plugin alpha.cplusplus.VirtualCall -load-plugin alpha.deadcode.UnreachableCode -maxloop 20 -k --use-analyzer Xcode -o check xcodebuild

uncrust:
	$(UNCRUSTIFY) -c VirtualH89/uncrust.cfg --no-backup VirtualH89/Src/*.cpp VirtualH89/Src/*.h
