// Archive/Cab/DataInByte.cpp

#include "stdafx.h"

#include "Archive/Cab/DataInByte.h"
#include "Common/Types.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {

#pragma pack(push, PragmaCabDataHeader)
#pragma pack(push, 1)

struct CDataBlockHeader
{
  UINT32  CheckSum;	/* checksum of this CFDATA entry */
  UINT16  PackSize;	/* number of compressed bytes in this block */
  UINT16  UnPackSize;	/* number of uncompressed bytes in this block */
  // BYTE  abReserve[];	/* (optional) per-datablock reserved area */
  // BYTE  ab[cbData];	/* compressed data bytes */
};

#pragma pack(pop)
#pragma pack(pop, PragmaCabDataHeader)

CInByte::CInByte(UINT32 aBufferSize):
  m_BufferSize(aBufferSize),
  m_Stream(0)
{
  m_Buffer = new BYTE[m_BufferSize];
}

CInByte::~CInByte()
{
  delete []m_Buffer;
  ReleaseStream();
}

void CInByte::Init(ISequentialInStream *aStream, BYTE aReservedSize, UINT32 aNumBlocks)
{
  m_ReservedSize = aReservedSize;
  m_NumBlocks = aNumBlocks;
  m_CurrentBlockIndex = 0;
  ReleaseStream();
  m_Stream = aStream;
  m_Stream->AddRef();
  m_ProcessedSize = 0;
  m_Pos = 0;
  m_NumReadBytesInBuffer = 0;
}

void CInByte::ReleaseStream()
{
  if(m_Stream != 0)
  {
    m_Stream->Release();
    m_Stream = 0;
  }
}

class CCheckSum
{
  UINT32 m_Value;
public:
  CCheckSum():  m_Value(0){};
  void Init() { m_Value = 0; }
  void Update(const void *aData, UINT32 aSize);
  UINT32 GetResult() const { return m_Value; } 
};

void CCheckSum::Update(const void *aData, UINT32 aSize)
{
  UINT32 aCheckSum = m_Value;
  const BYTE *aDataPointer = (const BYTE  *)aData;
  int aNumUINT32Words = aSize / 4;                    // Number of ULONGs
  
  // aDataPointer += aNumUINT32Words * 4;
  
  UINT32 aTemp;
  const UINT32 *aDataPointer32 = (const UINT32 *)aData;
  while (aNumUINT32Words-- > 0) 
  {
    // aTemp ^= *aDataPointer32++;              
    aTemp = *aDataPointer++;                     // Get low-order byte
    aTemp |= (((ULONG)(*aDataPointer++)) <<  8); // Add 2nd byte
    aTemp |= (((ULONG)(*aDataPointer++)) << 16); // Add 3nd byte
    aTemp |= (((ULONG)(*aDataPointer++)) << 24); // Add 4th byte
    aCheckSum ^= aTemp;                     // Update checksum
  }
  
  aTemp = 0;
  switch (aSize % 4) 
  {
  case 3:
    aTemp |= (((ULONG)(*aDataPointer++)) << 16); // Add 3nd byte
  case 2:
    aTemp |= (((ULONG)(*aDataPointer++)) <<  8); // Add 2nd byte
  case 1:
    aTemp |= *aDataPointer++;                    // Get low-order byte
  default:
    break;
  }
  aCheckSum ^= aTemp;       
  m_Value = aCheckSum;
}
HRESULT CInByte::ReadBlock(UINT32 &anUncompressedSize, bool &aDataAreCorrect)
{
  if (m_CurrentBlockIndex >= m_NumBlocks)
    throw "there is no more data blocks";

  m_ProcessedSize += m_NumReadBytesInBuffer;

  CDataBlockHeader aDataBlockHeader;
  UINT32 aNumProcessedBytes;
  RETURN_IF_NOT_S_OK(m_Stream->Read(&aDataBlockHeader, sizeof(aDataBlockHeader), &aNumProcessedBytes));
  if (aNumProcessedBytes != sizeof(aDataBlockHeader))
    throw "bad block";
  
  if (m_ReservedSize != 0)
  {
    BYTE aReservedArea[256];
    RETURN_IF_NOT_S_OK(m_Stream->Read(aReservedArea, m_ReservedSize, &aNumProcessedBytes));
    if (aNumProcessedBytes != m_ReservedSize)
      throw "bad block";
  }

  RETURN_IF_NOT_S_OK(m_Stream->Read(m_Buffer, aDataBlockHeader.PackSize, &m_NumReadBytesInBuffer));
  if (m_NumReadBytesInBuffer != aDataBlockHeader.PackSize)
    throw "bad block";

  if (aDataBlockHeader.CheckSum == 0)
    aDataAreCorrect = true;
  {
    CCheckSum aCheckSum;
    aCheckSum.Update(m_Buffer, aDataBlockHeader.PackSize);
    aCheckSum.Update(&aDataBlockHeader.PackSize, 
        sizeof(aDataBlockHeader) - sizeof(aDataBlockHeader.CheckSum));
    aDataAreCorrect = (aCheckSum.GetResult() == aDataBlockHeader.CheckSum);
  }

  m_Pos = 0;
  anUncompressedSize = aDataBlockHeader.UnPackSize;

  m_CurrentBlockIndex++;
  return S_OK;
}


}}
