/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Lyric Display Item Interface
 */

#pragma once

#include "PluginInterface.h"
#include <string>

class LyricDisplayItem : public IPluginItem
{
public:
    LyricDisplayItem();
    virtual ~LyricDisplayItem();

    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;

    virtual bool IsCustomDraw() const override { return true; }
    virtual int GetItemWidth() const override;
    virtual int GetItemWidthEx(void* hDC) const override;
    virtual void DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode) override;

    virtual int OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag) override;

private:
    std::wstring GetDisplayText() const;
    HFONT GetFont(HDC hDC) const;
    
    void DrawSimpleText(HDC dc, int x, int y, int w, int h, bool dark_mode);
    void DrawDualLine(HDC dc, int x, int y, int w, int h, bool dark_mode);
    void DrawWithYrcHighlight(HDC dc, int x, int y, int w, int h, bool dark_mode);
    void UpdateScrollAnimation(int textWidth, int areaWidth);

    mutable HFONT m_font = nullptr;
    mutable int m_lastFontSize = 0;
    mutable std::wstring m_lastFontName;
    mutable bool m_lastFontBold = false;

    // Scroll animation state (time-based)
    mutable float m_scrollOffset = 0.0f;
    mutable ULONGLONG m_scrollStartTime = 0;
    mutable ULONGLONG m_scrollCycleLength = 0;
};
