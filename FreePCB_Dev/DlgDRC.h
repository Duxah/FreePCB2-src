#pragma once
#include "stdafx.h"
#include "DesignRules.h"
#include "afxwin.h"

// DlgDRC dialog

class DlgDRC : public CDialog
{
	DECLARE_DYNAMIC(DlgDRC)

public:
	DlgDRC(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgDRC(); 
	void Initialize( int units, 
		DesignRules * dr, 
		CPartList * pl, 
		CNetList * nl, 
		CTextList * tl,
		DRErrorList * drelist,
		int copper_layers, 
		CArray<CPolyLine> * outline_poly,
		int CAM_annular_ring_pins,
		int CAM_annular_ring_vias,
		CDlgLog * log, CString file_name );
	void GetFields();
	void SetFields();
	void CheckDesign();

// Dialog Data
	enum { IDD = IDD_DRC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_units; 
	int m_repeat_drc;
	DesignRules m_dr_local;
	DesignRules * m_dr;
	CPartList * m_pl;
	CNetList * m_nl;
	CTextList * m_tl;
	CDlgLog * m_dlg_log;
	DRErrorList * m_drelist;
	int m_copper_layers;
	CArray<CPolyLine> * m_outline_poly;
	int m_CAM_annular_ring_pins;
	int m_CAM_annular_ring_vias;
	CComboBox m_combo_units;
	CEdit m_edit_pad_pad;
	CEdit m_edit_pad_trace;
	CEdit m_edit_trace_trace;
	CEdit m_edit_hole_copper;
	CEdit m_edit_annular_ring_pins;
	CEdit m_edit_board_edge_copper;
	afx_msg void OnCbnChangeUnits();
	CEdit m_edit_hole_hole;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	CEdit m_edit_annular_ring_vias;
	CEdit m_edit_copper_copper;
	CEdit m_edit_trace_width;
	CEdit m_edit_board_edge_hole;
	afx_msg void OnBnClickedCheck();
	CButton m_check_show_unrouted;
	CString m_f_n;
};
