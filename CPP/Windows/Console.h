// Windows/Console.h

#ifndef __WINDOWS_CONSOLE_H
#define __WINDOWS_CONSOLE_H

#include "Windows/Defs.h"

namespace NWindows{
namespace NConsole{

class CBase
{
protected:
  HANDLE m_Object;
public:
  void Attach(HANDLE aHandle) { m_Object = aHandle; };
  bool GetMode(DWORD &aMode)
    { return BOOLToBool(::GetConsoleMode(m_Object, &aMode)); }
  bool SetMode(DWORD aMode)
    { return BOOLToBool(::SetConsoleMode(m_Object, aMode)); }
};

class CIn: public CBase
{
public:
  bool PeekEvents(PINPUT_RECORD anEvents, DWORD aNumEvents, DWORD &aNumEventsRead)
    {  return BOOLToBool(::PeekConsoleInput(m_Object, anEvents, aNumEvents, &aNumEventsRead)); }
  bool PeekEvent(INPUT_RECORD &anEvent, DWORD &aNumEventsRead)
    {  return PeekEvents(&anEvent, 1, aNumEventsRead); }
  bool ReadEvents(PINPUT_RECORD anEvents, DWORD aNumEvents, DWORD &aNumEventsRead)
    {  return BOOLToBool(::ReadConsoleInput(m_Object, anEvents, aNumEvents, &aNumEventsRead)); }
  bool ReadEvent(INPUT_RECORD &anEvent, DWORD &aNumEventsRead)
    {  return ReadEvents(&anEvent, 1, aNumEventsRead); }
  bool GetNumberOfEvents(DWORD &aNumberOfEvents)
    {  return BOOLToBool(::GetNumberOfConsoleInputEvents(m_Object, &aNumberOfEvents)); }

  bool WriteEvents(const INPUT_RECORD *anEvents, DWORD aNumEvents, DWORD &aNumEventsWritten)
    {  return BOOLToBool(::WriteConsoleInput(m_Object, anEvents, aNumEvents, &aNumEventsWritten)); }
  bool WriteEvent(const INPUT_RECORD &anEvent, DWORD &aNumEventsWritten)
    {  return WriteEvents(&anEvent, 1, aNumEventsWritten); }
  
  bool Read(LPVOID aBuffer, DWORD aNumberOfCharsToRead, DWORD &aNumberOfCharsRead)
    {  return BOOLToBool(::ReadConsole(m_Object, aBuffer, aNumberOfCharsToRead, &aNumberOfCharsRead, NULL)); }

  bool Flush()
    {  return BOOLToBool(::FlushConsoleInputBuffer(m_Object)); }

};

}}

#endif