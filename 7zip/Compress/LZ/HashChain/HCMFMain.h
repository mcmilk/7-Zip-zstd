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

STDMETHODIMP CMatchFinderHC::Init(ISequentialInStream *inStream)
  { return m_MatchFinder.Init(inStream); }

STDMETHODIMP_(void) CMatchFinderHC::ReleaseStream()
{ 
  // m_MatchFinder.ReleaseStream(); 
}

STDMETHODIMP CMatchFinderHC::MovePos()
  { return m_MatchFinder.MovePos(); }

STDMETHODIMP_(Byte) CMatchFinderHC::GetIndexByte(Int32 index)
  { return m_MatchFinder.GetIndexByte(index); }

STDMETHODIMP_(UInt32) CMatchFinderHC::GetMatchLen(Int32 index, 
    UInt32 back, UInt32 limit)
  { return m_MatchFinder.GetMatchLen(index, back, limit); }

STDMETHODIMP_(UInt32) CMatchFinderHC::GetNumAvailableBytes()
  { return m_MatchFinder.GetNumAvailableBytes(); }
  
STDMETHODIMP CMatchFinderHC::Create(UInt32 historySize, 
      UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter)
{ 
  UInt32 windowReservSize = (historySize + keepAddBufferBefore + 
      matchMaxLen + keepAddBufferAfter) / 2 + 256;
  // try 
  {
    return m_MatchFinder.Create(historySize, keepAddBufferBefore, matchMaxLen, 
      keepAddBufferAfter, windowReservSize); 
  }
  /*
  catch(...)
  {
    return E_OUTOFMEMORY;
  }
  */
}

STDMETHODIMP_(UInt32) CMatchFinderHC::GetLongestMatch(UInt32 *distances)
  { return m_MatchFinder.GetLongestMatch(distances); }

STDMETHODIMP_(void) CMatchFinderHC::DummyLongestMatch()
  { m_MatchFinder.DummyLongestMatch(); }

STDMETHODIMP_(const Byte *) CMatchFinderHC::GetPointerToCurrentPos()
{
  return m_MatchFinder.GetPointerToCurrentPos();
}

// IMatchFinderSetCallback
STDMETHODIMP CMatchFinderHC::SetCallback(IMatchFinderCallback *callback)
{
  m_MatchFinder.SetCallback(callback);
  return S_OK;
}


}
