// BinTreeMain.h

#include "../../../../Common/Defs.h"
#include "../../../../Common/CRC.h"
#include "../../../../Common/Alloc.h"

namespace BT_NAMESPACE {

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
    static const UInt32 kNumHashDirectBytes = 3;
    static const UInt32 kNumHashBytes = 3;
    static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#else
  #ifdef HASH_ZIP 
    static const UInt32 kNumHashDirectBytes = 0;
    static const UInt32 kNumHashBytes = 3;
    static const UInt32 kHashSize = 1 << 16;
  #else
    #define THERE_ARE_DIRECT_HASH_BYTES
    static const UInt32 kNumHashDirectBytes = 2;
    static const UInt32 kNumHashBytes = 2;
    static const UInt32 kHashSize = 1 << (8 * kNumHashBytes);
  #endif
#endif

static const UInt32 kHashSizeSum = kHashSize
    #ifdef HASH_ARRAY_2
    + kHash2Size
    #ifdef HASH_ARRAY_3
    + kHash3Size
    #endif
    #endif
    ;

#ifdef HASH_ARRAY_2
static const UInt32 kHash2Offset = kHashSize;
#ifdef HASH_ARRAY_3
static const UInt32 kHash3Offset = kHashSize + kHash2Size;
#endif
#endif

CMatchFinderBinTree::CMatchFinderBinTree():
  _hash(0),
  _cutValue(0xFF)
{
}

void CMatchFinderBinTree::FreeThisClassMemory()
{
  BigFree(_hash);
  _hash = 0;
}

void CMatchFinderBinTree::FreeMemory()
{
  FreeThisClassMemory();
  CLZInWindow::Free();
}

CMatchFinderBinTree::~CMatchFinderBinTree()
{ 
  FreeMemory();
}

STDMETHODIMP CMatchFinderBinTree::Create(UInt32 historySize, UInt32 keepAddBufferBefore, 
    UInt32 matchMaxLen, UInt32 keepAddBufferAfter)
{
  UInt32 sizeReserv = (historySize + keepAddBufferBefore + 
      matchMaxLen + keepAddBufferAfter) / 2 + 256;
  if (CLZInWindow::Create(historySize + keepAddBufferBefore, 
      matchMaxLen + keepAddBufferAfter, sizeReserv))
  {
    if (historySize + 256 > kMaxValForNormalize)
    {
      FreeMemory();
      return E_INVALIDARG;
    }
    _matchMaxLen = matchMaxLen;
    UInt32 newCyclicBufferSize = historySize + 1;
    if (_hash != 0 && newCyclicBufferSize == _cyclicBufferSize)
      return S_OK;
    FreeThisClassMemory();
    _cyclicBufferSize = newCyclicBufferSize; // don't change it
    _hash = (CIndex *)BigAlloc((kHashSizeSum + _cyclicBufferSize * 2) * sizeof(CIndex));
    if (_hash != 0)
      return S_OK;
  }
  FreeMemory();
  return E_OUTOFMEMORY;
}

static const UInt32 kEmptyHashValue = 0;

STDMETHODIMP CMatchFinderBinTree::Init(ISequentialInStream *stream)
{
  RINOK(CLZInWindow::Init(stream));
  for(UInt32 i = 0; i < kHashSizeSum; i++)
    _hash[i] = kEmptyHashValue;
  _cyclicBufferPos = 0;
  ReduceOffsets(-1);
  return S_OK;
}

STDMETHODIMP_(void) CMatchFinderBinTree::ReleaseStream()
{ 
  // ReleaseStream(); 
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
  hash2Value = (CCRC::Table[pointer[0]] ^ pointer[1]) & (kHash2Size - 1);
  return ((UInt32(pointer[0]) << 16)) | ((UInt32(pointer[1]) << 8)) | pointer[2];
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

STDMETHODIMP_(UInt32) CMatchFinderBinTree::GetLongestMatch(UInt32 *distances)
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

  UInt32 matchMinPos = (_pos > _cyclicBufferSize) ? (_pos - _cyclicBufferSize) : 0;
  Byte *cur = _buffer + _pos;

  UInt32 maxLen = 0;

  #ifdef HASH_ARRAY_2
  UInt32 hash2Value;
  #ifdef HASH_ARRAY_3
  UInt32 hash3Value;
  UInt32 hashValue = Hash(cur, hash2Value, hash3Value);
  #else
  UInt32 hashValue = Hash(cur, hash2Value);
  #endif
  #else
  UInt32 hashValue = Hash(cur);
  #endif

  UInt32 curMatch = _hash[hashValue];
  #ifdef HASH_ARRAY_2
  UInt32 curMatch2 = _hash[kHash2Offset + hash2Value];
  #ifdef HASH_ARRAY_3
  UInt32 curMatch3 = _hash[kHash3Offset + hash3Value];
  #endif
  _hash[kHash2Offset + hash2Value] = _pos;
  distances[2] = 0xFFFFFFFF;
  if(curMatch2 > matchMinPos)
    if (_buffer[curMatch2] == cur[0])
    {
      distances[2] = _pos - curMatch2 - 1;
      maxLen = 2;
    }

  #ifdef HASH_ARRAY_3
  _hash[kHash3Offset + hash3Value] = _pos;
  distances[3] = 0xFFFFFFFF;
  if(curMatch3 > matchMinPos)
    if (_buffer[curMatch3] == cur[0])
    {
      distances[3] = _pos - curMatch3 - 1;
      maxLen = 3;
    }
  #endif
  #endif

  _hash[hashValue] = _pos;

  CIndex *son = _hash + kHashSizeSum;
  CIndex *ptr0 = son + (_cyclicBufferPos << 1) + 1;
  CIndex *ptr1 = son + (_cyclicBufferPos << 1);

  distances[kNumHashBytes] = 0xFFFFFFFF;

  #ifdef THERE_ARE_DIRECT_HASH_BYTES
  if (lenLimit == kNumHashDirectBytes)
  {
    if(curMatch > matchMinPos)
      while (maxLen < kNumHashDirectBytes)
        distances[++maxLen] = _pos - curMatch - 1;
    // We don't need tree in this case
  }
  else
  #endif
  {
    UInt32 len0, len1;
    len0 = len1 = kNumHashDirectBytes;
    UInt32 count = _cutValue;
    while(true)
    {
      if(curMatch <= matchMinPos || count-- == 0)
      {
        *ptr0 = kEmptyHashValue;
        *ptr1 = kEmptyHashValue;
        break;
      }
      Byte *pb = _buffer + curMatch;
      UInt32 len = MyMin(len0, len1);
      do
      {
        if (pb[len] != cur[len])
          break;
      }
      while(++len != lenLimit);
      
      UInt32 delta = _pos - curMatch;
      while (maxLen < len)
        distances[++maxLen] = delta - 1;
      
      UInt32 cyclicPos = (delta <= _cyclicBufferPos) ?
          (_cyclicBufferPos - delta):
          (_cyclicBufferPos - delta + _cyclicBufferSize);
      CIndex *pair = son + (cyclicPos << 1);
      
      if (len != lenLimit)
      {
        if (pb[len] < cur[len])
        {
          *ptr1 = curMatch;
          ptr1 = pair + 1;
          curMatch = *ptr1;
          len1 = len;
        }
        else
        {
          *ptr0 = curMatch;
          ptr0 = pair;
          curMatch = *ptr0;
          len0 = len;
        }
      }
      else
      {
        *ptr1 = pair[0];
        *ptr0 = pair[1];
        break;
      }
    }
  }
  #ifdef HASH_ARRAY_2
  #ifdef HASH_ARRAY_3
  if (distances[4] < distances[3])
    distances[3] = distances[4];
  #endif
  if (distances[3] < distances[2])
    distances[2] = distances[3];
  #endif
  return maxLen;
}

STDMETHODIMP_(void) CMatchFinderBinTree::DummyLongestMatch()
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
  UInt32 matchMinPos = (_pos > _cyclicBufferSize) ? (_pos - _cyclicBufferSize) : 0;
  Byte *cur = _buffer + _pos;

  #ifdef HASH_ARRAY_2
  UInt32 hash2Value;
  #ifdef HASH_ARRAY_3
  UInt32 hash3Value;
  UInt32 hashValue = Hash(cur, hash2Value, hash3Value);
  _hash[kHash3Offset + hash3Value] = _pos;
  #else
  UInt32 hashValue = Hash(cur, hash2Value);
  #endif
  _hash[kHash2Offset + hash2Value] = _pos;
  #else
  UInt32 hashValue = Hash(cur);
  #endif

  UInt32 curMatch = _hash[hashValue];
  _hash[hashValue] = _pos;

  CIndex *son = _hash + kHashSizeSum;
  CIndex *ptr0 = son + (_cyclicBufferPos << 1) + 1;
  CIndex *ptr1 = son + (_cyclicBufferPos << 1);

  #ifdef THERE_ARE_DIRECT_HASH_BYTES
  if (lenLimit != kNumHashDirectBytes)
  #endif
  {
    UInt32 len0, len1;
    len0 = len1 = kNumHashDirectBytes;
    UInt32 count = _cutValue;
    while(true)
    {
      if(curMatch <= matchMinPos || count-- == 0)
        break;
      Byte *pb = _buffer + curMatch;
      UInt32 len = MyMin(len0, len1);
      do
      {
        if (pb[len] != cur[len])
          break;
      }
      while(++len != lenLimit);
      
      UInt32 delta = _pos - curMatch;
      UInt32 cyclicPos = (delta <= _cyclicBufferPos) ?
        (_cyclicBufferPos - delta):
      (_cyclicBufferPos - delta + _cyclicBufferSize);
      CIndex *pair = son + (cyclicPos << 1);
      
      if (len != lenLimit)
      {
        if (pb[len] < cur[len])
        {
          *ptr1 = curMatch;
          ptr1 = pair + 1;
          curMatch = *ptr1;
          len1 = len;
        }
        else 
        {
          *ptr0 = curMatch;
          ptr0 = pair;
          curMatch = *ptr0;
          len0 = len;
        }
      }
      else
      {
        *ptr1 = pair[0];
        *ptr0 = pair[1];
        return;
      }
    }
  }
  *ptr0 = kEmptyHashValue;
  *ptr1 = kEmptyHashValue;
}

void CMatchFinderBinTree::Normalize()
{
  UInt32 subValue = _pos - _cyclicBufferSize;
  CIndex *items = _hash;
  UInt32 numItems = (kHashSizeSum + _cyclicBufferSize * 2);
  for (UInt32 i = 0; i < numItems; i++)
  {
    UInt32 value = items[i];
    if (value <= subValue)
      value = kEmptyHashValue;
    else
      value -= subValue;
    items[i] = value;
  }
  ReduceOffsets(subValue);
}

STDMETHODIMP CMatchFinderBinTree::MovePos()
{
  if (++_cyclicBufferPos == _cyclicBufferSize)
    _cyclicBufferPos = 0;
  RINOK(CLZInWindow::MovePos());
  if (_pos == kMaxValForNormalize)
    Normalize();
  return S_OK;
}

STDMETHODIMP_(Byte) CMatchFinderBinTree::GetIndexByte(Int32 index)
  { return CLZInWindow::GetIndexByte(index); }

STDMETHODIMP_(UInt32) CMatchFinderBinTree::GetMatchLen(Int32 index, 
    UInt32 back, UInt32 limit)
  { return CLZInWindow::GetMatchLen(index, back, limit); }

STDMETHODIMP_(UInt32) CMatchFinderBinTree::GetNumAvailableBytes()
  { return CLZInWindow::GetNumAvailableBytes(); }

STDMETHODIMP_(const Byte *) CMatchFinderBinTree::GetPointerToCurrentPos()
  { return CLZInWindow::GetPointerToCurrentPos(); }

// IMatchFinderSetCallback
STDMETHODIMP CMatchFinderBinTree::SetCallback(IMatchFinderCallback *callback)
{
  m_Callback = callback;
  return S_OK;
}

void CMatchFinderBinTree::BeforeMoveBlock()
{
  if (m_Callback)
    m_Callback->BeforeChangingBufferPos();
  CLZInWindow::BeforeMoveBlock();
}

void CMatchFinderBinTree::AfterMoveBlock()
{
  if (m_Callback)
    m_Callback->AfterChangingBufferPos();
  CLZInWindow::AfterMoveBlock();
}
 
}
