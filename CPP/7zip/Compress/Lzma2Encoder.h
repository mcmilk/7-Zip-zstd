// Lzma2Encoder.h

#ifndef ZIP7_INC_LZMA2_ENCODER_H
#define ZIP7_INC_LZMA2_ENCODER_H

#include "../../../C/Lzma2Enc.h"
#include "../../../C/fast-lzma2/fast-lzma2.h"

#include "../../Common/MyCom.h"
#include "../../Common/MyBuffer.h"

#include "../ICoder.h"

namespace NCompress {
namespace NLzma2 {

Z7_CLASS_IMP_COM_4(
  CEncoder
  , ICompressCoder
  , ICompressSetCoderProperties
  , ICompressWriteCoderProperties
  , ICompressSetCoderPropertiesOpt
)
  CLzma2EncHandle _encoder;
public:
  CEncoder();
  ~CEncoder();
};

class CFastEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
  class FastLzma2
  {
  public:
    FastLzma2();
    ~FastLzma2();
    HRESULT SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
    size_t GetDictSize() const;
    HRESULT Begin();
    BYTE* GetAvailableBuffer(unsigned long& size);
    HRESULT AddByteCount(size_t count, ISequentialOutStream *outStream, ICompressProgressInfo *progress);
    HRESULT End(ISequentialOutStream *outStream, ICompressProgressInfo *progress);
    void Cancel();

  private:
    bool UpdateProgress(ICompressProgressInfo *progress);
    HRESULT WaitAndReport(size_t& res, ICompressProgressInfo *progress);
    HRESULT WriteBuffers(ISequentialOutStream *outStream);

    FL2_CStream* fcs;
    FL2_dictBuffer dict;
    size_t dict_pos;

    FastLzma2(const FastLzma2&) = delete;
    FastLzma2& operator=(const FastLzma2&) = delete;
  };

  FastLzma2 _encoder;

public:
  MY_UNKNOWN_IMP3(
    ICompressCoder,
    ICompressSetCoderProperties,
    ICompressWriteCoderProperties)

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CFastEncoder();
  virtual ~CFastEncoder();
};

}}

#endif
