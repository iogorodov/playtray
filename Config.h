#pragma once

class Config
{
    std::vector<std::wstring> _names;
    std::vector<std::wstring> _urls;
    size_t _currentIndex;
    bool _autoPlay;

public:
    Config();

    void Read();

    void SetCurrentIndex(int index) { _currentIndex = index; }

    const std::vector<std::wstring>& GetItems() const { return _names; }
    size_t GetCurrentIndex() const { return _currentIndex; }

    const std::wstring& GetUrl(size_t index) const;

    bool IsAutoPlay() const { return _autoPlay; }
};

