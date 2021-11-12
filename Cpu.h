#ifndef __VIRTGBA_CPU_H
#define __VIRTGBA_CPU_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "Memory.h"
#include "AsmTranslator.h"

using namespace llvm;

class CPU {
  friend class AsmTranslator;

public:

  using DecodeStatus = MCDisassembler::DecodeStatus;

  enum CPUMode {
    ARM   = 0,
    THUMB = 1,
  };

  CPU();
  void execute();
  int getInstrWidth() { return InstrWidth; }
  void setMem(Memory *M) { Mem = M; }

  template<typename T>
  void StoreToMemory(uint32_t Addr, T Val) {
    *((T *)Mem->getPointer(Addr)) = Val;
  }

  void switchMode(CPUMode M) {
    Mode = M;
    InstrWidth = getARMOrThumb(4, 2);
    if (M == CPUMode::ARM) {
      CPSR &= ~(1 << 5);
    } else {
      CPSR |= (1 << 5);
    }
  };

  enum PrivMode {
    Normal = 0,
    FIQ,
    SVC,
    ABT,
    IRQ,
    UND,
  };

  enum Register {
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13, // SP
    R14, // LR
    R8_FIQ,
    R9_FIQ,
    R10_FIQ,
    R11_FIQ,
    R12_FIQ,
    R13_FIQ,
    R13_SVC,
    R13_ABT,
    R13_IRQ,
    R13_UND,
    R14_FIQ,
    R14_SVC,
    R14_ABT,
    R14_IRQ,
    R14_UND,
    PC,
    RegCount,
  };
  
private:
  DecodeStatus InstrFetch(MCInst &, uint32_t &);
  void printInst(MCInst &, uint64_t);
  DecodeStatus disassemble(ArrayRef<uint8_t>, MCInst &);
  PrivMode getPrivMode() { return PrivM; }
  template<typename T>
  T getARMOrThumb(T Arm, T Thumb) {
    return Mode == CPUMode::ARM ? Arm : Thumb;
  }
  void setPC(uint32_t pc) { GPR[Register::PC] = pc; }
  uint32_t getPC() { return GPR[Register::PC]; }
  uint32_t getIFProgCounter() { return getPC(); }
  uint32_t getEXProgCounter() { return getPC() + InstrWidth; }
  void addOffsetToPC(uint32_t Offset) { GPR[Register::PC] += Offset; }
  void advancePCtoNext() { addOffsetToPC(InstrWidth); }

  PrivMode PrivM;
  // General Purpose Register
  uint32_t GPR[Register::RegCount];
  // Control and Status Register
  uint32_t CPSR;
  uint32_t SPSR[6];

  CPUMode Mode;
  Memory *Mem;
  int InstrWidth;
  MCRegisterInfo *MRI;
  MCAsmInfo *AsmInfo;
  MCInstrInfo *MII;
  MCDisassembler *ARMDisAsm;
  MCDisassembler *ThumbDisAsm;
  MCInstPrinter  *IP;
  const MCSubtargetInfo *ARMSTI;
  const MCSubtargetInfo *ThumbSTI;
};

#endif