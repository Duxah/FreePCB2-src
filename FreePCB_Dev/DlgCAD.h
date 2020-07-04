#pragma once
#include "afxwin.h"
#include "DlgLog.h"


// CDlgCAD dialog

class CDlgCAD : public CDialog
{
	DECLARE_DYNAMIC(CDlgCAD)

public:
	CDlgCAD(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCAD();
	void Initialize(	double version, 
						CString * folder, 
						CString * project_folder,
						CString * app_folder,
						CString * file_name,
						int num_copper_layers, 
						int units, 
						BOOL bSMTconnect,
						int fill_clearance, 
						int mask_clearance, 
						int thermal_width,
						int pilot_diameter, 
						int min_silkscreen_wid,
						int highlight_width, 
						int hole_clearance, 
						int thermal_clearance,
						int annular_ring_pins, 
						int annular_ring_vias, 
						int shrink_paste,
						int n_x, int n_y, int space_x, int space_y,
						int flags, int layers, int drill_file,
						CArray<CPolyLine> * op, 
						BOOL * bShowMessageForClearance,
						CPartList * pl, CNetList * nl, CTextList * tl, CDisplayList * dl, Merge * ml,
						CDlgLog * log );
	void SetFields();
	void GetFields();
// Dialog Data
	enum { IDD = IDD_CAD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bShowMessageForClearance;
	BOOL m_bSMT_connect;
	double m_version;
	double m_file_version;
	CEdit m_edit_folder;
	CEdit m_edit_fill;
	CEdit m_edit_mask;
	CButton m_check_drill;
	CButton m_check_top_silk;
	CButton m_check_bottom_silk;
	CButton m_check_top_solder;
	CButton m_check_bottom_solder;
	CButton m_check_top_copper;
	CButton m_check_bottom_copper;
	CButton m_check_inner1;
	CButton m_check_inner2;
	CButton m_check_inner3;
	CButton m_check_inner4;
	CButton m_check_inner5;
	CButton m_check_inner6;
	CButton m_check_inner7;
	CButton m_check_inner8;
	CButton m_check_inner9;
	CButton m_check_inner10;
	CButton m_check_inner11;
	CButton m_check_inner12;
	CButton m_check_inner13;
	CButton m_check_inner14;
	CButton m_check_scribing;
	CButton m_check_refinetop;
	CButton m_check_refinebot;
	CButton m_check_outline;
	CButton m_check_moires;
	CButton m_check_layer_text;
	int m_num_copper_layers;
	int m_fill_clearance;
	int m_hole_clearance;
	int m_mask_clearance;
	int m_thermal_width;
	int m_thermal_clearance;
	int m_pilot_diameter;
	int m_min_silkscreen_width;
	int m_highlight_width;
	int m_annular_ring_pins;
	int m_annular_ring_vias;
	int m_units;
	int m_flags;
	int m_layers;
	int m_drill_file;
	CArray<CPolyLine> * m_op;
	CPartList * m_pl; 
	CNetList * m_nl; 
	CTextList * m_tl; 
	CDisplayList * m_dl;
	Merge * m_ml;
	CDlgLog * m_dlg_log;
	CString m_folder;
	CString m_project_folder;
	CString m_app_folder;
	CString m_f_name;
	afx_msg void OnBnClickedGo();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboCadUnits();
	CEdit m_edit_pilot_diam;
	CButton m_check_pilot;
	afx_msg void OnBnClickedCheckCadPilot();
	afx_msg void OnBnClickedCancel();
	CEdit m_edit_min_ss_w;
	CEdit m_edit_high_w;
	CEdit m_edit_thermal_width;
	CEdit m_edit_thermal_clearance;
	CEdit m_edit_hole_clearance;
	CEdit m_edit_ann_pins;
	CEdit m_edit_ann_vias;
	CButton m_check_thermal_pins;
	CButton m_check_thermal_vias;
	CButton m_check_mask_vias;
	afx_msg void OnBnClickedButtonDef();
	afx_msg void OnBnClickedButtonFolder();
	CButton m_check_board;
	CButton m_check_top_paste;
	CButton m_check_bottom_paste;
	int m_n_x;
	int m_n_y;
	CEdit m_edit_n_x;
	CEdit m_edit_n_y;
	CEdit m_edit_space_x;
	CEdit m_edit_space_y;
	int m_space_x, m_space_y;
	CEdit m_edit_shrink_paste;
	int m_paste_shrink;
	CButton m_check_90;
	afx_msg void OnBnClickedThermalPins();
	afx_msg void OnBnClickedThermalVias();
	CButton m_check_render_all;
	CButton m_check_mirror_bottom;
	afx_msg void OnBnClickedRenderAllGerbers();
	CButton m_check_smt_thermals;
	afx_msg void OnBnClickedButtonDone();
};
