#pragma once
#include "afxwin.h"


// CDlgMyMessageBox dialog

class CDlgMyMessageBox2 : public CDialog
{
	DECLARE_DYNAMIC(CDlgMyMessageBox2)

public:
	CDlgMyMessageBox2(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMyMessageBox2();

// Dialog Data
	enum { IDD = IDD_MY_MESSAGE2 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_message;
	CButton m_check_dont_show;
	void Initialize( CString * mess );
	BOOL bDontShowBoxState;
	CString * m_mess;
	BOOL m_bHideCursor;
};
