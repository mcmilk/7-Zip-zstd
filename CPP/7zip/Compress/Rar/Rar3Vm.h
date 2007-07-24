// Rar3Vm.h
// According to unRAR license, this code may not be used to develop 
// a program that creates RAR archives

#ifndef __RAR3VM_H
#define __RAR3VM_H

#include "Common/Types.h"
#include "Common/MyVector.h"

#include "../../../../C/CpuArch.h"

#define RARVM_STANDARD_FILTERS
#ifdef LITTLE_ENDIAN_UNALIGN
#define RARVM_LITTLE_ENDIAN_UNALIGN
#endif

namespace NCompress {
namespace NRar3 {

class CMemBitDecoder
{
  const Byte *_data;
  UInt32 _bitSize;
  UInt32 _bitPos;
public:
  void Init(const Byte *data, UInt32 byteSize)
  {
    _data = data;
    _bitSize = (byteSize << 3);
    _bitPos = 0;
  }
  UInt32 ReadBits(int numBits);
  UInt32 ReadBit();
  bool Avail() const { return (_bitPos < _bitSize); }
};

namespace NVm {

inline UInt32 GetValue32(const void *addr)
{
  #ifdef RARVM_LITTLE_ENDIAN_UNALIGN
  return *(const UInt32 *)addr;
  #else
  const Byte *b = (const Byte *)addr;
  return UInt32((UInt32)b[0]|((UInt32)b[1]<<8)|((UInt32)b[2]<<16)|((UInt32)b[3]<<24));
  #endif
}

inline void SetValue32(void *addr, UInt32 value)
{
  #ifdef RARVM_LITTLE_ENDIAN_UNALIGN
  *(UInt32 *)addr = value;
  #else
  ((Byte *)addr)[0] = (Byte)value;
  ((Byte *)addr)[1] = (Byte)(value >> 8);
  ((Byte *)addr)[2] = (Byte)(value >> 16);
  ((Byte *)addr)[3] = (Byte)(value >> 24);
  #endif
}

UInt32 ReadEncodedUInt32(CMemBitDecoder &inp);

const int kNumRegBits = 3;
const UInt32 kNumRegs = 1 << kNumRegBits;
const UInt32 kNumGpRegs = kNumRegs - 1;

const UInt32 kSpaceSize = 0x40000;
const UInt32 kSpaceMask = kSpaceSize -1;
const UInt32 kGlobalOffset = 0x3C000;
const UInt32 kGlobalSize = 0x2000;
const UInt32 kFixedGlobalSize = 64;

namespace NGlobalOffset
{
  const UInt32 kBlockSize = 0x1C;
  const UInt32 kBlockPos  = 0x20;
  const UInt32 kExecCount = 0x2C;
  const UInt32 kGlobalMemOutSize = 0x30;
}

enum ECommand
{
  CMD_MOV,  CMD_CMP,  CMD_ADD,  CMD_SUB,  CMD_JZ,   CMD_JNZ,  CMD_INC,  CMD_DEC,
  CMD_JMP,  CMD_XOR,  CMD_AND,  CMD_OR,   CMD_TEST, CMD_JS,   CMD_JNS,  CMD_JB,
  CMD_JBE,  CMD_JA,   CMD_JAE,  CMD_PUSH, CMD_POP,  CMD_CALL, CMD_RET,  CMD_NOT,
  CMD_SHL,  CMD_SHR,  CMD_SAR,  CMD_NEG,  CMD_PUSHA,CMD_POPA, CMD_PUSHF,CMD_POPF,
  CMD_MOVZX,CMD_MOVSX,CMD_XCHG, CMD_MUL,  CMD_DIV,  CMD_ADC,  CMD_SBB,  CMD_PRINT,

  CMD_MOVB, CMD_CMPB, CMD_ADDB, CMD_SUBB, CMD_INCB, CMD_DECB, 
  CMD_XORB, CMD_ANDB, CMD_ORB,  CMD_TESTB,CMD_NEGB,
  CMD_SHLB, CMD_SHRB, CMD_SARB, CMD_MULB
};

enum EOpType {OP_TYPE_REG, OP_TYPE_INT, OP_TYPE_REGMEM, OP_TYPE_NONE};

// Addr in COperand object can link (point) to CVm object!!!

struct COperand
{
  EOpType Type;
  UInt32 Data;
  UInt32 Base;
  COperand(): Type(OP_TYPE_NONE), Data(0), Base(0) {}
};

struct CCommand
{
  ECommand OpCode;
  bool ByteMode;
  COperand Op1, Op2;
};

struct CBlockRef
{
  UInt32 Offset;
  UInt32 Size;
};

struct CProgram
{
  CRecordVector<CCommand> Commands;
  #ifdef RARVM_STANDARD_FILTERS
  int StandardFilterIndex;
  #endif
  CRecordVector<Byte> StaticData;
};

struct CProgramInitState
{
  UInt32 InitR[kNumGpRegs];
  CRecordVector<Byte> GlobalData;

  void AllocateEmptyFixedGlobal()
  {
    GlobalData.Clear();
    GlobalData.Reserve(NVm::kFixedGlobalSize);
    for (UInt32 i = 0; i < NVm::kFixedGlobalSize; i++)
      GlobalData.Add(0);
  }
};

class CVm
{
  static UInt32 GetValue(bool byteMode, const void *addr)
  {
    if (byteMode)
      return(*(const Byte *)addr);
    else
    {
      #ifdef RARVM_LITTLE_ENDIAN_UNALIGN
      return *(const UInt32 *)addr;
      #else
      const Byte *b = (const Byte *)addr;
      return UInt32((UInt32)b[0]|((UInt32)b[1]<<8)|((UInt32)b[2]<<16)|((UInt32)b[3]<<24));
      #endif
    }
  }

  static void SetValue(bool byteMode, void *addr, UInt32 value)
  {
    if (byteMode)
      *(Byte *)addr = (Byte)value;
    else
    {
      #ifdef RARVM_LITTLE_ENDIAN_UNALIGN
      *(UInt32 *)addr = value;
      #else
      ((Byte *)addr)[0] = (Byte)value;
      ((Byte *)addr)[1] = (Byte)(value >> 8);
      ((Byte *)addr)[2] = (Byte)(value >> 16);
      ((Byte *)addr)[3] = (Byte)(value >> 24);
      #endif
    }
  }

  UInt32 GetFixedGlobalValue32(UInt32 globalOffset) { return GetValue(false, &Mem[kGlobalOffset + globalOffset]); }

  void SetBlockSize(UInt32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockSize], v); }
  void SetBlockPos(UInt32 v) { SetValue(&Mem[kGlobalOffset + NGlobalOffset::kBlockPos], v); }
public:
  static void SetValue(void *addr, UInt32 value) { SetValue(false, addr, value); }
private:
  UInt32 GetOperand32(const COperand *op) const;
  void SetOperand32(const COperand *op, UInt32 val);
  Byte GetOperand8(const COperand *op) const;
  void SetOperand8(const COperand *op, Byte val);
  UInt32 GetOperand(bool byteMode, const COperand *op) const;
  void SetOperand(bool byteMode, const COperand *op, UInt32 val);

  void DecodeArg(CMemBitDecoder &inp, COperand &op, bool byteMode);
  
  bool ExecuteCode(const CProgram *prg);
  
  #ifdef RARVM_STANDARD_FILTERS
  void ExecuteStandardFilter(int filterIndex);
  #endif
  
  Byte *Mem;
  UInt32 R[kNumRegs + 1]; // R[kNumRegs] = 0 always (speed optimization)
  UInt32 Flags;
  void ReadVmProgram(const Byte *code, UInt32 codeSize, CProgram *prg);
public:
  CVm();
  ~CVm();
  bool Create();
  void PrepareProgram(const Byte *code, UInt32 codeSize, CProgram *prg);
  void SetMemory(UInt32 pos, const Byte *data, UInt32 dataSize);
  bool Execute(CProgram *prg, const CProgramInitState *initState, 
      CBlockRef &outBlockRef, CRecordVector<Byte> &outGlobalData);
  const Byte *GetDataPointer(UInt32 offset) const { return Mem + offset; }

};

#endif

}}}
