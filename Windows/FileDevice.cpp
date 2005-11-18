// Windows/FileDevice.cpp

#include "StdAfx.h"

#include "FileDevice.h"

namespace NWindows {
namespace NFile {
namespace NDevice {

bool CFileBase::GetLengthSmart(UInt64 &length)
{
  PARTITION_INFORMATION partInfo;
  if (GetPartitionInfo(&partInfo))
  {
    length = partInfo.PartitionLength.QuadPart;
    return true;
  }
  DISK_GEOMETRY geom;
  if (!GetGeometry(&geom))
    if (!GetCdRomGeometry(&geom))
      return false;
  length = geom.Cylinders.QuadPart * geom.TracksPerCylinder * geom.SectorsPerTrack * geom.BytesPerSector;
  return true;
}

bool CInFile::Open(LPCTSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
  { return Create(fileName, GENERIC_READ, shareMode, creationDisposition, flagsAndAttributes); }

bool CInFile::Open(LPCTSTR fileName)
  { return Open(fileName, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL); }

#ifndef _UNICODE
bool CInFile::Open(LPCWSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes)
  { return Create(fileName, GENERIC_READ, shareMode, creationDisposition, flagsAndAttributes); }

bool CInFile::Open(LPCWSTR fileName)
  { return Open(fileName, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL); }
#endif

bool CInFile::Read(void *data, UInt32 size, UInt32 &processedSize)
{
  DWORD processedLoc = 0;
  bool res = BOOLToBool(::ReadFile(_handle, data, size, &processedLoc, NULL));
  processedSize = (UInt32)processedLoc;
  return res;
}

}}}
