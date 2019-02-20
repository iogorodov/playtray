#include "stdafx.h"
#include "Config.h"

#include "nlohmann/json.hpp"

Config::Config() :
    _currentIndex(0),
    _autoPlay(false)
{
}

void Config::Read()
{
    std::ifstream file(L"playtray.json");
    if (!file.is_open())
        return;

    nlohmann::json json;
    file >> json;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (auto& el : json.items()) 
    {
        _names.push_back(converter.from_bytes(el.key()));
        _urls.push_back(converter.from_bytes(el.value()));
    }
}

bool Config::SetCurrentIndex(size_t index)
{
    if (index < 0 || index >= _names.size())
        return false;

    _currentIndex = index;
    return true;
}

const std::wstring& Config::GetUrl(size_t index) const
{
    static std::wstring empty;

    if (index < 0 || index >= _urls.size())
        return empty;

    return _urls[index];
}