// UpdateCallbackAgent.h

#ifndef __UPDATECALLBACKAGENT_H
#define __UPDATECALLBACKAGENT_H

#include "../Common/UpdateCallback.h"
#include "IFolderArchive.h"

class CUpdateCallbackAgent: public IUpdateCallbackUI
{
  INTERFACE_IUpdateCallbackUI(;)
  CMyComPtr<ICryptoGetTextPassword2> _cryptoGetTextPassword;
  CMyComPtr<IFolderArchiveUpdateCallback> Callback;
  CMyComPtr<ICompressProgressInfo> _compressProgress;
public:
  void SetCallback(IFolderArchiveUpdateCallback *callback);
};

#endif
