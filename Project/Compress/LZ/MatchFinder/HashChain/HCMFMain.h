// HCMFMain.h

#include "HCMain.h"

namespace HC_NAMESPACE {

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

STDMETHODIMP CMatchFinderHC::Init(ISequentialInStream *aStream)
  { return m_MatchFinder.Init(aStream); }

STDMETHODIMP_(void) CMatchFinderHC::ReleaseStream()
  { m_MatchFinder.ReleaseStream(); }

STDMETHODIMP CMatchFinderHC::MovePos()
  { return m_MatchFinder.MovePos(); }

STDMETHODIMP_(BYTE) CMatchFinderHC::GetIndexByte(UINT32 anIndex)
  { return m_MatchFinder.GetIndexByte(anIndex); }

STDMETHODIMP_(UINT32) CMatchFinderHC::GetMatchLen(UINT32 aIndex, 
    UINT32 aBack, UINT32 aLimit)
  { return m_MatchFinder.GetMatchLen(aIndex, aBack, aLimit); }

STDMETHODIMP_(UINT32) CMatchFinderHC::GetNumAvailableBytes()
  { return m_MatchFinder.GetNumAvailableBytes(); }
  
STDMETHODIMP CMatchFinderHC::Create(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter)
{ 
  UINT32 aWindowReservSize = (aSizeHistory + aKeepAddBufferBefore + 
      aMatchMaxLen + aKeepAddBufferAfter) / 2 + 256;
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

STDMETHODIMP_(UINT32) CMatchFinderHC::GetLongestMatch(UINT32 *aDistances)
  { return m_MatchFinder.GetLongestMatch(aDistances); }

STDMETHODIMP_(void) CMatchFinderHC::DummyLongestMatch()
  { m_MatchFinder.DummyLongestMatch(); }

STDMETHODIMP_(const BYTE *) CMatchFinderHC::GetPointerToCurrentPos()
{
  return m_MatchFinder.GetPointerToCurrentPos();
}

// IMatchFinderSetCallback
STDMETHODIMP CMatchFinderHC::SetCallback(IMatchFinderCallback *aCallback)
{
  m_MatchFinder.SetCallback(aCallback);
  return S_OK;
}


}
