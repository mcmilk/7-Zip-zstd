// Common/StdOutStream.cpp

#include "StdAfx.h"

#include "StdOutStream.h"

static const char kNewLineChar =  '\n';

static const char *kOneStringFormat = "%s";
static const char *kOneIntFormat = "%d";
static const char *kOneUINT32Format = "%u";
static const char *kOneUINT64Format = "%I64u";

static LPCTSTR kFileOpenMode = "wt";

CStdOutStream  g_StdOut(stdout);
CStdOutStream  g_StdErr(stderr);

bool CStdOutStream::Open(LPCTSTR aFileName)
{
  Close();
  m_Stream = _tfopen(aFileName, kFileOpenMode);
  m_StreamIsOpen = (m_Stream != 0);
  return m_StreamIsOpen;
}

bool CStdOutStream::Close()
{
  if(!m_StreamIsOpen)
    return true;
  m_StreamIsOpen = (fclose(m_Stream) != 0);
  return !m_StreamIsOpen;
}

CStdOutStream ::~CStdOutStream ()
{
  Close();
}


CStdOutStream & CStdOutStream ::operator<<(CStdOutStream & (*aFunction)(CStdOutStream  &))
{
  (*aFunction)(*this);    
  return *this;
}

CStdOutStream & endl(CStdOutStream & anOut)
{
  return anOut << kNewLineChar;
}

CStdOutStream & CStdOutStream::operator<<(const char *aChars)
{
  fprintf(m_Stream, kOneStringFormat, aChars);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(char aChar)
{
  putc(aChar, m_Stream);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(int aNumber)
{
  fprintf(m_Stream, kOneIntFormat, aNumber);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(UINT32 aNumber)
{
  fprintf(m_Stream, kOneUINT32Format, aNumber);
  return *this;
}

CStdOutStream & CStdOutStream::operator<<(UINT64 aNumber)
{
  fprintf(m_Stream, kOneUINT64Format, aNumber);
  return *this;
}
