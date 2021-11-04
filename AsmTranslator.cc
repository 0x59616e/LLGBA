#include "Cpu.h"
#include "AsmTranslator.h"

int RegIdxTable[15][6] = {
  //  Normal           FIQ               SVC                ABT                IRQ                UND
  {CPU::Register::R0,  CPU::Register::R0,      CPU::Register::R0,      CPU::Register::R0,      CPU::Register::R0,      CPU::Register::R0},
  {CPU::Register::R1,  CPU::Register::R1,      CPU::Register::R1,      CPU::Register::R1,      CPU::Register::R1,      CPU::Register::R1},
  {CPU::Register::R2,  CPU::Register::R2,      CPU::Register::R2,      CPU::Register::R2,      CPU::Register::R2,      CPU::Register::R2},
  {CPU::Register::R3,  CPU::Register::R3,      CPU::Register::R3,      CPU::Register::R3,      CPU::Register::R3,      CPU::Register::R3},
  {CPU::Register::R4,  CPU::Register::R4,      CPU::Register::R4,      CPU::Register::R4,      CPU::Register::R4,      CPU::Register::R4},
  {CPU::Register::R5,  CPU::Register::R5,      CPU::Register::R5,      CPU::Register::R5,      CPU::Register::R5,      CPU::Register::R5},
  {CPU::Register::R6,  CPU::Register::R6,      CPU::Register::R6,      CPU::Register::R6,      CPU::Register::R6,      CPU::Register::R6},
  {CPU::Register::R7,  CPU::Register::R7,      CPU::Register::R7,      CPU::Register::R7,      CPU::Register::R7,      CPU::Register::R7},
  {CPU::Register::R8,  CPU::Register::R8_FIQ,  CPU::Register::R8,      CPU::Register::R8,      CPU::Register::R8,      CPU::Register::R8},
  {CPU::Register::R9,  CPU::Register::R9_FIQ,  CPU::Register::R9,      CPU::Register::R9,      CPU::Register::R9,      CPU::Register::R9},
  {CPU::Register::R10, CPU::Register::R10_FIQ, CPU::Register::R10,     CPU::Register::R10,     CPU::Register::R10,     CPU::Register::R10},
  {CPU::Register::R11, CPU::Register::R11_FIQ, CPU::Register::R11,     CPU::Register::R11,     CPU::Register::R11,     CPU::Register::R11},
  {CPU::Register::R12, CPU::Register::R12_FIQ, CPU::Register::R12,     CPU::Register::R12,     CPU::Register::R12,     CPU::Register::R12},
  {CPU::Register::R13, CPU::Register::R13_FIQ, CPU::Register::R13_SVC, CPU::Register::R13_ABT, CPU::Register::R13_IRQ, CPU::Register::R13_UND},
  {CPU::Register::R14, CPU::Register::R14_FIQ, CPU::Register::R14_SVC, CPU::Register::R14_ABT, CPU::Register::R14_IRQ, CPU::Register::R14_UND},
};

Instruction *AsmTranslator::translateMOVi(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // mov rd, imm
  Value *Imm = ConstantInt::get(C, APInt(32, Inst.getOperand(1).getImm()));
  return CreateStoreToReg(Imm, Inst.getOperand(0), Main, C, IB);
}

Instruction *AsmTranslator::translateBcc(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // b #offset
  int64_t TargetAddress = Inst.getOperand(0).getImm() + Cpu->getEXProgCounter();
  return IB.CreateRet(ConstantInt::get(C, APInt(32, TargetAddress)));
}

Instruction *AsmTranslator::translateMSR(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // msr (CPSR|SPSR) rs
  // fsxc
  // translate MSR to function call
  unsigned int bitmask;
  std::string FuncName;

  // why CPSR_fc is imm...???
  switch(Inst.getOperand(0).getImm()) {
  case 9:
    // CPSR_fc
    bitmask = 0x00ffff00;
    FuncName = "writeCPSR";
    break;
  default:
    errs() << "translateMSR: Unknown register\n";
    exit(-1);
  }

  Value *V = CreateLoadFromReg(Inst.getOperand(1), Main, C, IB);
  Function *F;
  if (!(F = M->getFunction(FuncName))) {
    F = Function::Create(FunctionType::get(Type::getVoidTy(C), {Type::getInt32Ty(C), Type::getInt32Ty(C)}, false), 
                                  Function::ExternalLinkage, std::move(FuncName), M);
  }
  return IB.CreateCall(F, {ConstantInt::get(C, APInt(32, bitmask)), V});
}


Instruction *AsmTranslator::translateLDRi12(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // ldr rd, [rs, #offset]
  Value *Rs = CreateLoadFromReg(Inst.getOperand(1), Main, C, IB);
  Value *Offset = ConstantInt::get(C, APInt(32, Inst.getOperand(2).getImm()));
  Value *Addr = IB.CreateAdd(Rs, Offset);
  
  Value *Val = CreateLoadFromMemory(PointerType::getInt32PtrTy(C), Addr, M, C, IB);

  return CreateStoreToReg(Val, Inst.getOperand(0), Main, C, IB);
}

Instruction *AsmTranslator::translateADDri(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // add rd, rs, #imm
  Value *Rs = CreateLoadFromReg(Inst.getOperand(1), Main, C, IB);
  Value *Res = IB.CreateAdd(Rs, ConstantInt::get(C, APInt(32, Inst.getOperand(2).getImm())));
  return CreateStoreToReg(Res, Inst.getOperand(0), Main, C, IB);
}

Instruction *AsmTranslator::translateSTRi12(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // str rs, [rd, #offset]
  Value *Rd = CreateLoadFromReg(Inst.getOperand(1), Main, C, IB);
  Value *Offset = ConstantInt::get(C, APInt(32, Inst.getOperand(2).getImm()));
  Value *Addr = IB.CreateAdd(Rd, Offset);
  Function *F;
  if (!(F = M->getFunction("storeToMemory32"))) {
    F = Function::Create(FunctionType::get(Type::getVoidTy(C), {Type::getInt32Ty(C), Type::getInt32Ty(C)}, false),
                          Function::ExternalLinkage, "storeToMemory32", M);
  }
  Value *Val = CreateLoadFromReg(Inst.getOperand(0), Main, C, IB);
  return IB.CreateCall(F, {Addr, Val});
}

Instruction *AsmTranslator::translateMOVr(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // mov rd, rs
  Value *Rs = CreateLoadFromReg(Inst.getOperand(1), Main, C, IB);
  return CreateStoreToReg(Rs, Inst.getOperand(0), Main, C, IB);
}

Instruction *AsmTranslator::translateBX(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // bx rd
  Function *F;
  if ((F = M->getFunction("switchMode")) == nullptr) {
    F = Function::Create(FunctionType::get(Type::getVoidTy(C), {Type::getInt32Ty(C)}, false),
                          Function::ExternalLinkage, "switchMode", M);
  }
  Value *Rd = CreateLoadFromReg(Inst.getOperand(0), Main, C, IB);
  IB.CreateCall(F, {Rd});
  Value *Addr = IB.CreateAnd(Rd, ConstantInt::get(C, APInt(32, ~1U)));
  return IB.CreateRet(Addr);
}

Instruction *AsmTranslator::translatetPUSH(LLVMContext &C, Module *M, Function *Main, IRBuilder<> &IB, MCInst &Inst)
{
  // push {<register list>}
  // stack pointer
  Function *F;
  if (!(F = M->getFunction("StoreToMemory32"))) {
    F = Function::Create(FunctionType::get(Type::getVoidTy(C), {Type::getInt32Ty(C), Type::getInt32Ty(C)}, false),
                            Function::ExternalLinkage, "StoreToMemory32", *M);
  }

  Value *StackBase = CreateLoadFromReg(ARM::SP, Main, C, IB);
  int Offset = 0;
  for (auto I = Inst.begin() + 2, E = Inst.end(); I != E; ++I) {
    Value *V = CreateLoadFromReg(*I, Main, C, IB);
    // push onto stack
    Value *Addr = IB.CreateAdd(StackBase, ConstantInt::get(C, APInt(32, Offset)));
    IB.CreateCall(F, {Addr, V});
    Offset += 4;
  }
  Value *NewStackPointer = IB.CreateAdd(StackBase, ConstantInt::get(C, APInt(32, Offset)));
  return CreateStoreToReg(NewStackPointer, ARM::SP, Main, C, IB);
}


int getRegIdx(int Reg, int PrivM)
{

#define HANDLE_REG(ARM_NAME, NAME) \
    case ARM::ARM_NAME:    \
      return RegIdxTable[CPU::Register::NAME][PrivM];

#define HANDLE_GPR(NAME) HANDLE_REG(NAME, NAME)

    switch(Reg) {
HANDLE_GPR(R0)
HANDLE_GPR(R1)
HANDLE_GPR(R2)
HANDLE_GPR(R3)
HANDLE_GPR(R4)
HANDLE_GPR(R5)
HANDLE_GPR(R6)
HANDLE_GPR(R7)
HANDLE_GPR(R8)
HANDLE_GPR(R9)
HANDLE_GPR(R10)
HANDLE_GPR(R11)
HANDLE_GPR(R12)
HANDLE_REG(SP, R13)
HANDLE_REG(LR, R14)
    case ARM::PC:
      return -1;

    default:
      errs() << "Unknown register: " << Reg << '\n';
      exit(-1);
    }
}

Value *AsmTranslator::CreateLoadFromReg(MCOperand &Op, Function *Main, LLVMContext &C, IRBuilder<> &IB) {
  return CreateLoadFromReg(Op.getReg(), Main, C, IB);
}

Value *AsmTranslator::CreateLoadFromReg(int Reg, Function *Main, LLVMContext &C, IRBuilder<> &IB) {
  int Idx;
  if ((Idx = getRegIdx(Reg, Cpu->getPrivMode())) == -1) {
    // program counter
    return ConstantInt::get(C, APInt(32, Cpu->getEXProgCounter()));
  }
  Value *BasePtr = Main->getArg(0);
  Value *Ptr = IB.CreateGEP(BasePtr->getType()->getPointerElementType(), BasePtr, {ConstantInt::get(C, APInt(32, Idx))});
  return IB.CreateLoad(Ptr->getType()->getPointerElementType(), Ptr);
}

Instruction *AsmTranslator::CreateStoreToReg(Value *V, MCOperand &Op, Function *Main, LLVMContext &C, IRBuilder<> &IB) {
  return CreateStoreToReg(V, Op.getReg(), Main, C, IB);
}

Instruction *AsmTranslator::CreateStoreToReg(Value *V, int Reg, Function *Main, LLVMContext &C, IRBuilder<> &IB) {
  int Idx;
  if ((Idx = getRegIdx(Reg, Cpu->getPrivMode())) == -1) {
    errs() << "Trying to write program counter\n";
    exit(-1);
  }
  Value *BasePtr = Main->getArg(0);
  Value *Ptr = IB.CreateGEP(BasePtr->getType()->getPointerElementType(), BasePtr, {ConstantInt::get(C, APInt(32, Idx))});
  return IB.CreateStore(V, Ptr);
}

Value *AsmTranslator::CreateLoadFromMemory(Type *Ty, Value *Addr, Module *M, LLVMContext &C, IRBuilder<> &IB)
{
  Value *Base = ConstantInt::get(Type::getInt64Ty(C), APInt(64, (uint64_t)Cpu->Mem->getPointer(0)));
  Value *Offset = IB.CreateZExt(Addr, Type::getInt64Ty(C));
  Value *Ptr = IB.CreateIntToPtr(IB.CreateAdd(Base, Offset), Ty);
  return IB.CreateLoad(Ty->getPointerElementType(), Ptr);
}
