#pragma once


// CDlgAddWidth dialog

class CDlgAddWidth : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddWidth)

public:
	CDlgAddWidth(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddWidth();

// Dialog Data
	enum { IDD = IDD_ADD_WIDTH_MENU_ITEM };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	char m_width[10];
	char m_via_w[10];
	char m_via_hole_w[10];
};
