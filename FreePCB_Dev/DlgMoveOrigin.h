#pragma once
#include "afxwin.h"


// CDlgMoveOrigin dialog

class CDlgMoveOrigin : public CDialog
{
	DECLARE_DYNAMIC(CDlgMoveOrigin)

public:
	CDlgMoveOrigin(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgMoveOrigin();

// Dialog Data
	enum { IDD = IDD_MOVE_ORIGIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void Initialize( int units );
	void GetFields();
	void SetFields();
	int m_units;
	double m_mult;
	int m_x, m_y;
	BOOL m_drag;
	CButton m_radio_drag;
	CButton m_radio_set;
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeUnits();
	CEdit m_edit_x;
	CEdit m_edit_y;
	afx_msg void OnBnClickedDrag();
	afx_msg void OnBnClickedSet();
};
