// BenchmarkDialog.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/MyException.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/System.h"
#include "Windows/Thread.h"

#include "../FileManager/HelpUtils.h"

#include "BenchmarkDialog.h"

using namespace NWindows;

static LPCWSTR kHelpTopic = L"fm/benchmark.htm";

static const UINT_PTR kTimerID = 4;
static const UINT kTimerElapse = 1000;

#ifdef LANG
#include "../FileManager/LangUtils.h"
#endif

using namespace NWindows;

UString HResultToMessage(HRESULT errorCode);

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

const LPCTSTR kProcessingString = TEXT("...");
const LPCTSTR kMB = TEXT(" MB");
const LPCTSTR kMIPS =  TEXT(" MIPS");
const LPCTSTR kKBs = TEXT(" KB/s");

#ifdef UNDER_CE
static const int kMinDicLogSize = 20;
#else
static const int kMinDicLogSize = 21;
#endif
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

  Sync.Init();

  if (TotalMode)
  {
    _consoleEdit.Attach(GetItem(IDC_BENCHMARK2_EDIT));
    LOGFONT f;
    memset(&f, 0, sizeof(f));
    f.lfHeight = 14;
    f.lfWidth = 0;
    f.lfWeight = FW_DONTCARE;
    f.lfCharSet = DEFAULT_CHARSET;
    f.lfOutPrecision = OUT_DEFAULT_PRECIS;
    f.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    f.lfQuality = DEFAULT_QUALITY;

    f.lfPitchAndFamily = FIXED_PITCH;
    MyStringCopy(f.lfFaceName, TEXT(""));
    _font.Create(&f);
    if (_font._font)
      _consoleEdit.SendMessage(WM_SETFONT, (WPARAM)_font._font, TRUE);
  }

  UInt32 numCPUs = NSystem::GetNumberOfProcessors();
  if (numCPUs < 1)
    numCPUs = 1;
  numCPUs = MyMin(numCPUs, (UInt32)(1 << 8));

  if (Sync.NumThreads == (UInt32)-1)
  {
    Sync.NumThreads = numCPUs;
    if (Sync.NumThreads > 1)
      Sync.NumThreads &= ~1;
  }
  m_NumThreads.Attach(GetItem(IDC_BENCHMARK_COMBO_NUM_THREADS));
  int cur = 0;
  for (UInt32 num = 1; num <= numCPUs * 2;)
  {
    TCHAR s[16];
    ConvertUInt32ToString(num, s);
    int index = (int)m_NumThreads.AddString(s);
    m_NumThreads.SetItemData(index, num);
    if (num <= Sync.NumThreads)
      cur = index;
    if (num > 1)
      num++;
    num++;
  }
  m_NumThreads.SetCurSel(cur);
  Sync.NumThreads = GetNumberOfThreads();

  m_Dictionary.Attach(GetItem(IDC_BENCHMARK_COMBO_DICTIONARY));
  cur = 0;
  UInt64 ramSize = NSystem::GetRamSize();
  
  #ifdef UNDER_CE
  const UInt32 kNormalizedCeSize = (16 << 20);
  if (ramSize > kNormalizedCeSize && ramSize < (33 << 20))
    ramSize = kNormalizedCeSize;
  #endif

  if (Sync.DictionarySize == (UInt32)-1)
  {
    int dicSizeLog;
    for (dicSizeLog = 25; dicSizeLog > kBenchMinDicLogSize; dicSizeLog--)
      if (GetBenchMemoryUsage(Sync.NumThreads, ((UInt32)1 << dicSizeLog)) + (8 << 20) <= ramSize)
        break;
    Sync.DictionarySize = (1 << dicSizeLog);
  }
  if (Sync.DictionarySize < kMinDicSize)
    Sync.DictionarySize = kMinDicSize;
  if (Sync.DictionarySize > kMaxDicSize)
    Sync.DictionarySize = kMaxDicSize;

  for (int i = kMinDicLogSize; i <= 30; i++)
    for (int j = 0; j < 2; j++)
    {
      UInt32 dictionary = (1 << i) + (j << (i - 1));
      if (dictionary > kMaxDicSize)
        continue;
      TCHAR s[16];
      ConvertUInt32ToString((dictionary >> 20), s);
      lstrcat(s, kMB);
      int index = (int)m_Dictionary.AddString(s);
      m_Dictionary.SetItemData(index, dictionary);
      if (dictionary <= Sync.DictionarySize)
        cur = index;
    }
  m_Dictionary.SetCurSel(cur);

  OnChangeSettings();

  Sync._startEvent.Set();
  _timer = SetTimer(kTimerID, kTimerElapse);

  if (TotalMode)
    NormalizeSize(true);
  else
    NormalizePosition();
  return CModalDialog::OnInit();
}

bool CBenchmarkDialog::OnSize(WPARAM /* wParam */, int xSize, int ySize)
{
  if (!TotalMode)
    return false;
  int mx, my;
  GetMargins(8, mx, my);
  int bx1, bx2, by;
  GetItemSizes(IDCANCEL, bx1, by);
  GetItemSizes(IDHELP, bx2, by);
  int y = ySize - my - by;
  int x = xSize - mx - bx1;

  InvalidateRect(NULL);

  MoveItem(IDCANCEL, x, y, bx1, by);
  MoveItem(IDHELP, x - mx - bx2, y, bx2, by);
  if (_consoleEdit)
  {
    int yPos = ySize - my - by;
    RECT rect;
    GetClientRectOfItem(IDC_BENCHMARK2_EDIT, rect);
    int y = rect.top;
    int ySize2 = yPos - my - y;
    const int kMinYSize = 20;
    int xx = xSize - mx * 2;
    if (ySize2 < kMinYSize)
    {
      ySize2 = kMinYSize;
    }
    _consoleEdit.Move(mx, y, xx, ySize2);
  }
  return false;
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
  ConvertUInt32ToString(NSystem::GetNumberOfProcessors(), s + 2);
  SetItemText(IDC_BENCHMARK_HARDWARE_THREADS, s);
  for (int i = 0; i < sizeof(g_IDs) / sizeof(g_IDs[0]); i++)
    SetItemText(g_IDs[i], kProcessingString);
  _startTime = GetTickCount();
  PrintTime();
  NWindows::NSynchronization::CCriticalSectionLock lock(Sync.CS);
  Sync.Init();
  Sync.DictionarySize = dictionary;
  Sync.Changed = true;
  Sync.NumThreads = GetNumberOfThreads();
}

void CBenchmarkDialog::OnRestartButton()
{
  OnChangeSettings();
}

void CBenchmarkDialog::OnStopButton()
{
  EnableItem(IDC_BUTTON_STOP, false);
  Sync.Pause();
}

void CBenchmarkDialog::OnHelp()
{
  ShowHelpWindow(NULL, kHelpTopic);
}

void CBenchmarkDialog::OnCancel()
{
  Sync.Stop();
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
  if (elapsedSec != 0 && Sync.WasPaused())
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
  PrintRating(info.GetRatingPerUsage(rating), rpuID);
  PrintUsage(info.GetUsage(), usageID);
}

bool CBenchmarkDialog::OnTimer(WPARAM /* timerID */, LPARAM /* callback */)
{
  bool printTime = true;
  if (TotalMode)
  {
    if (Sync.WasStopped())
      printTime = false;
  }
  if (printTime)
    PrintTime();
  NWindows::NSynchronization::CCriticalSectionLock lock(Sync.CS);

  if (TotalMode)
  {
    if (Sync.TextWasChanged)
    {
      _consoleEdit.SetText(GetSystemString(Sync.Text));
      Sync.TextWasChanged = false;
    }
    return true;
  }

  TCHAR s[40];
  ConvertUInt64ToString((Sync.ProcessedSize >> 20), s);
  lstrcat(s, kMB);
  SetItemText(IDC_BENCHMARK_SIZE_VALUE, s);

  ConvertUInt64ToString(Sync.NumPasses, s);
  SetItemText(IDC_BENCHMARK_PASSES_VALUE, s);

  /*
  ConvertUInt64ToString(Sync.NumErrors, s);
  SetItemText(IDC_BENCHMARK_ERRORS_VALUE, s);
  */

  {
    UInt32 dicSizeTemp = (UInt32)MyMax(Sync.ProcessedSize, UInt64(1) << 20);
    dicSizeTemp = MyMin(dicSizeTemp, Sync.DictionarySize),
    PrintResults(dicSizeTemp,
      Sync.CompressingInfoTemp,
      IDC_BENCHMARK_COMPRESSING_USAGE,
      IDC_BENCHMARK_COMPRESSING_SPEED,
      IDC_BENCHMARK_COMPRESSING_RPU,
      IDC_BENCHMARK_COMPRESSING_RATING);
  }

  {
    PrintResults(
      Sync.DictionarySize,
      Sync.CompressingInfo,
      IDC_BENCHMARK_COMPRESSING_USAGE2,
      IDC_BENCHMARK_COMPRESSING_SPEED2,
      IDC_BENCHMARK_COMPRESSING_RPU2,
      IDC_BENCHMARK_COMPRESSING_RATING2);
  }

  {
    PrintResults(
      Sync.DictionarySize,
      Sync.DecompressingInfoTemp,
      IDC_BENCHMARK_DECOMPRESSING_USAGE,
      IDC_BENCHMARK_DECOMPRESSING_SPEED,
      IDC_BENCHMARK_DECOMPRESSING_RPU,
      IDC_BENCHMARK_DECOMPRESSING_RATING,
      true);
  }
  {
    PrintResults(
      Sync.DictionarySize,
      Sync.DecompressingInfo,
      IDC_BENCHMARK_DECOMPRESSING_USAGE2,
      IDC_BENCHMARK_DECOMPRESSING_SPEED2,
      IDC_BENCHMARK_DECOMPRESSING_RPU2,
      IDC_BENCHMARK_DECOMPRESSING_RATING2,
      true);
    if (Sync.DecompressingInfo.GlobalTime > 0 &&
        Sync.CompressingInfo.GlobalTime > 0)
    {
      UInt64 comprRating = GetCompressRating(Sync.DictionarySize,
          Sync.CompressingInfo.GlobalTime, Sync.CompressingInfo.GlobalFreq, Sync.CompressingInfo.UnpackSize);
      UInt64 decomprRating = GetDecompressRating(Sync.DecompressingInfo.GlobalTime,
          Sync.DecompressingInfo.GlobalFreq, Sync.DecompressingInfo.UnpackSize,
          Sync.DecompressingInfo.PackSize, 1);
      PrintRating((comprRating + decomprRating) / 2, IDC_BENCHMARK_TOTAL_RATING_VALUE);
      PrintRating((
          Sync.CompressingInfo.GetRatingPerUsage(comprRating) +
          Sync.DecompressingInfo.GetRatingPerUsage(decomprRating)) / 2, IDC_BENCHMARK_TOTAL_RPU_VALUE);
      PrintUsage(
        (Sync.CompressingInfo.GetUsage() +
         Sync.DecompressingInfo.GetUsage()) / 2, IDC_BENCHMARK_TOTAL_USAGE_VALUE);
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
  CBenchmarkDialog *BenchmarkDialog;
  DECL_EXTERNAL_CODECS_VARS
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
  CProgressSyncInfo *Sync;
  HRESULT SetEncodeResult(const CBenchInfo &info, bool final);
  HRESULT SetDecodeResult(const CBenchInfo &info, bool final);
};

HRESULT CBenchCallback::SetEncodeResult(const CBenchInfo &info, bool final)
{
  NSynchronization::CCriticalSectionLock lock(Sync->CS);
  if (Sync->Changed || Sync->Paused || Sync->Stopped)
    return E_ABORT;
  Sync->ProcessedSize = info.UnpackSize;
  if (final && Sync->CompressingInfo.GlobalTime == 0)
  {
    (CBenchInfo&)Sync->CompressingInfo = info;
    if (Sync->CompressingInfo.GlobalTime == 0)
      Sync->CompressingInfo.GlobalTime = 1;
  }
  else
    (CBenchInfo&)Sync->CompressingInfoTemp = info;

  return S_OK;
}

HRESULT CBenchCallback::SetDecodeResult(const CBenchInfo &info, bool final)
{
  NSynchronization::CCriticalSectionLock lock(Sync->CS);
  if (Sync->Changed || Sync->Paused || Sync->Stopped)
    return E_ABORT;
  CBenchInfo info2 = info;
  if (info2.NumIterations == 0)
    info2.NumIterations = 1;

  info2.UnpackSize *= info2.NumIterations;
  info2.PackSize *= info2.NumIterations;
  info2.NumIterations = 1;

  if (final && Sync->DecompressingInfo.GlobalTime == 0)
  {
    (CBenchInfo&)Sync->DecompressingInfo = info2;
    if (Sync->DecompressingInfo.GlobalTime == 0)
      Sync->DecompressingInfo.GlobalTime = 1;
  }
  else
    (CBenchInfo&)Sync->DecompressingInfoTemp = info2;
  return S_OK;
}

struct CBenchCallback2: public IBenchPrintCallback
{
  CProgressSyncInfo *Sync;

  void Print(const char *s);
  void NewLine();
  HRESULT CheckBreak();
};

void CBenchCallback2::Print(const char *s)
{
  NSynchronization::CCriticalSectionLock lock(Sync->CS);
  Sync->Text += s;
  Sync->TextWasChanged = true;
}

void CBenchCallback2::NewLine()
{
  Print("\xD\n");
}

HRESULT CBenchCallback2::CheckBreak()
{
  if (Sync->Changed || Sync->Paused || Sync->Stopped)
    return E_ABORT;
  return S_OK;
}


HRESULT CThreadBenchmark::Process()
{
  CProgressSyncInfo &sync = BenchmarkDialog->Sync;
  sync.WaitCreating();
  try
  {
    for (;;)
    {
      if (sync.WasStopped())
        return 0;
      if (sync.WasPaused())
      {
        Sleep(200);
        continue;
      }
      UInt32 dictionarySize;
      UInt32 numThreads;
      {
        NSynchronization::CCriticalSectionLock lock(sync.CS);
        if (sync.Stopped || sync.Paused)
          continue;
        if (sync.Changed)
          sync.Init();
        dictionarySize = sync.DictionarySize;
        numThreads = sync.NumThreads;
      }
      
      CBenchCallback callback;
      callback.dictionarySize = dictionarySize;
      callback.Sync = &sync;
      CBenchCallback2 callback2;
      callback2.Sync = &sync;
      HRESULT result;
      try
      {
        CObjectVector<CProperty> props;
        if (BenchmarkDialog->TotalMode)
        {
          props = BenchmarkDialog->Props;
        }
        else
        {
          {
            CProperty prop;
            prop.Name = L"mt";
            wchar_t s[16];
            ConvertUInt32ToString(numThreads, s);
            prop.Value = s;
            props.Add(prop);
          }
          {
            CProperty prop;
            prop.Name = L"d";
            wchar_t s[16];
            ConvertUInt32ToString(dictionarySize, s);
            prop.Name += s;
            prop.Name += 'b';
            props.Add(prop);
          }
        }
        result = Bench(EXTERNAL_CODECS_VARS
            BenchmarkDialog->TotalMode ? &callback2 : NULL,
            BenchmarkDialog->TotalMode ? NULL : &callback,
            props, 1, false);
        if (BenchmarkDialog->TotalMode)
        {
          sync.Stop();
        }
      }
      catch(...)
      {
        result = E_FAIL;
      }

      if (result != S_OK)
      {
        if (result != E_ABORT)
        {
          // sync.NumErrors++;
          {
            NSynchronization::CCriticalSectionLock lock(sync.CS);
            sync.Pause();
          }
          UString message;
          if (result == S_FALSE)
            message = L"Decoding error";
          else if (result == CLASS_E_CLASSNOTAVAILABLE)
            message = L"Can't find 7z.dll";
          else
            message = HResultToMessage(result);
          BenchmarkDialog->MessageBoxError(message);
        }
      }
      else
      {
        NSynchronization::CCriticalSectionLock lock(sync.CS);
        sync.NumPasses++;
      }
    }
    // return S_OK;
  }
  catch(CSystemException &e)
  {
    BenchmarkDialog->MessageBoxError(HResultToMessage(e.ErrorCode));
    return E_FAIL;
  }
  catch(...)
  {
    BenchmarkDialog->MessageBoxError(HResultToMessage(E_FAIL));
    return E_FAIL;
  }
}

HRESULT Benchmark(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CObjectVector<CProperty> props, HWND hwndParent)
{
  CThreadBenchmark benchmarker;
  #ifdef EXTERNAL_CODECS
  benchmarker._codecsInfo = codecsInfo;
  benchmarker._externalCodecs = *externalCodecs;
  #endif

  CBenchmarkDialog bd;
  bd.Props = props;
  bd.TotalMode = false;
  for (int i = 0; i < props.Size(); i++)
  {
    const CProperty &prop = props[i];
    if (prop.Name.CompareNoCase(L"m") == 0 && prop.Value == L"*")
      bd.TotalMode = true;
  }
  bd.Sync.DictionarySize = (UInt32)-1;
  bd.Sync.NumThreads = (UInt32)-1;
  benchmarker.BenchmarkDialog = &bd;

  NWindows::CThread thread;
  RINOK(thread.Create(CThreadBenchmark::MyThreadFunction, &benchmarker));
  bd.Create(hwndParent);
  return thread.Wait();
}
