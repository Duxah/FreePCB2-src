// ivexDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CivexDlg dialog
class CivexDlg : public CDialog
{
// Construction
public:
	CivexDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_IVEX_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString in_pathname;
	CString out_pathname;
	afx_msg void OnBnClickedButtonInfile();
	afx_msg void OnBnClickedButtonOutfile();
	afx_msg void OnEnChangeEditInfile();
	afx_msg void OnEnChangeEditOutfile();
	CEdit m_edit_infile;
	CEdit m_edit_outfile;
	CEdit m_edit_messages;
	afx_msg void OnBnClickedConvert();
};
