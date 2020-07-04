#pragma once
#include "afxwin.h"


// CDlgChooseNetName dialog

class CDlgChooseNetName : public CDialog
{
	DECLARE_DYNAMIC(CDlgChooseNetName)

public:
	CDlgChooseNetName(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgChooseNetName();
	void Initialize( CArray<CString> * str );

// Dialog Data
	enum { IDD = IDD_CHOOSE_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_combo_names;
	CArray<CString> * m_str;
	CString m_sel_str;
};
