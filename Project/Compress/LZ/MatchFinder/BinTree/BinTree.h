// BinTree.h

// #pragma once

// #ifndef __BINTREE_H
// #define __BINTREE_H

#include "Stream/WindowIn.h"
#include "Common/Types.h"
#include "Windows/Defs.h"
 
namespace BT_NAMESPACE {


// #define __USE_3_BYTES

#ifdef __USE_3_BYTES

#pragma pack(push, PragmaBinTree)
#pragma pack(push, 1)

struct CIndex
{
  BYTE Data[3];
  CIndex(){}
  CIndex(UINT32 aValue)
  { 
    Data[0] = aValue & 0xFF;
    Data[1] = (aValue >> 8) & 0xFF;
    Data[2] = (aValue >> 16) & 0xFF;
  }
  operator UINT32() const { return (*((const UINT32 *)Data)) & 0xFFFFFF; }
};
const UINT32 kMaxValForNormalize = CIndex(-1);

#pragma pack(pop)
#pragma pack(pop, PragmaBinTree)

#else

typedef UINT32 CIndex;
const UINT32 kMaxValForNormalize = (UINT32(1) << 31) - 1;

#endif



// #define HASH_ARRAY_2

// #ifdef HASH_ARRAY_2

// #define HASH_ARRAY_3

// #else

// #define HASH_ZIP

// #endif

class CInTree: public NStream::NWindow::CIn
{
  UINT32 m_HistorySize;
  UINT32 m_MatchMaxLen;

  CIndex *m_Hash;
  
  #ifdef HASH_ARRAY_2
  CIndex *m_Hash2;
  #ifdef HASH_ARRAY_3
  CIndex *m_Hash3;
  #endif
  #endif
  
  CIndex *m_LeftSon;
  CIndex *m_RightSon;
  CIndex *m_LeftBase;
  CIndex *m_RightBase;

  UINT32 m_CutValue;

  void NormalizeLinks(CIndex *anArray, UINT32 aNumItems, UINT32 aSubValue);
  void Normalize();
  void FreeMemory();

protected:
  virtual void AfterMoveBlock();
public:
  CInTree();
  ~CInTree();
  HRESULT Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter, UINT32 _dwSizeReserv = (1<<17));
	HRESULT Init(ISequentialInStream *aStream);
  void SetCutValue(UINT32 aCutValue) { m_CutValue = aCutValue; }
  UINT32 GetLongestMatch(UINT32 *aDistances);
  void DummyLongestMatch();
  HRESULT MovePos()
  {
    RETURN_IF_NOT_S_OK(CIn::MovePos());
    if (m_Pos == kMaxValForNormalize)
      Normalize();
    return S_OK;
  }
  void ReduceOffsets(UINT32 aSubValue)
  {
    CIn::ReduceOffsets(aSubValue);
    m_LeftSon += aSubValue;
    m_RightSon += aSubValue;
  }
};

}

// #endif