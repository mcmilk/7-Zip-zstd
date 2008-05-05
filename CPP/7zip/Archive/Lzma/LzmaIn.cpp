// Archive/LzmaIn.cpp

#include "StdAfx.h"

#include "LzmaIn.h"

#include "../../Common/StreamUtils.h"

namespace NArchive {
namespace NLzma {
 
static bool CheckDictSize(const Byte *p)
{
  UInt32 dicSize = GetUi32(p);
  int i;
  for (i = 1; i <= 30; i++)
    if (dicSize == ((UInt32)2 << i) || dicSize == ((UInt32)3 << i))
      return true;
  return false;
}

HRESULT ReadStreamHeader(ISequentialInStream *inStream, CHeader &block)
{
  Byte sig[5 + 9];
  RINOK(ReadStream_FALSE(inStream, sig, 5 + 8));

  const Byte kMaxProp0Val = 5 * 5 * 9 - 1;
  if (sig[0] > kMaxProp0Val)
    return S_FALSE;

  for (int i = 0; i < 5; i++)
    block.LzmaProps[i] = sig[i];
  
  block.IsThereFilter = false;
  block.FilterMethod = 0;

  if (!CheckDictSize(sig + 1))
  {
    if (sig[0] > 1 || sig[1] > kMaxProp0Val)
      return S_FALSE;
    block.IsThereFilter = true;
    block.FilterMethod = sig[0];
    for (int i = 0; i < 5; i++)
      block.LzmaProps[i] = sig[i + 1];
    if (!CheckDictSize(block.LzmaProps + 1))
      return S_FALSE;
    RINOK(ReadStream_FALSE(inStream, sig + 5 + 8, 1));
  }
  UInt32 unpOffset = 5 + (block.IsThereFilter ? 1 : 0);
  block.UnpackSize = GetUi64(sig + unpOffset);
  if (block.HasUnpackSize() && block.UnpackSize >= ((UInt64)1 << 56))
    return S_FALSE;
  return S_OK;
}

}}
