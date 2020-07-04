#pragma once
#include "afxwin.h"


// CDlgGlue dialog

class CDlgGlue : public CDialog
{
	DECLARE_DYNAMIC(CDlgGlue)

public:
	CDlgGlue(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgGlue();
	void Initialize( GLUE_POS_TYPE pos_type, int units, int w, int x, int y );
	void SetFields();
	void GetFields();

// Dialog Data
	enum { IDD = IDD_GLUE_SPOT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	GLUE_POS_TYPE m_pos_type;
	int m_units, m_x, m_y, m_w;
	BOOL m_bUseDefaultSize;
	BOOL m_bDrag;
	CButton m_button_default;
	CButton m_button_set;
	CComboBox m_combo_units;
	CEdit m_edit_x;
	CEdit m_edit_y;
	afx_msg void OnBnClickedDefault();
	afx_msg void OnBnClickedSet();
	afx_msg void OnCbnSelChangeCombo1();
	CButton m_radio_drag;
	afx_msg void OnBnClickedDrag();
	afx_msg void OnBnClickedDefaultSize();
	CButton m_radio_default_size;
	CButton m_radio_set_size;
	CEdit m_edit_w;
	afx_msg void OnBnClickedRadioSetSize();
};
