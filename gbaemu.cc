#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Object/Binary.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ROMSTART 0x8000000LL

using namespace llvm;

static cl::opt<std::string>
RomName(cl::Positional, cl::desc("<input rom>"));

class VirtGBA {
  uint8_t *RomStart;
  uint64_t RomSize;
  std::unique_ptr<MCDisassembler> ARMDisAsm;
  std::unique_ptr<MCDisassembler> ThumbDisAsm;

public:
  VirtGBA(std::string Name);
};

VirtGBA::VirtGBA(std::string Name) {
  int fd = open(RomName.c_str(), O_RDONLY);
  if (fd == -1) {
    std::cout << "Cannot open file '" << RomName << "'\n";
    exit(-1);
  }

  struct stat statbuff;
  if (fstat(fd, &statbuff) == -1) {
    std::cout << "Cannot get file size\n";
    exit(-1);
  }

  RomSize = (uint64_t)statbuff.st_size;
  // FIXME: Memory remapping
  RomStart = (uint8_t *)mmap((void *)ROMSTART, RomSize, PROT_READ, MAP_PRIVATE, fd, 0);
  assert((uint64_t)RomStart == ROMSTART);

  std::string Error;
  std::string TripleName("armv7-unknown-unknown");

  const Target *TheTarget = TargetRegistry::lookupTarget(TripleName, Error);
  if (!TheTarget) {
    std::cout << Error << std::endl;
    exit(-1);
  }

  std::unique_ptr<const MCRegisterInfo> MRI(TheTarget->createMCRegInfo(TripleName));
  if (!MRI) {
    std::cout << "no register info for target '" << TripleName <<"'\n";
    exit(-1);
  }

  MCTargetOptions MCOptions;
  std::unique_ptr<const MCAsmInfo> AsmInfo(
    TheTarget->createMCAsmInfo(*MRI, TripleName, MCOptions));
  if (!AsmInfo) {
    std::cout << "no assembly info for target '" << TripleName << "'\n";
    exit(-1);
  }

  std::unique_ptr<const MCSubtargetInfo> ARMSTI(
    TheTarget->createMCSubtargetInfo(TripleName, "arm7tdmi", ""));
  if (!ARMSTI) {
    std::cout << "no subtarget info for target '" << TripleName << "'\n";
    exit(-1);
  }

  std::unique_ptr<const MCInstrInfo> MII(TheTarget->createMCInstrInfo());
  if (!MII) {
    std::cout << "no instruction info for target '" << TripleName << "'\n";
    exit(-1);
  }

  MCContext Ctx(Triple(TripleName), AsmInfo.get(), MRI.get(), ARMSTI.get());

  ARMDisAsm = std::unique_ptr<MCDisassembler>(
    TheTarget->createMCDisassembler(*ARMSTI, Ctx));

  if (!ARMDisAsm) {
    std::cout << "no disassembler for target '" << TripleName << "'\n";
    exit(-1);
  }

  std::unique_ptr<const MCSubtargetInfo> ThumbSTI(
    TheTarget->createMCSubtargetInfo(TripleName, "arm7tdmi", "+thumb-mode"));
  ThumbDisAsm = std::unique_ptr<MCDisassembler> (
    TheTarget->createMCDisassembler(*ThumbSTI, Ctx));

  if (!ThumbDisAsm) {
    std::cout << "no thumb disassembler\n";
    exit(-1);
  }
}

int main(int argc, char *argv[])
{
  cl::ParseCommandLineOptions(argc, argv, " GBA Emulator\n");
  if (RomName == "") {
      errs() << "You must specify rom name\nUse --help to figure out how to use\n";
      return -1;
  }

  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  InitializeAllDisassemblers();

  VirtGBA GBA(RomName);

  return 0;
}
