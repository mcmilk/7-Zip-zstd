// BenchmarkDialog.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringToInt.h"
#include "Common/Exception.h"
#include "Windows/Thread.h"
#include "Windows/PropVariant.h"
#include "Windows/Error.h"
#include "Windows/DLL.h"
#include "Windows/FileFind.h"
#include "../../ProgramLocation.h"
#include "../../HelpUtils.h"
#include "../../../Common/StreamObjects.h"
#include "resource.h"
#include "BenchmarkDialog.h"
#include "Common/CRC.h"

using namespace NWindows;

static LPCWSTR kHelpTopic = L"fm/benchmark.htm";

static const UINT_PTR kTimerID = 4;
static const UINT kTimerElapse = 1000;

static const UINT32 kAdditionalSize = (6 << 20);
static const UINT32 kCompressedAdditionalSize = (1 << 10);
static const int kSubBits = 8;

#ifdef LANG        
#include "../../LangUtils.h"
#endif

using namespace NWindows;

#ifdef LANG        
static CIDLangPair kIDLangPairs[] = 
{
  { IDC_BENCHMARK_DICTIONARY, 0x02000D0C },
  { IDC_BENCHMARK_MEMORY, 0x03080001 },
  { IDC_BENCHMARK_MULTITHREADING, 0x02000D09 },
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
  { IDC_BENCHMARK_ERRORS, 0x0308000A },
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

UINT64 GetTimeCount()
{
  return GetTickCount();
  /*
  LARGE_INTEGER value;
  if (::QueryPerformanceCounter(&value))
    return value.QuadPart;
  return GetTickCount();
  */
}

UINT64 GetFreq()
{
  return 1000;
  /*
  LARGE_INTEGER value;
  if (::QueryPerformanceFrequency(&value))
    return value.QuadPart;
  return 1000;
  */
}

class CRandomGenerator
{
  UINT32 A1;
  UINT32 A2;
public:
  CRandomGenerator() { Init(); }
  void Init() { A1 = 362436069; A2 = 521288629;}
  UINT32 GetRnd() 
  {
    return 
      ((A1 = 36969 * (A1 & 0xffff) + (A1 >> 16)) << 16) ^
      ((A2 = 18000 * (A2 & 0xffff) + (A2 >> 16)) );
  }
};

class CBitRandomGenerator
{
  CRandomGenerator RG;
  UINT32 Value;
  int NumBits;
public:
  void Init()
  {
    Value = 0;
    NumBits = 0;
  }
  UINT32 GetRnd(int numBits) 
  {
    if (NumBits > numBits)
    {
      UINT32 result = Value & ((1 << numBits) - 1);
      Value >>= numBits;
      NumBits -= numBits;
      return result;
    }
    numBits -= NumBits;
    UINT32 result = (Value << numBits);
    Value = RG.GetRnd();
    result |= Value & ((1 << numBits) - 1);
    Value >>= numBits;
    NumBits = 32 - numBits;
    return result;
  }
};

class CBenchRandomGenerator
{
  CBitRandomGenerator RG;
  UINT32 Pos;
public:
  UINT32 BufferSize;
  BYTE *Buffer;
  CBenchRandomGenerator(): Buffer(0) {} 
  ~CBenchRandomGenerator() { delete []Buffer; }
  void Init() { RG.Init(); }
  void Set(UINT32 bufferSize) 
  {
    delete []Buffer;
    Buffer = 0;
    Buffer = new BYTE[bufferSize];
    Pos = 0;
    BufferSize = bufferSize;
  }
  UINT32 GetRndBit() { return RG.GetRnd(1); }
  /*
  UINT32 GetLogRand(int maxLen)
  {
    UINT32 len = GetRnd() % (maxLen + 1);
    return GetRnd() & ((1 << len) - 1);
  }
  */
  UINT32 GetLogRandBits(int numBits)
  {
    UINT32 len = RG.GetRnd(numBits);
    return RG.GetRnd(len);
  }
  UINT32 GetOffset()
  {
    if (GetRndBit() == 0)
      return GetLogRandBits(4);
    return (GetLogRandBits(4) << 10) | RG.GetRnd(10);
  }
  UINT32 GetLen()
  {
    if (GetRndBit() == 0)
      return RG.GetRnd(2);
    if (GetRndBit() == 0)
      return 4 + RG.GetRnd(3);
    return 12 + RG.GetRnd(4);
  }
  void Generate()
  {
    while(Pos < BufferSize)
    {
      if (GetRndBit() == 0 || Pos < 1)
        Buffer[Pos++] = BYTE(RG.GetRnd(8));
      else
      {
        UINT32 offset = GetOffset();
        while (offset >= Pos)
          offset >>= 1;
        offset += 1;
        UINT32 len = 2 + GetLen();
        for (UINT32 i = 0; i < len && Pos < BufferSize; i++, Pos++)
          Buffer[Pos] = Buffer[Pos - offset];
      }
    }
  }
};

const LPCTSTR kProcessingString = TEXT("...");
const LPCTSTR kMB = TEXT(" MB");
const LPCTSTR kMIPS =  TEXT(" MIPS");
const LPCTSTR kKBs = TEXT(" KB/s");

bool CBenchmarkDialog::OnInit() 
{
  #ifdef LANG        
  LangSetWindowText(HWND(*this), 0x03080000);
  LangSetDlgItemsText(HWND(*this), kIDLangPairs, sizeof(kIDLangPairs) / sizeof(kIDLangPairs[0]));
  #endif

  m_Dictionary.Attach(GetItem(IDC_BENCHMARK_COMBO_DICTIONARY));
  for (int i = kNumBenchDictionaryBitsStart; i < 28; i++)
    for (int j = 0; j < 2; j++)
    {
      UINT32 dictionary = (1 << i) + (j << (i - 1));
      TCHAR s[40];
      ConvertUINT64ToString((dictionary >> 20), s);
      lstrcat(s, kMB);
      int index = m_Dictionary.AddString(s);
      m_Dictionary.SetItemData(index, dictionary);
    }
  m_Dictionary.SetCurSel(0);
  OnChangeSettings();

  _syncInfo.Init();
  _syncInfo.InitSettings();
 
  _syncInfo._startEvent.Set();
  _timer = SetTimer(kTimerID, kTimerElapse);
	return CModalDialog::OnInit();
}

static UINT64 GetLZMAUsage(UINT32 dictionary)
  { return ((UINT64)dictionary * 19 / 2) + (8 << 20); }

static UINT64 GetMemoryUsage(UINT32 dictionary)
{
  const UINT32 kBufferSize = dictionary + kAdditionalSize;
  const UINT32 kCompressedBufferSize = (kBufferSize / 2) + kCompressedAdditionalSize;
  return kBufferSize + kCompressedBufferSize +
    GetLZMAUsage(dictionary) + dictionary + (1 << 20);
}

UINT32 CBenchmarkDialog::OnChangeDictionary()
{
  UINT64 dictionary = m_Dictionary.GetItemData(m_Dictionary.GetCurSel());
  UINT64 memUsage = GetMemoryUsage(dictionary);
  memUsage = (memUsage + (1 << 20) - 1) >> 20;
  TCHAR s[40];
  ConvertUINT64ToString(memUsage, s);
  lstrcat(s, kMB);
  SetItemText(IDC_BENCHMARK_MEMORY_VALUE, s);
  return dictionary;
}

void CBenchmarkDialog::OnChangeSettings()
{
  EnableItem(IDC_BUTTON_STOP, true);
  UINT32 dictionary = OnChangeDictionary();
  SetItemText(IDC_BENCHMARK_COMPRESSING_SPEED, kProcessingString);
  SetItemText(IDC_BENCHMARK_COMPRESSING_SPEED2, kProcessingString);
  SetItemText(IDC_BENCHMARK_COMPRESSING_RATING, kProcessingString);
  SetItemText(IDC_BENCHMARK_COMPRESSING_RATING2, kProcessingString);
  SetItemText(IDC_BENCHMARK_DECOMPRESSING_SPEED, kProcessingString);
  SetItemText(IDC_BENCHMARK_DECOMPRESSING_SPEED2, kProcessingString);
  SetItemText(IDC_BENCHMARK_DECOMPRESSING_RATING, kProcessingString);
  SetItemText(IDC_BENCHMARK_DECOMPRESSING_RATING2, kProcessingString);
  SetItemText(IDC_BENCHMARK_TOTAL_RATING_VALUE, kProcessingString);
  _startTime = GetTickCount();
  PrintTime();
  NWindows::NSynchronization::CCriticalSectionLock lock(_syncInfo.CS);
  _syncInfo.Init();
  _syncInfo.DictionarySize = dictionary;
  _syncInfo.Changed = true;
  _syncInfo.MultiThread = IsButtonCheckedBool(IDC_BENCHMARK_MULTITHREADING);
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

static void GetTimeString(UINT64 timeValue, TCHAR *s)
{
  wsprintf(s, TEXT("%02d:%02d:%02d"), 
      UINT32(timeValue / 3600),
      UINT32((timeValue / 60) % 60), 
      UINT32(timeValue % 60));
}

void CBenchmarkDialog::PrintTime()
{
  UINT32 curTime = ::GetTickCount();
  UINT32 elapsedTime = (curTime - _startTime);
  UINT32 elapsedSec = elapsedTime / 1000;
  TCHAR s[40];
  GetTimeString(elapsedSec, s);
  SetItemText(IDC_BENCHMARK_ELAPSED_VALUE, s);
}

static UINT32 GetLogSize(UINT32 size)
{
  for (int i = kSubBits; i < 32; i++)
    for (UINT32 j = 0; j < (1 << kSubBits); j++)
      if (size <= (((UINT32)1) << i) + (j << (i - kSubBits)))
        return (i << kSubBits) + j;
  return (32 << kSubBits);
}

static UINT64 GetCompressRating(UINT32 dictionarySize, 
    UINT64 elapsedTime, UINT64 size)
{
  if (elapsedTime == 0)
    elapsedTime = 1;
  UINT64 t = GetLogSize(dictionarySize) - (19 << kSubBits);
  UINT64 numCommandsForOne = 2000 + ((t * t * 68) >> (2 * kSubBits));
  UINT64 numCommands = (UINT64)(size) * numCommandsForOne;
  return numCommands * GetFreq() / elapsedTime;
}

static UINT64 GetDecompressRating(UINT64 elapsedTime, 
    UINT64 outSize, UINT64 inSize)
{
  if (elapsedTime == 0)
    elapsedTime = 1;
  UINT64 numCommands = inSize * 250 + outSize * 20;
  return numCommands * GetFreq() / elapsedTime;
}

static UINT64 GetTotalRating(
    UINT32 dictionarySize, 
    UINT64 elapsedTimeEn, UINT64 sizeEn,
    UINT64 elapsedTimeDe, 
    UINT64 inSizeDe, UINT64 outSizeDe)
{
  return (GetCompressRating(dictionarySize, elapsedTimeEn, sizeEn) + 
    GetDecompressRating(elapsedTimeDe, inSizeDe, outSizeDe)) / 2;
}

void CBenchmarkDialog::PrintRating(UINT64 rating, UINT controlID)
{
  TCHAR s[40];
  ConvertUINT64ToString(rating / 1000000, s);
  lstrcat(s, kMIPS);
  SetItemText(controlID, s);
}

void CBenchmarkDialog::PrintResults(
    UINT32 dictionarySize,
    UINT64 elapsedTime, 
    UINT64 size, UINT speedID, UINT ratingID,
    bool decompressMode, UINT64 secondSize)
{
  TCHAR s[40];
  UINT64 speed = size * GetFreq() / elapsedTime;
  ConvertUINT64ToString(speed / 1024, s);
  lstrcat(s, kKBs);
  SetItemText(speedID, s);
  UINT64 rating;
  if (decompressMode)
    rating = GetDecompressRating(elapsedTime, size, secondSize);
  else
    rating = GetCompressRating(dictionarySize, elapsedTime, size);
  PrintRating(rating, ratingID);
}

bool CBenchmarkDialog::OnTimer(WPARAM timerID, LPARAM callback)
{
  PrintTime();
  NWindows::NSynchronization::CCriticalSectionLock lock(_syncInfo.CS);

  TCHAR s[40];
  ConvertUINT64ToString((_syncInfo.ProcessedSize >> 20), s);
  lstrcat(s, kMB);
  SetItemText(IDC_BENCHMARK_SIZE_VALUE, s);

  ConvertUINT64ToString(_syncInfo.NumPasses, s);
  SetItemText(IDC_BENCHMARK_PASSES_VALUE, s);

  ConvertUINT64ToString(_syncInfo.NumErrors, s);
  SetItemText(IDC_BENCHMARK_ERRORS_VALUE, s);

  UINT64 elapsedTime = _syncInfo.CompressingInfoTemp.Time;
  if (elapsedTime >= 1)
  {
    UINT32 dicSizeTemp = (UINT32)MyMax(_syncInfo.ProcessedSize, UINT64(1) << 20);
    dicSizeTemp = MyMin(dicSizeTemp, _syncInfo.DictionarySize), 
    PrintResults(dicSizeTemp, elapsedTime, 
      _syncInfo.CompressingInfoTemp.InSize, 
      IDC_BENCHMARK_COMPRESSING_SPEED,
      IDC_BENCHMARK_COMPRESSING_RATING);
  }

  if (_syncInfo.CompressingInfo.Time >= 1)
  {
    PrintResults(
      _syncInfo.DictionarySize, 
      _syncInfo.CompressingInfo.Time, 
      _syncInfo.CompressingInfo.InSize, 
      IDC_BENCHMARK_COMPRESSING_SPEED2,
      IDC_BENCHMARK_COMPRESSING_RATING2);
  }

  if (_syncInfo.DecompressingInfoTemp.Time >= 1)
  {
    PrintResults(
      _syncInfo.DictionarySize, 
      _syncInfo.DecompressingInfoTemp.Time, 
      _syncInfo.DecompressingInfoTemp.OutSize, 
      IDC_BENCHMARK_DECOMPRESSING_SPEED,
      IDC_BENCHMARK_DECOMPRESSING_RATING,
      true,
      _syncInfo.DecompressingInfoTemp.InSize);
  }
  if (_syncInfo.DecompressingInfo.Time >= 1)
  {
    PrintResults(
      _syncInfo.DictionarySize, 
      _syncInfo.DecompressingInfo.Time, 
      _syncInfo.DecompressingInfo.OutSize, 
      IDC_BENCHMARK_DECOMPRESSING_SPEED2,
      IDC_BENCHMARK_DECOMPRESSING_RATING2,
      true,
      _syncInfo.DecompressingInfo.InSize);
    if (_syncInfo.CompressingInfo.Time >= 1)
    {
      PrintRating(GetTotalRating(
        _syncInfo.DictionarySize, 
        _syncInfo.CompressingInfo.Time, 
        _syncInfo.CompressingInfo.InSize, 
        _syncInfo.DecompressingInfo.Time,
        _syncInfo.DecompressingInfo.OutSize, 
        _syncInfo.DecompressingInfo.InSize), 
        IDC_BENCHMARK_TOTAL_RATING_VALUE);
    }
  }
  return true;
}

bool CBenchmarkDialog::OnCommand(int code, int itemID, LPARAM lParam)
{
  if (code == CBN_SELCHANGE && itemID == IDC_BENCHMARK_COMBO_DICTIONARY)
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
    case IDC_BENCHMARK_MULTITHREADING:
      OnChangeSettings();
      return true;
  }
  return CModalDialog::OnButtonClicked(buttonID, buttonHWND);
}

class CBenchmarkInStream: 
  public ISequentialInStream,
  public CMyUnknownImp
{
  const BYTE *Data;
  UINT32 Pos;
  UINT32 Size;
public:
  MY_UNKNOWN_IMP
  void Init(const BYTE *data, UINT32 size)
  {
    Data = data;
    Size = size;
    Pos = 0;
  }
  STDMETHOD(Read)(void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(ReadPart)(void *data, UINT32 size, UINT32 *processedSize);
};

STDMETHODIMP CBenchmarkInStream::Read(void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 remain = Size - Pos;
  if (size > remain)
    size = remain;
  for (UINT32 i = 0; i < size; i++)
  {
    ((BYTE *)data)[i] = Data[Pos + i];
  }
  Pos += size;
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
STDMETHODIMP CBenchmarkInStream::ReadPart(void *data, UINT32 size, UINT32 *processedSize)
{
  return Read(data, size, processedSize);
}

class CBenchmarkOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  UINT32 BufferSize;
public:
  UINT32 Pos;
  BYTE *Buffer;
  CBenchmarkOutStream(): Buffer(0) {} 
  ~CBenchmarkOutStream() { delete []Buffer; }
  void Init(UINT32 bufferSize) 
  {
    delete []Buffer;
    Buffer = 0;
    Buffer = new BYTE[bufferSize];
    Pos = 0;
    BufferSize = bufferSize;
  }
  MY_UNKNOWN_IMP
  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

STDMETHODIMP CBenchmarkOutStream::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  UINT32 i;
  for (i = 0; i < size && Pos < BufferSize; i++)
    Buffer[Pos++] = ((const BYTE *)data)[i];
  if(processedSize != NULL)
    *processedSize = i;
  if (i != size)
  {
    MessageBoxW(0, L"Buffer is full", L"7-zip error", 0);
    return E_FAIL;
  }
  return S_OK;
}
  
STDMETHODIMP CBenchmarkOutStream::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}

class CCompareOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  CCRC CRC;
  MY_UNKNOWN_IMP
  void Init() { CRC.Init(); }
  STDMETHOD(Write)(const void *data, UINT32 size, UINT32 *processedSize);
  STDMETHOD(WritePart)(const void *data, UINT32 size, UINT32 *processedSize);
};

STDMETHODIMP CCompareOutStream::Write(const void *data, UINT32 size, UINT32 *processedSize)
{
  CRC.Update(data, size);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
STDMETHODIMP CCompareOutStream::WritePart(const void *data, UINT32 size, UINT32 *processedSize)
{
  return Write(data, size, processedSize);
}

typedef UINT32 (WINAPI * CreateObjectPointer)(const GUID *clsID, 
    const GUID *interfaceID, void **outObject);

struct CDecoderProgressInfo:
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  CProgressSyncInfo *SyncInfo;
  MY_UNKNOWN_IMP
  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};

STDMETHODIMP CDecoderProgressInfo::SetRatioInfo(
    const UINT64 *inSize, const UINT64 *outSize)
{
  NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
  if (SyncInfo->Changed)
    return E_ABORT;
  return S_OK;
}

struct CThreadBenchmark:
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  CProgressSyncInfo *SyncInfo;
  UINT64 _startTime;
  UINT64 _approvedStart;
  NDLL::CLibrary Library;
  CMyComPtr<ICompressCoder> Encoder;
  CMyComPtr<ICompressCoder> Decoder;
  DWORD Process();
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    return ((CThreadBenchmark *)param)->Process();
  }
  MY_UNKNOWN_IMP
  STDMETHOD(SetRatioInfo)(const UINT64 *inSize, const UINT64 *outSize);
};

DWORD CThreadBenchmark::Process()
{
  try
  {
  SyncInfo->WaitCreating();
  CBenchRandomGenerator randomGenerator;
  randomGenerator.Init();
  CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
  Encoder.QueryInterface(IID_ICompressWriteCoderProperties, 
      &writeCoderProperties);
  CMyComPtr<ICompressSetDecoderProperties> compressSetDecoderProperties;
  Decoder.QueryInterface(
        IID_ICompressSetDecoderProperties, &compressSetDecoderProperties);
  CSequentialOutStreamImp *propStreamSpec = 0;
  CMyComPtr<ISequentialOutStream> propStream;
  if (writeCoderProperties != NULL)
  {
    propStreamSpec = new CSequentialOutStreamImp;
    propStream = propStreamSpec;
  }

  CMyComPtr<ICompressSetCoderProperties> setCoderProperties;
  Encoder.QueryInterface(IID_ICompressSetCoderProperties, 
      &setCoderProperties);

  CBenchmarkInStream *propDecoderStreamSpec = 0;
  CMyComPtr<ISequentialInStream> propDecoderStream;
  if (compressSetDecoderProperties)
  {
    propDecoderStreamSpec = new CBenchmarkInStream;
    propDecoderStream = propDecoderStreamSpec;
  }

  CDecoderProgressInfo *decoderProgressInfoSpec = new 
      CDecoderProgressInfo;
  CMyComPtr<ICompressProgressInfo> decoderProgress = decoderProgressInfoSpec;
  decoderProgressInfoSpec->SyncInfo = SyncInfo;
    
  while(true)
  {
    if (SyncInfo->WasStopped())
      return 0;
    if (SyncInfo->WasPaused())
    {
      Sleep(200);
      continue;
    }
    UINT32 dictionarySize;
    bool multiThread;
    {
      NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
      dictionarySize = SyncInfo->DictionarySize;
      multiThread = SyncInfo->MultiThread;
      if (SyncInfo->Changed)
        SyncInfo->Init();
    }

    const UINT32 kBufferSize = dictionarySize + kAdditionalSize;
    const UINT32 kCompressedBufferSize = (kBufferSize / 2) + kCompressedAdditionalSize;

    if (setCoderProperties)
    {
      PROPID propIDs[] = 
      {
        NCoderPropID::kDictionarySize,
        NCoderPropID::kMultiThread
      };
      const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
      NWindows::NCOM::CPropVariant properties[kNumProps];
      properties[0] = UINT32(dictionarySize);
      properties[1] = bool(multiThread);
      RINOK(setCoderProperties->SetCoderProperties(propIDs,
          properties, kNumProps));
    }
      
    if (propStream)
    {
      propStreamSpec->Init();
      writeCoderProperties->WriteCoderProperties(propStream);
    }

    randomGenerator.Set(kBufferSize);
    randomGenerator.Generate();
    CCRC crc;

    // randomGenerator.BufferSize
    crc.Update(randomGenerator.Buffer, randomGenerator.BufferSize);

    {
      NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
      if (SyncInfo->Changed)
        continue;
    }
    
    CBenchmarkInStream *inStreamSpec = new CBenchmarkInStream;
    inStreamSpec->Init(randomGenerator.Buffer, randomGenerator.BufferSize);
    CMyComPtr<ISequentialInStream> inStream = inStreamSpec;
    CBenchmarkOutStream *outStreamSpec = new CBenchmarkOutStream;
    outStreamSpec->Init(kCompressedBufferSize);
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    _approvedStart = dictionarySize;
    _startTime = ::GetTimeCount();
    HRESULT result = Encoder->Code(inStream, outStream, 0, 0, this);
    UINT64 tickCount = ::GetTimeCount() - _startTime;
    UINT32 compressedSize = outStreamSpec->Pos;
    {
      NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
      if (result == S_OK)
      {
        if (SyncInfo->ApprovedInfo.InSize != 0)
        {
          SyncInfo->CompressingInfoTemp.InSize = kBufferSize - SyncInfo->ApprovedInfo.InSize;
          SyncInfo->CompressingInfoTemp.OutSize = compressedSize - SyncInfo->ApprovedInfo.OutSize;
          SyncInfo->CompressingInfoTemp.Time = tickCount - SyncInfo->ApprovedInfo.Time;
          if (SyncInfo->CompressingInfo.Time == 0)
            SyncInfo->CompressingInfo = SyncInfo->CompressingInfoTemp;
        }
      }
      SyncInfo->ApprovedInfo.Init();
    }
    if(result != S_OK)
    {
      if (result != E_ABORT)
      {
        SyncInfo->Pause();
        MessageBox(0, NError::MyFormatMessage(result), TEXT("7-Zip"), MB_ICONERROR);
      }
      continue;
    }
    {
      NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
      SyncInfo->NumPasses++;
    }
    
    ///////////////////////
    // Decompressing


    CCompareOutStream *outCompareStreamSpec = new CCompareOutStream;
    CMyComPtr<ISequentialOutStream> outCompareStream = outCompareStreamSpec;

    for (int i = 0; i < 2; i++)
    {
      inStreamSpec->Init(outStreamSpec->Buffer, compressedSize);
      outCompareStreamSpec->Init();

      if (compressSetDecoderProperties)
      {
        propDecoderStreamSpec->Init(
          propStreamSpec->GetBuffer(), propStreamSpec->GetSize());
        RINOK(compressSetDecoderProperties->SetDecoderProperties(propDecoderStream));
      }
      
      UINT64 outSize = kBufferSize;
      UINT64 startTime = ::GetTimeCount();
      result = Decoder->Code(inStream, outCompareStream, 0, &outSize, decoderProgress);
      tickCount = ::GetTimeCount() - startTime;
      {
        NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
        if (result == S_OK)
        {
          SyncInfo->DecompressingInfoTemp.InSize = compressedSize;
          SyncInfo->DecompressingInfoTemp.OutSize = kBufferSize;
          SyncInfo->DecompressingInfoTemp.Time = tickCount;
          if (SyncInfo->DecompressingInfo.Time == 0 && i >= 1)
            SyncInfo->DecompressingInfo = SyncInfo->DecompressingInfoTemp;
          if (outCompareStreamSpec->CRC.GetDigest() != crc.GetDigest())
          {
            SyncInfo->NumErrors++;
            break;
          }
        }
        else
        {
          if(result != E_ABORT)
          {
            SyncInfo->NumErrors++;
            break;
          }
        }
      }
    }
  }
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

STDMETHODIMP CThreadBenchmark::SetRatioInfo(
    const UINT64 *inSize, const UINT64 *outSize)
{
  NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
  if (SyncInfo->Changed || SyncInfo->WasPaused() || SyncInfo->WasStopped())
    return E_ABORT;
  CProgressInfo ci;
  ci.InSize = *inSize;
  ci.OutSize = *outSize;
  ci.Time = ::GetTimeCount() - _startTime;
  SyncInfo->ProcessedSize = *inSize;

  UINT64 deltaTime = ci.Time - SyncInfo->CompressingInfoPrev.Time;
  if (deltaTime >= GetFreq())
  {
    SyncInfo->CompressingInfoTemp.Time = deltaTime;
    SyncInfo->CompressingInfoTemp.InSize = ci.InSize - SyncInfo->CompressingInfoPrev.InSize;
    SyncInfo->CompressingInfoTemp.OutSize = ci.InSize - SyncInfo->CompressingInfoPrev.OutSize;
    SyncInfo->CompressingInfoPrev = ci;
  }
  if (*inSize >= _approvedStart && SyncInfo->ApprovedInfo.InSize == 0)
    SyncInfo->ApprovedInfo = ci;
  return S_OK;
}

static bool GetCoderPath(UString &path)
{
  if (!GetProgramFolderPath(path))
    return false;
  path += L"Codecs\\LZMA.dll";
  return true;
}
  
typedef UINT32 (WINAPI *GetMethodPropertyFunc)(
    UINT32 index, PROPID propID, PROPVARIANT *value);

static bool LoadCoder(
  const UString &filePath, 
  NWindows::NDLL::CLibrary &library,
  CLSID &encoder, CLSID &decoder)
{
  if (!library.Load(filePath))
    return false;
  GetMethodPropertyFunc getMethodProperty = (GetMethodPropertyFunc)
        library.GetProcAddress("GetMethodProperty");
  if (getMethodProperty == NULL)
    return false;

  NWindows::NCOM::CPropVariant propVariant;
  if (getMethodProperty (0, NMethodPropID::kEncoder, &propVariant) != S_OK)
    return false;
  if (propVariant.vt != VT_BSTR)
    return false;
  encoder = *(const GUID *)propVariant.bstrVal;
  propVariant.Clear();
      
  if (getMethodProperty (0, NMethodPropID::kDecoder, &propVariant) != S_OK)
    return false;
  if (propVariant.vt != VT_BSTR)
    return false;
  decoder = *(const GUID *)propVariant.bstrVal;
  propVariant.Clear();
  return true;
}

void Benchmark(HWND hwnd)
{
  UString path;
  if (!GetCoderPath(path))
  {
    MyMessageBoxError(hwnd, L"Can't find LZMA.dll");
    return;
  }
  CLSID encoder;
  CLSID decoder;
  CThreadBenchmark benchmarker;
  if (!LoadCoder(path, benchmarker.Library, encoder, decoder))
  {
    MyMessageBoxError(hwnd, L"Can't load LZMA.dll");
    return;
  }

  CreateObjectPointer createObject = (CreateObjectPointer)
      benchmarker.Library.GetProcAddress("CreateObject");
  if (createObject == NULL)
  {
    MyMessageBoxError(hwnd, L"Incorrect LZMA.dll");
    return;
  }
  if (createObject(&encoder, &IID_ICompressCoder, (void **)&benchmarker.Encoder) != S_OK)
  {
    MyMessageBoxError(hwnd, L"Can't create codec");
    return;
  }
  if (createObject(&decoder, &IID_ICompressCoder, (void **)&benchmarker.Decoder) != S_OK)
  {
    MyMessageBoxError(hwnd, L"Can't create codec");
    return;
  }

  CBenchmarkDialog benchmarkDialog;
  benchmarker.SyncInfo = &benchmarkDialog._syncInfo;
  CThread thread;
  if (!thread.Create(CThreadBenchmark::MyThreadFunction, &benchmarker))
  {
    MyMessageBoxError(hwnd, L"error");
    return;
  }
  benchmarkDialog.Create(hwnd);
  WaitForSingleObject(thread, INFINITE);
}
