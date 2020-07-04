#pragma once
#include "afxwin.h"


// CDlgDupFootprintName dialog

class CDlgDupFootprintName : public CDialog
{
	DECLARE_DYNAMIC(CDlgDupFootprintName)

public:
	CDlgDupFootprintName(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgDupFootprintName();
	void Initialize( CString * message, 
						CMapStringToPtr * shape_cache_map );
	BOOL GetReplaceAllFlag(){ return m_replace_all_flag; };
	CString * GetNewName(){ return &m_new_name_str; };

// Dialog Data
	enum { IDD = IDD_DUP_FP_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CMapStringToPtr * m_footprint_cache_map;
	BOOL m_replace_all_flag;
	CString m_new_name_str;
	CString * m_str;
	CEdit m_edit_message;
	CButton m_radio_replace_all;
	CButton m_radio_replace_this;
	CEdit m_edit_new_name;
	afx_msg void OnBnClickedRadioReplaceThis();
	afx_msg void OnBnClickedRadioReplaceAll();
	afx_msg void OnBnClickedOk();
};
