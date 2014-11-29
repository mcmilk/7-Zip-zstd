// Windows/FileName.h

#ifndef __WINDOWS_FILE_NAME_H
#define __WINDOWS_FILE_NAME_H

#include "../Common/MyString.h"

namespace NWindows {
namespace NFile {
namespace NName {

void NormalizeDirPathPrefix(FString &dirPath); // ensures that it ended with '\\', if dirPath is not epmty
void NormalizeDirPathPrefix(UString &dirPath);

#ifdef _WIN32

extern const wchar_t *kSuperPathPrefix; /* \\?\ */
const unsigned kDevicePathPrefixSize = 4;
const unsigned kSuperPathPrefixSize = 4;
const unsigned kSuperUncPathPrefixSize = kSuperPathPrefixSize + 4;

bool IsDevicePath(CFSTR s) throw(); /* \\.\ */
bool IsSuperUncPath(CFSTR s) throw();

bool IsDrivePath(const wchar_t *s) throw();
bool IsSuperPath(const wchar_t *s) throw();
bool IsSuperOrDevicePath(const wchar_t *s) throw();

#ifndef USE_UNICODE_FSTRING
bool IsDrivePath(CFSTR s) throw();
bool IsSuperPath(CFSTR s) throw();
bool IsSuperOrDevicePath(CFSTR s) throw();
#endif

#endif // _WIN32

bool IsAbsolutePath(const wchar_t *s) throw();
unsigned GetRootPrefixSize(const wchar_t *s) throw();

#ifdef WIN_LONG_PATH

const int kSuperPathType_UseOnlyMain = 0;
const int kSuperPathType_UseOnlySuper = 1;
const int kSuperPathType_UseMainAndSuper = 2;

int GetUseSuperPathType(CFSTR s) throw();
bool GetSuperPath(CFSTR path, UString &longPath, bool onlyIfNew);
bool GetSuperPaths(CFSTR s1, CFSTR s2, UString &d1, UString &d2, bool onlyIfNew);

#define USE_MAIN_PATH (__useSuperPathType != kSuperPathType_UseOnlySuper)
#define USE_MAIN_PATH_2 (__useSuperPathType1 != kSuperPathType_UseOnlySuper && __useSuperPathType2 != kSuperPathType_UseOnlySuper)

#define USE_SUPER_PATH (__useSuperPathType != kSuperPathType_UseOnlyMain)
#define USE_SUPER_PATH_2 (__useSuperPathType1 != kSuperPathType_UseOnlyMain || __useSuperPathType2 != kSuperPathType_UseOnlyMain)

#define IF_USE_MAIN_PATH int __useSuperPathType = GetUseSuperPathType(path); if (USE_MAIN_PATH)
#define IF_USE_MAIN_PATH_2(x1, x2) \
    int __useSuperPathType1 = GetUseSuperPathType(x1); \
    int __useSuperPathType2 = GetUseSuperPathType(x2); \
    if (USE_MAIN_PATH_2)

#else

#define IF_USE_MAIN_PATH
#define IF_USE_MAIN_PATH_2(x1, x2)

#endif // WIN_LONG_PATH

bool GetFullPath(CFSTR dirPrefix, CFSTR path, FString &fullPath);
bool GetFullPath(CFSTR path, FString &fullPath);

}}}

#endif
