// Archive/CabInBuffer.cpp

#include "stdafx.h"

#include "../../../Common/Alloc.h"
#include "../../../Common/MyCom.h"
#include "../../../Windows/Defs.h"
#include "CabInBuffer.h"

namespace NArchive {
namespace NCab {

static const UInt32 kDataBlockHeaderSize = 8;
/*
struct CDataBlockHeader
{
  UInt32  CheckSum;	// checksum of this CFDATA entry
  UInt16  PackSize;	// number of compressed bytes in this block
  UInt16  UnPackSize;	// number of uncompressed bytes in this block
  // Byte  abReserve[];	// (optional) per-datablock reserved area
  // Byte  ab[cbData];	// compressed data bytes
};
*/

class CTempCabInBuffer
{
public:
  Byte *Buffer;
  UInt32 Size;
  UInt32 Pos;
  Byte ReadByte()
  {
    if (Pos >= Size)
      throw "overflow";
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

bool CInBuffer::Create(UInt32 bufferSize)
{
  const UInt32 kMinBlockSize = 1;
  if (bufferSize < kMinBlockSize)
    bufferSize = kMinBlockSize;
  if (m_Buffer != 0 && m_BufferSize == bufferSize)
    return true;
  Free();
  m_BufferSize = bufferSize;
  m_Buffer = (Byte *)::BigAlloc(bufferSize);
  return (m_Buffer != 0);
}

void CInBuffer::Free()
{
  BigFree(m_Buffer);
  m_Buffer = 0;
}

void CInBuffer::Init(Byte reservedSize, UInt32 numBlocks)
{
  m_ReservedSize = reservedSize;
  m_NumBlocks = numBlocks;
  m_CurrentBlockIndex = 0;
  m_ProcessedSize = 0;
  m_Pos = 0;
  m_NumReadBytesInBuffer = 0;
}

class CCheckSum
{
  UInt32 m_Value;
public:
  CCheckSum():  m_Value(0){};
  void Init() { m_Value = 0; }
  void Update(const void *data, UInt32 size);
  void UpdateUInt32(UInt32 v) { m_Value ^= v; }
  UInt32 GetResult() const { return m_Value; } 
};

void CCheckSum::Update(const void *data, UInt32 size)
{
  UInt32 checkSum = m_Value;
  const Byte *dataPointer = (const Byte  *)data;
  int numUINT32Words = size / 4; // Number of ULONGs
  
  UInt32 temp;
  while (numUINT32Words-- > 0) 
  {
    temp = *dataPointer++;
    temp |= (((UInt32)(*dataPointer++)) <<  8);
    temp |= (((UInt32)(*dataPointer++)) << 16);
    temp |= (((UInt32)(*dataPointer++)) << 24);
    checkSum ^= temp;
  }
  
  temp = 0;
  int rem = (size & 3);
  if (rem >= 3)
    temp |= (((UInt32)(*dataPointer++)) << 16);
  if (rem >= 2)
    temp |= (((UInt32)(*dataPointer++)) <<  8);
  if (rem >= 1)
    temp |= *dataPointer++;
  checkSum ^= temp;       
  m_Value = checkSum;
}

HRESULT CInBuffer::ReadBlock(UInt32 &uncompressedSize, bool &dataAreCorrect)
{
  if (m_CurrentBlockIndex >= m_NumBlocks)
    throw "there is no more data blocks";

  m_ProcessedSize += m_NumReadBytesInBuffer;

  Byte buffer[kDataBlockHeaderSize];
  UInt32 numProcessedBytes;
  RINOK(m_Stream->Read(buffer, kDataBlockHeaderSize, &numProcessedBytes));
  if (numProcessedBytes != kDataBlockHeaderSize)
    throw "bad block";
  
  CTempCabInBuffer inBuffer;
  inBuffer.Size = kDataBlockHeaderSize;
  inBuffer.Buffer = (Byte *)buffer;
  inBuffer.Pos = 0;

  UInt32 checkSum = inBuffer.ReadUInt32();	// checksum of this CFDATA entry
  UInt16 packSize = inBuffer.ReadUInt16();	// number of compressed bytes in this block
  UInt16 unPackSize = inBuffer.ReadUInt16();	// number of uncompressed bytes in this block
  
  if (m_ReservedSize != 0)
  {
    Byte reservedArea[256];
    RINOK(m_Stream->Read(reservedArea, m_ReservedSize, &numProcessedBytes));
    if (numProcessedBytes != m_ReservedSize)
      throw "bad block";
  }

  RINOK(m_Stream->Read(m_Buffer, packSize, &m_NumReadBytesInBuffer));
  if (m_NumReadBytesInBuffer != packSize)
    throw "bad block";

  // Now I don't remember why (checkSum == 0) check is disbaled
  // Cab specification: 
  //   checkSum: May be set to zero if the checksum is not supplied.
  // but seems it's stupid rule.
  if (checkSum == 0)
    dataAreCorrect = true;
  {
    CCheckSum checkSumCalc;
    checkSumCalc.Update(m_Buffer, packSize);
    checkSumCalc.UpdateUInt32(packSize | (((UInt32)unPackSize) << 16));
    dataAreCorrect = (checkSumCalc.GetResult() == checkSum);
  }

  m_Pos = 0;
  uncompressedSize = unPackSize;

  m_CurrentBlockIndex++;
  return S_OK;
}

}}
