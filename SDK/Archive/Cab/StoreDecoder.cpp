// Archive/Cab/StoreDecoder.cpp

#include "StdAfx.h"

#include "Archive/Cab/StoreDecoder.h"
#include "Common/Defs.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {

static const UINT32 kBufferSize = 1 << 17;

CStoreDecoder::CStoreDecoder()
{
}

CStoreDecoder::~CStoreDecoder()
{
}

void CStoreDecoder::ReleaseStreams()
{
  m_InStream.ReleaseStream();
  m_OutStream.ReleaseStream();
}

STDMETHODIMP CStoreDecoder::Flush()
{
  return m_OutStream.Flush();
}

class CStoreDecoderFlusher
{
  CStoreDecoder *m_Decoder;
public:
  CStoreDecoderFlusher(CStoreDecoder *aDecoder): m_Decoder(aDecoder) {}
  ~CStoreDecoderFlusher()
  {
    m_Decoder->Flush();
    m_Decoder->ReleaseStreams();
  }
};

STDMETHODIMP CStoreDecoder::Code(ISequentialInStream *anInStream,
    ISequentialOutStream *anOutStream, 
    const UINT64 *anInSize, const UINT64 *anOutSize,
    ICompressProgressInfo *aProgress)
{
  if (anOutSize == NULL)
    return E_INVALIDARG;
  UINT64 aSize = *anOutSize;

  m_InStream.Init(anInStream, m_ReservedSize, m_NumInDataBlocks);
  m_OutStream.Init(anOutStream);
  CStoreDecoderFlusher aDecoderFlusher(this);

  UINT64 aNowPos64 = 0;
  while(aNowPos64 < aSize)
  {
    UINT32 aBlockSize;
    bool aDataAreCorrect;
    RETURN_IF_NOT_S_OK(m_InStream.ReadBlock(aBlockSize, aDataAreCorrect));
    if (!aDataAreCorrect)
    {
      throw 123456;
    }
    for (UINT32 i = 0; i < aBlockSize; i++)
      m_OutStream.WriteByte(m_InStream.ReadByte());
    aNowPos64 += aBlockSize;
    if (aProgress != NULL)
    {
      RETURN_IF_NOT_S_OK(aProgress->SetRatioInfo(&aNowPos64, &aNowPos64));
    }
  }
  return S_OK;
}

}}
