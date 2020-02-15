#include "stdafx.h"

#include <process.h>
#include "Player.h"

// reference additional headers your program requires here
#pragma comment(lib, "bass/bass.lib")

// HLS definitions (copied from BASSHLS.H)
#define BASS_SYNC_HLS_SEGMENT	0x10300
#define BASS_TAG_HLS_EXTINF		0x14000

void __cdecl Player::StaticOpenUrl(void* params)
{
    ((UrlParams*)params)->Player->OpenUrl(((UrlParams*)params)->Url);
    delete params;
}

void CALLBACK Player::StaticMetaSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
    ((Player*)user)->ProcessMeta();
}

void CALLBACK Player::StaticStallSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
    if (!data)
        ((Player*)user)->_callbacks->OnStall();
}

void CALLBACK Player::StaticEndSync(HSYNC handle, DWORD channel, DWORD data, void *user)
{
    ((Player*)user)->Stop();
}

void CALLBACK Player::StaticStatusProc(const void *buffer, DWORD length, void *user)
{
}

Player::Player(Player::ICallbacks* callbacks) :
    _callbacks(callbacks),
    _request(0),
    _stream(0),
    _ready(false),
    _wnd(NULL)
{
}


Player::~Player()
{
    Destroy();
}

bool Player::Init(HWND wnd)
{
    _wnd = wnd;
    if (HIWORD(BASS_GetVersion()) != BASSVERSION)
    {
        _callbacks->OnError(BASS_ERROR_VERSION);
        return false;
    }

    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing
    BASS_SetConfig(BASS_CONFIG_NET_PREBUF_WAIT, 0); // disable BASS_StreamCreateURL pre-buffering

    if (!BASS_Init(-1, 44100, 0, _wnd, NULL))
    {
        _callbacks->OnError(BASS_ErrorGetCode());
        return false;
    }

    InitializeCriticalSection(&_lock);
    _ready = true;
    return true;
}

void Player::Destroy()
{
    if (!_ready)
        return;

    Stop();
    BASS_Free();
    _ready = false;
}

void Player::Play(const std::wstring& url)
{
    UrlParams* params = new UrlParams(this, url);
    _beginthread(Player::StaticOpenUrl, 0, params);
}

void Player::Stop() 
{
    if (_stream)
        BASS_StreamFree(_stream);
    _callbacks->OnEnd();
    _stream = 0;
}

void Player::OpenUrl(const std::wstring& url)
{
    EnterCriticalSection(&_lock); // make sure only 1 thread at a time can do the following
    int request = ++_request;
    LeaveCriticalSection(&_lock);

    BASS_StreamFree(_stream);

    // Reset output device to default. Hust to be sure
    BASS_Free();
    if (!BASS_Init(-1, 44100, 0, _wnd, NULL))
    {
        _callbacks->OnError(BASS_ErrorGetCode());
        return;
    }

    // Connecting
    HSTREAM stream = BASS_StreamCreateURL((WCHAR*)url.c_str(), 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, Player::StaticStatusProc, this);

    EnterCriticalSection(&_lock);
    if (request != _request)
    {
        LeaveCriticalSection(&_lock);
        if (stream)
            BASS_StreamFree(stream);

        return;
    }

    _stream = stream;
    LeaveCriticalSection(&_lock);

    if (!_stream)
    {
        _callbacks->OnError(BASS_ErrorGetCode());
    }
    else {
        BASS_ChannelSetSync(_stream, BASS_SYNC_META, 0, Player::StaticMetaSync, this); // Shoutcast
        BASS_ChannelSetSync(_stream, BASS_SYNC_OGG_CHANGE, 0, Player::StaticMetaSync, this); // Icecast/OGG
        BASS_ChannelSetSync(_stream, BASS_SYNC_HLS_SEGMENT, 0, Player::StaticMetaSync, this); // HLS

        BASS_ChannelSetSync(_stream, BASS_SYNC_STALL, 0, Player::StaticStallSync, this);
        BASS_ChannelSetSync(_stream, BASS_SYNC_END, 0, Player::StaticEndSync, this);

        BASS_ChannelPlay(_stream, FALSE);
    }
}

void Player::ProcessMeta()
{
    std::wstring empty;

    const char *meta = BASS_ChannelGetTags(_stream, BASS_TAG_META);
    if (meta) 
    { 
        // got Shoutcast metadata
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        const char *p = strstr(meta, "StreamTitle='"); // locate the title
        if (p) 
        {
            const char *p2 = strstr(p, "';"); // locate the end of it
            if (p2) 
            {
                char *t = _strdup(p + 13);
                t[p2 - (p + 13)] = 0;
                _callbacks->OnMeta(converter.from_bytes(t), empty);
                free(t);
            }
        }
    }
    else 
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        meta = BASS_ChannelGetTags(_stream, BASS_TAG_OGG);
        if (meta) 
        { 
            // got Icecast/OGG tags
            const char *p = meta;
            std::wstring title;
            std::wstring artist;
            for (; *p; p += strlen(p) + 1) 
            {
                if (!_strnicmp(p, "artist=", 7))
                { 
                    // found the artist
                    artist = converter.from_bytes(p + 7);
                }
                if (!_strnicmp(p, "title=", 6))
                { 
                    // found the title
                    title = converter.from_bytes(p + 6);
                }
            }
            if (!title.empty())
            {
                _callbacks->OnMeta(title, artist);
            }
        }
        else 
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            meta = BASS_ChannelGetTags(_stream, BASS_TAG_HLS_EXTINF);
            if (meta) 
            { 
                // got HLS segment info
                const char *p = strchr(meta, ',');
                if (p)
                {
                    _callbacks->OnMeta(converter.from_bytes(p), empty);
                }
            }
        }
    }
}

bool Player::Update()
{
    if (_stream == 0)
        return false;

    if (BASS_ChannelIsActive(_stream) == BASS_ACTIVE_PLAYING) 
    {
        const char *icy = BASS_ChannelGetTags(_stream, BASS_TAG_ICY);
        if (!icy) 
            icy = BASS_ChannelGetTags(_stream, BASS_TAG_HTTP);

        std::wstring name;
        std::wstring url;

        if (icy) {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

            for (; *icy; icy += strlen(icy) + 1) {
                if (!_strnicmp(icy, "icy-name:", 9))
                    name = converter.from_bytes(icy + 9);
                if (!_strnicmp(icy, "icy-url:", 8))
                    url = converter.from_bytes(icy + 8);
            }
        }
        _callbacks->OnPlay(name, url);

        ProcessMeta();
        return true;
    }
    else 
    {
        QWORD percent = 100 - BASS_StreamGetFilePosition(_stream, BASS_FILEPOS_BUFFERING);
        if (percent > 0)
        {
            _callbacks->OnBuffer((int)percent);
            return true;
        }

        return false;
    }
}