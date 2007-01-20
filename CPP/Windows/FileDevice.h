// Windows/FileDevice.h

#ifndef __WINDOWS_FILEDEVICE_H
#define __WINDOWS_FILEDEVICE_H

#include "FileIO.h"
#include "Defs.h"

namespace NWindows {
namespace NFile {
namespace NDevice {

typedef struct _GET_LENGTH_INFORMATION 
{
  LARGE_INTEGER   Length;
} GET_LENGTH_INFORMATION, *PGET_LENGTH_INFORMATION;

#define IOCTL_DISK_GET_LENGTH_INFO  CTL_CODE(IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)

/*
typedef struct _DISK_GEOMETRY_EX {
  DISK_GEOMETRY Geometry; // Standard disk geometry: may be faked by driver.
  LARGE_INTEGER DiskSize; // Must always be correct
  BYTE  Data[1];  // Partition, Detect info
} DISK_GEOMETRY_EX, *PDISK_GEOMETRY_EX;
*/

#define IOCTL_CDROM_BASE  FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY  CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_MEDIA_REMOVAL  CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)

class CFileBase: public NIO::CFileBase
{
public:
  bool DeviceIoControl(DWORD controlCode, LPVOID inBuffer, DWORD inSize, 
      LPVOID outBuffer, DWORD outSize, LPDWORD bytesReturned, LPOVERLAPPED overlapped) const
  {
    return BOOLToBool(::DeviceIoControl(_handle, controlCode, inBuffer, inSize, 
        outBuffer, outSize, bytesReturned, overlapped));
  }

  bool DeviceIoControl(DWORD controlCode, LPVOID inBuffer,
      DWORD inSize, LPVOID outBuffer, DWORD outSize) const
  {
    DWORD ret;
    return DeviceIoControl(controlCode, inBuffer, inSize, outBuffer, outSize, &ret, 0);
  }

  bool DeviceIoControlIn(DWORD controlCode, LPVOID inBuffer, DWORD inSize) const
    { return DeviceIoControl(controlCode, inBuffer, inSize, NULL, 0); }
  
  bool DeviceIoControlOut(DWORD controlCode, LPVOID outBuffer, DWORD outSize) const
    { return DeviceIoControl(controlCode, NULL, 0, outBuffer, outSize); }
  
  bool GetGeometry(DISK_GEOMETRY *res) const
    { return DeviceIoControlOut(IOCTL_DISK_GET_DRIVE_GEOMETRY, res, sizeof(*res)); }

  bool GetCdRomGeometry(DISK_GEOMETRY *res) const
    { return DeviceIoControlOut(IOCTL_CDROM_GET_DRIVE_GEOMETRY, res, sizeof(*res)); }

  /*
  bool GetCdRomGeometryEx(DISK_GEOMETRY_EX *res) const
    { return DeviceIoControlOut(IOCTL_CDROM_GET_DRIVE_GEOMETRY, res, sizeof(*res));   }
  */

  bool CdRomLock(bool lock) const
  {
    PREVENT_MEDIA_REMOVAL rem;
    rem.PreventMediaRemoval = (BOOLEAN)(lock ? TRUE : FALSE);
    return DeviceIoControlIn(IOCTL_CDROM_MEDIA_REMOVAL, &rem, sizeof(rem));
  }

  bool GetLengthInfo(UInt64 &length) const
  {
    GET_LENGTH_INFORMATION lengthInfo;
    bool res  = DeviceIoControlOut(IOCTL_DISK_GET_LENGTH_INFO, &lengthInfo, sizeof(lengthInfo));
    length = lengthInfo.Length.QuadPart;
    return res;
  }

  bool GetLengthSmart(UInt64 &length);


  /*
  bool FormatTracks(const FORMAT_PARAMETERS *formatParams, 
    BAD_TRACK_NUMBER *badTrackNumbers, DWORD numBadTrackNumbers, 
    DWORD &numBadTrackNumbersReturned)
  {
    DWORD ret;
    // Check params, Probabably error
    bool res = DeviceIoControl(IOCTL_DISK_FORMAT_TRACKS, badTrackNumbers, sizeof(*formatParams),
      badTrackNumbers, numBadTrackNumbers * sizeof(*badTrackNumbers), &ret, NULL);
    numBadTrackNumbersReturned = ret / sizeof(*badTrackNumbers);
    return res;
  }
  */

  
  bool Performance(DISK_PERFORMANCE *res)
    { return DeviceIoControlOut(IOCTL_DISK_PERFORMANCE, LPVOID(res), sizeof(*res)); }
  
  bool GetPartitionInfo(PARTITION_INFORMATION *res)
    { return DeviceIoControlOut(IOCTL_DISK_GET_PARTITION_INFO, LPVOID(res), sizeof(*res)); }
  
  bool Verify(const VERIFY_INFORMATION *verifyInformation)
    { return DeviceIoControlIn(IOCTL_DISK_VERIFY, LPVOID(verifyInformation), sizeof(*verifyInformation)); }
};

class CInFile: public CFileBase
{
public:
  bool Open(LPCTSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open(LPCTSTR fileName);
  #ifndef _UNICODE
  bool Open(LPCWSTR fileName, DWORD shareMode, DWORD creationDisposition, DWORD flagsAndAttributes);
  bool Open(LPCWSTR fileName);
  #endif
  bool Read(void *data, UInt32 size, UInt32 &processedSize);
};

}}}

#endif
