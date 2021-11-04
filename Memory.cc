#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "Memory.h"
#include <fstream>

using namespace llvm;

ArrayRef<uint8_t> Memory::getMemDataRef(uint32_t Start, uint32_t Len)
{
  assert(Start + Len < MemorySize());
  return {getPointer(Start), Len};
}

void Memory::setRom(std::string &RomName)
{
  auto Buffer = MemoryBuffer::getFile(RomName);
  if (Buffer.getError()) {
    errs() << "ROM read failed\n";
    exit(-1);
  }
  const char *I = (*Buffer)->getBufferStart();
  const char *End   = (*Buffer)->getBufferEnd();
  for(int j = 0x8000000; I != End; ++I, j++) {
    InternalMem[j] = (uint8_t)*I;
  }
}
