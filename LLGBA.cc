#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "Cpu.h"
#include "Memory.h"
#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

using namespace llvm;

static cl::opt<std::string>
RomName(cl::Positional, cl::desc("<input rom>"));

class VirtGBA {
public:
  VirtGBA(std::string &RomName) {
    Mem.setRom(RomName);
    Mem.setBios("bios.bin");
    Cpu.setMem(&Mem);
  }
  void run();
private:
  CPU Cpu;
  Memory Mem;
};

void VirtGBA::run()
{
  Cpu.execute();
}

int main(int argc, char *argv[])
{
  InitLLVM X(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, " GBA Emulator\n");
  if (RomName == "") {
      errs() << "You must specify rom name\nUse --help to figure out how to use\n";
      return -1;
  }

  VirtGBA GBA(RomName);
  GBA.run();

  return 0;
}
