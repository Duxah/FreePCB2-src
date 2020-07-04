#pragma once
#include "afxwin.h"


// CDlgPadFlags dialog

class CDlgPadFlags : public CDialog
{
	DECLARE_DYNAMIC(CDlgPadFlags)

public:
	CDlgPadFlags(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgPadFlags();
	void Initialize( flag flags );

// Dialog Data
	enum { IDD = IDD_PAD_FLAGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	flag m_flags;
	CButton m_check_no_mask;
	CButton m_check_area_no_connect;
	CButton m_check_area_no_thermal;
	CButton m_check_area_thermal;
	afx_msg void OnBnClickedCheckNoArea();
	afx_msg void OnBnClickedCheckAreaThermal();
	afx_msg void OnBnClickedCheckArea();
};
