// Pat.h

// #pragma once

// #ifndef __PATRICIA__H
// #define __PATRICIA__H

#include "../../../Interface/CompressInterface.h"
#include "Common/AlignedBuffer.h"
#include "Stream/WindowIn.h"

namespace PAT_NAMESPACE {

struct CNode;

typedef CNode *CNodePointer;

#pragma pack(push, PragmaPatrTree)
#pragma pack(push, 1)

// #define __AUTO_REMOVE

// #define __USE_3_BYTES

// #define __NODE_4_BITS
// #define __NODE_3_BITS
// #define __NODE_2_BITS
// #define __NODE_2_BITS_PADDING

// #define __HASH_3


#ifdef __USE_3_BYTES
struct CIndex
{
  BYTE Data[3];
  operator =(UINT32 aValue)
  { 
    Data[0] = aValue & 0xFF;
    Data[1] = (aValue >> 8) & 0xFF;
    Data[2] = (aValue >> 16) & 0xFF;
  }
  operator UINT32() const { return (*((const UINT32 *)Data)) & 0xFFFFFF; }
};
#else
  typedef UINT32 CIndex;
#endif


#pragma pack(pop)
#pragma pack(pop, PragmaPatrTree)

#ifdef __NODE_4_BITS
  typedef UINT32 CIndex2;
  typedef UINT32 CSameBitsType;
#else
#ifdef __NODE_3_BITS
  typedef UINT32 CIndex2;
  typedef UINT32 CSameBitsType;
#else

  #ifdef __USE_3_BYTES
    typedef BYTE CSameBitsType;
  #else
    typedef UINT32 CIndex;
    typedef UINT32 CSameBitsType;
  #endif

  typedef CIndex CIndex2;
#endif
#endif

const UINT32 kNumBitsInIndex = sizeof(CIndex) * 8;
const UINT32 kMatchStartValue = UINT32(1) << (kNumBitsInIndex - 1);

typedef CIndex CMatchPointer;

const UINT32 kDescendantEmptyValue = kMatchStartValue - 1;

#pragma pack(push, PragmaPatrTree2)
#pragma pack(push, 1)

union CDescendant 
{
  CIndex NodePointer;
  CMatchPointer MatchPointer;
  bool IsEmpty() const { return NodePointer == kDescendantEmptyValue; }
  bool IsNode() const { return NodePointer < kDescendantEmptyValue; }
  bool IsMatch() const { return NodePointer > kDescendantEmptyValue; }
  void MakeEmpty() { NodePointer = kDescendantEmptyValue; }
};

#pragma pack(pop)
#pragma pack(pop, PragmaPatrTree2)

#pragma pack( push, PragmaBackNode)
#pragma pack( push, 1)

#undef MY_BYTE_SIZE

#ifdef __NODE_4_BITS
  #define MY_BYTE_SIZE 8
  const UINT32 kNumSubBits = 4;
#else
#ifdef __NODE_3_BITS
  #define MY_BYTE_SIZE 9
  const UINT32 kNumSubBits = 3;
#else
  #define MY_BYTE_SIZE 8
  #ifdef __NODE_2_BITS
    const UINT32 kNumSubBits = 2;
  #else
    const UINT32 kNumSubBits = 1;
  #endif
#endif
#endif

const UINT32 kNumSubNodes = 1 << kNumSubBits;
const UINT32 kSubNodesMask = kNumSubNodes - 1;

struct CNode
{
  CIndex2 LastMatch;
  CSameBitsType NumSameBits;
  union
  {
    CDescendant  Descendants[kNumSubNodes];
    UINT32 NextFreeNode;
  };
  #ifdef __NODE_2_BITS
  #ifdef __NODE_2_BITS_PADDING
  UINT32 Padding[2];
  #endif
  #endif
};

#pragma pack(pop)
#pragma pack(pop, PragmaBackNode)

#undef kIDNumBitsByte
#undef kIDNumBitsString

#ifdef __NODE_4_BITS
  #define kIDNumBitsByte 0x30
  #define kIDNumBitsString TEXT("4")
#else
#ifdef __NODE_3_BITS
  #define kIDNumBitsByte 0x20
  #define kIDNumBitsString TEXT("3")
#else
#ifdef __NODE_2_BITS
  #define kIDNumBitsByte 0x10
  #define kIDNumBitsString TEXT("2")
#else
  #define kIDNumBitsByte 0x00
  #define kIDNumBitsString TEXT("1")
#endif
#endif
#endif

#undef kIDManualRemoveByte
#undef kIDManualRemoveString

#ifdef __AUTO_REMOVE
  #define kIDManualRemoveByte 0x00
  #define kIDManualRemoveString TEXT("")
#else
  #define kIDManualRemoveByte 0x08
  #define kIDManualRemoveString TEXT("R")
#endif

#undef kIDHash3Byte
#undef kIDHash3String

#ifdef __HASH_3
  #define kIDHash3Byte 0x04
  #define kIDHash3String TEXT("H")
#else
  #define kIDHash3Byte 0x00
  #define kIDHash3String TEXT("")
#endif

#undef kIDUse3BytesByte
#undef kIDUse3BytesString

#ifdef __USE_3_BYTES
  #define kIDUse3BytesByte 0x02
  #define kIDUse3BytesString TEXT("T")
#else
  #define kIDUse3BytesByte 0x00
  #define kIDUse3BytesString TEXT("")
#endif

#undef kIDPaddingByte
#undef kIDPaddingString

#ifdef __NODE_2_BITS_PADDING
  #define kIDPaddingByte 0x01
  #define kIDPaddingString TEXT("P")
#else
  #define kIDPaddingByte 0x00
  #define kIDPaddingString TEXT("")
#endif


#undef kIDString
#define kIDString TEXT("Compress.MatchFinderPat") kIDNumBitsString kIDManualRemoveString kIDUse3BytesString kIDPaddingString kIDHash3String

// {23170F69-40C1-278C-01XX-0000000000}

DEFINE_GUID(PAT_CLSID, 
0x23170F69, 0x40C1, 0x278C, 0x01, 
kIDNumBitsByte | 
kIDManualRemoveByte | kIDHash3Byte | kIDUse3BytesByte | kIDPaddingByte, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// III(PAT_NAMESPACE)

class CPatricia: 
  public IInWindowStreamMatch,
  public CComObjectRoot,
  public CComCoClass<CPatricia, &PAT_CLSID>,
  NStream::NWindow::CIn
{
BEGIN_COM_MAP(CPatricia)
  COM_INTERFACE_ENTRY(IInWindowStreamMatch)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CPatricia)

  DECLARE_REGISTRY(CPatricia, kIDString TEXT(".1"), kIDString, 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Init)(ISequentialInStream *aStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(BYTE, GetIndexByte)(UINT32 anIndex);
  STDMETHOD_(UINT32, GetMatchLen)(UINT32 aIndex, UINT32 aBack, UINT32 aLimit);
  STDMETHOD_(UINT32, GetNumAvailableBytes)();
  STDMETHOD(Create)(UINT32 aSizeHistory, 
      UINT32 aKeepAddBufferBefore, UINT32 aMatchMaxLen, 
      UINT32 aKeepAddBufferAfter);
  STDMETHOD_(UINT32, GetLongestMatch)(UINT32 *aDistances);
  STDMETHOD_(void, DummyLongestMatch)();
  STDMETHOD_(const BYTE *, GetPointerToCurrentPos)();

public:
  CPatricia();
  ~CPatricia();

  UINT32 m_SizeHistory;
  UINT32 m_MatchMaxLen;

  CDescendant *m_HashDescendants;
  #ifdef __HASH_3
  CDescendant *m_Hash2Descendants;
  #endif

  CNode *m_Nodes;

  UINT32 m_FreeNode;
  UINT32 m_FreeNodeMax;

  #ifdef __AUTO_REMOVE
  UINT32 m_NumUsedNodes;
  UINT32 m_NumNodes;
  #else
  bool  m_SpecialRemoveMode;
  #endif

  bool  m_SpecialMode;
  UINT32 m_NumNotChangedCycles;
  UINT32 *m_TmpBacks;
  CAlignedBuffer m_AlignBuffer;


  void ChangeLastMatch(UINT32 aHashValue);
  
  #ifdef __AUTO_REMOVE
  void TestRemoveDescendant(CDescendant &aDescendant, UINT32 aLimitPos);
  void TestRemoveNodes();
  void RemoveNode(UINT32 anIndex);
  void TestRemoveAndNormalizeDescendant(CDescendant &aDescendant, 
      UINT32 aLimitPos, UINT32 aSubValue);
  void TestRemoveNodesAndNormalize();
  #else
  void NormalizeDescendant(CDescendant &aDescendant, UINT32 aSubValue);
  void Normalize();
  void RemoveMatch();
  #endif
private:
  void AddInternalNode(CNodePointer aNode, CIndex *aNodePointerPointer, 
      BYTE aByte, BYTE aByteXOR, UINT32 aNumSameBits, UINT32 aPos)
  {
    while((aByteXOR & kSubNodesMask) == 0)
    {
      aByteXOR >>= kNumSubBits;
      aByte >>= kNumSubBits;
      aNumSameBits -= kNumSubBits;
    }
    // Insert New Node
    CNodePointer aNewNode = &m_Nodes[m_FreeNode];
    UINT32 aNodeIndex = *aNodePointerPointer;
    *aNodePointerPointer = m_FreeNode;
    m_FreeNode = aNewNode->NextFreeNode;
    #ifdef __AUTO_REMOVE
    m_NumUsedNodes++;
    #endif
    if (m_FreeNode > m_FreeNodeMax)
    {
      m_FreeNodeMax = m_FreeNode;
      m_Nodes[m_FreeNode].NextFreeNode = m_FreeNode + 1;
    }

    UINT32 aBitsNew = aByte & kSubNodesMask;
    UINT32 aBitsOld = (aByte ^ aByteXOR) & kSubNodesMask;
    for (UINT32 i = 0; i < kNumSubNodes; i++)
      aNewNode->Descendants[i].NodePointer = kDescendantEmptyValue;
    aNewNode->Descendants[aBitsNew].MatchPointer = aPos + kMatchStartValue;
    aNewNode->Descendants[aBitsOld].NodePointer = aNodeIndex;
    aNewNode->NumSameBits = CSameBitsType(aNode->NumSameBits - aNumSameBits);
    aNewNode->LastMatch = aPos;
    
    aNode->NumSameBits = CSameBitsType(aNumSameBits - kNumSubBits);
  }

  void AddLeafNode(CNodePointer aNode, BYTE aByte, BYTE aByteXOR, 
      UINT32 aNumSameBits, UINT32 aPos, UINT32 aDescendantIndex)
  {
    for(;(aByteXOR & kSubNodesMask) == 0; aNumSameBits += kNumSubBits)
    {
      aByte >>= kNumSubBits;
      aByteXOR >>= kNumSubBits;
    }
    UINT32 aNewNodeIndex = m_FreeNode;
    CNodePointer aNewNode = &m_Nodes[m_FreeNode];
    m_FreeNode = aNewNode->NextFreeNode;
    #ifdef __AUTO_REMOVE
    m_NumUsedNodes++;
    #endif
    if (m_FreeNode > m_FreeNodeMax)
    {
      m_FreeNodeMax = m_FreeNode;
      m_Nodes[m_FreeNode].NextFreeNode = m_FreeNode + 1;
    }

    UINT32 aBitsNew = (aByte & kSubNodesMask);
    UINT32 aBitsOld = (aByte ^ aByteXOR) & kSubNodesMask;
    for (UINT32 i = 0; i < kNumSubNodes; i++)
      aNewNode->Descendants[i].NodePointer = kDescendantEmptyValue;
    aNewNode->Descendants[aBitsNew].MatchPointer = aPos + kMatchStartValue;
    aNewNode->Descendants[aBitsOld].MatchPointer = 
      aNode->Descendants[aDescendantIndex].MatchPointer;
    aNewNode->NumSameBits = CSameBitsType(aNumSameBits);
    aNewNode->LastMatch = aPos;
    aNode->Descendants[aDescendantIndex].NodePointer = aNewNodeIndex;
  }
};

}

// #endif
