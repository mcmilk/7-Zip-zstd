// Archive/Cab/MSZipDecoder.h

#pragma once

#ifndef __ARCHIVE_CAB_DECODER_H
#define __ARCHIVE_CAB_DECODER_H

#include "Interface/ICoder.h"

#include "Stream/WindowOut.h"
#include "Stream/LSBFDecoder.h"

#include "Archive/Cab/DataInByte.h"

#include "Compression/HuffmanDecoder.h"

#include "DeflateExtConst.h"

namespace NArchive {
namespace NCab {
namespace NMSZip {

class CDecoderException
{
public:
  enum ECauseType
  {
    kData
  } m_Cause;
  CDecoderException(ECauseType aCause): m_Cause(aCause) {}
};

class CMSZipBitDecoder: public NStream::NLSBF::CDecoder<NCab::CInByte>
{
public:
  void InitMain(ISequentialInStream *aStream, BYTE aReservedSize, UINT32 aNumBlocks)
  {
    m_Stream.Init(aStream, aReservedSize, aNumBlocks);
    Init();
  }
  HRESULT ReadBlock(UINT32 &anUncompressedSize, bool &aDataAreCorrect)
  {
    return m_Stream.ReadBlock(anUncompressedSize, aDataAreCorrect);
  }
};

typedef NCompression::NHuffman::CDecoder<NDeflate::kNumHuffmanBits> CHuffmanDecoder;

class CDecoder :
  public ICompressCoder,
  public CComObjectRoot
{
  NStream::NWindow::COut m_OutWindowStream;
  CMSZipBitDecoder m_InBitStream;
  CHuffmanDecoder m_MainDecoder;
  CHuffmanDecoder m_DistDecoder;
  CHuffmanDecoder m_LevelDecoder; // table for decoding other tables;

  bool m_FinalBlock;
  bool m_StoredMode;
  UINT32 m_StoredBlockSize;

  BYTE m_ReservedSize;
  UINT32 m_NumInDataBlocks;

  void DeCodeLevelTable(BYTE *aNewLevels, int aNumLevels);
  void ReadTables();
public:
  CDecoder();

  BEGIN_COM_MAP(CDecoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CDecoder)

  DECLARE_NO_REGISTRY()

  HRESULT Flush();
  void (ReleaseStreams)();

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  void SetParams(BYTE aReservedSize, UINT32 aNumInDataBlocks) 
  { 
    m_ReservedSize = aReservedSize;
    m_NumInDataBlocks = aNumInDataBlocks;
  }

};

}}}

#endif