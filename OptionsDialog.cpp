/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Options Dialog Implementation
 */

#include "pch.h"
#include "OptionsDialog.h"
#include "Config.h"

// Font enumeration callback
int CALLBACK EnumFontFamExProc(const LOGFONTW* lpelfe, const TEXTMETRICW* lpntme, DWORD FontType, LPARAM lParam)
{
    CComboBox* pCombo = (CComboBox*)lParam;
    if (pCombo->FindStringExact(-1, lpelfe->lfFaceName) == CB_ERR)
    {
        pCombo->AddString(lpelfe->lfFaceName);
    }
    return 1;
}

BEGIN_MESSAGE_MAP(COptionsDialog, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_COLOR, &COptionsDialog::OnBnClickedBtnColor)
    ON_BN_CLICKED(IDC_BTN_NORMAL_COLOR, &COptionsDialog::OnBnClickedBtnNormalColor)
END_MESSAGE_MAP()

COptionsDialog::COptionsDialog(CWnd* pParent)
    : CDialogEx(IDD_OPTIONS_DIALOG, pParent)
    , m_port(25885)
    , m_width(300)
    , m_fontSize(11)
    , m_fontName(L"Microsoft YaHei UI")
    , m_fontBold(FALSE)
    , m_enableScroll(TRUE)
    , m_enableYrc(FALSE)
    , m_highlightColor(RGB(0, 120, 215))
    , m_normalColor(RGB(180, 180, 180))
    , m_lyricOffset(0)
    , m_dualLine(FALSE)
    , m_secondLineType(0)
{
}

void COptionsDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_PORT, m_port);
    DDV_MinMaxInt(pDX, m_port, 1, 65535);
    DDX_Text(pDX, IDC_EDIT_WIDTH, m_width);
    DDV_MinMaxInt(pDX, m_width, 50, 1000);
    DDX_Text(pDX, IDC_EDIT_FONTSIZE, m_fontSize);
    DDV_MinMaxInt(pDX, m_fontSize, 6, 72);
    DDX_Check(pDX, IDC_CHECK_BOLD, m_fontBold);
    DDX_Check(pDX, IDC_CHECK_SCROLL, m_enableScroll);
    DDX_Check(pDX, IDC_CHECK_YRC, m_enableYrc);
    DDX_Text(pDX, IDC_EDIT_OFFSET, m_lyricOffset);
    DDX_Check(pDX, IDC_CHECK_DUALLINE, m_dualLine);
}

BOOL COptionsDialog::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    LoadSettings();
    UpdateData(FALSE);

    // Enumerate system fonts
    CComboBox* pFontCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FONT);
    if (pFontCombo)
    {
        LOGFONTW lf = { 0 };
        lf.lfCharSet = DEFAULT_CHARSET;
        
        HDC hdc = ::GetDC(NULL);
        EnumFontFamiliesExW(hdc, &lf, (FONTENUMPROCW)EnumFontFamExProc, (LPARAM)pFontCombo, 0);
        ::ReleaseDC(NULL, hdc);

        int idx = pFontCombo->FindStringExact(-1, m_fontName);
        if (idx != CB_ERR)
            pFontCombo->SetCurSel(idx);
        else
        {
            idx = pFontCombo->AddString(m_fontName);
            pFontCombo->SetCurSel(idx);
        }
    }

    // Initialize second line type combo
    CComboBox* pSecondLineCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SECONDLINE);
    if (pSecondLineCombo)
    {
        pSecondLineCombo->AddString(L"Next Line");
        pSecondLineCombo->AddString(L"Translation");
        pSecondLineCombo->SetCurSel(m_secondLineType);
    }

    return TRUE;
}

void COptionsDialog::OnOK()
{
    if (!UpdateData(TRUE))
        return;

    // Get font from combo
    CComboBox* pFontCombo = (CComboBox*)GetDlgItem(IDC_COMBO_FONT);
    if (pFontCombo)
    {
        int idx = pFontCombo->GetCurSel();
        if (idx != CB_ERR)
            pFontCombo->GetLBText(idx, m_fontName);
    }

    // Get second line type from combo
    CComboBox* pSecondLineCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SECONDLINE);
    if (pSecondLineCombo)
    {
        m_secondLineType = pSecondLineCombo->GetCurSel();
        if (m_secondLineType == CB_ERR)
            m_secondLineType = 0;
    }

    SaveSettings();
    CDialogEx::OnOK();
}

void COptionsDialog::OnBnClickedBtnColor()
{
    CColorDialog dlg(m_highlightColor, CC_FULLOPEN | CC_ANYCOLOR, this);
    if (dlg.DoModal() == IDOK)
    {
        m_highlightColor = dlg.GetColor();
    }
}

void COptionsDialog::OnBnClickedBtnNormalColor()
{
    CColorDialog dlg(m_normalColor, CC_FULLOPEN | CC_ANYCOLOR, this);
    if (dlg.DoModal() == IDOK)
    {
        m_normalColor = dlg.GetColor();
    }
}

void COptionsDialog::LoadSettings()
{
    const auto& config = g_config.Data();
    m_port = config.wsPort;
    m_width = config.displayWidth;
    m_fontSize = config.fontSize;
    m_fontBold = config.fontWeightBold ? TRUE : FALSE;
    m_fontName = config.fontName.c_str();
    m_enableScroll = config.enableScrolling ? TRUE : FALSE;
    m_enableYrc = config.enableYrc ? TRUE : FALSE;
    m_highlightColor = config.highlightColor;
    m_normalColor = config.normalColor;
    m_lyricOffset = config.lyricOffset;
    m_dualLine = config.dualLineDisplay ? TRUE : FALSE;
    m_secondLineType = config.secondLineType;
}

void COptionsDialog::SaveSettings()
{
    auto& config = g_config.Data();
    config.wsPort = m_port;
    config.displayWidth = m_width;
    config.fontSize = m_fontSize;
    config.fontWeightBold = (m_fontBold != FALSE);
    config.fontName = m_fontName.GetString();
    config.enableScrolling = (m_enableScroll != FALSE);
    config.enableYrc = (m_enableYrc != FALSE);
    config.highlightColor = m_highlightColor;
    config.normalColor = m_normalColor;
    config.lyricOffset = m_lyricOffset;
    config.dualLineDisplay = (m_dualLine != FALSE);
    config.secondLineType = m_secondLineType;

    g_config.Save();
}
