// CoderMixer2ST.h

#ifndef __CODER_MIXER2_ST_H
#define __CODER_MIXER2_ST_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"

#include "CoderMixer2.h"

class CSequentialInStreamCalcSize:
  public ISequentialInStream,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(ISequentialInStream)

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
private:
  CMyComPtr<ISequentialInStream> _stream;
  UInt64 _size;
  bool _wasFinished;
public:
  void SetStream(ISequentialInStream *stream) { _stream = stream;  }
  void Init()
  {
    _size = 0;
    _wasFinished = false;
  }
  void ReleaseStream() { _stream.Release(); }
  UInt64 GetSize() const { return _size; }
  bool WasFinished() const { return _wasFinished; }
};


class COutStreamCalcSize:
  public ISequentialOutStream,
  public IOutStreamFlush,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
public:
  MY_UNKNOWN_IMP2(ISequentialOutStream, IOutStreamFlush)
  
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Flush)();
  
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init() { _size = 0; }
  UInt64 GetSize() const { return _size; }
};



namespace NCoderMixer2 {

struct CCoderST: public CCoder
{
  bool CanRead;
  bool CanWrite;
  
  CCoderST(): CanRead(false), CanWrite(false) {}
};


struct CStBinderStream
{
  CSequentialInStreamCalcSize *InStreamSpec;
  COutStreamCalcSize *OutStreamSpec;
  CMyComPtr<IUnknown> StreamRef;

  CStBinderStream(): InStreamSpec(NULL), OutStreamSpec(NULL) {}
};


class CMixerST:
  public IUnknown,
  public CMixer,
  public CMyUnknownImp
{
  HRESULT GetInStream2(ISequentialInStream * const *inStreams, /* const UInt64 * const *inSizes, */
      UInt32 outStreamIndex, ISequentialInStream **inStreamRes);
  HRESULT GetInStream(ISequentialInStream * const *inStreams, /* const UInt64 * const *inSizes, */
      UInt32 inStreamIndex, ISequentialInStream **inStreamRes);
  HRESULT GetOutStream(ISequentialOutStream * const *outStreams, /* const UInt64 * const *outSizes, */
      UInt32 outStreamIndex, ISequentialOutStream **outStreamRes);

  HRESULT FlushStream(UInt32 streamIndex);
  HRESULT FlushCoder(UInt32 coderIndex);

public:
  CObjectVector<CCoderST> _coders;
  
  CObjectVector<CStBinderStream> _binderStreams;

  MY_UNKNOWN_IMP

  CMixerST(bool encodeMode);
  ~CMixerST();

  virtual void AddCoder(ICompressCoder *coder, ICompressCoder2 *coder2, bool isFilter);

  virtual CCoder &GetCoder(unsigned index);

  virtual void SelectMainCoder(bool useFirst);

  virtual void ReInit();
  
  virtual void SetCoderInfo(unsigned coderIndex, const UInt64 *unpackSize, const UInt64 * const *packSizes)
    { _coders[coderIndex].SetCoderInfo(unpackSize, packSizes); }

  virtual HRESULT Code(
      ISequentialInStream * const *inStreams,
      ISequentialOutStream * const *outStreams,
      ICompressProgressInfo *progress);

  virtual UInt64 GetBondStreamSize(unsigned bondIndex) const;

  HRESULT GetMainUnpackStream(
      ISequentialInStream * const *inStreams,
      ISequentialInStream **inStreamRes);
};

}

#endif
