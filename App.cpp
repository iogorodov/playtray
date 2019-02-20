#include "stdafx.h"

#include "App.h"

static const WCHAR* WINDOW_CLASS = L"PLAYTRAY_WND_CLASS";

#define MAX_LOADSTRING 128

#define WM_TASKBAR_CREATE RegisterWindowMessage(_T("TaskbarCreated"))

#define SHELL_ICON_UID 100
#define	WM_USER_SHELLICON WM_USER + 1

#define TIMER_ID 0

LRESULT CALLBACK App::StaticWndProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        SetWindowLong(wnd, GWL_USERDATA, (long)((LPCREATESTRUCT(lParam))->lpCreateParams));
    }

    App* app = (App*)GetWindowLong(wnd, GWL_USERDATA);

    if (app)
        return app->WndProc(wnd, message, wParam, lParam);
    else
        return DefWindowProc(wnd, message, wParam, lParam);
}

App::App(HINSTANCE instance) :
    _instance(nullptr),
    _trayIcon(SHELL_ICON_UID, WM_USER_SHELLICON),
    _player(this),
    _hasTimer(false)
{
    WCHAR titleStr[MAX_LOADSTRING];
    LoadStringW(instance, IDS_APP_TITLE, titleStr, MAX_LOADSTRING);


    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_UPDOWN_CLASS | ICC_LISTVIEW_CLASSES;

    if (!InitCommonControlsEx(&iccex))
        return;


    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = App::StaticWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_RADIOTRAY));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(WHITE_BRUSH);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = WINDOW_CLASS;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_RADIOTRAY));

    if (!RegisterClassExW(&wcex))
        return;


    HWND wnd = CreateWindowExW(WS_EX_CLIENTEDGE, WINDOW_CLASS, titleStr, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance, (LPVOID)this);

    if (!wnd)
        return;

    if (!_trayIcon.Init(instance, wnd))
        return;

    _config.Read();
    if (!_config.GetItems().empty())
        _trayIcon.SetPlayIcon(_config.GetItems()[_config.GetCurrentIndex()]);

    _player.Init(wnd);
    _instance = instance;
    _wnd = wnd;
}

bool App::AddMenuItem(HMENU menu, UINT position, UINT id, LPWSTR title, BOOL disabled, BOOL checked)
{
    MENUITEMINFOW mi;
    mi.cbSize = sizeof(MENUITEMINFOW);
    mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING;
    mi.fType = MFT_STRING;
    mi.fState = disabled ? MFS_DISABLED : (checked ? MFS_DEFAULT : MFS_ENABLED);
    mi.wID = id;
    mi.dwTypeData = title;
    mi.cch = wcslen(title);

    if (!InsertMenuItemW(menu, position, TRUE, &mi))
        return false;

    return true;
}

bool App::ShowTrayMenu(HWND wnd)
{
    HMENU menu = LoadMenu(_instance, MAKEINTRESOURCE(IDC_RADIOTRAY));
    if (!menu)
        return false;

    HMENU subMenu = GetSubMenu(menu, 0);
    if (!subMenu) {
        DestroyMenu(menu);
        return false;
    }

    if (_config.GetItems().empty())
    {
        WCHAR noConfigStr[MAX_LOADSTRING];
        LoadStringW(_instance, IDS_NO_CONFIG, noConfigStr, MAX_LOADSTRING);

        AddMenuItem(subMenu, 0, 0, noConfigStr, true, false);
    }
    else
    {
        UINT position = 0;
        for (auto& it : _config.GetItems())
        {
            AddMenuItem(subMenu, position, IDM_MENU_ITEM + position, (LPWSTR)it.c_str(), false, position == _config.GetCurrentIndex());
            position += 1;
        }
    }

    POINT clickPoint;
    GetCursorPos(&clickPoint);

    SetForegroundWindow(wnd);
    TrackPopupMenu(subMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, clickPoint.x, clickPoint.y, 0, wnd, NULL);
    SendMessage(wnd, WM_NULL, 0, 0);

    DestroyMenu(menu);
    return true;
}

void App::StartTimer()
{
    if (_hasTimer)
        return;
    SetTimer(_wnd, TIMER_ID, USER_TIMER_MINIMUM, 0);
    _hasTimer = true;
}

void App::StopTimer()
{
    KillTimer(_wnd, TIMER_ID);
    _hasTimer = false;
}


int App::Run()
{
    if (!_instance)
        return 1;

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

long App::WndProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_TASKBAR_CREATE)
    {
        if (!_trayIcon.Init(_instance, wnd))
            return -1;
    }

    switch (message)
    {
        case WM_COMMAND:
            {
                int commandId = LOWORD(wParam);
                if (commandId >= IDM_MENU_ITEM)
                {
                    size_t position = commandId - IDM_MENU_ITEM;
                    if (_config.SetCurrentIndex(position))
                    {
                        _player.Play(_config.GetUrl(position));
                        _trayIcon.SetLoading();
                        StartTimer();
                    }
                }
                else
                {
                    switch (commandId)
                    {
                    case IDM_EXIT:
                        DestroyWindow(wnd);
                        break;
                    default:
                        return DefWindowProc(wnd, message, wParam, lParam);
                    }
                }
            }
        break;
        case WM_TIMER:
            if (!_player.Update())
                _trayIcon.UpdateLoading();
            break;
        case WM_USER_SHELLICON:
            switch (LOWORD(lParam))
            {
                case WM_LBUTTONDOWN:
                    if (_player.IsPlaying())
                    {
                        _player.Stop();
                    }
                    else if (!_config.GetItems().empty())
                    {
                        _player.Play(_config.GetUrl(_config.GetCurrentIndex()));
                        _trayIcon.SetLoading();
                        StartTimer();
                    }
                    break;
                case WM_RBUTTONDOWN:
                    if (!ShowTrayMenu(wnd))
                        return -1;
                    break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(wnd);
            break;
        
        case WM_DESTROY:
            _player.Destroy();
            _trayIcon.RemoveIcon();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(wnd, message, wParam, lParam);
    }
    return 0;
}

void App::OnBuffer(int percent)
{
    _trayIcon.UpdateLoading(percent);
}

void App::OnPlay(const std::wstring& name, const std::wstring& url)
{
    _trayIcon.SetStopIcon(url);
    StopTimer();
}

void App::OnMeta(const std::wstring& text, const std::wstring& artist)
{
    StopTimer();
    if (artist.empty())
    {
        _trayIcon.SetStopIcon(text);
    }
    else
    {
        _trayIcon.SetStopIcon(artist + L" - " + text);
    }
}

void App::OnStall()
{
    _trayIcon.SetLoading();
    StartTimer();
}

void App::OnEnd()
{
    StopTimer();
    if (_config.GetItems().empty())
    {
        _trayIcon.SetPlayIcon(std::wstring());
    }
    else
    {
        _trayIcon.SetPlayIcon(_config.GetItems()[_config.GetCurrentIndex()]);
    }
}

void App::OnError(int errorCode)
{
    StopTimer();
    _trayIcon.SetErrorIcon(errorCode);
}

