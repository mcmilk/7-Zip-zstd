// Pat.h

// #ifndef __PATRICIA__H
// #define __PATRICIA__H

#include "../../../../Common/MyCom.h"
#include "../../../../Common/Types.h"
#include "../LZInWindow.h"

namespace PAT_NAMESPACE {

struct CNode;

typedef CNode *CNodePointer;

// #define __AUTO_REMOVE

// #define __NODE_4_BITS
// #define __NODE_3_BITS
// #define __NODE_2_BITS
// #define __NODE_2_BITS_PADDING

// #define __HASH_3


typedef UInt32 CIndex;

#ifdef __NODE_4_BITS
  typedef UInt32 CIndex2;
  typedef UInt32 CSameBitsType;
#else
#ifdef __NODE_3_BITS
  typedef UInt32 CIndex2;
  typedef UInt32 CSameBitsType;
#else

  typedef UInt32 CIndex;
  typedef UInt32 CSameBitsType;

  typedef CIndex CIndex2;
#endif
#endif

const UInt32 kNumBitsInIndex = sizeof(CIndex) * 8;
const UInt32 kMatchStartValue = UInt32(1) << (kNumBitsInIndex - 1);
// don't change kMatchStartValue definition, since it is used in 
// PatMain.h: 

typedef CIndex CMatchPointer;

const UInt32 kDescendantEmptyValue = kMatchStartValue - 1;

union CDescendant 
{
  CIndex NodePointer;
  CMatchPointer MatchPointer;
  bool IsEmpty() const { return NodePointer == kDescendantEmptyValue; }
  bool IsNode() const { return NodePointer < kDescendantEmptyValue; }
  bool IsMatch() const { return NodePointer > kDescendantEmptyValue; }
  void MakeEmpty() { NodePointer = kDescendantEmptyValue; }
};

#undef MY_BYTE_SIZE

#ifdef __NODE_4_BITS
  #define MY_BYTE_SIZE 8
  const UInt32 kNumSubBits = 4;
#else
#ifdef __NODE_3_BITS
  #define MY_BYTE_SIZE 9
  const UInt32 kNumSubBits = 3;
#else
  #define MY_BYTE_SIZE 8
  #ifdef __NODE_2_BITS
    const UInt32 kNumSubBits = 2;
  #else
    const UInt32 kNumSubBits = 1;
  #endif
#endif
#endif

const UInt32 kNumSubNodes = 1 << kNumSubBits;
const UInt32 kSubNodesMask = kNumSubNodes - 1;

struct CNode
{
  CIndex2 LastMatch;
  CSameBitsType NumSameBits;
  union
  {
    CDescendant  Descendants[kNumSubNodes];
    UInt32 NextFreeNode;
  };
  #ifdef __NODE_2_BITS
  #ifdef __NODE_2_BITS_PADDING
  UInt32 Padding[2];
  #endif
  #endif
};

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

#define kIDUse3BytesByte 0x00
#define kIDUse3BytesString TEXT("")

#undef kIDPaddingByte
#undef kIDPaddingString

#ifdef __NODE_2_BITS_PADDING
  #define kIDPaddingByte 0x01
  #define kIDPaddingString TEXT("P")
#else
  #define kIDPaddingByte 0x00
  #define kIDPaddingString TEXT("")
#endif


// #undef kIDString
// #define kIDString TEXT("Compress.MatchFinderPat") kIDNumBitsString kIDManualRemoveString kIDUse3BytesString kIDPaddingString kIDHash3String

// {23170F69-40C1-278C-01XX-0000000000}

DEFINE_GUID(PAT_CLSID, 
0x23170F69, 0x40C1, 0x278C, 0x01, 
kIDNumBitsByte | 
kIDManualRemoveByte | kIDHash3Byte | kIDUse3BytesByte | kIDPaddingByte, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// III(PAT_NAMESPACE)

class CPatricia: 
  public IMatchFinder,
  public IMatchFinderSetCallback,
  public CMyUnknownImp,
  CLZInWindow
{ 
  MY_UNKNOWN_IMP1(IMatchFinderSetCallback)

  STDMETHOD(Init)(ISequentialInStream *aStream);
  STDMETHOD_(void, ReleaseStream)();
  STDMETHOD(MovePos)();
  STDMETHOD_(Byte, GetIndexByte)(Int32 index);
  STDMETHOD_(UInt32, GetMatchLen)(Int32 index, UInt32 back, UInt32 limit);
  STDMETHOD_(UInt32, GetNumAvailableBytes)();
  STDMETHOD(Create)(UInt32 historySize, 
      UInt32 keepAddBufferBefore, UInt32 matchMaxLen, 
      UInt32 keepAddBufferAfter);
  STDMETHOD_(UInt32, GetLongestMatch)(UInt32 *distances);
  STDMETHOD_(void, DummyLongestMatch)();
  STDMETHOD_(const Byte *, GetPointerToCurrentPos)();

  void FreeMemory();
public:
  CPatricia();
  ~CPatricia();

  UInt32 _sizeHistory;
  UInt32 _matchMaxLen;

  CDescendant *m_HashDescendants;
  #ifdef __HASH_3
  CDescendant *m_Hash2Descendants;
  #endif

  CNode *m_Nodes;

  UInt32 m_FreeNode;
  UInt32 m_FreeNodeMax;

  #ifdef __AUTO_REMOVE
  UInt32 m_NumUsedNodes;
  UInt32 m_NumNodes;
  #else
  bool  m_SpecialRemoveMode;
  #endif

  bool  m_SpecialMode;
  UInt32 m_NumNotChangedCycles;
  UInt32 *m_TmpBacks;

  CMyComPtr<IMatchFinderCallback> m_Callback;

  virtual void BeforeMoveBlock();
  virtual void AfterMoveBlock();

  // IMatchFinderSetCallback
  STDMETHOD(SetCallback)(IMatchFinderCallback *callback);

  void ChangeLastMatch(UInt32 hashValue);
  
  #ifdef __AUTO_REMOVE
  void TestRemoveDescendant(CDescendant &descendant, UInt32 limitPos);
  void TestRemoveNodes();
  void RemoveNode(UInt32 index);
  void TestRemoveAndNormalizeDescendant(CDescendant &descendant, 
      UInt32 limitPos, UInt32 subValue);
  void TestRemoveNodesAndNormalize();
  #else
  void NormalizeDescendant(CDescendant &descendant, UInt32 subValue);
  void Normalize();
  void RemoveMatch();
  #endif
private:
  void AddInternalNode(CNodePointer aNode, CIndex *aNodePointerPointer, 
      Byte aByte, Byte aByteXOR, UInt32 aNumSameBits, UInt32 aPos)
  {
    while((aByteXOR & kSubNodesMask) == 0)
    {
      aByteXOR >>= kNumSubBits;
      aByte >>= kNumSubBits;
      aNumSameBits -= kNumSubBits;
    }
    // Insert New Node
    CNodePointer aNewNode = &m_Nodes[m_FreeNode];
    UInt32 aNodeIndex = *aNodePointerPointer;
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

    UInt32 aBitsNew = aByte & kSubNodesMask;
    UInt32 aBitsOld = (aByte ^ aByteXOR) & kSubNodesMask;
    for (UInt32 i = 0; i < kNumSubNodes; i++)
      aNewNode->Descendants[i].NodePointer = kDescendantEmptyValue;
    aNewNode->Descendants[aBitsNew].MatchPointer = aPos + kMatchStartValue;
    aNewNode->Descendants[aBitsOld].NodePointer = aNodeIndex;
    aNewNode->NumSameBits = CSameBitsType(aNode->NumSameBits - aNumSameBits);
    aNewNode->LastMatch = aPos;
    
    aNode->NumSameBits = CSameBitsType(aNumSameBits - kNumSubBits);
  }

  void AddLeafNode(CNodePointer aNode, Byte aByte, Byte aByteXOR, 
      UInt32 aNumSameBits, UInt32 aPos, UInt32 aDescendantIndex)
  {
    for(;(aByteXOR & kSubNodesMask) == 0; aNumSameBits += kNumSubBits)
    {
      aByte >>= kNumSubBits;
      aByteXOR >>= kNumSubBits;
    }
    UInt32 aNewNodeIndex = m_FreeNode;
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

    UInt32 aBitsNew = (aByte & kSubNodesMask);
    UInt32 aBitsOld = (aByte ^ aByteXOR) & kSubNodesMask;
    for (UInt32 i = 0; i < kNumSubNodes; i++)
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
