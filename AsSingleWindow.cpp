#include "AsSingleWindow.h"

int OnModulesLoaded(WPARAM wParam, LPARAM lParam);
int MsgWindowEvent(WPARAM wParam, LPARAM lParam);
int MsgWindowGetData(WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK CListWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MsgWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern "C" bool WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    hInst = hInstDLL;
    return true;
}

static const MUUID interfaces[] = {MIID_CLIST, MIID_CHAT, MIID_SRMM, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
    return interfaces;
}

extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
    pluginInfo.cbSize = sizeof(PLUGININFO);
    return (PLUGININFO*) &pluginInfo;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
    pluginInfo.cbSize = sizeof(PLUGININFOEX);
    return &pluginInfo;
}

extern "C" __declspec(dllexport) int Load(PLUGINLINK *link)
{
    pluginLink = link;

    IsUpdateInProgress = false;

    hookModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);

    return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
    UnhookEvent(hookModulesLoaded);
    UnhookEvent(hookMsgWindow);

    return 0;
}

int OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    hookMsgWindow = HookEvent(ME_MSG_WINDOWEVENT, MsgWindowEvent);
    hWndCListWindow = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
    oldCListWindowProc = (WNDPROC) SetWindowLongPtr(hWndCListWindow, GWLP_WNDPROC, (LONG_PTR) CListWindowProc);

    return 0;
}

/**
 * Обработка открытия и закрытия окна чата
 *
 * Во время открытия - сохраняем хендлер окна, во время закрытия - очищаем его.
 **/
int MsgWindowEvent(WPARAM wParam, LPARAM lParam)
{
    MessageWindowEventData* data = (MessageWindowEventData*) lParam;

    if ((data == NULL) || (data->cbSize < sizeof(MessageWindowEventData)))
        return 0;

    if ((data->uType == MSG_WINDOW_EVT_OPEN) && (hWndMsgWindow == 0))
    {
        hWndMsgWindow = GetParent(GetParent(data->hwndWindow));
        oldMsgWindowProc = (WNDPROC) SetWindowLongPtr(hWndMsgWindow, GWLP_WNDPROC, (LONG_PTR) MsgWindowProc);
    }
    else if (data->uType == MSG_WINDOW_EVT_CLOSING)
    {
        hWndMsgWindow = 0;
    }
    
    return 0;
}

void SyncMoveTwoWindows(HWND hWndSrc, HWND hWndDst, bool IsSrcCL)
{
    if ((hWndSrc == 0) || (hWndDst == 0))
        return;

    RECT rWndSrc, rWndDst;
    int x, y, w, h;

    GetWindowRect(hWndSrc, &rWndSrc);
    GetWindowRect(hWndDst, &rWndDst);

    if (IsSrcCL)
    {
        x = rWndSrc.left - (rWndDst.right - rWndDst.left);
        y = rWndSrc.top;
    }
    else
    {
        x = rWndSrc.right;
        y = rWndSrc.top;
    }

    w = rWndDst.right - rWndDst.left;
    h = rWndSrc.bottom - rWndSrc.top;

    SetWindowPos(hWndDst, hWndSrc, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void SyncActivateSecondWindow(HWND hWndSrc, HWND hWndDst, bool bNeedShow)
{
    if ((hWndSrc == NULL) || (hWndDst == NULL))
        return;

    //ShowWindow(hWndDst, IsShow ? SW_SHOW : SW_HIDE);
    SetWindowPos(hWndDst, hWndSrc, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | (bNeedShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

void ActivateCListWindow(HWND hWndSrc, HWND hWndDst, bool bNeedShow)
{
    cListActivations++;

    if ((hWndSrc == NULL) || (hWndDst == NULL))
        return;

    /*
    if (bNeedShow)
    {
        DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);
        ShowWindow(hWndDst, SW_RESTORE);
        SetWindowPos(hWndDst, hWndSrc, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        /*MoveWindow(hWndDst, rCListWindow.left, rCListWindow.top, 
            rCListWindow.right - rCListWindow.left, 
            rCListWindow.bottom - rCListWindow.top, true);*/
    /*
    }
    else
    {
        //GetWindowRect(hWndDst, &rCListWindow);
        /*
        DBWriteContactSettingByte(NULL, "CList", "State", SETTING_STATE_HIDDEN);
        ShowWindow(hWndSrc, SW_RESTORE);
        ShowWindow(hWndDst, SW_RESTORE);
        CallService(MS_CLIST_SHOWHIDE, 0, 0);
        
        MSG msg;
        msg.hwnd = hWndDst;
        msg.message = TIM_CALLBACK;
        msg.lParam = WM_MBUTTONUP;

        Shell_NotifyIconW

        CallService(MS_CLIST_TRAYICONPROCESSMESSAGE, (WPARAM)(MSG*)&msg, 0);
    //}
    
    /*
    RECT rWndDst;
    GetWindowRect(hWndDst, &rWndDst);

    ShowWindow(hWndDst, bNeedShow ? SW_RESTORE : SW_MINIMIZE);
    if (bNeedShow)
        SetWindowPos(hWndDst, hWndSrc, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    */
    
    //CallService(MS_CLIST_SHOWHIDE, 0, 0);

    /*
    int test = DBGetContactSettingByte(NULL, "CList", "State", SETTING_STATE_NORMAL);
    switch (test) {
        case SETTING_STATE_NORMAL:
            MessageBox(NULL, L"SETTING_STATE_NORMAL", L"CList State", MB_OK);
            break;
        case SETTING_STATE_MINIMIZED:
            MessageBox(NULL, L"SETTING_STATE_MINIMIZED", L"CList State", MB_OK);
            break;
        case SETTING_STATE_HIDDEN:
            MessageBox(NULL, L"SETTING_STATE_HIDDEN", L"CList State", MB_OK);
            break;
    }
    */    
}

void ActivateMessageWindow(HWND hWndSrc, HWND hWndDst, bool bNeedShow)
{
    if ((hWndSrc == NULL) || (hWndDst == NULL))
        return;

    SetWindowPos(hWndDst, hWndSrc, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | (bNeedShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

LRESULT CALLBACK CListWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!IsUpdateInProgress && IsWindow(hWndMsgWindow))
    {
        IsUpdateInProgress = true;

        switch (msg) {
            case WM_SIZE:
            case WM_MOVE:
                SyncMoveTwoWindows(hWndCListWindow, hWndMsgWindow, true);
                break;
            case WM_SHOWWINDOW:
                //ActivateMessageWindow(hWndCListWindow, hWndMsgWindow, wParam ? true : false);
                //SyncActivateSecondWindow(hWndCListWindow, hWndMsgWindow, wParam ? true : false);
                break;
            case TIM_CALLBACK:
                if ((lParam == WM_RBUTTONDOWN) || (lParam == WM_LBUTTONDOWN))
                {
                    WINDOWPLACEMENT wp;
                    GetWindowPlacement(hWndMsgWindow, &wp);                    
                
                    if (IsIconic(hWndMsgWindow))
                    {
                        ShowWindow(hWndMsgWindow, SW_SHOW);
                        ShowWindow(hWndCListWindow, SW_SHOW);
                        //MessageBox(NULL, L"done", L"TIM_CALLBACK", MB_OK);
                        IsUpdateInProgress = false;
                        return 0;
                    }
                }
                break;
        }

        IsUpdateInProgress = false;
    }

    return CallWindowProc(oldCListWindowProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK MsgWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!IsUpdateInProgress)
    {
        IsUpdateInProgress = true;

        switch (msg) {
            case WM_SIZE:
            case WM_MOVE:
                SyncMoveTwoWindows(hWndMsgWindow, hWndCListWindow, false);
                break;
            // Активация окна по альт-таб
            case WM_SHOWWINDOW:
                break;
            // Активация по клику
            case WM_ACTIVATE:
                if ((wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE))
                    ActivateCListWindow(hWndMsgWindow, hWndCListWindow, true);
                    //SyncActivateSecondWindow(hWndMsgWindow, hWndCListWindow, true);
                break;
        }

        IsUpdateInProgress = false;
    }

    return CallWindowProc(oldMsgWindowProc, hWnd, msg, wParam, lParam);
}

// end of file