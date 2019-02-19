#pragma once

class Player;

class Player
{
public:
    struct ICallbacks
    {
        virtual void OnBuffer(int percent) = 0;
        virtual void OnPlay(const std::wstring& name, const std::wstring& url) = 0;
        virtual void OnMeta(const std::wstring& text, const std::wstring& artist) = 0;
        virtual void OnStall() = 0;
        virtual void OnEnd() = 0;
        virtual void OnError(int errorCode) = 0;
    };

private:
    struct UrlParams
    {
        ::Player* Player;
        std::wstring Url;

        UrlParams(::Player* player, const std::wstring& url) :
            Player(player),
            Url(url)
        {
        }
    };

    ICallbacks* _callbacks;

    CRITICAL_SECTION _lock;
    int _request;
    HSTREAM _stream;

    bool _ready;

    static void __cdecl StaticOpenUrl(void* params);

    static void CALLBACK StaticMetaSync(HSYNC handle, DWORD channel, DWORD data, void *user);
    static void CALLBACK StaticStallSync(HSYNC handle, DWORD channel, DWORD data, void *user);
    static void CALLBACK StaticEndSync(HSYNC handle, DWORD channel, DWORD data, void *user);
    static void CALLBACK StaticStatusProc(const void *buffer, DWORD length, void *user);

    void OpenUrl(const std::wstring& url);
    void ProcessMeta();

public:
    Player(ICallbacks* callback);
    ~Player();

    bool Init(HWND wnd);
    void Destroy();

    void Play(const std::wstring& url);
    void Stop();

    bool Update();
};

