// Archive/CabInBuffer.cpp

#include "stdafx.h"

#include "Common/Types.h"
#include "Common/MyCom.h"
#include "Windows/Defs.h"
#include "CabInBuffer.h"

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

CInBuffer::CInBuffer(UINT32 bufferSize):
  m_BufferSize(bufferSize),
  m_Stream(0)
{
  m_Buffer = new BYTE[m_BufferSize];
}

CInBuffer::~CInBuffer()
{
  delete []m_Buffer;
  // ReleaseStream();
}

void CInBuffer::Init(ISequentialInStream *inStream, BYTE reservedSize, UINT32 numBlocks)
{
  m_ReservedSize = reservedSize;
  m_NumBlocks = numBlocks;
  m_CurrentBlockIndex = 0;
  // ReleaseStream();
  m_Stream = inStream;
  // m_Stream->AddRef();
  m_ProcessedSize = 0;
  m_Pos = 0;
  m_NumReadBytesInBuffer = 0;
}

/*
void CInBuffer::ReleaseStream()
{
  if(m_Stream != 0)
  {
    m_Stream->Release();
    m_Stream = 0;
  }
}
*/

class CCheckSum
{
  UINT32 m_Value;
public:
  CCheckSum():  m_Value(0){};
  void Init() { m_Value = 0; }
  void Update(const void *data, UINT32 size);
  UINT32 GetResult() const { return m_Value; } 
};

void CCheckSum::Update(const void *data, UINT32 size)
{
  UINT32 checkSum = m_Value;
  const BYTE *aDataPointer = (const BYTE  *)data;
  int numUINT32Words = size / 4;                    // Number of ULONGs
  
  // dataPointer += numUINT32Words * 4;
  
  UINT32 temp;
  const UINT32 *aDataPointer32 = (const UINT32 *)data;
  while (numUINT32Words-- > 0) 
  {
    // temp ^= *aDataPointer32++;              
    temp = *aDataPointer++;                     // Get low-order byte
    temp |= (((ULONG)(*aDataPointer++)) <<  8); // Add 2nd byte
    temp |= (((ULONG)(*aDataPointer++)) << 16); // Add 3nd byte
    temp |= (((ULONG)(*aDataPointer++)) << 24); // Add 4th byte
    checkSum ^= temp;                     // Update checksum
  }
  
  temp = 0;
  switch (size % 4) 
  {
  case 3:
    temp |= (((ULONG)(*aDataPointer++)) << 16); // Add 3nd byte
  case 2:
    temp |= (((ULONG)(*aDataPointer++)) <<  8); // Add 2nd byte
  case 1:
    temp |= *aDataPointer++;                    // Get low-order byte
  default:
    break;
  }
  checkSum ^= temp;       
  m_Value = checkSum;
}
HRESULT CInBuffer::ReadBlock(UINT32 &uncompressedSize, bool &dataAreCorrect)
{
  if (m_CurrentBlockIndex >= m_NumBlocks)
    throw "there is no more data blocks";

  m_ProcessedSize += m_NumReadBytesInBuffer;

  CDataBlockHeader dataBlockHeader;
  UINT32 numProcessedBytes;
  RINOK(m_Stream->Read(&dataBlockHeader, sizeof(dataBlockHeader), &numProcessedBytes));
  if (numProcessedBytes != sizeof(dataBlockHeader))
    throw "bad block";
  
  if (m_ReservedSize != 0)
  {
    BYTE reservedArea[256];
    RINOK(m_Stream->Read(reservedArea, m_ReservedSize, &numProcessedBytes));
    if (numProcessedBytes != m_ReservedSize)
      throw "bad block";
  }

  RINOK(m_Stream->Read(m_Buffer, dataBlockHeader.PackSize, &m_NumReadBytesInBuffer));
  if (m_NumReadBytesInBuffer != dataBlockHeader.PackSize)
    throw "bad block";

  if (dataBlockHeader.CheckSum == 0)
    dataAreCorrect = true;
  {
    CCheckSum checkSum;
    checkSum.Update(m_Buffer, dataBlockHeader.PackSize);
    checkSum.Update(&dataBlockHeader.PackSize, 
        sizeof(dataBlockHeader) - sizeof(dataBlockHeader.CheckSum));
    dataAreCorrect = (checkSum.GetResult() == dataBlockHeader.CheckSum);
  }

  m_Pos = 0;
  uncompressedSize = dataBlockHeader.UnPackSize;

  m_CurrentBlockIndex++;
  return S_OK;
}


}}
