// HC.h

#include "../../../../Common/Defs.h"
#include "../../../../Common/CRC.h"
#include "../../../../Common/Alloc.h"

namespace HC_NAMESPACE {

#ifdef HASH_ARRAY_2
  static const UInt32 kHash2Size = 1 << 10;
  #ifdef HASH_ARRAY_3
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 4;
    static const UInt32 kHash3Size = 1 << 18;
    #ifdef HASH_BIG
    static const UInt32 kHashSize = 1 << 23;
    #else
    static const UInt32 kHashSize = 1 << 20;
    #endif
  #else
    static const UInt32 kNumHashBytes = 3;
    // static const UInt32 kNumHashDirectBytes = 3;
    // static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kHashSize = 1 << (16);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 3;
    static const UInt32 kHashSize = 1 << 16;
  #else
    static const UInt32 kNumHashDirectBytes = 2;
    static const UInt32 kNumHashBytes = 2;
    static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif


CInTree::CInTree():
  _hash(0),
  #ifdef HASH_ARRAY_2
  _hash2(0),
  #ifdef HASH_ARRAY_3
  _hash3(0),
  #endif
  #endif
  _chain(0),
  _cutValue(16)
{
}

void CInTree::FreeMemory()
{
  BigFree(_chain);
  _chain = 0;
  BigFree(_hash);
  _hash = 0;
  CLZInWindow::Free();
}

CInTree::~CInTree()
{ 
  FreeMemory();
}

HRESULT CInTree::Create(UInt32 historySize, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter, UInt32 sizeReserv)
{
  FreeMemory();
  if (!CLZInWindow::Create(historySize + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, sizeReserv))
    return E_OUTOFMEMORY;

  
  if (_blockSize + 256 > kMaxValForNormalize)
    return E_INVALIDARG;
  
  _historySize = historySize;
  _matchMaxLen = matchMaxLen;
  _cyclicBufferSize = historySize + 1;
  
  
  UInt32 size = kHashSize;
  #ifdef HASH_ARRAY_2
  size += kHash2Size;
  #ifdef HASH_ARRAY_3
  size += kHash3Size;
  #endif
  #endif
  
  _chain = (CIndex *)::BigAlloc((_cyclicBufferSize + 1) * sizeof(CIndex));
  if (_chain == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  _hash = (CIndex *)::BigAlloc((size + 1) * sizeof(CIndex));
  if (_hash == 0)
  {
    FreeMemory();
    return E_OUTOFMEMORY;
  }
  
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

static const UInt32 kEmptyHashValue = 0;

HRESULT CInTree::Init(ISequentialInStream *aStream)
{
  RINOK(CLZInWindow::Init(aStream));
  UInt32 i;
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

  ReduceOffsets(-1);
  return S_OK;
}


#ifdef HASH_ARRAY_2
#ifdef HASH_ARRAY_3
inline UInt32 Hash(const Byte *pointer, UInt32 &hash2Value, UInt32 &hash3Value)
{
  UInt32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  hash3Value = (temp ^ (UInt32(pointer[2]) << 8)) & (kHash3Size - 1);
  return (temp ^ (UInt32(pointer[2]) << 8) ^ (CCRC::Table[pointer[3]] << 5)) & 
      (kHashSize - 1);
}
#else // no HASH_ARRAY_3
inline UInt32 Hash(const Byte *pointer, UInt32 &hash2Value)
{
  UInt32 temp = CCRC::Table[pointer[0]] ^ pointer[1];
  hash2Value = temp & (kHash2Size - 1);
  return (temp ^ (UInt32(pointer[2]) << 8)) & (kHashSize - 1);;
}
#endif // HASH_ARRAY_3
#else // no HASH_ARRAY_2
#ifdef HASH_ZIP 
inline UInt32 Hash(const Byte *pointer)
{
  return ((UInt32(pointer[0]) << 8) ^ 
      CCRC::Table[pointer[1]] ^ pointer[2]) & (kHashSize - 1);
}
#else // no HASH_ZIP 
inline UInt32 Hash(const Byte *pointer)
{
  return pointer[0] ^ (UInt32(pointer[1]) << 8);
}
#endif // HASH_ZIP
#endif // HASH_ARRAY_2


UInt32 CInTree::GetLongestMatch(UInt32 *distances)
{
  UInt32 lenLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    lenLimit = _matchMaxLen;
  else
  {
    lenLimit = _streamPos - _pos;
    if(lenLimit < kNumHashBytes)
      return 0; 
  }

  UInt32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  Byte *cur = _buffer + _pos;
  
  UInt32 matchHashLenMax = 0;

  #ifdef HASH_ARRAY_2
 
  UInt32 hash2Value;
  
  #ifdef HASH_ARRAY_3
  
  UInt32 hash3Value;
  UInt32 hashValue = Hash(cur, hash2Value, hash3Value);
  
  #else // no HASH_ARRAY_3
  
  UInt32 hashValue = Hash(cur, hash2Value);
  
  #endif // HASH_ARRAY_3
  
  #else // no HASH_ARRAY_2
  
  UInt32 hashValue = Hash(cur);

  #endif

  #ifdef HASH_ARRAY_2

  UInt32 curMatch2 = _hash2[hash2Value];
  _hash2[hash2Value] = _pos;
  bool matchLen2Exist = false;
  UInt32 len2Distance = 0;
  if(curMatch2 >= matchMinPos)
  {
    if (_buffer[curMatch2] == cur[0])
    {
      len2Distance = _pos - curMatch2 - 1;
      matchHashLenMax = 2;
      matchLen2Exist = true;
    }
  }

  #ifdef HASH_ARRAY_3
  
  UInt32 curMatch3 = _hash3[hash3Value];
  _hash3[hash3Value] = _pos;
  UInt32 matchLen3Exist = false;
  UInt32 len3Distance = 0;
  if(curMatch3 >= matchMinPos)
  {
    if (_buffer[curMatch3] == cur[0])
    {
      len3Distance = _pos - curMatch3 - 1;
      matchHashLenMax = 3;
      matchLen3Exist = true;
      if (matchLen2Exist)
      {
        if (len3Distance < len2Distance)
          len2Distance = len3Distance;
      }
      else
      {
        len2Distance = len3Distance;
        matchLen2Exist = true;
      }
    }
  }
  
  #endif
  #endif

  UInt32 curMatch = _hash[hashValue];
  _hash[hashValue] = _pos;
  if(curMatch < matchMinPos)
  {
    _chain[_cyclicBufferPos] = kEmptyHashValue; 

    #ifdef HASH_ARRAY_2
    distances[2] = len2Distance;
    #ifdef HASH_ARRAY_3
    distances[3] = len3Distance;
    #endif
    #endif

    return matchHashLenMax;
  }
  _chain[_cyclicBufferPos] = curMatch;

 
  #ifdef HASH_ARRAY_2
  #ifndef HASH_ARRAY_3
    if (matchLen2Exist)
      distances[2] = len2Distance;
    else
      if (kNumHashDirectBytes >= 2)
        distances[2] = _pos - curMatch - 1;
  #endif
  #endif

  UInt32 maxLen, minSame;
  
  maxLen = minSame = kNumHashDirectBytes;

  distances[maxLen] = _pos - curMatch - 1;
  
  for(UInt32 aCount = _cutValue; aCount > 0; aCount--)
  {
    Byte *pby1 = _buffer + curMatch;
    UInt32 currentLen;
    for(currentLen = minSame; currentLen < lenLimit; currentLen++/*, dwComps++*/)
      if (pby1[currentLen] != cur[currentLen])
        break;
    if (currentLen > maxLen)
    {
      UInt32 back =  _pos - curMatch - 1;
      for(UInt32 len = maxLen + 1; len <= currentLen; len++)
        distances[len] = back;
      maxLen = currentLen;
    }
    if(currentLen == lenLimit)
      break;

    UInt32 delta = _pos - curMatch;
    UInt32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
        (_cyclicBufferPos - delta + _cyclicBufferSize);

    curMatch = _chain[cyclicPos];
    if(curMatch < matchMinPos)
      break;
  }
  #ifdef HASH_ARRAY_2
  if (matchLen2Exist)
  {
    if (maxLen < 2)
    {
      distances[2] = len2Distance;
      maxLen = 2;
    }
    else if (len2Distance < distances[2])
      distances[2] = len2Distance;
  }
  #ifdef HASH_ARRAY_3
  if (matchLen3Exist)
  {
    if (maxLen < 3)
    {
      distances[3] = len3Distance;
      maxLen = 3;
    }
    else if (len3Distance < distances[3])
      distances[3] = len3Distance;
  }
  #endif
  #endif
  return maxLen;
}

void CInTree::DummyLongestMatch()
{
  UInt32 lenLimit;
  if (_pos + _matchMaxLen <= _streamPos)
    lenLimit = _matchMaxLen;
  else
  {
    lenLimit = _streamPos - _pos;
    if(lenLimit < kNumHashBytes)
      return; 
  }
  UInt32 matchMinPos = (_pos > _historySize) ? (_pos - _historySize) : 1;
  Byte *cur = _buffer + _pos;
  

  #ifdef HASH_ARRAY_2
  UInt32 hash2Value;
  #ifdef HASH_ARRAY_3
  UInt32 hash3Value;
  UInt32 hashValue = Hash(cur, hash2Value, hash3Value);
  _hash3[hash3Value] = _pos;
  #else
  UInt32 hashValue = Hash(cur, hash2Value);
  #endif
  _hash2[hash2Value] = _pos;

  
  #else // no hash
  UInt32 hashValue = Hash(cur);
  #endif

  UInt32 curMatch = _hash[hashValue];
  _hash[hashValue] = _pos;
  if(curMatch < matchMinPos)
  {
    _chain[_cyclicBufferPos] = kEmptyHashValue; 
    return;
  }
  _chain[_cyclicBufferPos] = curMatch;
}

void CInTree::NormalizeLinks(CIndex *items, UInt32 numItems, UInt32 subValue)
{
  for (UInt32 i = 0; i < numItems; i++)
  {
    UInt32 value = items[i];
    if (value <= subValue)
      value = kEmptyHashValue;
    else
      value -= subValue;
    items[i] = value;
  }
}

void CInTree::Normalize()
{
  UInt32 startItem = _pos - _historySize;
  UInt32 subValue = startItem - 1;
  
  // NormalizeLinks(_chain + startItem, _historySize, subValue);
  NormalizeLinks(_chain, _cyclicBufferSize, subValue);

  NormalizeLinks(_hash, kHashSize, subValue);

  #ifdef HASH_ARRAY_2
  NormalizeLinks(_hash2, kHash2Size, subValue);
  #ifdef HASH_ARRAY_3
  NormalizeLinks(_hash3, kHash3Size, subValue);
  #endif
  #endif

  ReduceOffsets(subValue);
}
 
}
