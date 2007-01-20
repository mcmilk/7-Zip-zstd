// LzhCRC.h

#ifndef __LZH_CRC_H
#define __LZH_CRC_H

#include <stddef.h>
#include "Common/Types.h"

namespace NArchive {
namespace NLzh {

class CCRC
{
  UInt16 _value;
public:
  static UInt16 Table[256];
  static void InitTable();
  
  CCRC():  _value(0){};
  void Init() { _value = 0; }
  void Update(const void *data, size_t size);
  UInt16 GetDigest() const { return _value; } 
};

}}

#endif
