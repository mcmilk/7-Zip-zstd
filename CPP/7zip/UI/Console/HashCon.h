// HashCon.h

#ifndef __HASH_CON_H
#define __HASH_CON_H

#include "../Common/HashCalc.h"

#include "UpdateCallbackConsole.h"

class CHashCallbackConsole: public IHashCallbackUI, public CCallbackConsoleBase
{
  UString m_FileName;

  void PrintSeparatorLine(const CObjectVector<CHasherState> &hashers);
  void PrintResultLine(UInt64 fileSize,
      const CObjectVector<CHasherState> &hashers, unsigned digestIndex, bool showHash);
  void PrintProperty(const char *name, UInt64 value);
public:
  ~CHashCallbackConsole() { }

  INTERFACE_IHashCallbackUI(;)
};

void PrintHashStat(CStdOutStream &stdStream, const CHashBundle &hb);

#endif
