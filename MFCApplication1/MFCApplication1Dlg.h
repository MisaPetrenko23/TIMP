
// MFCApplication1Dlg.h: файл заголовка
//

#pragma once

#include "FirebirdDb.h"


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
	CString m_strHistory;
	int m_nShift;
	CString m_strEvaluation;
	afx_msg void OnBnClickedBtnOpen();
	afx_msg void OnBnClickedBtnEncrypt();
	afx_msg void OnBnClickedBtnDecrypt();
	afx_msg void OnBnClickedBtnCrack();
	afx_msg void OnBnClickedBtnHistory();

private:
	// --- Подключение к RedDatabase / Firebird ---
	bool ConnectDatabase();
	bool EnsureFileRecord();
	bool LogOperation(const std::string& operationType,
	                  int shift,
	                  int foundShift = -1,
	                  double fitnessScore = 0.0,
	                  int textLength = 0);
	void ShowDbError(const std::string& error);

	FirebirdDb m_db;
	CString     m_strDbStatus;
	int         m_nUserId{ 0 };
	int         m_nFileDbId{ 0 };
	int         m_nFileSize{ 0 };
	std::string m_strFileNameUtf8;
	std::string m_strFileHashUtf8;
	bool        m_bDbWarned{ false };
};
