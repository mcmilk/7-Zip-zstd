// CabCopyDecoder.cpp

#include "StdAfx.h"

#include "CabCopyDecoder.h"
#include "Common/Defs.h"
#include "Windows/Defs.h"

namespace NArchive {
namespace NCab {

static const UINT32 kBufferSize = 1 << 17;

/*
void CCopyDecoder::ReleaseStreams()
{
  m_InStream.ReleaseStream();
  m_OutStream.ReleaseStream();
}
*/
class CCopyDecoderFlusher
{
  CCopyDecoder *m_Decoder;
public:
  CCopyDecoderFlusher(CCopyDecoder *aDecoder): m_Decoder(aDecoder) {}
  ~CCopyDecoderFlusher()
  {
    m_Decoder->Flush();
    // m_Decoder->ReleaseStreams();
  }
};

STDMETHODIMP CCopyDecoder::Code(ISequentialInStream *inStream,
    ISequentialOutStream *outStream, 
    const UINT64 *inSize, const UINT64 *outSize,
    ICompressProgressInfo *progress)
{
  if (outSize == NULL)
    return E_INVALIDARG;
  UINT64 size = *outSize;

  m_InStream.Init(inStream, m_ReservedSize, m_NumInDataBlocks);
  m_OutStream.Init(outStream);
  CCopyDecoderFlusher decoderFlusher(this);

  UINT64 nowPos64 = 0;
  while(nowPos64 < size)
  {
    UINT32 blockSize;
    bool dataAreCorrect;
    RINOK(m_InStream.ReadBlock(blockSize, dataAreCorrect));
    if (!dataAreCorrect)
    {
      throw 123456;
    }
    for (UINT32 i = 0; i < blockSize; i++)
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
