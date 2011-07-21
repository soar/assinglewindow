#include "stdafx.h"
#include "WindowsManager.h"
#include "AsSingleWindow.h"

void sWindowInfo::saveState()
{
    WINDOWPLACEMENT wndPlace;
    wndPlace.length = sizeof(wndPlace);
    if (! GetWindowPlacement(this->hWnd, &wndPlace))
        return;

    switch (wndPlace.showCmd)
    {
        case SW_HIDE:
            this->eState = WINDOW_STATE_HIDDEN;
            break;
        case SW_MINIMIZE:
        case SW_SHOWMINIMIZED:
        case SW_SHOWMINNOACTIVE:
            this->eState = WINDOW_STATE_MINIMIZED;
            break;
        case SW_MAXIMIZE:
        case SW_RESTORE:
        case SW_SHOW:
        case SW_SHOWNA:
        case SW_SHOWNOACTIVATE:
        case SW_SHOWNORMAL:
            this->eState = WINDOW_STATE_NORMAL;
            break;
    }
}

void sWindowInfo::saveRect()
{
    switch (this->eState)
    {
        case WINDOW_STATE_HIDDEN:
        case WINDOW_STATE_MINIMIZED:
            WINDOWPLACEMENT wndPlace;
            wndPlace.length = sizeof(wndPlace);
            if (GetWindowPlacement(this->hWnd, &wndPlace))
                this->rLastSavedPosition = wndPlace.rcNormalPosition;
            break;
        default:
            GetWindowRect(this->hWnd, &this->rLastSavedPosition);
            break;
    }
}

void pluginSetProgress()
{
    EnterCriticalSection(&pluginVars.m_CS);
    pluginVars.IsUpdateInProgress = true;
    LeaveCriticalSection(&pluginVars.m_CS);
}

void pluginSetDone()
{
    EnterCriticalSection(&pluginVars.m_CS);
    pluginVars.IsUpdateInProgress = false;
    LeaveCriticalSection(&pluginVars.m_CS);
}

bool pluginIsAlreadyRunning()
{
    EnterCriticalSection(&pluginVars.m_CS);
    bool result = pluginVars.IsUpdateInProgress;
    LeaveCriticalSection(&pluginVars.m_CS);
    return result;
}

/**
 * Поиск окна в списке
 * возвращается указатель на структуру с информацией
 */
sWindowInfo* windowFind(HWND hWnd)
{
    windowsList::iterator itr;
    for (itr = pluginVars.allWindows.begin(); itr != pluginVars.allWindows.end(); ++itr)
        if (itr->hWnd == hWnd)
            return &*itr;

    return NULL;
}

/**
 * Поиск окна в списке
 * возвращается итератор
 */
windowsList::iterator windowFindItr(HWND hWnd)
{
    windowsList::iterator itr;
    for (itr = pluginVars.allWindows.begin(); itr != pluginVars.allWindows.end(); ++itr)
        if (itr->hWnd == hWnd)
            return itr;

    return itr;
}

/**
 * Поиск окна в списке
 * возвращается реверсный итератор
 */
windowsList::reverse_iterator windowFindRevItr(HWND hWnd)
{
    windowsList::reverse_iterator ritr;
    for (ritr = pluginVars.allWindows.rbegin(); ritr != pluginVars.allWindows.rend(); ++ritr)
        if (ritr->hWnd == hWnd)
            return ritr;

    return ritr;
}

/**
 * Добавление окна в список окон и выставление всех начальных значений
 * здесь же выставляется хук на wndProc
 */
void windowAdd(HWND hWnd, bool IsMain)
{
    sWindowInfo thisWindowInfo;
    
    hWnd = windowGetRoot(hWnd);

    // already exists in list
    if (windowFind(hWnd) != NULL)
        return;

    windowSetStartPosition(hWnd);

    thisWindowInfo.hWnd = hWnd;
    thisWindowInfo.pPrevWndProc = (WNDPROC) SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) wndProcSync);
    thisWindowInfo.saveRect();
    
    pluginVars.allWindows.push_back(thisWindowInfo);

    if (IsMain)
        pluginVars.contactListHWND = hWnd;
}

/**
 * Удаление окна из списка окон
 */
void windowRemove(HWND hWnd)
{
    windowsList::iterator itr = windowFindItr(hWnd);
    if (itr != pluginVars.allWindows.end())
        pluginVars.allWindows.erase(itr);
}

/**
 * Поиск окна верхнего уровня
 */
HWND windowGetRoot(HWND hWnd)
{
    HWND hWndParent = GetParent(hWnd);
    while (IsWindow(hWndParent) && (hWndParent != GetDesktopWindow())) // IsWindowVisible() ?
    {
        hWnd = hWndParent;
        hWndParent = GetParent(hWnd);
    }
    return hWnd;
}

/**
 * Установка стартовых координат и размера окна
 * базируется на основе координат последнего окна в списке
 */
void windowSetStartPosition(HWND hWnd)
{
    hWnd = windowGetRoot(hWnd);

    RECT srcWindowPos, prevWindowPos;

    if (! GetWindowRect(hWnd, &srcWindowPos))
        return;

    windowsList::reverse_iterator ritr = pluginVars.allWindows.rbegin();

    if (ritr == pluginVars.allWindows.rend() || ! GetWindowRect(ritr->hWnd, &prevWindowPos))
        return;

    LONG pw = srcWindowPos.right - srcWindowPos.left;
    LONG ph = prevWindowPos.bottom - prevWindowPos.top;    
    LONG px = prevWindowPos.left - pw;
    LONG py = prevWindowPos.top;

    SetWindowPos(hWnd, 0, px, py, pw, ph, SWP_NOZORDER);
}

/**
 * Перемещение и ресайз всех окон списка
 * hWnd - окно-источник события перемещения/ресайза
 */
void allWindowsMoveAndSize(HWND hWnd)
{
    if (pluginIsAlreadyRunning())
        return;
    pluginSetProgress();

    RECT rWPC, rWPN;
    LONG cx, cy, cw, ch;
    
    // Просмотр окон от текущего до конца
    windowsList::iterator itrC = windowFindItr(hWnd);
    windowsList::iterator itrN = ++windowFindItr(hWnd);
    for (; itrC != pluginVars.allWindows.end(), itrN != pluginVars.allWindows.end(); itrC++, itrN++)
    {
        if (! GetWindowRect(itrC->hWnd, &rWPC) || ! GetWindowRect(itrN->hWnd, &rWPN))
            continue;

        cw = rWPN.right - rWPN.left;
        ch = rWPC.bottom - rWPC.top;
        cx = rWPC.left - cw;
        cy = rWPC.top;

        SetWindowPos(itrN->hWnd, itrC->hWnd, cx, cy, cw, ch, SWP_NOACTIVATE);
        itrN->saveRect();
    }

    // Просмотр окон от текущего до начала
    windowsList::reverse_iterator ritrC = windowFindRevItr(hWnd);
    windowsList::reverse_iterator ritrN = ++windowFindRevItr(hWnd);
    for (; ritrC != pluginVars.allWindows.rend(), ritrN != pluginVars.allWindows.rend(); ritrC++, ritrN++)
    {
        if (! GetWindowRect(ritrC->hWnd, &rWPC) || ! GetWindowRect(ritrN->hWnd, &rWPN))
            continue;

        cw = rWPN.right - rWPN.left;
        ch = rWPC.bottom - rWPC.top;
        cx = rWPC.right;
        cy = rWPC.top;

        SetWindowPos(ritrN->hWnd, ritrC->hWnd, cx, cy, cw, ch, SWP_NOACTIVATE);
        ritrN->saveRect();
    }

    windowFind(hWnd)->saveRect();
    pluginSetDone();
}

void windowChangeState(HWND hWnd)
{

}

LRESULT CALLBACK wndProcSync(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
            if (wParam == SIZE_RESTORED)
                allWindowsMoveAndSizeWithBase(hWnd);
            break;
        case WM_MOVE:
            allWindowsMoveAndSizeWithBase(hWnd);
            break;
    }

    if (sWindowInfo* wndInfo = windowFind(hWnd))
        return CallWindowProc(wndInfo->pPrevWndProc, hWnd, msg, wParam, lParam);
    else
        return 0;
}

#pragma optimize ("", off)
LRESULT CALLBACK debugWndCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);
    UINT8 x, y, dx, dy;
    x = rc.left;
    y = rc.top;
    dx = rc.right;
    dy = rc.bottom;

    UINT8 test = 0;

    WINDOWPLACEMENT wndPlace;
    wndPlace.length = sizeof(wndPlace);
    GetWindowPlacement(hWnd, &wndPlace);

    UINT sC = wndPlace.showCmd;
    
    switch (msg) 
    {
        case WM_SYSCOMMAND:
            test = 1;
            break;
        case WM_MOVE:
            test = 2;
            break;
        case WM_SIZE:
            test = 3;
            break;
    }
    
    test++;

    if (sWindowInfo* wndInfo = windowFind(hWnd))
        return CallWindowProc(wndInfo->pPrevWndProc, hWnd, msg, wParam, lParam);
    else
        return 0;
}

void debugColoring(HWND hWnd)
{
    HDC hdc;
    hdc = GetWindowDC(hWnd);
    RECT rct;
    GetWindowRect(hWnd, &rct);
    for (int i = 0; i < 10; i++)
    {
        FillRect(hdc, &rct, (HBRUSH) (GetSysColor(COLOR_WINDOWTEXT) + 1));
        Sleep(100);
        RedrawWindow(hWnd, NULL, NULL, RDW_ALLCHILDREN + RDW_UPDATENOW);
        Sleep(100);
    }
}
/*
LONG calcNewWindowPosition(HWND hWndLeading, HWND hWndDriven, RECT* rc, eWindowPosition wndPos)
{
    RECT rWndLeading, rWndDriven;

    if ((! GetWindowRect(hWndLeading, &rWndLeading)) || (! GetWindowRect(hWndDriven, &rWndDriven)))
        return 0;

    rc->bottom = rWndLeading.bottom;
    rc->top = rWndLeading.top;

    switch (wndPos)
    {
        case WINDOW_POSITION_RIGHT:
            rc->left = rWndLeading.left - (rWndDriven.right - rWndDriven.left);
            rc->right = rWndLeading.left;
            return rc->left;
        case WINDOW_POSITION_LEFT:
            rc->left = rWndLeading.right;
            rc->right = rWndLeading.right + (rWndDriven.right - rWndDriven.left);
            return rc->right;
    }
}
*/

// end of file