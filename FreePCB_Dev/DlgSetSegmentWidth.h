#pragma once
#include "afxwin.h"
#include "afxtempl.h"


// DlgSetSegmentWidth dialog

class DlgSetSegmentWidth : public CDialog
{
	DECLARE_DYNAMIC(DlgSetSegmentWidth)

public:
	DlgSetSegmentWidth(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgSetSegmentWidth();

// Dialog Data
	enum { IDD = IDD_SEGMENT_WIDTH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// these variables should be set on entry
	int m_mode;		// 0=segment, 1=connection, 2=net
	int m_init_w;	// initial value for width
	int m_init_via_w;	// initial value for via_w
	int m_init_via_hole_w;	// initial value for via_hole_w
	CArray<int> *m_w;	// array of default widths
	CArray<int> *m_v_w;	// array of via widths (matching m_w[] entries)
	CArray<int> *m_v_h_w;	// array of via hole widths

	// these variables will be set on exit
	int m_tv;		// 1=traces and vias, 2=traces only, 3=vias only
	int m_def;		// set default width (1=con, 2=net)
	int m_apply;	// apply width (1=seg, 2=con, 3=net)
	int m_width;	// trace width
	int m_via_width;	// trace width
	int m_hole_width;	// trace width
	int m_units;

	afx_msg void OnBnClickedDefNet();
	afx_msg void OnBnClickedApplyNet();
	afx_msg void OnBnClickedApplyCon();
	afx_msg void OnBnClickedApplySeg();
	afx_msg void OnBnClickedRadioDefVia();
	afx_msg void OnBnClickedRadioSetVia();
	virtual BOOL OnInitDialog();
	CComboBox m_width_box;
	CButton m_def_net;
	CButton m_apply_net;
	CButton m_apply_con;
	CButton m_apply_seg;
	CString m_width_str;
	CString m_via_w_str;
	CString m_via_hole_w_str;
	CEdit m_via_w;
	CEdit m_via_hole_w;
	CButton rb_set_via;
	CButton rb_def_via;
	afx_msg void OnCbnSelchangeComboWidth();
	afx_msg void OnCbnEditchangeComboWidth();
	CButton m_radio_mod_tv;
	CButton m_radio_mod_t;
	CButton m_radio_mod_v;
	afx_msg void OnBnClickedRadioModTv();
	afx_msg void OnBnClickedRadioModT();
	afx_msg void OnBnClickedRadioModV();
	afx_msg void OnBnClickedRadioModify();
};
