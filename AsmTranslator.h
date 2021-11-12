#ifndef __VIRTGBA_ASMTRANSLATOR_H
#define __VIRTGBA_ASMTRANSLATOR_H

#define GET_REGINFO_ENUM
#include "ARMGenRegisterInfo.inc"

class CPU;

class AsmTranslator {

public:
  AsmTranslator(CPU *C) : Cpu(C) {}

  Instruction *translateBcc(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateMSR(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateMOVi(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateLDRi12(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateADDri(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateSTRi12(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateMOVr(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateBX(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetPUSH(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetSUBspi(LLVMContext&, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetBL(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetMOVi8(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetLSLri(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetLDRpci(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translatetSVC(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);
  Instruction *translateSTMDB_UPD(LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);

  Instruction *translateLDRi12(int, int, int,
                                LLVMContext &, Module *, Function *, IRBuilder<> &, uint32_t);
  Instruction *translateSTMDB(int, LLVMContext &, Module *, Function *, IRBuilder<> &, MCInst &, uint32_t);

  // helper function
  Value *CreateLoadFromReg(MCOperand &, Function *, LLVMContext &, IRBuilder<> &, uint32_t);
  Value *CreateLoadFromReg(int        , Function *, LLVMContext &, IRBuilder<> &, uint32_t);
  Instruction *CreateStoreToReg(Value *, MCOperand &, Function *, LLVMContext &, IRBuilder<> &);
  Instruction *CreateStoreToReg(Value *, int        , Function *, LLVMContext &, IRBuilder<> &);
  Value *CreateLoadFromMemory(Type *, Value *, Module *M, LLVMContext &, IRBuilder<> &);

private:
  CPU* Cpu;
};
#endif