// ConsoleClose.cpp

#include "StdAfx.h"

#include "ConsoleClose.h"

static int g_BreakCounter = 0;
static const kBreakAbortThreshold = 2;

namespace NConsoleClose {

static BOOL WINAPI HandlerRoutine(DWORD aCtrlType)
{
  g_BreakCounter++;
  if (g_BreakCounter < kBreakAbortThreshold)
    return TRUE;
  return FALSE;
  /*
  switch(aCtrlType)
  {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      if (g_BreakCounter < kBreakAbortThreshold)
      return TRUE;
  }
  return FALSE;
  */
}

bool TestBreakSignal()
{
  /*
  if (g_BreakCounter > 0)
    return true;
  */
  return (g_BreakCounter > 0);
}

void CheckCtrlBreak()
{
  if (TestBreakSignal())
    throw CCtrlBreakException();
}

CCtrlHandlerSetter::CCtrlHandlerSetter()
{
  if(!SetConsoleCtrlHandler(HandlerRoutine, TRUE))
    throw "SetConsoleCtrlHandler fails";
}

CCtrlHandlerSetter::~CCtrlHandlerSetter()
{
  if(!SetConsoleCtrlHandler(HandlerRoutine, FALSE))
    throw "SetConsoleCtrlHandler fails";
}

}