#ifndef __VIRTGBA_ASMTRANSLATOR_H
#define __VIRTGBA_ASMTRANSLATOR_H

#define GET_REGINFO_ENUM
#include "ARMGenRegisterInfo.inc"

class CPU;

class AsmTranslator {

public:
  AsmTranslator(CPU *C) : Cpu(C) {}
  Instruction *translateBcc(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateMSR(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateMOVi(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateLDRi12(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateADDri(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateSTRi12(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateMOVr(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translateBX(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);
  Instruction *translatetPUSH(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &);

  // helper function
  Value *CreateLoadFromReg(MCOperand &, Function *, LLVMContext &, IRBuilder<> &);
  Value *CreateLoadFromReg(int        , Function *, LLVMContext &, IRBuilder<> &);
  Instruction *CreateStoreToReg(Value *, MCOperand &, Function *, LLVMContext &, IRBuilder<> &);
  Instruction *CreateStoreToReg(Value *, int        , Function *, LLVMContext &, IRBuilder<> &);
  Value *CreateLoadFromMemory(Type *, Value *, Module *M, LLVMContext &, IRBuilder<> &);

private:
  CPU* Cpu;
};
#endif