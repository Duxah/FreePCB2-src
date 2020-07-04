#pragma once
#include "afxwin.h"
#include "PartList.h"
#include "resource.h"

// CDlgValueText dialog

class CDlgValueText : public CDialog
{
	DECLARE_DYNAMIC(CDlgValueText)

public:
	CDlgValueText(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgValueText();
	void Initialize( CPartList * plist, cpart * part );
	void GetFields();
	void SetFields();

// Dialog Data
	enum { IDD = IDD_VALUE_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_vis;
	BOOL m_def_pos;
	int m_units;
	int m_height;
	int m_width;
	int m_def_width;
	CPartList * m_plist;
	cpart * m_part;
	CMapStringToPtr * m_footprint_cache_map;
	CEdit m_edit_ref_des;
	CEdit m_edit_value;
	CEdit m_edit_height;
	CButton m_radio_set;
	CButton m_radio_def;
	CEdit m_edit_width;
	CEdit m_edit_def_width;
	afx_msg void OnBnClickedRadioSet();
	afx_msg void OnEnChangeEditCharHeight();
	afx_msg void OnBnClickedRadioDef();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboRefTextUnits();
	CButton m_check_visible;
	CButton m_check_def_pos;
};
