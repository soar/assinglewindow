#include "stdafx.h"
#include "AsSingleWindow.h"
#include "Options.h"
#include "WindowsManager.h"

PLUGINLINK *pluginLink;
PLUGININFOEX pluginInfo = {
    sizeof(pluginInfo), // PLUGININFOEX
#ifdef _UNICODE
    "AsSingleWindow Mode Plugin (Unicode)",
#else
    "AsSingleWindow Mode Plugin",
#endif
    PLUGIN_MAKE_VERSION(0, 1, 0, 2),
    "Allow you work with contact-list and chat windows as with one single window.",
    "Aleksey Smyrnov aka Soar",
    "i@soar.name",
    "(c) Soar, 2010-2011",
    "http://soar.name/tag/assinglewindow/",
    UNICODE_AWARE,
    0,
#ifdef _UNICODE
    {0xF6C73B4, 0x2B2B, 0x711D, {0xFB, 0xB6, 0xBB, 0x26, 0x7D, 0xFD, 0x72, 0x08}}, // 0xF6C73B42B2B711DFBB6BB267DFD7208
#else
    {0xF6C73B4, 0x2B2B, 0x711D, {0xFB, 0xB6, 0xBB, 0x26, 0x7D, 0xFD, 0x72, 0x09}}, // 0xF6C73B42B2B711DFBB6BB267DFD7209
#endif
};

sPluginVars pluginVars;

extern "C" bool WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    pluginVars.hInst = hInstDLL;
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

    ::InitializeCriticalSection(&pluginVars.m_CS);
    pluginVars.IsUpdateInProgress = false;
    pluginVars.heModulesLoaded = HookEvent(ME_SYSTEM_MODULESLOADED, OnModulesLoaded);
    pluginVars.heOptionsLoaded = HookEvent(ME_OPT_INITIALISE, InitOptions);
    
    return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
    UnhookEvent(pluginVars.heOptionsLoaded);
    UnhookEvent(pluginVars.heModulesLoaded);

    ::DeleteCriticalSection(&pluginVars.m_CS);

    return 0;
}

int OnModulesLoaded(WPARAM wParam, LPARAM lParam)
{
    HWND hWndCListWindow = (HWND) CallService(MS_CLUI_GETHWND, 0, 0);
    windowAdd(hWndCListWindow, true);

    pluginVars.heMsgWndEvent = HookEvent(ME_MSG_WINDOWEVENT, MsgWindowEvent);

    optionsLoad();

    return 0;
}


int MsgWindowEvent(WPARAM wParam, LPARAM lParam)
{
    MessageWindowEventData* data = (MessageWindowEventData*) lParam;

    if ((data == NULL) || (data->cbSize < sizeof(MessageWindowEventData)))
        return 0;

    switch (data->uType)
    {
        case MSG_WINDOW_EVT_OPEN:
            windowAdd(data->hwndWindow, false);            
            break;
        // Не здесь ! Может быть закрытием дочернего окна
        case MSG_WINDOW_EVT_CLOSE:
            if (IsWindow(data->hwndWindow))
                UINT i = 1;
            else
                UINT i = 2;
            break;
    }

    return 0;
}

// end of file