// ArError.h

#pragma once

#ifndef __ARERROR_H
#define __ARERROR_H

namespace NExitCode {

struct CSystemError
{
  UINT32 ErrorValue;
  CSystemError(UINT32 anErrorValue): ErrorValue(anErrorValue) {}
};

struct CMultipleErrors
{
  UINT64 NumErrors;
  CMultipleErrors(UINT64 aNumErrors): NumErrors(aNumErrors) {}
};


enum EEnum {

  kSuccess       = 0,     // Successful operation (User exit)
  kWarning       = 1,     // Non fatal error(s) occurred
  kFatalError    = 2,     // A fatal error occurred
  kCRCError      = 3,     // A CRC error occurred when unpacking     
  kLockedArchive = 4,     // Attempt to modify an archive previously locked
  kWriteError    = 5,     // Write to disk error
  kOpenError     = 6,     // Open file error
  kUserError     = 7,     // Command line option error
  kMemoryError   = 8,     // Not enough memory for operation
  
  
  kNotSupported        = 102, // format of file doesn't supported 
  kFileError           = 103, //  
  
  kVerError            = 110, // Version doesn't supported 
  kMethodError         = 111, // Unsupported method 
  
  kUserQuit            = 120, // Unsupported method 
  
  kFileIsNotArchive    = 130, // File Is Not Archive 
  
  kCommonError         = 150, 
  
  kInputArchiveException  = 160, // archive file does not exist 
  
  kErrorsDuringDecompression  = 170, // Errors during decompression
  
  
  kDirFileWith64BitSize         = 171, 
  kFileTimeWinToDosConvertError = 172, 
  
  kFileChangedDuringOperation  = 180, 
  
  kUserBreak     = 255   // User stopped the process

};

}

#endif
