#pragma once
#include "resource.h"

// CDlgLog dialog

class CDlgLog : public CDialog      
{
	DECLARE_DYNAMIC(CDlgLog)

public:
	enum { 
		STATUS_CANCEL = 0x1,
		STATUS_OK = 0x2
	};
	CDlgLog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgLog();
	void Move( int x, int y ); 
//	void AddLine( CString * str );
	void AddLine( LPCTSTR str );
	void Clear();

// Dialog Data
	enum { IDD = IDD_LOG };
	BOOL m_running;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CEdit m_edit_log;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedHideMe();
};
