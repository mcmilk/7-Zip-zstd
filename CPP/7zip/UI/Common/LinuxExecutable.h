// LinuxExecutable.h
// Modified 2026-11-18: identify ELF executables and .sh script names.

#ifndef ZIP7_INC_LINUX_EXECUTABLE_H
#define ZIP7_INC_LINUX_EXECUTABLE_H

#include "../../../Common/MyString.h"
#include "UpdateCallback.h"

namespace NLinuxPermissions
{
inline bool IsSupportedFormat(const UString &name)
{
  return name.IsEqualTo_Ascii_NoCase("7z") || name.IsEqualTo_Ascii_NoCase("tar");
}

inline UInt64 ReadElfInteger(const Byte *p, unsigned size, bool littleEndian)
{
  UInt64 value = 0;
  for (unsigned i = 0; i < size; ++i)
    value = (value << 8) | p[littleEndian ? size - 1 - i : i];
  return value;
}

inline bool IsElfExecutable(const Byte *p, UInt32 size, UInt64 fileSize)
{
  if (size < 52 || p[0] != 0x7F || p[1] != 'E' || p[2] != 'L' || p[3] != 'F'
      || (p[4] != 1 && p[4] != 2) || (p[5] != 1 && p[5] != 2) || p[6] != 1)
    return false;
  const bool is64 = p[4] == 2;
  const unsigned headerSize = is64 ? 64 : 52;
  if (size < headerSize)
    return false;
  const bool little = p[5] == 1;
  const UInt64 type = ReadElfInteger(p + 16, 2, little);
  // Shared libraries normally have no entry point; PIE executables do.
  if ((type != 2 && type != 3) || ReadElfInteger(p + 24, is64 ? 8 : 4, little) == 0
      || ReadElfInteger(p + 20, 4, little) != 1
      || ReadElfInteger(p + (is64 ? 52 : 40), 2, little) != headerSize)
    return false;
  const UInt64 offset = ReadElfInteger(p + (is64 ? 32 : 28), is64 ? 8 : 4, little);
  const UInt64 entrySize = ReadElfInteger(p + (is64 ? 54 : 42), 2, little);
  const UInt64 count = ReadElfInteger(p + (is64 ? 56 : 44), 2, little);
  return count != 0 && count != 0xFFFF && entrySize >= (is64 ? 56u : 32u)
      && offset >= headerSize && offset <= fileSize
      && count <= (fileSize - offset) / entrySize;
}

inline bool IsShellScriptName(const UString &name)
{
  const unsigned len = name.Len();
  return len >= 3 && name[len - 3] == L'.'
      && (name[len - 2] == L's' || name[len - 2] == L'S')
      && (name[len - 1] == L'h' || name[len - 1] == L'H');
}

#if defined(_WIN32) && !defined(UNDER_CE)
inline HRESULT SetForItems(CDirItems &items, IUpdateCallbackUI *callback, IProgress *progress = NULL)
{
  FOR_VECTOR (i, items.Items)
  {
    if (progress)
    {
      RINOK(progress->SetCompleted(NULL))
    }
    else
    {
      RINOK(callback->CheckBreak())
    }
    CDirItem &item = items.Items[i];
    if (item.IsAltStream || item.IsDevice || item.Has_Attrib_ReparsePoint())
      continue;
    if (item.IsDir())
    {
      item.PosixModeOverride = MY_LIN_S_IFDIR | 0755;
      continue;
    }
    bool executable = IsShellScriptName(item.Name);
    if (!executable && item.Size >= 52)
    {
      NWindows::NFile::NIO::CInFile file;
      file.PreserveATime = true;
      const FString path = items.GetPhyPath(i);
      bool opened = file.OpenShared(path, items.ShareForWrite);
      if (!opened)
      {
        file.PreserveATime = false;
        opened = file.OpenShared(path, items.ShareForWrite);
      }
      if (opened)
      {
        Byte header[64];
        UInt32 processed = 0;
        if (file.Read(header, sizeof(header), processed))
          executable = IsElfExecutable(header, processed, item.Size);
      }
    }
    const UInt32 permissions = executable ? 0755u : 0644u;
    item.PosixModeOverride = MY_LIN_S_IFREG
        | (item.IsReadOnly() ? permissions & ~0222u : permissions);
  }
  return S_OK;
}
#endif
}

#endif
