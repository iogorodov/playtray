#pragma once

class TrayIcon
{
    UINT _iconUid;
    UINT _messageId;

    HWND _wnd;
public:
    TrayIcon(UINT iconUid, UINT messageId);
    ~TrayIcon();

    bool CreateIcon(HINSTANCE instance, HWND wnd);
    void RemoveIcon();
};

