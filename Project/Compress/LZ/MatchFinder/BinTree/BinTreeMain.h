// BinTreemain.h

// #include "StdAfx.h"

// #include "BinTree.h"
#include "Common/Defs.h"
#include "Common/CRC.h"

namespace BT_NAMESPACE {

#ifdef HASH_ARRAY_2


#ifdef HASH_ARRAY_3

static const UINT32 kNumHashDirectBytes = 0;
static const UINT32 kNumHash2MoveBits = 2;
static const UINT32 kHash2Size = 1 << (8 + kNumHash2MoveBits);

static const UINT32 kNumHashBytes = 4;

static const UINT32 kNumHash3MoveBits0 = 5;
static const UINT32 kNumHash3MoveBits1 = 5;
static const UINT32 kHash3Size = 1 << (8 + kNumHash3MoveBits0 + kNumHash3MoveBits1);

static const UINT32 kNumHashMoveBits0 = 4;
static const UINT32 kNumHashMoveBits1 = 4;
static const UINT32 kNumHashMoveBits2 = 4;
static const UINT32 kHashSize = 1 << (8 + kNumHashMoveBits0 + kNumHashMoveBits1 + kNumHashMoveBits2);

#else // no HASH_ARRAY_3

static const UINT32 kNumHashDirectBytes = 3;
static const UINT32 kNumHashBytes = 3;

static const UINT32 kHashSize = 1 << 24;
static const UINT32 kHash2Size = 1 << 16;

#endif // HASH_ARRAY_3

#else // no HASH_ARRAY_2

#ifdef HASH_ZIP 

static const UINT32 kNumHashBytes = 3;

static const UINT32 kNumHashMoveBits0 = 4;
static const UINT32 kNumHashMoveBits1 = 4;
static const UINT32 kNumHashDirectBytes = 0;
static const UINT32 kHashSize = 1 << (8 + kNumHashMoveBits0 + kNumHashMoveBits1);

#else // no HASH_ZIP 

static const UINT32 kNumHashBytes = 2;
static const UINT32 kNumHashDirectBytes = 2;
static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);

#endif // HASH_ZIP

#endif // HASH_ARRAY_2


CInTree::CInTree():
  #ifdef HASH_ARRAY_2
  m_Hash2(0),
  #ifdef HASH_ARRAY_3
  m_Hash3(0),
  #endif
  #endif
  m_Hash(0),
  m_LeftBase(0),
  m_RightBase(0),
  m_CutValue(0xFF)
{
}

template <class T>
class CArrayPtr
{
  T *m_Pointer;
public:
  CArrayPtr(): m_Pointer(0) {};
  CArrayPtr(T *p): m_Pointer(p) {};
  ~CArrayPtr()
  {
    if (m_Pointer != 0)
      delete []m_Pointer;
  }
  T *Release()
  {
    T *aTemp = m_Pointer;
    m_Pointer = 0;
    return aTemp;
  }
  CArrayPtr<T> & operator=(T *p)
  {
    if (m_Pointer != 0)
      delete []m_Pointer;
    m_Pointer = p;
    return *this;
  }
  operator T* () { return m_Pointer; }
};

typedef CArrayPtr<CIndex> CIndexArrayPtr;

HRESULT CInTree::Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, 
    UINT32 aMatchMaxLen, UINT32 aKeepAddBufferAfter, UINT32 aSizeReserv)
{
  CIn::Create(aSizeHistory + aKeepAddBufferBefore, 
                 aMatchMaxLen + aKeepAddBufferAfter, aSizeReserv);

  if (m_BlockSize + 256 > kMaxValForNormalize)
    return E_INVALIDARG;

  m_HistorySize = aSizeHistory;
  m_MatchMaxLen = aMatchMaxLen;
    
  delete []m_LeftBase;
  m_LeftBase = 0;
  delete []m_RightBase;
  m_RightBase = 0;

  CIndexArrayPtr aHash = m_Hash;
  m_Hash = 0;
  #ifdef HASH_ARRAY_2
  CIndexArrayPtr aHash2 = m_Hash2;
  m_Hash2 = 0; 
  #ifdef HASH_ARRAY_3
  CIndexArrayPtr aHash3 = m_Hash3;
  m_Hash3 = 0; 
  #endif
  #endif
  
  if (aHash == 0)
    aHash = new CIndex[kHashSize + 1];
  #ifdef HASH_ARRAY_2
  if (aHash2 == 0)
    aHash2 = new CIndex[kHash2Size + 1]; 
  #ifdef HASH_ARRAY_3
  if (aHash3 == 0)
    aHash3 = new CIndex[kHash3Size + 1]; 
  #endif
  #endif

  CArrayPtr<CIndex> aLeftBase = new CIndex[m_BlockSize + 1];
  CArrayPtr<CIndex> aRightBase = new CIndex[m_BlockSize + 1];

  m_Hash = aHash.Release();
  #ifdef HASH_ARRAY_2
  m_Hash2 = aHash2.Release(); 
  #ifdef HASH_ARRAY_3
  m_Hash3 = aHash3.Release(); 
  #endif
  #endif

  m_LeftBase = aLeftBase.Release();
  m_RightBase = aRightBase.Release();
  return S_OK;
}

static const UINT32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *aStream)
{
  RETURN_IF_NOT_S_OK(CIn::Init(aStream));
  for(int i = 0; i < kHashSize; i++)
    m_Hash[i] = kEmptyHashValue;

  #ifdef HASH_ARRAY_2
  for(i = 0; i < kHash2Size; i++)
    m_Hash2[i] = kEmptyHashValue;
  #ifdef HASH_ARRAY_3
  for(i = 0; i < kHash3Size; i++)
    m_Hash3[i] = kEmptyHashValue;
  #endif
  #endif

  m_LeftSon = m_LeftBase;
  m_RightSon = m_RightBase;

  ReduceOffsets(0 - 1);
  return S_OK;
}

CInTree::~CInTree()
{ 
  delete []m_RightBase;
  delete []m_LeftBase;
  delete []m_Hash; 

  #ifdef HASH_ARRAY_2
  delete []m_Hash2; 
  #ifdef HASH_ARRAY_3
  delete []m_Hash3; 
  #endif
  #endif

}

#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UINT32 Hash(const BYTE *aPointer, UINT32 &aHash2Value, UINT32 &aHash3Value)
{
  UINT32 aHash = UINT32(aPointer[0]) ^ CCRC::m_Table[aPointer[1]];

  aHash2Value = aHash & (kHash2Size - 1);

  aHash = (UINT32(aPointer[0]) << (kNumHash3MoveBits0 + kNumHash3MoveBits1)) ^
      (CCRC::m_Table[aPointer[1]]) ^
      aPointer[2];

  aHash3Value = aHash & (kHash3Size - 1);

  aHash = 
      (UINT32(aPointer[0]) << (kNumHashMoveBits0 + kNumHashMoveBits1 + kNumHashMoveBits2)) ^
      (CCRC::m_Table[aPointer[1]] << (kNumHashMoveBits1 + kNumHashMoveBits2)) ^
      (CCRC::m_Table[aPointer[2]]) ^
      aPointer[3];
  return aHash & (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UINT32 Hash(const BYTE *aPointer, UINT32 &aHash2Value)
{
  UINT32 aHash = (*((const UINT32 *)aPointer));
  aHash2Value = aHash &  0xFFFF;
  return aHash & 0xFFFFFF;
  /*
  UINT32 aHash = UINT32(aPointer[0]) ^ CCRC::m_Table[aPointer[1]];

  aHash2Value = aHash & (kHash2Size - 1);

  aHash = (UINT32(aPointer[0]) << (kNumHashMoveBits0 + kNumHashMoveBits1)) ^
      (CCRC::m_Table[aPointer[1]]) ^
      aPointer[2];
  return aHash & (kHashSize - 1);
  */
}

#endif // HASH_ARRAY_3
#else // no HASH_ARRAY_2

#ifdef HASH_ZIP 

inline UINT32 Hash(const BYTE *aPointer)
{
  /*
  return (UINT32(aPointer[0]) << (kNumHashMoveBits0 + kNumHashMoveBits1)) ^ 
    (UINT32(aPointer[1]) << kNumHashMoveBits1) ^ aPointer[2];
  */
  return ((UINT32(aPointer[0]) << (kNumHashMoveBits0 + kNumHashMoveBits1)) ^ 
    CCRC::m_Table[aPointer[1]] ^ aPointer[2]) & (kHashSize - 1);
}

#else // no HASH_ZIP 

inline UINT32 Hash(const BYTE *aPointer)
{
  return (UINT32(aPointer[0]) << 8) ^ aPointer[1];
}

#endif // HASH_ZIP

#endif // HASH_ARRAY_2


UINT32 CInTree::GetLongestMatch(UINT32 *aDistances)
{
  UINT32 aCurrentLimit;
  if (m_Pos + m_MatchMaxLen <= m_StreamPos)
    aCurrentLimit = m_MatchMaxLen;
  else
  {
    aCurrentLimit = m_StreamPos - m_Pos;
    if(aCurrentLimit < kNumHashBytes)
      return 0; 
  }

  UINT32 aMatchMinPos = (m_Pos > m_HistorySize) ? (m_Pos - m_HistorySize) : 1;
  BYTE *aCur = m_Buffer + m_Pos;
  
  UINT32 aMatchHashLenMax = 0;

  #ifdef HASH_ARRAY_2
 
  UINT32 aHash2Value;
  
  #ifdef HASH_ARRAY_3
  
  UINT32 aHash3Value;
  UINT32 aHashValue = Hash(aCur, aHash2Value, aHash3Value);
  
  #else // no HASH_ARRAY_3
  
  UINT32 aHashValue = Hash(aCur, aHash2Value);
  
  #endif // HASH_ARRAY_2
  
  #else // no hash
  
  UINT32 aHashValue = Hash(aCur);

  #endif

  #ifdef HASH_ARRAY_2

  UINT32 aCurMatch2 = m_Hash2[aHash2Value];
  m_Hash2[aHash2Value] = m_Pos;
  bool aMatchLen2Exist = false;
  UINT32 aLen2Distance = 0;
  if(aCurMatch2 >= aMatchMinPos)
  {
    if (m_Buffer[aCurMatch2] == aCur[0])
    {
      aLen2Distance = m_Pos - aCurMatch2 - 1;
      aMatchHashLenMax = 2;
      aMatchLen2Exist = true;
    }
  }

  #ifdef HASH_ARRAY_3
  
  UINT32 aCurMatch3 = m_Hash3[aHash3Value];
  m_Hash3[aHash3Value] = m_Pos;
  UINT32 aMatchLen3Exist = false;
  UINT32 aLen3Distance = 0;
  if(aCurMatch3 >= aMatchMinPos)
  {
    if (m_Buffer[aCurMatch3] == aCur[0] && m_Buffer[aCurMatch3 + 1] == aCur[1])
    {
      aLen3Distance = m_Pos - aCurMatch3 - 1;
      aMatchHashLenMax = 3;
      aMatchLen3Exist = true;
      if (aMatchLen2Exist)
      {
        if (aLen3Distance < aLen2Distance)
          aLen2Distance = aLen3Distance;
      }
      else
      {
        aLen2Distance = aLen3Distance;
        aMatchLen2Exist = true;
      }
    }
  }
  
  #endif
  #endif

  UINT32 aCurMatch = m_Hash[aHashValue];
  m_Hash[aHashValue] = m_Pos;
  if(aCurMatch < aMatchMinPos)
  {
    m_LeftSon[m_Pos] = kEmptyHashValue; 
    m_RightSon[m_Pos] = kEmptyHashValue; 

    #ifdef HASH_ARRAY_2
    aDistances[2] = aLen2Distance;
    #ifdef HASH_ARRAY_3
    aDistances[3] = aLen3Distance;
    #endif
    #endif


    return aMatchHashLenMax;
  }
  CIndex *aPtrLeft = &m_RightSon[m_Pos];
  CIndex *aPtrRight = &m_LeftSon[m_Pos];

  UINT32 aMax, aMinSameLeft, aMinSameRight, aMinSame;
  aMax = aMinSameLeft = aMinSameRight = aMinSame = kNumHashDirectBytes;

  #ifdef HASH_ARRAY_2
  #ifndef HASH_ARRAY_3
  aDistances[2] = aLen2Distance;
  #endif
  #endif

  aDistances[aMax] = m_Pos - aCurMatch - 1;
  
  for(UINT32 aCount = m_CutValue; aCount > 0; aCount--)
  {
    BYTE *pby1 = m_Buffer + aCurMatch;
    for(UINT32 aCurrentLen = aMinSame; aCurrentLen < aCurrentLimit; aCurrentLen++/*, dwComps++*/)
    {
      if (pby1[aCurrentLen] != aCur[aCurrentLen])
      {
        if (aCurrentLen > aMax)
        {
          UINT32 dwBack =  m_Pos - aCurMatch - 1;
          for(UINT32 dwLen = aMax + 1; dwLen <= aCurrentLen; dwLen++)
            aDistances[dwLen] = dwBack;
          aMax = aCurrentLen;
        }
        if (pby1[aCurrentLen] < aCur[aCurrentLen])
        {
          *aPtrRight = aCurMatch;
          aPtrRight = &m_RightSon[aCurMatch];
          aCurMatch = m_RightSon[aCurMatch];
          if(aCurrentLen > aMinSameLeft)
          {
            aMinSameLeft = aCurrentLen;
            aMinSame = MyMin(aMinSameLeft, aMinSameRight);
          }
        }
        else
        {
          *aPtrLeft = aCurMatch;
          aPtrLeft = &m_LeftSon[aCurMatch];
          aCurMatch = m_LeftSon[aCurMatch];
          if(aCurrentLen > aMinSameRight)
          {
            aMinSameRight = aCurrentLen;
            aMinSame = MyMin(aMinSameLeft, aMinSameRight);
          }
        }
        break;
      }
    }
    if(aCurrentLen == aCurrentLimit)
    {
      UINT32 dwBack =  m_Pos - aCurMatch - 1;
      for(UINT32 dwLen = aMax + 1; dwLen <= aCurrentLen; dwLen++)
        aDistances[dwLen] = dwBack;
      aMax = aCurrentLen;
      if(aCurrentLen < m_MatchMaxLen)
      {
        // assume that cur bytes 
        *aPtrLeft = aCurMatch;
        aPtrLeft = &m_LeftSon[aCurMatch];
        aCurMatch = m_LeftSon[aCurMatch];
        if(aCurrentLen > aMinSameRight)
        {
          aMinSameRight = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
      else
      {
        *aPtrLeft = m_RightSon[aCurMatch];
        *aPtrRight = m_LeftSon[aCurMatch];

        #ifdef HASH_ARRAY_2
        if (aMatchLen2Exist && aLen2Distance < aDistances[2])
          aDistances[2] = aLen2Distance;
        #ifdef HASH_ARRAY_3
        if (aMatchLen3Exist && aLen3Distance < aDistances[3])
          aDistances[3] = aLen3Distance;
        #endif
        #endif
      

        return aMax;
      }
    }
    if(aCurMatch < aMatchMinPos)
    {
      *aPtrLeft = kEmptyHashValue;
      *aPtrRight = kEmptyHashValue;
      #ifdef HASH_ARRAY_2
      if (aMatchLen2Exist)
      {
        if (aMax < 2)
        {
          aDistances[2] = aLen2Distance;
          aMax = 2;
        }
        else if (aLen2Distance < aDistances[2])
          aDistances[2] = aLen2Distance;
      }
      #ifdef HASH_ARRAY_3
      if (aMatchLen3Exist)
      {
        if (aMax < 3)
        {
          aDistances[3] = aLen3Distance;
          aMax = 3;
        }
        else if (aLen3Distance < aDistances[3])
          aDistances[3] = aLen3Distance;
      }
      #endif
      #endif

      return aMax;
    }
  }
  *aPtrLeft = kEmptyHashValue;
  *aPtrRight = kEmptyHashValue;
  #ifdef HASH_ARRAY_2
  if (aMatchLen2Exist)
  {
    if (aMax < 2)
    {
      aDistances[2] = aLen2Distance;
      aMax = 2;
    }
    else if (aLen2Distance < aDistances[2])
      aDistances[2] = aLen2Distance;
  }
  #ifdef HASH_ARRAY_3
  if (aMatchLen3Exist)
  {
    if (aMax < 3)
    {
      aDistances[3] = aLen3Distance;
      aMax = 3;
    }
    else if (aLen3Distance < aDistances[3])
      aDistances[3] = aLen3Distance;
  }
  #endif
  #endif
  return aMax;
}

void CInTree::DummyLongestMatch()
{
  UINT32 aCurrentLimit;
  if (m_Pos + m_MatchMaxLen <= m_StreamPos)
    aCurrentLimit = m_MatchMaxLen;
  else
  {
    aCurrentLimit = m_StreamPos - m_Pos;
    if(aCurrentLimit < kNumHashBytes)
      return; 
  }
  UINT32 aMatchMinPos = (m_Pos > m_HistorySize) ? (m_Pos - m_HistorySize) : 1;
  BYTE *aCur = m_Buffer + m_Pos;
  

  #ifdef HASH_ARRAY_2
  UINT32 aHash2Value;
  #ifdef HASH_ARRAY_3
  UINT32 aHash3Value;
  UINT32 aHashValue = Hash(aCur, aHash2Value, aHash3Value);
  m_Hash3[aHash3Value] = m_Pos;
  #else
  UINT32 aHashValue = Hash(aCur, aHash2Value);
  m_Hash2[aHash2Value] = m_Pos;
  #endif
  
  #else // no hash
  UINT32 aHashValue = Hash(aCur);
  #endif

  UINT32 aCurMatch = m_Hash[aHashValue];
  m_Hash[aHashValue] = m_Pos;
  if(aCurMatch < aMatchMinPos)
  {
    m_LeftSon[m_Pos] = kEmptyHashValue; 
    m_RightSon[m_Pos] = kEmptyHashValue; 
    return;
  }
  CIndex *aPtrLeft = &m_RightSon[m_Pos];
  CIndex *aPtrRight = &m_LeftSon[m_Pos];

  UINT32 aMax, aMinSameLeft, aMinSameRight, aMinSame;
  aMax = aMinSameLeft = aMinSameRight = aMinSame = kNumHashDirectBytes;
  for(UINT32 aCount = m_CutValue; aCount > 0; aCount--)
  {
    BYTE *pby1 = m_Buffer + aCurMatch;
    for(UINT32 aCurrentLen = aMinSame; aCurrentLen < aCurrentLimit; aCurrentLen++)
      if (pby1[aCurrentLen] != aCur[aCurrentLen])
      {
        if (pby1[aCurrentLen] < aCur[aCurrentLen])
        {
          *aPtrRight = aCurMatch;
          aPtrRight = &m_RightSon[aCurMatch];
          aCurMatch = m_RightSon[aCurMatch];
          if(aCurrentLen > aMinSameLeft)
          {
            aMinSameLeft = aCurrentLen;
            aMinSame = MyMin(aMinSameLeft, aMinSameRight);
          }
        }
        else 
        {
          *aPtrLeft = aCurMatch;
          aPtrLeft = &m_LeftSon[aCurMatch];
          aCurMatch = m_LeftSon[aCurMatch];
          if(aCurrentLen > aMinSameRight)
          {
            aMinSameRight = aCurrentLen;
            aMinSame = MyMin(aMinSameLeft, aMinSameRight);
          }
        }
        break;
      }
    if(aCurrentLen == aCurrentLimit)
    {
      if(aCurrentLen < m_MatchMaxLen)
      {
        *aPtrLeft = aCurMatch;
        aPtrLeft = &m_LeftSon[aCurMatch];
        aCurMatch = m_LeftSon[aCurMatch];
        if(aCurrentLen > aMinSameRight)
        {
          aMinSameRight = aCurrentLen;
          aMinSame = MyMin(aMinSameLeft, aMinSameRight);
        }
      }
      else
      {
        *aPtrLeft = m_RightSon[aCurMatch];
        *aPtrRight = m_LeftSon[aCurMatch];
        return;
      }
    }
    if(aCurMatch < aMatchMinPos)
    {
      *aPtrLeft = kEmptyHashValue;
      *aPtrRight = kEmptyHashValue;
      return;
    }
  }
  *aPtrLeft = kEmptyHashValue;
  *aPtrRight = kEmptyHashValue;
}

void CInTree::MoveBlock(UINT32 anOffset)
{
  UINT32 aNumBytesToMove = m_HistorySize * sizeof(CIndex);
  UINT32 aSpecOffset = ((m_LeftSon + m_Pos) - m_LeftBase) - m_HistorySize;
  memmove(m_LeftBase, m_LeftBase + aSpecOffset, aNumBytesToMove);
  m_LeftSon -= aSpecOffset;
  memmove(m_RightBase, m_RightBase + aSpecOffset, aNumBytesToMove);
  m_RightSon -= aSpecOffset;
  CIn::MoveBlock(anOffset);
}

void CInTree::NormalizeLinks(CIndex *anArray, UINT32 aNumItems, UINT32 aSubValue)
{
  for (UINT32 i = 0; i < aNumItems; i++)
  {
    UINT32 aValue = anArray[i];
    if (aValue <= aSubValue)
      aValue = kEmptyHashValue;
    else
      aValue -= aSubValue;
    anArray[i] = aValue;
  }
}

void CInTree::Normalize()
{
  UINT32 aStartItem = m_Pos - m_HistorySize;
  UINT32 aSubValue = aStartItem - 1;
  NormalizeLinks(m_LeftSon + aStartItem, m_HistorySize, aSubValue);
  NormalizeLinks(m_RightSon + aStartItem, m_HistorySize, aSubValue);
  NormalizeLinks(m_Hash, kHashSize, aSubValue);

  #ifdef HASH_ARRAY_2
  NormalizeLinks(m_Hash2, kHash2Size, aSubValue);
  #ifdef HASH_ARRAY_3
  NormalizeLinks(m_Hash3, kHash3Size, aSubValue);
  #endif
  #endif

  ReduceOffsets(aSubValue);
}
 
}
