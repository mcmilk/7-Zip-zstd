// UpdateArchiveOptions.h

#pragma once

#ifndef __UPDATEARCHIVEOPTIONS_H
#define __UPDATEARCHIVEOPTIONS_H

#include "Common/Vector.h"
#include "CompressionMethodUtils.h"
#include "../Common/UpdatePairBasic.h"

struct CUpdateArchiveCommand
{
  CSysString ArchivePath;
  NUpdateArchive::CActionSet ActionSet;
};

struct CUpdateArchiveOptions
{
  CObjectVector<CUpdateArchiveCommand> Commands;
  bool UpdateArchiveItself;

  bool SfxMode;
  CSysString SfxModule;

  CSysString ArchivePath;
  CCompressionMethodMode MethodMode;
};

#endif
