#include "stdafx.h"
#include "TrayIcon.h"

#define MAX_LOADSTRING 128
#define ICONS_IDS { IDI_PLAY, IDI_STOP, IDI_PLAY_ERROR, IDI_LOADING_01, IDI_LOADING_02, IDI_LOADING_03, IDI_LOADING_04 }

std::wstring LoadStringFromResource(UINT id, HINSTANCE instance = NULL)
{
    WCHAR* buffer = NULL;
    int len = LoadStringW(instance, id, reinterpret_cast<LPWSTR>(&buffer), 0);

    if (len)
        return std::wstring(buffer, len);
    else
        return std::wstring();
}

TrayIcon::TrayIcon(UINT iconUid, UINT messageId) :
    _iconUid(iconUid),
    _messageId(messageId),
    _wnd(nullptr),
    _state(PLAY_ICON),
    _phase(0)
{
    _tooltip = LoadStringFromResource(IDS_APP_TITLE);
}

TrayIcon::~TrayIcon()
{
    RemoveIcon();
}


HICON TrayIcon::GetIcon(State state, int phase)
{
    switch (state)
    {
    case PLAY_ICON:
        return _icons[0];
    case STOP_ICON:
        return _icons[1];
    case ERROR_ICON:
        return _icons[2];
    case LOADING_ICON:
        return _icons[phase % 4 + 3];
    }

    return 0;
}

bool TrayIcon::SetIcon(State state, int phase, const std::wstring& tooltip, bool create)
{
    if (create)
    {
        NOTIFYICONDATA nid;
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = _wnd;
        nid.uID = _iconUid;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        wcscpy_s(nid.szTip, tooltip.c_str());
        nid.hIcon = GetIcon(state, phase);
        nid.uCallbackMessage = _messageId;

        if (!Shell_NotifyIcon(NIM_ADD, &nid))
            return false;
    }
    else if (state != _state || (_state == LOADING_ICON && phase != _phase))
    {
        NOTIFYICONDATA nid;
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = _wnd;
        nid.uID = _iconUid;
        nid.uFlags = NIF_ICON | NIF_TIP;
        wcscpy_s(nid.szTip, tooltip.c_str());
        nid.hIcon = GetIcon(state, phase);

        if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
            return false;
    }
    else
    {
        NOTIFYICONDATA nid;
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = _wnd;
        nid.uID = _iconUid;
        nid.uFlags = NIF_TIP;
        wcscpy_s(nid.szTip, tooltip.c_str());

        if (!Shell_NotifyIcon(NIM_MODIFY, &nid))
            return false;
    }

    _state = state;
    _phase = phase;
    _tooltip = tooltip;
    return true;
}

bool TrayIcon::Init(HINSTANCE instance, HWND wnd)
{
    _wnd = wnd;
    WORD ids[] = ICONS_IDS;

    for (int i = 0; i < _ARRAYSIZE(_icons); ++i)
    {
        _icons[i] = LoadIcon(instance, (LPCTSTR)MAKEINTRESOURCE(ids[i]));
    }

    return SetIcon(_state, _phase, _tooltip, true);
}

void TrayIcon::RemoveIcon()
{
    if (!_wnd)
        return;

    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = _wnd;
    nid.uID = _iconUid;

    Shell_NotifyIcon(NIM_DELETE, &nid);
    _wnd = nullptr;
}

void TrayIcon::SetPlayIcon(const std::wstring& title)
{
    if (title.empty())
    {
        SetIcon(PLAY_ICON, 0, LoadStringFromResource(IDS_APP_TITLE), false);
    }
    else
    {
        SetIcon(PLAY_ICON, 0, LoadStringFromResource(IDS_APP_TITLE) + L" - " + title, false);
    }
}

void TrayIcon::SetStopIcon(const std::wstring& text)
{
    SetIcon(STOP_ICON, 0, text, false);
}

void TrayIcon::SetErrorIcon(int errorCode)
{
    SetIcon(ERROR_ICON, 0, LoadStringFromResource(errorCode - BASS_OK + IDS_BASS_OK), false);
}

void TrayIcon::SetLoading(int percent)
{
    std::wstring empty;
    SetIcon(LOADING_ICON, 0, empty, false);
}
