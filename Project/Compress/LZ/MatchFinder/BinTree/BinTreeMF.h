// BinTreeMF.h

// #pragma once

// #ifndef __BINTREEMF_H
// #define __BINTREEMF_H

#include "../../../Interface/CompressInterface.h"
#include "BinTree.h"

namespace BT_NAMESPACE {

#undef kIDByte
#undef kIDString

#ifdef HASH_ARRAY_2
  #ifdef HASH_ARRAY_3
    #define kIDByte 0x3
    #define kIDString "234"
  #else
    #define kIDByte 0x2
    #define kIDString "23"
  #endif
#else
  #ifdef HASH_ZIP
    #define kIDByte 0x0
    #define kIDString "3"
  #else
    #define kIDByte 0x1
    #define kIDString "2"
  #endif
#endif

#undef kIDUse3BytesByte
#undef kIDUse3BytesString

#ifdef __USE_3_BYTES
  #define kIDUse3BytesByte 0x80
  #define kIDUse3BytesString "T"
#else
  #define kIDUse3BytesByte 0x00
  #define kIDUse3BytesString ""
#endif

#undef kIDStringFull

#define kIDStringFull "Compress.MatchFinderBT" kIDString kIDUse3BytesString

// {23170F69-40C1-278C-02XX-0000000000}
DEFINE_GUID(BT_CLSID, 
0x23170F69, 0x40C1, 0x278C, 0x02, kIDByte | kIDUse3BytesByte, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

class CMatchFinderBinTree: 
  public IInWindowStreamMatch,
  public CComObjectRoot,
  public CComCoClass<CMatchFinderBinTree, &BT_CLSID>
{
BEGIN_COM_MAP(CMatchFinderBinTree)
  COM_INTERFACE_ENTRY(IInWindowStreamMatch)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CMatchFinderBinTree)

DECLARE_REGISTRY(CMatchFinderBinTree, kIDStringFull ".1", kIDStringFull, 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Init)(ISequentialInStream *aStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(BYTE, GetIndexByte)(UINT32 anIndex);
  STDMETHOD_(UINT32, GetMatchLen)(UINT32 aIndex, UINT32 aBack, UINT32 aLimit);
  STDMETHOD_(UINT32, GetNumAvailableBytes)();
  STDMETHOD_(const BYTE *, GetPointerToCurrentPos)();
  STDMETHOD(Create)(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter);
  STDMETHOD_(UINT32, GetLongestMatch)(UINT32 *aDistances);
  STDMETHOD_(void, DummyLongestMatch)();

private:
  // UINT32 m_WindowReservSize;
  CInTree m_MatchFinder;
public:
  // CMatchFinderBinTree(): m_WindowReservSize((1 << 19) + 256) {};
  void SetCutValue(UINT32 aCutValue) 
    { m_MatchFinder.SetCutValue(aCutValue); }
  /*
  void SetWindowReservSize(UINT32 aReservWindowSize)
    { m_WindowReservSize = aReservWindowSize; }
  */
};
 
}

// #endif

