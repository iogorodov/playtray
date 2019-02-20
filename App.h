#pragma once

#include "TrayIcon.h"
#include "Config.h"
#include "Player.h"

class App : Player::ICallbacks
{
    HINSTANCE _instance;

    TrayIcon _trayIcon;
    Config _config;
    Player _player;

    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool AddMenuItem(HMENU menu, UINT position, UINT id, LPWSTR title, BOOL disabled, BOOL checked);
    bool ShowTrayMenu(HWND wnd);

public:
    App(HINSTANCE instance);

    int Run();

    void OnBuffer(int percent) override;
    void OnPlay(const std::wstring& name, const std::wstring& url) override;
    void OnMeta(const std::wstring& text, const std::wstring& artist) override;
    void OnStall() override;
    void OnEnd() override;
    void OnError(int errorCode) override;
};
