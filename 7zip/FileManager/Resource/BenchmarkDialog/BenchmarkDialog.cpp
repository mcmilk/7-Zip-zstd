// BenchmarkDialog.cpp

#include "StdAfx.h"

#include "Common/IntToString.h"
#include "Common/StringToInt.h"
#include "Common/Exception.h"
#include "Common/Alloc.h"
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

static const UInt32 kAdditionalSize = (6 << 20);
static const UInt32 kCompressedAdditionalSize = (1 << 10);
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

UInt64 GetTimeCount()
{
  return GetTickCount();
  /*
  LARGE_INTEGER value;
  if (::QueryPerformanceCounter(&value))
    return value.QuadPart;
  return GetTickCount();
  */
}

UInt64 GetFreq()
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
  UInt32 A1;
  UInt32 A2;
public:
  CRandomGenerator() { Init(); }
  void Init() { A1 = 362436069; A2 = 521288629;}
  UInt32 GetRnd() 
  {
    return 
      ((A1 = 36969 * (A1 & 0xffff) + (A1 >> 16)) << 16) ^
      ((A2 = 18000 * (A2 & 0xffff) + (A2 >> 16)) );
  }
};

class CBitRandomGenerator
{
  CRandomGenerator RG;
  UInt32 Value;
  int NumBits;
public:
  void Init()
  {
    Value = 0;
    NumBits = 0;
  }
  UInt32 GetRnd(int numBits) 
  {
    if (NumBits > numBits)
    {
      UInt32 result = Value & ((1 << numBits) - 1);
      Value >>= numBits;
      NumBits -= numBits;
      return result;
    }
    numBits -= NumBits;
    UInt32 result = (Value << numBits);
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
  UInt32 Pos;
  UInt32 Rep0;
public:
  UInt32 BufferSize;
  Byte *Buffer;
  CBenchRandomGenerator(): Buffer(0) {} 
  ~CBenchRandomGenerator() { Free(); }
  void Free() 
  { 
    ::MidFree(Buffer);
    Buffer = 0;
  }
  bool Alloc(UInt32 bufferSize) 
  {
    if (Buffer != 0 && BufferSize == bufferSize)
      return true;
    Free();
    Buffer = (Byte *)::MidAlloc(bufferSize);
    Pos = 0;
    BufferSize = bufferSize;
    return (Buffer != 0);
  }
  UInt32 GetRndBit() { return RG.GetRnd(1); }
  /*
  UInt32 GetLogRand(int maxLen)
  {
    UInt32 len = GetRnd() % (maxLen + 1);
    return GetRnd() & ((1 << len) - 1);
  }
  */
  UInt32 GetLogRandBits(int numBits)
  {
    UInt32 len = RG.GetRnd(numBits);
    return RG.GetRnd(len);
  }
  UInt32 GetOffset()
  {
    if (GetRndBit() == 0)
      return GetLogRandBits(4);
    return (GetLogRandBits(4) << 10) | RG.GetRnd(10);
  }
  UInt32 GetLen1() { return RG.GetRnd(1 + RG.GetRnd(2)); }
  UInt32 GetLen2() { return RG.GetRnd(2 + RG.GetRnd(2)); }
  void Generate()
  {
    RG.Init(); 
    Rep0 = 1;
    while(Pos < BufferSize)
    {
      if (GetRndBit() == 0 || Pos < 1)
        Buffer[Pos++] = Byte(RG.GetRnd(8));
      else
      {
        UInt32 len;
        if (RG.GetRnd(3) == 0)
          len = 1 + GetLen1();
        else
        {
          do
            Rep0 = GetOffset();
          while (Rep0 >= Pos);
          Rep0++;
          len = 2 + GetLen2();
        }
        for (UInt32 i = 0; i < len && Pos < BufferSize; i++, Pos++)
          Buffer[Pos] = Buffer[Pos - Rep0];
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
  for (int i = kNumBenchDictionaryBitsStart; i <= 30; i++)
    for (int j = 0; j < 2; j++)
    {
      UInt32 dictionary = (1 << i) + (j << (i - 1));
      if(dictionary >
      #ifdef _WIN64
      (1 << 30)
      #else
      (1 << 27)
      #endif
      )
        continue;
      TCHAR s[40];
      ConvertUInt64ToString((dictionary >> 20), s);
      lstrcat(s, kMB);
      int index = (int)m_Dictionary.AddString(s);
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

static UInt64 GetLZMAUsage(UInt32 dictionary)
{ 
  UInt32 hs = dictionary - 1;
  hs |= (hs >> 1);
  hs |= (hs >> 2);
  hs |= (hs >> 4);
  hs |= (hs >> 8);
  hs >>= 1;
  hs |= 0xFFFF;
  if (hs > (1 << 24))
    hs >>= 1;
  hs++;
  return ((hs + (1 << 16)) + (UInt64)dictionary * 2) * 4 + (UInt64)dictionary * 3 / 2 + (1 << 20);
}

static UInt64 GetMemoryUsage(UInt32 dictionary, bool mtMode)
{
  const UInt32 kBufferSize = dictionary + kAdditionalSize;
  const UInt32 kCompressedBufferSize = (kBufferSize / 2) + kCompressedAdditionalSize;
  return (mtMode ? (6 << 20) : 0 )+ kBufferSize + kCompressedBufferSize +
    GetLZMAUsage(dictionary) + dictionary + (2 << 20);
}

UInt32 CBenchmarkDialog::OnChangeDictionary()
{
  UInt32 dictionary = (UInt32)m_Dictionary.GetItemData(m_Dictionary.GetCurSel());
  UInt64 memUsage = GetMemoryUsage(dictionary, IsButtonCheckedBool(IDC_BENCHMARK_MULTITHREADING));
  memUsage = (memUsage + (1 << 20) - 1) >> 20;
  TCHAR s[40];
  ConvertUInt64ToString(memUsage, s);
  lstrcat(s, kMB);
  SetItemText(IDC_BENCHMARK_MEMORY_VALUE, s);
  return dictionary;
}

void CBenchmarkDialog::OnChangeSettings()
{
  EnableItem(IDC_BUTTON_STOP, true);
  UInt32 dictionary = OnChangeDictionary();
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
  TCHAR s[40];
  GetTimeString(elapsedSec, s);
  SetItemText(IDC_BENCHMARK_ELAPSED_VALUE, s);
}

static UInt32 GetLogSize(UInt32 size)
{
  for (int i = kSubBits; i < 32; i++)
    for (UInt32 j = 0; j < (1 << kSubBits); j++)
      if (size <= (((UInt32)1) << i) + (j << (i - kSubBits)))
        return (i << kSubBits) + j;
  return (32 << kSubBits);
}

static UInt64 GetCompressRating(UInt32 dictionarySize, 
    UInt64 elapsedTime, UInt64 size)
{
  if (elapsedTime == 0)
    elapsedTime = 1;
  UInt64 t = GetLogSize(dictionarySize) - (18 << kSubBits);
  UInt64 numCommandsForOne = 1060 + ((t * t * 10) >> (2 * kSubBits));
  UInt64 numCommands = (UInt64)(size) * numCommandsForOne;
  return numCommands * GetFreq() / elapsedTime;
}

static UInt64 GetDecompressRating(UInt64 elapsedTime, 
    UInt64 outSize, UInt64 inSize)
{
  if (elapsedTime == 0)
    elapsedTime = 1;
  UInt64 numCommands = inSize * 220 + outSize * 20;
  return numCommands * GetFreq() / elapsedTime;
}

static UInt64 GetTotalRating(
    UInt32 dictionarySize, 
    UInt64 elapsedTimeEn, UInt64 sizeEn,
    UInt64 elapsedTimeDe, 
    UInt64 inSizeDe, UInt64 outSizeDe)
{
  return (GetCompressRating(dictionarySize, elapsedTimeEn, sizeEn) + 
    GetDecompressRating(elapsedTimeDe, inSizeDe, outSizeDe)) / 2;
}

void CBenchmarkDialog::PrintRating(UInt64 rating, UINT controlID)
{
  TCHAR s[40];
  ConvertUInt64ToString(rating / 1000000, s);
  lstrcat(s, kMIPS);
  SetItemText(controlID, s);
}

void CBenchmarkDialog::PrintResults(
    UInt32 dictionarySize,
    UInt64 elapsedTime, 
    UInt64 size, UINT speedID, UINT ratingID,
    bool decompressMode, UInt64 secondSize)
{
  TCHAR s[40];
  UInt64 speed = size * GetFreq() / elapsedTime;
  ConvertUInt64ToString(speed / 1024, s);
  lstrcat(s, kKBs);
  SetItemText(speedID, s);
  UInt64 rating;
  if (decompressMode)
    rating = GetDecompressRating(elapsedTime, size, secondSize);
  else
    rating = GetCompressRating(dictionarySize, elapsedTime, size);
  PrintRating(rating, ratingID);
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

  ConvertUInt64ToString(_syncInfo.NumErrors, s);
  SetItemText(IDC_BENCHMARK_ERRORS_VALUE, s);

  UInt64 elapsedTime = _syncInfo.CompressingInfoTemp.Time;
  if (elapsedTime >= 1)
  {
    UInt32 dicSizeTemp = (UInt32)MyMax(_syncInfo.ProcessedSize, UInt64(1) << 20);
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
  const Byte *Data;
  UInt32 Pos;
  UInt32 Size;
public:
  MY_UNKNOWN_IMP
  void Init(const Byte *data, UInt32 size)
  {
    Data = data;
    Size = size;
    Pos = 0;
  }
  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CBenchmarkInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 remain = Size - Pos;
  if (size > remain)
    size = remain;
  for (UInt32 i = 0; i < size; i++)
  {
    ((Byte *)data)[i] = Data[Pos + i];
  }
  Pos += size;
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
class CBenchmarkOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
  UInt32 BufferSize;
public:
  UInt32 Pos;
  Byte *Buffer;
  CBenchmarkOutStream(): Buffer(0) {} 
  ~CBenchmarkOutStream() { Free(); }
  void Free() 
  { 
    ::MidFree(Buffer);
    Buffer = 0;
  }

  bool Alloc(UInt32 bufferSize) 
  {
    if (Buffer != 0 && BufferSize == bufferSize)
      return true;
    Free();
    Buffer = (Byte *)::MidAlloc(bufferSize);
    Init();
    BufferSize = bufferSize;
    return (Buffer != 0);
  }

  void Init() 
  {
    Pos = 0;
  }

  MY_UNKNOWN_IMP
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CBenchmarkOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 i;
  for (i = 0; i < size && Pos < BufferSize; i++)
    Buffer[Pos++] = ((const Byte *)data)[i];
  if(processedSize != NULL)
    *processedSize = i;
  if (i != size)
  {
    MessageBoxW(0, L"Buffer is full", L"7-zip error", 0);
    return E_FAIL;
  }
  return S_OK;
}
  
class CCompareOutStream: 
  public ISequentialOutStream,
  public CMyUnknownImp
{
public:
  CCRC CRC;
  MY_UNKNOWN_IMP
  void Init() { CRC.Init(); }
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
};

STDMETHODIMP CCompareOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  CRC.Update(data, size);
  if(processedSize != NULL)
    *processedSize = size;
  return S_OK;
}
  
typedef UInt32 (WINAPI * CreateObjectPointer)(const GUID *clsID, 
    const GUID *interfaceID, void **outObject);

struct CDecoderProgressInfo:
  public ICompressProgressInfo,
  public CMyUnknownImp
{
  CProgressSyncInfo *SyncInfo;
  MY_UNKNOWN_IMP
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);
};

STDMETHODIMP CDecoderProgressInfo::SetRatioInfo(
    const UInt64 * /* inSize */, const UInt64 * /* outSize */)
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
  UInt64 _startTime;
  UInt64 _approvedStart;
  NDLL::CLibrary Library;
  CMyComPtr<ICompressCoder> Encoder;
  CMyComPtr<ICompressCoder> Decoder;
  HRESULT Process();
  HRESULT Result;
  static DWORD WINAPI MyThreadFunction(void *param)
  {
    ((CThreadBenchmark *)param)->Result = ((CThreadBenchmark *)param)->Process();
    return 0;
  }
  MY_UNKNOWN_IMP
  STDMETHOD(SetRatioInfo)(const UInt64 *inSize, const UInt64 *outSize);
};

HRESULT CThreadBenchmark::Process()
{
  try
  {
  SyncInfo->WaitCreating();
  CBenchRandomGenerator randomGenerator;
  CMyComPtr<ICompressWriteCoderProperties> writeCoderProperties;
  Encoder.QueryInterface(IID_ICompressWriteCoderProperties, 
      &writeCoderProperties);
  CMyComPtr<ICompressSetDecoderProperties2> compressSetDecoderProperties;
  Decoder.QueryInterface(
        IID_ICompressSetDecoderProperties2, &compressSetDecoderProperties);
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

  CDecoderProgressInfo *decoderProgressInfoSpec = new 
      CDecoderProgressInfo;
  CMyComPtr<ICompressProgressInfo> decoderProgress = decoderProgressInfoSpec;
  decoderProgressInfoSpec->SyncInfo = SyncInfo;
    
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
    bool multiThread;
    {
      NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
      dictionarySize = SyncInfo->DictionarySize;
      multiThread = SyncInfo->MultiThread;
      if (SyncInfo->Changed)
        SyncInfo->Init();
    }

    const UInt32 kBufferSize = dictionarySize + kAdditionalSize;
    const UInt32 kCompressedBufferSize = (kBufferSize / 2) + kCompressedAdditionalSize;

    if (setCoderProperties)
    {
      PROPID propIDs[] = 
      {
        NCoderPropID::kDictionarySize,
        NCoderPropID::kMultiThread
      };
      const int kNumProps = sizeof(propIDs) / sizeof(propIDs[0]);
      NWindows::NCOM::CPropVariant properties[kNumProps];
      properties[0] = UInt32(dictionarySize);
      properties[1] = bool(multiThread);
      HRESULT res = setCoderProperties->SetCoderProperties(propIDs,
          properties, kNumProps);
      if (res != S_OK)
      {
        // SyncInfo->Pause();
        MessageBox(0, NError::MyFormatMessage(res), TEXT("7-Zip"), MB_ICONERROR);
        return res;
      }
    }
      
    if (propStream)
    {
      propStreamSpec->Init();
      writeCoderProperties->WriteCoderProperties(propStream);
    }

    if (!randomGenerator.Alloc(kBufferSize))
      return E_OUTOFMEMORY;

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
    CMyComPtr<ISequentialInStream> inStream = inStreamSpec;
    CBenchmarkOutStream *outStreamSpec = new CBenchmarkOutStream;
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    if (!outStreamSpec->Alloc(kCompressedBufferSize))
      return E_OUTOFMEMORY;

    {
      // this code is for reducing time of memory allocationg
      inStreamSpec->Init(randomGenerator.Buffer, MyMin((UInt32)1, randomGenerator.BufferSize));
      outStreamSpec->Init();
      /* HRESULT result = */ Encoder->Code(inStream, outStream, 0, 0, NULL);
    }

    inStreamSpec->Init(randomGenerator.Buffer, randomGenerator.BufferSize);
    outStreamSpec->Init();

    _approvedStart = dictionarySize;
    _startTime = ::GetTimeCount();
    HRESULT result = Encoder->Code(inStream, outStream, 0, 0, this);
    UInt64 tickCount = ::GetTimeCount() - _startTime;
    UInt32 compressedSize = outStreamSpec->Pos;
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
        RINOK(compressSetDecoderProperties->SetDecoderProperties2(
          propStreamSpec->GetBuffer(), (UInt32)propStreamSpec->GetSize()));
      }
      
      UInt64 outSize = kBufferSize;
      UInt64 startTime = ::GetTimeCount();
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
    const UInt64 *inSize, const UInt64 *outSize)
{
  NSynchronization::CCriticalSectionLock lock(SyncInfo->CS);
  if (SyncInfo->Changed || SyncInfo->WasPaused() || SyncInfo->WasStopped())
    return E_ABORT;
  CProgressInfo ci;
  ci.InSize = *inSize;
  ci.OutSize = *outSize;
  ci.Time = ::GetTimeCount() - _startTime;
  SyncInfo->ProcessedSize = *inSize;

  UInt64 deltaTime = ci.Time - SyncInfo->CompressingInfoPrev.Time;
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
  
typedef UInt32 (WINAPI *GetMethodPropertyFunc)(
    UInt32 index, PROPID propID, PROPVARIANT *value);

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
