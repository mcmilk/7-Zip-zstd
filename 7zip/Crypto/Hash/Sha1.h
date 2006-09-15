// Sha1.h
// This file is based on public domain 
// Steve Reid and Wei Dai's code from Crypto++

#ifndef __SHA1_H
#define __SHA1_H

#include <stddef.h>
#include "../../../Common/Types.h"

// Sha1 implementation in RAR before version 3.60 has bug:
// it changes data bytes in some cases.
// So this class supports both versions: normal_SHA and rar3Mode

namespace NCrypto {
namespace NSha1 {

const unsigned int kBlockSize = 64;
const unsigned int kDigestSize = 20;

const unsigned int kBlockSizeInWords = (kBlockSize >> 2);
const unsigned int kDigestSizeInWords = (kDigestSize >> 2);

class CContextBase
{
protected:
  UInt32 _state[5];
  UInt64 _count;
  void UpdateBlock(UInt32 *data, bool returnRes = false) 
  { 
    GetBlockDigest(data, _state, returnRes);  
    _count++;
  }
public:
  void Init();
  void GetBlockDigest(UInt32 *blockData, UInt32 *destDigest, bool returnRes = false);
  // PrepareBlock can be used only when size <= 13. size in Words
  void PrepareBlock(UInt32 *block, unsigned int size) const;
};

class CContextBase2: public CContextBase
{
protected:
  unsigned int _count2;
  UInt32 _buffer[kBlockSizeInWords];
  void UpdateBlock() { CContextBase::UpdateBlock(_buffer); }
public:
  void Init() { CContextBase::Init(); _count2 = 0; }
};

class CContext: public CContextBase2
{
public:
  void Update(Byte *data, size_t size, bool rar350Mode = false);
  void Update(const Byte *data, size_t size) { Update((Byte *)data, size, false); }
  void Final(Byte *digest);
};

class CContext32: public CContextBase2
{
public:
  void Update(const UInt32 *data, size_t size);
  void Final(UInt32 *digest);
};

}}

#endif
