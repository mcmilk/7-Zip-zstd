// RegistryUtils.h

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

void SaveFullRow(bool enable);
bool ReadFullRow();

void SaveShowGrid(bool enable);
bool ReadShowGrid();

void SaveAlternativeSelection(bool enable);
bool ReadAlternativeSelection();

/*
void SaveSingleClick(bool enable);
bool ReadSingleClick();

void SaveUnderline(bool enable);
bool ReadUnderline();
*/

#endif
