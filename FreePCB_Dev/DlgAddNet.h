#pragma once


// CDlgAddNet dialog

class CDlgAddNet : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddNet)

public:
	CDlgAddNet(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddNet();

// Dialog Data
	enum { IDD = IDD_ADD_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
