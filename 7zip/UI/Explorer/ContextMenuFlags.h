// ContextMenuFlags.h

#ifndef __SEVENZIP_CONTEXTMENUFLAGS_H
#define __SEVENZIP_CONTEXTMENUFLAGS_H

namespace NContextMenuFlags
{
  const UINT32 kExtract = 1 << 0;
  const UINT32 kExtractHere = 1 << 1;
  const UINT32 kExtractTo = 1 << 2;
  // const UINT32 kExtractEach = 1 << 3;

  const UINT32 kTest = 1 << 4;

  const UINT32 kOpen = 1 << 5;

  const UINT32 kCompress = 1 << 8;
  const UINT32 kCompressTo = 1 << 9;
  const UINT32 kCompressEmail = 1 << 10;
  const UINT32 kCompressToEmail = 1 << 11;

  inline UINT32 GetDefaultFlags() { 
      return kOpen | kExtract | kExtractHere | kCompress | kTest; }     
}

#endif
