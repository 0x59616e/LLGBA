CXXFLAGS=$(shell llvm-config --cxxflags) -Wall -g -Werror
LDFLAGS=$(shell llvm-config --ldflags)
ARM_INCLUDE_PATH=/home/sheng/project/compiler/llvm-project/build/lib/Target/ARM
SYSTEM_LIBS=$(shell llvm-config --system-libs)
LIBS=$(shell llvm-config --libs Support Target MC ARM X86 Core OrcJIT)
LD_PATH=/home/sheng/project/mold/bin
SOURCES=$(wildcard *.cc)
HEADERS=$(wildcard *.h)
OBJECTS=$(patsubst %.cc, %.o, $(SOURCES))
PROJ=LLGBA

all: $(PROJ) $(HEADERS)
#-fuse-ld=/home/sheng/project/mold/mold
$(PROJ): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -Xlinker --export-dynamic -B$(LD_PATH)  $(LDFLAGS) $(OBJECTS) $(LIBS) $(SYSTEM_LIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -fPIC -I$(ARM_INCLUDE_PATH) -c $< -o $@

run: all
	./$(PROJ) rom.gba

clear:
	rm *.o
	rm $(PROJ)