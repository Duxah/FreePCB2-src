#pragma once
#include "afxwin.h"


// CDlgMyMessageBox dialog

class CDlgMyMessageBox : public CDialog
{
	DECLARE_DYNAMIC(CDlgMyMessageBox)

public:
	CDlgMyMessageBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMyMessageBox();

// Dialog Data
	enum { IDD = IDD_MY_MESSAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_message;
	CButton m_check_dont_show;
	void Initialize( LPCTSTR mess );
	BOOL bDontShowBoxState;
	CString m_mess;
	BOOL m_bHideCursor;
};
