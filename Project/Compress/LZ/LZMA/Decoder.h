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
  NStream::NWindow::COut m_OutWindowStream;
  CMyRangeDecoder m_RangeDecoder;

  CMyBitDecoder2 m_MainChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];
  CMyBitDecoder2 m_MatchChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRepChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRep1ChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRep2ChoiceDecoders[kNumStates];
  CMyBitDecoder2 m_MatchRepShortChoiceDecoders[kNumStates][NLength::kNumPosStatesMax];

  CBitTreeDecoder<kNumMoveBitsForPosSlotCoder, kNumPosSlotBits> m_PosSlotDecoder[kNumLenToPosStates];

  CReverseBitTreeDecoder2<kNumMoveBitsForPosCoders> m_PosDecoders[kNumPosModels];
  CReverseBitTreeDecoder<kNumMoveBitsForAlignCoders, kNumAlignBits> m_PosAlignDecoder;
  // CBitTreeDecoder2<kNumMoveBitsForPosCoders> m_PosDecoders[kNumPosModels];
  // CBitTreeDecoder<kNumMoveBitsForAlignCoders, kNumAlignBits> m_PosAlignDecoder;
  
  NLength::CDecoder m_LenDecoder;
  NLength::CDecoder m_RepMatchLenDecoder;

  NLiteral::CDecoder m_LiteralDecoder;

  UINT32 m_DictionarySize;
  UINT32 m_DictionarySizeCheck;
  
  UINT32 m_PosStateMask;

public:

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

DECLARE_REGISTRY(CDecoder, TEXT("Compress.LZMADecoder.1"), 
                 TEXT("Compress.LZMADecoder"), UINT(0), THREADFLAGS_APARTMENT)
//DECLARE_NO_REGISTRY()

  CDecoder();
  HRESULT Create();


  HRESULT Init(ISequentialInStream *anInStream, 
      ISequentialOutStream *anOutStream);
  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_RangeDecoder.ReleaseStream();
  }

  class CDecoderFlusher
  {
    CDecoder *m_Decoder;
  public:
    bool m_NeedFlush;
    CDecoderFlusher(CDecoder *aDecoder): 
          m_Decoder(aDecoder), m_NeedFlush(true) {}
    ~CDecoderFlusher() 
    { 
      if (m_NeedFlush)
        m_Decoder->Flush();
      m_Decoder->ReleaseStreams(); 
    }
  };

  HRESULT Flush()
    {  return m_OutWindowStream.Flush(); }  

  // ICompressCoder interface
  STDMETHOD(CodeReal)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
  
  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *anInStream);

  HRESULT SetDictionarySize(UINT32 aDictionarySize);
  HRESULT SetLiteralProperties(UINT32 aLiteralPosStateBits, UINT32 aLiteralContextBits);
  HRESULT SetPosBitsProperties(UINT32 aNumPosStateBits);
};

}}

#endif