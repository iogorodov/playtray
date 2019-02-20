#pragma once

class TrayIcon
{
    enum State
    {
        PLAY_ICON,
        STOP_ICON,
        ERROR_ICON,
        LOADING_ICON
    };

    UINT _iconUid;
    UINT _messageId;

    HWND _wnd;

    State _state;
    int _phase;
    std::wstring _tooltip;

    HICON _icons[7];

    HICON GetIcon(State state, int phase);
    bool SetIcon(State state, int phase, const std::wstring& tooltip, bool create);
public:
    TrayIcon(UINT iconUid, UINT messageId);
    ~TrayIcon();

    bool Init(HINSTANCE instance, HWND wnd);
    void RemoveIcon();

    void SetPlayIcon(const std::wstring& title);
    void SetStopIcon(const std::wstring& text);
    void SetErrorIcon(int errorCode);
    void SetLoading(int percent);
};

