/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Main Plugin Class Interface
 */

#pragma once

#include "PluginInterface.h"
#include "LyricDisplayItem.h"

class SPlayerLyricPlugin : public ITMPlugin
{
private:
    SPlayerLyricPlugin();

public:
    static SPlayerLyricPlugin& Instance();

    virtual IPluginItem* GetItem(int index) override;
    virtual void DataRequired() override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;
    virtual const wchar_t* GetTooltipInfo() override;
    virtual void OnInitialize(ITrafficMonitor* pApp) override;

private:
    void InitWebSocketCallbacks();

    LyricDisplayItem m_lyricItem;
    ITrafficMonitor* m_pApp = nullptr;
    std::wstring m_tooltipText;
    bool m_initialized = false;

    static SPlayerLyricPlugin m_instance;
};

#ifdef __cplusplus
extern "C" {
#endif
    __declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif
