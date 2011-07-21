#pragma once

#ifndef WINDOWSMANAGER_H
#define WINDOWSMANAGER_H

#include "stdafx.h"

enum WindowState {
    WINDOW_STATE_NORMAL = 0,
    WINDOW_STATE_MINIMIZED = 1,
    WINDOW_STATE_HIDDEN = 2,
};

/*
enum eWindowPosition {
    WINDOW_POSITION_LEFT = 1,
    WINDOW_POSITION_RIGHT = 2,
};
*/

struct sWindowInfo {
    HWND hWnd;
    WindowState eState;
    WNDPROC pPrevWndProc;
    RECT rLastSavedPosition;

    void saveState();
    void saveRect();
    //void restoreRect();
};

// critical section tools
void pluginSetProgress();
void pluginSetDone();
bool pluginIsAlreadyRunning();

// system
sWindowInfo* windowFind(HWND);
//windowsList::iterator windowFindItr(HWND);
//windowsList::reverse_iterator windowFindRevItr(HWND);
void windowAdd(HWND, bool);
void windowRemove(HWND);
HWND windowGetRoot(HWND);
void windowSetStartPosition(HWND);

// tools
//LONG calcNewWindowPosition(HWND, HWND, RECT*, eWindowPosition);

// window callbacks
LRESULT CALLBACK wndProcSync(HWND, UINT, WPARAM, LPARAM);
void allWindowsMoveAndSizeWithDiff(HWND);
void allWindowsMoveAndSizeWithBase(HWND);

// debug
LRESULT CALLBACK debugWndCallback(HWND, UINT, WPARAM, LPARAM);
void debugColoring(HWND);

#endif WINDOWSMANAGER_H

// end of file