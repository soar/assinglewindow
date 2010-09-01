
//#define _MT
//#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <newpluginapi.h>
#include <m_system.h>
//#include <m_langpack.h>
#include <m_database.h>
#include <m_message.h>
#include <m_clist.h>
#include <m_clui.h>

HINSTANCE hInst;
PLUGINLINK *pluginLink;

bool IsUpdateInProgress;

HANDLE hookModulesLoaded;
HANDLE hookMsgWindow;

HWND hWndCListWindow;
HWND hWndMsgWindow;

WNDPROC oldCListWindowProc;
WNDPROC oldMsgWindowProc;

PLUGININFOEX pluginInfo = {
    sizeof(PLUGININFOEX),
#ifdef UNICODE
    "AsSingleWindow Mode Plugin (Unicode)",
#else
    "AsSingleWindow Mode Plugin",
#endif
    PLUGIN_MAKE_VERSION(0, 0, 0, 4),
    "Allow you work with contact-list and chat windows as with one single window.",
    "Aleksey Smyrnov aka Soar",
    "to-me@soar.name",
    "© 2010 http://soar.name",
    "http://soar.name/",
    UNICODE_AWARE,
    0,
#ifdef UNICODE
    {0xF6C73B4, 0x2B2B, 0x711D, {0xFB, 0xB6, 0xBB, 0x26, 0x7D, 0xFD, 0x72, 0x08}}, // 0xF6C73B42B2B711DFBB6BB267DFD7208
#else
    {0xF6C73B4, 0x2B2B, 0x711D, {0xFB, 0xB6, 0xBB, 0x26, 0x7D, 0xFD, 0x72, 0x09}}, // 0xF6C73B42B2B711DFBB6BB267DFD7209
#endif
};

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

    //MoveWindow(hWndDst, x, y, w, h, true);
    //ShowWindow(hWndDst, SW_SHOW);
    SetWindowPos(hWndDst, hWndSrc, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void SyncActivateSecondWindow(HWND hWndSrc, HWND hWndDst, bool bNeedShow)
{
    if ((hWndSrc == 0) || (hWndDst == 0))
        return;

    //ShowWindow(hWndDst, IsShow ? SW_SHOW : SW_HIDE);
    SetWindowPos(hWndDst, hWndSrc, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | (IsShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

LRESULT CALLBACK CListWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!IsUpdateInProgress)
    {
        IsUpdateInProgress = true;

        switch (msg) {
            case WM_SIZE:
            case WM_MOVE:
                SyncMoveTwoWindows(hWndCListWindow, hWndMsgWindow, true);
                break;
            case WM_SHOWWINDOW:
                SyncActivateSecondWindow(hWndCListWindow, hWndMsgWindow, wParam ? true : false);
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
            case WM_SHOWWINDOW:
                break;
            case WM_ACTIVATE:
                if ((wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE))
                    SyncActivateSecondWindow(hWndMsgWindow, hWndCListWindow, true);
                break;
        }

        IsUpdateInProgress = false;
    }

    return CallWindowProc(oldMsgWindowProc, hWnd, msg, wParam, lParam);
}

// end of file