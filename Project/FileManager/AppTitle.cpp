// AppTitle.cpp

#include "StdAfx.h"

#include "resource.h"

#include "Common/IntToString.h"
#include "Windows/ResourceString.h"

#include "AppTitle.h"

using namespace NWindows;

// extern HWND g_HWND;

// void CAppTitle::LoadTittle()

CAppTitle::CAppTitle(): Window(0) 
{
  _range = 0;
  _prevPercent = _UI64_MAX;
}

CAppTitle::~CAppTitle()
{
  AddToTitle(TEXT(""));
}

void CAppTitle::AddToTitle(LPCTSTR string)
{
  ::SetWindowText(Window, string + CSysString(Title));
}

void CAppTitle::PrintPercent(UINT64 percent)
{
  TCHAR string[64];
  ConvertUINT64ToString(percent, string);
  lstrcat(string, TEXT("% "));
  CSysString string2 = CSysString(string) + AddTitle;
  AddToTitle(string2);
}

UINT64 GetPercent(UINT64 range, UINT64 pos)
{
  if (range > _UI64_MAX / 100 || pos > _UI64_MAX / 100)
  {
    range /= 100;
    pos /= 100;
  }
  if (range == 0)
    return 0;
  return (pos * 100) / range;
}


void CAppTitle::SetRange(UINT64 range)
{
  _range = range;
  _prevPercent = _UI64_MAX;
}

void CAppTitle::SetPos(UINT64 pos)
{
  UINT64 percent = GetPercent(_range, pos);
  if (percent == _prevPercent)
    return;
  PrintPercent(percent);
  _prevPercent = percent;
}

