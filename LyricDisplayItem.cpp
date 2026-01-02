/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Lyric Display Item Implementation with Dual Line and Scroll Animation
 */

#include "pch.h"
#include "LyricDisplayItem.h"
#include "LyricManager.h"
#include "WebSocketClient.h"
#include "Config.h"

// Static instance pointer for timer callback
static LyricDisplayItem* g_pLyricItem = nullptr;

LyricDisplayItem::LyricDisplayItem()
{
    g_pLyricItem = this;
}

LyricDisplayItem::~LyricDisplayItem()
{
    StopHighFreqRefresh();
    g_pLyricItem = nullptr;
    
    if (m_font != nullptr)
    {
        DeleteObject(m_font);
        m_font = nullptr;
    }
}

const wchar_t* LyricDisplayItem::GetItemName() const
{
    if (m_itemName.empty())
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        CString str;
        str.LoadString(IDS_LYRIC_ITEM_NAME);
        m_itemName = str.GetString();
    }
    return m_itemName.c_str();
}

const wchar_t* LyricDisplayItem::GetItemId() const
{
    return L"SPlayerLyric";
}

const wchar_t* LyricDisplayItem::GetItemLableText() const
{
    return L"";
}

const wchar_t* LyricDisplayItem::GetItemValueText() const
{
    return L"";
}

const wchar_t* LyricDisplayItem::GetItemValueSampleText() const
{
    return L"Sample Lyric Text For Width Calculation";
}

int LyricDisplayItem::GetItemWidth() const
{
    return g_config.Data().displayWidth;
}

int LyricDisplayItem::GetItemWidthEx(void* hDC) const
{
    return 0;
}

HFONT LyricDisplayItem::GetFont(HDC hDC) const
{
    // Try to capture taskbar window handle if not already captured
    if (g_pLyricItem && g_pLyricItem->m_taskbarWnd == NULL)
    {
        HWND hWnd = WindowFromDC(hDC);
        if (hWnd)
        {
            g_pLyricItem->m_taskbarWnd = hWnd;
             wchar_t buf[64];
             swprintf_s(buf, L"[SPlayerLyric] Captured hWnd from DC: %p\n", hWnd);
             OutputDebugStringW(buf);
        }
    }

    const auto& config = g_config.Data();

    if (m_font == nullptr ||
        m_lastFontSize != config.fontSize ||
        m_lastFontName != config.fontName ||
        m_lastFontBold != config.fontWeightBold)
    {
        if (m_font != nullptr)
        {
            DeleteObject(m_font);
        }

        int dpi = GetDeviceCaps(hDC, LOGPIXELSY);
        int fontHeight = -MulDiv(config.fontSize, dpi, 72);

        m_font = CreateFontW(
            fontHeight,
            0,
            0,
            0,
            config.fontWeightBold ? FW_BOLD : FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            config.fontName.c_str()
        );

        m_lastFontSize = config.fontSize;
        m_lastFontName = config.fontName;
        m_lastFontBold = config.fontWeightBold;
    }

    return m_font;
}

std::wstring LyricDisplayItem::GetDisplayText() const
{
    if (!g_wsClient.IsConnected())
    {
        return g_config.StringRes(IDS_NOT_CONNECTED);
    }

    std::wstring lyric = g_lyricMgr.GetCurrentLyricText();
    if (!lyric.empty())
    {
        return lyric;
    }

    std::wstring songInfo = g_lyricMgr.GetSongInfoText();
    if (!songInfo.empty())
    {
        return songInfo;
    }

    return g_config.StringRes(IDS_NO_LYRIC);
}

void LyricDisplayItem::UpdateScrollAnimation(int textWidth, int areaWidth)
{
    if (!g_config.Data().enableScrolling || textWidth <= areaWidth)
    {
        m_scrollOffset = 0;
        m_scrollStartTime = 0;
        m_scrollCycleLength = 0;
        return;
    }

    int maxOffset = textWidth - areaWidth + 20; // 20px padding
    
    // Scroll speed: 50 pixels per second (adjustable)
    float scrollSpeed = 50.0f;
    
    // Calculate time for one-way scroll
    float scrollDuration = (float)maxOffset / scrollSpeed * 1000.0f; // in ms
    float pauseDuration = 1500.0f; // 1.5 second pause at each end
    
    // Total cycle: scroll right -> pause -> scroll left -> pause
    float cycleDuration = scrollDuration * 2.0f + pauseDuration * 2.0f;
    
    ULONGLONG now = GetTickCount64();
    
    // Initialize start time if needed
    if (m_scrollStartTime == 0 || m_scrollCycleLength != (ULONGLONG)cycleDuration)
    {
        m_scrollStartTime = now;
        m_scrollCycleLength = (ULONGLONG)cycleDuration;
    }
    
    // Calculate position in cycle
    float cyclePos = (float)((now - m_scrollStartTime) % (ULONGLONG)cycleDuration);
    
    if (cyclePos < pauseDuration)
    {
        // Pause at start
        m_scrollOffset = 0;
    }
    else if (cyclePos < pauseDuration + scrollDuration)
    {
        // Scrolling right
        float t = (cyclePos - pauseDuration) / scrollDuration;
        // Use ease-in-out for smoother animation
        float eased = t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        m_scrollOffset = eased * maxOffset;
    }
    else if (cyclePos < pauseDuration * 2.0f + scrollDuration)
    {
        // Pause at end
        m_scrollOffset = (float)maxOffset;
    }
    else
    {
        // Scrolling left
        float t = (cyclePos - pauseDuration * 2.0f - scrollDuration) / scrollDuration;
        float eased = t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        m_scrollOffset = (1.0f - eased) * maxOffset;
    }
}

void LyricDisplayItem::DrawItem(void* hDC, int x, int y, int w, int h, bool dark_mode)
{
    HDC dc = static_cast<HDC>(hDC);
    const auto& config = g_config.Data();

    // Hide when not playing if enabled
    if (config.hideWhenNotPlaying && (!g_wsClient.IsConnected() || !g_lyricMgr.IsPlaying()))
    {
        return;
    }

    // Set font
    HFONT font = GetFont(dc);
    HFONT oldFont = nullptr;
    if (font != nullptr)
    {
        oldFont = (HFONT)SelectObject(dc, font);
    }

    SetBkMode(dc, TRANSPARENT);

    // Check if dual line display is enabled
    if (config.desktopDualLine)
    {
        DrawDualLine(dc, x, y, w, h, dark_mode);
    }
    // Check if we have YRC data for highlight rendering
    else if (config.enableYrc && g_lyricMgr.HasYrcData() && g_wsClient.IsConnected())
    {
        DrawWithYrcHighlight(dc, x, y, w, h, dark_mode);
    }
    else
    {
        DrawSimpleText(dc, x, y, w, h, dark_mode);
    }

    // Restore font
    if (oldFont != nullptr)
    {
        SelectObject(dc, oldFont);
    }
}

void LyricDisplayItem::DrawDualLine(HDC dc, int x, int y, int w, int h, bool dark_mode)
{
    const auto& config = g_config.Data();
    
    // Create font for dual line mode (uses dualLineFontSize instead of fontSize)
    int dpi = GetDeviceCaps(dc, LOGPIXELSY);
    int fontHeight = -MulDiv(config.dualLineFontSize, dpi, 72);
    HFONT dualFont = CreateFontW(
        fontHeight, 0, 0, 0,
        config.fontWeightBold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        config.fontName.c_str()
    );
    HFONT oldFont = (HFONT)SelectObject(dc, dualFont);
    
    // Get current and second line text
    std::wstring line1 = g_lyricMgr.GetCurrentLyricText();
    std::wstring line2;
    
    if (config.secondLineType == 0)
    {
        // Next line
        line2 = g_lyricMgr.GetNextLyricText();
    }
    else if (config.secondLineType == 1)
    {
        // Translation
        line2 = g_lyricMgr.GetCurrentTranslation();
    }
    else
    {
        // Artist/Song info
        line2 = g_lyricMgr.GetSongInfoText();
    }
    
    // If no current lyric, show song info or default
    if (line1.empty())
    {
        if (!g_wsClient.IsConnected())
        {
            line1 = g_config.StringRes(IDS_NOT_CONNECTED);
        }
        else
        {
            line1 = g_lyricMgr.GetSongInfoText();
            if (line1.empty())
                line1 = g_config.StringRes(IDS_NO_LYRIC);
        }
        line2.clear();
    }
    
    // If second line is empty, and we are not in "Artist" mode, check if we should show single line
    if (line2.empty() && config.secondLineType != 2)
    {
        // Restore old font and use DrawSimpleText instead to use the larger font size if appropriate,
        // OR just draw it centered here with the current dual line font.
        // The user said "single line display", usually implying the standard single line look.
        
        SelectObject(dc, oldFont);
        DeleteObject(dualFont);
        
        DrawSimpleText(dc, x, y, w, h, dark_mode);
        return;
    }
    
    // Fallback to song info if second line is still empty (e.g., in Artist mode but info empty)
    if (line2.empty())
    {
        line2 = g_lyricMgr.GetSongInfoText();
    }

    // Double check if we still have only one line after fallback
    if (line2.empty())
    {
        SelectObject(dc, oldFont);
        DeleteObject(dualFont);
        DrawSimpleText(dc, x, y, w, h, dark_mode);
        return;
    }
    
    // Calculate layout
    // Instead of h/2, we'll give each line a bit more breathing room by not clipping too strictly
    int lineHeight = h / 2;
    
    // Set colors based on dark mode and adaptive setting
    COLORREF primaryColor, secondaryColor, highlightColor;
    
    // Choose color set based on mode
    bool useDarkModeColors = dark_mode;
    if (!config.adaptiveColor)
    {
        // If adaptive disabled, maybe force dark mode colors (legacy behavior) or stick to user preference?
        // Let's assume non-adaptive means using the "Dark" set (or the legacy set which mapped to Dark)
        useDarkModeColors = true; 
    }

    if (useDarkModeColors)
    {
        primaryColor = config.darkNormalColor;
        // For secondary, we can make it slightly dimmer or same as primary
        // Let's make it 80% brightness of primary or just hardcoded dim if user doesn't specify secondary
        // Since we only have "Normal" and "Highlight" in config, we derive secondary line color.
        // Simple approach: Secondary is same as Primary for now, or slight transparency if we could? 
        // GDI doesn't support alpha easily. Let's stick to Primary.
        // Actually, user requested "Deep/Light mode color customization". 
        // We have lightNormalColor and darkNormalColor.
        
        secondaryColor = primaryColor; 
        
        // To distinguish secondary line, maybe hardcode a dimmer simple calculation or just use same color.
        // Previous hardcoded logic: RGB(200, 200, 200) vs White.
        // Let's try to slightly dim it if it's near white.
        if (GetRValue(primaryColor) > 200 && GetGValue(primaryColor) > 200 && GetBValue(primaryColor) > 200)
             secondaryColor = RGB(200, 200, 200);
             
        highlightColor = config.darkHighlightColor;
    }
    else
    {
        primaryColor = config.lightNormalColor;
        secondaryColor = primaryColor;
        // Dim slightly if near black?
        if (GetRValue(primaryColor) < 50 && GetGValue(primaryColor) < 50 && GetBValue(primaryColor) < 50)
             secondaryColor = RGB(80, 80, 80);
             
        highlightColor = config.lightHighlightColor;
    }
    
    // Dim if not playing
    if (!g_wsClient.IsConnected() || !g_lyricMgr.IsPlaying())
    {
        if (dark_mode)
        {
            primaryColor = RGB(150, 150, 150);
            secondaryColor = RGB(120, 120, 120);
        }
        else
        {
            primaryColor = RGB(100, 100, 100);
            secondaryColor = RGB(130, 130, 130);
        }
    }
    
    // Draw first line (current lyric) - use top half
    // If YRC is enabled, use word-by-word discrete highlight for the first line
    auto words = g_lyricMgr.GetCurrentYrcWords();
    if (config.enableYrc && !words.empty() && g_wsClient.IsConnected() && g_lyricMgr.IsPlaying())
    {
        int64_t currentTime = g_lyricMgr.GetCurrentTime();
        // Use the highlightColor determined by adaptive logic above, do NOT re-read from config
        
        // Calculate total width of YRC words
        int totalWidth = 0;
        std::vector<SIZE> wordSizes;
        for (const auto& word : words)
        {
            SIZE sz;
            GetTextExtentPoint32W(dc, word.text.c_str(), (int)word.text.length(), &sz);
            wordSizes.push_back(sz);
            totalWidth += sz.cx;
        }

        // Calculate alignment X
        int textX1 = x + 5; // Default Left
        if (totalWidth < w) // Only apply alignment if text fits (otherwise we scroll)
        {
            if (config.dualLineAlignment == 1) // Center
                textX1 = x + (w - totalWidth) / 2;
            else if (config.dualLineAlignment == 2) // Right
                textX1 = x + w - totalWidth - 5;
            else if (config.dualLineAlignment == 3) // Split (Line 1 Left)
                textX1 = x + 5;
        }

        // Apply scroll if needed (override alignment if scrolling)
        if (config.enableScrolling && totalWidth > w - 10)
        {
            UpdateScrollAnimation(totalWidth, w - 10);
            textX1 = x + 5 - (int)m_scrollOffset;
        }

        int textY1 = y + (lineHeight - (wordSizes.empty() ? 0 : wordSizes[0].cy)) / 2;

        // Clip region for first line
        HRGN clipRgn1 = CreateRectRgn(x, y, x + w, y + lineHeight + 2);
        SelectClipRgn(dc, clipRgn1);

        int curX = textX1;
        for (size_t i = 0; i < words.size(); ++i)
        {
            const auto& word = words[i];
            int width = wordSizes[i].cx;

            // 1. Draw Normal Text (Background)
            SetTextColor(dc, primaryColor);
            TextOutW(dc, curX, textY1, word.text.c_str(), (int)word.text.length());

            // 2. Draw Highlight Text (Foreground with clip)
            long long endTime = word.startTime + word.duration;
            double progress = 0.0;
            
            if (currentTime >= endTime) 
                progress = 1.0;
            else if (currentTime >= word.startTime && word.duration > 0)
                progress = (double)(currentTime - word.startTime) / word.duration;

            if (progress > 0.001)
            {
                int effectiveWidth = (int)(width * progress);
                if (effectiveWidth > 0)
                {
                    int saveId = SaveDC(dc);
                    HRGN wordClip = CreateRectRgn(curX, y, curX + effectiveWidth, y + lineHeight + 2);
                    ExtSelectClipRgn(dc, wordClip, RGN_AND);
                    
                    SetTextColor(dc, highlightColor);
                    TextOutW(dc, curX, textY1, word.text.c_str(), (int)word.text.length());
                    
                    RestoreDC(dc, saveId);
                    DeleteObject(wordClip);
                }
            }

            curX += width;
        }
        
        SelectClipRgn(dc, NULL);
        DeleteObject(clipRgn1);
    }
    else
    {
        // Simple text for first line
        SetTextColor(dc, primaryColor);
        SIZE size1;
        GetTextExtentPoint32W(dc, line1.c_str(), (int)line1.length(), &size1);
        
        int textY1 = y + (lineHeight - size1.cy) / 2;
        int textX1 = x + 5; // Default Left
        
        if (size1.cx < w)
        {
            if (config.dualLineAlignment == 1) // Center
                textX1 = x + (w - size1.cx) / 2;
            else if (config.dualLineAlignment == 2) // Right
                textX1 = x + w - size1.cx - 5;
            else if (config.dualLineAlignment == 3) // Split (Line 1 Left)
                textX1 = x + 5;
        }
        
        if (config.enableScrolling && size1.cx > w - 10)
        {
            UpdateScrollAnimation(size1.cx, w - 10);
            textX1 = x + 5 - (int)m_scrollOffset;
        }
        
        HRGN clipRgn1 = CreateRectRgn(x, y, x + w, y + lineHeight + 2);
        SelectClipRgn(dc, clipRgn1);
        TextOutW(dc, textX1, textY1, line1.c_str(), (int)line1.length());
        SelectClipRgn(dc, NULL);
        DeleteObject(clipRgn1);
    }
    
    // Draw second line
    SetTextColor(dc, secondaryColor);
    SIZE size2;
    GetTextExtentPoint32W(dc, line2.c_str(), (int)line2.length(), &size2);
    
    int textY2 = y + lineHeight + (lineHeight - size2.cy) / 2;
    int textX2 = x + 5;
    
    // Apply alignment for second line
    if (size2.cx < w) // Only if fits
    {
        if (config.dualLineAlignment == 1) // Center
            textX2 = x + (w - size2.cx) / 2;
        else if (config.dualLineAlignment == 2) // Right
            textX2 = x + w - size2.cx - 5;
        else if (config.dualLineAlignment == 3) // Split (Line 2 Right)
            textX2 = x + w - size2.cx - 5;
    }
    
    // Scrolling for second line (independent or just static?) - keep it static for now as per design
    
    // Clip for second line - allow a bit room at top for ascenders
    HRGN clipRgn2 = CreateRectRgn(x, y + lineHeight - 1, x + w, y + h);
    SelectClipRgn(dc, clipRgn2);
    TextOutW(dc, textX2, textY2, line2.c_str(), (int)line2.length());
    SelectClipRgn(dc, NULL);
    DeleteObject(clipRgn2);
    
    // Restore original font and cleanup
    SelectObject(dc, oldFont);
    DeleteObject(dualFont);
}

void LyricDisplayItem::DrawSimpleText(HDC dc, int x, int y, int w, int h, bool dark_mode)
{
    std::wstring text = GetDisplayText();
    if (text.empty())
        return;

    // Set text color based on dark mode
    COLORREF textColor;
    
    bool useDarkModeColors = dark_mode;
    if (!g_config.Data().adaptiveColor) useDarkModeColors = true; // Force dark set if not adaptive

    if (useDarkModeColors)
        textColor = g_config.Data().darkNormalColor;
    else
        textColor = g_config.Data().lightNormalColor;

    // Dim if not playing
    if (!g_wsClient.IsConnected() || !g_lyricMgr.IsPlaying())
    {
        if (dark_mode)
            textColor = RGB(150, 150, 150);
        else
            textColor = RGB(100, 100, 100);
    }

    SetTextColor(dc, textColor);

    // Calculate text size
    SIZE textSize;
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.length(), &textSize);

    // Update scroll animation
    UpdateScrollAnimation(textSize.cx, w);

    int textY = y + (h - textSize.cy) / 2;

    if (textSize.cx > w && g_config.Data().enableScrolling)
    {
        // Create clipping region
        HRGN clipRgn = CreateRectRgn(x, y, x + w, y + h);
        SelectClipRgn(dc, clipRgn);

        int textX = x - (int)m_scrollOffset + g_config.Data().desktopXOffset; // Apply offset
        TextOutW(dc, textX, textY, text.c_str(), (int)text.length());

        SelectClipRgn(dc, NULL);
        DeleteObject(clipRgn);
    }
    else if (textSize.cx > w)
    {
        // Ellipsis mode
        RECT drawRect = { x, textY, x + w, textY + textSize.cy };
        DrawTextW(dc, text.c_str(), (int)text.length(), &drawRect,
            DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }
    else
    {
        // Center
        int textX = x + (w - textSize.cx) / 2;
        textX += g_config.Data().desktopXOffset; // Apply offset
        TextOutW(dc, textX, textY, text.c_str(), (int)text.length());
    }
}

void LyricDisplayItem::DrawWithYrcHighlight(HDC dc, int x, int y, int w, int h, bool dark_mode)
{
    auto words = g_lyricMgr.GetCurrentYrcWords();
    if (words.empty())
    {
        DrawSimpleText(dc, x, y, w, h, dark_mode);
        return;
    }

    int64_t currentTime = g_lyricMgr.GetCurrentTime();
    const auto& config = g_config.Data();

    // Set colors based on dark mode
    COLORREF normalColor, highlightColor;
    
    bool useDarkModeColors = dark_mode;
    if (!config.adaptiveColor) useDarkModeColors = true;

    if (useDarkModeColors)
    {
        normalColor = config.darkNormalColor;
        highlightColor = config.darkHighlightColor;
    }
    else
    {
        normalColor = config.lightNormalColor;
        highlightColor = config.lightHighlightColor;
    }

    // Dim if not playing
    if (!g_wsClient.IsConnected() || !g_lyricMgr.IsPlaying())
    {
        if (dark_mode)
        {
            normalColor = RGB(120, 120, 120);
            highlightColor = RGB(150, 150, 150);
        }
        else
        {
            normalColor = RGB(150, 150, 150);
            highlightColor = RGB(100, 100, 100);
        }
    }

    // First pass: calculate total width
    int totalWidth = 0;
    std::vector<SIZE> wordSizes;
    for (const auto& word : words)
    {
        SIZE size;
        GetTextExtentPoint32W(dc, word.text.c_str(), (int)word.text.length(), &size);
        wordSizes.push_back(size);
        totalWidth += size.cx;
    }

    // Update scroll
    UpdateScrollAnimation(totalWidth, w);

    int textY = y + (h - (wordSizes.empty() ? 0 : wordSizes[0].cy)) / 2;
    
    // Calculate starting X position
    // Calculate starting X position based on alignment
    int startX = x + 5; // Default Left
    
    if (totalWidth < w)
    {
        if (config.dualLineAlignment == 1) // Center
        {
            startX = x + (w - totalWidth) / 2;
        }
        else if (config.dualLineAlignment == 2) // Right
        {
            startX = x + w - totalWidth - 5;
        }
        else if (config.dualLineAlignment == 3) // Split -> Line 1 Left
        {
            startX = x + 5;
        }
        // else 0 (Left) -> x + 5
    }
    else if (config.enableScrolling)
    {
        startX = x + 5 - (int)m_scrollOffset;
    }
    
    // Apply global offset
    startX += config.desktopXOffset;

    // Create clipping region if needed
    HRGN clipRgn = nullptr;
    if (totalWidth > w)
    {
        clipRgn = CreateRectRgn(x, y, x + w, y + h);
        SelectClipRgn(dc, clipRgn);
    }

    // Draw each word
    // Draw each word with smooth highlight
    int currentX = startX;
    for (size_t i = 0; i < words.size(); i++)
    {
        const auto& word = words[i];
        const auto& size = wordSizes[i];

        // 1. Draw Background (Normal Color)
        SetTextColor(dc, normalColor);
        TextOutW(dc, currentX, textY, word.text.c_str(), (int)word.text.length());

        // 2. Calculate Progress
        double progress = 0.0;
        long long endTime = word.startTime + word.duration;
        
        if (currentTime >= endTime)
            progress = 1.0;
        else if (currentTime >= word.startTime && word.duration > 0)
            progress = (double)(currentTime - word.startTime) / word.duration;

        // 3. Draw Foreground (Highlight Color) with Clip
        if (progress > 0.001)
        {
            int fillWidth = (int)(size.cx * progress);
            if (fillWidth > 0)
            {
                int saveId = SaveDC(dc);
                
                // Clip rect: currentX to currentX + fillWidth
                // Must intersect with existing clip if scrolling
                HRGN wordClip = CreateRectRgn(currentX, y, currentX + fillWidth, y + h);
                ExtSelectClipRgn(dc, wordClip, RGN_AND);
                
                SetTextColor(dc, highlightColor);
                TextOutW(dc, currentX, textY, word.text.c_str(), (int)word.text.length());
                
                RestoreDC(dc, saveId);
                DeleteObject(wordClip);
            }
        }

        currentX += size.cx;
    }

    if (clipRgn)
    {
        SelectClipRgn(dc, NULL);
        DeleteObject(clipRgn);
    }
}

int LyricDisplayItem::OnMouseEvent(MouseEventType type, int x, int y, void* hWnd, int flag)
{
    // Save taskbar window handle for high-frequency refresh
    if (flag & MF_TASKBAR_WND)
    {
        m_taskbarWnd = (HWND)hWnd;
    }
    
    switch (type)
    {
    case MT_LCLICKED:
        g_wsClient.SendControl(SPlayerProtocol::ControlCommand::Toggle);
        return 1;

    case MT_DBCLICKED:
        return 0;

    case MT_RCLICKED:
        return 0;

    case MT_WHEEL_UP:
        g_wsClient.SendControl(SPlayerProtocol::ControlCommand::Prev);
        return 1;

    case MT_WHEEL_DOWN:
        g_wsClient.SendControl(SPlayerProtocol::ControlCommand::Next);
        return 1;

    default:
        return 0;
    }
}

// High-frequency refresh timer callback
void CALLBACK LyricDisplayItem::HighFreqTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    if (g_pLyricItem && g_pLyricItem->m_highFreqEnabled && g_pLyricItem->m_taskbarWnd)
    {
        // Only refresh when YRC is enabled and playing
        if (g_config.Data().enableYrc && g_lyricMgr.IsPlaying() && g_lyricMgr.HasYrcData())
        {
            // Invalidate the entire taskbar window to trigger redraw
            // TrafficMonitor will call DrawItem when processing WM_PAINT
            InvalidateRect(g_pLyricItem->m_taskbarWnd, NULL, FALSE);
        }
    }
}

void LyricDisplayItem::StartHighFreqRefresh()
{
    if (m_highFreqTimerId == 0)
    {
        // Create timer with ~100ms interval (10 FPS boost)
        // Using NULL for hwnd makes it a thread timer
        m_highFreqTimerId = SetTimer(NULL, 0, 30, HighFreqTimerProc);
        m_highFreqEnabled = true;
        OutputDebugStringW(L"[SPlayerLyric] High-frequency refresh started\n");
    }
}

void LyricDisplayItem::StopHighFreqRefresh()
{
    if (m_highFreqTimerId != 0)
    {
        KillTimer(NULL, m_highFreqTimerId);
        m_highFreqTimerId = 0;
        m_highFreqEnabled = false;
        OutputDebugStringW(L"[SPlayerLyric] High-frequency refresh stopped\n");
    }
}
