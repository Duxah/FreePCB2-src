#pragma once

// CDlgSideStyle dialog

class CDlgSideStyle : public CDialog
{
	DECLARE_DYNAMIC(CDlgSideStyle)

public:
	CDlgSideStyle(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSideStyle();
	void Initialize( int style );
	
	int m_style;

// Dialog Data
	enum { IDD = IDD_SIDE_STYLE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
