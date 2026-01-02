/*
 * SPlayer Desktop Lyric - Critical Visibility Fix
 */

#include <afxwin.h>
#include <afxdialogex.h>
#include <afxdlgs.h>
#include <windows.h>
#include <windowsx.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <shlwapi.h>
#include <string>
#include <vector>

#include "Config.h"
#include "LyricManager.h"
#include "WebSocketClient.h"
#include "OptionsDialog.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")

ID2D1Factory* g_pD2DFactory = nullptr;
ID2D1DCRenderTarget* g_pDCRenderTarget = nullptr;
IDWriteFactory* g_pDWriteFactory = nullptr;
IDWriteTextFormat* g_pTextFormat = nullptr;

std::wstring g_lastLine;
std::wstring g_currentLine;
std::wstring g_secondLine;
float g_scrollPos = 0.0f; 
int64_t g_lastLyricIndex = -1;
const int REFRESH_INTERVAL = 16; 

std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size);
    return wstr;
}

class CDesktopLyricApp : public CWinApp {
public:
    virtual BOOL InitInstance() override { return TRUE; }
} theApp;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitD2D(HWND hWnd);
void CleanupD2D();
void Render(HWND hWnd);
void UpdatePosition(HWND hWnd);
void SetAutoStart(bool enable);
void InitWebSocketCallbacks();

D2D1::ColorF ColorRefToD2D(COLORREF color, float alpha = 1.0f) {
    return D2D1::ColorF(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, alpha);
}

bool IsTaskbarDarkMode() {
    HKEY hKey;
    DWORD value = 1; 
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"SystemUsesLightTheme", NULL, NULL, (BYTE*)&value, &size) != ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (BYTE*)&value, &size);
        }
        RegCloseKey(hKey);
    }
    return value == 0;
}

void InitWebSocketCallbacks() {
    WebSocketCallbacks callbacks;
    callbacks.onConnected = []() { OutputDebugStringW(L"[DesktopLyric] Connected\n"); };
    callbacks.onDisconnected = []() { g_lyricMgr.Clear(); };
    callbacks.onStatusChange = [](bool isPlaying) { g_lyricMgr.UpdatePlayStatus(isPlaying); };
    callbacks.onSongChange = [](const SPlayerProtocol::SongInfo& info) { g_lyricMgr.UpdateSongInfo(info); };
    callbacks.onProgressChange = [](const SPlayerProtocol::ProgressInfo& info) { g_lyricMgr.UpdateProgress(info.currentTime); };
    callbacks.onLyricChange = [](const SPlayerProtocol::LyricData& data) { g_lyricMgr.UpdateLyrics(data); };
    callbacks.onError = [](const std::string& msg) { 
        std::wstring wmsg = L"[DesktopLyric] Error: " + Utf8ToWide(msg) + L"\n";
        OutputDebugStringW(wmsg.c_str()); 
    };
    g_wsClient.SetCallbacks(callbacks);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    if (!AfxWinInit(hInstance, hPrevInstance, lpCmdLine, nCmdShow)) return 1;

    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    g_config.Load(path);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, 
                       LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), 
                       nullptr, nullptr, L"DesktopLyricClass", nullptr };
    RegisterClassExW(&wc);

    HWND hTaskbar = FindWindowExW(nullptr, nullptr, L"Shell_TrayWnd", nullptr);
    HWND hWnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST, 
                                L"DesktopLyricClass", L"SPlayer Desktop Lyric", WS_POPUP, 
                                0, 0, 300, 48, hTaskbar, nullptr, hInstance, nullptr);

    if (!hWnd) return 0;

    InitD2D(hWnd);
    UpdatePosition(hWnd);
    InitWebSocketCallbacks();
    g_wsClient.Start(g_config.Data().wsPort);

    SetTimer(hWnd, 1, REFRESH_INTERVAL, nullptr);
    SetTimer(hWnd, 2, 2000, nullptr); 

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    g_wsClient.Stop();
    CleanupD2D();
    return (int)msg.wParam;
}

void SetAutoStart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(NULL, path, MAX_PATH);
            RegSetValueExW(hKey, L"SPlayerDesktopLyric", 0, REG_SZ, (BYTE*)path, (lstrlenW(path) + 1) * sizeof(wchar_t));
        } else {
            RegDeleteValueW(hKey, L"SPlayerDesktopLyric");
        }
        RegCloseKey(hKey);
    }
}

void UpdatePosition(HWND hWnd) {
    const auto& config = g_config.Data();
    HWND hTaskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!hTaskbar) return;

    RECT rcT; GetWindowRect(hTaskbar, &rcT);
    int tbH = rcT.bottom - rcT.top;
    int tbW = rcT.right - rcT.left;

    int width = config.displayWidth;
    int winHeight = config.desktopDualLine ? 48 : 32; 

    int winX = rcT.left + config.desktopXOffset;
    int winY = rcT.top + (tbH - winHeight) / 2;

    if (tbW < tbH) {
        winX = rcT.left + (tbW - width) / 2;
        winY = rcT.top + config.desktopXOffset;
    }

    SetWindowPos(hWnd, HWND_TOPMOST, winX, winY, width, winHeight,  SWP_SHOWWINDOW | SWP_NOACTIVATE);
    Render(hWnd);
    SetAutoStart(config.autoStart);
}

void InitD2D(HWND hWnd) {
    if (g_pD2DFactory) CleanupD2D();
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory);
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    g_pD2DFactory->CreateDCRenderTarget(&props, &g_pDCRenderTarget);
    g_pDCRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_pDWriteFactory);
    
    // Robust Font Fallback: Try User Font -> YaHei UI -> YaHei -> Segoe UI -> Arial
    const auto& config = g_config.Data();
    std::vector<std::wstring> fontList;
    if (!config.fontName.empty()) fontList.push_back(config.fontName);
    fontList.push_back(L"Microsoft YaHei UI");
    fontList.push_back(L"Microsoft YaHei");
    fontList.push_back(L"Segoe UI");
    fontList.push_back(L"Arial");

    for (const auto& name : fontList) {
        HRESULT hr = g_pDWriteFactory->CreateTextFormat(
            name.c_str(), nullptr,
            config.fontWeightBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            (float)config.fontSize * 1.33f, L"zh-CN", &g_pTextFormat
        );
        if (SUCCEEDED(hr) && g_pTextFormat) break;
    }
    
    if (g_pTextFormat) {
        g_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        g_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        g_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    }
}

void Render(HWND hWnd) {
    if (!g_pDCRenderTarget) return;
    RECT rc; GetClientRect(hWnd, &rc);
    int w = rc.right, h = rc.bottom;
    
    HDC hdcS = GetDC(NULL), hdcM = CreateCompatibleDC(hdcS);
    BITMAPINFO bmi = {sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB};
    void* pBits; HBITMAP hbmp = CreateDIBSection(hdcM, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOld = (HBITMAP)SelectObject(hdcM, hbmp);

    g_pDCRenderTarget->BindDC(hdcM, &rc);
    g_pDCRenderTarget->BeginDraw();
    g_pDCRenderTarget->Clear(D2D1::ColorF(0,0,0, 0.0f));

    const auto& config = g_config.Data();
    ID2D1SolidColorBrush *pNormal, *pHi, *pInactive;
    
    bool isDark = IsTaskbarDarkMode();
    COLORREF baseColor = isDark ? RGB(255, 255, 255) : RGB(0, 0, 0);
    COLORREF highlight = config.highlightColor;
    
    // Use OPAQUE brushes to ensure visibility
    // Window transparency is handled by UpdateLayeredWindow
    g_pDCRenderTarget->CreateSolidColorBrush(ColorRefToD2D(baseColor, 0.8f), &pNormal); // Slightly dim for non-highlight
    g_pDCRenderTarget->CreateSolidColorBrush(ColorRefToD2D(highlight, 1.0f), &pHi);    // Highlight
    g_pDCRenderTarget->CreateSolidColorBrush(ColorRefToD2D(baseColor, 0.5f), &pInactive);

    int64_t curIdx = g_lyricMgr.GetCurrentLineIndex();
    if (curIdx != g_lastLyricIndex) {
        g_lastLine = g_currentLine; g_currentLine = g_lyricMgr.GetCurrentLyricText();
        g_secondLine = config.secondLineType == 0 ? g_lyricMgr.GetNextLyricText() : g_lyricMgr.GetCurrentTranslation();
        g_scrollPos = 0.0f; g_lastLyricIndex = curIdx;
    }
    
    // Ensure default text is shown if empty
    if (g_currentLine.empty()) g_currentLine = L"SPlayer Lyric Ready";
    
    if (g_scrollPos < 1.0f) g_scrollPos += 0.05f;

    float lineH = (float)h / (config.desktopDualLine ? 2.0f : 1.0f);
    float offsetV = (1.0f - g_scrollPos) * 6.0f; 

    // Helper to draw text
    auto DrawSingle = [&](const std::wstring& txt, float y, bool isCurrentLine) {
        if (txt.empty() || !g_pTextFormat) return;
        D2D1_RECT_F r = D2D1::RectF(0, y, (float)w, y + lineH);
        
        // Simplified rendering for stability first
        // TODO: Restore YRC per-word highlighting if needed, but text first
        g_pDCRenderTarget->DrawText(txt.c_str(), (UINT32)txt.length(), g_pTextFormat, r, isCurrentLine ? pHi : pInactive);
    };

    if (config.desktopDualLine) { 
        DrawSingle(g_currentLine, 0 - offsetV, true); 
        DrawSingle(g_secondLine, lineH - offsetV, false); 
    } else { 
        DrawSingle(g_currentLine, 0 - offsetV, true); 
    }
    
    if (pNormal) pNormal->Release(); if (pHi) pHi->Release(); if (pInactive) pInactive->Release();
    g_pDCRenderTarget->EndDraw();
    
    // Apply User's Window Transparency preference
    // Default to 255 if configured value is weird (e.g. 0)
    BYTE alpha = (config.desktopTransparency == 0) ? 255 : (BYTE)config.desktopTransparency;
    
    BLENDFUNCTION bl = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
    POINT ptS = {0,0}; SIZE sz = {w,h}; RECT rcW; GetWindowRect(hWnd, &rcW); POINT ptD = {rcW.left, rcW.top};
    UpdateLayeredWindow(hWnd, hdcS, &ptD, &sz, hdcM, &ptS, 0, &bl, ULW_ALPHA);
    SelectObject(hdcM, hOld); DeleteObject(hbmp); DeleteDC(hdcM); ReleaseDC(NULL, hdcS);
}

void CleanupD2D() {
    if (g_pTextFormat) g_pTextFormat->Release(); if (g_pDWriteFactory) g_pDWriteFactory->Release();
    if (g_pDCRenderTarget) g_pDCRenderTarget->Release(); if (g_pD2DFactory) g_pD2DFactory->Release();
    g_pTextFormat = nullptr; g_pDWriteFactory = nullptr; g_pDCRenderTarget = nullptr; g_pD2DFactory = nullptr;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TIMER: 
        if (wParam == 1) Render(hWnd); 
        else if (wParam == 2) {
            HWND hPopup = GetWindow(hWnd, GW_ENABLEDPOPUP);
            if (hPopup == hWnd || hPopup == NULL) UpdatePosition(hWnd);
        }
        return 0;
    case WM_RBUTTONUP: {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 101, L"歌词设置 (Settings)");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, 102, L"退出程序 (Exit)");
        POINT pt; GetCursorPos(&pt);
        SetForegroundWindow(hWnd);
        int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
        if (cmd == 101) {
            COptionsDialog dlg;
            if (dlg.DoModal() == IDOK) { CleanupD2D(); InitD2D(hWnd); UpdatePosition(hWnd); }
        }
        if (cmd == 102) PostQuitMessage(0);
        DestroyMenu(hMenu);
        return 0;
    }
    case WM_SETTINGS_CHANGED: CleanupD2D(); InitD2D(hWnd); Render(hWnd); return 0;
    case WM_NCHITTEST: return HTCLIENT; 
    case WM_SETCURSOR: SetCursor(LoadCursor(nullptr, IDC_HAND)); return TRUE;
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS* pos = (WINDOWPOS*)lParam;
        pos->hwndInsertAfter = HWND_TOPMOST;
        pos->flags &= ~SWP_HIDEWINDOW;
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
