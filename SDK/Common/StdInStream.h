// Common/StdInStream.h

#pragma once 

#ifndef __COMMON_STDINSTREAM_H
#define __COMMON_STDINSTREAM_H

#include "Common/String.h"

class CStdInStream 
{
  bool m_StreamIsOpen;
  FILE *m_Stream;
public:
  CStdInStream(): m_StreamIsOpen(false) {};
  CStdInStream(FILE *aStream): m_StreamIsOpen(false), m_Stream(aStream) {};
  ~CStdInStream();
  bool Open(LPCTSTR aFileName);
  bool Close();

  AString ScanStringUntilNewLine();
  bool Eof();
  int GetChar();
};

extern CStdInStream g_StdIn;

#endif
