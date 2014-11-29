// HashCon.cpp

#include "StdAfx.h"

#include "../../../Common/IntToString.h"
#include "../../../Common/StringConvert.h"

#include "../../../Windows/ErrorMsg.h"

#include "ConsoleClose.h"
#include "HashCon.h"

static const wchar_t *kEmptyFileAlias = L"[Content]";

static const char *kScanningMessage = "Scanning";

HRESULT CHashCallbackConsole::CheckBreak()
{
  return NConsoleClose::TestBreakSignal() ? E_ABORT : S_OK;
}

HRESULT CHashCallbackConsole::StartScanning()
{
  (*OutStream) << kScanningMessage;
  return CheckBreak();
}

HRESULT CHashCallbackConsole::ScanProgress(UInt64 /* numFolders */, UInt64 /* numFiles */, UInt64 /* totalSize */, const wchar_t * /* path */, bool /* isDir */)
{
  return CheckBreak();
}

HRESULT CHashCallbackConsole::CanNotFindError(const wchar_t *name, DWORD systemError)
{
  return CanNotFindError_Base(name, systemError);
}

HRESULT CHashCallbackConsole::FinishScanning()
{
  (*OutStream) << endl << endl;
  return CheckBreak();
}

HRESULT CHashCallbackConsole::SetNumFiles(UInt64 /* numFiles */)
{
  return CheckBreak();
}

HRESULT CHashCallbackConsole::SetTotal(UInt64 size)
{
  if (EnablePercents)
    m_PercentPrinter.SetTotal(size);
  return CheckBreak();
}

HRESULT CHashCallbackConsole::SetCompleted(const UInt64 *completeValue)
{
  if (completeValue && EnablePercents)
  {
    m_PercentPrinter.SetRatio(*completeValue);
    m_PercentPrinter.PrintRatio();
  }
  return CheckBreak();
}

static void AddMinuses(AString &s, unsigned num)
{
  for (unsigned i = 0; i < num; i++)
    s += '-';
}

static void SetSpaces(char *s, int num)
{
  for (int i = 0; i < num; i++)
    s[i] = ' ';
}

static void SetSpacesAndNul(char *s, int num)
{
  SetSpaces(s, num);
  s[num] = 0;
}

static void AddSpaces(UString &s, int num)
{
  for (int i = 0; i < num; i++)
    s += ' ';
}

static const int kSizeField_Len = 13;
static const int kNameField_Len = 12;

static unsigned GetColumnWidth(unsigned digestSize)
{
  unsigned width = digestSize * 2;
  const unsigned kMinColumnWidth = 8;
  return width < kMinColumnWidth ? kMinColumnWidth: width;
}

void CHashCallbackConsole::PrintSeparatorLine(const CObjectVector<CHasherState> &hashers)
{
  AString s;
  for (unsigned i = 0; i < hashers.Size(); i++)
  {
    const CHasherState &h = hashers[i];
    AddMinuses(s, GetColumnWidth(h.DigestSize));
    s += ' ';
  }
  AddMinuses(s, kSizeField_Len);
  s += "  ";
  AddMinuses(s, kNameField_Len);
  m_PercentPrinter.PrintString(s);
  m_PercentPrinter.PrintNewLine();
}

HRESULT CHashCallbackConsole::BeforeFirstFile(const CHashBundle &hb)
{
  UString s;
  FOR_VECTOR (i, hb.Hashers)
  {
    const CHasherState &h = hb.Hashers[i];
    s += h.Name;
    AddSpaces(s, (int)GetColumnWidth(h.DigestSize) - h.Name.Len() + 1);
  }
  UString s2 = L"Size";
  AddSpaces(s, kSizeField_Len - s2.Len());
  s += s2;
  s += L"  ";
  s += L"Name";
  m_PercentPrinter.PrintString(s);
  m_PercentPrinter.PrintNewLine();
  PrintSeparatorLine(hb.Hashers);
  return CheckBreak();
}

HRESULT CHashCallbackConsole::OpenFileError(const wchar_t *name, DWORD systemError)
{
  FailedCodes.Add(systemError);
  FailedFiles.Add(name);
  // if (systemError == ERROR_SHARING_VIOLATION)
  {
    m_PercentPrinter.PrintString(name);
    m_PercentPrinter.PrintString(": WARNING: ");
    m_PercentPrinter.PrintString(NWindows::NError::MyFormatMessage(systemError));
    return S_FALSE;
  }
  // return systemError;
}

HRESULT CHashCallbackConsole::GetStream(const wchar_t *name, bool /* isFolder */)
{
  m_FileName = name;
  return CheckBreak();
}

void CHashCallbackConsole::PrintResultLine(UInt64 fileSize,
    const CObjectVector<CHasherState> &hashers, unsigned digestIndex, bool showHash)
{
  FOR_VECTOR (i, hashers)
  {
    const CHasherState &h = hashers[i];

    char s[k_HashCalc_DigestSize_Max * 2 + 64];
    s[0] = 0;
    if (showHash)
      AddHashHexToString(s, h.Digests[digestIndex], h.DigestSize);
    SetSpacesAndNul(s + strlen(s), (int)GetColumnWidth(h.DigestSize) - (int)strlen(s) + 1);
    m_PercentPrinter.PrintString(s);
  }
  char s[64];
  s[0] = 0;
  char *p = s;
  if (showHash && fileSize != 0)
  {
    p = s + 32;
    ConvertUInt64ToString(fileSize, p);
    int numSpaces = kSizeField_Len - (int)strlen(p);
    if (numSpaces > 0)
    {
      p -= numSpaces;
      SetSpaces(p, numSpaces);
    }
  }
  else
    SetSpacesAndNul(s, kSizeField_Len - (int)strlen(s));
  unsigned len = (unsigned)strlen(p);
  p[len] = ' ';
  p[len + 1] = ' ';
  p[len + 2] = 0;
  m_PercentPrinter.PrintString(p);
}

HRESULT CHashCallbackConsole::SetOperationResult(UInt64 fileSize, const CHashBundle &hb, bool showHash)
{
  PrintResultLine(fileSize, hb.Hashers, k_HashCalc_Index_Current, showHash);
  if (m_FileName.IsEmpty())
    m_PercentPrinter.PrintString(kEmptyFileAlias);
  else
    m_PercentPrinter.PrintString(m_FileName);
  m_PercentPrinter.PrintNewLine();
  return S_OK;
}

static const char *k_DigestTitles[] =
{
    " :"
  , " for data:              "
  , " for data and names:    "
  , " for streams and names: "
};

static void PrintSum(CStdOutStream &p, const CHasherState &h, unsigned digestIndex)
{
  char s[k_HashCalc_DigestSize_Max * 2 + 64];
  UString name = h.Name;
  AddSpaces(name, 6 - (int)name.Len());
  p << name;
  p << k_DigestTitles[digestIndex];
  s[0] = 0;
  AddHashHexToString(s, h.Digests[digestIndex], h.DigestSize);
  p << s;
  p << "\n";
}


void PrintHashStat(CStdOutStream &p, const CHashBundle &hb)
{
  FOR_VECTOR (i, hb.Hashers)
  {
    const CHasherState &h = hb.Hashers[i];
    p << "\n";
    PrintSum(p, h, k_HashCalc_Index_DataSum);
    if (hb.NumFiles != 1 || hb.NumDirs != 0)
      PrintSum(p, h, k_HashCalc_Index_NamesSum);
    if (hb.NumAltStreams != 0)
      PrintSum(p, h, k_HashCalc_Index_StreamsSum);
  }
}

void CHashCallbackConsole::PrintProperty(const char *name, UInt64 value)
{
  char s[32];
  s[0] = ':';
  s[1] = ' ';
  ConvertUInt64ToString(value, s + 2);
  m_PercentPrinter.PrintString(name);
  m_PercentPrinter.PrintString(s);
  m_PercentPrinter.PrintNewLine();
}

HRESULT CHashCallbackConsole::AfterLastFile(const CHashBundle &hb)
{
  PrintSeparatorLine(hb.Hashers);

  PrintResultLine(hb.FilesSize, hb.Hashers, k_HashCalc_Index_DataSum, true);
  m_PercentPrinter.PrintNewLine();
  m_PercentPrinter.PrintNewLine();
  
  if (hb.NumFiles != 1 || hb.NumDirs != 0)
  {
    if (hb.NumDirs != 0)
      PrintProperty("Folders", hb.NumDirs);
    PrintProperty("Files", hb.NumFiles);
  }
  PrintProperty("Size", hb.FilesSize);
  if (hb.NumAltStreams != 0)
  {
    PrintProperty("AltStreams", hb.NumAltStreams);
    PrintProperty("AltStreams size", hb.AltStreamsSize);
  }
  PrintHashStat(*m_PercentPrinter.OutStream, hb);
  m_PercentPrinter.PrintNewLine();
  return S_OK;
}
