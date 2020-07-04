#pragma once
#include "afxwin.h"


// CDlgWizQuad dialog

class CDlgWizQuad : public CDialog
{
	DECLARE_DYNAMIC(CDlgWizQuad)

public:
	CDlgWizQuad(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgWizQuad();
	void Initialize( CMapStringToPtr * shape_cache_map,
					 CFootLibFolderMap * footlibfoldermap, 
					 BOOL enable_save,
					 CDlgLog * log );
	BOOL MakeFootprint();

// Dialog Data
	enum { IDD = IDD_WIZ_QUAD };


	CShape m_footprint;
	CString m_str_name;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support 

	DECLARE_MESSAGE_MAP()
private:
	enum{ ROUND, SQUARE, SQ1, OCTAGON, RECT, RRECT, OVAL, NONE };
	enum{ TOP_CENTER, BOTTOM_LEFT, TOP_LEFT };
	enum{ SIP, DIP, QUAD, HDR1, HDR2, BGA, EDGE, EDGE2 };
	CMapStringToPtr * m_footprint_cache_map;
	CFootLibFolderMap * m_footlibfoldermap;
	void FormatDimensionString( CString * str, int dim, int units );
	void ReviseDependentVariables();
	BOOL m_enable_save;
	int m_type;
	int m_shape;
	int m_mult;		// multiplication factor (nm per unit)
	int m_pin1;		// position of pin 1
	int m_npins;	// total number of pins
	int m_hpins;	// pins per horiz. row
	int m_vpins;	// pins per vert. row
	int m_hd;	// hole diameter (0 for SMT)
	int m_x;	// all dimensions are in nanometers
	int m_y;
	int m_r;
	int m_e;
	int m_c1;
	int m_g1;
	int m_z1;
	int m_c2;
	int m_g2;
	int m_z2;
	CStatic m_bmp_quad;
	CBitmap m_bmp;
	CButton m_radio_C1;
	CButton m_radio_G1;
	CButton m_radio_Z1;
	CButton m_radio_C2;
	CButton m_radio_G2;
	CButton m_radio_Z2;
	CEdit m_edit_C1;
	CEdit m_edit_G1;
	CEdit m_edit_Z1;
	CEdit m_edit_C2;
	CEdit m_edit_G2;
	CEdit m_edit_Z2;
	CEdit m_edit_E;
	CComboBox m_combo_shape;
	CComboBox m_combo_units;
	CComboBox m_combo_type;
	CComboBox m_combo_pin1;
	CEdit m_edit_y;
	CEdit m_edit_hd;
	CEdit m_edit_x;
	CEdit m_edit_npins;
	CEdit m_edit_hpins;
	CEdit m_edit_vpins;
	CEdit m_edit_name;
	CEdit m_edit_radius;
	CButton m_button_save;
	CButton m_button_exit;
	CDlgLog * m_dlg_log;
	afx_msg void OnBnClickedRadioC1();
	afx_msg void OnBnClickedRadioG1();
	afx_msg void OnBnClickedRadioZ1();
	afx_msg void OnBnClickedRadioC2();
	afx_msg void OnBnClickedRadioG2();
	afx_msg void OnBnClickedRadioZ2();
	afx_msg void OnCbnSelchangeComboWizShape();
	afx_msg void OnEnChangeEditWizNpins();
	afx_msg void OnEnChangeEditWizHpins();
	afx_msg void OnEnChangeEditWizX();
	afx_msg void OnEnChangeEditWizY();
	afx_msg void OnEnChangeEditWizR();
	afx_msg void OnEnChangeEditE();
	afx_msg void OnEnChangeEditC1();
	afx_msg void OnEnChangeEditG1();
	afx_msg void OnEnChangeEditZ1();
	afx_msg void OnEnChangeEditC2();
	afx_msg void OnEnChangeEditG2();
	afx_msg void OnEnChangeEditZ2();
	afx_msg void OnCbnSelchangeComboWizType();
	afx_msg void OnCbnSelchangeComboWizUnits();
	afx_msg void OnBnClickedButtonSave();
	afx_msg void OnCbnSelchangeComboPin1();
	afx_msg void OnEnChangeEditWizHd();
	afx_msg void OnBnClickedWizButtonExit();
	CButton m_button_preview;
	afx_msg void OnBnClickedButton2();
	CStatic m_preview;
public:
	int m_units;	// dimensional units (MM or MIL)
};
