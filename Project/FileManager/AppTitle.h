// AppTitle.h

#pragma once

#ifndef __APPTITLE_H
#define __APPTITLE_H

class CAppTitle
{
  UINT64 _range;
  UINT64 _prevPercent;
  void PrintPercent(UINT64 percent);

  // void LoadTittle();
public:
  HWND Window;
  CSysString Title;
  CSysString AddTitle;

  CAppTitle();
  ~CAppTitle();
  void AddToTitle(LPCTSTR string);
  void SetRange(UINT64 range);
  void SetPos(UINT64 pos);
};

#endif

