// RegistryUtils.h

#pragma once

#include "Common/StringConvert.h"

#ifndef __REGISTRYUTILS_H
#define __REGISTRYUTILS_H

void SaveRegLang(const CSysString &langFile);
void ReadRegLang(CSysString &langFile);

void SaveRegEditor(const CSysString &langFile);
void ReadRegEditor(CSysString &langFile);

void SaveShowDots(bool showDots);
bool ReadShowDots();

void SaveShowRealFileIcons(bool show);
bool ReadShowRealFileIcons();

void SaveShowSystemMenu(bool showSystemMenu);
bool ReadShowSystemMenu();

#endif