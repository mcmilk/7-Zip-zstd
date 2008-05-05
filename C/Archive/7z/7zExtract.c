/* 7zExtract.c -- Extracting from 7z archive
2008-04-09
Igor Pavlov
Copyright (c) 1999-2008 Igor Pavlov
Read 7zExtract.h for license options */

#include "7zExtract.h"
#include "7zDecode.h"
#include "../../7zCrc.h"

SRes SzAr_Extract(
    const CSzArEx *p,
    ISzInStream *inStream, 
    UInt32 fileIndex,
    UInt32 *blockIndex,
    Byte **outBuffer, 
    size_t *outBufferSize,
    size_t *offset, 
    size_t *outSizeProcessed, 
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  UInt32 folderIndex = p->FileIndexToFolderIndexMap[fileIndex];
  SRes res = SZ_OK;
  *offset = 0;
  *outSizeProcessed = 0;
  if (folderIndex == (UInt32)-1)
  {
    IAlloc_Free(allocMain, *outBuffer);
    *blockIndex = folderIndex;
    *outBuffer = 0;
    *outBufferSize = 0;
    return SZ_OK;
  }

  if (*outBuffer == 0 || *blockIndex != folderIndex)
  {
    CSzFolder *folder = p->db.Folders + folderIndex;
    CFileSize unPackSizeSpec = SzFolder_GetUnPackSize(folder);
    size_t unPackSize = (size_t)unPackSizeSpec;
    CFileSize startOffset = SzArEx_GetFolderStreamPos(p, folderIndex, 0);

    if (unPackSize != unPackSizeSpec)
      return SZ_ERROR_MEM;
    *blockIndex = folderIndex;
    IAlloc_Free(allocMain, *outBuffer);
    *outBuffer = 0;
    
    RINOK(inStream->Seek(inStream, startOffset, SZ_SEEK_SET));
    
    if (res == SZ_OK)
    {
      *outBufferSize = unPackSize;
      if (unPackSize != 0)
      {
        *outBuffer = (Byte *)IAlloc_Alloc(allocMain, unPackSize);
        if (*outBuffer == 0)
          res = SZ_ERROR_MEM;
      }
      if (res == SZ_OK)
      {
        res = SzDecode(p->db.PackSizes + 
          p->FolderStartPackStreamIndex[folderIndex], folder, 
          inStream, startOffset, 
          *outBuffer, unPackSize, allocTemp);
        if (res == SZ_OK)
        {
          if (folder->UnPackCRCDefined)
          {
            if (CrcCalc(*outBuffer, unPackSize) != folder->UnPackCRC)
              res = SZ_ERROR_CRC;
          }
        }
      }
    }
  }
  if (res == SZ_OK)
  {
    UInt32 i; 
    CSzFileItem *fileItem = p->db.Files + fileIndex;
    *offset = 0;
    for(i = p->FolderStartFileIndex[folderIndex]; i < fileIndex; i++)
      *offset += (UInt32)p->db.Files[i].Size;
    *outSizeProcessed = (size_t)fileItem->Size;
    if (*offset + *outSizeProcessed > *outBufferSize)
      return SZ_ERROR_FAIL;
    {
      if (fileItem->IsFileCRCDefined)
      {
        if (CrcCalc(*outBuffer + *offset, *outSizeProcessed) != fileItem->FileCRC)
          res = SZ_ERROR_CRC;
      }
    }
  }
  return res;
}
