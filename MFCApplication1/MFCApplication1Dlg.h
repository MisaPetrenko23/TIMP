
// MFCApplication1Dlg.h: файл заголовка
//

#pragma once


// Диалоговое окно CMFCApplication1Dlg
class CMFCApplication1Dlg : public CDialog
{
// Создание
public:
	CMFCApplication1Dlg(CWnd* pParent = nullptr);	// стандартный конструктор

// Данные диалогового окна
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCAPPLICATION1_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// поддержка DDX/DDV


// Реализация
protected:
	HICON m_hIcon;

	// Созданные функции схемы сообщений
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strDecryptedText;
	CString m_strPlainText;
	CString m_strCipherText;
	CString m_strCrackedText;
	CString m_strFilePath;
	int m_nShift;
	CString m_strEvaluation;
	afx_msg void OnBnClickedBtnOpen();
	afx_msg void OnBnClickedBtnEncrypt();
	afx_msg void OnBnClickedBtnDecrypt();
	afx_msg void OnBnClickedBtnCrack();
};
