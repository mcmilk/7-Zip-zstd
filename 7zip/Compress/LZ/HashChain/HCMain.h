// HC.h

// #include "StdAfx.h"

// #include "BinTree.h"
#include "Common/NewHandler.h"

#include "Common/Defs.h"
#include "Common/CRC.h"

namespace HC_NAMESPACE {

#ifdef HASH_ARRAY_2
  static const UINT32 kHash2Size = 1 << 10;
  #ifdef HASH_ARRAY_3
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kNumHashBytes = 4;
    static const UINT32 kHash3Size = 1 << 18;
    #ifdef HASH_BIG
    static const UINT32 kHashSize = 1 << 23;
    #else
    static const UINT32 kHashSize = 1 << 20;
    #endif
  #else
    static const UINT32 kNumHashBytes = 3;
    // static const UINT32 kNumHashDirectBytes = 3;
    // static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kHashSize = 1 << (16);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UINT32 kNumHashDirectBytes = 0;
    static const UINT32 kNumHashBytes = 3;
    static const UINT32 kHashSize = 1 << 16;
  #else
    static const UINT32 kNumHashDirectBytes = 2;
    static const UINT32 kNumHashBytes = 2;
    static const UINT32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif


CInTree::CInTree():
  #ifdef HASH_ARRAY_2
  _hash2(0),
  #ifdef HASH_ARRAY_3
  _hash3(0),
  #endif
  #endif
  _hash(0),
  _chain(0),
  _cutValue(16)
{
}

void CInTree::FreeMemory()
{
  #ifdef WIN32
  if (_chain != 0)
    VirtualFree(_chain, 0, MEM_RELEASE);
  if (_hash != 0)
    VirtualFree(_hash, 0, MEM_RELEASE);
  #else
  delete []_chain;
  delete []_hash;
  #endif
  _chain = 0;
  _hash = 0;
  CLZInWindow::Free();
}

CInTree::~CInTree()
{ 
  FreeMemory();
}

HRESULT CInTree::Create(UINT32 aSizeHistory, UINT32 aKeepAddBufferBefore, 
    UINT32 aMatchMaxLen, UINT32 aKeepAddBufferAfter, UINT32 aSizeReserv)
{
  FreeMemory();
  try
  {
    CLZInWindow::Create(aSizeHistory + aKeepAddBufferBefore, 
      aMatchMaxLen + aKeepAddBufferAfter, aSizeReserv);
    
    if (_blockSize + 256 > kMaxValForNormalize)
      return E_INVALIDARG;
    
    _historySize = aSizeHistory;
    _matchMaxLen = aMatchMaxLen;
    _cyclicBufferSize = aSizeHistory + 1;
    
    
    UINT32 aSize = kHashSize;
    #ifdef HASH_ARRAY_2
    aSize += kHash2Size;
    #ifdef HASH_ARRAY_3
    aSize += kHash3Size;
    #endif
    #endif
    
    #ifdef WIN32
    _chain = (CIndex *)::VirtualAlloc(0, (_cyclicBufferSize + 1) * sizeof(CIndex), MEM_COMMIT, PAGE_READWRITE);
    if (_chain == 0)
      throw CNewException();
    _hash = (CIndex *)::VirtualAlloc(0, (aSize + 1) * sizeof(CIndex), MEM_COMMIT, PAGE_READWRITE);
    if (_hash == 0)
      throw CNewException();
    #else
    _chain = new CIndex[_cyclicBufferSize + 1];
    _hash = new CIndex[aSize + 1];
    #endif
    
    // m_RightBase = &m_LeftBase[_blockSize];
    
    // _hash = &m_RightBase[_blockSize];
    #ifdef HASH_ARRAY_2
    _hash2 = &_hash[kHashSize]; 
    #ifdef HASH_ARRAY_3
    _hash3 = &_hash2[kHash2Size]; 
    #endif
    #endif
    return S_OK;
  }
  catch(...)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
}

static const UINT32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *aStream)
{
  RINOK(CLZInWindow::Init(aStream));
  int i;
  for(i = 0; i < kHashSize; i++)
    _hash[i] = kEmptyHashValue;

  #ifdef HASH_ARRAY_2
  for(i = 0; i < kHash2Size; i++)
    _hash2[i] = kEmptyHashValue;
  #ifdef HASH_ARRAY_3
  for(i = 0; i < kHash3Size; i++)
    _hash3[i] = kEmptyHashValue;
  #endif
  #endif

  _cyclicBufferPos = 0;

  ReduceOffsets(0 - 1);
  return S_OK;
}


#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UINT32 Hash(const BYTE *pointer, UINT32 &hash2Value, UINT32 &aHash3Value)
{
  UINT32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  aHash3Value = (temp ^ (UINT32(pointer[2]) << 8)) & (kHash3Size - 1);
  return (temp ^ (UINT32(pointer[2]) << 8) ^ (CCRC::Table[pointer[3]] << 5)) & 
      (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UINT32 Hash(const BYTE *pointer, UINT32 &hash2Value)
{
  UINT32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  return (temp ^ (UINT32(pointer[2]) << 8)) & (kHashSize - 1);;
}
#endif // HASH_ARRAY_3
#else // no HASH_ARRAY_2
#ifdef HASH_ZIP 
inline UINT32 Hash(const BYTE *pointer)
{
  return ((UINT32(pointer[0]) << 8) ^ 
      CCRC::Table[pointer[1]] ^ pointer[2]) & (kHashSize - 1);
}
#else // no HASH_ZIP 
inline UINT32 Hash(const BYTE *pointer)
{
  return pointer[0] ^ (UINT32(pointer[1]) << 8);
}
#endif // HASH_ZIP
#endif // HASH_ARRAY_2


UINT32 CInTree::GetLongestMatch(UINT32 *aDistances)
{
  UINT32 aCurrentLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    aCurrentLimit = _matchMaxLen;
  else
  {
    aCurrentLimit = _streamPos - _pos;
    if(aCurrentLimit < kNumHashBytes)
      return 0; 
  }

  UINT32 aMatchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  BYTE *aCur = _buffer + _pos;
  
  UINT32 aMatchHashLenMax = 0;

  #ifdef HASH_ARRAY_2
 
  UINT32 hash2Value;
  
  #ifdef HASH_ARRAY_3
  
  UINT32 aHash3Value;
  UINT32 hashValue = Hash(aCur, hash2Value, aHash3Value);
  
  #else // no HASH_ARRAY_3
  
  UINT32 hashValue = Hash(aCur, hash2Value);
  
  #endif // HASH_ARRAY_3
  
  #else // no HASH_ARRAY_2
  
  UINT32 hashValue = Hash(aCur);

  #endif

  #ifdef HASH_ARRAY_2

  UINT32 aCurMatch2 = _hash2[hash2Value];
  _hash2[hash2Value] = _pos;
  bool aMatchLen2Exist = false;
  UINT32 aLen2Distance = 0;
  if(aCurMatch2 >= aMatchMinPos)
  {
    if (_buffer[aCurMatch2] == aCur[0])
    {
      aLen2Distance = _pos - aCurMatch2 - 1;
      aMatchHashLenMax = 2;
      aMatchLen2Exist = true;
    }
  }

  #ifdef HASH_ARRAY_3
  
  UINT32 aCurMatch3 = _hash3[aHash3Value];
  _hash3[aHash3Value] = _pos;
  UINT32 aMatchLen3Exist = false;
  UINT32 aLen3Distance = 0;
  if(aCurMatch3 >= aMatchMinPos)
  {
    if (_buffer[aCurMatch3] == aCur[0])
    {
      aLen3Distance = _pos - aCurMatch3 - 1;
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

  UINT32 aCurMatch = _hash[hashValue];
  _hash[hashValue] = _pos;
  if(aCurMatch < aMatchMinPos)
  {
    _chain[_cyclicBufferPos] = kEmptyHashValue; 

    #ifdef HASH_ARRAY_2
    aDistances[2] = aLen2Distance;
    #ifdef HASH_ARRAY_3
    aDistances[3] = aLen3Distance;
    #endif
    #endif

    return aMatchHashLenMax;
  }
  _chain[_cyclicBufferPos] = aCurMatch;

 
  #ifdef HASH_ARRAY_2
  #ifndef HASH_ARRAY_3
    if (aMatchLen2Exist)
      aDistances[2] = aLen2Distance;
    else
      if (kNumHashDirectBytes >= 2)
        aDistances[2] = _pos - aCurMatch - 1;
  #endif
  #endif

  UINT32 aMax, aMinSame;
  
  aMax = aMinSame = kNumHashDirectBytes;

  aDistances[aMax] = _pos - aCurMatch - 1;
  
  for(UINT32 aCount = _cutValue; aCount > 0; aCount--)
  {
    BYTE *pby1 = _buffer + aCurMatch;
    UINT32 aCurrentLen;
    for(aCurrentLen = aMinSame; aCurrentLen < aCurrentLimit; aCurrentLen++/*, dwComps++*/)
      if (pby1[aCurrentLen] != aCur[aCurrentLen])
        break;
    if (aCurrentLen > aMax)
    {
      UINT32 dwBack =  _pos - aCurMatch - 1;
      for(UINT32 dwLen = aMax + 1; dwLen <= aCurrentLen; dwLen++)
        aDistances[dwLen] = dwBack;
      aMax = aCurrentLen;
    }
    if(aCurrentLen == aCurrentLimit)
      break;

    UINT32 aDelta = _pos - aCurMatch;
    UINT32 aCyclicPos = (aDelta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - aDelta):
        (_cyclicBufferPos - aDelta + _cyclicBufferSize);

    aCurMatch = _chain[aCyclicPos];
    if(aCurMatch < aMatchMinPos)
      break;
  }
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
  if (_pos + _matchMaxLen <= _streamPos)
    aCurrentLimit = _matchMaxLen;
  else
  {
    aCurrentLimit = _streamPos - _pos;
    if(aCurrentLimit < kNumHashBytes)
      return; 
  }
  UINT32 aMatchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  BYTE *aCur = _buffer + _pos;
  

  #ifdef HASH_ARRAY_2
  UINT32 hash2Value;
  #ifdef HASH_ARRAY_3
  UINT32 aHash3Value;
  UINT32 hashValue = Hash(aCur, hash2Value, aHash3Value);
  _hash3[aHash3Value] = _pos;
  #else
  UINT32 hashValue = Hash(aCur, hash2Value);
  #endif
  _hash2[hash2Value] = _pos;

  
  #else // no hash
  UINT32 hashValue = Hash(aCur);
  #endif

  UINT32 aCurMatch = _hash[hashValue];
  _hash[hashValue] = _pos;
  if(aCurMatch < aMatchMinPos)
  {
    _chain[_cyclicBufferPos] = kEmptyHashValue; 
    return;
  }
  _chain[_cyclicBufferPos] = aCurMatch;
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
  UINT32 aStartItem = _pos - _historySize;
  UINT32 aSubValue = aStartItem - 1;
  
  // NormalizeLinks(_chain + aStartItem, _historySize, aSubValue);
  NormalizeLinks(_chain, _cyclicBufferSize, aSubValue);

  NormalizeLinks(_hash, kHashSize, aSubValue);

  #ifdef HASH_ARRAY_2
  NormalizeLinks(_hash2, kHash2Size, aSubValue);
  #ifdef HASH_ARRAY_3
  NormalizeLinks(_hash3, kHash3Size, aSubValue);
  #endif
  #endif

  ReduceOffsets(aSubValue);
}
 
}
