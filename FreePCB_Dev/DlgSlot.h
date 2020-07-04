#pragma once
#include "afxwin.h"


// CDlgSlot dialog

class CDlgSlot : public CDialog
{
	DECLARE_DYNAMIC(CDlgSlot)

public:
	CDlgSlot(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSlot();
	void Initialize( int units, int w, int travel, int dir, 
		int x, int y, BOOL bPlated=FALSE, BOOL bDrag=TRUE );
	void SetFields();
	void GetFields();

// Dialog Data
	enum { IDD = IDD_SLOT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bDrag;
	BOOL m_bPlated;
	int m_units, m_x, m_y, m_w, m_travel, m_dir;
	// controls
	CButton m_radio_set, m_radio_drag;
	CButton m_radio_plated, m_radio_unplated;
	CComboBox m_combo_units;
	CEdit m_edit_x;
	CEdit m_edit_y;
	CEdit m_edit_w;
	CEdit m_edit_travel;
	CComboBox m_combo_direction;
	// message handlers
	afx_msg void OnCbnSelChangeCombo1();
	afx_msg void OnBnClickedSet();
	afx_msg void OnBnClickedDrag();
	afx_msg void OnBnClickedPlated();
	afx_msg void OnBnClickedUnplated();
};
