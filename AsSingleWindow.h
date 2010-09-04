// #include <map>

#include <windows.h>

#include <newpluginapi.h>
#include <m_system.h>
#include <m_langpack.h>
#include <m_database.h>
#include <m_message.h>
#include <m_clist.h>
#include <m_clistint.h>
#include <m_clui.h>

enum WindowState {
    WINDOW_STATE_NORMAL = 0,
    WINDOW_STATE_MINIMIZED = 1,
    WINDOW_STATE_HIDEN = 2,
};

struct WindowInfo {
    HWND hWnd;
    WindowState eState;
    WNDPROC pPrevWndProc;
};

// std::map<HWND, WindowInfo> MsgWindows; - for future development

int cListActivations = 0;

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
    PLUGIN_MAKE_VERSION(0, 0, 0, 5),
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

// end of file