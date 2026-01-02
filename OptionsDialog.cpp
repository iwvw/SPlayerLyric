/*
 * SPlayerLyric - Options Dialog Implementation (Fixed)
 */

#include "pch.h"
#include <afxwin.h>
#include <afxdialogex.h>
#include <afxdlgs.h>
#include "OptionsDialog.h"
#include "Config.h"

int CALLBACK EnumFontFamExProc(const LOGFONTW* lpelfe, const TEXTMETRICW* lpntme, DWORD FontType, LPARAM lParam)
{
    CComboBox* pCombo = (CComboBox*)lParam;
    if (pCombo->FindStringExact(-1, lpelfe->lfFaceName) == CB_ERR) pCombo->AddString(lpelfe->lfFaceName);
    return 1;
}

BEGIN_MESSAGE_MAP(COptionsDialog, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_COLOR, &COptionsDialog::OnBnClickedBtnColor)
    ON_BN_CLICKED(IDC_BTN_NORMAL_COLOR, &COptionsDialog::OnBnClickedBtnNormalColor)
    ON_BN_CLICKED(IDC_BTN_LIGHT_HIGHLIGHT, &COptionsDialog::OnBnClickedBtnLightHighlight)
    ON_BN_CLICKED(IDC_BTN_LIGHT_NORMAL, &COptionsDialog::OnBnClickedBtnLightNormal)
    ON_BN_CLICKED(IDC_BTN_APPLY, &COptionsDialog::OnBnClickedBtnApply)
END_MESSAGE_MAP()

COptionsDialog::COptionsDialog(CWnd* pParent)
    : CDialogEx(IDD_OPTIONS_DIALOG, pParent)
    , m_port(25885), m_width(300), m_fontSize(11), m_dualLineFontSize(9), m_fontName(L"Microsoft YaHei UI"), m_fontBold(FALSE)
    , m_enableScroll(TRUE), m_enableYrc(FALSE), m_highlightColor(RGB(0, 120, 215)), m_normalColor(RGB(180, 180, 180))
    , m_lyricOffset(0), m_secondLineType(0), m_desktopXOffset(60), m_desktopDualLine(FALSE), m_autoStart(TRUE)
    , m_desktopTransparency(255), m_hideWhenNotPlaying(FALSE), m_adaptiveColor(TRUE)
    , m_dualLineAlignment(0)
    , m_darkHighlightColor(RGB(0, 120, 215)), m_darkNormalColor(RGB(200, 200, 200))
    , m_lightHighlightColor(RGB(0, 80, 160)), m_lightNormalColor(RGB(60, 60, 60))
{
}

void COptionsDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_PORT, m_port); DDV_MinMaxInt(pDX, m_port, 1, 65535);
    DDX_Text(pDX, IDC_EDIT_WIDTH, m_width); DDV_MinMaxInt(pDX, m_width, 50, 2000);
    DDX_Text(pDX, IDC_EDIT_FONTSIZE, m_fontSize); DDV_MinMaxInt(pDX, m_fontSize, 6, 72);
    DDX_Text(pDX, IDC_EDIT_DUALLINE_FONTSIZE, m_dualLineFontSize); DDV_MinMaxInt(pDX, m_dualLineFontSize, 6, 72);
    DDX_Check(pDX, IDC_CHECK_BOLD, m_fontBold);
    DDX_Check(pDX, IDC_CHECK_SCROLL, m_enableScroll);
    DDX_Check(pDX, IDC_CHECK_YRC, m_enableYrc);
    DDX_Text(pDX, IDC_EDIT_OFFSET, m_lyricOffset);
    DDX_Text(pDX, IDC_EDIT_DESKTOP_X, m_desktopXOffset);
    DDX_Text(pDX, IDC_EDIT_DESKTOP_ALPHA, m_desktopTransparency); DDV_MinMaxInt(pDX, m_desktopTransparency, 0, 255);
    DDX_Check(pDX, IDC_CHECK_DESKTOP_DUALLINE, m_desktopDualLine);
    DDX_Check(pDX, IDC_CHECK_AUTOSTART, m_autoStart);
    DDX_Check(pDX, IDC_CHECK_HIDE_NOT_PLAYING, m_hideWhenNotPlaying);
    DDX_Check(pDX, IDC_CHECK_ADAPTIVE_COLOR, m_adaptiveColor);
}

BOOL COptionsDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    LoadSettings();
    UpdateData(FALSE);
    CComboBox* pFontCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FONT);
    if (pFontCombo) {
        LOGFONTW lf = { 0 }; lf.lfCharSet = DEFAULT_CHARSET;
        HDC hdc = ::GetDC(NULL); EnumFontFamiliesExW(hdc, &lf, (FONTENUMPROCW)EnumFontFamExProc, (LPARAM)pFontCombo, 0); ::ReleaseDC(NULL, hdc);
        int idx = pFontCombo->FindStringExact(-1, m_fontName);
        if (idx != CB_ERR) pFontCombo->SetCurSel(idx);
        else pFontCombo->SetCurSel(pFontCombo->AddString(m_fontName));
    }
    CComboBox* pSecondLineCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SECONDLINE);
    if (pSecondLineCombo) {
        pSecondLineCombo->AddString(L"下一行"); 
        pSecondLineCombo->AddString(L"翻译"); 
        pSecondLineCombo->AddString(L"歌手"); 
        pSecondLineCombo->SetCurSel(m_secondLineType);
    }
    CComboBox* pAlignCombo = (CComboBox*)GetDlgItem(IDC_COMBO_ALIGNMENT);
    if (pAlignCombo) {
        pAlignCombo->AddString(L"左对齐");
        pAlignCombo->AddString(L"居中");
        pAlignCombo->AddString(L"右对齐");
        pAlignCombo->AddString(L"两侧分布");
        pAlignCombo->SetCurSel(m_dualLineAlignment);
    }
    return TRUE;
}

void COptionsDialog::OnOK() { OnBnClickedBtnApply(); CDialogEx::OnOK(); }

void COptionsDialog::OnBnClickedBtnColor() {
    CColorDialog dlg(m_darkHighlightColor, CC_FULLOPEN | CC_ANYCOLOR, this);
    if (dlg.DoModal() == IDOK) m_darkHighlightColor = dlg.GetColor();
}

void COptionsDialog::OnBnClickedBtnNormalColor() {
    CColorDialog dlg(m_darkNormalColor, CC_FULLOPEN | CC_ANYCOLOR, this);
    if (dlg.DoModal() == IDOK) m_darkNormalColor = dlg.GetColor();
}

void COptionsDialog::OnBnClickedBtnLightHighlight() {
    CColorDialog dlg(m_lightHighlightColor, CC_FULLOPEN | CC_ANYCOLOR, this);
    if (dlg.DoModal() == IDOK) m_lightHighlightColor = dlg.GetColor();
}

void COptionsDialog::OnBnClickedBtnLightNormal() {
    CColorDialog dlg(m_lightNormalColor, CC_FULLOPEN | CC_ANYCOLOR, this);
    if (dlg.DoModal() == IDOK) m_lightNormalColor = dlg.GetColor();
}

void COptionsDialog::OnBnClickedBtnApply() {
    if (!UpdateData(TRUE)) return;
    CComboBox* pFontCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FONT);
    if (pFontCombo) { int idx = pFontCombo->GetCurSel(); if (idx != CB_ERR) pFontCombo->GetLBText(idx, m_fontName); }
    CComboBox* pSecondLineCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SECONDLINE);
    if (pSecondLineCombo) { m_secondLineType = pSecondLineCombo->GetCurSel(); if (m_secondLineType == CB_ERR) m_secondLineType = 0; }
    CComboBox* pAlignCombo = (CComboBox*)GetDlgItem(IDC_COMBO_ALIGNMENT);
    if (pAlignCombo) { m_dualLineAlignment = pAlignCombo->GetCurSel(); if (m_dualLineAlignment == CB_ERR) m_dualLineAlignment = 0; }
    SaveSettings();
    CWnd* pParent = GetParent(); if (pParent) pParent->SendMessage(WM_SETTINGS_CHANGED, 0, 0);
}

void COptionsDialog::LoadSettings() {
    const auto& config = g_config.Data();
    m_port = config.wsPort; m_width = config.displayWidth; m_fontSize = config.fontSize;
    m_dualLineFontSize = config.dualLineFontSize;
    m_fontBold = config.fontWeightBold ? TRUE : FALSE; m_fontName = config.fontName.c_str();
    m_enableScroll = config.enableScrolling ? TRUE : FALSE; m_enableYrc = config.enableYrc ? TRUE : FALSE;
    m_highlightColor = config.highlightColor; m_normalColor = config.normalColor;
    m_lyricOffset = config.lyricOffset; m_secondLineType = config.secondLineType;
    m_dualLineAlignment = config.dualLineAlignment;
    m_desktopXOffset = config.desktopXOffset; m_desktopDualLine = config.desktopDualLine ? TRUE : FALSE;
    m_autoStart = config.autoStart ? TRUE : FALSE; m_desktopTransparency = config.desktopTransparency;
    m_hideWhenNotPlaying = config.hideWhenNotPlaying ? TRUE : FALSE;
    
    // Load new color settings
    m_adaptiveColor = config.adaptiveColor ? TRUE : FALSE;
    m_darkHighlightColor = config.darkHighlightColor;
    m_darkNormalColor = config.darkNormalColor;
    m_lightHighlightColor = config.lightHighlightColor;
    m_lightNormalColor = config.lightNormalColor;
}

void COptionsDialog::SaveSettings() {
    auto& config = g_config.Data();
    config.wsPort = m_port; config.displayWidth = m_width; config.fontSize = m_fontSize;
    config.dualLineFontSize = m_dualLineFontSize;
    config.fontWeightBold = (m_fontBold != FALSE); config.fontName = m_fontName.GetString();
    config.enableScrolling = (m_enableScroll != FALSE); config.enableYrc = (m_enableYrc != FALSE);
    config.highlightColor = m_highlightColor; config.normalColor = m_normalColor;
    config.lyricOffset = m_lyricOffset; config.secondLineType = m_secondLineType;
    config.dualLineAlignment = m_dualLineAlignment;
    config.desktopXOffset = m_desktopXOffset; config.desktopDualLine = (m_desktopDualLine != FALSE);
    config.autoStart = (m_autoStart != FALSE); config.desktopTransparency = m_desktopTransparency;
    config.hideWhenNotPlaying = (m_hideWhenNotPlaying != FALSE);
    
    // Save new color settings
    config.adaptiveColor = (m_adaptiveColor != FALSE);
    config.darkHighlightColor = m_darkHighlightColor;
    config.darkNormalColor = m_darkNormalColor;
    config.lightHighlightColor = m_lightHighlightColor;
    config.lightNormalColor = m_lightNormalColor;
    
    // Also update legacy fields for backward compatibility if needed, using dark mode values
    config.highlightColor = m_darkHighlightColor;
    config.normalColor = m_darkNormalColor;
    g_config.Save();
}
