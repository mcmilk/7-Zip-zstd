// CabCopyDecoder.cpp

#include "StdAfx.h"

#include "CabCopyDecoder.h"
#include "Common/Defs.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {

static const UInt32 kBufferSize = 1 << 17;

void CCopyDecoder::ReleaseStreams()
{
  m_InStream.ReleaseStream();
  m_OutStream.ReleaseStream();
}

class CCopyDecoderFlusher
{
  CCopyDecoder *m_Decoder;
public:
  CCopyDecoderFlusher(CCopyDecoder *decoder): m_Decoder(decoder) {}
  ~CCopyDecoderFlusher()
  {
    m_Decoder->Flush();
    m_Decoder->ReleaseStreams();
  }
};

STDMETHODIMP CCopyDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UInt64 *inSize, const UInt64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UInt64 size = *outSize;

  if (!m_OutStream.Create(1 << 20))
    return E_OUTOFMEMORY;
  if (!m_InStream.Create(1 << 20))
    return E_OUTOFMEMORY;

  m_InStream.SetStream(inStream);
  m_InStream.Init(m_ReservedSize, m_NumInDataBlocks);
  m_OutStream.SetStream(outStream);
  m_OutStream.Init();
  CCopyDecoderFlusher decoderFlusher(this);

  UInt64 nowPos64 = 0;
  while(nowPos64 < size)
  {
    UInt32 blockSize;
    bool dataAreCorrect;
    RINOK(m_InStream.ReadBlock(blockSize, dataAreCorrect));
    if (!dataAreCorrect)
    {
      throw 123456;
    }
    for (UInt32 i = 0; i < blockSize; i++)
      m_OutStream.WriteByte(m_InStream.ReadByte());
    nowPos64 += blockSize;
    if (progress != NULL)
    {
      RINOK(progress->SetRatioInfo(&nowPos64, &nowPos64));
    }
  }
  return S_OK;
}

}}
