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

    DECLARE_MESSAGE_MAP()

private:
    int m_port;
    int m_width;
    int m_fontSize;
    CString m_fontName;
    BOOL m_fontBold;
    BOOL m_enableScroll;
    BOOL m_enableYrc;
    COLORREF m_highlightColor;
    COLORREF m_normalColor;
    int m_lyricOffset;
    BOOL m_dualLine;
    int m_secondLineType;

    void LoadSettings();
    void SaveSettings();
};
