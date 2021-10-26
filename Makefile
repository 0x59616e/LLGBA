CXXFLAGS=$(shell llvm-config --cxxflags)
LDFLAGS=$(shell llvm-config --ldflags)
INCLUDE_PATH=$(shell llvm-config --includedir)
SYSTEM_LIBS=$(shell llvm-config --system-libs)
CPPFLAGS=$(shell llvm-config --cppflags)
LIBS=$(shell llvm-config --libs Support Target MC ARM X86)
LD_PATH=/home/sheng/project/mold/bin

all: gbaemu

gbaemu: gbaemu.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -B$(LD_PATH) -I$(INCLUDE_PATH) $(LDFLAGS) $< $(LIBS) $(SYSTEM_LIBS) -o $@

clear:
	rm gbaemu