// BenchmarkDialog.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringToInt.h"
#include "Common/MyException.h"
#include "Windows/Thread.h"
#include "Windows/Error.h"
#include "Windows/System.h"
#include "../FileManager/HelpUtils.h"
// #include "BenchmarkDialogRes.h"
#include "BenchmarkDialog.h"

using namespace NWindows;

// const int kNumBenchDictionaryBitsStart = 21;

static LPCWSTR kHelpTopic = L"fm/benchmark.htm";

static const UINT_PTR kTimerID = 4;
static const UINT kTimerElapse = 1000;

#ifdef LANG        
#include "../FileManager/LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_BENCHMARK_DICTIONARY, 0x02000D0C },
  { IDC_BENCHMARK_MEMORY, 0x03080001 },
  { IDC_BENCHMARK_NUM_THREADS, 0x02000D12 },
  { IDC_BENCHMARK_SPEED_LABEL, 0x03080004 },
  { IDC_BENCHMARK_RATING_LABEL, 0x03080005 },
  { IDC_BENCHMARK_COMPRESSING, 0x03080002 },
  { IDC_BENCHMARK_DECOMPRESSING, 0x03080003 },
  { IDC_BENCHMARK_CURRENT, 0x03080007 },
  { IDC_BENCHMARK_RESULTING, 0x03080008 },
  { IDC_BENCHMARK_CURRENT2, 0x03080007 },
  { IDC_BENCHMARK_RESULTING2, 0x03080008 },
  { IDC_BENCHMARK_TOTAL_RATING, 0x03080006 },
  { IDC_BENCHMARK_ELAPSED, 0x02000C01 },
  { IDC_BENCHMARK_SIZE, 0x02000C03 },
  { IDC_BENCHMARK_PASSES, 0x03080009 },
  // { IDC_BENCHMARK_ERRORS, 0x0308000A },
  { IDC_BENCHMARK_USAGE_LABEL, 0x0308000B },
  { IDC_BENCHMARK_RPU_LABEL, 0x0308000C },
  { IDC_BENCHMARK_COMBO_NUM_THREADS, 0x02000D12},
 
  { IDC_BUTTON_STOP, 0x02000714 },
  { IDC_BUTTON_RESTART, 0x02000715 },
  { IDHELP, 0x02000720 },
  { IDCANCEL, 0x02000710 }
};
#endif

static void MyMessageBoxError(HWND hwnd, LPCWSTR message)
{
  MessageBoxW(hwnd, message, L"7-Zip", MB_ICONERROR);
}

const LPCTSTR kProcessingString = TEXT("...");
const LPCTSTR kMB = TEXT(" MB");
const LPCTSTR kMIPS =  TEXT(" MIPS");
const LPCTSTR kKBs = TEXT(" KB/s");

static const int kMinDicLogSize = 21;
static const UInt32 kMinDicSize = (1 << kMinDicLogSize);
static const UInt32 kMaxDicSize =       
    #ifdef _WIN64
    (1 << 30);
    #else
    (1 << 27);
    #endif

bool CBenchmarkDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x03080000);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif

  _syncInfo.Init();

  m_Dictionary.Attach(GetItem(IDC_BENCHMARK_COMBO_DICTIONARY));
  int cur = 0;
  // _syncInfo.DictionarySize = (1 << kNumBenchDictionaryBitsStart);

  UInt32 numCPUs = NSystem::GetNumberOfProcessors();
  if (numCPUs < 1)
    numCPUs = 1;
  numCPUs = MyMin(numCPUs, (UInt32)(1 << 8));
  cur = 0;
  bool setDefaultThreads = (_syncInfo.NumThreads == (UInt32)(-1));
  if (setDefaultThreads)
  {
    _syncInfo.NumThreads = numCPUs;
    if (_syncInfo.NumThreads > 1)
      _syncInfo.NumThreads &= ~1;
  }

  UInt64 ramSize = NSystem::GetRamSize();
  bool setDefaultDictionary = (_syncInfo.DictionarySize == (UInt32)(-1));
  if (setDefaultDictionary)
  {
    int dicSizeLog;
    for (dicSizeLog = 25; dicSizeLog >= kBenchMinDicLogSize; dicSizeLog--)
      if (GetBenchMemoryUsage(_syncInfo.NumThreads, ((UInt32)1 << dicSizeLog)) + (8 << 20) <= ramSize)
        break;
    _syncInfo.DictionarySize = (1 << dicSizeLog);
  }
  if (_syncInfo.DictionarySize < kMinDicSize)
    _syncInfo.DictionarySize = kMinDicSize;
  if (_syncInfo.DictionarySize > kMaxDicSize)
    _syncInfo.DictionarySize = kMaxDicSize;

  for (int i = kMinDicLogSize; i <= 30; i++)
    for (int j = 0; j < 2; j++)
    {
      UInt32 dictionary = (1 << i) + (j << (i - 1));
      if (dictionary > kMaxDicSize)
        continue;
      TCHAR s[40];
      ConvertUInt64ToString((dictionary >> 20), s);
      lstrcat(s, kMB);
      int index = (int)m_Dictionary.AddString(s);
      m_Dictionary.SetItemData(index, dictionary);
      if (dictionary <= _syncInfo.DictionarySize)
        cur = index;
    }
  m_Dictionary.SetCurSel(cur);

  m_NumThreads.Attach(GetItem(IDC_BENCHMARK_COMBO_NUM_THREADS));
  for (UInt32 num = 1; ;)
  {
    if (num > numCPUs * 2)
      break;
    TCHAR s[40];
    ConvertUInt64ToString(num, s);
    int index = (int)m_NumThreads.AddString(s);
    m_NumThreads.SetItemData(index, num);
    if (num <= numCPUs && setDefaultThreads)
    {
      _syncInfo.NumThreads = num;
      cur = index;
    }
    if (num > 1)
      num++;
    num++;
  }
  m_NumThreads.SetCurSel(cur);

  OnChangeSettings();

  _syncInfo._startEvent.Set();
  _timer = SetTimer(kTimerID, kTimerElapse);
  return CModalDialog::OnInit();
}

UInt32 CBenchmarkDialog::GetNumberOfThreads()
{
  return (UInt32)m_NumThreads.GetItemData(m_NumThreads.GetCurSel());
}

UInt32 CBenchmarkDialog::OnChangeDictionary()
{
  UInt32 dictionary = (UInt32)m_Dictionary.GetItemData(m_Dictionary.GetCurSel());
  UInt64 memUsage = GetBenchMemoryUsage(GetNumberOfThreads(), dictionary);
  memUsage = (memUsage + (1 << 20) - 1) >> 20;
  TCHAR s[40];
  ConvertUInt64ToString(memUsage, s);
  lstrcat(s, kMB);
  SetItemText(IDC_BENCHMARK_MEMORY_VALUE, s);
  return dictionary;
}

static const UInt32 g_IDs[] = 
{
  IDC_BENCHMARK_COMPRESSING_USAGE,
  IDC_BENCHMARK_COMPRESSING_USAGE2,
  IDC_BENCHMARK_COMPRESSING_SPEED,
  IDC_BENCHMARK_COMPRESSING_SPEED2,
  IDC_BENCHMARK_COMPRESSING_RATING,
  IDC_BENCHMARK_COMPRESSING_RATING2,
  IDC_BENCHMARK_COMPRESSING_RPU,
  IDC_BENCHMARK_COMPRESSING_RPU2,
  
  IDC_BENCHMARK_DECOMPRESSING_SPEED,
  IDC_BENCHMARK_DECOMPRESSING_SPEED2,
  IDC_BENCHMARK_DECOMPRESSING_RATING,
  IDC_BENCHMARK_DECOMPRESSING_RATING2,
  IDC_BENCHMARK_DECOMPRESSING_USAGE,
  IDC_BENCHMARK_DECOMPRESSING_USAGE2,
  IDC_BENCHMARK_DECOMPRESSING_RPU,
  IDC_BENCHMARK_DECOMPRESSING_RPU2,
  
  IDC_BENCHMARK_TOTAL_USAGE_VALUE,
  IDC_BENCHMARK_TOTAL_RATING_VALUE,
  IDC_BENCHMARK_TOTAL_RPU_VALUE
};
  
void CBenchmarkDialog::OnChangeSettings()
{
  EnableItem(IDC_BUTTON_STOP, true);
  UInt32 dictionary = OnChangeDictionary();
  TCHAR s[40] = { TEXT('/'), TEXT(' '), 0 };
  ConvertUInt64ToString(NSystem::GetNumberOfProcessors(), s + 2);
  SetItemText(IDC_BENCHMARK_HARDWARE_THREADS, s);
  for (int i = 0; i < sizeof(g_IDs) / sizeof(g_IDs[0]); i++)
    SetItemText(g_IDs[i], kProcessingString);
  _startTime = GetTickCount();
  PrintTime();
  NWindows::NSynchronization::CCriticalSectionLock lock(_syncInfo.CS);
  _syncInfo.Init();
  _syncInfo.DictionarySize = dictionary;
  _syncInfo.Changed = true;
  _syncInfo.NumThreads = GetNumberOfThreads();
}

void CBenchmarkDialog::OnRestartButton()
{
  OnChangeSettings();
}

void CBenchmarkDialog::OnStopButton()
{
  EnableItem(IDC_BUTTON_STOP, false);
  _syncInfo.Pause();
}

void CBenchmarkDialog::OnHelp() 
{
  ShowHelpWindow(NULL, kHelpTopic);
}

void CBenchmarkDialog::OnCancel() 
{
  _syncInfo.Stop();
  KillTimer(_timer);
  CModalDialog::OnCancel();
}

static void GetTimeString(UInt64 timeValue, TCHAR *s)
{
  wsprintf(s, TEXT("%02d:%02d:%02d"), 
      UInt32(timeValue / 3600),
      UInt32((timeValue / 60) % 60), 
      UInt32(timeValue % 60));
}

void CBenchmarkDialog::PrintTime()
{
  UInt32 curTime = ::GetTickCount();
  UInt32 elapsedTime = (curTime - _startTime);
  UInt32 elapsedSec = elapsedTime / 1000;
  if (elapsedSec != 0 && _syncInfo.WasPaused())
    return;
  TCHAR s[40];
  GetTimeString(elapsedSec, s);
  SetItemText(IDC_BENCHMARK_ELAPSED_VALUE, s);
}

void CBenchmarkDialog::PrintRating(UInt64 rating, UINT controlID)
{
  TCHAR s[40];
  ConvertUInt64ToString(rating / 1000000, s);
  lstrcat(s, kMIPS);
  SetItemText(controlID, s);
}

void CBenchmarkDialog::PrintUsage(UInt64 usage, UINT controlID)
{
  TCHAR s[40];
  ConvertUInt64ToString((usage + 5000) / 10000, s);
  lstrcat(s, TEXT("%"));
  SetItemText(controlID, s);
}

void CBenchmarkDialog::PrintResults(
    UInt32 dictionarySize,
    const CBenchInfo2 &info, 
    UINT usageID, UINT speedID, UINT rpuID, UINT ratingID,
    bool decompressMode)
{
  if (info.GlobalTime == 0)
    return;

  UInt64 size = info.UnpackSize;
  TCHAR s[40];
  {
    UInt64 speed = size * info.GlobalFreq / info.GlobalTime;
    ConvertUInt64ToString(speed / 1024, s);
    lstrcat(s, kKBs);
    SetItemText(speedID, s);
  }
  UInt64 rating;
  if (decompressMode)
    rating = GetDecompressRating(info.GlobalTime, info.GlobalFreq, size, info.PackSize, 1);
  else
    rating = GetCompressRating(dictionarySize, info.GlobalTime, info.GlobalFreq, size * info.NumIterations);

  PrintRating(rating, ratingID);
  PrintRating(GetRatingPerUsage(info, rating), rpuID);
  PrintUsage(GetUsage(info), usageID);
}

bool CBenchmarkDialog::OnTimer(WPARAM /* timerID */, LPARAM /* callback */)
{
  PrintTime();
  NWindows::NSynchronization::CCriticalSectionLock lock(_syncInfo.CS);

  TCHAR s[40];
  ConvertUInt64ToString((_syncInfo.ProcessedSize >> 20), s);
  lstrcat(s, kMB);
  SetItemText(IDC_BENCHMARK_SIZE_VALUE, s);

  ConvertUInt64ToString(_syncInfo.NumPasses, s);
  SetItemText(IDC_BENCHMARK_PASSES_VALUE, s);

  /*
  ConvertUInt64ToString(_syncInfo.NumErrors, s);
  SetItemText(IDC_BENCHMARK_ERRORS_VALUE, s);
  */

  {
    UInt32 dicSizeTemp = (UInt32)MyMax(_syncInfo.ProcessedSize, UInt64(1) << 20);
    dicSizeTemp = MyMin(dicSizeTemp, _syncInfo.DictionarySize), 
    PrintResults(dicSizeTemp, 
      _syncInfo.CompressingInfoTemp,
      IDC_BENCHMARK_COMPRESSING_USAGE,
      IDC_BENCHMARK_COMPRESSING_SPEED,
      IDC_BENCHMARK_COMPRESSING_RPU,
      IDC_BENCHMARK_COMPRESSING_RATING);
  }

  {
    PrintResults(
      _syncInfo.DictionarySize, 
      _syncInfo.CompressingInfo,
      IDC_BENCHMARK_COMPRESSING_USAGE2,
      IDC_BENCHMARK_COMPRESSING_SPEED2,
      IDC_BENCHMARK_COMPRESSING_RPU2,
      IDC_BENCHMARK_COMPRESSING_RATING2);
  }

  {
    PrintResults(
      _syncInfo.DictionarySize, 
      _syncInfo.DecompressingInfoTemp,
      IDC_BENCHMARK_DECOMPRESSING_USAGE,
      IDC_BENCHMARK_DECOMPRESSING_SPEED,
      IDC_BENCHMARK_DECOMPRESSING_RPU,
      IDC_BENCHMARK_DECOMPRESSING_RATING,
      true);
  }
  {
    PrintResults(
      _syncInfo.DictionarySize, 
      _syncInfo.DecompressingInfo, 
      IDC_BENCHMARK_DECOMPRESSING_USAGE2,
      IDC_BENCHMARK_DECOMPRESSING_SPEED2,
      IDC_BENCHMARK_DECOMPRESSING_RPU2,
      IDC_BENCHMARK_DECOMPRESSING_RATING2,
      true);
    if (_syncInfo.DecompressingInfo.GlobalTime > 0 &&
        _syncInfo.CompressingInfo.GlobalTime > 0)
    {
      UInt64 comprRating = GetCompressRating(_syncInfo.DictionarySize, 
          _syncInfo.CompressingInfo.GlobalTime, _syncInfo.CompressingInfo.GlobalFreq, _syncInfo.CompressingInfo.UnpackSize);
      UInt64 decomprRating = GetDecompressRating(_syncInfo.DecompressingInfo.GlobalTime, 
          _syncInfo.DecompressingInfo.GlobalFreq, _syncInfo.DecompressingInfo.UnpackSize, 
          _syncInfo.DecompressingInfo.PackSize, 1);
      PrintRating((comprRating + decomprRating) / 2, IDC_BENCHMARK_TOTAL_RATING_VALUE);
      PrintRating((
          GetRatingPerUsage(_syncInfo.CompressingInfo, comprRating) + 
          GetRatingPerUsage(_syncInfo.DecompressingInfo, decomprRating)) / 2, IDC_BENCHMARK_TOTAL_RPU_VALUE);
      PrintUsage((GetUsage(_syncInfo.CompressingInfo) + GetUsage(_syncInfo.DecompressingInfo)) / 2, IDC_BENCHMARK_TOTAL_USAGE_VALUE);
    }
  }
  return true;
}

bool CBenchmarkDialog::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == CBN_SELCHANGE && 
      (itemID == IDC_BENCHMARK_COMBO_DICTIONARY || 
       itemID == IDC_BENCHMARK_COMBO_NUM_THREADS))
  {
    OnChangeSettings();
    return true;
  }
  return CModalDialog::OnCommand(code, itemID, lParam);
}

bool CBenchmarkDialog::OnButtonClicked(int buttonID, HWND buttonHWND) 
{ 
  switch(buttonID)
  {
    case IDC_BUTTON_RESTART:
      OnRestartButton();
      return true;
    case IDC_BUTTON_STOP:
      OnStopButton();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

struct CThreadBenchmark
{
  CProgressSyncInfo *SyncInfo;
  UInt64 _startTime;
  #ifdef EXTERNAL_LZMA
  CCodecs *codecs;
  #endif
  // UInt32 dictionarySize;
  // UInt32 numThreads;

  HRESULT Process();
  HRESULT Result;
  static THREAD_FUNC_DECL MyThreadFunction(void *param)
  {
    ((CThreadBenchmark *)param)->Result = ((CThreadBenchmark *)param)->Process();
    return 0;
  }
};

struct CBenchCallback: public IBenchCallback
{
  UInt32 dictionarySize;
  CProgressSyncInfo *SyncInfo;
  HRESULT SetEncodeResult(const CBenchInfo &info, bool final);
  HRESULT SetDecodeResult(const CBenchInfo &info, bool final);
};

HRESULT CBenchCallback::SetEncodeResult(const CBenchInfo &info, bool final)
{
  NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
  if (SyncInfo->Changed || SyncInfo->Paused || SyncInfo->Stopped)
    return E_ABORT;
  SyncInfo->ProcessedSize = info.UnpackSize;
  if (final && SyncInfo->CompressingInfo.GlobalTime == 0)
  {
    (CBenchInfo&)SyncInfo->CompressingInfo = info;
    if (SyncInfo->CompressingInfo.GlobalTime == 0)
      SyncInfo->CompressingInfo.GlobalTime = 1;
  }
  else
    (CBenchInfo&)SyncInfo->CompressingInfoTemp = info;

  return S_OK;
}

HRESULT CBenchCallback::SetDecodeResult(const CBenchInfo &info, bool final)
{
  NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
  if (SyncInfo->Changed || SyncInfo->Paused || SyncInfo->Stopped)
    return E_ABORT;
  CBenchInfo info2 = info;
  if (info2.NumIterations == 0)
    info2.NumIterations = 1;

  info2.UnpackSize *= info2.NumIterations;
  info2.PackSize *= info2.NumIterations;
  info2.NumIterations = 1;

  if (final && SyncInfo->DecompressingInfo.GlobalTime == 0)
  {
    (CBenchInfo&)SyncInfo->DecompressingInfo = info2;
    if (SyncInfo->DecompressingInfo.GlobalTime == 0)
      SyncInfo->DecompressingInfo.GlobalTime = 1;
  }
  else
    (CBenchInfo&)SyncInfo->DecompressingInfoTemp = info2;
  return S_OK;
}

HRESULT CThreadBenchmark::Process()
{
  try
  {
    SyncInfo->WaitCreating();
    for (;;)
    {
      if (SyncInfo->WasStopped())
        return 0;
      if (SyncInfo->WasPaused())
      {
        Sleep(200);
        continue;
      }
      UInt32 dictionarySize;
      UInt32 numThreads;
      {
        NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
        if (SyncInfo->Stopped || SyncInfo->Paused)
          continue;
        if (SyncInfo->Changed)
          SyncInfo->Init();
        dictionarySize = SyncInfo->DictionarySize;
        numThreads = SyncInfo->NumThreads;
      }
      
      CBenchCallback callback;
      callback.dictionarySize = dictionarySize;
      callback.SyncInfo = SyncInfo;
      HRESULT result;
      try
      {
        result = LzmaBench(
          #ifdef EXTERNAL_LZMA
          codecs,
          #endif
          numThreads, dictionarySize, &callback);
      }
      catch(...)
      {
        result = E_FAIL;
      }

      if (result != S_OK)
      {
        if (result != E_ABORT)
        {
          // SyncInfo->NumErrors++;
          {
            NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
            SyncInfo->Pause();
          }
          CSysString message;
          if (result == S_FALSE)
            message = TEXT("Decoding error");
          else
            message = NError::MyFormatMessage(result);
          MessageBox(0, message, TEXT("7-Zip"), MB_ICONERROR);
        }
      }
      else
      {
        NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
        SyncInfo->NumPasses++;
      }
    }
    // return S_OK;
  }
  catch(CSystemException &e)
  {
    MessageBox(0, NError::MyFormatMessage(e.ErrorCode), TEXT("7-Zip"), MB_ICONERROR);
    return E_FAIL;
  }
  catch(...)
  {
    MyMessageBoxError(0, L"Some error");
    return E_FAIL;
  }
}

HRESULT Benchmark(
  #ifdef EXTERNAL_LZMA
  CCodecs *codecs,
  #endif
  UInt32 numThreads, UInt32 dictionarySize)
{
  CThreadBenchmark benchmarker;
  #ifdef EXTERNAL_LZMA
  benchmarker.codecs = codecs;
  #endif

  CBenchmarkDialog benchmarkDialog;
  benchmarkDialog._syncInfo.DictionarySize = dictionarySize;
  benchmarkDialog._syncInfo.NumThreads = numThreads;

  benchmarker.SyncInfo = &benchmarkDialog._syncInfo;
  NWindows::CThread thread;
  RINOK(thread.Create(CThreadBenchmark::MyThreadFunction, &benchmarker));
  benchmarkDialog.Create(0);
  return thread.Wait();
}
