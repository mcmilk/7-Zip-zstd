// BinTreeMFMain.h

// #include "StdAfx.h"

// #include "BinTreeMF.h"
#include "BinTreeMain.h"

namespace BT_NAMESPACE {

void CInTree2::BeforeMoveBlock()
{
  if (_callback)
    _callback->BeforeChangingBufferPos();
  CInTree::BeforeMoveBlock();
}

void CInTree2::AfterMoveBlock()
{
  CInTree::AfterMoveBlock();
  if (_callback)
    _callback->AfterChangingBufferPos();
}

STDMETHODIMP CMatchFinderBinTree::Init(ISequentialInStream *stream)
  { return _matchFinder.Init(stream); }

STDMETHODIMP_(void) CMatchFinderBinTree::ReleaseStream()
{ 
  // _matchFinder.ReleaseStream(); 
}

STDMETHODIMP CMatchFinderBinTree::MovePos()
  { return _matchFinder.MovePos(); }

STDMETHODIMP_(BYTE) CMatchFinderBinTree::GetIndexByte(UINT32 index)
  { return _matchFinder.GetIndexByte(index); }

STDMETHODIMP_(UINT32) CMatchFinderBinTree::GetMatchLen(UINT32 index, 
    UINT32 back, UINT32 limit)
  { return _matchFinder.GetMatchLen(index, back, limit); }

STDMETHODIMP_(UINT32) CMatchFinderBinTree::GetNumAvailableBytes()
  { return _matchFinder.GetNumAvailableBytes(); }
  
STDMETHODIMP CMatchFinderBinTree::Create(UINT32 sizeHistory, 
      UINT32 keepAddBufferBefore, UINT32 matchMaxLen, 
      UINT32 keepAddBufferAfter)
{ 
  UINT32 windowReservSize = (sizeHistory + keepAddBufferBefore + 
      matchMaxLen + keepAddBufferAfter) / 2 + 256;
  try 
  {
    return _matchFinder.Create(sizeHistory, keepAddBufferBefore, 
        matchMaxLen, keepAddBufferAfter, windowReservSize); 
  }
  catch(...)
  {
    return E_OUTOFMEMORY;
  }
}

STDMETHODIMP_(UINT32) CMatchFinderBinTree::GetLongestMatch(UINT32 *distances)
  { return _matchFinder.GetLongestMatch(distances); }

STDMETHODIMP_(void) CMatchFinderBinTree::DummyLongestMatch()
  { _matchFinder.DummyLongestMatch(); }

STDMETHODIMP_(const BYTE *) CMatchFinderBinTree::GetPointerToCurrentPos()
{
  return _matchFinder.GetPointerToCurrentPos();
}

// IMatchFinderSetCallback
STDMETHODIMP CMatchFinderBinTree::SetCallback(IMatchFinderCallback *callback)
{
  _matchFinder.SetCallback(callback);
  return S_OK;
}


}
