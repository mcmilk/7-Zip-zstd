// FileFolderPluginOpen.h

#pragma once

#ifndef __FILEFOLDERPLUGINOPEN_H
#define __FILEFOLDERPLUGINOPEN_H

#include "../Archiver/Common/IArchiveHandler2.h" 

HRESULT OpenFileFolderPlugin(const UString &path, IFolderFolder **resultFolder);

#endif
