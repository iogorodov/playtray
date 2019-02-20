#include "stdafx.h"
#include "TrayIcon.h"

#define ICONS_IDS { IDI_PLAY, IDC_STOP, IDC_ERROR, IDC_LOADING_01, IDC_LOADING_02, IDC_LOADING_03, IDC_LOADING_04 }

TrayIcon::TrayIcon(UINT iconUid, UINT messageId) :
    _iconUid(iconUid),
    _messageId(messageId),
    _wnd(nullptr),
    _state(PLAY_ICON),
    _phase(0)
{
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
        wcscpy_s(nid.szTip, L"");
        nid.hIcon = GetIcon(state, phase);
        nid.uCallbackMessage = _messageId;

        if (!Shell_NotifyIcon(NIM_ADD, &nid))
            return false;

        return true;
    }

    
}

bool TrayIcon::Init(HINSTANCE instance, HWND wnd)
{
    _wnd = wnd;
    WORD ids[] = ICONS_IDS;

    for (int i = 0; i < ARRAYSIZE(_icons); ++i)
    {
        _icons[i] = LoadIcon(instance, (LPCTSTR)MAKEINTRESOURCE(ids[i]));
    }

    SetIcon(_state, _phase, _tooltip, true);
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
    _state = PLAY_ICON;
    _tooltip = title;
}

void TrayIcon::SetStopIcon(const std::wstring& text, const std::wstring& artist)
{
    _state = STOP_ICON;
    _tooltip = title;
}

void TrayIcon::SetErrorIcon(const std::wstring& text)
{

}

void TrayIcon::SetLoading(int percent)
{

}
