#pragma once
#include "afxwin.h"


// CDlgCentroid dialog

class CDlgCentroid : public CDialog
{
	DECLARE_DYNAMIC(CDlgCentroid)

public:
	CDlgCentroid(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCentroid();
	void Initialize( CENTROID_TYPE type, int units, int x, int y, int angle );
	void SetFields();
	void GetFields();

// Dialog Data
	enum { IDD = IDD_CENTROID };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CENTROID_TYPE m_type;
	int m_units, m_x, m_y, m_angle;
	CButton m_button_default;
	CButton m_button_set;
	CComboBox m_combo_units;
	CEdit m_edit_x;
	CEdit m_edit_y;
	afx_msg void OnBnClickedDefault();
	afx_msg void OnBnClickedSet();
	afx_msg void OnCbnSelChangeCombo1();
	CComboBox m_combo_angle;
};
