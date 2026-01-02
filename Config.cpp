/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Configuration Management Implementation
 */

#include "pch.h"
#include "Config.h"

Config& Config::Instance()
{
    static Config instance;
    return instance;
}

void Config::Load(const std::wstring& configDir)
{
    m_configPath = configDir + L"\\SPlayerLyric.ini";

    m_config.wsPort = GetPrivateProfileIntW(L"Connection", L"Port", 25885, m_configPath.c_str());
    m_config.reconnectInterval = GetPrivateProfileIntW(L"Connection", L"ReconnectInterval", 5000, m_configPath.c_str());

    m_config.displayWidth = GetPrivateProfileIntW(L"Display", L"Width", 300, m_configPath.c_str());
    m_config.fontSize = GetPrivateProfileIntW(L"Display", L"FontSize", 11, m_configPath.c_str());
    m_config.fontWeightBold = GetPrivateProfileIntW(L"Display", L"FontBold", 0, m_configPath.c_str()) != 0;

    wchar_t fontBuffer[64] = { 0 };
    GetPrivateProfileStringW(L"Display", L"FontName", L"Microsoft YaHei UI", fontBuffer, 64, m_configPath.c_str());
    m_config.fontName = fontBuffer;

    m_config.enableScrolling = GetPrivateProfileIntW(L"Lyric", L"EnableScrolling", 1, m_configPath.c_str()) != 0;
    m_config.enableYrc = GetPrivateProfileIntW(L"Lyric", L"EnableYrc", 0, m_configPath.c_str()) != 0;  // Default OFF
    m_config.highlightColor = GetPrivateProfileIntW(L"Lyric", L"HighlightColor", RGB(0, 120, 215), m_configPath.c_str());
    m_config.normalColor = GetPrivateProfileIntW(L"Lyric", L"NormalColor", RGB(180, 180, 180), m_configPath.c_str());
    m_config.lyricOffset = (int)GetPrivateProfileIntW(L"Lyric", L"LyricOffset", 0, m_configPath.c_str());
    m_config.dualLineDisplay = GetPrivateProfileIntW(L"Lyric", L"DualLine", 0, m_configPath.c_str()) != 0;
    m_config.secondLineType = GetPrivateProfileIntW(L"Lyric", L"SecondLineType", 0, m_configPath.c_str());
}

void Config::Save()
{
    if (m_configPath.empty())
        return;

    wchar_t buffer[64];

    swprintf_s(buffer, L"%d", m_config.wsPort);
    WritePrivateProfileStringW(L"Connection", L"Port", buffer, m_configPath.c_str());

    swprintf_s(buffer, L"%d", m_config.reconnectInterval);
    WritePrivateProfileStringW(L"Connection", L"ReconnectInterval", buffer, m_configPath.c_str());

    swprintf_s(buffer, L"%d", m_config.displayWidth);
    WritePrivateProfileStringW(L"Display", L"Width", buffer, m_configPath.c_str());

    swprintf_s(buffer, L"%d", m_config.fontSize);
    WritePrivateProfileStringW(L"Display", L"FontSize", buffer, m_configPath.c_str());

    WritePrivateProfileStringW(L"Display", L"FontBold", m_config.fontWeightBold ? L"1" : L"0", m_configPath.c_str());

    WritePrivateProfileStringW(L"Display", L"FontName", m_config.fontName.c_str(), m_configPath.c_str());

    WritePrivateProfileStringW(L"Lyric", L"EnableScrolling", m_config.enableScrolling ? L"1" : L"0", m_configPath.c_str());
    WritePrivateProfileStringW(L"Lyric", L"EnableYrc", m_config.enableYrc ? L"1" : L"0", m_configPath.c_str());

    swprintf_s(buffer, L"%d", m_config.highlightColor);
    WritePrivateProfileStringW(L"Lyric", L"HighlightColor", buffer, m_configPath.c_str());

    swprintf_s(buffer, L"%d", m_config.normalColor);
    WritePrivateProfileStringW(L"Lyric", L"NormalColor", buffer, m_configPath.c_str());

    swprintf_s(buffer, L"%d", m_config.lyricOffset);
    WritePrivateProfileStringW(L"Lyric", L"LyricOffset", buffer, m_configPath.c_str());

    WritePrivateProfileStringW(L"Lyric", L"DualLine", m_config.dualLineDisplay ? L"1" : L"0", m_configPath.c_str());
    
    swprintf_s(buffer, L"%d", m_config.secondLineType);
    WritePrivateProfileStringW(L"Lyric", L"SecondLineType", buffer, m_configPath.c_str());
}

const wchar_t* Config::StringRes(UINT id)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    m_strBuffer.LoadString(id);
    return m_strBuffer.GetString();
}
