// DeflateProps.h

#ifndef __DEFLATE_PROPS_H
#define __DEFLATE_PROPS_H

#include "../ICoder.h"

namespace NArchive {

class CDeflateProps
{
  UInt32 Level;
  UInt32 NumPasses;
  UInt32 Fb;
  UInt32 Algo;
  UInt32 Mc;
  bool McDefined;

  void Init()
  {
    Level = NumPasses = Fb = Algo = Mc = 0xFFFFFFFF;
    McDefined = false;
  }
  void Normalize();
public:
  CDeflateProps() { Init(); }
  bool IsMaximum() const { return Algo > 0; }

  HRESULT SetCoderProperties(ICompressSetCoderProperties *setCoderProperties);
  HRESULT SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProps);
};

}

#endif
