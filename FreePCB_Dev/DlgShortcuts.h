#pragma once


// CDlgShortcuts dialog

class CDlgShortcuts : public CDialog
{
	DECLARE_DYNAMIC(CDlgShortcuts)

public:
	CDlgShortcuts(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgShortcuts();

// Dialog Data
	enum { IDD = IDD_SHORTCUTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
