#include "stdafx.h"
#include "AsSingleWindow.h"
#include "Options.h"
#include "..\resource.h"

int InitOptions(WPARAM wParam, LPARAM lParam)
{
    OPTIONSDIALOGPAGE Opts;

    Opts.cbSize = sizeof(Opts);

    Opts.ptszTitle = LPGENT("AsSingleWindow");
    Opts.ptszGroup = LPGENT("Customize");

    Opts.pfnDlgProc = &cbOptionsDialog;
    Opts.pszTemplate = (char *) MAKEINTRESOURCE(IDD_ASW_OPTIONSPAGE);
    Opts.hInstance = pluginVars.hInst;
#ifdef _UNICODE
    Opts.flags = ODPF_BOLDGROUPS | ODPF_TCHAR;
#else
    Opts.flags = ODPF_BOLDGROUPS;
#endif

    CallService(MS_OPT_ADDPAGE, wParam, (LPARAM)(&Opts));

    return 0;
}

INT_PTR CALLBACK cbOptionsDialog(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            dlgProcessInit(hWnd, msg, wParam, lParam);
            break;
        case WM_COMMAND:
            dlgProcessCommand(hWnd, msg, wParam, lParam);
            break;
        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->idFrom == 0) {
                switch (((LPNMHDR)lParam)->code) {
                    case PSN_RESET:
                        optionsLoad();
                        break;
                    case PSN_APPLY:
                        optionsUpdate(hWnd);
                        optionsSave();
                        windowReposition(hWnd); // ���������� ���������� ���������
                        break;
                }
            }
            break;
        case WM_DESTROY:
            // free up resources
            break;
    }

    return false;
}

void dlgProcessInit(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TranslateDialogDefault(hWnd);

    CheckDlgButton(hWnd, IDC_RADIO_G1_RIGHTCL, (pluginVars.Options.DrivenWindowPos == ASW_CLWINDOWPOS_RIGHT));
    CheckDlgButton(hWnd, IDC_RADIO_G1_LEFTCL, (pluginVars.Options.DrivenWindowPos == ASW_CLWINDOWPOS_LEFT));
    CheckDlgButton(hWnd, IDC_RADIO_G1_DONTMERGEWINDOWS, (pluginVars.Options.DrivenWindowPos == ASW_CLWINDOWPOS_DISABLED));

    CheckDlgButton(hWnd, IDC_RADIO_G2_MERGEALL, (pluginVars.Options.WindowsMerging == ASW_WINDOWS_MERGEALL));
    CheckDlgButton(hWnd, IDC_RADIO_G2_MERGEONE, (pluginVars.Options.WindowsMerging == ASW_WINDOWS_MERGEONE));
    CheckDlgButton(hWnd, IDC_RADIO_G2_DISABLEMERGE, (pluginVars.Options.WindowsMerging == ASW_WINDOWS_MERGEDISABLE));

    dlgUpdateControls(hWnd);
}

void dlgProcessCommand(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WORD idCtrl = LOWORD(wParam);
    WORD idNotifyCode = HIWORD(wParam);

    switch (idCtrl)
    {
        case IDC_RADIO_G1_LEFTCL:
        case IDC_RADIO_G1_RIGHTCL:
        case IDC_RADIO_G1_DONTMERGEWINDOWS:
            if (idNotifyCode == BN_CLICKED)
            {
                dlgUpdateControls(hWnd);
                SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
            }
            break;
        case IDC_RADIO_G2_MERGEALL:
        case IDC_RADIO_G2_MERGEONE:
        case IDC_RADIO_G2_DISABLEMERGE:
            if (idNotifyCode == BN_CLICKED)
                SendMessage(GetParent(hWnd), PSM_CHANGED, 0, 0);
            break;
    }
}

void dlgUpdateControls(HWND hWnd)
{
    UINT idState;

    idState = IsDlgButtonChecked(hWnd, IDC_RADIO_G1_DONTMERGEWINDOWS);
    EnableWindow(GetDlgItem(hWnd, IDC_RADIO_G2_MERGEALL), ! idState);
    EnableWindow(GetDlgItem(hWnd, IDC_RADIO_G2_MERGEONE), ! idState);
    EnableWindow(GetDlgItem(hWnd, IDC_RADIO_G2_DISABLEMERGE), ! idState);
}

void optionsLoad()
{
    pluginVars.Options.DrivenWindowPos = DBGetContactSettingByte(0, SETTINGSNAME, "DrivenWindowPosition", ASW_CLWINDOWPOS_RIGHT);
    pluginVars.Options.WindowsMerging = DBGetContactSettingByte(0, SETTINGSNAME, "WindowsMerging", ASW_WINDOWS_MERGEONE);
}

void optionsUpdate(HWND hWnd)
{
    pluginVars.Options.DrivenWindowPos =
        (IsDlgButtonChecked(hWnd, IDC_RADIO_G1_LEFTCL) * ASW_CLWINDOWPOS_LEFT) +
        (IsDlgButtonChecked(hWnd, IDC_RADIO_G1_RIGHTCL) * ASW_CLWINDOWPOS_RIGHT) +
        (IsDlgButtonChecked(hWnd, IDC_RADIO_G1_DONTMERGEWINDOWS) * ASW_CLWINDOWPOS_DISABLED);

    pluginVars.Options.WindowsMerging =
        (IsDlgButtonChecked(hWnd, IDC_RADIO_G2_MERGEALL) * ASW_WINDOWS_MERGEALL) +
        (IsDlgButtonChecked(hWnd, IDC_RADIO_G2_MERGEONE) * ASW_WINDOWS_MERGEONE) +
        (IsDlgButtonChecked(hWnd, IDC_RADIO_G2_DISABLEMERGE) * ASW_WINDOWS_MERGEDISABLE);
}

void optionsSave()
{
    DBWriteContactSettingByte(0, SETTINGSNAME, "DrivenWindowPosition", pluginVars.Options.DrivenWindowPos);
    DBWriteContactSettingByte(0, SETTINGSNAME, "WindowsMerging", pluginVars.Options.WindowsMerging);
}

// end of file