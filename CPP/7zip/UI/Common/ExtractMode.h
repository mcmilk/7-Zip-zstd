// ExtractMode.h

#ifndef __EXTRACT_MODE_H
#define __EXTRACT_MODE_H

namespace NExtract {
  
namespace NPathMode
{
  enum EEnum
  {
    kFullPaths,
    kCurPaths,
    kNoPaths,
    kAbsPaths
  };
}

namespace NOverwriteMode
{
  enum EEnum
  {
    kAsk,
    kOverwrite,
    kSkip,
    kRename,
    kRenameExisting
  };
}

}

#endif
