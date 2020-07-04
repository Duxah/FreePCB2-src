#pragma once
#include "afxwin.h"

// CDlgAddPin dialog

class CDlgAddPin : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddPin)

public:
	enum { ADD, EDIT };		// modes
	CDlgAddPin(CWnd* pParent = NULL);   // standard constructor 
	virtual ~CDlgAddPin();
	void InitDialog( CShape * fp, int mode, 
		int pin_num, int units );
//	void EnableFields();

	CShape * m_fp;	// original footprint

// Dialog Data
	enum { IDD = IDD_ADD_PIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void SetFields();
	void GetFields();

	DECLARE_MESSAGE_MAP()
public:
	int m_mode;				// ADD or EDIT
	int m_units;			// MIL or MM
	int m_pin_num;			// pin num to EDIT
	int m_as_pin;			// m_combo_same_as_pin
	CString m_pin_name;
	CString m_pin_description;
	int m_num_pins;
	int m_increment;
	int m_same_as_pin_flag;
	CString m_same_as_pin_name;
	int m_ps_angle;
	int m_padstack_type;	// 0=SMT, 1=TH, 2=SMT(bottom)
	int m_hole_diam;		// for TH 
	int m_top_shape;
	int m_top_width;
	int m_top_length;
	int m_top_radius;
	int m_top_mask_shape;
	int m_top_mask_width;
	int m_top_mask_length;
	int m_top_mask_radius;
	int m_top_paste_shape;
	int m_top_paste_width;
	int m_top_paste_length;
	int m_top_paste_radius;
	int m_top_flags;

	int m_inner_shape;
	int m_inner_width;
	int m_inner_length;
	int m_inner_radius;
	int m_inner_flags;

	BOOL m_bottom_same_as_top;
	int m_bottom_shape;
	int m_bottom_width;
	int m_bottom_length;
	int m_bottom_radius;
	int m_bottom_mask_shape;
	int m_bottom_mask_width;
	int m_bottom_mask_length;
	int m_bottom_mask_radius;
	int m_bottom_paste_shape;
	int m_bottom_paste_width;
	int m_bottom_paste_length;
	int m_bottom_paste_radius;
	int m_bottom_flags;

	int m_x;
	int m_y;
	int m_x_edge;
	int m_y_edge;

	int m_row_orient;
	int m_row_spacing;
	BOOL m_drag_flag;	// TRUE to drag pad after exiting

	CButton m_radio_add_pin;
	CButton m_radio_add_row;
	afx_msg void OnBnClickedRadioAddPin();
	afx_msg void OnBnClickedRadioAddRow();
	CComboBox m_combo_units;
	CButton m_check_same_as_pin;
	CButton m_radio_smt;
	CButton m_radio_th;
	CComboBox m_combo_same_as_pin;
	CEdit m_edit_pin_name;
	CEdit m_edit_pin_description;
	CEdit m_edit_num_pins;
	CEdit m_edit_increment;
	CEdit m_edit_hole_diam;
	CButton m_radio_drag;
	afx_msg void OnBnClickedRadioDragPin();
	afx_msg void OnBnClickedRadioSetPinPos();
	CEdit m_edit_pin_x;
	CEdit m_edit_pin_y;
	CComboBox m_combo_top_shape;
	CEdit m_edit_top_width;
	CEdit m_edit_top_length;
	CEdit m_edit_top_radius;
	CComboBox m_combo_top_shape2;
	CEdit m_edit_top_width2;
	CEdit m_edit_top_length2;
	CEdit m_edit_top_radius2;
	CComboBox m_combo_top_shape3;
	CEdit m_edit_top_width3;
	CEdit m_edit_top_length3;
	CEdit m_edit_top_radius3;
	afx_msg void OnBnClickedCheckInnerSameAs();
	CComboBox m_combo_inner_shape;
	CEdit m_edit_inner_width;
	CEdit m_edit_inner_length;
	CEdit m_edit_inner_radius;
	CButton m_check_bottom_same_as;
	afx_msg void OnBnClickedCheckBottomSameAs();
	CComboBox m_combo_bottom_shape;
	CEdit m_edit_bottom_width;
	CEdit m_edit_bottom_length;
	CEdit m_edit_bottom_radius;
	CComboBox m_combo_bottom_shape2;
	CEdit m_edit_bottom_width2;
	CEdit m_edit_bottom_length2;
	CEdit m_edit_bottom_radius2;
	CComboBox m_combo_bottom_shape3;
	CEdit m_edit_bottom_width3;
	CEdit m_edit_bottom_length3;
	CEdit m_edit_bottom_radius3;
	CComboBox m_combo_row_orient;
	CEdit m_edit_row_spacing;
	CListBox m_list_pins;
	CComboBox m_combo_x_edge;
	CComboBox m_combo_y_edge;
	CButton m_radio_set_pos;
	CButton m_radio_connect[3][4];
	afx_msg void OnCbnSelchangeComboPinUnits();
	afx_msg void OnCbnSelchangeComboRowOrient();
	afx_msg void OnChange();
	afx_msg void OnChangeTopPadParam();
	afx_msg void OnBnClickedCheckBottomSameAs2();
	afx_msg void OnBnClickedOk();
};
