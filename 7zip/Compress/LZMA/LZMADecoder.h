// LZMA/Decoder.h

// #pragma once 

#ifndef __LZMA_DECODER_H
#define __LZMA_DECODER_H

#include "../../ICoder.h"
#include "../../../Common/MyCom.h"
#include "../LZ/LZOutWindow.h"

#include "LZMA.h"
#include "LZMALen.h"
#include "LZMALiteral.h"

namespace NCompress {
namespace NLZMA {

typedef NRangeCoder::CBitDecoder<kNumMoveBits> CMyBitDecoder;

class CDecoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public CMyUnknownImp
{
  CLZOutWindow _outWindowStream;
  NRangeCoder::CDecoder _rangeDecoder;

  CMyBitDecoder _mainChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];
  CMyBitDecoder _matchChoiceDecoders[kNumStates];
  CMyBitDecoder _matchRepChoiceDecoders[kNumStates];
  CMyBitDecoder _matchRep1ChoiceDecoders[kNumStates];
  CMyBitDecoder _matchRep2ChoiceDecoders[kNumStates];
  CMyBitDecoder _matchRepShortChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];

  NRangeCoder::CBitTreeDecoder<kNumMoveBits, kNumPosSlotBits> _posSlotDecoder[kNumLenToPosStates];

  NRangeCoder::CReverseBitTreeDecoder2<kNumMoveBits> _posDecoders[kNumPosModels];
  NRangeCoder::CReverseBitTreeDecoder<kNumMoveBits, kNumAlignBits> _posAlignDecoder;
  
  NLength::CDecoder _lenDecoder;
  NLength::CDecoder _repMatchLenDecoder;

  NLiteral::CDecoder _literalDecoder;

  UINT32 _dictionarySize;
  UINT32 _dictionarySizeCheck;
  
  UINT32 _posStateMask;

public:

  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties)

  CDecoder();
  HRESULT Create();


  HRESULT Init(ISequentialInStream *inStream, 
      ISequentialOutStream *outStream);
  /*
  void ReleaseStreams()
  {
    _outWindowStream.ReleaseStream();
    _rangeDecoder.ReleaseStream();
  }
  */

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
      // _decoder->ReleaseStreams(); 
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
