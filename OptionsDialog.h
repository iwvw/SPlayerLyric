/*
 * SPlayerLyric - SPlayer Lyric Display Plugin for TrafficMonitor
 * 
 * Options Dialog
 */

#pragma once

#include "resource.h"

class COptionsDialog : public CDialogEx
{
public:
    COptionsDialog(CWnd* pParent = nullptr);

    enum { IDD = IDD_OPTIONS_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();

    afx_msg void OnBnClickedBtnColor();
    afx_msg void OnBnClickedBtnNormalColor();
    afx_msg void OnBnClickedBtnApply();

    DECLARE_MESSAGE_MAP()

private:
    int m_port;
    int m_width;
    int m_fontSize;
    int m_dualLineFontSize;
    CString m_fontName;
    BOOL m_fontBold;
    BOOL m_enableScroll;
    BOOL m_enableYrc;
    COLORREF m_highlightColor;
    COLORREF m_normalColor;
    int m_lyricOffset;
    int m_secondLineType;
    int m_desktopXOffset;
    int m_desktopTransparency;
    BOOL m_desktopDualLine;
    BOOL m_autoStart;
    BOOL m_hideWhenNotPlaying;
    int m_dualLineAlignment; // 0=Left, 1=Center, 2=Right, 3=Split
    
    // New Color Settings
    BOOL m_adaptiveColor;
    COLORREF m_darkHighlightColor;
    COLORREF m_darkNormalColor;
    COLORREF m_lightHighlightColor;
    COLORREF m_lightNormalColor;

    afx_msg void OnBnClickedBtnLightHighlight();
    afx_msg void OnBnClickedBtnLightNormal();

    void LoadSettings();
    void SaveSettings();
};
