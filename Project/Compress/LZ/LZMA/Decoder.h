// LZArichmetic/Decoder.h

#pragma once 

#ifndef __LZARITHMETIC_DECODER_H
#define __LZARITHMETIC_DECODER_H

#include "../../Interface/CompressInterface.h"

#include "Stream/WindowOut.h"

#include "LZMA.h"
#include "LenCoder.h"
#include "LiteralCoder.h"


// {23170F69-40C1-278B-0301-010000000000}
DEFINE_GUID(CLSID_CLZMADecoder, 
0x23170F69, 0x40C1, 0x278B, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompress {
namespace NLZMA {

typedef CMyBitDecoder<kNumMoveBitsForMainChoice> CMyBitDecoder2;

class CDecoder : 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CLZMADecoder>
{
  NStream::NWindow::COut _outWindowStream;
  CMyRangeDecoder _rangeDecoder;

  CMyBitDecoder2 _mainChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];
  CMyBitDecoder2 _matchChoiceDecoders[kNumStates];
  CMyBitDecoder2 _matchRepChoiceDecoders[kNumStates];
  CMyBitDecoder2 _matchRep1ChoiceDecoders[kNumStates];
  CMyBitDecoder2 _matchRep2ChoiceDecoders[kNumStates];
  CMyBitDecoder2 _matchRepShortChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];

  CBitTreeDecoder<kNumMoveBitsForPosSlotCoder, kNumPosSlotBits> _posSlotDecoder[kNumLenToPosStates];

  CReverseBitTreeDecoder2<kNumMoveBitsForPosCoders> _posDecoders[kNumPosModels];
  CReverseBitTreeDecoder<kNumMoveBitsForAlignCoders, kNumAlignBits> _posAlignDecoder;
  
  NLength::CDecoder _lenDecoder;
  NLength::CDecoder _repMatchLenDecoder;

  NLiteral::CDecoder _literalDecoder;

  UINT32 _dictionarySize;
  UINT32 _dictionarySizeCheck;
  
  UINT32 _posStateMask;

public:

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

DECLARE_REGISTRY(CDecoder, 
    // TEXT("Compress.LZMADecoder.1"), TEXT("Compress.LZMADecoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)
//DECLARE_NO_REGISTRY()

  CDecoder();
  HRESULT Create();


  HRESULT Init(ISequentialInStream *inStream, 
      ISequentialOutStream *outStream);
  void ReleaseStreams()
  {
    _outWindowStream.ReleaseStream();
    _rangeDecoder.ReleaseStream();
  }

  class CDecoderFlusher
  {
    CDecoder *_decoder;
  public:
    bool NeedFlush;
    CDecoderFlusher(CDecoder *decoder): 
          _decoder(decoder), NeedFlush(true) {}
    ~CDecoderFlusher() 
    { 
      if (NeedFlush)
        _decoder->Flush();
      _decoder->ReleaseStreams(); 
    }
  };

  HRESULT Flush()
    {  return _outWindowStream.Flush(); }  

  // ICompressCoder interface
  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);
  
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

  HRESULT SetDictionarySize(UINT32 dictionarySize);
  HRESULT SetLiteralProperties(UINT32 numLiteralPosStateBits, 
      UINT32 numLiteralContextBits);
  HRESULT SetPosBitsProperties(UINT32 numPosStateBits);
};

}}

#endif