#pragma once
#include "afxwin.h"


// CDlgAddText dialog

class CDlgAddText : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddText)

public:
	CDlgAddText(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddText();
	void Initialize( BOOL fp_flag, int num_layers, int drag_flag, 
		CString * str, int units, int layer, int mirror, BOOL bNegative,
		int height, int width, int x, int y );

// Dialog Data
	enum { IDD = IDD_ADD_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL bNewText;
	void SetFields();
	void GetFields();
	BOOL m_fp_flag;
	int m_num_layers;
	int m_x, m_y;			// set on entry if editing
	int m_width;
	int m_height;
	BOOL m_bDrag;
	BOOL m_bMirror;
	BOOL m_bNegative;
	BOOL m_bUseDefaultWidth;
	int m_layer;
	int m_units;
	int m_unit_mult;
	CString m_str;
	CListBox m_layer_list;
	virtual BOOL OnInitDialog();
	CEdit m_edit_height;
	CButton m_button_set_width;
	CButton m_button_def_width;
	CEdit m_edit_width;
	CEdit m_text;
	CEdit m_edit_y;
	CEdit m_edit_x;
	CButton m_button_set_position;
	CButton m_button_drag;
	afx_msg void OnBnClickedSetPosition();
	afx_msg void OnBnClickedDrag();
	afx_msg void OnEnChangeEditHeight();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedSetWidth();
	afx_msg void OnBnClickedDefWidth();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboAddTextUnits();
	CButton m_check_negative;
	CButton m_check_mirror;
	afx_msg void OnLbnSelchangeListLayer();
};
