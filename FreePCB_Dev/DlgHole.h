#pragma once
#include "afxwin.h"


// CDlgHole dialog

class CDlgHole : public CDialog
{
	DECLARE_DYNAMIC(CDlgHole)

public:
	CDlgHole(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgHole();
	void Initialize( int units, int w, int x, int y, BOOL bPlated=FALSE );
	void SetFields();
	void GetFields();

// Dialog Data
	enum { IDD = IDD_HOLE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bDrag;
	BOOL m_bPlated;
	int m_units, m_x, m_y, m_w;
	// controls
	CButton m_radio_set, m_radio_drag;
	CButton m_radio_plated, m_radio_unplated;
	CComboBox m_combo_units;
	CEdit m_edit_x;
	CEdit m_edit_y;
	CEdit m_edit_w;
	// message handlers
	afx_msg void OnCbnSelChangeCombo1();
	afx_msg void OnBnClickedSet();
	afx_msg void OnBnClickedDrag();
	afx_msg void OnBnClickedPlated();
	afx_msg void OnBnClickedUnplated();
};
