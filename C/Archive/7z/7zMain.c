/* 
7zMain.c
Test application for 7z Decoder
LZMA SDK 4.43 Copyright (c) 1999-2006 Igor Pavlov (2006-06-04)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define USE_WINDOWS_FUNCTIONS
#endif

#ifdef USE_WINDOWS_FUNCTIONS
#include <windows.h>
#endif

#include "7zIn.h"
#include "7zExtract.h"

#include "../../7zCrc.h"


#ifdef USE_WINDOWS_FUNCTIONS
typedef HANDLE MY_FILE_HANDLE;
#else
typedef FILE *MY_FILE_HANDLE;
#endif

void ConvertNumberToString(CFileSize value, char *s)
{
  char temp[32];
  int pos = 0;
  do 
  {
    temp[pos++] = (char)('0' + (int)(value % 10));
    value /= 10;
  }
  while (value != 0);
  do
    *s++ = temp[--pos];
  while(pos > 0);
  *s = '\0';
}


#ifdef USE_WINDOWS_FUNCTIONS
// ReadFile and WriteFile functions in Windows have BUG:
// If you Read or Write 64MB or more (probably min_failure_size = 64MB - 32KB + 1) 
// from/to Network file, it returns ERROR_NO_SYSTEM_RESOURCES 
// (Insufficient system resources exist to complete the requested service).
#define kChunkSizeMax (1 << 24)
#endif

size_t MyReadFile(MY_FILE_HANDLE file, void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  #ifdef USE_WINDOWS_FUNCTIONS
  {
    size_t processedSize = 0;
    do
    {
      DWORD curSize = (size > kChunkSizeMax) ? kChunkSizeMax : (DWORD)size;
      DWORD processedLoc = 0;
      BOOL res = ReadFile(file, data, curSize, &processedLoc, NULL);
      data = (void *)((unsigned char *)data + processedLoc);
      size -= processedLoc;
      processedSize += processedLoc;
      if (!res || processedLoc == 0)
        break;
    }
    while (size > 0);
    return processedSize;
  }
  #else
  return fread(data, 1, size, file); 
  #endif
}

size_t MyWriteFile(MY_FILE_HANDLE file, void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  #ifdef USE_WINDOWS_FUNCTIONS
  {
    size_t processedSize = 0;
    do
    {
      DWORD curSize = (size > kChunkSizeMax) ? kChunkSizeMax : (DWORD)size;
      DWORD processedLoc = 0;
      BOOL res = WriteFile(file, data, curSize, &processedLoc, NULL);
      data = (void *)((unsigned char *)data + processedLoc);
      size -= processedLoc;
      processedSize += processedLoc;
      if (!res)
        break;
    }
    while (size > 0);
    return processedSize;
  }
  #else
  return fwrite(data, 1, size, file); 
  #endif
}

int MyCloseFile(MY_FILE_HANDLE file)
{ 
  #ifdef USE_WINDOWS_FUNCTIONS
  return (CloseHandle(file) != FALSE) ? 0 : 1;
  #else
  return fclose(file); 
  #endif
}

typedef struct _CFileInStream
{
  ISzInStream InStream;
  MY_FILE_HANDLE File;
} CFileInStream;

#ifdef _LZMA_IN_CB

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc;
  if (maxRequiredSize > kBufferSize)
    maxRequiredSize = kBufferSize;
  processedSizeLoc = MyReadFile(s->File, g_Buffer, maxRequiredSize);
  *buffer = g_Buffer;
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#else

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = MyReadFile(s->File, buffer, size);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#endif

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;

  #ifdef USE_WINDOWS_FUNCTIONS
  {
    LARGE_INTEGER value;
    value.LowPart = (DWORD)pos;
    value.HighPart = (LONG)(pos >> 32);
    value.LowPart = SetFilePointer(s->File, value.LowPart, &value.HighPart, FILE_BEGIN);
    if (value.LowPart == 0xFFFFFFFF)
      if(GetLastError() != NO_ERROR) 
        return SZE_FAIL;
    return SZ_OK;
  }
  #else
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
  #endif
}

void PrintError(char *sz)
{
  printf("\nERROR: %s\n", sz);
}

int main(int numargs, char *args[])
{
  CFileInStream archiveStream;
  CArchiveDatabaseEx db;
  SZ_RESULT res;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;

  printf("\n7z ANSI-C Decoder 4.44  Copyright (c) 1999-2006 Igor Pavlov  2006-08-27\n");
  if (numargs == 1)
  {
    printf(
      "\nUsage: 7zDec <command> <archive_name>\n\n"
      "<Commands>\n"
      "  e: Extract files from archive\n"
      "  l: List contents of archive\n"
      "  t: Test integrity of archive\n");
    return 0;
  }
  if (numargs < 3)
  {
    PrintError("incorrect command");
    return 1;
  }

  archiveStream.File = 
  #ifdef USE_WINDOWS_FUNCTIONS
  CreateFile(args[2], GENERIC_READ, FILE_SHARE_READ, 
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (archiveStream.File == INVALID_HANDLE_VALUE)
  #else
  archiveStream.File = fopen(args[2], "rb");
  if (archiveStream.File == 0)
  #endif
  {
    PrintError("can not open input file");
    return 1;
  }

  archiveStream.InStream.Read = SzFileReadImp;
  archiveStream.InStream.Seek = SzFileSeekImp;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  CrcGenerateTable();

  SzArDbExInit(&db);
  res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
  if (res == SZ_OK)
  {
    char *command = args[1];
    int listCommand = 0;
    int testCommand = 0;
    int extractCommand = 0;
    if (strcmp(command, "l") == 0)
      listCommand = 1;
    if (strcmp(command, "t") == 0)
      testCommand = 1;
    else if (strcmp(command, "e") == 0)
      extractCommand = 1;

    if (listCommand)
    {
      UInt32 i;
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        CFileItem *f = db.Database.Files + i;
        char s[32];
        ConvertNumberToString(f->Size, s);
        printf("%10s  %s\n", s, f->Name);
      }
    }
    else if (testCommand || extractCommand)
    {
      UInt32 i;

      /*
      if you need cache, use these 3 variables.
      if you use external function, you can make these variable as static.
      */
      UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
      Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
      size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

      printf("\n");
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        size_t offset;
        size_t outSizeProcessed;
        CFileItem *f = db.Database.Files + i;
        if (f->IsDirectory)
          printf("Directory ");
        else
          printf(testCommand ? 
            "Testing   ":
            "Extracting");
        printf(" %s", f->Name);
        if (f->IsDirectory)
        {
          printf("\n");
          continue;
        }
        res = SzExtract(&archiveStream.InStream, &db, i, 
            &blockIndex, &outBuffer, &outBufferSize, 
            &offset, &outSizeProcessed, 
            &allocImp, &allocTempImp);
        if (res != SZ_OK)
          break;
        if (!testCommand)
        {
          MY_FILE_HANDLE outputHandle;
          UInt32 processedSize;
          char *fileName = f->Name;
          size_t nameLen = strlen(f->Name);
          for (; nameLen > 0; nameLen--)
            if (f->Name[nameLen - 1] == '/')
            {
              fileName = f->Name + nameLen;
              break;
            }
            
          outputHandle = 
          #ifdef USE_WINDOWS_FUNCTIONS
            CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, 
                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
          if (outputHandle == INVALID_HANDLE_VALUE)
          #else
          fopen(fileName, "wb+");
          if (outputHandle == 0)
          #endif
          {
            PrintError("can not open output file");
            res = SZE_FAIL;
            break;
          }
          processedSize = MyWriteFile(outputHandle, outBuffer + offset, outSizeProcessed);
          if (processedSize != outSizeProcessed)
          {
            PrintError("can not write output file");
            res = SZE_FAIL;
            break;
          }
          if (MyCloseFile(outputHandle))
          {
            PrintError("can not close output file");
            res = SZE_FAIL;
            break;
          }
        }
        printf("\n");
      }
      allocImp.Free(outBuffer);
    }
    else
    {
      PrintError("incorrect command");
      res = SZE_FAIL;
    }
  }
  SzArDbExFree(&db, allocImp.Free);

  MyCloseFile(archiveStream.File);
  if (res == SZ_OK)
  {
    printf("\nEverything is Ok\n");
    return 0;
  }
  if (res == SZE_OUTOFMEMORY)
    PrintError("can not allocate memory");
  else     
    printf("\nERROR #%d\n", res);
  return 1;
}
