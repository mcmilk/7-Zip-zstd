// BenchmarkDialog.h

#ifndef __BENCHMARKDIALOG_H
#define __BENCHMARKDIALOG_H

#include "resource.h"

#include "Common/MyCom.h"
#include "Windows/Control/Dialog.h"
#include "Windows/Control/ComboBox.h"
#include "Windows/Synchronization.h"
#include "../../../ICoder.h"

const int kNumBenchDictionaryBitsStart = 21;

struct CProgressInfo
{
  UINT64 InSize;
  UINT64 OutSize;
  UINT64 Time;
  void Init()
  {
    InSize = 0;
    OutSize = 0;
    Time = 0;
  }
};

class CProgressSyncInfo
{
  bool Stopped;
  bool Paused;
public:
  bool Changed;
  UINT32 DictionarySize;
  bool MultiThread;
  UINT64 NumPasses;
  UINT64 NumErrors;
  NWindows::NSynchronization::CManualResetEvent _startEvent;
  NWindows::NSynchronization::CCriticalSection CS;

  CProgressInfo ApprovedInfo;
  CProgressInfo CompressingInfoPrev;
  CProgressInfo CompressingInfoTemp;
  CProgressInfo CompressingInfo;
  UINT64 ProcessedSize;

  CProgressInfo DecompressingInfoTemp;
  CProgressInfo DecompressingInfo;

  void Init()
  {
    Changed = false;
    ApprovedInfo.Init();
    CompressingInfoPrev.Init();
    CompressingInfoTemp.Init();
    CompressingInfo.Init();
    ProcessedSize = 0;
    
    DecompressingInfoTemp.Init();
    DecompressingInfo.Init();

    Stopped = false;
    Paused = false;
    NumPasses = 0;
    NumErrors = 0;
  }
  void InitSettings()
  {
    DictionarySize = (1 << kNumBenchDictionaryBitsStart);
    MultiThread = false;
  }
  void Stop()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(CS);
    Stopped = true;
  }
  bool WasStopped()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(CS);
    return Stopped;
  }
  void Pause()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(CS);
    Paused = true;
  }
  void Start()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(CS);
    Paused = false;
  }
  bool WasPaused()
  {
    NWindows::NSynchronization::CCriticalSectionLock lock(CS);
    return Paused;
  }
  void WaitCreating() { _startEvent.Lock(); }
};

class CBenchmarkDialog: 
  public NWindows::NControl::CModalDialog
{
  NWindows::NControl::CComboBox m_Dictionary;
  UINT_PTR _timer;
  UINT32 _startTime;

  bool OnTimer(WPARAM timerID, LPARAM callback);
	virtual bool OnInit();
  void OnRestartButton();
  void OnStopButton();
  void OnHelp();
	virtual void OnCancel();
  bool OnButtonClicked(int buttonID, HWND buttonHWND);
  bool OnCommand(int code, int itemID, LPARAM lParam);

  void PrintTime();
  void PrintRating(UINT64 rating, UINT controlID);
  void PrintResults(
      UINT32 dictionarySize,
      UINT64 elapsedTime, 
      UINT64 size, UINT speedID, UINT ratingID, 
      bool decompressMode = false, UINT64 secondSize = 0);

  UINT32 OnChangeDictionary();
  void OnChangeSettings();
public:
  CProgressSyncInfo _syncInfo;

  CBenchmarkDialog(): _timer(0) {}

  INT_PTR Create(HWND wndParent = 0)
  { 
    return CModalDialog::Create(MAKEINTRESOURCE(IDD_DIALOG_BENCHMARK), wndParent); 
  }
};

void Benchmark(HWND hwnd);

#endif
