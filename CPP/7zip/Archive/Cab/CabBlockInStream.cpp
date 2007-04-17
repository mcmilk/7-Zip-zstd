// CabBlockInStream.cpp

#include "StdAfx.h"

extern "C" 
{ 
#include "../../../../C/Alloc.h"
}

#include "Common/Defs.h"

#include "../../Common/StreamUtils.h"

#include "CabBlockInStream.h"

namespace NArchive {
namespace NCab {

static const UInt32 kBlockSize = (1 << 16);

bool CCabBlockInStream::Create()
{
  if (!_buffer)
    _buffer = (Byte *)::MyAlloc(kBlockSize);
  return (_buffer != 0);
}

CCabBlockInStream::~CCabBlockInStream()
{
  MyFree(_buffer);
}

class CCheckSum2
{
  UInt32 m_Value;
  int m_Pos;
  Byte m_Hist[4];
public:
  CCheckSum2(): m_Value(0){};
  void Init() { m_Value = 0;  m_Pos = 0; }
  void Update(const void *data, UInt32 size);
  void FinishDataUpdate()
  {
    for (int i = 0; i < m_Pos; i++)
      m_Value ^= ((UInt32)(m_Hist[i])) << (8 * (m_Pos - i - 1));
  }
  void UpdateUInt32(UInt32 v) { m_Value ^= v; }
  UInt32 GetResult() const {  return m_Value; } 
};

void CCheckSum2::Update(const void *data, UInt32 size)
{
  UInt32 checkSum = m_Value;
  const Byte *dataPointer = (const Byte *)data;

  while (size != 0 && m_Pos != 0)
  {
    m_Hist[m_Pos] = *dataPointer++;
    m_Pos = (m_Pos + 1) & 3;
    size--;
    if (m_Pos == 0)
      for (int i = 0; i < 4; i++)
        checkSum ^= ((UInt32)m_Hist[i]) << (8 * i);
  }

  int numWords = size / 4;
  
  while (numWords-- != 0) 
  {
    UInt32 temp = *dataPointer++;
    temp |= ((UInt32)(*dataPointer++)) <<  8;
    temp |= ((UInt32)(*dataPointer++)) << 16;
    temp |= ((UInt32)(*dataPointer++)) << 24;
    checkSum ^= temp;
  }
  m_Value = checkSum;

  size &= 3;
  
  while (size != 0)
  {
    m_Hist[m_Pos] = *dataPointer++;
    m_Pos = (m_Pos + 1) & 3;
    size--;
  }
}

static const UInt32 kDataBlockHeaderSize = 8;

class CTempCabInBuffer2
{
public:
  Byte Buffer[kDataBlockHeaderSize];
  UInt32 Pos;
  Byte ReadByte()
  {
    return Buffer[Pos++];
  }
  UInt32 ReadUInt32()
  {
    UInt32 value = 0;
    for (int i = 0; i < 4; i++)
      value |= (((UInt32)ReadByte()) << (8 * i));
    return value;
  }
  UInt16 ReadUInt16()
  {
    UInt16 value = 0;
    for (int i = 0; i < 2; i++)
      value |= (((UInt16)ReadByte()) << (8 * i));
    return value;
  }
};

HRESULT CCabBlockInStream::PreRead(UInt32 &packSize, UInt32 &unpackSize)
{
  CTempCabInBuffer2 inBuffer;
  inBuffer.Pos = 0;
  UInt32 processedSizeLoc; 
  RINOK(ReadStream(_stream, inBuffer.Buffer, kDataBlockHeaderSize, &processedSizeLoc))
  if (processedSizeLoc != kDataBlockHeaderSize)
    return S_FALSE;  // bad block

  UInt32 checkSum = inBuffer.ReadUInt32();
  packSize = inBuffer.ReadUInt16();
  unpackSize = inBuffer.ReadUInt16();
  if (ReservedSize != 0)
  {
    RINOK(ReadStream(_stream, _buffer, ReservedSize, &processedSizeLoc));
    if(ReservedSize != processedSizeLoc)
      return S_FALSE; // bad block;
  }
  _pos = 0;
  CCheckSum2 checkSumCalc;
  checkSumCalc.Init();
  UInt32 packSize2 = packSize;
  if (MsZip && _size == 0)
  {
    if (packSize < 2)
      return S_FALSE; // bad block;
    Byte sig[2];
    RINOK(ReadStream(_stream, sig, 2, &processedSizeLoc));
    if(processedSizeLoc != 2)
      return S_FALSE;
    if (sig[0] != 0x43 || sig[1] != 0x4B)
      return S_FALSE;
    packSize2 -= 2;
    checkSumCalc.Update(sig, 2);
  }

  if (kBlockSize - _size < packSize2)
    return S_FALSE;

  UInt32 curSize = packSize2;
  if (curSize != 0)
  {
    RINOK(ReadStream(_stream, _buffer + _size, curSize, &processedSizeLoc));
    checkSumCalc.Update(_buffer + _size, processedSizeLoc);
    _size += processedSizeLoc;
    if (processedSizeLoc != curSize)
      return S_FALSE;
  }
  TotalPackSize = _size;

  checkSumCalc.FinishDataUpdate();
  
  bool dataError;
  if (checkSum == 0)
    dataError = false;
  else
  {
    checkSumCalc.UpdateUInt32(packSize | (((UInt32)unpackSize) << 16));
    dataError = (checkSumCalc.GetResult() != checkSum);
  }
  DataError |= dataError;
  return dataError ? S_FALSE : S_OK;
}

STDMETHODIMP CCabBlockInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize != 0)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  if (_size != 0)
  {
    size = MyMin(_size, size);
    memmove(data, _buffer + _pos, size);
    _pos += size;
    _size -= size;
    if (processedSize != 0)
      *processedSize = size;
    return S_OK;
  }
  return S_OK; // no blocks data
}

}}
