/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Configuration Management
 */

#pragma once

#include <string>
#include <Windows.h>

struct PluginConfig
{
    // Connection
    int wsPort = 25885;
    int reconnectInterval = 5000;

    // Display
    int displayWidth = 300;
    int fontSize = 11;
    bool fontWeightBold = false;
    std::wstring fontName = L"Microsoft YaHei UI";

    bool enableScrolling = true;
    bool enableYrc = false;  // Default OFF due to TrafficMonitor's low refresh rate
    COLORREF highlightColor = RGB(0, 120, 215);
    COLORREF normalColor = RGB(180, 180, 180);
    int lyricOffset = 0; // in milliseconds
    
    // Dual line display
    bool dualLineDisplay = false;
    int secondLineType = 0; // 0 = next line, 1 = translation
};

class Config
{
public:
    static Config& Instance();

    void Load(const std::wstring& configDir);
    void Save();

    PluginConfig& Data() { return m_config; }
    const PluginConfig& Data() const { return m_config; }

    const wchar_t* StringRes(UINT id);

private:
    Config() = default;

    std::wstring m_configPath;
    PluginConfig m_config;
    CString m_strBuffer;
};

#define g_config Config::Instance()
