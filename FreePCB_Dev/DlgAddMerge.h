#pragma once


// CDlgAddMerge dialog

class CDlgAddMerge : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddMerge)

public:
	CDlgAddMerge(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddMerge();

// Dialog Data
	enum { IDD = IDD_ADD_MERGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_cl; // area clearance
	CString m_merge_name; //
	CComboBox m_edit;
	CEdit m_edit_cl;
	CEdit m_edit_u;
	CFreePcbDoc * Doc;
};