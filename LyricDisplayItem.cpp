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

// Note: Timer-based high-FPS refresh doesn't work with TrafficMonitor's architecture
// TrafficMonitor only calls DrawItem in its own timer callback (~1 second interval)

LyricDisplayItem::LyricDisplayItem()
{
}

LyricDisplayItem::~LyricDisplayItem()
{
    if (m_font != nullptr)
    {
        DeleteObject(m_font);
        m_font = nullptr;
    }
}

const wchar_t* LyricDisplayItem::GetItemName() const
{
    return g_config.StringRes(IDS_LYRIC_ITEM_NAME);
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

    // Set font
    HFONT font = GetFont(dc);
    HFONT oldFont = nullptr;
    if (font != nullptr)
    {
        oldFont = (HFONT)SelectObject(dc, font);
    }

    SetBkMode(dc, TRANSPARENT);

    // Check if dual line display is enabled
    if (config.dualLineDisplay)
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
    
    // Get current and second line text
    std::wstring line1 = g_lyricMgr.GetCurrentLyricText();
    std::wstring line2;
    
    if (config.secondLineType == 0)
    {
        // Next line
        line2 = g_lyricMgr.GetNextLyricText();
    }
    else
    {
        // Translation - TODO: implement translation support
        line2 = L"";
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
    
    // Calculate line heights
    int lineHeight = h / 2;
    
    // Set colors
    COLORREF primaryColor = dark_mode ? RGB(255, 255, 255) : RGB(0, 0, 0);
    COLORREF secondaryColor = config.normalColor;
    
    if (!g_wsClient.IsConnected() || !g_lyricMgr.IsPlaying())
    {
        primaryColor = dark_mode ? RGB(150, 150, 150) : RGB(100, 100, 100);
    }
    
    // Draw first line (current lyric)
    SetTextColor(dc, primaryColor);
    SIZE size1;
    GetTextExtentPoint32W(dc, line1.c_str(), (int)line1.length(), &size1);
    
    int textY1 = y + (lineHeight - size1.cy) / 2;
    int textX1 = x + 5;
    
    // Apply scroll if needed
    if (config.enableScrolling && size1.cx > w - 10)
    {
        UpdateScrollAnimation(size1.cx, w - 10);
        textX1 -= (int)m_scrollOffset;
    }
    
    // Create clip region for first line
    HRGN clipRgn1 = CreateRectRgn(x, y, x + w, y + lineHeight);
    SelectClipRgn(dc, clipRgn1);
    TextOutW(dc, textX1, textY1, line1.c_str(), (int)line1.length());
    SelectClipRgn(dc, NULL);
    DeleteObject(clipRgn1);
    
    // Draw second line if available
    if (!line2.empty())
    {
        SetTextColor(dc, secondaryColor);
        SIZE size2;
        GetTextExtentPoint32W(dc, line2.c_str(), (int)line2.length(), &size2);
        
        int textY2 = y + lineHeight + (lineHeight - size2.cy) / 2;
        int textX2 = x + 5;
        
        // Create clip region for second line
        HRGN clipRgn2 = CreateRectRgn(x, y + lineHeight, x + w, y + h);
        SelectClipRgn(dc, clipRgn2);
        TextOutW(dc, textX2, textY2, line2.c_str(), (int)line2.length());
        SelectClipRgn(dc, NULL);
        DeleteObject(clipRgn2);
    }
}

void LyricDisplayItem::DrawSimpleText(HDC dc, int x, int y, int w, int h, bool dark_mode)
{
    std::wstring text = GetDisplayText();
    if (text.empty())
        return;

    // Set text color
    COLORREF textColor;
    if (dark_mode)
        textColor = RGB(255, 255, 255);
    else
        textColor = RGB(0, 0, 0);

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

        int textX = x - (int)m_scrollOffset;
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

    // Define colors - use configured colors
    COLORREF normalColor = config.normalColor;
    COLORREF highlightColor = config.highlightColor;

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
    int startX;
    if (totalWidth <= w)
    {
        startX = x + (w - totalWidth) / 2;
    }
    else if (config.enableScrolling)
    {
        startX = x - (int)m_scrollOffset;
    }
    else
    {
        startX = x;
    }

    // Create clipping region if needed
    HRGN clipRgn = nullptr;
    if (totalWidth > w)
    {
        clipRgn = CreateRectRgn(x, y, x + w, y + h);
        SelectClipRgn(dc, clipRgn);
    }

    // Draw each word
    int currentX = startX;
    for (size_t i = 0; i < words.size(); i++)
    {
        const auto& word = words[i];
        const auto& size = wordSizes[i];

        // Calculate highlight progress for this word
        float progress = 0.0f;
        if (currentTime >= word.startTime + word.duration)
        {
            progress = 1.0f; // Fully highlighted
        }
        else if (currentTime >= word.startTime)
        {
            progress = (float)(currentTime - word.startTime) / (float)word.duration;
        }

        if (progress >= 1.0f)
        {
            // Fully highlighted
            SetTextColor(dc, highlightColor);
            TextOutW(dc, currentX, textY, word.text.c_str(), (int)word.text.length());
        }
        else if (progress <= 0.0f)
        {
            // Not highlighted
            SetTextColor(dc, normalColor);
            TextOutW(dc, currentX, textY, word.text.c_str(), (int)word.text.length());
        }
        else
        {
            // Partial highlight - draw in two parts
            int highlightWidth = (int)(size.cx * progress);

            // Create clip for highlighted part
            HRGN hlRgn = CreateRectRgn(currentX, y, currentX + highlightWidth, y + h);
            SelectClipRgn(dc, hlRgn);
            SetTextColor(dc, highlightColor);
            TextOutW(dc, currentX, textY, word.text.c_str(), (int)word.text.length());
            DeleteObject(hlRgn);

            // Create clip for normal part
            HRGN nmRgn = CreateRectRgn(currentX + highlightWidth, y, currentX + size.cx, y + h);
            SelectClipRgn(dc, nmRgn);
            SetTextColor(dc, normalColor);
            TextOutW(dc, currentX, textY, word.text.c_str(), (int)word.text.length());
            DeleteObject(nmRgn);

            // Restore main clip
            if (clipRgn)
                SelectClipRgn(dc, clipRgn);
            else
                SelectClipRgn(dc, NULL);
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
