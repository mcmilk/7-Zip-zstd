// HelpUtils.h

#ifndef __HELPUTILS_H
#define __HELPUTILS_H

#pragma once

#include "Common/String.h"

bool GetProgramDirPrefix(CSysString &aFolder);
void ShowHelpWindow(HWND aHWNDForHelp, LPCTSTR aTopicFile);

#endif