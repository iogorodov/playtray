#include "stdafx.h"
#include "TrayIcon.h"

TrayIcon::TrayIcon(UINT iconUid, UINT messageId) :
    _iconUid(iconUid),
    _messageId(messageId),
    _wnd(nullptr)
{
}

TrayIcon::~TrayIcon()
{
    RemoveIcon();
}

bool TrayIcon::CreateIcon(HINSTANCE instance, HWND wnd)
{
    _wnd = wnd;

    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = wnd;
    nid.uID = _iconUid;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    wcscpy_s(nid.szTip, L"");
    nid.hIcon = LoadIcon(instance, (LPCTSTR)MAKEINTRESOURCE(IDI_RADIOTRAY));
    nid.uCallbackMessage = _messageId;

    // Display tray icon
    if (!Shell_NotifyIcon(NIM_ADD, &nid))
    {
        _wnd = nullptr;
        return false;
    }

    return true;
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