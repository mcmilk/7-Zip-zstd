// MyLoadMenu.h

#ifndef __MYLOADMENU_H
#define __MYLOADMENU_H

void OnMenuActivating(HWND hWnd, HMENU hMenu, int position);
// void OnMenuUnActivating(HWND hWnd, HMENU hMenu, int id);
// void OnMenuUnActivating(HWND hWnd);

void MyLoadMenu(HWND hWnd);
bool OnMenuCommand(HWND hWnd, int id);
void MyLoadMenu();
void LoadFileMenu(HMENU hMenu, int startPos, bool programMenu,
    bool isFsFolder, int numItems, bool allAreFiles);
bool ExecuteFileCommand(int id);

#endif
