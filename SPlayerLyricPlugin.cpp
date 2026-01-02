/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Main Plugin Class Implementation
 */

#include "pch.h"
#include "SPlayerLyricPlugin.h"
#include "WebSocketClient.h"
#include "LyricManager.h"
#include "Config.h"
#include "JsonParser.h"

SPlayerLyricPlugin SPlayerLyricPlugin::m_instance;

SPlayerLyricPlugin::SPlayerLyricPlugin()
{
}

SPlayerLyricPlugin& SPlayerLyricPlugin::Instance()
{
    return m_instance;
}

IPluginItem* SPlayerLyricPlugin::GetItem(int index)
{
    if (index == 0)
        return &m_lyricItem;
    return nullptr;
}

void SPlayerLyricPlugin::DataRequired()
{
    // WebSocket is async, data already updated by callbacks
}

const wchar_t* SPlayerLyricPlugin::GetInfo(PluginInfoIndex index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    static CString str;

    switch (index)
    {
    case TMI_NAME:
        str.LoadString(IDS_PLUGIN_NAME);
        return str.GetString();

    case TMI_DESCRIPTION:
        str.LoadString(IDS_PLUGIN_DESCRIPTION);
        return str.GetString();

    case TMI_AUTHOR:
        return L"SPlayer Lyric Plugin";

    case TMI_COPYRIGHT:
        return L"Copyright (C) 2026";

    case TMI_VERSION:
        return L"1.0.0";

    case TMI_URL:
        return L"https://github.com/imsyy/SPlayer";

    default:
        break;
    }

    return L"";
}

#include "OptionsDialog.h"

ITMPlugin::OptionReturn SPlayerLyricPlugin::ShowOptionsDialog(void* hParent)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    COptionsDialog dlg(CWnd::FromHandle((HWND)hParent));
    if (dlg.DoModal() == IDOK)
    {
        return OR_OPTION_CHANGED;
    }

    return OR_OPTION_UNCHANGED;
}

void SPlayerLyricPlugin::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data)
{
    switch (index)
    {
    case EI_CONFIG_DIR:
        g_config.Load(data);

        if (!m_initialized)
        {
            InitWebSocketCallbacks();
            g_wsClient.Start(g_config.Data().wsPort);
            m_initialized = true;
        }
        break;

    default:
        break;
    }
}

const wchar_t* SPlayerLyricPlugin::GetTooltipInfo()
{
    std::wstring songInfo = g_lyricMgr.GetSongInfoText();

    if (!g_wsClient.IsConnected())
    {
        m_tooltipText = g_config.StringRes(IDS_NOT_CONNECTED);
    }
    else if (!songInfo.empty())
    {
        m_tooltipText = L"* " + songInfo;
        if (g_lyricMgr.IsPlaying())
            m_tooltipText += L" (Playing)";
        else
            m_tooltipText += L" (Paused)";
    }
    else
    {
        m_tooltipText.clear();
    }

    return m_tooltipText.c_str();
}

void SPlayerLyricPlugin::OnInitialize(ITrafficMonitor* pApp)
{
    m_pApp = pApp;
}

void SPlayerLyricPlugin::InitWebSocketCallbacks()
{
    WebSocketCallbacks callbacks;

    callbacks.onConnected = []() {
        OutputDebugStringW(L"[SPlayerLyric] Connected to SPlayer\n");
    };

    callbacks.onDisconnected = []() {
        g_lyricMgr.Clear();
        OutputDebugStringW(L"[SPlayerLyric] Disconnected from SPlayer\n");
    };

    callbacks.onStatusChange = [](bool isPlaying) {
        g_lyricMgr.UpdatePlayStatus(isPlaying);
    };

    callbacks.onSongChange = [](const SPlayerProtocol::SongInfo& info) {
        g_lyricMgr.UpdateSongInfo(info);
    };

    callbacks.onProgressChange = [](const SPlayerProtocol::ProgressInfo& info) {
        g_lyricMgr.UpdateProgress(info.currentTime);
    };

    callbacks.onLyricChange = [](const SPlayerProtocol::LyricData& data) {
        g_lyricMgr.UpdateLyrics(data);
    };

    callbacks.onError = [](const std::string& msg) {
        std::wstring wmsg = L"[SPlayerLyric] Error: ";
        wmsg += Utf8ToWide(msg);
        wmsg += L"\n";
        OutputDebugStringW(wmsg.c_str());
    };

    g_wsClient.SetCallbacks(callbacks);
}

// DLL Export
ITMPlugin* TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &SPlayerLyricPlugin::Instance();
}
