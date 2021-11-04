#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/CodeGen/GlobalISel/MachineIRBuilder.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Object/Binary.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryBufferRef.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "Cpu.h"
#include "Memory.h"
#include "AsmTranslator.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#define GET_INSTRINFO_ENUM
#include "ARMGenInstrInfo.inc"

using namespace llvm;
using namespace llvm::orc;

ExitOnError ExitOnErr;
CPU *Cpu;

CPU::CPU(): PrivM(PrivMode::Normal), Mode(CPUMode::ARM), InstrWidth(4)
{
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  InitializeAllDisassemblers();

  ExitOnErr.setBanner("gbaemu: ");

  std::memset(&GPR[0], 0, sizeof(int) * Register::RegCount);
  setPC(0x8000000U);
  Cpu = this;
}

void CPU::InitializeDisassembler()
{
  std::string Error;
  std::string TripleName("armv7-unknown-unknown");

  const Target *TheTarget = TargetRegistry::lookupTarget(TripleName, Error);

  MRI = TheTarget->createMCRegInfo(TripleName);

  MCTargetOptions MCOptions;
  AsmInfo = TheTarget->createMCAsmInfo(*MRI, TripleName, MCOptions);

  ARMSTI = TheTarget->createMCSubtargetInfo(TripleName, "arm7tdmi", "");

  MII = TheTarget->createMCInstrInfo();

  MCContext Ctx(Triple(TripleName), AsmInfo, MRI, ARMSTI);

  ARMDisAsm = TheTarget->createMCDisassembler(*ARMSTI, Ctx);

  ThumbSTI = TheTarget->createMCSubtargetInfo(TripleName, "arm7tdmi", "+thumb-mode");

  ThumbDisAsm = TheTarget->createMCDisassembler(*ThumbSTI, Ctx);

  int AsmPrinterVariant = AsmInfo->getAssemblerDialect();
  IP = TheTarget->createMCInstPrinter(Triple(TripleName), AsmPrinterVariant, *AsmInfo, *MII, *MRI);
}

CPU::DecodeStatus CPU::disassemble(ArrayRef<uint8_t> BinaryCode, MCInst &Inst)
{
  MCDisassembler *DisAsm = getARMOrThumb(ARMDisAsm, ThumbDisAsm);
  uint64_t Size = getARMOrThumb(4, 2);
  return DisAsm->getInstruction(Inst, Size, BinaryCode, 0, outs());
}

bool CPU::InstrFetch(MCInst &Inst)
{
  uint32_t ProgCounter = getIFProgCounter();
  assert((ProgCounter & (InstrWidth - 1)) == 0 && "Program Counter is not aligned !");
  ArrayRef<uint8_t> BinaryCode = Mem->getMemDataRef(ProgCounter, InstrWidth);
  advancePCtoNext();
  return disassemble(BinaryCode, Inst) == DecodeStatus::Success ? true : false;
}

void CPU::execute()
{
  auto J = ExitOnErr(LLJITBuilder().create());
  auto G = ExitOnErr(DynamicLibrarySearchGenerator::GetForCurrentProcess(J->getDataLayout().getGlobalPrefix()));
  J->getMainJITDylib().addGenerator(std::move(G));
  InitializeDisassembler();
  AsmTranslator AT(this);

  while(true) {
    // FIXME: find a way to avoid reconstrucing these things every time ?
    std::unique_ptr<LLVMContext> C = std::make_unique<LLVMContext>();
    std::unique_ptr<Module> Mod = std::make_unique<Module>("GBA", *C);

    APInt Addr(32, getIFProgCounter());
    std::string Name = toString(Addr, 16, false);
    Function *Main = Function::Create(FunctionType::get(Type::getInt32Ty(*C), {PointerType::getInt32PtrTy(*C)}, false),
                                      Function::ExternalLinkage,
                                      Name,
                                      Mod.get());
    IRBuilder<> IB(BasicBlock::Create(*C, "only_this_block", Main));
    MCInst Inst;

    while(true) {
      Instruction *I;
      if (!InstrFetch(Inst)) {
        errs() << "Instruction fetch failed\n";
        return;
      }

      switch(Inst.getOpcode()) {

#define HANDLE_MCINST(CODE) \
  case ARM::CODE:        \
    I = AT.translate##CODE(*C, Mod.get(), Main, IB, Inst); \
    break;

      HANDLE_MCINST(Bcc)
      HANDLE_MCINST(MOVi)
      HANDLE_MCINST(MSR)
      HANDLE_MCINST(LDRi12)
      HANDLE_MCINST(ADDri)
      HANDLE_MCINST(STRi12)
      HANDLE_MCINST(MOVr)
      HANDLE_MCINST(BX)
      HANDLE_MCINST(tPUSH)

      default:
        Mod->dump();
        errs() << "Unknown instruction\n";
        Inst.dump();
        printInst(Inst, getEXProgCounter());
        errs() << '\n';
        return;
      }

      printInst(Inst, getEXProgCounter());
      outs() << '\n';
      if (I && I->isTerminator()) {
        Mod->dump();
        break;
      }
    }

    ExitOnErr(J->addIRModule(ThreadSafeModule(std::move(Mod), std::move(C))));

    auto MainSymbol = ExitOnErr(J->lookup(Name));
    int (*MainFunc)(uint32_t *) = (int(*)(uint32_t *))MainSymbol.getAddress();
    int BranchAddress = MainFunc(&GPR[0]);
    outs() << "BranchAddress " << format_hex(BranchAddress, 8) << '\n';
    setPC(BranchAddress);
  }
}

void CPU::printInst(MCInst &Inst, uint64_t Address)
{
  outs() << format_hex(getIFProgCounter() - InstrWidth, 8) << ": ";
  if (Mode == CPUMode::ARM) {
    outs() << format_hex(*(uint32_t *)Mem->getPointer(getIFProgCounter() - InstrWidth), 8) << "  ";
  } else {
    outs() << format_hex(*(uint16_t *)Mem->getPointer(getIFProgCounter() - InstrWidth), 4) << "  ";
  }
  const MCSubtargetInfo *STI = getARMOrThumb(ARMSTI, ThumbSTI);
  IP->printInst(&Inst, Address, "", *STI, outs());
}

extern "C" {

void switchMode(uint32_t Addr) {
  Cpu->switchMode((Addr & 1) ? CPU::CPUMode::THUMB : CPU::CPUMode::ARM);
}

void writeCPSR(unsigned int bitmask, int val)
{
  outs() << "----------warning: writeCPSR still not working---------------\n";
}

void storeToMemory32(uint32_t Addr, uint32_t Val)
{
  outs() << "----------warning: MMIO still not working-----------\n";
  Cpu->StoreToMemory<uint32_t>(Addr, Val);
}

}