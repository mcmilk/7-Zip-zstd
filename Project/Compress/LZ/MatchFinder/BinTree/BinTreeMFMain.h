// BinTreeMFMain.h

// #include "StdAfx.h"

// #include "BinTreeMF.h"
#include "BinTreeMain.h"

namespace BT_NAMESPACE {

void CInTree2::BeforeMoveBlock()
{
  if (m_Callback)
    m_Callback->BeforeChangingBufferPos();
  CInTree::BeforeMoveBlock();
}

void CInTree2::AfterMoveBlock()
{
  if (m_Callback)
    m_Callback->AfterChangingBufferPos();
  CInTree::AfterMoveBlock();
}

STDMETHODIMP CMatchFinderBinTree::Init(ISequentialInStream *aStream)
  { return m_MatchFinder.Init(aStream); }

STDMETHODIMP_(void) CMatchFinderBinTree::ReleaseStream()
  { m_MatchFinder.ReleaseStream(); }

STDMETHODIMP CMatchFinderBinTree::MovePos()
  { return m_MatchFinder.MovePos(); }

STDMETHODIMP_(BYTE) CMatchFinderBinTree::GetIndexByte(UINT32 anIndex)
  { return m_MatchFinder.GetIndexByte(anIndex); }

STDMETHODIMP_(UINT32) CMatchFinderBinTree::GetMatchLen(UINT32 aIndex, 
    UINT32 aBack, UINT32 aLimit)
  { return m_MatchFinder.GetMatchLen(aIndex, aBack, aLimit); }

STDMETHODIMP_(UINT32) CMatchFinderBinTree::GetNumAvailableBytes()
  { return m_MatchFinder.GetNumAvailableBytes(); }
  
STDMETHODIMP CMatchFinderBinTree::Create(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter)
{ 
  const UINT32 kAlignMask = (1 << 16) - 1;
  UINT32 aWindowReservSize = aSizeHistory / 2;
  aWindowReservSize += kAlignMask;
  aWindowReservSize &= ~(kAlignMask);

  const kMinDictSize = (1 << 19);
  if (aWindowReservSize < kMinDictSize)
    aWindowReservSize = kMinDictSize;
  aWindowReservSize += 256;

  try 
  {
    return m_MatchFinder.Create(aSizeHistory, aKeepAddBufferBefore, aMatchMaxLen, 
      aKeepAddBufferAfter, aWindowReservSize); 
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }
}

STDMETHODIMP_(UINT32) CMatchFinderBinTree::GetLongestMatch(UINT32 *aDistances)
  { return m_MatchFinder.GetLongestMatch(aDistances); }

STDMETHODIMP_(void) CMatchFinderBinTree::DummyLongestMatch()
  { m_MatchFinder.DummyLongestMatch(); }

STDMETHODIMP_(const BYTE *) CMatchFinderBinTree::GetPointerToCurrentPos()
{
  return m_MatchFinder.GetPointerToCurrentPos();
}

// IMatchFinderSetCallback
STDMETHODIMP CMatchFinderBinTree::SetCallback(IMatchFinderCallback *aCallback)
{
  m_MatchFinder.SetCallback(aCallback);
  return S_OK;
}


}
