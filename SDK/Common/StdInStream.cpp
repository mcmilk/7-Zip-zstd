// Common/StdInStream.cpp

#include "StdAfx.h"

#include "StdInStream.h"

static const char kIllegalChar = '\0';
static const char kNewLineChar = '\n';

static const char *kEOFMessage = "Unexpected end of input stream";
static const char *kReadErrorMessage  ="Error reading input stream"; 
static const char *kIllegalCharMessage = "Illegal character in input stream";

static LPCTSTR kFileOpenMode = _T("rt");

CStdInStream g_StdIn(stdin);

bool CStdInStream::Open(LPCTSTR aFileName)
{
  Close();
  m_Stream = _tfopen(aFileName, kFileOpenMode);
  m_StreamIsOpen = (m_Stream != 0);
  return m_StreamIsOpen;
}

bool CStdInStream::Close()
{
  if(!m_StreamIsOpen)
    return true;
  m_StreamIsOpen = (fclose(m_Stream) != 0);
  return !m_StreamIsOpen;
}

CStdInStream::~CStdInStream()
{
  Close();
}

AString CStdInStream::ScanStringUntilNewLine()
{
  AString aString;
  while(true)
  {
    int aIntChar = GetChar();
    if(aIntChar == EOF)
      throw kEOFMessage;
    char aChar = char(aIntChar);
    if (aChar == kIllegalChar)
      throw kIllegalCharMessage;
    if(aChar == kNewLineChar)
      return aString;
    aString += aChar;
  }
}

bool CStdInStream::Eof()
{
  return (feof(m_Stream) != 0);
}

int CStdInStream::GetChar()
{
  int aIntChar = getc(m_Stream);
  if(aIntChar == EOF && !Eof())
    throw kReadErrorMessage;
  return aIntChar;
}


