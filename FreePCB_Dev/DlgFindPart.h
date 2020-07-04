#pragma once
#include "afxwin.h"


// CDlgFindPart dialog

class CDlgFindPart : public CDialog
{
	DECLARE_DYNAMIC(CDlgFindPart)

public:
	CDlgFindPart(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgFindPart();

// Dialog Data
	enum { IDD = IDD_SHOW_PART };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_combo_ref_des;
	CPartList * m_pl;
	CString sel_ref_des;
	void Initialize( CPartList * pl );
};
