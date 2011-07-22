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

    // Если окно уже есть в списке (могло быть спрятано)
    if (sWindowInfo* wndInfo = windowFind(hWnd))
    {
        wndInfo->eState = WINDOW_STATE_NORMAL; // если окно пряталось ранее
        windowReposition(hWnd);
        return;
    }

    thisWindowInfo.hWnd = hWnd;
    thisWindowInfo.eState = WINDOW_STATE_NORMAL;
    thisWindowInfo.pPrevWndProc = (WNDPROC) SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) wndProcSync);
    
    pluginVars.allWindows.push_back(thisWindowInfo);

    if (IsMain)
        pluginVars.contactListHWND = hWnd;

    windowReposition(hWnd);
}

/**
 * Удаление окна из списка окон
 */
void windowRemove(HWND hWnd)
{
    windowsList::iterator itr = windowFindItr(hWnd);
    if (itr != pluginVars.allWindows.end())
    {
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) itr->pPrevWndProc);
        pluginVars.allWindows.erase(itr);
    }
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
 * базируется на основе координат другого окна в списке
 */
void windowReposition(HWND hWnd)
{
    RECT prevWindowPos;

    hWnd = windowGetRoot(hWnd);    

    if (sWindowInfo* wndInfo = windowFind(hWnd))
    {
        for (windowsList::iterator itr = pluginVars.allWindows.begin(); itr != pluginVars.allWindows.end(); itr++)
            if (itr->hWnd != hWnd)
                if (GetWindowRect(itr->hWnd, &prevWindowPos))
                    SendMessage(itr->hWnd, WM_MOVE, 0, MAKELPARAM(prevWindowPos.left, prevWindowPos.top));
    }
    /*
    else
    {
        if (! GetWindowRect(hWnd, &srcWindowPos))
            return;

        windowsList::reverse_iterator ritr = pluginVars.allWindows.rbegin();

        if (ritr == pluginVars.allWindows.rend())
            return;

        if (ritr->hWnd == hWnd)
            ritr++;

        if (! GetWindowRect(ritr->hWnd, &prevWindowPos)))
            return;

        LONG pw = srcWindowPos.right - srcWindowPos.left;
        LONG ph = prevWindowPos.bottom - prevWindowPos.top;    
        LONG px = prevWindowPos.left - pw;
        LONG py = prevWindowPos.top;

        SetWindowPos(hWnd, 0, px, py, pw, ph, SWP_NOZORDER);
    }
    */
}

/**
 * Перемещение и ресайз всех окон списка
 * hWnd - окно-источник события перемещения/ресайза
 */
void allWindowsMoveAndSize(HWND hWnd)
{
    if (windowFind(hWnd)->eState != WINDOW_STATE_NORMAL)
        return;

    if (pluginIsAlreadyRunning())
        return;
    pluginSetProgress();

    // Просмотр окон от текущего до конца
    windowsList::iterator itrC = windowFindItr(hWnd);
    windowsList::iterator itrN = ++windowFindItr(hWnd);
    for (; itrC != pluginVars.allWindows.end(), itrN != pluginVars.allWindows.end(); itrC++, itrN++)
    {
        // itrC проверяется в начале функции
        UINT incCount = 0;
        bool isItrInList = true;
        while (itrN->eState != WINDOW_STATE_NORMAL)
        {
            itrN++;
            incCount++;
            if (itrN == pluginVars.allWindows.end())
            {
                isItrInList = false;
                break;
            }
        }
        if (! isItrInList)
            break;

        sWndCoords wndCoord;
        calcNewWindowPosition(itrC->hWnd, itrN->hWnd, &wndCoord, WINDOW_POSITION_RIGHT);
        SetWindowPos(itrN->hWnd, itrC->hWnd, wndCoord.x, wndCoord.y, wndCoord.width, wndCoord.height, SWP_NOACTIVATE);

        itrN->saveRect();

        for (; incCount != 0; incCount--)
            itrC++;
    }

    // Просмотр окон от текущего до начала
    windowsList::reverse_iterator ritrC = windowFindRevItr(hWnd);
    windowsList::reverse_iterator ritrN = ++windowFindRevItr(hWnd);
    for (; ritrC != pluginVars.allWindows.rend(), ritrN != pluginVars.allWindows.rend(); ritrC++, ritrN++)
    {
        UINT incCount = 0;
        bool isItrInList = true;
        while (ritrN->eState != WINDOW_STATE_NORMAL)
        {
            ritrN++;
            incCount++;
            if (ritrN == pluginVars.allWindows.rend())
            {
                isItrInList = false;
                break;
            }
        }
        if (! isItrInList)
            break;

        sWndCoords wndCoord;
        calcNewWindowPosition(ritrC->hWnd, ritrN->hWnd, &wndCoord, WINDOW_POSITION_LEFT);
        SetWindowPos(ritrN->hWnd, ritrC->hWnd, wndCoord.x, wndCoord.y, wndCoord.width, wndCoord.height, SWP_NOACTIVATE);

        ritrN->saveRect();

        for (; incCount != 0; incCount--)
            ritrC++;
    }

    windowFind(hWnd)->saveRect();
    pluginSetDone();
}

void windowChangeState(HWND hWnd, WPARAM cmd, LPARAM cursorCoords)
{
    if (sWindowInfo* wndInfo = windowFind(hWnd))
    {
        switch (cmd)
        {
            case SC_CLOSE:
                wndInfo->eState = WINDOW_STATE_CLOSED;
                windowReposition(hWnd);
                break;
            case SC_MAXIMIZE:
                wndInfo->eState = WINDOW_STATE_MAXIMIZED;
                windowReposition(hWnd);
                break;
            case SC_MINIMIZE:
                wndInfo->eState = WINDOW_STATE_MINIMIZED;
                break;
            case SC_RESTORE:
            case SC_MOVE:
            case SC_SIZE:
                wndInfo->eState = WINDOW_STATE_NORMAL;
                break;
        }
    }
}

LRESULT CALLBACK wndProcSync(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SYSCOMMAND:
            windowChangeState(hWnd, wParam, lParam);
            break;
        case WM_SIZE:
            //if (wParam == SIZE_RESTORED)
                allWindowsMoveAndSize(hWnd);
            break;
        case WM_MOVE:
            allWindowsMoveAndSize(hWnd);
            break;
        case WM_SHOWWINDOW:
            break;
    }

    if (sWindowInfo* wndInfo = windowFind(hWnd))
    {
        // closing - not realy closes
        //if (wndInfo->eState == WINDOW_STATE_CLOSED)
        //    windowRemove(hWnd);
        return CallWindowProc(wndInfo->pPrevWndProc, hWnd, msg, wParam, lParam);
    }
    else
        return 0;
}

void calcNewWindowPosition(HWND hWndLeading, HWND hWndDriven, sWndCoords* wndCoord, eWindowPosition wndPos)
{
    RECT rWndLeading, rWndDriven;

    if (! GetWindowRect(hWndLeading, &rWndLeading) || ! GetWindowRect(hWndDriven, &rWndDriven))
        return;

    wndCoord->width = rWndDriven.right - rWndDriven.left;
    wndCoord->height = rWndLeading.bottom - rWndLeading.top;
    wndCoord->y = rWndLeading.top;
    if (wndPos == WINDOW_POSITION_RIGHT)
        wndCoord->x = rWndLeading.left - wndCoord->width;
    else if (wndPos == WINDOW_POSITION_LEFT)
        wndCoord->x = rWndLeading.right;
}

// end of file