// 7zItem.h

#ifndef __7Z_ITEM_H
#define __7Z_ITEM_H

#include "../../../Common/MyBuffer.h"
#include "../../../Common/MyString.h"

#include "../../Common/MethodId.h"

#include "7zHeader.h"

namespace NArchive {
namespace N7z {

const UInt64 k_AES = 0x06F10701;

typedef UInt32 CNum;
const CNum kNumMax     = 0x7FFFFFFF;
const CNum kNumNoIndex = 0xFFFFFFFF;

struct CCoderInfo
{
  CMethodId MethodID;
  CByteBuffer Props;
  CNum NumInStreams;
  CNum NumOutStreams;
  
  bool IsSimpleCoder() const { return (NumInStreams == 1) && (NumOutStreams == 1); }
};

struct CBindPair
{
  CNum InIndex;
  CNum OutIndex;
};

struct CFolder
{
  CObjArray2<CCoderInfo> Coders;
  CObjArray2<CBindPair> BindPairs;
  CObjArray2<CNum> PackStreams;

  CNum GetNumOutStreams() const
  {
    CNum result = 0;
    FOR_VECTOR(i, Coders)
      result += Coders[i].NumOutStreams;
    return result;
  }

  int FindBindPairForInStream(CNum inStreamIndex) const
  {
    FOR_VECTOR(i, BindPairs)
      if (BindPairs[i].InIndex == inStreamIndex)
        return i;
    return -1;
  }
  int FindBindPairForOutStream(CNum outStreamIndex) const
  {
    FOR_VECTOR(i, BindPairs)
      if (BindPairs[i].OutIndex == outStreamIndex)
        return i;
    return -1;
  }
  int FindPackStreamArrayIndex(CNum inStreamIndex) const
  {
    FOR_VECTOR(i, PackStreams)
      if (PackStreams[i] == inStreamIndex)
        return i;
    return -1;
  }

  int GetIndexOfMainOutStream() const
  {
    for (int i = (int)GetNumOutStreams() - 1; i >= 0; i--)
      if (FindBindPairForOutStream(i) < 0)
        return i;
    throw 1;
  }

  bool IsEncrypted() const
  {
    for (int i = Coders.Size() - 1; i >= 0; i--)
      if (Coders[i].MethodID == k_AES)
        return true;
    return false;
  }

  bool CheckStructure(unsigned numUnpackSizes) const;
};

struct CUInt32DefVector
{
  CBoolVector Defs;
  CRecordVector<UInt32> Vals;

  void ClearAndSetSize(unsigned newSize)
  {
    Defs.ClearAndSetSize(newSize);
    Vals.ClearAndSetSize(newSize);
  }

  void Clear()
  {
    Defs.Clear();
    Vals.Clear();
  }

  void ReserveDown()
  {
    Defs.ReserveDown();
    Vals.ReserveDown();
  }

  bool ValidAndDefined(unsigned i) const { return i < Defs.Size() && Defs[i]; }
};

struct CUInt64DefVector
{
  CBoolVector Defs;
  CRecordVector<UInt64> Vals;
  
  void Clear()
  {
    Defs.Clear();
    Vals.Clear();
  }
  
  void ReserveDown()
  {
    Defs.ReserveDown();
    Vals.ReserveDown();
  }

  bool GetItem(unsigned index, UInt64 &value) const
  {
    if (index < Defs.Size() && Defs[index])
    {
      value = Vals[index];
      return true;
    }
    value = 0;
    return false;
  }
  
  void SetItem(unsigned index, bool defined, UInt64 value);

  bool CheckSize(unsigned size) const { return Defs.Size() == size || Defs.Size() == 0; }
};

struct CFileItem
{
  UInt64 Size;
  UInt32 Attrib;
  UInt32 Crc;
  /*
  int Parent;
  bool IsAltStream;
  */
  bool HasStream; // Test it !!! it means that there is
                  // stream in some folder. It can be empty stream
  bool IsDir;
  bool CrcDefined;
  bool AttribDefined;

  CFileItem():
    /*
    Parent(-1),
    IsAltStream(false),
    */
    HasStream(true),
    IsDir(false),
    CrcDefined(false),
    AttribDefined(false)
      {}
  void SetAttrib(UInt32 attrib)
  {
    AttribDefined = true;
    Attrib = attrib;
  }
};

}}

#endif
