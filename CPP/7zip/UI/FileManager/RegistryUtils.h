// RegistryUtils.h

#include "Common/StringConvert.h"

#ifndef __REGISTRYUTILS_H
#define __REGISTRYUTILS_H

void SaveRegLang(const UString &langFile);
void ReadRegLang(UString &langFile);

void SaveRegEditor(const UString &editorPath);
void ReadRegEditor(UString &editorPath);

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

// void SaveLockMemoryAdd(bool enable);
// bool ReadLockMemoryAdd();

bool ReadLockMemoryEnable();
void SaveLockMemoryEnable(bool enable);

/*
void SaveSingleClick(bool enable);
bool ReadSingleClick();

void SaveUnderline(bool enable);
bool ReadUnderline();
*/

#endif
