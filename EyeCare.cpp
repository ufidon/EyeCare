// 强制 Unicode
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>      // SHGetSpecialFolderPath
#include <objbase.h>     // CoInitialize, CoCreateInstance
#include <shlguid.h>     // IID_IShellLink, CLSID_ShellLink
#include <shlwapi.h>
#include <strsafe.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")   // COM 需要
#pragma comment(lib, "shlwapi.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// --------------------- 常量 ---------------------
#define WM_TRAYICON (WM_USER + 100)
#define WM_SETTINGS (WM_USER + 101)
#define ID_TIMER_MAIN 1
#define ID_TIMER_REST 2
#define DEFAULT_WORK_MIN  45
#define DEFAULT_REST_MIN  5
#define INI_SECTION L"Settings"

enum LockMode { LOCK_HARD = 0, LOCK_SOFT = 1 };

// 全局变量
int g_workMinutes = DEFAULT_WORK_MIN;
int g_restMinutes = DEFAULT_REST_MIN;
int g_lockMode = LOCK_HARD;
bool g_bAutoStart = false;          // 是否开机启动

enum State { STATE_WORK, STATE_REST };
State g_state = STATE_WORK;
bool g_isPaused = false;

HHOOK g_hKeyboardHook = NULL;
HHOOK g_hMouseHook = NULL;

NOTIFYICONDATA g_nid;
HWND g_hwnd = NULL;
HWND g_hwndRest = NULL;
HWND g_hwndRestStatic = NULL;

DWORD g_startTick = 0;
int   g_totalSeconds = 0;

// 函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK RestWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
void UpdateTrayTip();
void SwitchToWork();
void SwitchToRest();
void ShowSettingsDialog();
bool CreateStartupShortcut(bool enable);   // 创建/删除快捷方式
void LoadSettings();
void SaveSettings();
void GetIniPath(wchar_t* buf, size_t size);
void InstallHooks();
void UninstallHooks();
int GetRemainingSeconds();
void FormatTime(int seconds, wchar_t* buffer, size_t bufsize);

// --------------------- 辅助函数 ---------------------
int GetRemainingSeconds() {
    DWORD elapsed = (GetTickCount() - g_startTick) / 1000;
    int remaining = g_totalSeconds - (int)elapsed;
    return (remaining > 0) ? remaining : 0;
}

void FormatTime(int seconds, wchar_t* buffer, size_t bufsize) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    StringCchPrintfW(buffer, bufsize, L"%02d:%02d", mins, secs);
}

// --------------------- INI 文件操作 ---------------------
void GetIniPath(wchar_t* buf, size_t size) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    StringCchCopyW(buf, size, exePath);
    PathRemoveExtensionW(buf);
    StringCchCatW(buf, size, L".ini");
}

void LoadSettings() {
    wchar_t iniPath[MAX_PATH];
    GetIniPath(iniPath, MAX_PATH);
    wchar_t buf[16];

    // 工作分钟
    GetPrivateProfileStringW(INI_SECTION, L"WorkMinutes", L"", buf, 16, iniPath);
    if (buf[0]) g_workMinutes = StrToIntW(buf);
    else g_workMinutes = DEFAULT_WORK_MIN;

    // 休息分钟
    GetPrivateProfileStringW(INI_SECTION, L"RestMinutes", L"", buf, 16, iniPath);
    if (buf[0]) g_restMinutes = StrToIntW(buf);
    else g_restMinutes = DEFAULT_REST_MIN;

    // 锁屏模式
    GetPrivateProfileStringW(INI_SECTION, L"LockMode", L"0", buf, 16, iniPath);
    g_lockMode = StrToIntW(buf);
    if (g_lockMode < 0 || g_lockMode > 1) g_lockMode = LOCK_HARD;

    // 开机启动
    GetPrivateProfileStringW(INI_SECTION, L"AutoStart", L"0", buf, 16, iniPath);
    g_bAutoStart = (StrToIntW(buf) == 1);
}

void SaveSettings() {
    wchar_t iniPath[MAX_PATH];
    GetIniPath(iniPath, MAX_PATH);
    wchar_t buf[16];

    StringCchPrintfW(buf, 16, L"%d", g_workMinutes);
    WritePrivateProfileStringW(INI_SECTION, L"WorkMinutes", buf, iniPath);

    StringCchPrintfW(buf, 16, L"%d", g_restMinutes);
    WritePrivateProfileStringW(INI_SECTION, L"RestMinutes", buf, iniPath);

    StringCchPrintfW(buf, 16, L"%d", g_lockMode);
    WritePrivateProfileStringW(INI_SECTION, L"LockMode", buf, iniPath);

    StringCchPrintfW(buf, 16, L"%d", g_bAutoStart ? 1 : 0);
    WritePrivateProfileStringW(INI_SECTION, L"AutoStart", buf, iniPath);
}

// --------------------- 开机启动（启动文件夹快捷方式） ---------------------
bool CreateStartupShortcut(bool enable) {
    WCHAR startupPath[MAX_PATH];
    if (SHGetSpecialFolderPathW(NULL, startupPath, CSIDL_STARTUP, FALSE) == FALSE)
        return false;

    WCHAR shortcutPath[MAX_PATH];
    StringCchPrintfW(shortcutPath, MAX_PATH, L"%s\\EyeCare.lnk", startupPath);

    if (enable) {
        HRESULT hr = CoInitialize(NULL);
        if (FAILED(hr)) return false;

        IShellLink* psl = NULL;
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl);
        bool ok = false;
        if (SUCCEEDED(hr)) {
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            psl->SetPath(exePath);
            psl->SetWorkingDirectory(exePath);

            IPersistFile* ppf = NULL;
            hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
            if (SUCCEEDED(hr)) {
                hr = ppf->Save(shortcutPath, TRUE);
                ok = SUCCEEDED(hr);
                ppf->Release();
            }
            psl->Release();
        }
        CoUninitialize();
        return ok;
    } else {
        return DeleteFileW(shortcutPath) != 0;
    }
}

// --------------------- 入口 ---------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 加载设置
    LoadSettings();

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"EyeCareClass";
    if (!RegisterClass(&wc)) return 1;

    g_hwnd = CreateWindow(L"EyeCareClass", L"EyeCare", WS_OVERLAPPEDWINDOW,
                          0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!g_hwnd) return 1;

    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    StringCchCopyW(g_nid.szTip, 128, L"护眼助手 (工作中)");
    Shell_NotifyIcon(NIM_ADD, &g_nid);

    // 如果设置了开机启动，但快捷方式不存在，这里不自动创建，由用户通过设置控制
    // （我们只会在设置确定时创建/删除）

    g_state = STATE_WORK;
    g_totalSeconds = g_workMinutes * 60;
    g_startTick = GetTickCount();
    SetTimer(g_hwnd, ID_TIMER_MAIN, 1000, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 退出前保存设置
    SaveSettings();

    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    UninstallHooks();
    if (g_hwndRest) DestroyWindow(g_hwndRest);
    return 0;
}

// --------------------- 主窗口过程 ---------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_TIMER:
            if (g_isPaused) break;
            if (wParam == ID_TIMER_MAIN) {
                int remaining = GetRemainingSeconds();
                UpdateTrayTip();
                if (remaining <= 0) {
                    if (g_state == STATE_WORK) SwitchToRest();
                    else SwitchToWork();
                }
            }
            break;

        case WM_TRAYICON:
            if (LOWORD(lParam) == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, 1, L"设置");
                AppendMenu(hMenu, MF_STRING, 2, L"立即休息");
                AppendMenu(hMenu, MF_STRING, 3, L"退出");
                SetForegroundWindow(hwnd);
                POINT pt;
                GetCursorPos(&pt);
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
                switch (cmd) {
                    case 1: ShowSettingsDialog(); break;
                    case 2:
                        if (g_state == STATE_WORK) {
                            KillTimer(hwnd, ID_TIMER_MAIN);
                            SwitchToRest();
                        }
                        break;
                    case 3: PostQuitMessage(0); break;
                }
            }
            break;

        case WM_SETTINGS:
            g_totalSeconds = (g_state == STATE_WORK) ? g_workMinutes * 60 : g_restMinutes * 60;
            g_startTick = GetTickCount();
            SetTimer(hwnd, ID_TIMER_MAIN, 1000, NULL);
            UpdateTrayTip();
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// --------------------- 状态切换 ---------------------
void SwitchToWork() {
    if (g_hwndRest) {
        DestroyWindow(g_hwndRest);
        g_hwndRest = NULL;
        g_hwndRestStatic = NULL;
    }
    UninstallHooks();

    g_state = STATE_WORK;
    g_totalSeconds = g_workMinutes * 60;
    g_startTick = GetTickCount();
    SetTimer(g_hwnd, ID_TIMER_MAIN, 1000, NULL);
    UpdateTrayTip();
    StringCchCopyW(g_nid.szTip, 128, L"护眼助手 (工作中)");
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

void SwitchToRest() {
    KillTimer(g_hwnd, ID_TIMER_MAIN);
    g_state = STATE_REST;
    g_totalSeconds = g_restMinutes * 60;
    g_startTick = GetTickCount();

    if (g_lockMode == LOCK_HARD) {
        InstallHooks();
    } else {
        UninstallHooks();
    }

    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(g_hwnd, GWLP_HINSTANCE);
    WNDCLASS wc = {};
    wc.lpfnWndProc = RestWndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_DESKTOP + 1);
    wc.lpszClassName = L"RestWindowClass";
    RegisterClass(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    DWORD dwStyle = WS_POPUP | WS_VISIBLE;
    DWORD dwExStyle = WS_EX_TOPMOST;

    if (g_lockMode == LOCK_HARD) {
        g_hwndRest = CreateWindowEx(dwExStyle, L"RestWindowClass", L"休息一下",
                                    dwStyle, 0, 0, screenW, screenH,
                                    NULL, NULL, hInst, NULL);
    } else {
        dwExStyle |= WS_EX_TRANSPARENT | WS_EX_LAYERED;
        int winW = 400, winH = 200;
        int x = (screenW - winW) / 2;
        int y = (screenH - winH) / 2;
        g_hwndRest = CreateWindowEx(dwExStyle, L"RestWindowClass", L"休息提醒",
                                    dwStyle, x, y, winW, winH,
                                    NULL, NULL, hInst, NULL);
        if (g_hwndRest) {
            SetLayeredWindowAttributes(g_hwndRest, 0, (BYTE)180, LWA_ALPHA);
        }
    }

    if (g_hwndRest) {
            g_hwndRestStatic = CreateWindowEx(WS_EX_TRANSPARENT, L"STATIC", L"",
                                        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
                                        0, 0, 400, 100,
                                        g_hwndRest, NULL, hInst, NULL);
        HFONT hFont = CreateFont(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, L"Arial");
        SendMessage(g_hwndRestStatic, WM_SETFONT, (WPARAM)hFont, TRUE);

        RECT rc;
        GetClientRect(g_hwndRest, &rc);
        SetWindowPos(g_hwndRestStatic, NULL, (rc.right - 400)/2, (rc.bottom - 100)/2,
                     400, 100, SWP_NOZORDER);

        wchar_t buf[64];
        int remaining = GetRemainingSeconds();
        FormatTime(remaining, buf, 64);
        wchar_t display[128];
        StringCchPrintfW(display, 128, L"休息倒计时\n%s", buf);
        SetWindowText(g_hwndRestStatic, display);

        // 定时器绑定到休息窗口
        SetTimer(g_hwndRest, ID_TIMER_REST, 1000, NULL);
        SetFocus(g_hwndRest);
    }

    UpdateTrayTip();
    StringCchCopyW(g_nid.szTip, 128, L"护眼助手 (休息中)");
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

// --------------------- 休息窗口过程 ---------------------
LRESULT CALLBACK RestWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCHITTEST:
            if (g_lockMode == LOCK_SOFT) {
                if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) {
                    return HTCAPTION;
                }
                return HTTRANSPARENT;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_TIMER:
            if (wParam == ID_TIMER_REST) {
                int remaining = GetRemainingSeconds();
                wchar_t buf[64];
                FormatTime(remaining, buf, 64);
                wchar_t display[128];
                StringCchPrintfW(display, 128, L"休息倒计时\n%s", buf);
                if (g_hwndRestStatic) SetWindowText(g_hwndRestStatic, display);
                UpdateTrayTip();
                if (remaining <= 0) {
                    KillTimer(hwnd, ID_TIMER_REST);
                    SwitchToWork();
                }
            }
            break;

        case WM_CLOSE:
            return 0;

        case WM_DESTROY:
            g_hwndRest = NULL;
            g_hwndRestStatic = NULL;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// --------------------- 钩子回调 ---------------------
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) return 1;
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) return 1;
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void InstallHooks() {
    if (!g_hKeyboardHook)
        g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                                           GetModuleHandle(NULL), 0);
    if (!g_hMouseHook)
        g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc,
                                        GetModuleHandle(NULL), 0);
}

void UninstallHooks() {
    if (g_hKeyboardHook) { UnhookWindowsHookEx(g_hKeyboardHook); g_hKeyboardHook = NULL; }
    if (g_hMouseHook)    { UnhookWindowsHookEx(g_hMouseHook);    g_hMouseHook = NULL; }
}

// --------------------- 托盘提示更新 ---------------------
void UpdateTrayTip() {
    wchar_t buf[128];
    int remaining = GetRemainingSeconds();
    wchar_t timeStr[16];
    FormatTime(remaining, timeStr, 16);
    if (g_state == STATE_WORK)
        StringCchPrintfW(buf, 128, L"护眼助手 - 距离休息还有 %s", timeStr);
    else
        StringCchPrintfW(buf, 128, L"护眼助手 - 休息倒计时 %s", timeStr);
    StringCchCopyW(g_nid.szTip, 128, buf);
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

// --------------------- 设置对话框 ---------------------
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            SetDlgItemInt(hDlg, 1001, g_workMinutes, FALSE);
            SetDlgItemInt(hDlg, 1002, g_restMinutes, FALSE);
            HWND hCombo = GetDlgItem(hDlg, 1003);
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"硬锁屏（锁定输入）");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"软锁屏（仅提醒）");
            SendMessage(hCombo, CB_SETCURSEL, g_lockMode, 0);

            // 复选框
            CheckDlgButton(hDlg, 1004, g_bAutoStart ? BST_CHECKED : BST_UNCHECKED);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    BOOL trans;
                    int work = GetDlgItemInt(hDlg, 1001, &trans, FALSE);
                    int rest = GetDlgItemInt(hDlg, 1002, &trans, FALSE);
                    if (work < 1 || work > 120 || rest < 1 || rest > 30) {
                        MessageBox(hDlg, L"请输入合理数值 (工作1-120分，休息1-30分)", L"错误", MB_OK);
                        return TRUE;
                    }
                    g_workMinutes = work;
                    g_restMinutes = rest;
                    g_lockMode = (int)SendMessage(GetDlgItem(hDlg, 1003), CB_GETCURSEL, 0, 0);

                    // 开机启动复选框
                    bool newAutoStart = (IsDlgButtonChecked(hDlg, 1004) == BST_CHECKED);
                    if (newAutoStart != g_bAutoStart) {
                        g_bAutoStart = newAutoStart;
                        CreateStartupShortcut(g_bAutoStart);
                    }

                    // 保存所有设置到INI
                    SaveSettings();

                    PostMessage(g_hwnd, WM_SETTINGS, 0, 0);
                    EndDialog(hDlg, IDOK);
                    break;
                }
                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
            }
            return TRUE;
    }
    return FALSE;
}

void ShowSettingsDialog() {
    g_isPaused = true;
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(1000), g_hwnd, SettingsDlgProc);
    g_isPaused = false;
}