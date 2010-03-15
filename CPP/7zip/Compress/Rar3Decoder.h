// Rar3Decoder.h
// According to unRAR license, this code may not be used to develop
// a program that creates RAR archives

/* This code uses Carryless rangecoder (1999): Dmitry Subbotin : Public domain */

#ifndef __COMPRESS_RAR3_DECODER_H
#define __COMPRESS_RAR3_DECODER_H

#include "../../../C/Ppmd7.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "HuffmanDecoder.h"
#include "Rar3Vm.h"

namespace NCompress {
namespace NRar3 {

const UInt32 kWindowSize = 1 << 22;
const UInt32 kWindowMask = (kWindowSize - 1);

const UInt32 kNumReps = 4;
const UInt32 kNumLen2Symbols = 8;
const UInt32 kLenTableSize = 28;
const UInt32 kMainTableSize = 256 + 1 + 1 + 1 + kNumReps + kNumLen2Symbols + kLenTableSize;
const UInt32 kDistTableSize = 60;

const int kNumAlignBits = 4;
const UInt32 kAlignTableSize = (1 << kNumAlignBits) + 1;

const UInt32 kLevelTableSize = 20;

const UInt32 kTablesSizesSum = kMainTableSize + kDistTableSize + kAlignTableSize + kLenTableSize;

class CBitDecoder
{
  UInt32 m_Value;
  unsigned m_BitPos;
public:
  CInBuffer m_Stream;
  bool Create(UInt32 bufferSize) { return m_Stream.Create(bufferSize); }
  void SetStream(ISequentialInStream *inStream) { m_Stream.SetStream(inStream);}
  void ReleaseStream() { m_Stream.ReleaseStream();}

  void Init()
  {
    m_Stream.Init();
    m_BitPos = 0;
    m_Value = 0;
  }
  
  UInt64 GetProcessedSize() const { return m_Stream.GetProcessedSize() - (m_BitPos) / 8; }
  UInt32 GetBitPosition() const { return ((8 - m_BitPos) & 7); }
  
  UInt32 GetValue(unsigned numBits)
  {
    if (m_BitPos < numBits)
    {
      m_BitPos += 8;
      m_Value = (m_Value << 8) | m_Stream.ReadByte();
      if (m_BitPos < numBits)
      {
        m_BitPos += 8;
        m_Value = (m_Value << 8) | m_Stream.ReadByte();
      }
    }
    return m_Value >> (m_BitPos - numBits);
  }
  
  void MovePos(unsigned numBits)
  {
    m_BitPos -= numBits;
    m_Value = m_Value & ((1 << m_BitPos) - 1);
  }
  
  UInt32 ReadBits(unsigned numBits)
  {
    UInt32 res = GetValue(numBits);
    MovePos(numBits);
    return res;
  }
};

const UInt32 kTopValue = (1 << 24);
const UInt32 kBot = (1 << 15);

struct CRangeDecoder
{
  IPpmd7_RangeDec s;
  UInt32 Range;
  UInt32 Code;
  UInt32 Low;
  CBitDecoder bitDecoder;
  SRes Res;

public:
  void InitRangeCoder()
  {
    Code = 0;
    Low = 0;
    Range = 0xFFFFFFFF;
    for (int i = 0; i < 4; i++)
      Code = (Code << 8) | bitDecoder.ReadBits(8);
  }

  void Normalize()
  {
    while ((Low ^ (Low + Range)) < kTopValue ||
       Range < kBot && ((Range = (0 - Low) & (kBot - 1)), 1))
    {
      Code = (Code << 8) | bitDecoder.m_Stream.ReadByte();
      Range <<= 8;
      Low <<= 8;
    }
  }

  CRangeDecoder();
};

struct CFilter: public NVm::CProgram
{
  CRecordVector<Byte> GlobalData;
  UInt32 BlockStart;
  UInt32 BlockSize;
  UInt32 ExecCount;
  CFilter(): BlockStart(0), BlockSize(0), ExecCount(0) {}
};

struct CTempFilter: public NVm::CProgramInitState
{
  UInt32 BlockStart;
  UInt32 BlockSize;
  UInt32 ExecCount;
  bool NextWindow;
  
  UInt32 FilterIndex;
};

const int kNumHuffmanBits = 15;

class CDecoder:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  CRangeDecoder m_InBitStream;
  Byte *_window;
  UInt32 _winPos;
  UInt32 _wrPtr;
  UInt64 _lzSize;
  UInt64 _unpackSize;
  UInt64 _writtenFileSize; // if it's > _unpackSize, then _unpackSize only written
  CMyComPtr<ISequentialOutStream> _outStream;
  NHuffman::CDecoder<kNumHuffmanBits, kMainTableSize> m_MainDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kDistTableSize> m_DistDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kAlignTableSize> m_AlignDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kLenTableSize> m_LenDecoder;
  NHuffman::CDecoder<kNumHuffmanBits, kLevelTableSize> m_LevelDecoder;

  UInt32 _reps[kNumReps];
  UInt32 _lastLength;
  
  Byte m_LastLevels[kTablesSizesSum];

  Byte *_vmData;
  Byte *_vmCode;
  NVm::CVm _vm;
  CRecordVector<CFilter *> _filters;
  CRecordVector<CTempFilter *>  _tempFilters;
  UInt32 _lastFilter;

  bool m_IsSolid;

  bool _lzMode;

  UInt32 PrevAlignBits;
  UInt32 PrevAlignCount;

  bool TablesRead;

  CPpmd7 _ppmd;
  int PpmEscChar;
  bool PpmError;
  
  HRESULT WriteDataToStream(const Byte *data, UInt32 size);
  HRESULT WriteData(const Byte *data, UInt32 size);
  HRESULT WriteArea(UInt32 startPtr, UInt32 endPtr);
  void ExecuteFilter(int tempFilterIndex, NVm::CBlockRef &outBlockRef);
  HRESULT WriteBuf();

  void InitFilters();
  bool AddVmCode(UInt32 firstByte, UInt32 codeSize);
  bool ReadVmCodeLZ();
  bool ReadVmCodePPM();
  
  UInt32 ReadBits(int numBits);

  HRESULT InitPPM();
  int DecodePpmSymbol();
  HRESULT DecodePPM(Int32 num, bool &keepDecompressing);

  HRESULT ReadTables(bool &keepDecompressing);
  HRESULT ReadEndOfBlock(bool &keepDecompressing);
  HRESULT DecodeLZ(bool &keepDecompressing);
  HRESULT CodeReal(ICompressProgressInfo *progress);
public:
  CDecoder();
  ~CDecoder();

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  void ReleaseStreams()
  {
    _outStream.Release();
    m_InBitStream.bitDecoder.ReleaseStream();
  }

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);

  void CopyBlock(UInt32 distance, UInt32 len)
  {
    _lzSize += len;
    UInt32 pos = (_winPos - distance - 1) & kWindowMask;
    Byte *window = _window;
    UInt32 winPos = _winPos;
    if (kWindowSize - winPos > len && kWindowSize - pos > len)
    {
      const Byte *src = window + pos;
      Byte *dest = window + winPos;
      _winPos += len;
      do
        *dest++ = *src++;
      while(--len != 0);
      return;
    }
    do
    {
      window[winPos] = window[pos];
      winPos = (winPos + 1) & kWindowMask;
      pos = (pos + 1) & kWindowMask;
    }
    while(--len != 0);
    _winPos = winPos;
  }
  
  void PutByte(Byte b)
  {
    _window[_winPos] = b;
    _winPos = (_winPos + 1) & kWindowMask;
    _lzSize++;
  }


};

}}

#endif
